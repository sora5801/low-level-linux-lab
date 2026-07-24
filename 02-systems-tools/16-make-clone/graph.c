/* ===========================================================================
 * graph.c — build the dependency DAG, stat(2) the files, decide staleness,
 * and detect cycles.
 * ===========================================================================
 *
 * After parse.c we have a flat list of `rule`s. This file turns them into a
 * graph of `node`s (one per distinct target/prerequisite name) with edges in
 * both directions:
 *
 *     node.deps[]        target -> prerequisite  ("I must be built AFTER these")
 *     node.dependents[]  prerequisite -> target  ("building me may unblock these")
 *
 * The forward edges give the build ORDER; the reverse edges let the parallel
 * scheduler, when a node finishes, cheaply decrement the "unbuilt prerequisite"
 * counter of everyone waiting on it (that counter is Kahn's in-degree, run live).
 *
 * STALENESS is make's whole reason to exist: a target is rebuilt only if it is
 * older than something it depends on. We compute it bottom-up (post-order) so a
 * prerequisite that gets rebuilt correctly forces its dependents to rebuild too.
 * ===========================================================================
 */
#include "mk.h"

#include <string.h>    /* strcmp                                             */
#include <stdio.h>     /* fprintf                                            */
#include <errno.h>     /* errno, ENOENT, ENOTDIR                             */
#include <sys/stat.h>  /* stat, struct stat                                  */

/* ===========================================================================
 * NODE TABLE
 * ===========================================================================
 */

/* find-or-create a node by name. Linear scan (fine for teaching-sized graphs).
 * IMPORTANT: this may realloc m->nodes, so callers must not hold node pointers
 * across a call to mk_node_get — resolve names to pointers only once the table
 * has stopped growing (see mk_graph_build's two phases). */
node *mk_node_get(mk *m, const char *name)
{
    for (size_t i = 0; i < m->nnode; i++)
        if (strcmp(m->nodes[i]->name, name) == 0)
            return m->nodes[i];

    if (m->nnode == m->nodecap) {
        m->nodecap = m->nodecap ? m->nodecap * 2 : 16;
        m->nodes   = xrealloc(m->nodes, m->nodecap * sizeof *m->nodes);
    }
    node *n = xcalloc(1, sizeof *n);   /* zero-init: color WHITE, state WAITING,  */
    n->name  = xstrdup(name);          /*   mtime 0, all counters 0               */
    n->mtime = MK_TIME_MISSING;
    m->nodes[m->nnode++] = n;
    return n;
}

/* Is `dep` already a prerequisite of `n`? Used to de-duplicate edges so a
 * target listed in several rules does not get counted twice (which would leave
 * its unbuilt_deps counter permanently above zero). */
static int node_has_dep(node *n, node *dep)
{
    for (size_t i = 0; i < n->ndep; i++)
        if (n->deps[i] == dep) return 1;
    return 0;
}

static void node_add_dep(node *n, node *dep)
{
    n->deps = xrealloc(n->deps, (n->ndep + 1) * sizeof *n->deps);
    n->deps[n->ndep++] = dep;
    dep->dependents = xrealloc(dep->dependents,
                               (dep->ndependent + 1) * sizeof *dep->dependents);
    dep->dependents[dep->ndependent++] = n;
}

/* ===========================================================================
 * mk_graph_build — rules -> nodes + edges, with .PHONY handling.
 * ===========================================================================
 * Done in two phases precisely because mk_node_get can realloc the node array:
 *   Phase 1 CREATES every node (targets and prerequisites) and attaches
 *           recipes; after it, the set of nodes — and thus their addresses —
 *           is final.
 *   Phase 2 RESOLVES prerequisite names to the now-stable node pointers and
 *           wires up the forward/reverse edges.
 */
