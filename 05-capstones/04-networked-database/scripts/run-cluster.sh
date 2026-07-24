#!/usr/bin/env bash
# =============================================================================
# run-cluster.sh — start a 3-node localhost cluster and wait for a leader.
# =============================================================================
# Each node gets its OWN data directory (per-node WAL + raft.log + raft.state).
# Ports (client / peer):  n1 7001/8001   n2 7002/8002   n3 7003/8003.
# Logs (with DB_TRACE on) go to data/nN/node.log; PIDs to data/nN/pid.
#
# Talk to the cluster from another shell, e.g.:
#   ./scripts/client.sh 7001 STATUS
#   ./scripts/client.sh 7001 PUT greeting "hello raft"
#   ./scripts/client.sh 7001 GET greeting
# Writes must go to the leader; a follower answers "-MOVED host:port" so you know
# where to retry. Stop everything with ./scripts/stop-cluster.sh.
set -euo pipefail
cd "$(dirname "$0")/.."

BIN=./dbnode
[ -x "$BIN" ] || { echo "build first: make"; exit 1; }

# The full peer list for each node is "everyone except me".
#   peer spec = id:host:peer_port:client_port
start_node() {
  local id=$1 cport=$2 pport=$3; shift 3
  local dir="data/n$id"
  mkdir -p "$dir"
  echo "starting node $id  client=:$cport peer=:$pport  (log: $dir/node.log)"
  DB_TRACE=1 "$BIN" "$id" "$dir" "$cport" "$pport" "$@" \
      >"$dir/node.log" 2>&1 &
  echo $! >"$dir/pid"
}

start_node 1 7001 8001  2:127.0.0.1:8002:7002  3:127.0.0.1:8003:7003
start_node 2 7002 8002  1:127.0.0.1:8001:7001  3:127.0.0.1:8003:7003
start_node 3 7003 8003  1:127.0.0.1:8001:7001  2:127.0.0.1:8002:7002

echo
echo "cluster up. give it a moment to elect a leader, then:"
echo "  ./scripts/client.sh 7001 STATUS"
echo "  tail -f data/n1/node.log      # watch the election/replication trace"
echo "stop with: ./scripts/stop-cluster.sh"
