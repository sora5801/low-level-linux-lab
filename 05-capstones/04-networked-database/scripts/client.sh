#!/usr/bin/env bash
# =============================================================================
# client.sh — a one-shot line-protocol client (thin wrapper over nc/bash TCP).
# =============================================================================
# Usage:
#   ./scripts/client.sh <client_port> <COMMAND ...>
# Examples:
#   ./scripts/client.sh 7001 STATUS
#   ./scripts/client.sh 7001 PUT greeting "hello raft"
#   ./scripts/client.sh 7001 GET greeting
#   ./scripts/client.sh 7001 DEL greeting
#   ./scripts/client.sh 7001 ADMIN partition 2 on     # simulate a partition of peer 2
#
# The wire protocol is one text line terminated by CRLF. Replies:
#   +OK / +PONG / +role=...        success / status
#   $<len>CRLF<bytes>CRLF          a GET hit (length-prefixed value)
#   $-1                            a GET miss (nil)
#   -MOVED host:port               you hit a follower; retry on that leader
#   -ERR <reason>                  error
set -euo pipefail
port="${1:?usage: client.sh <port> <command...>}"; shift
line="$*"

# Prefer nc if present; fall back to bash's /dev/tcp. We send one CRLF-terminated
# request and print whatever the node sends back (a short timeout ends the read).
if command -v nc >/dev/null 2>&1; then
  printf '%s\r\n' "$line" | nc -q1 127.0.0.1 "$port"
else
  exec 3<>"/dev/tcp/127.0.0.1/$port"
  printf '%s\r\n' "$line" >&3
  # read for up to ~1s; the node keeps the connection open (keep-alive)
  timeout 1 cat <&3 || true
  exec 3<&- 3>&-
fi
