/* ===========================================================================
 * commit.c — commit objects, the HEAD/branch refs, and the history walk.
 * ===========================================================================
 *
 * A COMMIT is the smallest object but the one that ties everything together. Its
 * content is plain text:
 *
 *       tree <40-hex root-tree id>\n
 *       parent <40-hex id>\n           (zero for the first commit, one normally,
 *       parent <40-hex id>\n            two+ for a merge)
 *       author <name> <email> <epoch> <tz>\n
 *       committer <name> <email> <epoch> <tz>\n
 *       \n
 *       <commit message>\n
 *
 * hashed, like every object, as "commit <size>\0<that text>".
 *
 * REFS: A COMMIT ID IS IMMUTABLE, BUT A BRANCH IS A MOVING POINTER
 * ---------------------------------------------------------------
 * Objects never change. To make "the latest commit on master" a thing that can
 * advance, git adds a layer of MUTABLE named pointers called refs:
 *
 *     .mygit/HEAD               ->  "ref: refs/heads/master\n"   (a symbolic ref)
 *     .mygit/refs/heads/master  ->  "<40-hex commit id>\n"
 *
 * `commit` writes a new immutable commit object, then overwrites the branch ref
 * to point at it. Nothing about the old commit changes — it simply stops being
 * the branch tip. History is a chain of immutable snapshots; a branch is a
 * sticky note on one of them.
 * ===========================================================================
 */
#include "mygit.h"

#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <time.h>       /* time() for the commit timestamp                    */

#define HEAD_PATH  (GIT_DIR "/HEAD")

/* ---------------------------------------------------------------------------
 * head_ref_path — resolve HEAD to the branch ref file it points at.
 *
 * Reads .mygit/HEAD. If it is a symbolic ref ("ref: refs/heads/xxx"), we return
 * ".mygit/refs/heads/xxx" in `out`. Returns 1 on a symref, 0 if HEAD holds a raw
 * id (detached HEAD — supported for reads). This indirection is exactly how git
 * lets `commit` move a branch without HEAD itself changing.
 * --------------------------------------------------------------------------- */
static int head_ref_path(char *out, size_t outsz)
{
    size_t len;
    char *head = (char *)read_file(HEAD_PATH, &len);

    int is_symref = 0;
    if (strncmp(head, "ref: ", 5) == 0) {
        char *ref = head + 5;
        char *nl = strchr(ref, '\n');
        if (nl) *nl = '\0';                 /* trim the trailing newline        */
        int n = snprintf(out, outsz, "%s/%s", GIT_DIR, ref);
        if (n < 0 || (size_t)n >= outsz) die("ref path too long");
        is_symref = 1;
    }
    free(head);
    return is_symref;
}

/* ---------------------------------------------------------------------------
 * resolve_head — get the commit id HEAD currently names.
 *
 * Returns 0 and fills `oid` if there is a commit, or -1 if HEAD is "unborn" (the
 * branch ref does not exist yet — a fresh repo with no commits). Callers use the
 * -1 case to make the FIRST commit parentless.
 * --------------------------------------------------------------------------- */
int resolve_head(unsigned char oid[OID_RAWSZ])
{
    char refpath[4096];
    int symref = head_ref_path(refpath, sizeof refpath);

    const char *idfile = symref ? refpath : HEAD_PATH;
    if (!path_exists(idfile))
        return -1;                          /* unborn branch: no commits yet    */

    size_t len;
    char *txt = (char *)read_file(idfile, &len);
    /* The ref file is "<40 hex>\n"; ignore anything past the 40 digits. */
    if (len < OID_HEXSZ || hex_decode(txt, oid, OID_RAWSZ) != 0) {
        free(txt);
        die("ref %s is not a valid object id", idfile);
    }
    free(txt);
    return 0;
}

/* update_head — point HEAD's branch at `oid` (this is what "advances" a branch).
 * We resolve HEAD to its branch ref file, make sure refs/heads exists, and write
 * the new id. A detached HEAD (no symref) is updated in place. */
void update_head(const unsigned char oid[OID_RAWSZ])
{
    char refpath[4096];
    int symref = head_ref_path(refpath, sizeof refpath);

    char hex[OID_HEXSZ + 2];
    hex_encode(oid, OID_RAWSZ, hex);
    hex[OID_HEXSZ]     = '\n';               /* refs are newline-terminated      */
    hex[OID_HEXSZ + 1] = '\0';

    if (symref) {
        mkdir_p(GIT_DIR "/refs/heads");      /* first commit creates it          */
        write_file(refpath, hex, OID_HEXSZ + 1);
    } else {
        write_file(HEAD_PATH, hex, OID_HEXSZ + 1);
    }
}

