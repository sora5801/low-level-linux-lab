/* ===========================================================================
 * object.c — the content-addressed object store (the heart of git).
 * ===========================================================================
 *
 * A git object is stored in three transformations, in this order:
 *
 *   1. CANONICAL FORM   :  "<type> <size>\0<content>"
 *                          e.g.  "blob 11\0hello world"
 *      The header is ASCII type + a space + the DECIMAL content length + a NUL.
 *      The size lets a reader split header from content unambiguously even
 *      though content may itself contain NULs.
 *
 *   2. OBJECT ID        :  SHA-1(canonical form)   -> 20 bytes / 40 hex chars
 *      Note: the hash covers the header too, so a 3-byte blob and a 3-byte tree
 *      with the same bytes get DIFFERENT ids. Type is part of identity.
 *
 *   3. ON-DISK FORM     :  zlib-deflate(canonical form)
 *      written to  .mygit/objects/<id[0:2]>/<id[2:40]>
 *      The 2-char fan-out directory keeps any one directory from holding
 *      millions of files (a real filesystem concern at scale).
 *
 * Because we compute the exact same canonical bytes git does and hash them with
 * plain SHA-1, our object ids MATCH git's. Real `git cat-file` can read objects
 * this program writes (see the README's interoperability demo). That is the most
 * convincing proof that "content addressing" is not magic.
 * ===========================================================================
 */
#include "mygit.h"

#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <zlib.h>       /* deflate/inflate — the ONE external dependency      */

/* ---------------------------------------------------------------------------
 * zlib_deflate_all — compress `in` into a fresh heap buffer.
 *
 * We ask zlib for compressBound(in_len), the guaranteed-sufficient output size,
 * allocate that, and run compress2() at the default level. git uses zlib's
 * default compression too; the exact compressed bytes are not part of the object
 * id (the id is over the UNcompressed canonical form), so any valid zlib stream
 * round-trips correctly regardless of level. Caller frees the result.
 * --------------------------------------------------------------------------- */
unsigned char *zlib_deflate_all(const unsigned char *in, size_t in_len, size_t *out_len)
{
    uLongf bound = compressBound((uLong)in_len);  /* worst-case compressed size */
    unsigned char *out = xmalloc(bound);
    uLongf dlen = bound;

    /* compress2 -> Z_OK on success, filling `dlen` with the real size. Z_MEM/
     * Z_BUF errors are impossible here (we sized to the bound) but we check
     * anyway — silent compression failure would corrupt the store. */
    int rc = compress2(out, &dlen, in, (uLong)in_len, Z_DEFAULT_COMPRESSION);
    if (rc != Z_OK) die("zlib compress2 failed (rc=%d)", rc);

    *out_len = (size_t)dlen;
    return out;
}

/* ---------------------------------------------------------------------------
 * zlib_inflate_all — decompress a whole zlib stream of UNKNOWN output size.
 *
 * We do not know the decompressed length up front (that is precisely what the
 * object header will tell us, but we must inflate to read it). So we drive the
 * streaming inflate() in a loop, doubling the output buffer whenever zlib fills
 * it, until it reports Z_STREAM_END. This is the general pattern for "inflate
 * something whose size you cannot predict". Caller frees the result.
 * --------------------------------------------------------------------------- */
unsigned char *zlib_inflate_all(const unsigned char *in, size_t in_len, size_t *out_len)
{
    z_stream zs;
    memset(&zs, 0, sizeof zs);         /* zlib requires the struct zero-inited  */
    if (inflateInit(&zs) != Z_OK) die("zlib inflateInit failed");

    size_t cap = in_len ? in_len * 4 : 64;   /* a rough first guess            */
    unsigned char *out = xmalloc(cap);

    zs.next_in  = (Bytef *)in;
    zs.avail_in = (uInt)in_len;

    int rc;
    do {
        if (zs.total_out >= cap) {     /* output buffer full: grow and continue */
            cap *= 2;
            out = xrealloc(out, cap);
        }
        zs.next_out  = out + zs.total_out;
        zs.avail_out = (uInt)(cap - zs.total_out);

        rc = inflate(&zs, Z_NO_FLUSH);
        /* Z_OK = made progress, more to do; Z_STREAM_END = done. Anything else
         * (Z_DATA_ERROR from a corrupt object, Z_MEM_ERROR) is fatal. */
        if (rc != Z_OK && rc != Z_STREAM_END) {
            inflateEnd(&zs);
            die("zlib inflate failed (rc=%d) — corrupt object?", rc);
        }
    } while (rc != Z_STREAM_END);

    *out_len = (size_t)zs.total_out;
    inflateEnd(&zs);                   /* release zlib's internal window buffer */
    return out;
}

/* ---------------------------------------------------------------------------
 * make_canonical — build "<type> <size>\0<content>" in one heap buffer.
 *
 * Returns the buffer and its total length via *out_len. This exact byte layout
 * is what gets hashed AND what gets compressed, so both paths call this to stay
 * in agreement. The caller frees.
 * --------------------------------------------------------------------------- */
