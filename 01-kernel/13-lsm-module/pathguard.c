// SPDX-License-Identifier: GPL-2.0
/* ===========================================================================
 * pathguard.c — a tiny stackable Linux Security Module (LSM).
 * ===========================================================================
 *
 * WHAT THIS IS
 * ------------
 * PathGuard is a minimal Mandatory Access Control (MAC) module written in the
 * modern, *stackable* LSM style. It registers two hooks and enforces a simple
 * path-prefix policy:
 *
 *   1. file_open           — deny unprivileged processes from OPENING files
 *                            under a protected directory (e.g. /root, a secrets
 *                            dir). A read/write denylist.
 *   2. bprm_check_security — deny unprivileged processes from EXECUTING any
 *                            binary that lives OUTSIDE an allowlist of trusted
 *                            directories (/usr, /bin, ...). "No exec from /tmp".
 *
 * Both decisions come down to one pure function pair — pg_path_has_prefix() and
 * pg_policy_lookup() — which is exactly the code extracted to asm/demo.c and
 * annotated in asm/demo.annotated.s. Read that first if you want to see the
 * hottest path of this module as raw x86-64.
 *
 * WHAT AN LSM IS, AND HOW STACKING WORKS  (read this before the code)
 * ------------------------------------------------------------------
 * The Linux Security Module framework is a set of ~250 hook points sprinkled
 * through the kernel at the moments a security decision is meaningful: opening
 * a file, execing a binary, creating a socket, sending a signal, mounting a
 * filesystem, and so on. Each hook point is a call to `call_int_hook(...)`
 * which walks a per-hook list (`struct security_hook_heads`) of registered
 * callbacks. SELinux, AppArmor, Smack, Yama, Landlock, and the always-present
 * "capability" LSM all plug into these same lists.
 *
 * Since Linux ~5.1 the framework is *stackable*: many LSMs coexist, ordered by
 * the boot parameter `lsm=` / config `CONFIG_LSM="landlock,lockdown,yama,...,
 * pathguard"`. For a RESTRICTIVE hook like file_open or bprm_check_security the
 * composition rule is a logical AND: `call_int_hook` walks the list and returns
 * the FIRST non-zero (denying) result. So *every* LSM must agree to allow;
 * *any* LSM can veto. A hook returns 0 to permit and a negative errno (we use
 * -EACCES) to forbid — that errno is what userspace sees from open(2)/execve(2).
 *
 * The two big historical "major" LSMs (SELinux, Smack, AppArmor) set the flag
 * LSM_FLAG_LEGACY_MAJOR and were once mutually exclusive. We deliberately do
 * NOT set that flag: PathGuard is a "minor" module that stacks unconditionally
 * next to whatever major LSM the distro ships.
 *
 * WHY THIS CANNOT BE A LOADABLE .ko  (the honest, important caveat)
 * ----------------------------------------------------------------
 * A real LSM is compiled INTO the kernel image, not loaded with insmod. Two
 * kernel facts force this, and they are the deepest lesson of the project:
 *
 *   (a) security_add_hooks() — the call that registers our hooks — is marked
 *       `__init`. Its code and the surrounding init machinery are FREED once
 *       boot finishes. A module loaded later literally cannot call it.
 *   (b) DEFINE_LSM() places our descriptor in the special `.lsm_info.init`
 *       ELF section, which the framework iterates ONCE, very early in boot
 *       (before the initramfs, before any module could be loaded), to run each
 *       LSM's .init in the order given by CONFIG_LSM.
 *
 * So to actually RUN PathGuard you drop this file into the kernel source tree
 * (security/pathguard/), wire it into Kconfig + Makefile, add "pathguard" to
 * CONFIG_LSM, rebuild the kernel, and boot it — ideally inside a QEMU VM so a
 * buggy exec policy can't lock you out of your real machine. The README spells
 * out every step. The provided Kbuild Makefile still lets you COMPILE this file
 * against your kernel headers (a genuine type-check against the real hook
 * signatures, `struct file`, `struct linux_binprm`, `struct cred`), which is
 * valuable on its own — it just cannot be insmod-ed.
 * ===========================================================================
 */

