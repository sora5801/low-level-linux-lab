# =============================================================================
# Makefile — build mini-libc (libminilibc.a) and link the demo programs.
# =============================================================================
#
# Run `make` on Linux or WSL: the demos are real Linux ELF executables that use
# OUR _start and OUR syscalls, so they only *run* on Linux. On any host you can
# still `make asm` to regenerate the teaching assembly (clang cross-targets the
# Linux SysV ABI without a Linux linker).
#
# The whole library is built with -nostdlib: no crt1.o, no libc. We supply
# start.o (the _start entry) and libminilibc.a (everything else) ourselves.
# =============================================================================

CC      ?= clang
AR      ?= ar

# COMPILE flags:
#   -ffreestanding        : no hosted-environment assumptions (no implicit libc)
#   -fno-builtin          : do NOT rewrite our memcpy/memset/printf into calls to
#                           themselves — that would infinitely recurse.
#   -fno-stack-protector  : the canary calls __stack_chk_fail in libc, which we
#                           don't link; turn it off (we explain canaries instead).
#   -Iinclude             : find <minilibc.h>
CFLAGS  ?= -Wall -Wextra -O2 -ffreestanding -fno-builtin -fno-stack-protector -Iinclude

# LINK flags:
#   -nostdlib  : link no CRT objects and no libc — we provide _start.
#   -static    : no dynamic loader (ld.so); the binary is self-contained.
#   -no-pie    : fixed load address, so no startup relocation is needed.
LDFLAGS ?= -nostdlib -static -no-pie

# For asm generation we force the Linux target so output is the ABI this repo
# assumes, regardless of host OS.
ASM_TARGET ?= x86_64-pc-linux-gnu

# Library translation units (everything except the _start entry object).
LIB_SRCS := src/syscall.c src/crt.c src/string.c src/malloc.c src/printf.c
LIB_OBJS := $(LIB_SRCS:.c=.o)
CRT      := src/start.o          # the _start entry point (assembled from .S)
LIB      := libminilibc.a
DEMOS    := hello args

.PHONY: all
all: $(DEMOS)

# --- compile rules -----------------------------------------------------------
%.o: %.c include/minilibc.h
	$(CC) $(CFLAGS) -c $< -o $@

src/start.o: src/start.S
	$(CC) $(CFLAGS) -c $< -o $@

# --- the archive -------------------------------------------------------------
# `ar rcs`: r=insert, c=create quietly, s=write an index (so the linker can
# pull only the objects a demo actually references).
$(LIB): $(LIB_OBJS)
	$(AR) rcs $@ $^

# --- link the demos ----------------------------------------------------------
# Order matters: the CRT (start.o) provides the entry symbol, then main() from
# the demo, then the archive resolves the rest (printf, malloc, syscalls...).
hello: $(CRT) $(LIB) demo/hello.o
	$(CC) $(CFLAGS) $(LDFLAGS) $(CRT) demo/hello.o $(LIB) -o $@
	@echo "built ./hello — run:  ./hello one two three ; echo exit=\$$?"

args: $(CRT) $(LIB) demo/args.o
	$(CC) $(CFLAGS) $(LDFLAGS) $(CRT) demo/args.o $(LIB) -o $@
	@echo "built ./args  — run:  ./args foo bar"

# --- run / test --------------------------------------------------------------
# Smoke test: run both demos. On success hello exits 0 and args prints the auxv.
.PHONY: run test
run: $(DEMOS)
	@echo "==== ./hello one two three ===="; ./hello one two three; echo "exit=$$?"
	@echo "==== ./args  foo bar        ===="; ./args foo bar
test: run

# --- teaching assembly (works on any host) -----------------------------------
# Regenerate the three committed compiler outputs for asm/demo.c. The
# hand-written asm/demo.annotated.s and src/start.S are authored, NOT overwritten.
.PHONY: asm
asm:
	$(CC) --target=$(ASM_TARGET) -S -O0 -fno-asynchronous-unwind-tables -fno-jump-tables -fno-omit-frame-pointer asm/demo.c -o asm/demo.O0.s
	$(CC) --target=$(ASM_TARGET) -S -O1 -fno-asynchronous-unwind-tables -fno-jump-tables -fno-omit-frame-pointer asm/demo.c -o asm/demo.s
	$(CC) --target=$(ASM_TARGET) -S -O2 -fno-asynchronous-unwind-tables -fno-jump-tables asm/demo.c -o asm/demo.O2.s
	@echo "regenerated asm/demo.{O0.s,s,O2.s}; demo.annotated.s is hand-authored"

# Optional: show the genuine encoded bytes of the _start star artifact.
.PHONY: disasm
disasm: src/start.o
	objdump -dr -M att src/start.o

.PHONY: clean
clean:
	rm -f $(LIB_OBJS) $(CRT) demo/*.o $(LIB) $(DEMOS)
