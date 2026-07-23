// SPDX-License-Identifier: GPL-2.0
/* ===========================================================================
 * netfilter_hook.c — a minimal in-kernel packet firewall / logger.
 * ===========================================================================
 *
 * WHAT THIS IS
 * ------------
 * A loadable kernel module that plants a *Netfilter hook* into the IPv4 stack.
 * Netfilter is the framework iptables/nftables are built on: at five well-known
 * points in the packet's journey through the kernel, the stack calls out to any
 * registered hook function and lets it return a *verdict* — most importantly
 * NF_ACCEPT ("let it continue") or NF_DROP ("silently discard"). We register at
 * two of those points, parse the IPv4 + TCP/UDP headers out of the sk_buff,
 * consult a tiny rule table, log matches (rate-limited), and drop what matches.
 *
 * This is the same machinery a real firewall uses; iptables is essentially a
 * very elaborate, table-driven hook function. Here we keep the hook trivial so
 * the *mechanism* — registration, the sk_buff, the verdict — is the lesson.
 *
 * THE FIVE NETFILTER HOOK POINTS (IPv4), in packet-flow order:
 *
 *      ── incoming ──► [PRE_ROUTING] ──► routing decision
 *                                          │
 *                        ┌─────────────────┴─────────────────┐
 *                        ▼                                    ▼
 *                  destined here                      being forwarded
 *                   [LOCAL_IN]                         [FORWARD]
 *                        │                                    │
 *                    local socket                             ▼
 *                        │                             [POST_ROUTING] ──► out
 *                   [LOCAL_OUT]
 *                        │
 *                  [POST_ROUTING] ──► outgoing
 *
 * We hook PRE_ROUTING (sees EVERY arriving packet, before the routing decision,
 * so it can filter traffic that is merely passing through) and LOCAL_IN (sees
 * only packets whose destination is THIS host, after routing). Watching the two
 * fire is itself instructive: a packet addressed to us trips both; a forwarded
 * packet trips PRE_ROUTING but not LOCAL_IN.
 *
 * WHERE THIS RUNS
 * ---------------
 * This is kernel code. It compiles only against real Linux kernel headers and
 * loads only on Linux. Do NOT test it on your daily-driver machine — a bad hook
 * can wedge your network. Build and run it inside a throwaway QEMU/KVM VM (the
 * README shows how). It cannot be built on this Windows host; see asm/demo.c for
 * the standalone teaching extract that CAN be compiled to assembly here.
 * ===========================================================================
 */

#include <linux/module.h>          /* module_init/exit, MODULE_* macros        */
#include <linux/kernel.h>          /* pr_info / pr_err and friends             */
#include <linux/init.h>            /* __init / __exit section annotations      */
#include <linux/moduleparam.h>     /* module_param — runtime-tunable knobs     */

#include <linux/netfilter.h>       /* nf_hook_ops, NF_ACCEPT/NF_DROP verdicts  */
#include <linux/netfilter_ipv4.h>  /* NF_INET_PRE_ROUTING, NF_IP_PRI_* prios   */

#include <linux/skbuff.h>          /* struct sk_buff, skb_header_pointer()     */
#include <linux/ip.h>              /* struct iphdr, ip_hdr()                    */
#include <linux/tcp.h>             /* struct tcphdr                            */
#include <linux/udp.h>             /* struct udphdr                            */
#include <linux/in.h>              /* IPPROTO_TCP / IPPROTO_UDP                 */
#include <net/net_namespace.h>     /* init_net — the initial network namespace */

MODULE_LICENSE("GPL");             /* GPL: required to use the EXPORT_SYMBOL_GPL
                                    * netfilter registration functions below.  */
MODULE_AUTHOR("low-level-linux-lab");
MODULE_DESCRIPTION("Minimal Netfilter IPv4 firewall/logger (teaching core)");
MODULE_VERSION("1.0");

/* ---------------------------------------------------------------------------
 * Tunable knobs (module parameters).
 *
 * module_param() wires a variable to /sys/module/netfilter_hook/parameters/<n>
 * and to `insmod netfilter_hook.ko drop_tcp_port=2222`. The third arg is the
 * sysfs permission mask: 0644 = world-readable, root-writable, so you can also
 * retune a loaded module with `echo N > /sys/module/.../parameters/drop_tcp_port`.
 *
 * Ports here are in HOST byte order (what a human types). The wire carries them
 * big-endian (network byte order) as __be16; we convert with ntohs() at compare
 * time. Keeping the knob in host order is the least surprising for the operator.
 * --------------------------------------------------------------------------- */