static unsigned char *make_canonical(const char *type,
                                     const unsigned char *content, size_t len,
                                     size_t *out_len)
{
    /* Header: type, space, decimal size, NUL. snprintf tells us the exact header
     * length (its return value is what it *would* have written, excluding NUL). */
    char header[64];
    int hlen = snprintf(header, sizeof header, "%s %zu", type, len);
    if (hlen < 0 || (size_t)hlen >= sizeof header)
        die("object header formatting failed");
    size_t total = (size_t)hlen + 1 /*NUL*/ + len;

    unsigned char *buf = xmalloc(total);
    memcpy(buf, header, (size_t)hlen);     /* "<type> <size>"                   */
    buf[hlen] = '\0';                      /* the header-terminating NUL        */
    memcpy(buf + hlen + 1, content, len);  /* the raw content                   */

    *out_len = total;
    return buf;
}

/* hash_object — id of the canonical form, without touching the disk. */
void hash_object(const char *type, const unsigned char *content, size_t len,
                 unsigned char oid[OID_RAWSZ])
{
    size_t clen;
    unsigned char *canon = make_canonical(type, content, len, &clen);

    sha1_ctx ctx;
    sha1_init(&ctx);
    sha1_update(&ctx, canon, clen);
    sha1_final(&ctx, oid);

    free(canon);
}

/* ---------------------------------------------------------------------------
 * object_path — fill `out` with .mygit/objects/xx/rest for a given id.
 * xx  = first two hex chars (the fan-out directory)
 * rest = the remaining 38 hex chars (the file name within it)
 * --------------------------------------------------------------------------- */
static void object_path(const unsigned char oid[OID_RAWSZ], char *out, size_t outsz)
{
    char hex[OID_HEXSZ + 1];
    hex_encode(oid, OID_RAWSZ, hex);
    int n = snprintf(out, outsz, "%s/objects/%c%c/%s",
                     GIT_DIR, hex[0], hex[1], hex + 2);
    if (n < 0 || (size_t)n >= outsz) die("object path too long");
}

/* write_object — hash, then (if new) compress and store. */
void write_object(const char *type, const unsigned char *content, size_t len,
                  unsigned char oid[OID_RAWSZ])
{
    /* 1. Canonical form -> id. */
    size_t clen;
    unsigned char *canon = make_canonical(type, content, len, &clen);

    sha1_ctx ctx;
    sha1_init(&ctx);
    sha1_update(&ctx, canon, clen);
    sha1_final(&ctx, oid);

    /* 2. Where does it live? If it already exists, we are DONE — identical bytes
     *    hash to the identical id, so the object on disk is already correct.
     *    This "write is idempotent" property is why git can be so aggressive
     *    about re-adding files: re-storing an unchanged blob costs one stat. */
    char path[4096];
    object_path(oid, path, sizeof path);
    if (path_exists(path)) {
        free(canon);
        return;
    }

    /* 3. Ensure the fan-out directory .mygit/objects/xx exists, then write the
     *    zlib-compressed canonical form to it. */
    char dir[4096];
    { char hex[OID_HEXSZ + 1];
      hex_encode(oid, OID_RAWSZ, hex);
      int n = snprintf(dir, sizeof dir, "%s/objects/%c%c", GIT_DIR, hex[0], hex[1]);
      if (n < 0 || (size_t)n >= sizeof dir) die("object dir path too long");
    }
    mkdir_p(dir);

    size_t zlen;
    unsigned char *z = zlib_deflate_all(canon, clen, &zlen);
    write_file(path, z, zlen);

    free(z);
    free(canon);
}

/* ---------------------------------------------------------------------------
 * read_object — the inverse: load, inflate, and split off the header.
 *
 * After inflation we have the canonical "<type> <size>\0<content>" back. We
 * parse the type (up to the space), the size (up to the NUL), verify the size
 * matches what actually follows, and hand back a fresh buffer of just the
 * content. Verifying the size is a cheap integrity check that catches truncation
 * or corruption. Caller frees the returned content.
 * --------------------------------------------------------------------------- */
