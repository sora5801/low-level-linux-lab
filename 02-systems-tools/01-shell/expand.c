/* ===========================================================================
 * expand.c — filesystem glob expansion (the "* ? []" that hits the disk).
 * ===========================================================================
 *
 * The lexer already did the pure-string expansions ('$', quotes, '~'). What is
 * left is the one expansion that must consult the FILESYSTEM: turning a
 * directory pattern (a literal directory plus a wildcard component, such as
 * "src" then a slash then a star-dot-c) into the list of names that actually
 * exist. That is a two-part job — walk a directory, and test each entry against
 * the pattern — and this file does the walking while match.c does the testing.
 *
 * SCOPE (honest teaching limits, all documented in the README):
 *   - We glob the LAST path component only. A pattern of the form "dir/PATTERN"
 *     works (literal directory, wildcard final component); a pattern that would
 *     glob across multiple directory levels does not.
 *   - Standard "hidden file" rule: a leading '.' in a name is matched only by an
 *     explicit leading '.' in the pattern.
 *   - "nullglob off": if nothing matches, the caller keeps the literal pattern,
 *     exactly like bash's default.
 * ===========================================================================
 */
#define _GNU_SOURCE
#include <dirent.h>      /* opendir, readdir, closedir, struct dirent          */
#include <limits.h>      /* PATH_MAX                                           */
#include <stdio.h>       /* snprintf                                           */
#include <stdlib.h>      /* qsort                                             */
#include <string.h>      /* strrchr, memcpy, strcmp                            */

#include "match.h"
#include "shell.h"

/* qsort comparator: lexicographic order, so `*.c` expands in a stable, sorted
 * order (the shell convention) rather than raw directory order. */
static int cmp_str(const void *a, const void *b)
{
    const char *const *pa = a;
    const char *const *pb = b;
    return strcmp(*pa, *pb);
}

/* ---------------------------------------------------------------------------
 * glob_expand — see shell.h. Returns the number of matches appended to `out`.
 * --------------------------------------------------------------------------- */
int glob_expand(const char *pattern, strvec *out)
{
    /* Split the pattern into a literal directory prefix and the component we
     * actually match. `prefix` keeps the trailing '/' so we can just concatenate
     * it back onto each matching name. */
    const char *slash = strrchr(pattern, '/');
    char        dirpath[PATH_MAX];
    char        prefix[PATH_MAX];
    const char *pat;

    if (slash) {
        size_t dlen = (size_t)(slash - pattern);        /* chars before the '/' */
        if (dlen == 0) {                                /* pattern like "/foo*" */
            dirpath[0] = '/'; dirpath[1] = '\0';
        } else {
            if (dlen >= sizeof dirpath) return 0;        /* pathologically long  */
            memcpy(dirpath, pattern, dlen);
            dirpath[dlen] = '\0';
        }
        size_t plen = (size_t)(slash - pattern) + 1;    /* include the '/'      */
        if (plen >= sizeof prefix) return 0;
        memcpy(prefix, pattern, plen);
        prefix[plen] = '\0';
        pat = slash + 1;                                /* the globbable part   */
    } else {
        dirpath[0] = '.'; dirpath[1] = '\0';            /* current directory    */
        prefix[0] = '\0';                               /* no prefix to rejoin  */
        pat = pattern;
    }

    DIR *d = opendir(dirpath);
    if (!d)
        return 0;   /* unreadable/nonexistent dir -> no match -> keep literal   */

    /* Collect matches locally so we can sort before handing them back. */
    strvec local;
    strvec_init(&local);

    struct dirent *de;
    /* readdir returns entries one at a time; NULL ends the scan (errno stays 0
     * on normal end). We do not recurse — only this one directory level. */
    while ((de = readdir(d)) != NULL) {
        const char *name = de->d_name;

        /* Hidden-file rule: names starting with '.' require the pattern to also
         * start with '.'. This is why `*` does not match `.` or `..`. */
        if (name[0] == '.' && pat[0] != '.')
            continue;

        if (wildcard_match(pat, name)) {
            char full[PATH_MAX];
            /* Rejoin the literal directory prefix. snprintf bounds the write. */
            int n = snprintf(full, sizeof full, "%s%s", prefix, name);
            if (n > 0 && n < (int)sizeof full)
                strvec_push(&local, xstrdup(full));
        }
    }
    closedir(d);

    if (local.len == 0) {
        strvec_free(&local);
        return 0;   /* nothing matched: caller falls back to the literal word  */
    }

    /* Sort for deterministic, shell-conventional ordering. */
    qsort(local.items, (size_t)local.len, sizeof local.items[0], cmp_str);

    /* Transfer the (now sorted) owned strings into the caller's vector. We move
     * pointers, not copies, then free only the backing array — the strings live
     * on inside `out`. */
    int count = local.len;
    for (int i = 0; i < local.len; i++)
        strvec_push(out, local.items[i]);
    free(local.items);   /* NOT strvec_free: the strings were transferred       */

    return count;
}
