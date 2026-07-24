/* ===========================================================================
 * mygit.h — shared types and the internal API of the mini-git.
 * ===========================================================================
 *
 * This header is included by every translation unit that talks to the object
 * store, the index, or the ref namespace. It pulls in the freestanding-ish
 * <stddef.h> (for size_t) and our own SHA-1, and declares the vocabulary of the
 * whole program: object ids, the on-disk layout constants, and the module
 * boundaries (object / index / tree / commit / diff).
 *
 * THE ONE IDEA TO HOLD ONTO
 * -------------------------
 * Everything git stores is an immutable, content-addressed OBJECT living at
 *     .mygit/objects/<first 2 hex>/<remaining 38 hex>
 * whose name IS the SHA-1 of its own contents. Blobs hold file bytes, trees
 * hold directory listings (name -> object id + mode), commits point at one tree
 * plus parent commit(s). Because a tree's id depends on every id it contains,
 * and a commit's id depends on its tree's id, a commit id transitively fixes the
 * entire snapshot: it cannot change without changing the id. That is the whole
 * magic, and it is just hashing all the way down.
 * ===========================================================================
 */
#ifndef MYGIT_H
#define MYGIT_H

#include <stddef.h>     /* size_t                                             */
#include "sha1.h"

/* --- on-disk layout constants --------------------------------------------- */
#define GIT_DIR    ".mygit"     /* our repo directory (git uses ".git")       */
#define OID_RAWSZ  20           /* a SHA-1 is 20 raw bytes                     */
#define OID_HEXSZ  40           /* ...or 40 lowercase hex characters          */

/* git's canonical file modes, as they appear (in octal-looking ASCII) inside
 * tree objects. We deliberately store them as the exact strings git uses so our
 * objects are byte-identical to git's — see README "interoperability". */
#define MODE_BLOB     "100644"  /* a normal file                              */
#define MODE_BLOB_X   "100755"  /* an executable file                         */
#define MODE_TREE     "40000"   /* a subdirectory (note: NO leading zero)     */

/* ===========================================================================
 * util.c — errors, memory, raw file I/O, hex.
 * =========================================================================== */

/* die/warn: our uniform error exits. die() prints to stderr and exit(1)s; it
 * never returns, so callers need no error plumbing after it. */
void  die(const char *fmt, ...) __attribute__((noreturn, format(printf, 1, 2)));
void  die_errno(const char *fmt, ...) __attribute__((noreturn, format(printf, 1, 2)));

/* Checked allocation wrappers: they die() rather than return NULL, so the rest
 * of the code can assume success. Ownership is always "caller frees". */
void *xmalloc(size_t n);
void *xrealloc(void *p, size_t n);
char *xstrdup(const char *s);

/* Raw file I/O built on open/read/write/close (see util.c for the syscall-level
 * commentary on EINTR and short reads/writes). read_file returns a malloc'd
 * buffer of the whole file (NUL-terminated for convenience but *len is the true
 * byte count); the caller frees it. write_file writes atomically-enough for a
 * teaching tool (truncate + full write). Both die() on I/O error. */
unsigned char *read_file(const char *path, size_t *len);
void           write_file(const char *path, const void *data, size_t len);

/* mkdir -p: create `path` and any missing parents (mode 0777 & umask). Existing
 * directories are not an error. */
void mkdir_p(const char *path);

/* Does a filesystem path exist / is it a directory? (thin stat wrappers) */
int  path_exists(const char *path);
int  is_dir(const char *path);

/* hex_encode: 20 raw bytes -> 40 lowercase hex chars + NUL (out is >= 41).
 * hex_decode: 40 hex chars -> 20 raw bytes; returns 0 on success, -1 on a bad
 * digit or short input. These are the human<->machine boundary for object ids. */
void hex_encode(const unsigned char *raw, size_t n, char *out);
int  hex_decode(const char *hex, unsigned char *raw, size_t n);

/* Convenience: format a 20-byte oid as a fresh 41-char string owned by caller.
 * (Small enough that returning by value into a caller buffer is cleaner:) */
void oid_to_hex(const unsigned char oid[OID_RAWSZ], char out[OID_HEXSZ + 1]);

