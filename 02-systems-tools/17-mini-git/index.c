/* ===========================================================================
 * index.c — the staging area (git calls it "the index" or "the cache").
 * ===========================================================================
 *
 * The index is the bridge between your working tree and the next commit. `add`
 * copies a file's CURRENT contents into a blob object and records the mapping
 * "path -> blob id + mode" here. `write-tree` later turns this flat list into
 * tree objects; `commit` points a commit at that tree. Nothing is committed
 * until you stage it, which is why the index exists as a separate step.
 *
 * ON-DISK FORMAT — a DELIBERATE SIMPLIFICATION
 * --------------------------------------------
 * Real git stores the index as a binary file ("DIRC" signature, a version, then
 * fixed-size cache entries carrying cached stat(2) data — ctime, mtime, dev,
 * ino, size — so it can detect changes without re-hashing). We store a plain,
 * sorted TEXT file, one entry per line:
 *
 *       <mode> <40-hex-oid> <path>\n
 *
 * That throws away the stat cache (so our `status`/`diff` must re-hash files),
 * but it makes the staging concept completely transparent: you can `cat
 * .mygit/index` and read exactly what will be committed. The README calls out
 * this trade-off explicitly.
 * ===========================================================================
 */
#include "mygit.h"

#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <sys/stat.h>   /* stat, S_IXUSR — to detect the executable bit        */

#define INDEX_PATH  (GIT_DIR "/index")

/* ---------------------------------------------------------------------------
 * index_read — load the index file into memory (empty index if none exists).
 *
 * We parse line by line: "<mode> <hexoid> <path>". Paths never contain spaces in
 * this teaching tool (documented limitation), so a two-space split is safe: the
 * mode ends at the first space, the oid at the second, the rest is the path.
 * --------------------------------------------------------------------------- */
void index_read(index_t *ix)
{
    ix->entries = NULL;
    ix->n = ix->cap = 0;

    if (!path_exists(INDEX_PATH))    /* an unborn index is simply empty         */
        return;

    size_t len;
    char *data = (char *)read_file(INDEX_PATH, &len);

    char *p = data;
    char *end = data + len;
    while (p < end) {
        char *nl = memchr(p, '\n', (size_t)(end - p));
        if (!nl) break;              /* ignore a trailing partial line          */
        *nl = '\0';

        /* mode <sp> oid <sp> path */
        char *sp1 = strchr(p, ' ');
        if (!sp1) { p = nl + 1; continue; }
        *sp1 = '\0';
        char *oidhex = sp1 + 1;
        char *sp2 = strchr(oidhex, ' ');
        if (!sp2) { p = nl + 1; continue; }
        *sp2 = '\0';
        char *path = sp2 + 1;

        unsigned char oid[OID_RAWSZ];
        if (strlen(oidhex) == OID_HEXSZ && hex_decode(oidhex, oid, OID_RAWSZ) == 0)
            index_upsert(ix, path, oid, p);

        p = nl + 1;
    }
    free(data);
}

/* Comparator for the sort in index_write: byte-wise by path. Keeping the index
 * sorted gives deterministic tree building and lets a reader binary-search. */
static int entry_cmp(const void *a, const void *b)
{
    const index_entry *ea = a, *eb = b;
    return strcmp(ea->path, eb->path);
}

/* index_write — persist the (sorted) index back to disk. We build the whole text
 * in one heap buffer and write it in a single write_file call, so a reader never
 * observes a half-written index. */
