# =============================================================================
# Makefile — build the ptrace debugger, its sample inferior, and teaching asm.
# =============================================================================
# Platform: Linux / WSL only. The debugger calls ptrace(2), reads
# /proc/<pid>/{mem,maps}, and decodes ELF/DWARF — all Linux specifics. `make asm`
# works on ANY host because clang cross-targets Linux to emit the assembly.
#
#   make            build ./ptrace-dbg and ./sample
#   make run        debug ./sample interactively (a real REPL session)
#   make test       run a scripted session (break/continue/bt/regs) non-interactively
#   make asm        regenerate asm/demo.{O0.s,s,O2.s} with the EXACT clang commands
#   make clean
# =============================================================================

CC      ?= clang
# -Wall -Wextra: the house rule. -g: debug info for debugging the debugger itself.
# -O2: optimise; the code is written to be warning-clean at -O2 as well as -O0.
CFLAGS  ?= -Wall -Wextra -O2 -g
# _GNU_SOURCE is set per-file, but harmless to also state the C standard.
CSTD    ?= -std=gnu11

# The debugger is four translation units (see debugger.h for the layer map).
DBG_SRC := debugger.c inferior.c breakpoint.c debuginfo.c
DBG_OBJ := $(DBG_SRC:.c=.o)

# Force the System V AMD64 (Linux) ABI for asm generation regardless of host OS.
ASM_TARGET ?= x86_64-pc-linux-gnu

.PHONY: all
all: ptrace-dbg sample

# ---- the debugger -----------------------------------------------------------
ptrace-dbg: $(DBG_OBJ)
	$(CC) $(CFLAGS) $^ -o $@
	@echo "built ./ptrace-dbg — try:  ./ptrace-dbg ./sample"

%.o: %.c debugger.h
	$(CC) $(CFLAGS) $(CSTD) -c $< -o $@

# ---- the inferior we debug --------------------------------------------------
# CRITICAL build flags for a debuggable target:
#   -g -gdwarf-4          emit a DWARF *version 4* line table (our parser reads v2-4;
#                         clang defaults to v5, whose header layout we don't decode).
#   -fno-omit-frame-pointer   keep the %rbp chain so `bt` can walk the stack.
#   -O0                   one basic block per source line => clean single-stepping.
# It stays a normal PIE, which is exactly why the debugger reads /proc/<pid>/maps.
sample: sample.c
	$(CC) -g -gdwarf-4 -fno-omit-frame-pointer -O0 sample.c -o sample
	@echo "built ./sample (DWARF v4, frame pointers kept)"

# ---- interactive session ----------------------------------------------------
.PHONY: run
run: ptrace-dbg sample
	./ptrace-dbg ./sample

# ---- scripted smoke test ----------------------------------------------------
# Feeds a canned command stream: break on the recursion, walk down three frames,
# print a backtrace and registers, then let it run to exit. Proves the whole
# pipeline (int3, SIGTRAP handling, RIP rewind, DWARF line lookup, fp backtrace).
.PHONY: test
test: ptrace-dbg sample
	printf 'break sample.c:28\ncontinue\ncontinue\ncontinue\nbt\nregs\ncontinue\ncontinue\nquit\n' \
	    | ./ptrace-dbg ./sample

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
	rm -f ptrace-dbg sample $(DBG_OBJ)
