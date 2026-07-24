#!/usr/bin/env bash
# =============================================================================
# stop-cluster.sh — stop every node started by run-cluster.sh.
# =============================================================================
# Sends SIGTERM to each recorded PID. Persistent state (data/nN/*.wal, raft.log,
# raft.state) is LEFT ON DISK on purpose: restart the cluster and each node
# recovers its KV by replaying store.wal and its Raft log — that is the whole
# durability lesson. Use `make distclean` to wipe state for a fresh start.
set -uo pipefail
cd "$(dirname "$0")/.."

for id in 1 2 3; do
  pidf="data/n$id/pid"
  if [ -f "$pidf" ]; then
    pid=$(cat "$pidf")
    if kill "$pid" 2>/dev/null; then
      echo "stopped node $id (pid $pid)"
    fi
    rm -f "$pidf"
  fi
done
