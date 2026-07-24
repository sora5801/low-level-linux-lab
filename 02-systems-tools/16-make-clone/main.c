/* ===========================================================================
 * main.c — command-line driver for mmake, the teaching make(1) clone.
 * ===========================================================================
 *
 * Wires the pipeline together:
 *     args ─► read makefile ─► parse ─► build graph ─► per goal:
 *                stat -> cycle check -> staleness -> build (fork/exec, -jN)
 *
 * Supported flags (a practical subset of make's):
 *     -f FILE   read FILE instead of ./Makefile (or ./makefile)
 *     -jN, -j N run up to N recipe jobs in parallel (default 1 = serial)
 *     -n        dry run: print recipes, execute nothing
 *     -s        silent: never echo recipe lines
 *     -k        keep going: on failure, build whatever still can be built
 *     -B        always-make: treat every target as out of date
 *     -q        question: exit 1 iff a goal is out of date, build nothing
 *     -C DIR    chdir to DIR before doing anything
 *     -h        help
 * Plus `VAR=value` command-line variable overrides and one or more goal names.
 * ===========================================================================
 */
#include "mk.h"

#include <stdio.h>     /* fprintf, printf                                    */
#include <stdlib.h>    /* exit, atoi                                         */
#include <string.h>    /* strcmp, strchr, strlen, memset                     */
#include <ctype.h>     /* isdigit                                            */
#include <errno.h>     /* errno (cleared to 0 before non-syscall die() calls)*/
#include <unistd.h>    /* chdir, access                                      */

/* An override captured from the command line: `VAR=value`. These win over the
 * makefile's own assignments (applied both before AND after parsing — before so
 * a `?=` sees them as already-set, after so a plain `=` cannot clobber them). */
typedef struct { char *name, *value; } cli_var;

static void usage(const char *prog)
{
    fprintf(stderr,
        "usage: %s [-f FILE] [-jN] [-n] [-s] [-k] [-B] [-q] [-C DIR]"
        " [VAR=val ...] [target ...]\n", prog);
}

/* Is the whole string a run of decimal digits? Used to decide whether the token
 * after a bare `-j` is its numeric argument. */
static int all_digits(const char *s)
{
    if (!*s) return 0;
    for (; *s; s++) if (!isdigit((unsigned char)*s)) return 0;
    return 1;
}

/* ===========================================================================
 * mk_free — release everything the mk structure owns.
 * ===========================================================================
 * Ownership recap: rules own their target/prereq/recipe STRINGS; a node's
 * `recipe` pointer merely BORROWS a rule's recipe array, so we must not free it
 * through the node. Nodes own their name and their deps/dependents ARRAYS.
 */
void mk_free(mk *m)
{
    for (size_t i = 0; i < m->nvar; i++) {
        free(m->vars[i].name);
        free(m->vars[i].value);
    }
    free(m->vars);

    for (size_t i = 0; i < m->nrule; i++) {
        rule *r = &m->rules[i];
        for (size_t j = 0; j < r->ntarget; j++) free(r->targets[j]);
        for (size_t j = 0; j < r->nprereq; j++) free(r->prereqs[j]);
        for (size_t j = 0; j < r->nrecipe; j++) free(r->recipe[j]);
        free(r->targets);
        free(r->prereqs);
        free(r->recipe);
    }
    free(m->rules);

    for (size_t i = 0; i < m->nnode; i++) {
        node *n = m->nodes[i];
        free(n->name);
        free(n->deps);          /* array only; the nodes it points to are freed  */
        free(n->dependents);    /*   by their own iteration                       */
        /* n->recipe is borrowed from a rule — do NOT free it here.               */
        free(n);
    }
    free(m->nodes);
}

/* Build a single goal end to end. Returns a process exit code (0 = success). */
static int build_one_goal(mk *m, const char *name)
{
    node *goal = mk_node_get(m, name);

    /* Reset the per-build scheduler scratch so multiple goals don't interfere. */
    for (size_t i = 0; i < m->nnode; i++) {
        m->nodes[i]->reachable = 0;
        m->nodes[i]->state     = BUILD_WAITING;
    }

    if (mk_graph_check_cycles(m, goal) != 0) {
        errno = 0;
        die("dependency graph has a cycle involving '%s'", name);
    }

    mk_graph_stat(m);                  /* fresh mtimes (files may have just built)*/
    mk_compute_staleness(m, goal);

    if (m->question)                   /* -q: report staleness, build nothing     */
        return goal->needs_rebuild ? 1 : 0;

    if (!goal->needs_rebuild) {
        printf("mmake: '%s' is up to date.\n", goal->name);
        return 0;
    }

    return mk_build(m, goal);
}

