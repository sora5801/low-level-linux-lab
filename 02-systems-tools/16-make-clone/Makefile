# =============================================================================
# Makefile for mmake — a teaching make(1) clone.
#
#   make            -> build ./mmake            (Linux/WSL only)
#   make test       -> build, then run mmake on examples/ to show a parallel build
#   make asm        -> regenerate the committed teaching assembly (asm/demo*.s)
#   make clean      -> remove the binary and example build artifacts
#
# The BINARY builds only on Linux/WSL: it uses fork/execve, pipe2, stat, waitpid,
# sigaction and killpg. The ASSEMBLY target regenerates on ANY host, because
# clang cross-targets the System V AMD64 (Linux) ABI regardless of where it runs.
# =============================================================================

CC      ?= cc
# -std=c11        : the language we target.
# -Wall -Wextra   : the house rule — warnings are errors in spirit.
# -D_GNU_SOURCE   : expose pipe2(2), the SA_* sigaction flags, and struct stat's
#                   nanosecond st_mtim member from glibc's headers.
CFLAGS  ?= -std=c11 -Wall -Wextra -O2 -D_GNU_SOURCE

SRCS    = main.c parse.c graph.c jobserver.c job.c util.c
HDRS    = mk.h
BIN     = mmake

# clang cross-target used for the teaching assembly (System V AMD64 / Linux),
# regardless of the host OS. Matches every other project in the lab.
ASM_TARGET ?= x86_64-pc-linux-gnu
ASM_COMMON = --target=$(ASM_TARGET) -S -fno-asynchronous-unwind-tables -fno-jump-tables

$(BIN): $(SRCS) $(HDRS)
	$(CC) $(CFLAGS) $(SRCS) -o $@
	@echo "built ./$(BIN) — try:  make test   or   ./$(BIN) -C examples -j3 all"

# Demonstrate a real parallel, staleness-aware build against examples/.
# Run once and everything builds; run again and it reports 'up to date'.
.PHONY: test
test: $(BIN)
	@echo "=== first build (-j3, watch the 3 compiles overlap) ==="
	./$(BIN) -C examples -j3 all
	@echo "=== second build (nothing changed) ==="
	./$(BIN) -C examples all
	@echo "=== touch one source, rebuild only what depends on it ==="
	touch examples/b.src
	./$(BIN) -C examples -j3 all

# Regenerate the committed teaching assembly with the EXACT flags the lab uses.
# Only asm/demo.c is compiled here: it is self-contained (no system headers), so
# it cross-compiles on any host. The real translation units (parse.c, graph.c,
# job.c, ...) need Linux libc headers, which a cross-target clang may not find on
# a non-Linux host — hence the didactic extraction in asm/demo.c. The hand-
# written asm/demo.annotated.s is authored, not generated, and is NOT overwritten.
.PHONY: asm
asm:
	clang $(ASM_COMMON) -O0 -fno-omit-frame-pointer asm/demo.c -o asm/demo.O0.s
	clang $(ASM_COMMON) -O1 -fno-omit-frame-pointer asm/demo.c -o asm/demo.s
	clang $(ASM_COMMON) -O2 asm/demo.c -o asm/demo.O2.s
	@echo "regenerated asm/demo.{O0.s,s,O2.s}; demo.annotated.s is hand-authored"

.PHONY: clean
clean:
	rm -f $(BIN)
	rm -f examples/a.o examples/b.o examples/c.o examples/app
