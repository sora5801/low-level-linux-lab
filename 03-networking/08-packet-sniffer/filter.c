/* ===========================================================================
 * filter.c — compile "tcp port 80" / "host 1.2.3.4" into classic BPF byte code.
 * ===========================================================================
 *
 * WHAT CLASSIC BPF IS (the mental model you need to read this file)
 * ----------------------------------------------------------------
 * A cBPF program is an array of fixed 8-byte instructions:
 *
 *     struct sock_filter { u16 code; u8 jt; u8 jf; u32 k; };
 *
 * running on a tiny register machine with:
 *   - A   : the 32-bit accumulator (everything computes through it)
 *   - X   : a 32-bit index register (used for variable offsets)
 *   - M[] : 16 words of scratch memory
 *   - an implicit input "packet" P[] of `len` bytes.
 *
 * `code` is a bitfield: the low 3 bits are the instruction CLASS (LD, LDX, ALU,
 * JMP, RET, ...) and the higher bits pick the size / addressing mode / operator.
 * The classic dispatch is exactly the switch we extract into asm/demo.c.
 *
 * Two facts drive ALL of the codegen below:
 *
 *   1. LOADS ARE BIG-ENDIAN. `ldh [12]` reads a 16-bit value in NETWORK byte
 *      order (most-significant byte first) straight out of the packet. That is
 *      why we compare against host constants like 0x0800 directly: the VM has
 *      already done the ntohs() for us. Packets on the wire are big-endian
 *      because that is what every IETF protocol chose (RFC 791 & friends), so a
 *      machine reading a header off the wire always reads MSB-first.
 *
 *   2. JUMPS ARE FORWARD-ONLY, RELATIVE, 8-BIT. A conditional (BPF_JMP) carries
 *      TWO offsets: `jt` (taken when the test is true) and `jf` (false). Each is
 *      the number of instructions to SKIP, measured from the instruction *after*
 *      the jump, in the range 0..255. There is no backward jump in cBPF — that
 *      is the structural reason a cBPF program always terminates (no loops), so
 *      the kernel can run untrusted user code on every packet without a halting
 *      problem. Our compiler exploits this: every jump target is a label that
 *      appears LATER in the program, so every offset is non-negative.
 *
 * OUR TARGET LINK LAYER is Ethernet (DLT_EN10MB): a 14-byte header
 *   [ dst MAC :6 ][ src MAC :6 ][ ethertype :2 ].
 * So the IPv4 header starts at byte 14, its `protocol` field at 14+9 = 23, the
 * source IP at 14+12 = 26, the destination IP at 14+16 = 30. Those magic
 * numbers below are all "14 + offset-within-header".
 * ===========================================================================
 */

#include "filter.h"

#include <stdio.h>          /* FILE, fprintf — only for filter_dump()         */
#include <string.h>         /* strtok_r, strcmp, strlen                       */
#include <stdlib.h>         /* strtoul                                        */
#include <arpa/inet.h>      /* inet_pton — parse "1.2.3.4" into a u32         */
#include <stdint.h>

/* --- Ethernet / IPv4 header offsets, as absolute byte positions ----------- */
#define OFF_ETHERTYPE   12          /* ethertype field in the Ethernet header  */
#define ETH_HLEN        14          /* Ethernet header length                  */
#define OFF_IP_PROTO    (ETH_HLEN + 9)   /* IPv4 `protocol` byte  -> 23         */
#define OFF_IP_SRC      (ETH_HLEN + 12)  /* IPv4 source address   -> 26         */
#define OFF_IP_DST      (ETH_HLEN + 16)  /* IPv4 dest   address   -> 30         */
#define OFF_IP_FLAGS    (ETH_HLEN + 6)   /* IPv4 flags+frag-offset -> 20        */

/* EtherType values (what protocol rides inside the Ethernet frame). */
#define ETYPE_IP        0x0800
#define ETYPE_ARP       0x0806

/* IPv4 `protocol` field values (RFC 790 assigned numbers). */
#define IPPROTO_ICMP_   1
#define IPPROTO_TCP_    6
#define IPPROTO_UDP_    17

