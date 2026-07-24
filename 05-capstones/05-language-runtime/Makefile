# =============================================================================
# Makefile — build the Lumen language runtime, run demos, regenerate teaching asm.
# =============================================================================
#
# Targets:
#   make            build ./lumen
#   make run        build, then run examples/demo.lum
#   make test       alias for run (the demo IS the smoke test)
#   make repl       build, then start the interactive REPL
#   make jit        build, then run the x86-64 JIT demo (sum 1..100)  [Linux only]
#   make coro       build, then run the green-thread demo             [Linux only]
#   make debug      build with per-instruction execution tracing + code dumps
#   make gc-test    build with the GC forced to run on EVERY allocation, then run
#                   the demo — a missing GC root crashes here, deterministically
#   make asm        regenerate asm/demo.{O0.s,s,O2.s} from asm/demo.c
#                   (the hand-written asm/demo.annotated.s is NEVER overwritten)
#   make clean      remove build products
#
# PLATFORM: the RUNTIME is Linux/x86-64 (it uses mmap/mprotect for the GC heap and
# the JIT page, and <ucontext.h> for green threads). It builds and runs on Linux
# and WSL. `make asm` works on ANY host (Windows/macOS included) because clang
# cross-targets Linux System V AMD64 regardless of where it runs.
#
# The interpreter's computed-goto dispatch uses the GNU "labels as values"
# extension (clang/gcc); a non-GNU compiler automatically falls back to a portable
# switch (see vm.c).

CC      ?= clang
# -std=gnu11 : we deliberately use GNU extensions (computed goto; address-of-label
#              tables) and POSIX/Linux APIs (mmap, ucontext).
# -Wall -Wextra : warnings are errors in spirit (CONVENTIONS.md).
# -O2 : dispatch and GC throughput are the point; measure an optimized build.
CFLAGS  ?= -std=gnu11 -Wall -Wextra -O2
LDFLAGS ?=

SRCDIR  = src
SRC     = $(SRCDIR)/main.c   $(SRCDIR)/vm.c       $(SRCDIR)/compiler.c \
          $(SRCDIR)/scanner.c $(SRCDIR)/chunk.c   $(SRCDIR)/value.c    \
          $(SRCDIR)/object.c  $(SRCDIR)/table.c   $(SRCDIR)/heap.c     \
          $(SRCDIR)/gc.c      $(SRCDIR)/jit.c     $(SRCDIR)/sched.c
HDR     = $(wildcard $(SRCDIR)/*.h)
BIN     = lumen

# --- Assembly generation (REQUIRES clang for --target cross-compilation). ----
CLANG      ?= clang
ASM_TARGET ?= x86_64-pc-linux-gnu
# Shared flags: drop CFI noise and jump tables so the asm reads cleanly and the
# computed-goto vs. switch contrast stays visible.
ASM_COMMON  = --target=$(ASM_TARGET) -S -fno-asynchronous-unwind-tables -fno-jump-tables

$(BIN): $(SRC) $(HDR)
	$(CC) $(CFLAGS) $(SRC) -o $@ $(LDFLAGS)
	@echo "built ./$(BIN) -- try:  ./$(BIN) examples/demo.lum   or   ./$(BIN) --jit-demo"

.PHONY: run
run: $(BIN)
	./$(BIN) examples/demo.lum

.PHONY: test
test: run

.PHONY: repl
repl: $(BIN)
	./$(BIN)

.PHONY: jit
jit: $(BIN)
	./$(BIN) --jit-demo 100

.PHONY: coro
coro: $(BIN)
	./$(BIN) --coro-demo

# Trace every instruction as it executes and disassemble each compiled function.
.PHONY: debug
debug: CFLAGS += -DDEBUG_TRACE_EXECUTION -DDEBUG_PRINT_CODE
debug: $(BIN)
	./$(BIN) examples/demo.lum

# Force a full collection on every allocation: the strongest test that the GC's
# root set is complete. If any live object is missing, the demo crashes here.
.PHONY: gc-test
gc-test: CFLAGS += -DDEBUG_STRESS_GC
gc-test: $(BIN)
	./$(BIN) examples/demo.lum
	@echo "gc-test passed: survived stress collection on every allocation"

# Regenerate the committed teaching assembly for the extracted dispatch core.
# The real .c files pull in <stdio.h>/<sys/mman.h>/<ucontext.h>, whose Linux
# headers aren't present when cross-targeting from a non-Linux host, so asm/demo.c
# is the SELF-CONTAINED stand-in (the bytecode dispatch core) — see the README.
.PHONY: asm
asm:
	@echo ">> asm for asm/demo.c (the computed-goto bytecode dispatch core)"
	$(CLANG) $(ASM_COMMON) -O0 -fno-omit-frame-pointer asm/demo.c -o asm/demo.O0.s
	$(CLANG) $(ASM_COMMON) -O1 -fno-omit-frame-pointer asm/demo.c -o asm/demo.s
	$(CLANG) $(ASM_COMMON) -O2                          asm/demo.c -o asm/demo.O2.s
	@echo "   (asm/demo.annotated.s is hand-authored and left untouched)"

.PHONY: clean
clean:
	rm -f $(BIN) $(BIN).exe
