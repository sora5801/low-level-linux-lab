/* ===========================================================================
 * main.c — node entry point: parse config, open the engine + Raft, run the loop.
 * ===========================================================================
 *
 * One process = one node. This file only wires modules together; every line of
 * real behaviour is in store.c / wal.c / raft.c / server.c. Ownership: the store
 * and raft structs live on this stack frame for the whole process lifetime, so
 * every pointer we hand into server_run() outlives the loop.
 *
 * USAGE
 * -----
 *   dbnode <id> <data_dir> <client_port> <peer_port> [peer ...]
 *       peer = id:host:peer_port:client_port
 *
 * Examples:
 *   # single node (still exercises WAL + fsync + recovery + the Raft code paths)
 *   dbnode 1 ./data/n1 7001 8001
 *
 *   # one member of a 3-node localhost cluster (node 1 of {1,2,3})
 *   dbnode 1 ./data/n1 7001 8001  2:127.0.0.1:8002:7002  3:127.0.0.1:8003:7003
 *
 * Set DB_TRACE=1 in the environment to watch elections/commits narrate on stderr.
 * ===========================================================================
 */
#include "db.h"

#include <sys/stat.h>   /* mkdir                                                */
#include <string.h>
#include <stdlib.h>
#include <stdio.h>
#include <errno.h>

/* Parse "id:host:peer_port:client_port" and register it as a peer. Returns 0 on
 * success, -1 on a malformed spec. We mutate a local copy, not argv. */
static int parse_peer(struct raft *r, const char *spec)
{
    char buf[128];
    snprintf(buf, sizeof buf, "%s", spec);

    /* id : host : peer_port : client_port  — split on ':'. */
    char *id_s   = buf;
    char *host_s = strchr(id_s, ':');   if (!host_s) return -1; *host_s++ = '\0';
    char *pp_s   = strchr(host_s, ':'); if (!pp_s)   return -1; *pp_s++   = '\0';
    char *cp_s   = strchr(pp_s, ':');   if (!cp_s)   return -1; *cp_s++   = '\0';

    int id = atoi(id_s);
    int peer_port = atoi(pp_s);
    int client_port = atoi(cp_s);
    if (id <= 0 || peer_port <= 0 || client_port <= 0 || *host_s == '\0')
        return -1;

    raft_add_peer(r, id, host_s, peer_port, client_port);
    return 0;
}

int main(int argc, char **argv)
{
    /* Tracing is opt-in so a benchmark run stays quiet, but on for the demos. */
    g_trace_enabled = (getenv("DB_TRACE") != NULL);

    if (argc < 5) {
        fprintf(stderr,
            "usage: %s <id> <data_dir> <client_port> <peer_port> "
            "[id:host:peer_port:client_port ...]\n", argv[0]);
        return 2;
    }
    int         id          = atoi(argv[1]);
    const char *data_dir    = argv[2];
    int         client_port = atoi(argv[3]);
    int         peer_port   = atoi(argv[4]);
    if (id <= 0) { fprintf(stderr, "id must be a positive integer\n"); return 2; }

    /* Create the data directory if it does not exist (idempotent). Each node
     * MUST have its own directory: store.wal, raft.log, and raft.state are all
     * per-node persistent state and must never be shared. */
    if (mkdir(data_dir, 0755) < 0 && errno != EEXIST) {
        perror("mkdir data_dir");
        return 1;
    }

    /* Open the storage engine: replays store.wal to rebuild the KV (recovery). */
    struct store store;
    if (store_open(&store, data_dir) < 0) { perror("store_open"); return 1; }

    /* Open Raft: loads persistent term/vote and replays raft.log. */
    struct raft raft;
    if (raft_open(&raft, id, data_dir, &store) < 0) {
        perror("raft_open");
        store_close(&store);
        return 1;
    }

    /* Register peers (cluster size = 1 + these). Zero peers ⇒ single-node mode. */
    for (int i = 5; i < argc; i++) {
        if (parse_peer(&raft, argv[i]) < 0)
            fprintf(stderr, "warning: ignoring malformed peer spec '%s'\n", argv[i]);
    }

    fprintf(stderr, "dbnode %d: data=%s client=:%d peer=:%d peers=%d%s\n",
            id, data_dir, client_port, peer_port, raft.npeers,
            g_trace_enabled ? "  (DB_TRACE on)" : "");

    /* Enter the event loop. Returns only on a fatal error (it otherwise runs
     * until the process is signalled). */
    int rc = server_run(&raft, client_port, peer_port);

    raft_close(&raft);
    store_close(&store);
    return rc;
}