/* Prefix every pr_info/pr_warn from this file with "PathGuard: ". Must be
 * defined BEFORE including printk.h (pulled in by module.h). */
#define pr_fmt(fmt) "PathGuard: " fmt

#include <linux/module.h>       /* MODULE_*, module_param                     */
#include <linux/kernel.h>       /* pr_*, ARRAY_SIZE                           */
#include <linux/init.h>         /* __init                                     */
#include <linux/version.h>      /* LINUX_VERSION_CODE, KERNEL_VERSION         */
#include <linux/lsm_hooks.h>    /* security_hook_list, LSM_HOOK_INIT,         */
                                /*   security_add_hooks, DEFINE_LSM, lsm_id   */
#include <linux/cred.h>         /* current_cred, kuid_t, uid_eq,              */
                                /*   GLOBAL_ROOT_UID                          */
#include <linux/fs.h>           /* struct file, struct path                  */
#include <linux/binfmts.h>      /* struct linux_binprm                       */
#include <linux/dcache.h>       /* d_path()                                  */
#include <linux/gfp.h>          /* __get_free_page, GFP_KERNEL               */
#include <linux/mm.h>           /* free_page                                 */
#include <linux/err.h>          /* IS_ERR, PTR_ERR                           */
#include <linux/types.h>        /* size_t                                    */
#include <linux/uidgid.h>       /* kuid_t helpers (also via cred.h)          */

/* ---------------------------------------------------------------------------
 * Runtime knob: enforcing vs permissive.
 *
 * When built into the kernel, module_param exposes this as the boot parameter
 * `pathguard.enforce=0`, and (mode 0644) as the writable sysfs file
 * /sys/module/pathguard/parameters/enforce. Permissive mode LOGS what it would
 * have blocked without actually blocking — indispensable when bringing up a new
 * policy so you can watch for false positives before turning on enforcement.
 * SELinux's "permissive" mode is the same idea.
 * --------------------------------------------------------------------------- */
static bool pathguard_enforce = true;
module_param_named(enforce, pathguard_enforce, bool, 0644);
MODULE_PARM_DESC(enforce, "1 = block denied operations, 0 = permissive (log only)");

/* ===========================================================================
 * PART 1: the pure policy core.
 *
 * These two functions are byte-for-byte the algorithm in asm/demo.c. They touch
 * no kernel API at all — just characters and a small table — which is why they
 * are the piece we can compile to standalone teaching assembly. Keeping the
 * decision logic isolated and table-driven is also what makes the policy
 * auditable: everything the module forbids is visible in the two arrays below.
 * ===========================================================================
 */

enum pg_verdict {
	PG_ALLOW = 0,   /* permit — hook returns 0                              */
	PG_DENY  = 1,   /* forbid — hook returns -EACCES (when enforcing)       */
};

/* One policy rule: "any path at or under `prefix` gets `verdict`". */
struct pg_rule {
	const char *prefix;   /* absolute path, NO trailing slash               */
	int         verdict;  /* enum pg_verdict                                */
};

/*
 * pg_path_has_prefix — does `path` sit at or under directory `prefix`?
 *
 * A raw strncmp() is the classic wrong answer: prefix "/root" would then also
 * match "/rootkit". Correct path matching requires the byte in `path` right
 * after the matched prefix to be a COMPONENT BOUNDARY: end-of-string (the path
 * *is* the directory) or '/' (the path is something inside it). Precondition:
 * `prefix` carries no trailing slash. See the annotated assembly for the exact
 * `orb` that encodes this boundary rule.
 */
static bool pg_path_has_prefix(const char *path, const char *prefix)
{
	size_t i = 0;

	while (prefix[i] != '\0') {
		if (path[i] != prefix[i])
			return false;   /* diverged before the prefix ended     */
		i++;
	}
	/* Whole prefix matched; require a component boundary in `path`. */
	return path[i] == '\0' || path[i] == '/';
}