/* ---------------------------------------------------------------------------
 * A tiny label-based assembler.
 *
 * cBPF jump offsets are concrete little numbers, but computing them by hand is
 * error-prone. So we emit instructions that reference symbolic LABELS, remember
 * where each label lands, and back-patch the real offsets in one final pass.
 * This is exactly how a real assembler resolves forward references.
 *
 * Label ids: two are reserved for the program's two terminal instructions,
 * which we always append last:
 *     L_ACCEPT — `ret #SNAPLEN`  (keep the packet)
 *     L_REJECT — `ret #0`        (drop the packet)
 * The value L_NEXT means "fall through to the very next instruction" (offset 0)
 * and needs no fixup.
 * --------------------------------------------------------------------------- */
enum { L_ACCEPT = 0, L_REJECT = 1, L_FIRST_USER = 2 };
#define L_NEXT (-1)
#define MAX_LABELS 16

struct asm_ctx {
    struct sock_filter *prog;   /* output instruction array (caller-owned)     */
    int                 n;      /* number emitted so far                       */
    int                 cap;    /* capacity of prog[]                          */

    int lbl_pos[MAX_LABELS];    /* lbl_pos[id] = instruction index it binds to */
    int nlabels;                /* next free user label id                     */

    /* Deferred jump fixups: "instruction `insn`, field `is_jt`, wants label". */
    struct { int insn; int is_jt; int lbl; } fix[FILTER_MAX_INSNS * 2];
    int nfix;

    int error;                  /* sticky: set if we ever overflow prog[]      */
};

/* Emit a non-jump instruction (BPF_STMT-style: no jt/jf). Returns its index. */
static int emit(struct asm_ctx *c, uint16_t code, uint32_t k)
{
    if (c->n >= c->cap) { c->error = 1; return -1; }   /* refuse to overflow  */
    c->prog[c->n].code = code;
    c->prog[c->n].jt   = 0;
    c->prog[c->n].jf   = 0;
    c->prog[c->n].k    = k;
    return c->n++;
}

/* Record that instruction `insn`'s jt (is_jt=1) or jf (is_jt=0) field should
 * become the relative offset to label `lbl`, once we know where `lbl` lands. */
static void want_jump(struct asm_ctx *c, int insn, int is_jt, int lbl)
{
    if (lbl == L_NEXT) return;                 /* offset 0, nothing to patch   */
    c->fix[c->nfix].insn  = insn;
    c->fix[c->nfix].is_jt = is_jt;
    c->fix[c->nfix].lbl   = lbl;
    c->nfix++;
}

/* Emit a conditional jump. `jt_lbl`/`jf_lbl` are labels (or L_NEXT). */
static int emit_jmp(struct asm_ctx *c, uint16_t code, uint32_t k,
                    int jt_lbl, int jf_lbl)
{
    int i = emit(c, code, k);
    if (i < 0) return -1;
    want_jump(c, i, 1, jt_lbl);
    want_jump(c, i, 0, jf_lbl);
    return i;
}

/* Allocate a fresh forward label; bind it later with bind_label(). */
static int new_label(struct asm_ctx *c)
{
    if (c->nlabels >= MAX_LABELS) { c->error = 1; return L_REJECT; }
    return c->nlabels++;
}

/* Bind `lbl` to the NEXT instruction to be emitted (i.e. current position). */
static void bind_label(struct asm_ctx *c, int lbl)
{
    c->lbl_pos[lbl] = c->n;
}

/* Final pass: turn every deferred label reference into a concrete 8-bit offset.
 * off = target_index - (jump_index + 1), measured in instructions. Because all
 * targets are forward, off >= 0; we still assert the 0..255 range the encoding
 * allows. Returns 0 on success, -1 if any offset does not fit (program too big).*/
