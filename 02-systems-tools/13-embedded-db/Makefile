# =============================================================================
# Makefile — build the embedded KV store CLI and (re)generate teaching assembly.
# =============================================================================
# Platform: Linux / WSL. The store uses pread/pwrite/fdatasync/ftruncate, all
# POSIX; it builds on Linux, WSL2, and macOS (where fdatasync exists). `make asm`
# works on ANY host because clang cross-targets Linux to emit the teaching asm.
#
#   make            build ./db
#   make run        build, then run the self-checking demo (2000 inserts + scan)
#   make test       a quick put/get/del/scan + crash-recovery smoke test
#   make asm        regenerate asm/demo.{O0.s,s,O2.s} with the exact clang cmds
#   make clean      remove build artifacts and scratch databases
# =============================================================================

CC      ?= clang
# -Wall -Wextra: warnings are errors in spirit (CONVENTIONS §7).
# -g keeps symbols for gdb/perf; -O2 because a KV store should be quick.
# -fno-omit-frame-pointer keeps stack traces legible.
CFLAGS  ?= -Wall -Wextra -O2 -g -fno-omit-frame-pointer
# _GNU_SOURCE exposes fdatasync(3)'s prototype from <unistd.h> on glibc.
CPPFLAGS ?= -D_GNU_SOURCE

# Force the System V AMD64 (Linux) ABI for asm generation regardless of host OS.
ASM_TARGET ?= x86_64-pc-linux-gnu

OBJS = crc32.o pager.o wal.o btree.o db.o main.o
HDRS = db.h

.PHONY: all
all: db

# ---- the CLI binary ---------------------------------------------------------
db: $(OBJS)
	$(CC) $(CFLAGS) $(OBJS) -o $@

%.o: %.c $(HDRS)
	$(CC) $(CFLAGS) $(CPPFLAGS) -c $< -o $@

# ---- run the self-checking demo --------------------------------------------
# Inserts 2000 ordered keys (forcing many B-tree splits), does point lookups,
# then a full scan whose count MUST equal what we inserted. Prints page count.
.PHONY: run
run: db
	rm -f demo.db demo.db-wal
	./db demo.db demo 2000

# ---- functional + durability smoke test ------------------------------------
# Proves basic ops, then injects a crash right after the WAL barrier (the write
# is durable in the log but no page has been checkpointed) and confirms the
# store reopens intact — recovery replays the committed WAL. All-or-nothing.
.PHONY: test
test: db
	rm -f t.db t.db-wal
	./db t.db put alpha  one
	./db t.db put bravo  two
	./db t.db put alpha  ONE          # overwrite in place
	@echo -n "get alpha -> "; ./db t.db get alpha    # expect: ONE
	@echo -n "get bravo -> "; ./db t.db get bravo    # expect: two
	./db t.db del bravo
	@echo -n "get bravo -> "; ./db t.db get bravo || echo "(deleted, as expected)"
	@echo "--- scan ---"; ./db t.db scan
	@echo "--- crash AFTER the WAL barrier, BEFORE checkpoint (exits 99) ---"
	-KVDB_CRASH_AFTER_WAL=1 ./db t.db put charlie three
	@echo "--- reopen: recovery replays the committed WAL ---"
	@echo -n "get charlie -> "; ./db t.db get charlie   # expect: three (recovered)
	rm -f t.db t.db-wal

# ---- regenerate the committed teaching assembly -----------------------------
# These are the EXACT commands the project spec mandates; the hand-written
# asm/demo.annotated.s is authored, NOT overwritten here.
.PHONY: asm
asm:
	clang --target=$(ASM_TARGET) -S -O0 -fno-asynchronous-unwind-tables -fno-jump-tables -fno-omit-frame-pointer asm/demo.c -o asm/demo.O0.s
	clang --target=$(ASM_TARGET) -S -O1 -fno-asynchronous-unwind-tables -fno-jump-tables -fno-omit-frame-pointer asm/demo.c -o asm/demo.s
	clang --target=$(ASM_TARGET) -S -O2 -fno-asynchronous-unwind-tables -fno-jump-tables asm/demo.c -o asm/demo.O2.s
	@echo "regenerated asm/demo.{O0.s,s,O2.s}; asm/demo.annotated.s is hand-written"

.PHONY: clean
clean:
	rm -f $(OBJS) db demo.db demo.db-wal t.db t.db-wal
