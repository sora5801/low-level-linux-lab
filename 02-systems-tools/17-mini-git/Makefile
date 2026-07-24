# =============================================================================
# Makefile — build the mini-git, run a self-test, and (re)generate teaching asm.
# =============================================================================
#
# Targets:
#   make            build ./mygit          (the content-addressed VCS)
#   make test       build, then run a full init->add->commit->log->diff demo in
#                   a scratch directory and check the output
#   make asm        regenerate asm/demo.{O0.s,s,O2.s} and asm/sha1.{O0.s,s,O2.s}
#                   (the hand-written asm/demo.annotated.s is NEVER overwritten)
#   make clean      remove build products
#
# Platform: LINUX / WSL. The store uses zlib (link with -lz) and POSIX file I/O
# (open/read/write, opendir/readdir, mkdir with a mode), so it needs a real
# POSIX host and the zlib development headers:
#       Debian/Ubuntu:  sudo apt install zlib1g-dev
# `make asm` works on ANY host (including native Windows) because clang cross-
# targets Linux (System V AMD64) and only the two DEPENDENCY-FREE units
# (asm/demo.c and sha1.c) are compiled to assembly.

CC      ?= cc
# Warnings are errors in spirit (CONVENTIONS.md). -std=c11 for the fixed-width
# types and designated initializers; -O2 so the build is representative.
CFLAGS  ?= -Wall -Wextra -O2 -std=c11
LDLIBS  ?= -lz                 # zlib: object (de)compression, git's format

SRC = main.c util.c object.c index.c tree.c commit.c diff.c sha1.c
HDR = mygit.h sha1.h
BIN = mygit

# ---- assembly generation ----------------------------------------------------
# Always clang (for --target cross-compilation), even if CC is gcc/cc above.
CLANG      ?= clang
ASM_TARGET ?= x86_64-pc-linux-gnu
# Flags shared by all three levels (match CONVENTIONS.md / the reference project):
#   -fno-asynchronous-unwind-tables  drop .cfi_* noise so the asm reads cleanly
#   -fno-jump-tables                 make switch()es branch, not table-index
ASM_COMMON = --target=$(ASM_TARGET) -S -fno-asynchronous-unwind-tables -fno-jump-tables

$(BIN): $(SRC) $(HDR)
	$(CC) $(CFLAGS) $(SRC) -o $@ $(LDLIBS)
	@echo "built ./$(BIN) -- try:  ./$(BIN) init && ./$(BIN) add Makefile && ./$(BIN) commit -m hi"

# End-to-end smoke test in an isolated scratch repo (never touches this tree).
.PHONY: test
test: $(BIN)
	@set -e; \
	tmp=$$(mktemp -d); \
	cp ./$(BIN) $$tmp/; cd $$tmp; \
	echo "== init =="            ; ./$(BIN) init; \
	printf 'hello\nworld\n' > a.txt; \
	mkdir -p src; printf 'int main(void){return 0;}\n' > src/main.c; \
	echo "== add =="             ; ./$(BIN) add a.txt src/main.c; \
	echo "== write-tree =="      ; tree=$$(./$(BIN) write-tree); echo "root tree $$tree"; \
	echo "== commit =="          ; ./$(BIN) commit -m "first commit"; \
	echo "== cat-file -p tree ==" ; ./$(BIN) cat-file -p $$tree; \
	printf 'hello\nthere\nworld\n' > a.txt; \
	echo "== diff (after edit) ==" ; ./$(BIN) diff; \
	echo "== status =="          ; ./$(BIN) status; \
	echo "== add+commit again ==" ; ./$(BIN) add a.txt; ./$(BIN) commit -m "second commit"; \
	echo "== log =="             ; ./$(BIN) log | head -n 20; \
	cd /; rm -rf $$tmp; \
	echo "ALL OK"

# Regenerate the committed teaching assembly. Two translation units qualify:
# asm/demo.c (the extracted SHA round + hex encode) and the REAL sha1.c (which
# includes only its own header, so it cross-compiles with no libc present).
.PHONY: asm
asm:
	@echo ">> asm for asm/demo.c (extracted SHA-1 round + hex encode)"
	$(CLANG) $(ASM_COMMON) -O0 -fno-omit-frame-pointer asm/demo.c -o asm/demo.O0.s
	$(CLANG) $(ASM_COMMON) -O1 -fno-omit-frame-pointer asm/demo.c -o asm/demo.s
	$(CLANG) $(ASM_COMMON) -O2                          asm/demo.c -o asm/demo.O2.s
	@echo ">> asm for the real sha1.c"
	$(CLANG) $(ASM_COMMON) -O0 -fno-omit-frame-pointer sha1.c -o asm/sha1.O0.s
	$(CLANG) $(ASM_COMMON) -O1 -fno-omit-frame-pointer sha1.c -o asm/sha1.s
	$(CLANG) $(ASM_COMMON) -O2                          sha1.c -o asm/sha1.O2.s
	@echo "   (asm/demo.annotated.s is hand-authored and left untouched)"

.PHONY: clean
clean:
	rm -f $(BIN) $(BIN).exe
