# =============================================================================
# Makefile for masm — a tiny x86-64 assembler that emits relocatable ELF64.
# =============================================================================
# `make`         builds ./masm (works on any host: it only WRITES ELF bytes).
# `make run`     assembles + links + runs examples/dots.s   (needs Linux/WSL:
#                 the link and run steps use an ELF-native `ld` and kernel).
# `make test`    byte-compares masm's encoding against GNU `as`, and round-trips
#                 an object through objdump. (Linux; degrades gracefully.)
# `make asm`     regenerates the committed teaching assembly for asm/demo.c.
# `make clean`   removes build artefacts.
# =============================================================================

CC      ?= clang
CFLAGS  ?= -Wall -Wextra -O2 -std=c11
SRCS     = buf.c lex.c encode.c assemble.c elf.c main.c
HDRS     = asm.h

# clang cross-targets Linux so the generated teaching asm is the repo's ABI
# regardless of the host OS.
ASM_TARGET ?= x86_64-pc-linux-gnu

masm: $(SRCS) $(HDRS)
	$(CC) $(CFLAGS) $(SRCS) -o $@
	@echo "built ./masm — try:  ./masm examples/dots.s -o dots.o -v"

# ---- assemble + link + run the flagship example (Linux) --------------------
# We link with a bare `ld` and use ONLY syscalls, so no libc/CRT is involved —
# exactly like the nolibc reference project. Prints ten dots, exits 0.
.PHONY: run
run: masm
	./masm examples/dots.s -o dots.o -v
	ld dots.o -o dots
	./dots ; echo " (exit=$$?)"

# ---- correctness checks ----------------------------------------------------
# 1) Encode encode_zoo.s with masm AND GNU as; the .text bytes must match
#    (compared up to masm's length, since GNU as may NOP-pad .text to 16).
# 2) Round-trip dots.o back through the disassembler to eyeball the result.
.PHONY: test
test: masm
	./masm examples/encode_zoo.s -o encode_zoo.o
	@if command -v as >/dev/null 2>&1 && command -v objcopy >/dev/null 2>&1; then \
	  as --64 examples/encode_zoo.s -o zoo_gas.o && \
	  objcopy -O binary --only-section=.text zoo_gas.o   zoo_gas.bin && \
	  objcopy -O binary --only-section=.text encode_zoo.o zoo_masm.bin && \
	  n=$$(wc -c < zoo_masm.bin) && \
	  if cmp -n $$n zoo_gas.bin zoo_masm.bin; then \
	    echo "PASS: masm .text is byte-identical to GNU as ($$n bytes)"; \
	  else echo "FAIL: encoding mismatch"; exit 1; fi; \
	else echo "skip: GNU as/objcopy not found (encoding cross-check)"; fi
	./masm examples/dots.s -o dots.o
	@if command -v objdump >/dev/null 2>&1; then \
	  echo "-- objdump round-trip of masm's object --"; objdump -dr dots.o; fi

# ---- regenerate committed teaching assembly for asm/demo.c -----------------
# These are the EXACT commands whose .s outputs are committed. The hand-written
# asm/demo.annotated.s is authored, not generated, so it is never overwritten.
.PHONY: asm
asm:
	$(CC) --target=$(ASM_TARGET) -S -O0 -fno-asynchronous-unwind-tables -fno-jump-tables -fno-omit-frame-pointer asm/demo.c -o asm/demo.O0.s
	$(CC) --target=$(ASM_TARGET) -S -O1 -fno-asynchronous-unwind-tables -fno-jump-tables -fno-omit-frame-pointer asm/demo.c -o asm/demo.s
	$(CC) --target=$(ASM_TARGET) -S -O2 -fno-asynchronous-unwind-tables -fno-jump-tables asm/demo.c -o asm/demo.O2.s
	@echo "regenerated asm/demo.{O0.s,s,O2.s} (demo.annotated.s is hand-authored)"

.PHONY: clean
clean:
	rm -f masm masm.exe dots dots.o encode_zoo.o \
	      zoo_gas.o zoo_gas.bin zoo_masm.bin
