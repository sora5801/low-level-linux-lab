/* ===========================================================================
 * diff.c — compare the WORKING TREE against the INDEX.
 * ===========================================================================
 *
 * There are three "trees" in git's mental model at any moment:
 *
 *     working tree  --(add)-->  index  --(commit)-->  HEAD commit
 *
 * This file answers the left arrow's question: "how does what's on disk right
 * now differ from what I've staged?" For each staged path we re-hash the file on
 * disk (our simplified index keeps no stat cache, so we must actually read it)
 * and compare that blob id to the staged one:
 *
 *   - id equal            -> unchanged (nothing to report)
 *   - id differs          -> modified  (and `diff` prints the line changes)
 *   - file missing        -> deleted
 * A file on disk that is not in the index at all is "untracked".
 *
 * THE DIFF ITSELF is intentionally a SIMPLE one: we trim the common leading and
 * trailing lines, then show the differing middle as removed (`-`) / added (`+`)
 * lines. That is not the minimal-edit (Myers) diff git computes, and we say so —
 * but common-prefix/suffix trimming is a real technique git also applies first,
 * and it produces correct, readable output for the edits you make while learning.
 * ===========================================================================
 */
#include "mygit.h"

#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <dirent.h>     /* opendir, readdir — to find untracked files          */
#include <sys/stat.h>

/* ---------------------------------------------------------------------------
 * blob_oid_of_file — the id a file's CURRENT contents would hash to as a blob,
 * WITHOUT writing anything. This is how we tell "changed vs staged": compare
 * this to the index entry's stored id. Returns 0, or -1 if the file is gone.
 * --------------------------------------------------------------------------- */
static int blob_oid_of_file(const char *path, unsigned char oid[OID_RAWSZ])
{
    if (!path_exists(path)) return -1;
    size_t len;
    unsigned char *data = read_file(path, &len);
    hash_object("blob", data, len, oid);      /* hash only; no store           */
    free(data);
    return 0;
}

/* ===========================================================================
 * A tiny growable list of strings, used to collect untracked paths.
 * =========================================================================== */
typedef struct { char **v; size_t n, cap; } strlist;

static void sl_push(strlist *s, const char *str)
{
    if (s->n == s->cap) {
        s->cap = s->cap ? s->cap * 2 : 16;
        s->v = xrealloc(s->v, s->cap * sizeof *s->v);
    }
    s->v[s->n++] = xstrdup(str);
}
static void sl_free(strlist *s)
{
    for (size_t i = 0; i < s->n; i++) free(s->v[i]);
    free(s->v);
}

/* ---------------------------------------------------------------------------
 * walk_worktree — recursively collect every regular file under `dir`.
 *
 * `rel` is the path of `dir` relative to the repo root ("" at the top). We skip
 * the repo's own metadata directory (.mygit) and the usual "." / ".." entries.
 * This is a straightforward opendir/readdir/closedir walk; each recursion level
 * opens one directory handle and releases it before returning, so we never hold
 * more than the current depth of handles open.
 * --------------------------------------------------------------------------- */
static void walk_worktree(const char *dir, const char *rel, strlist *out)
{
    DIR *d = opendir(dir);
    if (!d) return;                           /* unreadable dir: skip quietly    */

    struct dirent *de;
    while ((de = readdir(d)) != NULL) {
        const char *name = de->d_name;
        if (strcmp(name, ".") == 0 || strcmp(name, "..") == 0) continue;
        if (rel[0] == '\0' && strcmp(name, GIT_DIR) == 0) continue;  /* skip .mygit */

        /* Build the child's on-disk path and repo-relative path. */
        char child[4096], childrel[4096];
        snprintf(child,    sizeof child,    "%s/%s", dir, name);
        if (rel[0] == '\0') snprintf(childrel, sizeof childrel, "%s", name);
        else                snprintf(childrel, sizeof childrel, "%s/%s", rel, name);

        struct stat st;
        if (stat(child, &st) != 0) continue;
        if (S_ISDIR(st.st_mode))
            walk_worktree(child, childrel, out);   /* recurse into subdirectory */
        else if (S_ISREG(st.st_mode))
            sl_push(out, childrel);                /* a tracked-or-not file      */
    }
    closedir(d);
}

/* Is `path` present in the index? (linear scan — the index is small here). */
static int in_index(const index_t *ix, const char *path)
{
    for (size_t i = 0; i < ix->n; i++)
        if (strcmp(ix->entries[i].path, path) == 0) return 1;
    return 0;
}

/* ===========================================================================
 * cmd_status — `mygit status`
 *
 * Summarize the working-tree-vs-index comparison: modified, deleted, untracked.
 * =========================================================================== */
