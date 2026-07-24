# =============================================================================
# Makefile — build the teaching shell, smoke-test it, and (re)generate the asm.
# =============================================================================
#
# Targets:
#   make            build ./mysh
#   make run        build, then start an interactive session (needs a terminal)
#   make test       build, then run a scripted, non-interactive smoke test
#   make asm        regenerate asm/demo.{O0.s,s,O2.s} and asm/match.{O0.s,s,O2.s}
#                   (the hand-written asm/demo.annotated.s is NEVER overwritten)
#   make clean      remove build products
#
# Platform: the shell is Linux-only (it uses process groups, tcsetpgrp, and the
# job-control signals). Build and run it on Linux or WSL. `make asm` works on ANY
# host because clang cross-targets Linux (System V AMD64) regardless of where it
# runs — that is the whole point of committing the teaching assembly.

CC      ?= cc
# Warnings are errors in spirit (CONVENTIONS.md). C11 for standard behavior.
CFLAGS  ?= -Wall -Wextra -O2 -std=c11
BIN      = mysh
SRC      = main.c lexer.c parser.c expand.c jobs.c builtins.c exec.c match.c util.c
HDR      = shell.h match.h

# Assembly generation REQUIRES clang (for --target cross-compilation), even if
# the build above uses gcc/cc. The teaching asm is always the Linux SysV ABI.
CLANG      ?= clang
ASM_TARGET ?= x86_64-pc-linux-gnu
# Flags shared by all three optimization levels (match CONVENTIONS.md / the lab):
#   -fno-asynchronous-unwind-tables  drop .cfi_* noise so the asm reads cleanly
#   -fno-jump-tables                 make switch() branch, not table-index
ASM_COMMON = --target=$(ASM_TARGET) -S -fno-asynchronous-unwind-tables -fno-jump-tables

$(BIN): $(SRC) $(HDR)
	$(CC) $(CFLAGS) $(SRC) -o $@
	@echo "built ./$(BIN) — start it with:  ./$(BIN)     (try: ls | wc -l, sleep 5 &, Ctrl-Z, fg)"

.PHONY: run
run: $(BIN)
	./$(BIN)

# A non-interactive smoke test: pipe a script in on stdin. Job control is off in
# this mode (no terminal), but pipes, redirection, globbing, $-expansion, and
# builtins are all exercised. Interactive job control is demonstrated in the
# README with a pty; see also the asm/ notes.
.PHONY: test
test: $(BIN)
	@printf '%s\n' \
	  'echo hello world' \
	  'echo pipe | tr a-z A-Z' \
	  'echo one two three | wc -w' \
	  'echo redirected > /tmp/mysh_test.out' \
	  'cat < /tmp/mysh_test.out' \
	  'export MYSH_TEST=ok' \
	  'echo var is $$MYSH_TEST' \
	  'false' \
	  'echo status $$?' \
	  'echo sources: *.c' \
	  'exit 0' \
	| ./$(BIN)
	@echo "smoke test finished (exit $$?)"

# Regenerate the committed teaching assembly. Two translation units get the
# treatment: the extracted asm/demo.c and the REAL matcher match.c (which uses
# NO system headers, so it cross-compiles cleanly). The other source files
# include POSIX headers and so are not self-contained; their logic is exercised
# by demo.c instead.
.PHONY: asm
asm:
	@echo ">> asm for asm/demo.c (extracted glob '*' backtracking core)"
	$(CLANG) $(ASM_COMMON) -O0 -fno-omit-frame-pointer asm/demo.c -o asm/demo.O0.s
	$(CLANG) $(ASM_COMMON) -O1 -fno-omit-frame-pointer asm/demo.c -o asm/demo.s
	$(CLANG) $(ASM_COMMON) -O2                          asm/demo.c -o asm/demo.O2.s
	@echo ">> asm for the real matcher match.c (with [a-z] bracket classes)"
	$(CLANG) $(ASM_COMMON) -O0 -fno-omit-frame-pointer match.c -o asm/match.O0.s
	$(CLANG) $(ASM_COMMON) -O1 -fno-omit-frame-pointer match.c -o asm/match.s
	$(CLANG) $(ASM_COMMON) -O2                          match.c -o asm/match.O2.s
	@echo "   (asm/demo.annotated.s is hand-authored and left untouched)"

.PHONY: clean
clean:
	rm -f $(BIN) $(BIN).exe /tmp/mysh_test.out
