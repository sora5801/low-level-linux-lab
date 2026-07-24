# =============================================================================
# Makefile — build the language VM, run its demo, and (re)generate teaching asm.
# =============================================================================
#
# Targets:
#   make            build ./lvm            (the compiler + VM binary)
#   make run        build, then run examples/demo.lox
#   make test       alias for run (the demo IS the smoke test)
#   make debug      build with execution tracing + code disassembly on
#   make gc-test    build with the GC forced to run on EVERY allocation, then
#                   run the demo — a missing GC root crashes here, deterministically
#   make asm        regenerate asm/demo.{O0.s,s,O2.s} from asm/demo.c
#                   (the hand-written asm/demo.annotated.s is NEVER overwritten)
#   make clean      remove build products
#
# Platform: portable ISO/GNU C11. Builds on Linux, WSL, and clang on Windows.
# The interpreter's computed-goto dispatch uses the GNU "labels as values"
# extension, so it needs clang or gcc (any MSVC build falls back to a portable
# switch automatically — see common.h/VM_COMPUTED_GOTO). `make asm` works on ANY
# host because clang cross-targets Linux (System V AMD64) regardless of where it runs.

CC      ?= clang
# -std=gnu11: we deliberately use two GNU extensions — computed goto (labels as
#   values) for dispatch and taking the address of labels into a static table.
# -Wall -Wextra: warnings are errors in spirit (CONVENTIONS.md).
# -O2: dispatch performance is the whole point; measure with an optimized build.
CFLAGS  ?= -std=gnu11 -Wall -Wextra -O2
SRC      = chunk.c compiler.c debug.c main.c memory.c object.c \
           scanner.c table.c value.c vm.c
HDR      = common.h chunk.h compiler.h debug.h memory.h object.h \
           scanner.h table.h value.h vm.h
BIN      = lvm

# Assembly generation REQUIRES clang (for --target cross-compilation).
CLANG   ?= clang
# The teaching asm is always the Linux System V ABI, even on other hosts.
ASM_TARGET ?= x86_64-pc-linux-gnu
# Flags shared by all three optimization levels (match CONVENTIONS.md / the lab):
#   -fno-asynchronous-unwind-tables  drop .cfi_* noise so the asm reads cleanly
#   -fno-jump-tables                 make the SWITCH fallback branch, not table-
#                                    index, so the two dispatch styles contrast
ASM_COMMON = --target=$(ASM_TARGET) -S -fno-asynchronous-unwind-tables -fno-jump-tables

$(BIN): $(SRC) $(HDR)
	$(CC) $(CFLAGS) $(SRC) -o $@
	@echo "built ./$(BIN) -- try:  ./$(BIN) examples/demo.lox   or   ./$(BIN)  (REPL)"

.PHONY: run
run: $(BIN)
	./$(BIN) examples/demo.lox

.PHONY: test
test: run

# Trace every instruction as it executes and disassemble each compiled function.
.PHONY: debug
debug: CFLAGS += -DDEBUG_TRACE_EXECUTION -DDEBUG_PRINT_CODE
debug: $(BIN)
	./$(BIN) examples/demo.lox

# Force a full collection on every allocation: the strongest test that the GC's
# root set is complete. If any root is missing, the demo crashes here.
.PHONY: gc-test
gc-test: CFLAGS += -DDEBUG_STRESS_GC
gc-test: $(BIN)
	./$(BIN) examples/demo.lox
	@echo "gc-test passed: survived stress collection on every allocation"

# Regenerate the committed teaching assembly for the extracted dispatch core.
# The real .c files are NOT cross-compilable here (they include <stdio.h> etc.,
# libc headers absent for the Linux target on a non-Linux host), so asm/demo.c
# is the self-contained stand-in — see the README "Assembly notes".
.PHONY: asm
asm:
	@echo ">> asm for asm/demo.c (the computed-goto dispatch core)"
	$(CLANG) $(ASM_COMMON) -O0 -fno-omit-frame-pointer asm/demo.c -o asm/demo.O0.s
	$(CLANG) $(ASM_COMMON) -O1 -fno-omit-frame-pointer asm/demo.c -o asm/demo.s
	$(CLANG) $(ASM_COMMON) -O2                          asm/demo.c -o asm/demo.O2.s
	@echo "   (asm/demo.annotated.s is hand-authored and left untouched)"

.PHONY: clean
clean:
	rm -f $(BIN) $(BIN).exe