static int drop_tcp_port = 23;     /* default: block inbound telnet (port 23)  */
module_param(drop_tcp_port, int, 0644);
MODULE_PARM_DESC(drop_tcp_port, "TCP destination port to DROP (0 = disable)");

static int drop_udp_port;          /* default 0 => the UDP knob is disabled     */
module_param(drop_udp_port, int, 0644);
MODULE_PARM_DESC(drop_udp_port, "UDP destination port to DROP (0 = disable)");

static bool log_accept;            /* default false: only log DROPs, not ACCEPTs */
module_param(log_accept, bool, 0644);
MODULE_PARM_DESC(log_accept, "Also log accepted packets (very chatty)");

/* ---------------------------------------------------------------------------
 * The rule table.
 *
 * Real firewalls are just a fancy version of this: a list of matches, each with
 * a verdict. Ours is intentionally tiny and static so the data structure never
 * distracts from the hook mechanics. proto==0 or dport==0 mean "wildcard".
 *
 * These particular defaults are chosen to be recognizable: telnet and its IoT
 * variant (Mirai-style botnets scan 23/2323), and SSDP (UDP 1900), a favourite
 * reflection/amplification vector. The module-parameter ports are consulted
 * separately in fw_verdict() so an operator can add a rule without recompiling.
 * --------------------------------------------------------------------------- */
struct fw_rule {
	u8          proto;   /* IPPROTO_TCP, IPPROTO_UDP, or 0 for "any protocol"  */
	u16         dport;   /* destination port in HOST order, or 0 for "any"     */
	const char *why;     /* human reason, printed when the rule fires          */
};

static const struct fw_rule drop_rules[] = {
	{ IPPROTO_TCP,   23, "telnet (plaintext remote login)"        },
	{ IPPROTO_TCP, 2323, "telnet-alt (Mirai-class IoT scanning)"  },
	{ IPPROTO_UDP, 1900, "SSDP/UPnP (reflection & amplification)" },
};

/* Counters, for the load-bearing "did it actually work?" question. These are
 * plain longs updated from softirq context; on SMP several CPUs can run the
 * hook concurrently, so a truly accurate counter would need per-CPU storage or
 * an atomic. We keep them simple (approximate is fine for a teaching stat) and
 * call out the race here rather than pretend it doesn't exist. */
static unsigned long stat_seen;    /* IPv4 packets the hook inspected           */
static unsigned long stat_dropped; /* packets we returned NF_DROP for           */

/* ---------------------------------------------------------------------------
 * fw_match_port — does (proto, dport) match any rule? Returns the reason, or
 * NULL for "no match". Pure function over the rule table + module params.
 * --------------------------------------------------------------------------- */
static const char *fw_match_port(u8 proto, u16 dport)
{
	size_t i;

	/* The static table first. */
	for (i = 0; i < ARRAY_SIZE(drop_rules); i++) {
		const struct fw_rule *r = &drop_rules[i];

		if (r->proto && r->proto != proto)   /* protocol set and mismatched  */
			continue;
		if (r->dport && r->dport != dport)   /* port set and mismatched      */
			continue;
		return r->why;                       /* wildcards fell through: hit  */
	}

	/* Then the operator-supplied knobs (0 means "knob disabled"). */
	if (proto == IPPROTO_TCP && drop_tcp_port && dport == drop_tcp_port)
		return "drop_tcp_port module parameter";
	if (proto == IPPROTO_UDP && drop_udp_port && dport == drop_udp_port)
		return "drop_udp_port module parameter";

	return NULL;
}

/* ===========================================================================
 * fw_hook_fn — the Netfilter hook itself. THE CENTRE OF THE MODULE.
 *
 * Netfilter calls this for every packet reaching the hook point we registered.
 * The signature is fixed by the framework (see struct nf_hook_ops.hook):
 *
 *   priv  : the .priv pointer we stored at registration (unused here).
 *   skb   : the packet, as a socket buffer. This is THE core kernel networking
 *           object — a descriptor around the packet bytes plus metadata. See
 *           the sk_buff notes at each access below.
 *   state : hook context — which hook fired (state->hook), in/out net devices,
 *           the net namespace, etc.
 *
 * CONTEXT & CONCURRENCY: we run in the receive softirq (bottom half), not in a
 * process. That means: (1) we MUST NOT sleep — no blocking allocations, no
 * mutex_lock, no copy_*_user; (2) the same function can run on every CPU at
 * once for different packets, so any shared writable state needs its own
 * synchronization. Our rule table is read-only (const) so it needs none.
 *
 * RETURN: an NF verdict. The two we use:
 *   NF_ACCEPT — hand the skb back to the stack; continue normal processing.
 *   NF_DROP   — free the skb here; the packet vanishes with no reply (unlike a
 *               REJECT, which would send an ICMP/RST). On NF_DROP netfilter
 *               takes ownership and frees the skb; we must NOT touch it after.
 * ===========================================================================
 */
