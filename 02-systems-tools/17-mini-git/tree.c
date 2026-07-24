/* ===========================================================================
 * tree.c — build tree objects from the index (the "snapshot" step).
 * ===========================================================================
 *
 * `write-tree` is where a FLAT staging list becomes a DIRECTORY TREE. Given the
 * index entries
 *
 *       100644 <oid> README.md
 *       100644 <oid> src/main.c
 *       100755 <oid> src/build.sh
 *
 * we must produce one tree object for `src/` and one for the root that points at
 * it. A tree object's body is a sequence of entries, each:
 *
 *       "<mode> <name>\0<20 RAW id bytes>"
 *
 * — mode as ASCII ("100644", "40000" for a subtree), the name, a NUL, then the
 * pointed-at object's 20 raw bytes (NOT hex; trees are binary). Entries are
 * ordered by git's tree-sort rule (below).
 *
 * WHY THIS MAKES COMMITS IMMUTABLE SNAPSHOTS
 * ------------------------------------------
 * The root tree's id is SHA-1 over a body that literally contains the id of the
 * `src` subtree, whose id is SHA-1 over a body containing the ids of main.c and
 * build.sh. Change one byte of main.c and:
 *     its blob id changes -> the src tree's body changes -> src tree id changes
 *         -> the root tree's body changes -> root tree id changes.
 * A commit records the root tree id, so the commit id transitively pins every
 * byte of every file. You cannot alter history without minting new ids — that
 * is a Merkle tree, and it is the whole security/immutability story of git.
 * ===========================================================================
 */
#include "mygit.h"

#include <stdio.h>
#include <stdlib.h>
#include <string.h>

/* Byte-order comparator on the path field, so entries under one directory form
 * a contiguous run (build_tree relies on that). Forward-declared here, defined
 * at the bottom. */
static int entry_path_cmp(const void *a, const void *b);

/* One resolved child of the directory currently being built. */
typedef struct {
    char          *name;              /* just the component (no slashes)        */
    char           mode[8];           /* "100644" / "100755" / "40000"          */
    unsigned char  oid[OID_RAWSZ];    /* id of the blob or subtree              */
    int            is_tree;           /* affects the sort (see tent_cmp)        */
} tent;

/* ---------------------------------------------------------------------------
 * tent_cmp — git's tree-entry ordering ("base_name_compare").
 *
 * Entries sort by name bytes, EXCEPT that a directory is compared as though its
 * name ended in '/'. This one rule is why, for a file "foo.txt" and a directory
 * "foo", git orders them the way it does — and getting it exactly right is what
 * makes our tree ids byte-identical to git's. We compare the shared prefix, then
 * the first differing byte, substituting '/' for a directory's implicit tail.
 * --------------------------------------------------------------------------- */
static int tent_cmp(const void *pa, const void *pb)
{
    const tent *a = pa, *b = pb;
    size_t la = strlen(a->name), lb = strlen(b->name);
    size_t l  = la < lb ? la : lb;

    int c = memcmp(a->name, b->name, l);
    if (c) return c;

    /* Equal up to the shorter name. The next "character" of each is its NUL
     * terminator, but for a directory we pretend it is '/'. name[l] is a valid
     * read: it is the '\0' when the name is exhausted. */
    unsigned char ca = (unsigned char)a->name[l];
    unsigned char cb = (unsigned char)b->name[l];
    if (ca == 0 && a->is_tree) ca = '/';
    if (cb == 0 && b->is_tree) cb = '/';
    return (ca < cb) ? -1 : (ca > cb) ? 1 : 0;
}

/* Append a child to a growing tent[] array (geometric growth). */
static void push_kid(tent **kids, size_t *nk, size_t *cap,
                     const char *name, size_t namelen,
                     const char *mode, const unsigned char oid[OID_RAWSZ],
                     int is_tree)
{
    if (*nk == *cap) {
        *cap = *cap ? *cap * 2 : 8;
        *kids = xrealloc(*kids, *cap * sizeof **kids);
    }
    tent *e = &(*kids)[(*nk)++];
    e->name = xmalloc(namelen + 1);
    memcpy(e->name, name, namelen);
    e->name[namelen] = '\0';
    snprintf(e->mode, sizeof e->mode, "%s", mode);
    memcpy(e->oid, oid, OID_RAWSZ);
    e->is_tree = is_tree;
}

/* ---------------------------------------------------------------------------
 * build_tree — recursively build the tree for one directory and return its id.
 *
 * The index entries are pre-sorted by full path, so all entries under a given
 * directory form a CONTIGUOUS range [start, end). `prefix_len` is how many
 * leading characters of each path name this directory (0 for the root). We scan
 * the range once:
 *   - a remainder with no '/'  is a FILE directly in this directory -> blob kid
 *   - a remainder with a '/'   names a SUBDIRECTORY -> gather its whole run and
 *     recurse, producing a subtree kid
 * then sort the kids, serialize, and store the tree object.
 * --------------------------------------------------------------------------- */