int main(int argc, char **argv)
{
    mk m;
    memset(&m, 0, sizeof m);
    m.jobs = 1;                        /* serial unless -j says otherwise         */

    const char *makefile = NULL;
    const char *chdir_to = NULL;

    cli_var *cvars = NULL; size_t ncvar = 0, cvarcap = 0;
    char   **goals = NULL; size_t ngoal = 0, goalcap = 0;

    /* ---- 1. Parse the command line ---- */
    for (int i = 1; i < argc; i++) {
        char *a = argv[i];

        if (a[0] == '-' && a[1] != '\0') {
            char opt = a[1];
            switch (opt) {
            case 'f':                  /* -f FILE or -fFILE                       */
                makefile = a[2] ? a + 2 : (++i < argc ? argv[i] : NULL);
                if (!makefile) { usage(argv[0]); return 2; }
                break;
            case 'C':                  /* -C DIR or -CDIR                         */
                chdir_to = a[2] ? a + 2 : (++i < argc ? argv[i] : NULL);
                if (!chdir_to) { usage(argv[0]); return 2; }
                break;
            case 'j':                  /* -jN, -j N, or bare -j (unlimited-ish)   */
                if (a[2]) {
                    m.jobs = atoi(a + 2);
                } else if (i + 1 < argc && all_digits(argv[i + 1])) {
                    m.jobs = atoi(argv[++i]);
                } else {
                    m.jobs = 256;      /* bare -j: "unlimited", capped for the pool */
                }
                if (m.jobs < 1) m.jobs = 1;
                break;
            case 'n': m.dry_run     = 1; break;
            case 's': m.silent      = 1; break;
            case 'k': m.keep_going  = 1; break;
            case 'B': m.always_make = 1; break;
            case 'q': m.question    = 1; break;
            case 'h': usage(argv[0]); return 0;
            default:
                fprintf(stderr, "mmake: unknown option -%c\n", opt);
                usage(argv[0]);
                return 2;
            }
            continue;
        }

        /* A `VAR=value` token is a command-line variable override. We detect the
         * '=' the same loose way the parser does (first '=' not preceded by ':').*/
        char *eq = strchr(a, '=');
        if (eq && eq != a) {
            if (ncvar == cvarcap) {
                cvarcap = cvarcap ? cvarcap * 2 : 8;
                cvars   = xrealloc(cvars, cvarcap * sizeof *cvars);
            }
            *eq = '\0';
            cvars[ncvar].name  = a;    /* points into argv (stable for the run)   */
            cvars[ncvar].value = eq + 1;
            ncvar++;
            continue;
        }

        /* Otherwise it is a goal (target) name. */
        if (ngoal == goalcap) {
            goalcap = goalcap ? goalcap * 2 : 8;
            goals   = xrealloc(goals, goalcap * sizeof *goals);
        }
        goals[ngoal++] = a;
    }

    /* ---- 2. Change directory first, so -f and stat see the right tree ---- */
    if (chdir_to && chdir(chdir_to) < 0)
        die("chdir %s", chdir_to);

    /* ---- 3. Locate the makefile ---- */
    if (!makefile) {
        if      (access("Makefile", R_OK) == 0) makefile = "Makefile";
        else if (access("makefile", R_OK) == 0) makefile = "makefile";
        else { errno = 0; die("no makefile found (looked for Makefile, makefile)"); }
    }

    /* ---- 4. A couple of convenient built-in variables ---- */
    mk_var_set(&m, "MAKE", argv[0], VAR_SIMPLE);   /* so $(MAKE) recurses to us   */

    /* Apply command-line overrides BEFORE parsing so `?=` respects them. */
    for (size_t i = 0; i < ncvar; i++)
        mk_var_set(&m, cvars[i].name, cvars[i].value, VAR_SIMPLE);

    /* ---- 5. Read + parse the makefile ---- */
    size_t len;
    char  *text = read_file(makefile, &len);
    mk_parse(&m, text);
    free(text);

    /* Re-apply overrides AFTER parsing so a plain `=` in the file cannot win. */
    for (size_t i = 0; i < ncvar; i++)
        mk_var_set(&m, cvars[i].name, cvars[i].value, VAR_SIMPLE);

    /* ---- 6. Build the graph ---- */
    mk_graph_build(&m);
    mk_install_signal_handlers();

    /* ---- 7. Choose goals: explicit args, else the file's first target ---- */
    if (ngoal == 0) {
        const char *def = NULL;
        for (size_t i = 0; i < m.nrule && !def; i++)
            for (size_t t = 0; t < m.rules[i].ntarget; t++)
                if (strcmp(m.rules[i].targets[t], ".PHONY") != 0) {
                    def = m.rules[i].targets[t];
                    break;
                }
        if (!def) { errno = 0; die("no targets. Stop."); }
        if (ngoal == goalcap) { goalcap = 8; goals = xrealloc(goals, 8 * sizeof *goals); }
        goals[ngoal++] = (char *)def;
    }

    /* ---- 8. Build each goal in turn; the worst exit code wins ---- */
    int rc = 0;
    for (size_t i = 0; i < ngoal; i++) {
        int r = build_one_goal(&m, goals[i]);
        if (r > rc) rc = r;
        if (r != 0 && !m.keep_going) break;   /* stop at the first failed goal    */
    }

    free(cvars);
    free(goals);
    mk_free(&m);
    return rc;
}