static int resolve(struct asm_ctx *c)
{
    for (int i = 0; i < c->nfix; i++) {
        int tgt = c->lbl_pos[c->fix[i].lbl];
        int off = tgt - (c->fix[i].insn + 1);
        if (off < 0 || off > 255) return -1;   /* would not fit in a u8 field  */
        if (c->fix[i].is_jt) c->prog[c->fix[i].insn].jt = (uint8_t)off;
        else                 c->prog[c->fix[i].insn].jf = (uint8_t)off;
    }
    return 0;
}

/* ---------------------------------------------------------------------------
 * The parsed shape of an expression.
 *
 * Our grammar (a deliberate subset of pcap's) is:
 *
 *     expr := [ proto ] [ dir ] [ primitive ]
 *     proto := "ip" | "arp" | "tcp" | "udp" | "icmp"
 *     dir   := "src" | "dst"
 *     primitive := "host" IPv4  |  "port" NUMBER
 *
 * Examples that compile: "" (all), "tcp", "udp port 53", "host 1.2.3.4",
 * "src host 10.0.0.1", "tcp dst port 80", "icmp".
 *
 * We parse into these fields, then a single codegen function lays out the BPF.
 * We intentionally support at most ONE primitive (host OR port) plus a proto —
 * that keeps the jump wiring linear and covers every example in the spec.
 * --------------------------------------------------------------------------- */
enum proto_kind { P_ANY, P_IP, P_ARP, P_TCP, P_UDP, P_ICMP };
enum dir_kind   { D_EITHER, D_SRC, D_DST };
enum prim_kind  { PRIM_NONE, PRIM_HOST, PRIM_PORT };

struct parsed {
    enum proto_kind proto;
    enum dir_kind   dir;
    enum prim_kind  prim;
    uint32_t        host;   /* network-order IPv4 for PRIM_HOST                */
    uint16_t        port;   /* host-order port number for PRIM_PORT            */
};

/* Small helper: does token `t` match keyword `k`? */
static int is(const char *t, const char *k) { return strcmp(t, k) == 0; }

/* Parse the expression string. Returns 0 ok, -1 on error (fills errbuf). */
static int parse(const char *expr, struct parsed *out, char *errbuf, size_t errlen)
{
    out->proto = P_ANY; out->dir = D_EITHER; out->prim = PRIM_NONE;

    /* Empty / whitespace-only => "match all". We copy into a mutable buffer
     * because strtok_r writes NULs into the string as it tokenizes. */
    char buf[256];
    if (!expr) return 0;
    if (strlen(expr) >= sizeof buf) {
        snprintf(errbuf, errlen, "filter expression too long");
        return -1;
    }
    strcpy(buf, expr);

    char *save = NULL;
    char *tok = strtok_r(buf, " \t", &save);
    if (!tok) return 0;                          /* only whitespace => all     */

    /* [ proto ] --------------------------------------------------------- */
    if      (is(tok, "ip"))   { out->proto = P_IP;   tok = strtok_r(NULL, " \t", &save); }
    else if (is(tok, "arp"))  { out->proto = P_ARP;  tok = strtok_r(NULL, " \t", &save); }
    else if (is(tok, "tcp"))  { out->proto = P_TCP;  tok = strtok_r(NULL, " \t", &save); }
    else if (is(tok, "udp"))  { out->proto = P_UDP;  tok = strtok_r(NULL, " \t", &save); }
    else if (is(tok, "icmp")) { out->proto = P_ICMP; tok = strtok_r(NULL, " \t", &save); }

    if (!tok) return 0;                          /* proto with no primitive    */

    /* [ dir ] ----------------------------------------------------------- */
    if      (is(tok, "src")) { out->dir = D_SRC; tok = strtok_r(NULL, " \t", &save); }
    else if (is(tok, "dst")) { out->dir = D_DST; tok = strtok_r(NULL, " \t", &save); }

    if (!tok) {
        snprintf(errbuf, errlen, "expected 'host' or 'port' after direction");
        return -1;
    }

    /* [ primitive ] ----------------------------------------------------- */
    if (is(tok, "host")) {
        tok = strtok_r(NULL, " \t", &save);
        if (!tok) { snprintf(errbuf, errlen, "'host' needs an IPv4 address"); return -1; }
        struct in_addr a;
        /* inet_pton writes the address in NETWORK byte order, which is exactly
         * how it sits in the packet — so we can compare the BPF-loaded word
         * against a.s_addr with no swap. */
        if (inet_pton(AF_INET, tok, &a) != 1) {
            snprintf(errbuf, errlen, "bad IPv4 address: %s", tok);
            return -1;
        }
        out->prim = PRIM_HOST;
        out->host = a.s_addr;                    /* already network order      */
    } else if (is(tok, "port")) {
        tok = strtok_r(NULL, " \t", &save);
        if (!tok) { snprintf(errbuf, errlen, "'port' needs a number"); return -1; }
        char *end = NULL;
        unsigned long v = strtoul(tok, &end, 10);
        if (*end != '\0' || v > 65535) {
            snprintf(errbuf, errlen, "bad port: %s", tok);
            return -1;
        }
        out->prim = PRIM_PORT;
        out->port = (uint16_t)v;
    } else {
        snprintf(errbuf, errlen, "unexpected token: %s", tok);
        return -1;
    }

    /* Any trailing tokens mean we don't understand the (richer) expression. */
    if (strtok_r(NULL, " \t", &save)) {
        snprintf(errbuf, errlen, "trailing tokens (only one primitive supported)");
        return -1;
    }
    return 0;
}