int cmd_status(int argc, char **argv)
{
    (void)argc; (void)argv;
    need_repo();

    index_t ix;
    index_read(&ix);

    int changes = 0;

    /* Staged files: modified or deleted on disk? */
    for (size_t i = 0; i < ix.n; i++) {
        unsigned char cur[OID_RAWSZ];
        int rc = blob_oid_of_file(ix.entries[i].path, cur);
        if (rc < 0) {
            printf("  deleted:  %s\n", ix.entries[i].path);
            changes = 1;
        } else if (memcmp(cur, ix.entries[i].oid, OID_RAWSZ) != 0) {
            printf("  modified: %s\n", ix.entries[i].path);
            changes = 1;
        }
    }

    /* Everything on disk that is not staged is untracked. */
    strlist files = {0};
    walk_worktree(".", "", &files);
    for (size_t i = 0; i < files.n; i++) {
        if (!in_index(&ix, files.v[i])) {
            printf("  untracked:%s%s\n", " ", files.v[i]);
            changes = 1;
        }
    }

    if (!changes) printf("clean — working tree matches the index\n");

    sl_free(&files);
    index_free(&ix);
    return 0;
}

/* ===========================================================================
 * A minimal line-oriented diff (common prefix/suffix trim).
 * =========================================================================== */
typedef struct { const char *p; size_t len; } line;

/* Split `buf` into lines. Each line spans up to and INCLUDING its '\n' (the last
 * line may have none). Comparing whole lines including the newline keeps "no
 * trailing newline" changes visible. Returns a malloc'd array; caller frees. */
static line *split_lines(const char *buf, size_t n, size_t *count)
{
    size_t cap = 32, c = 0;
    line *v = xmalloc(cap * sizeof *v);
    size_t i = 0;
    while (i < n) {
        size_t start = i;
        while (i < n && buf[i] != '\n') i++;
        if (i < n) i++;                       /* include the newline             */
        if (c == cap) { cap *= 2; v = xrealloc(v, cap * sizeof *v); }
        v[c].p = buf + start;
        v[c].len = i - start;
        c++;
    }
    *count = c;
    return v;
}

static int line_eq(line a, line b)
{
    return a.len == b.len && memcmp(a.p, b.p, a.len) == 0;
}

/* Print one line with a prefix marker, always terminating with a newline so the
 * output stays aligned even when the source line lacked one. */
static void put_line(char marker, line l)
{
    putchar(marker);
    fwrite(l.p, 1, l.len, stdout);
    if (l.len == 0 || l.p[l.len - 1] != '\n') putchar('\n');
}

/* Emit a coarse unified-ish diff of two buffers for one path. */
static void diff_bufs(const char *path,
                      const char *old, size_t oldn,
                      const char *new, size_t newn)
{
    size_t on, nn;
    line *ol = split_lines(old, oldn, &on);
    line *nl = split_lines(new, newn, &nn);

    /* Common leading lines. */
    size_t pre = 0;
    while (pre < on && pre < nn && line_eq(ol[pre], nl[pre])) pre++;

    /* Common trailing lines (not overlapping the prefix). */
    size_t suf = 0;
    while (suf < (on - pre) && suf < (nn - pre) &&
           line_eq(ol[on - 1 - suf], nl[nn - 1 - suf]))
        suf++;

    printf("--- a/%s\n", path);
    printf("+++ b/%s\n", path);
    /* One hunk header covering the changed middle (1-based line numbers). */
    printf("@@ -%zu,%zu +%zu,%zu @@\n",
           pre + 1, (on - suf) - pre, pre + 1, (nn - suf) - pre);

    for (size_t i = pre; i < on - suf; i++) put_line('-', ol[i]);   /* removed  */
    for (size_t i = pre; i < nn - suf; i++) put_line('+', nl[i]);   /* added    */

    free(ol);
    free(nl);
}

/* ===========================================================================
 * cmd_diff — `mygit diff`
 *
 * For every staged path whose on-disk contents differ from the staged blob,
 * print the change. Deleted files are reported; the actual line diff is shown
 * for modified files by reading the staged blob out of the object store and
 * comparing it to the working copy.
 * =========================================================================== */
int cmd_diff(int argc, char **argv)
{
    (void)argc; (void)argv;
    need_repo();

    index_t ix;
    index_read(&ix);

    for (size_t i = 0; i < ix.n; i++) {
        const char *path = ix.entries[i].path;

        if (!path_exists(path)) {
            printf("--- a/%s\n+++ /dev/null   (deleted)\n", path);
            continue;
        }

        unsigned char cur[OID_RAWSZ];
        blob_oid_of_file(path, cur);
        if (memcmp(cur, ix.entries[i].oid, OID_RAWSZ) == 0)
            continue;                          /* unchanged                       */

        /* Staged version comes from the object store; working version from disk. */
        char type[8]; size_t staged_len;
        unsigned char *staged = read_object(ix.entries[i].oid, type, &staged_len);

        size_t work_len;
        unsigned char *work = read_file(path, &work_len);

        diff_bufs(path, (char *)staged, staged_len, (char *)work, work_len);

        free(staged);
        free(work);
    }

    index_free(&ix);
    return 0;
}
