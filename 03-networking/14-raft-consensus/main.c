/* ===========================================================================
 * main.c — the Raft test harness: build a cluster, then break the network.
 * ===========================================================================
 *
 * A consensus algorithm is only believable if you watch it survive the failures
 * it claims to survive. This driver spins up a 5-node cluster of threads sharing
 * the simulated network from net.c and runs five scenarios, each printing what it
 * expects and checking that it happened:
 *
 *   1. LEADER ELECTION      — from a cold start, exactly one leader emerges.
 *   2. LOG REPLICATION      — a client write commits and every replica converges.
 *   3. PARTITION SAFETY     — isolate the leader; the majority elects a new one
 *                             and keeps committing, while the isolated old leader
 *                             CANNOT commit. Heal, and the stale write is
 *                             overwritten — nobody ever saw two different values
 *                             at the same committed index.
 *   4. CRASH RECOVERY       — kill a node, keep committing, restart it, and watch
 *                             it rebuild from fsync'd disk state and catch up.
 *   5. SNAPSHOT/COMPACTION  — drive enough writes to trigger log compaction, then
 *                             crash+restart a node so it recovers via the snapshot
 *                             plus the log tail.
 *
 * Timing note: this is an asynchronous system with randomized timeouts, so the
 * harness WAITS for conditions (with generous bounds) rather than assuming
 * instant results. On a wildly loaded machine a bound could be missed; that is a
 * scheduling artifact, not a Raft bug. Cluster size is odd (5) so a clean
 * majority always exists.
 *
 * Platform: Linux / WSL2 (pthreads + the fsync-based persistence in persist.c).
 * No root or special capabilities needed. Build: `make`; run: `./raft` (or
 * `make run`). Each run starts from a clean state directory (rm_rf below).
 * =========================================================================== */

#include "raft.h"

#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <time.h>       /* nanosleep                                            */
#include <dirent.h>     /* opendir/readdir for rm_rf                            */
#include <sys/stat.h>   /* stat, S_ISDIR                                        */
#include <unistd.h>     /* unlink, rmdir                                        */

/* ---- tiny helpers -------------------------------------------------------- */

static int g_pass = 0, g_fail = 0;

/* Sleep for `ms` milliseconds (nanosleep restarts on EINTR would be ideal, but a
 * best-effort sleep is fine for a test harness). */
static void msleep(int ms)
{
    struct timespec ts = { ms / 1000, (long)(ms % 1000) * 1000000L };
    nanosleep(&ts, NULL);
}

/* Print a check result and tally it. */
static void check(const char *what, bool ok)
{
    printf("    [%s] %s\n", ok ? "PASS" : "FAIL", what);
    if (ok) g_pass++; else g_fail++;
}

static void banner(const char *title)
{
    printf("\n=== %s ===\n", title);
}

/* Recursively delete a directory tree so each run starts from a blank slate.
 * Uses opendir/readdir to walk entries, unlink for files, and a recursive call +
 * rmdir for subdirectories. Absent path is a no-op. */
static void rm_rf(const char *path)
{
    struct stat st;
    if (stat(path, &st) != 0) return;            /* nothing there                    */
    if (!S_ISDIR(st.st_mode)) { unlink(path); return; }
    DIR *d = opendir(path);
    if (!d) return;
    struct dirent *ent;
    while ((ent = readdir(d)) != NULL) {
        if (strcmp(ent->d_name, ".") == 0 || strcmp(ent->d_name, "..") == 0)
            continue;
        char child[512];
        snprintf(child, sizeof(child), "%s/%s", path, ent->d_name);
        rm_rf(child);                            /* recurse (files or subdirs)       */
    }
    closedir(d);
    rmdir(path);
}

/* Block until a leader exists, or the timeout expires. Returns the leader id or
 * -1. Elections take a randomized 250-500 ms, so callers pass a bound above that. */
static int wait_for_leader(struct cluster *c, int timeout_ms)
{
    int elapsed = 0;
    for (;;) {
        int L = raft_find_leader(c);
        if (L >= 0) return L;
        if (elapsed >= timeout_ms) return -1;
        msleep(20); elapsed += 20;
    }
}

/* Submit a command to whichever node is currently leader, retrying across nodes
 * and across brief leaderless windows. Returns the committed log index, or -1 on
 * timeout. `raft_submit` returns -1 from a non-leader, which is how we probe. */