static unsigned int fw_hook_fn(void *priv,
			       struct sk_buff *skb,
			       const struct nf_hook_state *state)
{
	const struct iphdr *iph;   /* the IPv4 header                              */
	u8   proto;
	u16  dport = 0;            /* destination port, host order (0 if no ports) */
	const char *reason;

	/* A hook can be handed a NULL skb in rare paths; defend against it before
	 * any dereference. Accepting on "nothing to inspect" is the safe default —
	 * a firewall that drops what it cannot parse is a self-inflicted outage. */
	if (!skb)
		return NF_ACCEPT;

	/* ip_hdr(skb) == skb_network_header(skb) reinterpreted as an iphdr. At
	 * PRE_ROUTING/LOCAL_IN the IPv4 receive path has already validated and
	 * pulled the IP header into the skb's *linear* region, so this pointer is
	 * safe to read for the fixed part of the header. We still sanity-check it. */
	iph = ip_hdr(skb);
	if (!iph)
		return NF_ACCEPT;

	/* We only handle IPv4 here (we registered as NFPROTO_IPV4, but belt-and-
	 * braces: a malformed frame could carry a bogus version nibble). */
	if (iph->version != 4)
		return NF_ACCEPT;

	stat_seen++;               /* approximate; see the note at the definition   */
	proto = iph->protocol;

	/* --- Reach the transport header SAFELY ------------------------------- *
	 * The IP header length is iph->ihl (Internet Header Length) measured in
	 * 32-bit words, so the transport header starts at ihl*4 bytes in. But we
	 * cannot simply cast (iph + ihl*4): an sk_buff may be *non-linear* — its
	 * tail can live in page fragments, so the transport header might not be in
	 * the contiguous region at all. Dereferencing a raw pointer there reads
	 * kernel memory that isn't the packet: a bug and a security hole.
	 *
	 * skb_header_pointer() is the idiom that makes this correct: it returns a
	 * pointer to the requested bytes IF they are already linear, otherwise it
	 * copies them into the on-stack scratch buffer we pass and returns that.
	 * Either way we get a valid, readable pointer — or NULL if the packet is
	 * truncated (fewer bytes than a header), which we treat as "accept, not our
	 * problem to parse." This single call is the most important defensive
	 * pattern in sk_buff parsing.
	 * -------------------------------------------------------------------- */
	if (proto == IPPROTO_TCP) {
		struct tcphdr _tcph;   /* scratch buffer skb_header_pointer may fill   */
		const struct tcphdr *th;

		th = skb_header_pointer(skb, iph->ihl * 4, sizeof(_tcph), &_tcph);
		if (!th)
			return NF_ACCEPT;              /* truncated — don't parse garbage */
		dport = ntohs(th->dest);          /* __be16 on the wire -> host order */

	} else if (proto == IPPROTO_UDP) {
		struct udphdr _udph;
		const struct udphdr *uh;

		uh = skb_header_pointer(skb, iph->ihl * 4, sizeof(_udph), &_udph);
		if (!uh)
			return NF_ACCEPT;
		dport = ntohs(uh->dest);

	} else {
		/* ICMP, etc.: no ports to match on. Our rule table only drops on
		 * (proto,port), so anything portless is accepted. Optionally noted. */
		if (log_accept && net_ratelimit())
			pr_info("accept %pI4 -> %pI4 proto=%u (no ports)\n",
				&iph->saddr, &iph->daddr, proto);
		return NF_ACCEPT;
	}

	/* --- Consult the rules --------------------------------------------- */
	reason = fw_match_port(proto, dport);

	if (reason) {
		stat_dropped++;

		/* Rate-limited logging. A hostile flood could hit this hook millions
		 * of times a second; an un-throttled pr_info would turn our "firewall"
		 * into a log-flood self-DoS and could livelock the console. net_ratelimit()
		 * consults the global net.core ratelimit and returns false once we've
		 * exceeded the burst, so at most a handful of lines print per interval.
		 * %pI4 formats a __be32 IPv4 address (pass its ADDRESS, &iph->saddr);
		 * the kernel's vsprintf handles the byte order and dotted-quad. */
		if (net_ratelimit())
			pr_info("DROP %pI4 -> %pI4 %s dport=%u [%s] (hook=%s)\n",
				&iph->saddr, &iph->daddr,
				proto == IPPROTO_TCP ? "TCP" : "UDP",
				dport, reason,
				state->hook == NF_INET_PRE_ROUTING ?
					"PRE_ROUTING" : "LOCAL_IN");

		return NF_DROP;   /* netfilter frees the skb; we must not touch it now */
	}

	if (log_accept && net_ratelimit())
		pr_info("accept %pI4 -> %pI4 %s dport=%u\n",
			&iph->saddr, &iph->daddr,
			proto == IPPROTO_TCP ? "TCP" : "UDP", dport);

	return NF_ACCEPT;
}

