/* ===========================================================================
 * demo.c — the PURE-LOGIC core of the PathGuard LSM, extracted so it can be
 *          compiled to standalone assembly on any host.
 * ===========================================================================
 *
 * WHY THIS FILE EXISTS
 * --------------------
 * The real module (../pathguard.c) is Linux-kernel C: it includes
 * <linux/lsm_hooks.h>, dereferences `struct file`, calls `d_path()`, and can
 * only be compiled inside a configured kernel tree. You therefore cannot run
 *
 *     clang -S pathguard.c
 *
 * on this machine — the kernel headers are not standalone. So we lift out the
 * one piece that is *pure computation with no kernel dependencies at all*: the
 * path-prefix matcher and the policy-table lookup that every hook in the LSM
 * ultimately calls to turn a path string into an ALLOW/DENY verdict.
 *
 * This file declares its own types, includes NOTHING, and is byte-for-byte the
 * same algorithm the kernel module uses. That makes its generated assembly an
 * honest window into the hottest, most security-critical code in the module:
 * the string comparison that decides whether a process may open or execute a
 * file. Getting the *boundary rule* wrong here is a real class of security bug
 * (e.g. treating "/etc/secretssh" as being under "/etc/secret"), which is
 * exactly why it is worth reading in assembly.
 *
 * Compile the teaching assembly (done for you by `make asm`):
 *     clang --target=x86_64-pc-linux-gnu -S -O0 ... demo.c -o asm/demo.O0.s
 *     clang --target=x86_64-pc-linux-gnu -S -O1 ... demo.c -o asm/demo.s
 *     clang --target=x86_64-pc-linux-gnu -S -O2 ... demo.c -o asm/demo.O2.s
 * ===========================================================================
 */

/* We are freestanding: no <stddef.h>, no <stdbool.h>. Declare the few types we
 * need ourselves so the translation unit depends on nothing. `pg_size` stands
 * in for size_t; on LP64 (Linux x86-64) an unsigned long is 64 bits, wide
 * enough for any in-kernel string length. Verdicts are a tiny enum so the
 * intent (ALLOW vs DENY) is visible in both the C and the disassembly. */
typedef unsigned long pg_size;

enum pg_verdict {
    PG_ALLOW = 0,   /* let the operation proceed (hook returns 0)           */
    PG_DENY  = 1,   /* block it (hook returns -EACCES in the real module)   */
};

/* One policy rule: "any path under `prefix` gets `verdict`". The kernel keeps
 * a small static array of these per hook (protected-read paths, exec
 * allowlist, ...). Ordered first-match, so more-specific rules go first. */
struct pg_rule {
    const char *prefix;   /* an absolute path with NO trailing slash        */
    int         verdict;  /* one of enum pg_verdict                         */
};

/* ---------------------------------------------------------------------------
 * pg_path_has_prefix — does `path` lie at or under directory `prefix`?
 *
 * THE INVARIANT THAT MATTERS: a plain `strncmp(path, prefix, strlen(prefix))`
 * is WRONG for path matching. With prefix "/etc/secret" it would also match
 * "/etc/secretkeys" and "/etc/secretstuff", silently widening the protected
 * set to unrelated files. Correct prefix-of-a-*path* matching requires that
 * the character in `path` immediately after the matched prefix be a component
 * boundary — either the end of the string ('\0', the path IS the directory)
 * or a '/' (the path is something strictly inside the directory).
 *
 * Precondition (documented, not enforced): `prefix` is normalised and carries
 * no trailing '/'. That keeps this routine branch-light — the star of the
 * assembly — instead of special-casing "/". Returns 1 on match, else 0.
 * --------------------------------------------------------------------------- */
int pg_path_has_prefix(const char *path, const char *prefix)
{
    pg_size i = 0;

    /* Walk the prefix. As long as prefix still has characters, every one of
     * them must equal the corresponding character in path. The moment path
     * runs out (its '\0') or diverges, path[i] != prefix[i] and we bail —
     * note path[i] can never spuriously equal a non-NUL prefix[i] once we are
     * past path's end, because C strings are NUL-terminated. */
    while (prefix[i] != '\0') {
        if (path[i] != prefix[i])
            return 0;               /* diverged before the prefix ended     */
        i++;
    }

    /* Every prefix character matched. Now enforce the component boundary:
     * path[i] is the first character in `path` past the prefix. It is a real
     * match only if that character ends the path ('\0') or starts a new
     * component ('/'). Anything else (a letter, a digit) means we merely
     * matched a longer sibling name, which must NOT count. */
    if (path[i] == '\0' || path[i] == '/')
        return 1;

    return 0;
}

/* ---------------------------------------------------------------------------
 * pg_policy_lookup — turn a path into a verdict via an ordered rule table.
 *
 * Scans `rules[0..n)` and returns the verdict of the FIRST rule whose prefix
 * covers `path` (first-match, so callers put specific carve-outs before broad
 * catch-alls). If nothing matches, the caller-supplied `default_verdict`
 * applies — this is the policy's "fail-open vs fail-closed" knob: an exec
 * allowlist passes PG_DENY as the default (unlisted paths are refused), while
 * a protected-read denylist passes PG_ALLOW (only listed paths are refused).
 *
 * The whole LSM's decision reduces to this function; keeping it tiny and
 * table-driven is what makes the policy auditable. Returns an enum pg_verdict.
 * --------------------------------------------------------------------------- */
int pg_policy_lookup(const char *path, const struct pg_rule *rules, int n,
                     int default_verdict)
{
    int i;

    for (i = 0; i < n; i++) {
        /* First rule to cover this path wins; its verdict is final. */
        if (pg_path_has_prefix(path, rules[i].prefix))
            return rules[i].verdict;
    }

    /* No rule mentioned this path: apply the table's default disposition. */
    return default_verdict;
}