void index_write(const index_t *ix)
{
    /* Sort a shallow copy of the entry array by path. */
    index_entry *sorted = xmalloc(ix->n * sizeof *sorted + 1);
    memcpy(sorted, ix->entries, ix->n * sizeof *sorted);
    qsort(sorted, ix->n, sizeof *sorted, entry_cmp);

    /* Assemble the text. Grow a buffer as we append each line. */
    size_t cap = 256, len = 0;
    char *buf = xmalloc(cap);
    for (size_t i = 0; i < ix->n; i++) {
        char hex[OID_HEXSZ + 1];
        hex_encode(sorted[i].oid, OID_RAWSZ, hex);

        char line[4200];
        int n = snprintf(line, sizeof line, "%s %s %s\n",
                         sorted[i].mode, hex, sorted[i].path);
        if (n < 0) die("index line formatting failed");

        if (len + (size_t)n + 1 > cap) {
            while (len + (size_t)n + 1 > cap) cap *= 2;
            buf = xrealloc(buf, cap);
        }
        memcpy(buf + len, line, (size_t)n);
        len += (size_t)n;
    }

    write_file(INDEX_PATH, buf, len);
    free(buf);
    free(sorted);
}

/* index_upsert — insert `path`, or update it in place if already staged.
 *
 * Upsert (not append) is what makes `add`-ing the same file twice do the right
 * thing: the second add replaces the first entry's blob id rather than creating
 * a duplicate. We grow the array geometrically (cap *= 2) so N adds cost O(N)
 * amortized allocations, not O(N^2). */
void index_upsert(index_t *ix, const char *path,
                  const unsigned char oid[OID_RAWSZ], const char *mode)
{
    for (size_t i = 0; i < ix->n; i++) {
        if (strcmp(ix->entries[i].path, path) == 0) {
            memcpy(ix->entries[i].oid, oid, OID_RAWSZ);
            snprintf(ix->entries[i].mode, sizeof ix->entries[i].mode, "%s", mode);
            return;
        }
    }
    if (ix->n == ix->cap) {
        ix->cap = ix->cap ? ix->cap * 2 : 16;
        ix->entries = xrealloc(ix->entries, ix->cap * sizeof *ix->entries);
    }
    index_entry *e = &ix->entries[ix->n++];
    e->path = xstrdup(path);
    memcpy(e->oid, oid, OID_RAWSZ);
    snprintf(e->mode, sizeof e->mode, "%s", mode);
}

void index_free(index_t *ix)
{
    for (size_t i = 0; i < ix->n; i++) free(ix->entries[i].path);
    free(ix->entries);
    ix->entries = NULL;
    ix->n = ix->cap = 0;
}

/* ---------------------------------------------------------------------------
 * add_one — stage a single regular file: hash its contents into a blob object,
 * write that object, and upsert the index entry with the right mode.
 *
 * The mode is either 100644 (normal) or 100755 (executable). git tracks exactly
 * one permission bit — "is the owner-execute bit set?" — and nothing else, so a
 * repo behaves identically regardless of the host umask. We read that bit from
 * stat(2)'s st_mode.
 * --------------------------------------------------------------------------- */
static void add_one(index_t *ix, const char *path)
{
    struct stat st;
    if (stat(path, &st) != 0) die_errno("stat %s", path);
    if (S_ISDIR(st.st_mode)) return;      /* directories are added via recursion below */

    size_t len;
    unsigned char *data = read_file(path, &len);

    unsigned char oid[OID_RAWSZ];
    write_object("blob", data, len, oid); /* content-address the file bytes     */
    free(data);

    const char *mode = (st.st_mode & S_IXUSR) ? MODE_BLOB_X : MODE_BLOB;
    index_upsert(ix, path, oid, mode);
    printf("added %s\n", path);
}

/* ===========================================================================
 * cmd_add — `mygit add <path>...`
 *
 * Stage the given files. For a directory argument we do NOT recurse in this
 * teaching core (git would, honoring .gitignore); we simply skip it and tell the
 * user. Naming a file re-stages its current contents.
 * =========================================================================== */
int cmd_add(int argc, char **argv)
{
    if (argc < 1) die("usage: mygit add <path>...");
    need_repo();

    index_t ix;
    index_read(&ix);

    for (int i = 0; i < argc; i++) {
        if (is_dir(argv[i])) {
            fprintf(stderr, "mygit: skipping directory '%s' "
                            "(this teaching core does not recurse; add files)\n",
                    argv[i]);
            continue;
        }
        add_one(&ix, argv[i]);
    }

    index_write(&ix);
    index_free(&ix);
    return 0;
}