/* ---------------------------------------------------------------------------
 * Codegen. Given the parsed spec, lay out the BPF program.
 *
 * The overall shape is a straight-line sequence of GATES. Each gate tests one
 * thing and, on failure, jumps to L_REJECT; on success it falls through to the
 * next gate. The final primitive jumps to L_ACCEPT on a match. After all gates
 * we append the two terminals. Because every jump goes forward to a gate we
 * emit later (or to the terminals), offsets are always non-negative.
 * --------------------------------------------------------------------------- */
static int codegen(struct asm_ctx *c, const struct parsed *p)
{
    int need_ipv4 = (p->proto == P_IP || p->proto == P_TCP ||
                     p->proto == P_UDP || p->proto == P_ICMP ||
                     p->prim == PRIM_HOST || p->prim == PRIM_PORT);

    /* -- ARP is a dead-end special case: match ethertype and accept -------- */
    if (p->proto == P_ARP) {
        /* ldh [12]  — load the ethertype (big-endian 16-bit). */
        emit(c, BPF_LD | BPF_H | BPF_ABS, OFF_ETHERTYPE);
        /* jeq #0x0806 ? accept : reject */
        emit_jmp(c, BPF_JMP | BPF_JEQ | BPF_K, ETYPE_ARP, L_ACCEPT, L_REJECT);
        return 0;
    }

    /* -- IPv4 gate: everything else rides on IPv4 -------------------------- */
    if (need_ipv4) {
        emit(c, BPF_LD | BPF_H | BPF_ABS, OFF_ETHERTYPE);          /* ldh [12] */
        /* If NOT IPv4, reject immediately; else fall through. jf=L_REJECT,
         * jt=L_NEXT (offset 0) continues to the next gate. */
        emit_jmp(c, BPF_JMP | BPF_JEQ | BPF_K, ETYPE_IP, L_NEXT, L_REJECT);
    }

    /* -- L4 protocol gate: tcp / udp / icmp ------------------------------- */
    if (p->proto == P_TCP || p->proto == P_UDP || p->proto == P_ICMP) {
        uint32_t ipproto = (p->proto == P_TCP) ? IPPROTO_TCP_ :
                           (p->proto == P_UDP) ? IPPROTO_UDP_ : IPPROTO_ICMP_;
        /* ldb [23]  — the IPv4 `protocol` byte. */
        emit(c, BPF_LD | BPF_B | BPF_ABS, OFF_IP_PROTO);
        emit_jmp(c, BPF_JMP | BPF_JEQ | BPF_K, ipproto, L_NEXT, L_REJECT);
    }

    /* -- primitive: host --------------------------------------------------- */
    if (p->prim == PRIM_HOST) {
        /* host constant is in network order; the BPF word load is big-endian,
         * so we must compare against the address as a HOST-order u32. On the
         * wire and after `ld [26]` the bytes are MSB-first, and BPF hands us
         * that as an integer whose numeric value equals ntohl(addr). So we
         * compare against ntohl(host). */
        uint32_t k = ntohl(p->host);
        if (p->dir != D_DST) {
            emit(c, BPF_LD | BPF_W | BPF_ABS, OFF_IP_SRC);        /* ld [26]  */
            /* src match -> accept. If also allowed to match dst, a miss falls
             * through to the dst check; otherwise a miss rejects. */
            emit_jmp(c, BPF_JMP | BPF_JEQ | BPF_K, k, L_ACCEPT,
                     (p->dir == D_EITHER) ? L_NEXT : L_REJECT);
        }
        if (p->dir != D_SRC) {
            emit(c, BPF_LD | BPF_W | BPF_ABS, OFF_IP_DST);        /* ld [30]  */
            emit_jmp(c, BPF_JMP | BPF_JEQ | BPF_K, k, L_ACCEPT, L_REJECT);
        }
        return 0;
    }

    /* -- primitive: port --------------------------------------------------- */
    if (p->prim == PRIM_PORT) {
        /* If the user didn't fix a proto, a bare "port N" means TCP *or* UDP.
         * We insert a small either-or gate before the port comparison. */
        if (p->proto != P_TCP && p->proto != P_UDP) {
            int l_ports = new_label(c);
            emit(c, BPF_LD | BPF_B | BPF_ABS, OFF_IP_PROTO);      /* ldb [23] */
            /* tcp? -> ports. else check udp. */
            emit_jmp(c, BPF_JMP | BPF_JEQ | BPF_K, IPPROTO_TCP_, l_ports, L_NEXT);
            /* udp? -> ports. else reject (not a ported protocol). */
            emit_jmp(c, BPF_JMP | BPF_JEQ | BPF_K, IPPROTO_UDP_, l_ports, L_REJECT);
            bind_label(c, l_ports);
        }

        /* FRAGMENT GUARD. L4 port fields live only in the FIRST IP fragment.
         * The low 13 bits of the flags/frag-offset word are the fragment
         * offset; if any are set this is a trailing fragment with no TCP/UDP
         * header, so we cannot read a port — reject it.
         *   ldh [20]; jset #0x1fff -> reject : continue */
        emit(c, BPF_LD | BPF_H | BPF_ABS, OFF_IP_FLAGS);
        emit_jmp(c, BPF_JMP | BPF_JSET | BPF_K, 0x1fff, L_REJECT, L_NEXT);

        /* VARIABLE IP HEADER LENGTH. IPv4 options make the header 20..60 bytes.
         * The classic idiom `ldx 4*([14]&0xf)` uses the special BPF_MSH mode:
         * load byte P[14], mask low nibble (IHL, in 32-bit words), multiply by
         * 4 -> header length in bytes, into X. Then indexed loads read L4 fields
         * at [X + ETH_HLEN + field]. */
        emit(c, BPF_LDX | BPF_B | BPF_MSH, ETH_HLEN);            /* X = IP hlen */

        /* src port = [X + 14 + 0], dst port = [X + 14 + 2], both big-endian. */
        if (p->dir != D_DST) {
            emit(c, BPF_LD | BPF_H | BPF_IND, ETH_HLEN + 0);     /* ldh [x+14] */
            emit_jmp(c, BPF_JMP | BPF_JEQ | BPF_K, p->port, L_ACCEPT,
                     (p->dir == D_EITHER) ? L_NEXT : L_REJECT);
        }
        if (p->dir != D_SRC) {
            emit(c, BPF_LD | BPF_H | BPF_IND, ETH_HLEN + 2);     /* ldh [x+16] */
            emit_jmp(c, BPF_JMP | BPF_JEQ | BPF_K, p->port, L_ACCEPT, L_REJECT);
        }
        return 0;
    }

    /* No primitive: the proto gate(s) alone decide. Falling off the last gate
     * means every gate passed, so accept. We emit nothing here; the trailing
     * L_ACCEPT (appended by the caller) is the fall-through target. But if there
     * were NO gates at all (proto==ANY, prim==NONE) the caller handles the
     * "match all" case before calling us, so we never reach here empty. */
    return 0;
}