/* ---------------------------------------------------------------------------
 * identity — build one "Name <email> <epoch> <tz>" line.
 *
 * git reads the author/committer identity from environment/config. We honor the
 * standard GIT_AUTHOR_NAME / GIT_AUTHOR_EMAIL variables and fall back to a
 * placeholder. The timestamp is Unix epoch SECONDS (UTC) from time(2); we record
 * the zone as +0000 to keep it self-consistent (epoch seconds are already UTC),
 * which is a small simplification over git computing your local offset.
 * --------------------------------------------------------------------------- */
static void identity(char *out, size_t outsz, long epoch)
{
    const char *name  = getenv("GIT_AUTHOR_NAME");
    const char *email = getenv("GIT_AUTHOR_EMAIL");
    if (!name  || !*name)  name  = "You";
    if (!email || !*email) email = "you@example.com";

    int n = snprintf(out, outsz, "%s <%s> %ld +0000", name, email, epoch);
    if (n < 0 || (size_t)n >= outsz) die("identity string too long");
}

/* build_commit — assemble the commit text and store it; return its id.
 * `nparents` may be 0 (root commit) or more; parents[] are raw ids. */
static void build_commit(const unsigned char tree[OID_RAWSZ],
                         const unsigned char parents[][OID_RAWSZ], int nparents,
                         const char *message, unsigned char out[OID_RAWSZ])
{
    long epoch = (long)time(NULL);
    char who[512];
    identity(who, sizeof who, epoch);

    char treehex[OID_HEXSZ + 1];
    hex_encode(tree, OID_RAWSZ, treehex);

    /* Grow a text buffer as we append header lines and the message. */
    size_t cap = 1024, len = 0;
    char  *buf = xmalloc(cap);

    #define APPEND(fmt, ...) do {                                             \
        char line[1024];                                                      \
        int  ln = snprintf(line, sizeof line, fmt, __VA_ARGS__);              \
        if (ln < 0) die("commit line formatting failed");                     \
        if (len + (size_t)ln + 1 > cap) {                                     \
            while (len + (size_t)ln + 1 > cap) cap *= 2;                      \
            buf = xrealloc(buf, cap);                                         \
        }                                                                     \
        memcpy(buf + len, line, (size_t)ln); len += (size_t)ln;               \
    } while (0)

    APPEND("tree %s\n", treehex);
    for (int i = 0; i < nparents; i++) {
        char ph[OID_HEXSZ + 1];
        hex_encode(parents[i], OID_RAWSZ, ph);
        APPEND("parent %s\n", ph);
    }
    APPEND("author %s\n", who);
    APPEND("committer %s\n", who);
    APPEND("%s", "\n");                       /* blank line: headers end here     */

    /* The message, guaranteed to end with exactly one newline. */
    size_t mlen = strlen(message);
    int need_nl = (mlen == 0 || message[mlen - 1] != '\n');
    if (len + mlen + 2 > cap) { while (len + mlen + 2 > cap) cap *= 2; buf = xrealloc(buf, cap); }
    memcpy(buf + len, message, mlen); len += mlen;
    if (need_nl) buf[len++] = '\n';

    #undef APPEND

    write_object("commit", (unsigned char *)buf, len, out);
    free(buf);
}

/* ===========================================================================
 * cmd_commit_tree — `mygit commit-tree <tree> [-p <parent>]... -m <msg>`
 *
 * The plumbing command: make a commit object out of an existing tree id and
 * optional parents, print the new commit id, and DO NOT touch any ref. Porcelain
 * `commit` (below) is this plus write-tree plus a branch update.
 * =========================================================================== */
int cmd_commit_tree(int argc, char **argv)
{
    need_repo();

    const char *treehex = NULL;
    const char *message = NULL;
    unsigned char parents[16][OID_RAWSZ];
    int nparents = 0;

    for (int i = 0; i < argc; i++) {
        if (strcmp(argv[i], "-p") == 0 && i + 1 < argc) {
            if (nparents >= 16) die("too many parents");
            if (hex_decode(argv[++i], parents[nparents], OID_RAWSZ) != 0)
                die("invalid parent id: %s", argv[i]);
            nparents++;
        } else if (strcmp(argv[i], "-m") == 0 && i + 1 < argc) {
            message = argv[++i];
        } else {
            treehex = argv[i];
        }
    }
    if (!treehex) die("usage: mygit commit-tree <tree> [-p parent]... -m <msg>");
    if (!message) message = "";

    unsigned char tree[OID_RAWSZ];
    if (strlen(treehex) != OID_HEXSZ || hex_decode(treehex, tree, OID_RAWSZ) != 0)
        die("invalid tree id: %s", treehex);

    unsigned char commit[OID_RAWSZ];
    build_commit(tree, parents, nparents, message, commit);

    char hex[OID_HEXSZ + 1];
    hex_encode(commit, OID_RAWSZ, hex);
    printf("%s\n", hex);
    return 0;
}

