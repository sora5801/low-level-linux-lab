# =============================================================================
# Makefile — build the conservative GC as a library + demo, and as an
# LD_PRELOAD shared object; regenerate the teaching assembly.
# =============================================================================
# Platform: Linux / WSL (the collector calls mmap/mprotect, setjmp, and reads
# /proc/self/stat). `make asm` works on ANY host because clang cross-targets
# Linux to emit the didactic assembly.
#
#   make            build ./gctest (the demo driven by the gc_* API)
#   make run        build and run the demo
#   make test       run the demo and sanity-check its output
#   make preload    build libgc.so (the LD_PRELOAD interposer)
#   make asm        regenerate asm/demo.{O0.s,s,O2.s} with the exact clang cmds
#   make clean
# =============================================================================

CC      ?= clang
# -O2 for realistic codegen; -g keeps symbols for gdb; warnings are errors in
# spirit. -fno-omit-frame-pointer keeps stack walking legible.
CFLAGS  ?= -Wall -Wextra -O2 -g -fno-omit-frame-pointer

# The scanner relies on the stack actually holding the roots it expects. At -O2
# the compiler is free to keep values only in registers or to reorder frames;
# the setjmp spill + reading rsp handle registers, but we also forbid tail-call
# optimisation so frames the scanner expects are really on the stack.
GCFLAGS := -fno-optimize-sibling-calls

# Force the System V AMD64 (Linux) ABI for asm generation regardless of host OS.
ASM_TARGET ?= x86_64-pc-linux-gnu

.PHONY: all
all: gctest

# ---- the demo, linked directly against the collector ------------------------
gctest: gctest.c gc.c gc.h
	$(CC) $(CFLAGS) $(GCFLAGS) gctest.c gc.c -o $@
	@echo "built ./gctest — run it with:  make run"

# ---- the LD_PRELOAD shared object -------------------------------------------
# -fPIC/-shared make a .so; -DGC_INTERPOSE exports strong malloc/free/... so the
# dynamic linker routes a program's allocations through the collector (free is a
# no-op; automatic collection bounds the heap). See the honesty note in gc.c.
libgc.so: gc.c gc.h
	$(CC) $(CFLAGS) $(GCFLAGS) -DGC_INTERPOSE -fPIC -shared gc.c -o $@
	@echo "built ./libgc.so — try:  LD_PRELOAD=./libgc.so <program>"

.PHONY: preload
preload: libgc.so

.PHONY: run
run: gctest
	./gctest

# ---- smoke test: the demo must reach the end and report the churn bound ------
.PHONY: test
test: gctest
	./gctest | tee /tmp/gc_out.txt
	@grep -q "done." /tmp/gc_out.txt && echo "PASS: demo completed" \
		|| (echo "FAIL: demo did not finish" && exit 1)

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
	rm -f gctest libgc.so