static long submit_to_leader(struct cluster *c, const char *cmd, int timeout_ms)
{
    int elapsed = 0;
    for (;;) {
        int L = raft_find_leader(c);
        if (L >= 0) {
            long idx = raft_submit(c, L, cmd);
            if (idx >= 0) return idx;
        }
        if (elapsed >= timeout_ms) return -1;
        msleep(20); elapsed += 20;
    }
}

/* Wait until every node in `ids` reports `key == expected` in its local state
 * machine (i.e. the replicas converged). `crashed` marks nodes to skip. */
static bool wait_kv_converged(struct cluster *c, const int *ids, int nids,
                              const char *key, const char *expected,
                              const bool *crashed, int timeout_ms)
{
    int elapsed = 0;
    for (;;) {
        bool all = true;
        for (int j = 0; j < nids; j++) {
            int id = ids[j];
            if (crashed && crashed[id]) continue;
            char val[KV_VALLEN];
            bool ok = raft_kv_get(c, id, key, val, sizeof(val));
            if (!ok || strcmp(val, expected) != 0) { all = false; break; }
        }
        if (all) return true;
        if (elapsed >= timeout_ms) return false;
        msleep(20); elapsed += 20;
    }
}

/* ===========================================================================
 * main — run the five scenarios end to end.
 * =========================================================================== */
