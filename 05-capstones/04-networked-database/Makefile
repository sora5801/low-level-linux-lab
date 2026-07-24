# =============================================================================
# Makefile — the networked-database teaching core.
# =============================================================================
# Build on Linux (or WSL2): the node uses epoll, timerfd, accept4, and eventfd-
# style primitives that are Linux-only. It will NOT build on Windows or macOS.
# On ANY host you can still regenerate the teaching assembly with `make asm`,
# because clang cross-compiles to the Linux target without a Linux toolchain.

CC      ?= clang
# -Wall -Wextra : warnings are errors in spirit (repo rule 7).
# -O2           : the node is I/O bound, but keep the hot paths (CRC, framing) fast.
# -g            : symbols so gdb/perf/strace map back to source.
# -std=gnu11    : C11 + the GNU/POSIX extensions; _GNU_SOURCE is set in db.h,
#                 which every .c includes FIRST so the feature macro precedes the
#                 first system header (required for accept4/timerfd visibility).
CFLAGS  ?= -Wall -Wextra -O2 -g -std=gnu11

# The System V AMD64 (Linux) target used for the committed teaching assembly.
ASM_TARGET ?= x86_64-pc-linux-gnu

BIN   := dbnode
SRCS  := src/crc32.c src/wal.c src/store.c src/raft.c src/server.c src/main.c
HDRS  := src/db.h

.PHONY: all
all: $(BIN)

# Single translation-unit-per-file build; small enough to skip object files.
$(BIN): $(SRCS) $(HDRS)
	$(CC) $(CFLAGS) $(SRCS) -o $@
	@echo "built ./$(BIN)  — try:  make run-single   or   make run-cluster"

# ---- run helpers ----------------------------------------------------------
# Single node: still exercises WAL + fsync + recovery + the full Raft code path
# (a one-node cluster elects itself instantly and commits with no network hop).
.PHONY: run-single
run-single: $(BIN)
	@mkdir -p data/n1
	DB_TRACE=1 ./$(BIN) 1 ./data/n1 7001 8001

# Three-node localhost cluster (spawns n1/n2/n3, waits for an election). Use a
# second terminal to talk to a node; see scripts/client.sh and the README.
.PHONY: run-cluster
run-cluster: $(BIN)
	./scripts/run-cluster.sh

.PHONY: stop-cluster
stop-cluster:
	./scripts/stop-cluster.sh

# Injected-fault demo: form a cluster, partition the leader, watch a new leader
# take over, then heal the partition and confirm the logs reconcile.
.PHONY: partition-demo
partition-demo: $(BIN)
	./scripts/partition-demo.sh

# ---- teaching assembly ----------------------------------------------------
# Regenerates asm/demo.{s,O0.s,O2.s} from asm/demo.c. The hand-written
# asm/demo.annotated.s is authored, never overwritten. Works on any host.
.PHONY: asm
asm:
	clang --target=$(ASM_TARGET) -S -O0 -fno-asynchronous-unwind-tables \
	      -fno-jump-tables -fno-omit-frame-pointer asm/demo.c -o asm/demo.O0.s
	clang --target=$(ASM_TARGET) -S -O1 -fno-asynchronous-unwind-tables \
	      -fno-jump-tables -fno-omit-frame-pointer asm/demo.c -o asm/demo.s
	clang --target=$(ASM_TARGET) -S -O2 -fno-asynchronous-unwind-tables \
	      -fno-jump-tables asm/demo.c -o asm/demo.O2.s
	@echo "regenerated asm/*.s — asm/demo.annotated.s is hand-written, left alone"

.PHONY: clean
clean:
	rm -f $(BIN)

# Wipe per-node persistent state (WALs, raft logs) so the next run starts fresh.
.PHONY: distclean
distclean: clean
	rm -rf data