void mk_graph_build(mk *m)
{
    /* ---- Phase 1: create all nodes, attach recipes, note .PHONY names ---- */
    for (size_t ri = 0; ri < m->nrule; ri++) {
        rule *r = &m->rules[ri];

        /* The special target ".PHONY" does not create a normal rule; its
         * prerequisites are the names to mark phony. We defer marking until the
         * nodes exist (they might be created by a later rule), so just ensure
         * each named node exists here and mark it below. */
        int is_phony_decl = 0;
        for (size_t t = 0; t < r->ntarget; t++)
            if (strcmp(r->targets[t], ".PHONY") == 0) is_phony_decl = 1;

        if (is_phony_decl) {
            for (size_t pj = 0; pj < r->nprereq; pj++)
                mk_node_get(m, r->prereqs[pj])->is_phony = 1;
            continue;                  /* .PHONY contributes no build edges       */
        }

        for (size_t t = 0; t < r->ntarget; t++) {
            node *n = mk_node_get(m, r->targets[t]);
            /* Attach this rule's recipe to the target. GNU make allows many
             * rules to add prerequisites but only ONE to supply the recipe; a
             * second recipe "overrides" the first (with a warning). */
            if (r->nrecipe > 0) {
                if (n->nrecipe > 0)
                    fprintf(stderr,
                            "mmake: warning: overriding recipe for target '%s'\n",
                            n->name);
                n->recipe  = r->recipe;    /* borrow the rule's array (not owned) */
                n->nrecipe = r->nrecipe;
            }
        }
        /* Also make sure every prerequisite has a node (a source file with no
         * rule still needs one so we can stat it). */
        for (size_t pj = 0; pj < r->nprereq; pj++)
            mk_node_get(m, r->prereqs[pj]);
    }

    /* ---- Phase 2: wire edges now that node addresses are stable ---- */
    for (size_t ri = 0; ri < m->nrule; ri++) {
        rule *r = &m->rules[ri];

        int is_phony_decl = 0;
        for (size_t t = 0; t < r->ntarget; t++)
            if (strcmp(r->targets[t], ".PHONY") == 0) is_phony_decl = 1;
        if (is_phony_decl) continue;

        for (size_t t = 0; t < r->ntarget; t++) {
            node *n = mk_node_get(m, r->targets[t]);
            for (size_t pj = 0; pj < r->nprereq; pj++) {
                node *dep = mk_node_get(m, r->prereqs[pj]);
                if (dep == n) continue;            /* ignore trivial self-edge   */
                if (!node_has_dep(n, dep))
                    node_add_dep(n, dep);          /* forward + reverse edge      */
            }
        }
    }

    /* The live in-degree the scheduler consumes starts equal to the number of
     * (de-duplicated) prerequisites. */
    for (size_t i = 0; i < m->nnode; i++)
        m->nodes[i]->unbuilt_deps = m->nodes[i]->ndep;
}

/* ===========================================================================
 * CYCLE DETECTION — three-color DFS.
 * ===========================================================================
 * WHITE: unseen. GRAY: on the current recursion stack. BLACK: fully explored.
 * Descending into a GRAY node means we found a back edge, i.e. a cycle: a set
 * of targets that (transitively) depend on themselves and can never be ordered.
 * GNU make prints this and drops the edge; we treat it as a hard error because
 * a cyclic build has no valid order to run.
 */
static int dfs_cycle(node *n)
{
    if (n->color == NODE_BLACK) return 0;   /* already proven acyclic           */
    if (n->color == NODE_GRAY) return -1;   /* back edge -> cycle               */

    n->color = NODE_GRAY;                   /* enter: put n on the DFS stack     */
    for (size_t i = 0; i < n->ndep; i++) {
        if (dfs_cycle(n->deps[i]) != 0) {
            /* Unwind, naming the edge we were on. The messages stack up from the
             * deepest offender outward, mirroring make's diagnostic. */
            fprintf(stderr,
                    "mmake: circular %s <- %s dependency dropped\n",
                    n->name, n->deps[i]->name);
            return -1;
        }
    }
    n->color = NODE_BLACK;                   /* leave: fully explored            */
    return 0;
}