/* ---------------------------------------------------------------------------
 * Hook registration descriptors.
 *
 * struct nf_hook_ops tells netfilter WHERE to call us and with what priority:
 *   .hook     — our function.
 *   .pf       — protocol family: NFPROTO_IPV4 (we parse IPv4). A parallel set
 *               with NFPROTO_IPV6 + ipv6_hdr() would cover v6.
 *   .hooknum  — which of the five hook points.
 *   .priority — ordering among multiple hooks at the same point. Lower runs
 *               first. NF_IP_PRI_FIRST puts us ahead of the default iptables
 *               chains, so our verdict is decided before their rules see it.
 *
 * We use two ops structs, one per hook point, sharing the same .hook function.
 * --------------------------------------------------------------------------- */
static struct nf_hook_ops nfho_prerouting = {
	.hook     = fw_hook_fn,
	.pf       = NFPROTO_IPV4,
	.hooknum  = NF_INET_PRE_ROUTING,
	.priority = NF_IP_PRI_FIRST,
};

static struct nf_hook_ops nfho_local_in = {
	.hook     = fw_hook_fn,
	.pf       = NFPROTO_IPV4,
	.hooknum  = NF_INET_LOCAL_IN,
	.priority = NF_IP_PRI_FIRST,
};

/* ---------------------------------------------------------------------------
 * Module lifecycle.
 *
 * nf_register_net_hook(net, ops) installs a hook into ONE network namespace.
 * We use &init_net, the initial namespace — so this filters the host's main
 * stack but NOT containers/netns spun up separately. Filtering every namespace
 * (present and future) is what register_pernet_subsys() is for; see the README
 * "Going further". Binding to init_net keeps the teaching core to one concept.
 *
 * __init/__exit place these in special sections the kernel frees after use.
 * --------------------------------------------------------------------------- */
static int __init nf_hook_init(void)
{
	int ret;

	/* Install PRE_ROUTING first. Every nf_register_net_hook can fail (e.g.
	 * -ENOMEM building the hook array); a module MUST check and unwind, or it
	 * leaves the kernel in a half-registered state. */
	ret = nf_register_net_hook(&init_net, &nfho_prerouting);
	if (ret < 0) {
		pr_err("failed to register PRE_ROUTING hook: %d\n", ret);
		return ret;
	}

	/* Install LOCAL_IN. If THIS one fails we must roll back the first hook,
	 * or unloading later would unregister a hook we never fully owned. This
	 * ordered "register A, then B; if B fails undo A" cleanup is the standard
	 * kernel init discipline. */
	ret = nf_register_net_hook(&init_net, &nfho_local_in);
	if (ret < 0) {
		pr_err("failed to register LOCAL_IN hook: %d\n", ret);
		nf_unregister_net_hook(&init_net, &nfho_prerouting);
		return ret;
	}

	pr_info("loaded: hooking PRE_ROUTING+LOCAL_IN on init_net "
		"(drop TCP:%d UDP:%d)\n", drop_tcp_port, drop_udp_port);
	return 0;   /* 0 => module stays loaded */
}

static void __exit nf_hook_exit(void)
{
	/* Unregister in the REVERSE order of registration. nf_unregister_net_hook
	 * both removes the hook from the chain AND waits (synchronize_net) until no
	 * CPU is still executing inside fw_hook_fn before returning — so once these
	 * calls complete, it is safe for the module text to be unloaded. Skipping
	 * this would let a packet call into freed code: an instant kernel oops. */
	nf_unregister_net_hook(&init_net, &nfho_local_in);
	nf_unregister_net_hook(&init_net, &nfho_prerouting);

	pr_info("unloaded: inspected %lu packets, dropped %lu\n",
		stat_seen, stat_dropped);
}

module_init(nf_hook_init);
module_exit(nf_hook_exit);