/*
 * pg_policy_lookup — first matching rule wins; else the table default.
 *
 * `default_verdict` is the fail-open/fail-closed knob: the exec allowlist passes
 * PG_DENY (unlisted paths are refused), the protected-read denylist passes
 * PG_ALLOW (only listed paths are refused). One function, both policies.
 */
static int pg_policy_lookup(const char *path, const struct pg_rule *rules,
			    int n, int default_verdict)
{
	int i;

	for (i = 0; i < n; i++)
		if (pg_path_has_prefix(path, rules[i].prefix))
			return rules[i].verdict;

	return default_verdict;
}

/* ---------------------------------------------------------------------------
 * The actual policy. Editing security posture = editing these two tables.
 * --------------------------------------------------------------------------- */

/* Protected-read denylist. Unprivileged opens of anything under these prefixes
 * are refused. Table default is PG_ALLOW, so only listed paths are affected. */
static const struct pg_rule pg_protected_rules[] = {
	{ "/etc/pathguard-secret", PG_DENY },  /* a demo secrets directory      */
	{ "/root",                 PG_DENY },  /* root's home, off-limits to users */
};

/* Exec allowlist. Unprivileged execution is permitted ONLY from these trusted
 * directories; the table default is PG_DENY, so a binary in /tmp, /dev/shm, a
 * home directory, etc. cannot be exec'd by a non-root user. This is the "no
 * exec from world-writable places" pattern that stops a whole class of
 * drop-a-payload-and-run attacks — but note (README) that an incomplete
 * allowlist WILL break legitimate programs, which is why you test in a VM. */
static const struct pg_rule pg_exec_rules[] = {
	{ "/usr",  PG_ALLOW },
	{ "/bin",  PG_ALLOW },
	{ "/sbin", PG_ALLOW },
	{ "/lib",  PG_ALLOW },
	{ "/opt",  PG_ALLOW },
};

/* ===========================================================================
 * PART 2: turning a kernel `struct path` into a string, then a verdict.
 * ===========================================================================
 */

/*
 * pathguard_check_path — resolve `path` to text and run it through a table.
 *
 * The subtle part is d_path(). The kernel stores paths as a chain of dentries,
 * not a string; to compare against our text prefixes we must render one. d_path
 * writes the path into `buf` BUILDING IT BACKWARDS FROM THE END, and returns a
 * pointer to wherever inside `buf` the string actually starts — that returned
 * pointer, NOT buf, is the C string. On overflow it returns ERR_PTR(-ENAMETOOLONG),
 * so IS_ERR() must be checked. We own `buf`: we allocate it and we free it.
 *
 * Buffer: __get_free_page() hands back exactly one PAGE_SIZE (4096 on x86-64)
 * region, which equals PATH_MAX — the right size for any path d_path can emit.
 * We use GFP_KERNEL because these hooks run in process context and may sleep;
 * a hook that ran in atomic context (holding a spinlock, in an interrupt) would
 * have to use GFP_ATOMIC or avoid allocation entirely.
 */