unsigned char *read_object(const unsigned char oid[OID_RAWSZ],
                           char *type_out, size_t *size_out)
{
    char path[4096];
    object_path(oid, path, sizeof path);
    if (!path_exists(path)) {
        char hex[OID_HEXSZ + 1];
        hex_encode(oid, OID_RAWSZ, hex);
        die("object %s not found", hex);
    }

    size_t rawlen;
    unsigned char *raw = read_file(path, &rawlen);   /* the compressed bytes    */

    size_t canlen;
    unsigned char *canon = zlib_inflate_all(raw, rawlen, &canlen);
    free(raw);

    /* Parse "<type> <size>\0". Find the space, then the NUL. */
    size_t sp = 0;
    while (sp < canlen && canon[sp] != ' ') sp++;
    if (sp == canlen) die("malformed object: no type/size separator");

    size_t nul = sp + 1;
    while (nul < canlen && canon[nul] != '\0') nul++;
    if (nul == canlen) die("malformed object: no header NUL");

    /* type string (before the space) */
    size_t tlen = sp;
    if (tlen >= 8) die("object type too long");
    memcpy(type_out, canon, tlen);
    type_out[tlen] = '\0';

    /* decimal size (between space and NUL) */
    size_t size = 0;
    for (size_t i = sp + 1; i < nul; i++) {
        if (canon[i] < '0' || canon[i] > '9') die("malformed object size");
        size = size * 10 + (size_t)(canon[i] - '0');
    }

    size_t content_off = nul + 1;
    if (content_off + size != canlen)
        die("object size mismatch (header says %zu, have %zu)",
            size, canlen - content_off);

    /* Copy out just the content so the caller can free() one clean buffer. */
    unsigned char *content = xmalloc(size + 1);
    memcpy(content, canon + content_off, size);
    content[size] = '\0';
    free(canon);

    if (size_out) *size_out = size;
    return content;
}

/* ===========================================================================
 * cmd_hash_object — `mygit hash-object [-w] <file>`
 *
 * Print the object id a file's contents would get as a blob. With -w, also write
 * it into the store. This is git's lowest-level "put bytes in, get an id out".
 * =========================================================================== */
int cmd_hash_object(int argc, char **argv)
{
    int write = 0;
    const char *file = NULL;
    for (int i = 0; i < argc; i++) {
        if (strcmp(argv[i], "-w") == 0) write = 1;
        else file = argv[i];
    }
    if (!file) die("usage: mygit hash-object [-w] <file>");
    need_repo();

    size_t len;
    unsigned char *data = read_file(file, &len);

    unsigned char oid[OID_RAWSZ];
    if (write) write_object("blob", data, len, oid);
    else       hash_object ("blob", data, len, oid);
    free(data);

    char hex[OID_HEXSZ + 1];
    hex_encode(oid, OID_RAWSZ, hex);
    printf("%s\n", hex);
    return 0;
}

/* Pretty-print a tree object as git does:  "<mode> <type> <hex>\t<name>". Each
 * raw entry is "<mode> <name>\0<20 raw id bytes>"; we walk them in order. */
static void print_tree(const unsigned char *content, size_t size)
{
    size_t i = 0;
    while (i < size) {
        /* mode: ASCII up to the space */
        size_t ms = i;
        while (i < size && content[i] != ' ') i++;
        char mode[8] = {0};
        size_t mlen = i - ms;
        if (mlen >= sizeof mode) die("tree entry mode too long");
        memcpy(mode, content + ms, mlen);
        i++;   /* skip space */

        /* name: up to NUL */
        size_t ns = i;
        while (i < size && content[i] != '\0') i++;
        size_t nlen = i - ns;
        i++;   /* skip NUL */

        /* 20 raw id bytes follow */
        if (i + OID_RAWSZ > size) die("truncated tree entry");
        char hex[OID_HEXSZ + 1];
        hex_encode(content + i, OID_RAWSZ, hex);
        i += OID_RAWSZ;

        /* A "40000" mode means a subtree; anything else is a blob. git prints
         * the object type it points to, derived from the mode. */
        const char *type = (strcmp(mode, MODE_TREE) == 0) ? "tree" : "blob";
        printf("%s %s %s\t%.*s\n", mode, type, hex, (int)nlen, content + ns);
    }
}

/* ===========================================================================
 * cmd_cat_file — `mygit cat-file (-t|-s|-p) <oid>`
 *
 *   -t  print the object's type
 *   -s  print its content size in bytes
 *   -p  "pretty print": raw bytes for a blob/commit, decoded entries for a tree
 * =========================================================================== */
int cmd_cat_file(int argc, char **argv)
{
    if (argc < 2) die("usage: mygit cat-file (-t|-s|-p) <oid>");
    const char *opt = argv[0];
    const char *idhex = argv[1];
    need_repo();

    if (strlen(idhex) != OID_HEXSZ) die("not a full 40-char object id: %s", idhex);
    unsigned char oid[OID_RAWSZ];
    if (hex_decode(idhex, oid, OID_RAWSZ) != 0) die("invalid object id: %s", idhex);

    char type[8];
    size_t size;
    unsigned char *content = read_object(oid, type, &size);

    if (strcmp(opt, "-t") == 0) {
        printf("%s\n", type);
    } else if (strcmp(opt, "-s") == 0) {
        printf("%zu\n", size);
    } else if (strcmp(opt, "-p") == 0) {
        if (strcmp(type, "tree") == 0) {
            print_tree(content, size);         /* decode binary entries         */
        } else {
            /* blob/commit: emit the exact bytes. fwrite (not printf) because the
             * content may contain NULs and is not a C string. */
            fwrite(content, 1, size, stdout);
        }
    } else {
        die("unknown cat-file option: %s", opt);
    }

    free(content);
    return 0;
}
