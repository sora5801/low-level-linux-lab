# =============================================================================
# Makefile — build the regex engine, run its tests, and (re)generate teaching asm.
# =============================================================================
#
# Targets:
#   make            build ./regex        (the grep-like CLI)
#   make test       build, then run the built-in correctness self-test
#   make bench      build, then show linear-time / no-catastrophic-backtracking
#   make run        a quick demo grep over this Makefile
#   make asm        regenerate asm/demo.{O0.s,s,O2.s} and asm/sim.{O0.s,s,O2.s}
#                   (the hand-written asm/demo.annotated.s is NEVER overwritten)
#   make clean      remove build products
#
# Platform: the engine is portable ISO C11 and builds on Linux, WSL, and the
# msys2 clang on Windows. `make asm` works on ANY host because clang cross-targets
# Linux (System V AMD64) regardless of where it runs.

CC      ?= clang
# Warnings are errors in spirit (CONVENTIONS.md). -O2 so the bench is meaningful.
CFLAGS  ?= -Wall -Wextra -O2 -std=c11
SRC      = parser.c nfa.c sim.c main.c
BIN      = regex

# Assembly generation REQUIRES clang (for --target cross-compilation); the build
# above may use any C compiler, but `make asm` always uses clang explicitly so it
# is not shadowed by a make built-in default of CC=cc.
CLANG   ?= clang
# The teaching asm is always the Linux System V ABI, even on other hosts.
ASM_TARGET ?= x86_64-pc-linux-gnu
# Flags shared by all three optimization levels (match CONVENTIONS.md / the lab):
#   -fno-asynchronous-unwind-tables  drop .cfi_* noise so the asm reads cleanly
#   -fno-jump-tables                 make switch() branch, not table-index
ASM_COMMON = --target=$(ASM_TARGET) -S -fno-asynchronous-unwind-tables -fno-jump-tables

$(BIN): $(SRC) regex.h ast.h
	$(CC) $(CFLAGS) $(SRC) -o $@
	@echo "built ./$(BIN) -- try:  ./$(BIN) '[0-9]+' Makefile"

.PHONY: test
test: $(BIN)
	./$(BIN) --selftest

.PHONY: bench
bench: $(BIN)
	./$(BIN) --bench

.PHONY: run
run: $(BIN)
	@echo "# lines of this Makefile containing an identifier followed by '=':"
	./$(BIN) -n '[A-Za-z_]+ *=' Makefile

# Regenerate the committed teaching assembly. Two translation units get the
# treatment: asm/demo.c (the self-contained extraction) and the REAL matcher
# sim.c (which uses only freestanding headers, so it cross-compiles cleanly).
.PHONY: asm
asm:
	@echo ">> asm for asm/demo.c (the extracted state-set step)"
	$(CLANG) $(ASM_COMMON) -O0 -fno-omit-frame-pointer asm/demo.c -o asm/demo.O0.s
	$(CLANG) $(ASM_COMMON) -O1 -fno-omit-frame-pointer asm/demo.c -o asm/demo.s
	$(CLANG) $(ASM_COMMON) -O2                          asm/demo.c -o asm/demo.O2.s
	@echo ">> asm for the real matcher sim.c"
	$(CLANG) $(ASM_COMMON) -O0 -fno-omit-frame-pointer sim.c -o asm/sim.O0.s
	$(CLANG) $(ASM_COMMON) -O1 -fno-omit-frame-pointer sim.c -o asm/sim.s
	$(CLANG) $(ASM_COMMON) -O2                          sim.c -o asm/sim.O2.s
	@echo "   (asm/demo.annotated.s is hand-authored and left untouched)"

.PHONY: clean
clean:
	rm -f $(BIN) $(BIN).exe
