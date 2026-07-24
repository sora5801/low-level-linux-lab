#!/usr/bin/env bash
# =============================================================================
# partition-demo.sh — the injected-fault demo: partition the leader, watch Raft
# elect a new one, keep writing, then heal and confirm the cluster reconciles.
# =============================================================================
# This is the payoff of the whole capstone. It shows the three Raft guarantees in
# action: a partitioned old leader CANNOT commit (no majority), the majority side
# elects a NEW leader in a higher term and keeps serving, and on heal the stale
# leader steps down and its log is overwritten to match — no acknowledged write
# is ever lost.
#
# HOW THE PARTITION IS INJECTED
# -----------------------------
# We do NOT need root/iptables. Each node has an ADMIN command that toggles a
# per-peer "drop all traffic" flag (see server.c). To isolate leader L we tell L
# to drop both peers AND tell both peers to drop L — a clean two-way partition,
# entirely in userspace. (A production test would use `iptables`/`tc netem`; the
# effect on Raft is identical. This is called out honestly in the README.)
set -uo pipefail
cd "$(dirname "$0")/.."

BIN=./dbnode
[ -x "$BIN" ] || { echo "build first: make"; exit 1; }

port_of() { echo $((7000 + $1)); }          # node id -> client port
cli()     { ./scripts/client.sh "$1" "${@:2}"; }   # cli <port> <command...>

# Parse "leader=<id>" out of a node's STATUS reply. Returns 0..N or empty.
leader_from() { cli "$1" STATUS 2>/dev/null | sed -n 's/.*leader=\([0-9-]*\).*/\1/p'; }

find_leader() {                              # echo the current leader id (or nothing)
  for id in 1 2 3; do
    local l; l=$(leader_from "$(port_of "$id")")
    if [ -n "${l:-}" ] && [ "$l" -ge 1 ] 2>/dev/null; then echo "$l"; return; fi
  done
}

echo "== 1. starting a fresh 3-node cluster =="
make -s distclean >/dev/null 2>&1 || true
./scripts/run-cluster.sh
echo "   waiting for the first election..."
LEADER=""; for _ in $(seq 1 20); do sleep 0.5; LEADER=$(find_leader); [ -n "$LEADER" ] && break; done
[ -n "$LEADER" ] || { echo "no leader elected — check data/nN/node.log"; exit 1; }
echo "   leader is node $LEADER"

echo "== 2. write a value through the leader =="
cli "$(port_of "$LEADER")" PUT k1 "before-partition"
cli "$(port_of "$LEADER")" GET k1

echo "== 3. PARTITION the leader (node $LEADER) from the other two =="
OTHERS=(); for id in 1 2 3; do [ "$id" != "$LEADER" ] && OTHERS+=("$id"); done
for o in "${OTHERS[@]}"; do
  cli "$(port_of "$LEADER")" ADMIN partition "$o" on >/dev/null   # L drops each peer
  cli "$(port_of "$o")"      ADMIN partition "$LEADER" on >/dev/null # each peer drops L
done
echo "   node $LEADER is now isolated; the majority side is nodes ${OTHERS[*]}"

echo "== 4. wait for the majority side to elect a NEW leader =="
NEW=""; for _ in $(seq 1 20); do
  sleep 0.5
  for o in "${OTHERS[@]}"; do
    l=$(leader_from "$(port_of "$o")")
    if [ "${l:-}" = "$o" ]; then NEW=$o; break; fi
  done
  [ -n "$NEW" ] && break
done
[ -n "$NEW" ] || { echo "no new leader — check logs"; ./scripts/stop-cluster.sh; exit 1; }
echo "   new leader is node $NEW (higher term); old leader $LEADER can't commit"

echo "== 5. the isolated old leader must REFUSE writes (no majority) =="
echo -n "   write to old leader $LEADER -> "; cli "$(port_of "$LEADER")" PUT k2 "should-fail"
echo "== 5b. the new leader accepts writes normally =="
cli "$(port_of "$NEW")" PUT k2 "after-partition"
cli "$(port_of "$NEW")" GET k2

echo "== 6. HEAL the partition =="
for o in "${OTHERS[@]}"; do
  cli "$(port_of "$LEADER")" ADMIN partition "$o" off >/dev/null
  cli "$(port_of "$o")"      ADMIN partition "$LEADER" off >/dev/null
done
sleep 2
echo "   old leader $LEADER should now be a follower and have caught up:"
cli "$(port_of "$LEADER")" STATUS
echo -n "   k2 as seen after redirect from old leader: "
cli "$(port_of "$LEADER")" GET k2      # a follower answers -MOVED -> the new leader

echo
echo "== done. Inspect the narration:  tail -n 40 data/n*/node.log =="
echo "   stop the cluster with:        ./scripts/stop-cluster.sh"