int main(int argc, char **argv)
{
    const char *state_dir = (argc > 1) ? argv[1] : "./raftdata";
    const int N = 5;

    /* Fresh start so the election/replication scenarios are reproducible. The
     * crash-recovery scenario re-reads whatever THIS run fsync'd, which is the
     * honest test of durability. */
    rm_rf(state_dir);

    printf("Raft consensus — teaching harness\n");
    printf("cluster: %d nodes, state dir: %s\n", N, state_dir);

    struct cluster *c = cluster_create(N, state_dir);
    if (!c) { fprintf(stderr, "cluster_create failed\n"); return 1; }
    cluster_start(c);

    int all_ids[5] = {0, 1, 2, 3, 4};

    /* ---- 1. Leader election ------------------------------------------------*/
    banner("1. Leader election");
    int leader = wait_for_leader(c, 3000);
    check("a leader was elected from a cold start", leader >= 0);
    if (leader < 0) { cluster_stop(c); return 1; }
    printf("    node %d is leader in term %llu\n",
           leader, (unsigned long long)raft_current_term(c, leader));

    /* ---- 2. Log replication ------------------------------------------------*/
    banner("2. Log replication");
    long idx = submit_to_leader(c, "SET color blue", 3000);
    check("client write accepted by the leader", idx >= 0);
    check("write committed cluster-wide", raft_wait_commit(c, idx, 3000));
    bool converged = wait_kv_converged(c, all_ids, N, "color", "blue", NULL, 3000);
    check("all replicas converged on color=blue", converged);

    /* ---- 3. Partition safety ----------------------------------------------*/
    banner("3. Partition safety (isolate the leader)");
    int old_leader = raft_find_leader(c);
    if (old_leader < 0) old_leader = leader;
    int solo[1] = { old_leader };
    net_partition(c, solo, 1);                    /* old leader alone; other 4 = majority */
    printf("    partitioned node %d away from the majority\n", old_leader);

    /* The isolated leader may still THINK it is leader briefly; a write it takes
     * can never reach a majority, so it must not commit. */
    long stale = raft_submit(c, old_leader, "SET color red");
    printf("    submitted 'SET color red' to isolated node %d (index=%ld)\n",
           old_leader, stale);

    /* The majority side must elect a new leader and keep serving. */
    int new_leader = -1, waited = 0;
    for (;;) {
        int L = raft_find_leader(c);
        if (L >= 0 && L != old_leader) { new_leader = L; break; }
        if (waited >= 4000) break;
        msleep(20); waited += 20;
    }
    check("majority elected a new leader", new_leader >= 0 && new_leader != old_leader);
    if (new_leader >= 0) {
        printf("    node %d is the new leader in term %llu\n",
               new_leader, (unsigned long long)raft_current_term(c, new_leader));
        long widx = raft_submit(c, new_leader, "SET color green");
        check("new leader accepts writes", widx >= 0);
        check("new leader's write commits on the majority",
              raft_wait_commit(c, widx, 3000));
    }

    /* The isolated node's stale write must NOT have committed there. */
    if (stale >= 0) {
        bool committed_on_isolated = raft_commit_index(c, old_leader) >= (uint64_t)stale;
        check("isolated node could NOT commit its write", !committed_on_isolated);
    }

    /* ---- Heal the partition: logs must reconcile to a single history. ------*/
    banner("3b. Heal partition (logs reconcile, stale write overwritten)");
    net_heal(c);
    printf("    network healed\n");
    /* Everyone — including the rejoining old leader — must converge on green. */
    bool healed = wait_kv_converged(c, all_ids, N, "color", "green", NULL, 5000);
    check("every node (incl. rejoined old leader) converged on color=green", healed);
    {
        char v[KV_VALLEN];
        bool got = raft_kv_get(c, old_leader, "color", v, sizeof(v));
        check("old leader's uncommitted 'red' was discarded",
              got && strcmp(v, "red") != 0);
    }

    /* ---- 4. Crash recovery -------------------------------------------------*/
    banner("4. Crash recovery (kill a follower, commit, restart it)");
    int victim = -1;
    for (int i = 0; i < N; i++) if (i != raft_find_leader(c)) { victim = i; break; }
    bool crashed[5] = { false, false, false, false, false };
    raft_crash_node(c, victim);
    crashed[victim] = true;
    printf("    crashed node %d\n", victim);

    long r1 = submit_to_leader(c, "SET k1 v1", 3000);
    long r2 = submit_to_leader(c, "SET k2 v2", 3000);
    check("cluster keeps committing without the crashed node",
          raft_wait_commit(c, r1, 3000) && raft_wait_commit(c, r2, 3000));

    raft_restart_node(c, victim);                 /* reboots purely from fsync'd disk */
    crashed[victim] = false;
    printf("    restarted node %d (state rebuilt from disk)\n", victim);
    bool caught_up =
        wait_kv_converged(c, all_ids, N, "k1", "v1", NULL, 5000) &&
        wait_kv_converged(c, all_ids, N, "k2", "v2", NULL, 5000);
    check("restarted node caught up to the committed log", caught_up);

    /* ---- 5. Snapshot / log compaction -------------------------------------*/
    banner("5. Snapshot & log compaction");
    /* Push well past RAFT_SNAPSHOT_THRESHOLD so the leader compacts its log. */
    long last_idx = -1;
    for (int i = 0; i < 25; i++) {
        char cmd[RAFT_CMD_MAX];
        snprintf(cmd, sizeof(cmd), "SET seq %d", i);
        last_idx = submit_to_leader(c, cmd, 3000);
    }
    check("burst of writes committed", raft_wait_commit(c, last_idx, 5000));
    msleep(400);                                  /* let apply + snapshot run         */
    int lead = raft_find_leader(c);
    uint64_t snap_idx = (lead >= 0) ? raft_snapshot_index(c, lead) : 0;
    printf("    leader node %d: snapshot base index = %llu, last log index = %llu\n",
           lead, (unsigned long long)snap_idx,
           (unsigned long long)(lead >= 0 ? raft_last_log_index(c, lead) : 0));
    check("leader compacted its log into a snapshot", snap_idx > 0);

    /* Crash+restart a node so recovery must go THROUGH the snapshot. */
    int v2 = -1;
    for (int i = 0; i < N; i++) if (i != lead) { v2 = i; break; }
    raft_crash_node(c, v2);
    msleep(100);
    raft_restart_node(c, v2);
    printf("    crashed and restarted node %d (recovers via snapshot + tail)\n", v2);
    {
        char cmd[RAFT_CMD_MAX];
        snprintf(cmd, sizeof(cmd), "SET seq final");
        long fi = submit_to_leader(c, cmd, 3000);
        raft_wait_commit(c, fi, 3000);
    }
    bool snap_recovered = wait_kv_converged(c, all_ids, N, "seq", "final", NULL, 5000);
    check("node recovered from snapshot and reconverged", snap_recovered);

    /* ---- shutdown ----------------------------------------------------------*/
    banner("Summary");
    printf("    %d passed, %d failed\n", g_pass, g_fail);
    cluster_stop(c);
    return g_fail == 0 ? 0 : 1;
}