int mk_graph_check_cycles(mk *m, node *goal)
{
    for (size_t i = 0; i < m->nnode; i++)   /* reset coloring before the walk    */
        m->nodes[i]->color = NODE_WHITE;
    return dfs_cycle(goal);
}

/* ===========================================================================
 * STAT PASS — get every file's modification time.
 * ===========================================================================
 * Syscall: stat(2) (x86-64 number 4). stat(path, &st) fills a struct stat; we
 * only need st_mtim (last-modification timestamp, seconds + nanoseconds). We
 * fold the two fields into one ns count (see mk_time in mk.h) so staleness is a
 * single integer comparison.
 *
 * Error handling: ENOENT ("no such file") and ENOTDIR (a path component is not
 * a directory) simply mean "this target is not on disk yet" — completely normal
 * for something we are about to build — so we record it as MISSING rather than
 * failing. Any OTHER stat error (e.g. EACCES) is a real problem and aborts.
 */
void mk_graph_stat(mk *m)
{
    for (size_t i = 0; i < m->nnode; i++) {
        node *n = m->nodes[i];
        if (n->is_phony) {             /* a phony target never corresponds to a  */
            n->mtime = MK_TIME_MISSING;/*   file; leave it MISSING => always     */
            continue;                  /*   stale.                               */
        }
        struct stat st;
        if (stat(n->name, &st) == 0) {
            /* seconds * 1e9 + nanoseconds, in a signed 64-bit ns counter. */
            n->mtime = (mk_time)st.st_mtim.tv_sec * 1000000000LL
                     + (mk_time)st.st_mtim.tv_nsec;
        } else if (errno == ENOENT || errno == ENOTDIR) {
            n->mtime = MK_TIME_MISSING;/* not built yet — expected, not an error */
        } else {
            die("stat %s", n->name);   /* EACCES/ELOOP/... is a genuine failure  */
        }
    }
}

/* ===========================================================================
 * STALENESS — the core make decision, computed post-order.
 * ===========================================================================
 * `needs_rebuild` is 1 when a target's recipe must run. Bottom-up so that a
 * rebuilt prerequisite propagates: if X depends on Y and Y rebuilds, X is stale
 * even if X's file currently looks newer than Y's on disk.
 *
 * A node is stale when ANY of:
 *   -B was given (force everything);
 *   it is phony (no file: run every time);
 *   its file is missing;
 *   a prerequisite is stale (will be rebuilt);
 *   a prerequisite's mtime is strictly newer than ours.
 *
 * Guard: a node with no recipe, no prerequisites, and no file is a SOURCE that
 * does not exist — the classic "No rule to make target 'x'" error.
 */
static void compute(mk *m, node *n)
{
    if (n->staleness_done) return;     /* memoized: compute each node once        */
    n->staleness_done = 1;

    for (size_t i = 0; i < n->ndep; i++)   /* prerequisites first (post-order)    */
        compute(m, n->deps[i]);

    if (n->nrecipe == 0 && n->ndep == 0 && !n->is_phony
        && n->mtime == MK_TIME_MISSING) {
        errno = 0;                     /* not a syscall failure; suppress strerror */
        die("No rule to make target '%s'", n->name);
    }

    int stale = m->always_make || n->is_phony || (n->mtime == MK_TIME_MISSING);

    for (size_t i = 0; i < n->ndep; i++) {
        node *d = n->deps[i];
        if (d->needs_rebuild) stale = 1;               /* prereq will rebuild     */
        else if (d->mtime > n->mtime) stale = 1;       /* prereq strictly newer   */
    }

    n->needs_rebuild = stale;
}

void mk_compute_staleness(mk *m, node *goal)
{
    for (size_t i = 0; i < m->nnode; i++) {
        m->nodes[i]->staleness_done = 0;
        m->nodes[i]->needs_rebuild  = 0;
    }
    compute(m, goal);
}