static int pathguard_check_path(const struct path *path, const char *op,
				const struct pg_rule *rules, int nrules,
				int default_verdict)
{
	char *buf;
	char *resolved;
	int verdict;

	buf = (char *)__get_free_page(GFP_KERNEL);
	if (!buf) {
		/* Out of memory. We choose to FAIL OPEN (allow) rather than turn
		 * transient memory pressure into a security lockout that could
		 * wedge the system. A hardened deployment might fail closed
		 * instead; that is a real policy tradeoff, so it is a comment,
		 * not a silent default. */
		pr_warn_ratelimited("no memory to evaluate %s; allowing\n", op);
		return 0;
	}

	resolved = d_path(path, buf, PAGE_SIZE);
	if (IS_ERR(resolved)) {
		/* Path too long or unreachable — we could not build a name to
		 * judge, so allow and move on (same fail-open reasoning). */
		free_page((unsigned long)buf);
		return 0;
	}

	/* NOTE: pass `resolved`, the pointer INTO buf, never buf itself. */
	verdict = pg_policy_lookup(resolved, rules, nrules, default_verdict);

	if (verdict == PG_DENY) {
		/* Ratelimited: file_open is one of the hottest paths in the
		 * kernel; an un-throttled pr_warn here could livelock the log.
		 * We print the offending pid/comm/path for the audit trail. */
		pr_warn_ratelimited("deny %s: pid=%d comm=%s path=%s%s\n",
				    op, current->pid, current->comm, resolved,
				    pathguard_enforce ? "" : " (permissive)");
		free_page((unsigned long)buf);
		/* Enforcing -> -EACCES (userspace sees EACCES). Permissive ->
		 * allow, so the operator can observe would-be denials safely. */
		return pathguard_enforce ? -EACCES : 0;
	}

	free_page((unsigned long)buf);
	return 0;   /* PG_ALLOW */
}

/* ===========================================================================
 * PART 3: the LSM hooks themselves.
 * ===========================================================================
 */

/*
 * file_open hook.
 *
 * WHERE IT FIRES: do_dentry_open() in fs/open.c, after the dentry is resolved
 * and the `struct file` is built, but BEFORE the fd is handed to userspace.
 * Returning non-zero here makes open(2)/openat(2) fail with that errno. This
 * hook sees every successful path resolution, so it is extremely hot — keep it
 * cheap and bail early.
 *
 * current_cred() returns THIS task's credentials. It is safe to dereference
 * without extra locking because a task's own cred can only be replaced by the
 * task itself (via commit_creds), never concurrently from another CPU; the RCU
 * protection on ->cred matters for reading *another* task's creds, not ours.
 * We check fsuid, the id the VFS uses for file-access decisions (it can differ
 * from the real/effective uid, e.g. under setfsuid(2) or NFS servers).
 */
static int pathguard_file_open(struct file *file)
{
	const struct cred *cred = current_cred();

	/* Root bypass: privileged processes are outside this MAC policy. This
	 * also makes the common case (root/daemons opening files) a single uid
	 * compare with zero string work. A stricter module would gate on a
	 * capability (capable(CAP_...)) or a security label instead of uid==0. */
	if (uid_eq(cred->fsuid, GLOBAL_ROOT_UID))
		return 0;

	/* Denylist semantics: default ALLOW, only protected prefixes are DENY. */
	return pathguard_check_path(&file->f_path, "open",
				    pg_protected_rules,
				    ARRAY_SIZE(pg_protected_rules),
				    PG_ALLOW);
}

/*
 * bprm_check_security hook.
 *
 * WHERE IT FIRES: security_bprm_check(), called from the exec path in
 * fs/exec.c (bprm_execve -> exec_binprm) AFTER the target binary has been
 * opened and the new credentials computed into bprm->cred, but BEFORE the new
 * process image is committed. Returning non-zero aborts execve(2) with that
 * errno and the caller keeps running its old image.
 *
 * We judge the CALLER (current_cred()->fsuid): "is this user allowed to exec
 * this path?". Note bprm->cred holds the DIFFERENT, post-exec identity the
 * program will gain (this is where a setuid-root binary's elevation lives); a
 * policy about the resulting privilege would inspect that instead. We match on
 * bprm->file->f_path, the kernel's resolved path to the real on-disk binary,
 * not bprm->filename (the possibly-relative string the user passed).
 */
static int pathguard_bprm_check_security(struct linux_binprm *bprm)
{
	const struct cred *cred = current_cred();

	if (uid_eq(cred->fsuid, GLOBAL_ROOT_UID))
		return 0;

	/* Allowlist semantics: default DENY, only trusted prefixes are ALLOW. */
	return pathguard_check_path(&bprm->file->f_path, "exec",
				    pg_exec_rules,
				    ARRAY_SIZE(pg_exec_rules),
				    PG_DENY);
}

/* ===========================================================================
 * PART 4: registration — the static, stackable LSM boilerplate.
 * ===========================================================================
 */

