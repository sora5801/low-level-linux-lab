/* ===========================================================================
 * coreutils.c — the busybox-style multiplexer (argv[0] dispatch).
 * ===========================================================================
 *
 * A single binary that IS six tools. There are two ways it gets invoked:
 *
 *   1. By applet name via a symlink/hardlink:  `./cat file`   (argv[0]=="cat")
 *      This is how busybox installs: /bin/cat, /bin/ls, ... are all links to
 *      the one binary, and it looks at argv[0] to decide which tool to be.
 *
 *   2. By subcommand:  `./coreutils cat file`  (argv[0]=="coreutils")
 *      Here argv[1] selects the applet and we shift the argument vector so the
 *      applet sees a normal (argc, argv) with argv[0] == its own name.
 *
 * The dispatch table is the whole design: name -> function pointer. Adding a
 * tool is one line. `main` does argv[0] basename extraction, table lookup, and
 * the one-position shift for the subcommand form — nothing else.
 * =========================================================================== */
#include "coreutils.h"

#include <stdio.h>    /* fprintf                                               */
#include <string.h>   /* strcmp, strrchr                                       */

/* One row per tool: the name the user types and the function that implements
 * it. `const` + static so the whole table lives in .rodata (read-only, shared
 * across any forked children, never mutated at runtime). */
struct applet {
    const char *name;
    int (*fn)(int, char **);
    const char *summary;                 /* one line for the usage screen       */
};

static const struct applet applets[] = {
    { "cat",   cat_main,   "concatenate files to stdout (robust I/O loops)"      },
    { "cp",    cp_main,    "copy a file, preserving mode and sparse holes"       },
    { "ls",    ls_main,    "list a directory via getdents64 + stat, in columns"  },
    { "wc",    wc_main,    "count lines/words/bytes/chars (UTF-8 aware)"         },
    { "xargs", xargs_main, "build & run command lines under ARG_MAX, forking"    },
    { "tailf", tailf_main, "follow a file with inotify (tail -f, rotation-safe)" },
};
#define NAPPLETS (sizeof(applets) / sizeof(applets[0]))

/* Return the applet whose name matches `name`, or NULL. Linear scan over six
 * entries — a hash table would be silly here; the branch predictor eats this. */
static const struct applet *find_applet(const char *name)
{
    for (size_t i = 0; i < NAPPLETS; i++)
        if (strcmp(applets[i].name, name) == 0)
            return &applets[i];
    return NULL;
}

/* basename without mutating the input or pulling libgen's non-reentrant path:
 * return the segment after the last '/'. argv[0] can be "/usr/bin/cat" (link
 * install) or "./coreutils" (dev tree) or "cat" (PATH lookup). */
static const char *base_name(const char *path)
{
    const char *slash = strrchr(path, '/');
    return slash ? slash + 1 : path;
}

static void usage(void)
{
    fprintf(stderr, "coreutils — a teaching multiplexer. Usage:\n");
    fprintf(stderr, "  coreutils <applet> [args...]      (or symlink the binary "
                    "to the applet name)\n\n");
    fprintf(stderr, "applets:\n");
    for (size_t i = 0; i < NAPPLETS; i++)
        fprintf(stderr, "  %-6s  %s\n", applets[i].name, applets[i].summary);
}

int main(int argc, char **argv)
{
    /* argv[0] can legitimately be NULL/empty if the process was started oddly
     * (execve with an empty argv). Guard so base_name() never dereferences a
     * bad pointer. */
    const char *self = (argc > 0 && argv[0]) ? base_name(argv[0]) : "coreutils";

    /* FORM 1: invoked by applet name (symlink install). If argv[0]'s basename
     * is a known applet, become it directly — argv is already correct for the
     * tool (argv[0]=="cat", argv[1..] its args). */
    const struct applet *a = find_applet(self);
    if (a) {
        g_prog = a->name;                /* diagnostics now say e.g. "cat: ..." */
        return a->fn(argc, argv);
    }

    /* FORM 2: invoked as `coreutils <applet> ...`. Need at least argv[1]. */
    if (argc < 2) {
        usage();
        return 2;                        /* 2 == usage error, like GNU tools    */
    }

    a = find_applet(argv[1]);
    if (!a) {
        fprintf(stderr, "coreutils: unknown applet '%s'\n\n", argv[1]);
        usage();
        return 2;
    }

    /* Shift left by one so the applet sees a natural argument vector: its own
     * name in argv[0], its arguments after. We pass &argv[1] rather than
     * copying — the array is ours to view however we like, and argv[argc] is
     * guaranteed NULL by the C runtime, so the applet's argv stays NULL-
     * terminated (execvp/getopt rely on that). */
    g_prog = a->name;
    return a->fn(argc - 1, &argv[1]);
}