int filter_compile(const char *expr, struct compiled_filter *out,
                   char *errbuf, size_t errlen)
{
    /* Default the error buffer so callers can always print something. */
    if (errlen) errbuf[0] = '\0';

    struct parsed p;
    if (parse(expr, &p, errbuf, errlen) != 0)
        return -1;

    /* "match all": no proto, no primitive -> emit zero instructions and let the
     * caller skip SO_ATTACH_FILTER entirely (cheaper than a one-line accept). */
    if (p.proto == P_ANY && p.prim == PRIM_NONE) {
        out->len = 0;
        return 0;
    }

    struct asm_ctx c;
    memset(&c, 0, sizeof c);
    c.prog = out->prog;
    c.cap  = FILTER_MAX_INSNS;
    c.nlabels = L_FIRST_USER;
    for (int i = 0; i < MAX_LABELS; i++) c.lbl_pos[i] = -1;

    if (codegen(&c, &p) != 0 || c.error) {
        snprintf(errbuf, errlen, "filter too complex for %d instructions",
                 FILTER_MAX_INSNS);
        return -1;
    }

    /* Append the two terminal instructions and bind their labels HERE, so the
     * back-patcher can compute offsets to them. Order matters: accept first. */
    bind_label(&c, L_ACCEPT);
    emit(&c, BPF_RET | BPF_K, FILTER_SNAPLEN);   /* ret #snaplen -> keep packet */
    bind_label(&c, L_REJECT);
    emit(&c, BPF_RET | BPF_K, 0);                /* ret #0       -> drop packet  */

    if (c.error || resolve(&c) != 0) {
        snprintf(errbuf, errlen, "jump offset out of range (program too large)");
        return -1;
    }

    out->len = (unsigned short)c.n;
    return 0;
}