/* ===========================================================================
 * cmd_commit — `mygit commit -m <msg>`
 *
 * The everyday porcelain: snapshot the index into trees, make a commit whose
 * parent is the current HEAD (if any), and advance the branch. This is exactly
 *     write-tree  ->  commit-tree -p HEAD  ->  update-ref HEAD
 * composed into one step.
 * =========================================================================== */
int cmd_commit(int argc, char **argv)
{
    need_repo();

    const char *message = NULL;
    for (int i = 0; i < argc; i++)
        if (strcmp(argv[i], "-m") == 0 && i + 1 < argc)
            message = argv[++i];
    if (!message) die("usage: mygit commit -m <message>");

    /* 1. Snapshot the staging area. */
    index_t ix;
    index_read(&ix);
    if (ix.n == 0) die("nothing staged — use 'mygit add <file>' first");
    unsigned char tree[OID_RAWSZ];
    write_tree_from_index(&ix, tree);
    index_free(&ix);

    /* 2. Parent = whatever HEAD points at now (none for the first commit). */
    unsigned char parent[OID_RAWSZ];
    int have_parent = (resolve_head(parent) == 0);
    unsigned char (*pp)[OID_RAWSZ] = have_parent ? &parent : NULL;

    /* 3. Create the commit object. */
    unsigned char commit[OID_RAWSZ];
    build_commit(tree, (const unsigned char (*)[OID_RAWSZ])pp, have_parent ? 1 : 0,
                 message, commit);

    /* 4. Advance the branch ref to the new commit. */
    update_head(commit);

    char hex[OID_HEXSZ + 1];
    hex_encode(commit, OID_RAWSZ, hex);
    printf("[%s%s] %s\n", have_parent ? "" : "root-commit ", hex, message);
    return 0;
}

/* ---------------------------------------------------------------------------
 * commit_parent — extract the FIRST "parent <hex>" line from commit content.
 * Returns 1 and fills `parent` if present, else 0. Headers end at the first
 * blank line, so we stop there and never mistake a message line for a header.
 * --------------------------------------------------------------------------- */
static int commit_parent(const unsigned char *content, size_t size,
                         unsigned char parent[OID_RAWSZ])
{
    const char *p = (const char *)content;
    const char *end = p + size;
    while (p < end) {
        const char *nl = memchr(p, '\n', (size_t)(end - p));
        size_t linelen = nl ? (size_t)(nl - p) : (size_t)(end - p);
        if (linelen == 0) break;                     /* blank line -> body begins */
        if (linelen >= 7 && strncmp(p, "parent ", 7) == 0) {
            if (linelen >= 7 + OID_HEXSZ &&
                hex_decode(p + 7, parent, OID_RAWSZ) == 0)
                return 1;
        }
        if (!nl) break;
        p = nl + 1;
    }
    return 0;
}

/* ===========================================================================
 * cmd_log — `mygit log`
 *
 * Walk first-parent history from HEAD toward the root, printing each commit.
 * This is a pure graph walk over immutable objects: read commit, print it,
 * follow its parent id, repeat until a commit has no parent.
 * =========================================================================== */
int cmd_log(int argc, char **argv)
{
    (void)argc; (void)argv;
    need_repo();

    unsigned char oid[OID_RAWSZ];
    if (resolve_head(oid) != 0) {
        printf("(no commits yet)\n");
        return 0;
    }

    for (;;) {
        char type[8];
        size_t size;
        unsigned char *content = read_object(oid, type, &size);
        if (strcmp(type, "commit") != 0) die("HEAD does not point at a commit");

        char hex[OID_HEXSZ + 1];
        hex_encode(oid, OID_RAWSZ, hex);

        /* Print "commit <id>" then the raw header/body, indented like git. */
        printf("commit %s\n", hex);
        fwrite(content, 1, size, stdout);
        printf("\n");

        int has_parent = commit_parent(content, size, oid);
        free(content);
        if (!has_parent) break;              /* reached the root commit          */
    }
    return 0;
}
