# =============================================================================
# Makefile — build the green-thread demos and (re)generate the teaching asm.
# =============================================================================
# Platform: Linux / WSL. gt.c uses mmap/mprotect/munmap and links the hand-
# written switch.S; ucontext_demo.c uses glibc's <ucontext.h>. `make asm` works
# on ANY host because clang cross-targets Linux to emit the teaching assembly.
#
#   make            build ./gt (the round-robin + join demo)
#   make run        build and run ./gt
#   make test       same as run — the interleaved output IS the test
#   make ucontext   build ./ucontext_demo (the libc makecontext version)
#   make guard      build ./guard_demo (proves the guard page catches overflow)
#   make asm        regenerate asm/demo.{O0.s,s,O2.s} with the EXACT clang cmds
#   make clean
# =============================================================================

CC      ?= clang
# -Wall -Wextra: warnings on (the point of the lab). -g: keep symbols for gdb.
# -fno-omit-frame-pointer: readable backtraces through the coroutine stacks.
CFLAGS  ?= -Wall -Wextra -O2 -g -fno-omit-frame-pointer

# The library is the scheduler (C) plus the hand-written context switch (asm).
# switch.S goes through the C preprocessor (capital .S) and clang assembles it.
LIB_SRC := gt.c switch.S

# Force the System V AMD64 (Linux) ABI for asm generation regardless of host OS.
ASM_TARGET ?= x86_64-pc-linux-gnu

# ---- default: the main demo -------------------------------------------------
gt: main.c $(LIB_SRC) gt.h
	$(CC) $(CFLAGS) main.c $(LIB_SRC) -o $@
	@echo "built ./gt — run it with:  make run"

.PHONY: run test
run: gt
	./gt
# The visible interleaving (workers round-robin; supervisor joins children) is
# the correctness check; there is no hidden oracle to compare against.
test: run

# ---- the ucontext illustration ---------------------------------------------
ucontext_demo: ucontext_demo.c
	$(CC) $(CFLAGS) ucontext_demo.c -o $@
	@echo "built ./ucontext_demo"

.PHONY: ucontext
ucontext: ucontext_demo
	./ucontext_demo

# ---- the guard-page overflow demo ------------------------------------------
# Expected to die with SIGSEGV when the runaway recursion hits the guard page.
guard_demo: guard_demo.c $(LIB_SRC) gt.h
	$(CC) $(CFLAGS) guard_demo.c $(LIB_SRC) -o $@
	@echo "built ./guard_demo — it SHOULD segfault at the guard page"

.PHONY: guard
guard: guard_demo
	-./guard_demo ; echo "exit=$$? (139 == 128+SIGSEGV: the guard page fired)"

# ---- regenerate the committed teaching assembly -----------------------------
# These are the EXACT commands the project spec mandates. The star asm file is
# the hand-written switch.S itself; asm/demo.annotated.s is authored, NOT
# overwritten here. We also assemble switch.S to prove it is valid.
.PHONY: asm
asm:
	clang --target=$(ASM_TARGET) -S -O0 -fno-asynchronous-unwind-tables -fno-jump-tables -fno-omit-frame-pointer asm/demo.c -o asm/demo.O0.s
	clang --target=$(ASM_TARGET) -S -O1 -fno-asynchronous-unwind-tables -fno-jump-tables -fno-omit-frame-pointer asm/demo.c -o asm/demo.s
	clang --target=$(ASM_TARGET) -S -O2 -fno-asynchronous-unwind-tables -fno-jump-tables asm/demo.c -o asm/demo.O2.s
	clang --target=$(ASM_TARGET) -c switch.S -o /dev/null && echo "switch.S assembles OK"
	@echo "regenerated asm/demo.{O0.s,s,O2.s}; asm/demo.annotated.s is hand-written"

.PHONY: clean
clean:
	rm -f gt ucontext_demo guard_demo
