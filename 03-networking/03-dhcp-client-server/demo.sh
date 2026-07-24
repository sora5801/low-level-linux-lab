#!/usr/bin/env bash
# =============================================================================
# demo.sh — a self-contained, end-to-end DORA exchange on one Linux box.
# =============================================================================
#
# We can't safely run a rogue DHCP server on your real LAN (it would fight the
# router's DHCP and hand addresses to your neighbours), so we build a private
# two-host network out of NETWORK NAMESPACES joined by a veth pair:
#
#     +-----------------+                         +-----------------+
#     |  netns dhcpsrv  |   veth-srv <==> veth-cli|  netns dhcpcli  |
#     |  192.168.50.1   |     (a virtual cable)   |  (no IP yet)    |
#     |  ./dhcp_server  |                         |  ./dhcp_client  |
#     +-----------------+                         +-----------------+
#
# The server side gets a static 192.168.50.1/24; the client side is bare — it
# must obtain its address via DHCP, which is the whole point. Both programs use
# AF_PACKET raw sockets, so they need CAP_NET_RAW; running the whole script as
# root (via `sudo make demo`) is the simplest way to grant that.
#
# Everything is cleaned up on exit, even on error, by the trap below.
# =============================================================================
set -euo pipefail

NS_SRV=dhcpsrv
NS_CLI=dhcpcli
SRV_PID=""

cleanup() {
    # Kill the backgrounded server if it is still alive, then remove the
    # namespaces (which also destroys the veth pair inside them).
    [ -n "$SRV_PID" ] && kill "$SRV_PID" 2>/dev/null || true
    ip netns pids "$NS_SRV" 2>/dev/null | xargs -r kill 2>/dev/null || true
    ip netns del "$NS_SRV" 2>/dev/null || true
    ip netns del "$NS_CLI" 2>/dev/null || true
}
trap cleanup EXIT

if [ "$(id -u)" -ne 0 ]; then
    echo "demo.sh must run as root (AF_PACKET needs CAP_NET_RAW). Try: sudo make demo" >&2
    exit 1
fi

# Clear any leftovers from a previous interrupted run.
cleanup

echo "== creating namespaces and the veth pair =="
ip netns add "$NS_SRV"
ip netns add "$NS_CLI"
# Create the pair with each end already placed in its target namespace.
ip link add veth-srv netns "$NS_SRV" type veth peer name veth-cli netns "$NS_CLI"

# Server side: give it the static server address and bring the links up.
ip netns exec "$NS_SRV" ip addr add 192.168.50.1/24 dev veth-srv
ip netns exec "$NS_SRV" ip link set veth-srv up
ip netns exec "$NS_SRV" ip link set lo up
# Client side: link up, but deliberately NO IP address.
ip netns exec "$NS_CLI" ip link set veth-cli up
ip netns exec "$NS_CLI" ip link set lo up

echo "== starting dhcp_server in netns $NS_SRV =="
ip netns exec "$NS_SRV" ./dhcp_server veth-srv &
SRV_PID=$!
sleep 1     # give the server a moment to bind its socket

echo "== running dhcp_client in netns $NS_CLI =="
ip netns exec "$NS_CLI" ./dhcp_client veth-cli

echo "== demo complete; tearing down =="
# cleanup() runs via the EXIT trap.
