/* ===========================================================================
 * main.c — argument dispatch and `mygit init`.
 * ===========================================================================
 *
 * This is the thin front door. It maps the first argument to a subcommand and
 * forwards the rest. `init` lives here because it is the one command that
 * CREATES the repository the others require.
 *
 * The command surface, from plumbing (low-level) to porcelain (everyday):
 *
 *     init                         create an empty .mygit repository
 *     hash-object [-w] <file>      id of a file's contents (write with -w)
 *     cat-file (-t|-s|-p) <oid>    inspect an object: type / size / contents
 *     add <path>...                stage files into the index
 *     write-tree                   snapshot the index into tree objects
 *     commit-tree <tree> ... -m    make a commit object from a tree
 *     commit -m <msg>              add-nothing porcelain: tree+commit+move branch
 *     log                          walk history from HEAD
 *     status                       working tree vs index summary
 *     diff                         working tree vs index, line by line
 *
 * Every subcommand returns an int exit status that becomes the process's status.
 * ===========================================================================
 */
#include "mygit.h"

#include <stdio.h>
#include <string.h>

/* ---------------------------------------------------------------------------
 * cmd_init — `mygit init`
 *
 * Lay down the minimum on-disk skeleton the rest of the program assumes:
 *
 *     .mygit/objects/          where content-addressed objects live
 *     .mygit/refs/heads/       where branch pointers live
 *     .mygit/HEAD              a symbolic ref naming the current branch
 *
 * We point HEAD at refs/heads/master even though that ref file does not exist
 * yet — that is a valid "unborn branch", and it is precisely what resolve_head()
 * detects to make the first commit parentless. Creating a repo touches no object
 * store; objects appear only when you hash/add/commit.
 * --------------------------------------------------------------------------- */
static int cmd_init(int argc, char **argv)
{
    (void)argc; (void)argv;

    if (is_dir(GIT_DIR))
        die("%s already exists here — repository already initialized", GIT_DIR);

    /* mkdir_p creates every parent, so these two calls suffice to build the
     * whole tree (.mygit and .mygit/objects share a parent). */
    mkdir_p(GIT_DIR "/objects");
    mkdir_p(GIT_DIR "/refs/heads");

    /* HEAD is a symbolic ref: "ref: <path>\n". This indirection is what lets a
     * commit move a branch without HEAD's own contents changing. */
    static const char head[] = "ref: refs/heads/master\n";
    write_file(GIT_DIR "/HEAD", head, sizeof head - 1);

    printf("Initialized empty mygit repository in %s/\n", GIT_DIR);
    return 0;
}

/* Print the one-line usage/command list to stderr. */
static void usage(void)
{
    fprintf(stderr,
        "usage: mygit <command> [args]\n"
        "\n"
        "  init                         create an empty .mygit repository\n"
        "  hash-object [-w] <file>      compute (and with -w, store) a blob id\n"
        "  cat-file (-t|-s|-p) <oid>    show an object's type/size/contents\n"
        "  add <path>...                stage files into the index\n"
        "  write-tree                   write tree objects from the index\n"
        "  commit-tree <tree> [-p p] -m msg   create a commit object\n"
        "  commit -m <message>          snapshot the index as a new commit\n"
        "  log                          show history from HEAD\n"
        "  status                       working tree vs index summary\n"
        "  diff                         working tree vs index, line by line\n");
}

/* ---------------------------------------------------------------------------
 * main — dispatch. argv[1] is the subcommand; argv+2.. are its arguments, which
 * we pass through as (argc-2, argv+2) so each cmd_* sees a clean argument list.
 * --------------------------------------------------------------------------- */
int main(int argc, char **argv)
{
    if (argc < 2) { usage(); return 2; }

    const char *cmd = argv[1];
    int   sub_argc  = argc - 2;
    char **sub_argv = argv + 2;

    if (strcmp(cmd, "init")        == 0) return cmd_init(sub_argc, sub_argv);
    if (strcmp(cmd, "hash-object") == 0) return cmd_hash_object(sub_argc, sub_argv);
    if (strcmp(cmd, "cat-file")    == 0) return cmd_cat_file(sub_argc, sub_argv);
    if (strcmp(cmd, "add")         == 0) return cmd_add(sub_argc, sub_argv);
    if (strcmp(cmd, "write-tree")  == 0) return cmd_write_tree(sub_argc, sub_argv);
    if (strcmp(cmd, "commit-tree") == 0) return cmd_commit_tree(sub_argc, sub_argv);
    if (strcmp(cmd, "commit")      == 0) return cmd_commit(sub_argc, sub_argv);
    if (strcmp(cmd, "log")         == 0) return cmd_log(sub_argc, sub_argv);
    if (strcmp(cmd, "status")      == 0) return cmd_status(sub_argc, sub_argv);
    if (strcmp(cmd, "diff")        == 0) return cmd_diff(sub_argc, sub_argv);

    fprintf(stderr, "mygit: unknown command '%s'\n\n", cmd);
    usage();
    return 2;
}