static void build_tree(const index_entry *e, size_t start, size_t end,
                       size_t prefix_len, unsigned char out[OID_RAWSZ])
{
    tent  *kids = NULL;
    size_t nk = 0, cap = 0;

    size_t i = start;
    while (i < end) {
        const char *rem   = e[i].path + prefix_len;   /* path relative to here  */
        const char *slash = strchr(rem, '/');

        if (!slash) {
            /* A plain file living directly in this directory. */
            push_kid(&kids, &nk, &cap, rem, strlen(rem),
                     e[i].mode, e[i].oid, /*is_tree=*/0);
            i++;
        } else {
            /* A subdirectory named rem[0..sublen). Because the range is sorted,
             * every entry that lives under "<sub>/" is contiguous from here. */
            size_t sublen = (size_t)(slash - rem);
            size_t j = i;
            while (j < end) {
                const char *r2 = e[j].path + prefix_len;
                if (strncmp(r2, rem, sublen) == 0 && r2[sublen] == '/')
                    j++;
                else
                    break;
            }

            /* Recurse: build the subtree from [i, j), advancing the prefix past
             * "<sub>/". Its returned id becomes this level's tree-kid. */
            unsigned char sub_oid[OID_RAWSZ];
            build_tree(e, i, j, prefix_len + sublen + 1, sub_oid);
            push_kid(&kids, &nk, &cap, rem, sublen,
                     MODE_TREE, sub_oid, /*is_tree=*/1);
            i = j;
        }
    }

    /* Order the children exactly as git would. */
    qsort(kids, nk, sizeof *kids, tent_cmp);

    /* Serialize the tree body: for each kid, "<mode> <name>\0<20 raw bytes>". */
    size_t bcap = 256, blen = 0;
    unsigned char *body = xmalloc(bcap);
    for (size_t k = 0; k < nk; k++) {
        size_t mlen = strlen(kids[k].mode);
        size_t nlen = strlen(kids[k].name);
        size_t need = mlen + 1 + nlen + 1 + OID_RAWSZ;   /* mode ' ' name '\0' id */
        if (blen + need > bcap) {
            while (blen + need > bcap) bcap *= 2;
            body = xrealloc(body, bcap);
        }
        memcpy(body + blen, kids[k].mode, mlen);         blen += mlen;
        body[blen++] = ' ';
        memcpy(body + blen, kids[k].name, nlen);         blen += nlen;
        body[blen++] = '\0';
        memcpy(body + blen, kids[k].oid, OID_RAWSZ);     blen += OID_RAWSZ;
    }

    /* Store the tree object; its id is our return value. An empty directory
     * yields the well-known empty-tree id 4b825dc6...4904. */
    write_object("tree", body, blen, out);

    free(body);
    for (size_t k = 0; k < nk; k++) free(kids[k].name);
    free(kids);
}

/* Public entry: build every tree implied by the index, return the root's id. */
void write_tree_from_index(const index_t *ix, unsigned char root_oid[OID_RAWSZ])
{
    /* The entries must be sorted by path for the contiguity assumption above.
     * The index is written sorted, but we re-sort a copy defensively so this
     * function is correct for any caller-supplied index_t. */
    index_entry *sorted = xmalloc(ix->n * sizeof *sorted + 1);
    memcpy(sorted, ix->entries, ix->n * sizeof *sorted);
    qsort(sorted, ix->n, sizeof *sorted, entry_path_cmp);

    build_tree(sorted, 0, ix->n, 0, root_oid);
    free(sorted);
}

/* Byte-order comparator on the path field (see forward declaration up top). */
static int entry_path_cmp(const void *a, const void *b)
{
    const index_entry *ea = a, *eb = b;
    return strcmp(ea->path, eb->path);
}

/* ===========================================================================
 * cmd_write_tree — `mygit write-tree`
 *
 * Turn the current index into tree objects and print the root tree id. Nothing
 * about HEAD changes; this is a pure "snapshot the staging area" primitive that
 * `commit` builds on.
 * =========================================================================== */
int cmd_write_tree(int argc, char **argv)
{
    (void)argc; (void)argv;
    need_repo();

    index_t ix;
    index_read(&ix);

    unsigned char root[OID_RAWSZ];
    write_tree_from_index(&ix, root);
    index_free(&ix);

    char hex[OID_HEXSZ + 1];
    hex_encode(root, OID_RAWSZ, hex);
    printf("%s\n", hex);
    return 0;
}