/*
 * The hook table. LSM_HOOK_INIT(name, fn) expands to an initializer that pairs
 * the framework's per-hook list head (security_hook_heads.name) with our
 * callback fn. security_add_hooks() below splices each of these onto its list.
 *
 * __ro_after_init: the array is written exactly once, during registration at
 * boot, then the kernel makes the page read-only. That hardening matters — the
 * hook table is a prime target; if an attacker could overwrite a function
 * pointer here they would own every security decision. (In-tree code often
 * spells this __lsm_ro_after_init, which is the same thing unless the debug
 * option CONFIG_SECURITY_WRITABLE_HOOKS is set.)
 */
static struct security_hook_list pathguard_hooks[] __ro_after_init = {
	LSM_HOOK_INIT(file_open, pathguard_file_open),
	LSM_HOOK_INIT(bprm_check_security, pathguard_bprm_check_security),
};

#if LINUX_VERSION_CODE >= KERNEL_VERSION(6, 8, 0)
/*
 * Since 6.8 every LSM must present a `struct lsm_id` (a stable name + numeric
 * id) so userspace interfaces like /sys and the lsm_list_modules(2) /
 * lsm_get_self_attr(2) syscalls can enumerate active modules. A module that
 * gets merged upstream claims its own LSM_ID_* constant in
 * <uapi/linux/lsm.h>; an out-of-tree teaching module has none, so we use
 * LSM_ID_UNDEF (0) and note the gap honestly.
 */
static const struct lsm_id pathguard_lsmid = {
	.name = "pathguard",
	.id   = LSM_ID_UNDEF,   /* a real upstream LSM claims an assigned value */
};
#endif

/*
 * pathguard_init — run once, very early in boot, by the LSM framework.
 *
 * __init: this function (and everything it calls) is discarded after boot to
 * reclaim memory. That single attribute is *why* an LSM cannot be a loadable
 * module: by the time insmod could run, this code no longer exists. Registration
 * is intentionally a boot-only, one-way operation — you cannot unregister an LSM.
 */
static int __init pathguard_init(void)
{
	pr_info("initialising (enforce=%d, %zu protected prefixes, %zu exec-allow prefixes)\n",
		pathguard_enforce,
		ARRAY_SIZE(pg_protected_rules), ARRAY_SIZE(pg_exec_rules));

	/* Splice our two callbacks onto the framework's per-hook lists. After
	 * this returns, every open(2) and execve(2) system-wide will consult
	 * pathguard_file_open / pathguard_bprm_check_security. */
#if LINUX_VERSION_CODE >= KERNEL_VERSION(6, 8, 0)
	security_add_hooks(pathguard_hooks, ARRAY_SIZE(pathguard_hooks),
			   &pathguard_lsmid);
#else
	/* Pre-6.8 the last argument was just the LSM's name string. */
	security_add_hooks(pathguard_hooks, ARRAY_SIZE(pathguard_hooks),
			   "pathguard");
#endif
	return 0;
}

/*
 * DEFINE_LSM(pathguard) emits a `struct lsm_info` into the `.lsm_info.init`
 * ELF section. During boot the framework walks that section in the order named
 * by CONFIG_LSM and calls each .init. We do NOT set .flags = LSM_FLAG_LEGACY_MAJOR,
 * which is precisely what lets PathGuard stack alongside SELinux/AppArmor/Smack
 * instead of being mutually exclusive with them.
 */
DEFINE_LSM(pathguard) = {
	.name = "pathguard",
	.init = pathguard_init,
};

/* Module metadata. A built-in LSM does not strictly consume these, but they are
 * required for the `make` compile-check build (and are simply good hygiene).
 * GPL is not optional here: the LSM hook machinery uses GPL-only symbols. */
MODULE_LICENSE("GPL");
MODULE_AUTHOR("low-level-linux-lab");
MODULE_DESCRIPTION("PathGuard: a minimal stackable path-prefix MAC LSM (teaching core)");
