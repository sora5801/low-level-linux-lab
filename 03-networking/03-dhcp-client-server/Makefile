# =============================================================================
# Makefile — build the DHCP client & server, run an end-to-end demo, and
#            regenerate the teaching assembly.
# =============================================================================
#
# Targets:
#   make            build ./dhcp_client and ./dhcp_server (Linux only)
#   make demo       spin up two network namespaces joined by a veth pair and
#                   run a real DORA exchange between them (needs root)
#   make asm        regenerate asm/demo.{O0.s,s,O2.s} from asm/demo.c (any host;
#                   clang cross-targets Linux — this is the only step that runs
#                   here on Windows/macOS)
#   make clean      remove binaries
# =============================================================================

CC      ?= cc
# Warnings are errors in spirit (CONVENTIONS §7). -O2 so the checksum loop is
# tight; -g keeps symbols for gdb/strace correlation.
CFLAGS  ?= -Wall -Wextra -O2 -g
CLANG   ?= clang

# The two programs share dhcp_common.o (the pure checksum/option/frame logic).
COMMON_SRC = dhcp_common.c
HEADERS    = dhcp.h

BINS = dhcp_client dhcp_server

.PHONY: all demo demo-teardown asm clean

all: $(BINS)

dhcp_client: dhcp_client.c $(COMMON_SRC) $(HEADERS)
	$(CC) $(CFLAGS) -o $@ dhcp_client.c $(COMMON_SRC)

dhcp_server: dhcp_server.c $(COMMON_SRC) $(HEADERS)
	$(CC) $(CFLAGS) -o $@ dhcp_server.c $(COMMON_SRC)

# --- End-to-end demo in two network namespaces (Linux, root) -----------------
# demo.sh creates the namespaces + veth pair, starts the server, runs the client
# through a full DISCOVER/OFFER/REQUEST/ACK, then tears everything down.
demo: all
	sudo ./demo.sh

demo-teardown:
	-sudo ip netns del dhcpsrv 2>/dev/null || true
	-sudo ip netns del dhcpcli 2>/dev/null || true

# --- Teaching assembly (the exact commands from CONVENTIONS §4) --------------
# Generated from the header-free asm/demo.c so it builds on any host.
asm:
	$(CLANG) --target=x86_64-pc-linux-gnu -S -O0 -fno-asynchronous-unwind-tables \
		-fno-jump-tables -fno-omit-frame-pointer asm/demo.c -o asm/demo.O0.s
	$(CLANG) --target=x86_64-pc-linux-gnu -S -O1 -fno-asynchronous-unwind-tables \
		-fno-jump-tables -fno-omit-frame-pointer asm/demo.c -o asm/demo.s
	$(CLANG) --target=x86_64-pc-linux-gnu -S -O2 -fno-asynchronous-unwind-tables \
		-fno-jump-tables asm/demo.c -o asm/demo.O2.s
	@echo "regenerated asm/demo.{O0.s,s,O2.s}; asm/demo.annotated.s is hand-written"

clean:
	rm -f $(BINS)
