# =============================================================================
# Makefile — Raft consensus (teaching core)
# =============================================================================
# Targets:
#   make            build ./raft                      (Linux / WSL2)
#   make run        build, then run the test harness (elections, partitions,
#                   crash recovery, snapshotting) against a 5-node cluster
#   make asm        regenerate the teaching assembly in asm/ (any host)
#   make clean      remove build artifacts and the ./raftdata state directory
#
# The node threads use pthreads and the persistence layer uses fsync + a
# directory fsync (open(dir)/fsync), so `make`/`make run` require Linux (or
# WSL2). The `asm` target cross-targets Linux via clang and works on any host.
# =============================================================================

CC      ?= cc
CFLAGS  ?= -Wall -Wextra -O2 -g -std=c11 -D_POSIX_C_SOURCE=200809L
LDFLAGS ?= -pthread

BIN     := raft
SRC     := raft.c persist.c net.c kv.c main.c
HDR     := raft.h
OBJ     := $(SRC:.c=.o)

# ---- toolchain for the assembly deliverable (matches tools/gen_asm.sh) ------
ASM_CC     ?= clang
ASM_TARGET ?= x86_64-pc-linux-gnu
ASM_COMMON := --target=$(ASM_TARGET) -S -fno-asynchronous-unwind-tables -fno-jump-tables

.PHONY: all run asm clean

all: $(BIN)

$(BIN): $(OBJ)
	$(CC) $(CFLAGS) -o $@ $(OBJ) $(LDFLAGS)

%.o: %.c $(HDR)
	$(CC) $(CFLAGS) -c -o $@ $<

# Run the harness. It wipes ./raftdata at startup so each run is reproducible,
# then drives five failure scenarios and prints PASS/FAIL for each check.
run: $(BIN)
	./$(BIN) ./raftdata

# ---- teaching assembly: EXACT commands from the project brief ---------------
# Regenerates asm/demo.{O0.s,s,O2.s} from the self-contained asm/demo.c.
# asm/demo.annotated.s is hand-written from demo.s (-O1) and not regenerated.
asm:
	$(ASM_CC) $(ASM_COMMON) -O0 -fno-omit-frame-pointer asm/demo.c -o asm/demo.O0.s
	$(ASM_CC) $(ASM_COMMON) -O1 -fno-omit-frame-pointer asm/demo.c -o asm/demo.s
	$(ASM_CC) $(ASM_COMMON) -O2 asm/demo.c -o asm/demo.O2.s
	@echo "regenerated asm/demo.O0.s, asm/demo.s, asm/demo.O2.s"

clean:
	rm -f $(BIN) $(OBJ)
	rm -rf ./raftdata