/* ===========================================================================
 * object.c — the content-addressed object store.
 * =========================================================================== */

/* Compute the object id of a would-be object WITHOUT writing it: hashes the
 * canonical form "<type> <size>\0<content>" into `oid`. This is the definition
 * of git's content addressing, isolated in one place. */
void hash_object(const char *type, const unsigned char *content, size_t len,
                 unsigned char oid[OID_RAWSZ]);

/* Same, but also zlib-deflate the canonical form and write it to
 * .mygit/objects/xx/rest (a no-op if that object already exists — identical
 * content means identical id means it is already stored). Returns via `oid`. */
void write_object(const char *type, const unsigned char *content, size_t len,
                  unsigned char oid[OID_RAWSZ]);

/* Read object `oid` back: inflate it, split off the "<type> <size>\0" header,
 * and return a malloc'd buffer of just the content (caller frees). `type_out`
 * (size >= 8) receives the type string; `size_out` the content length. */
unsigned char *read_object(const unsigned char oid[OID_RAWSZ],
                           char *type_out, size_t *size_out);

/* zlib helpers (thin wrappers over deflate/inflate that grow a heap buffer).
 * Both return a malloc'd buffer the caller frees, and die() on a zlib error. */
unsigned char *zlib_deflate_all(const unsigned char *in, size_t in_len, size_t *out_len);
unsigned char *zlib_inflate_all(const unsigned char *in, size_t in_len, size_t *out_len);

/* CLI entry points implemented in object.c. */
int cmd_hash_object(int argc, char **argv);
int cmd_cat_file(int argc, char **argv);

/* ===========================================================================
 * index.c — the staging area ("the index").
 * ===========================================================================
 *
 * The index is the list of what the NEXT commit will contain. Real git stores
 * it as a versioned BINARY file full of cached stat(2) data; we store a simple,
 * sorted TEXT file (one "<mode> <hexoid> <path>" line per entry) so the staging
 * concept is legible. The README is explicit about this simplification.
 */
typedef struct {
    char          *path;             /* repo-relative path, '/'-separated      */
    unsigned char  oid[OID_RAWSZ];   /* id of the staged blob                  */
    char           mode[8];          /* "100644" / "100755"                    */
} index_entry;

typedef struct {
    index_entry *entries;            /* sorted by path (byte order)            */
    size_t       n, cap;             /* count / capacity                       */
} index_t;

void index_read(index_t *ix);                 /* load .mygit/index (empty if none) */
void index_write(const index_t *ix);          /* persist, sorted by path           */
void index_upsert(index_t *ix, const char *path,
                  const unsigned char oid[OID_RAWSZ], const char *mode);
void index_free(index_t *ix);

int cmd_add(int argc, char **argv);

/* ===========================================================================
 * tree.c — turn the flat index into a tree of tree objects.
 * =========================================================================== */

/* Build the nested tree objects that represent the index and return the root
 * tree's id. This is where "a flat staging list" becomes "a directory tree",
 * writing one tree object per directory. */
void write_tree_from_index(const index_t *ix, unsigned char root_oid[OID_RAWSZ]);

int cmd_write_tree(int argc, char **argv);

/* ===========================================================================
 * commit.c — commits, refs (HEAD), and the log walk.
 * =========================================================================== */

int cmd_commit_tree(int argc, char **argv);   /* low-level: make a commit object */
int cmd_commit(int argc, char **argv);        /* porcelain: write-tree+commit+HEAD */
int cmd_log(int argc, char **argv);           /* walk first-parent history         */

/* Ref helpers (shared with diff/status). resolve_head returns 0 and fills `oid`
 * with HEAD's commit, or -1 if HEAD is unborn (no commits yet). */
int  resolve_head(unsigned char oid[OID_RAWSZ]);
void update_head(const unsigned char oid[OID_RAWSZ]);

/* ===========================================================================
 * diff.c — working tree vs index.
 * =========================================================================== */
int cmd_diff(int argc, char **argv);
int cmd_status(int argc, char **argv);

/* ===========================================================================
 * shared repo guard — every subcommand except `init` needs a repo to exist.
 * =========================================================================== */
void need_repo(void);

#endif /* MYGIT_H */