/* ---------------------------------------------------------------------------
 * filter_dump — print the program the way `tcpdump -d` does. This is a pure
 * teaching aid: it decodes the `code` bitfield back into mnemonics so a reader
 * can eyeball the accept/reject VM. We only decode the opcodes our compiler
 * actually emits; anything else prints as a hex fallback.
 * --------------------------------------------------------------------------- */
void filter_dump(const struct compiled_filter *f, void *vfp)
{
    FILE *fp = (FILE *)vfp;
    for (unsigned i = 0; i < f->len; i++) {
        const struct sock_filter *s = &f->prog[i];
        fprintf(fp, "(%03u) ", i);
        switch (s->code) {
        case BPF_LD | BPF_H | BPF_ABS:
            fprintf(fp, "ldh      [%u]\n", s->k); break;
        case BPF_LD | BPF_B | BPF_ABS:
            fprintf(fp, "ldb      [%u]\n", s->k); break;
        case BPF_LD | BPF_W | BPF_ABS:
            fprintf(fp, "ld       [%u]\n", s->k); break;
        case BPF_LD | BPF_H | BPF_IND:
            fprintf(fp, "ldh      [x + %u]\n", s->k); break;
        case BPF_LDX | BPF_B | BPF_MSH:
            fprintf(fp, "ldxb     4*([%u]&0xf)\n", s->k); break;
        case BPF_JMP | BPF_JEQ | BPF_K:
            fprintf(fp, "jeq      #0x%x   jt %u  jf %u\n",
                    s->k, i + 1 + s->jt, i + 1 + s->jf); break;
        case BPF_JMP | BPF_JSET | BPF_K:
            fprintf(fp, "jset     #0x%x   jt %u  jf %u\n",
                    s->k, i + 1 + s->jt, i + 1 + s->jf); break;
        case BPF_RET | BPF_K:
            fprintf(fp, "ret      #%u\n", s->k); break;
        default:
            fprintf(fp, "{ code=0x%04x k=0x%x jt=%u jf=%u }\n",
                    s->code, s->k, s->jt, s->jf); break;
        }
    }
}
