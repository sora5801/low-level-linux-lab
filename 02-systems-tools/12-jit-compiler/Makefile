# =============================================================================
# Makefile — a Brainfuck JIT compiler (interpreter + native-code back end).
# =============================================================================
#
# `make`         builds ./bfjit  (LINUX/WSL ONLY: needs mmap/mprotect + PROT_EXEC)
# `make run`     builds, then prints "Hello World!" through the JIT
# `make bench`   builds, then benchmarks the JIT against the interpreter
# `make test`    builds, then runs the correctness+speed self-check (--bench)
# `make asm`     regenerates the committed teaching assembly (works on ANY host,
#                because clang cross-targets Linux; no linker/libc needed for -S)
#
# The runtime target is Linux because the whole point is exercising the Linux
# memory-protection syscalls. The ASSEMBLY, however, is host-portable: we force
# --target=x86_64-pc-linux-gnu so the .s files show the System V AMD64 ABI every
# project in this lab assumes, regardless of the machine you run `make asm` on.

CC      ?= clang
# -Wall -Wextra: warnings are errors in spirit (CONVENTIONS.md §7).
# The JIT itself needs no special flags — it builds executable memory at RUNTIME
# via mmap, not at link time, so there is no -z execstack or trampoline nonsense.
CFLAGS  ?= -Wall -Wextra -O2 -g
LDFLAGS ?=

ASM_TARGET ?= x86_64-pc-linux-gnu

# Object files. emit.o and parse.o are self-contained-ish; jit.o and main.o pull
# in the Linux headers (sys/mman.h, time.h).
OBJS = main.o parse.o interp.o jit.o emit.o

bfjit: $(OBJS)
	$(CC) $(CFLAGS) $(OBJS) -o $@ $(LDFLAGS)
	@echo "built ./bfjit — try:  ./bfjit --hello   or   ./bfjit --bench"

%.o: %.c bf.h emit.h
	$(CC) $(CFLAGS) -c $< -o $@

.PHONY: run
run: bfjit
	./bfjit --hello

.PHONY: bench test
bench test: bfjit
	./bfjit --bench

# -----------------------------------------------------------------------------
# Teaching assembly. We generate .s for BOTH:
#   - asm/demo.c  : the self-contained instruction-encoder extraction, and
#   - emit.c      : the REAL encoder source (it includes no system headers, so
#                   it compiles standalone to the same Linux target).
# The hand-written asm/demo.annotated.s is authored, never overwritten here.
# -----------------------------------------------------------------------------
ASMFLAGS = --target=$(ASM_TARGET) -S -fno-asynchronous-unwind-tables -fno-jump-tables

.PHONY: asm
asm:
	$(CC) $(ASMFLAGS) -O0 -fno-omit-frame-pointer asm/demo.c -o asm/demo.O0.s
	$(CC) $(ASMFLAGS) -O1 -fno-omit-frame-pointer asm/demo.c -o asm/demo.s
	$(CC) $(ASMFLAGS) -O2 asm/demo.c -o asm/demo.O2.s
	$(CC) $(ASMFLAGS) -O0 -fno-omit-frame-pointer emit.c    -o asm/emit.O0.s
	$(CC) $(ASMFLAGS) -O1 -fno-omit-frame-pointer emit.c    -o asm/emit.s
	$(CC) $(ASMFLAGS) -O2 emit.c    -o asm/emit.O2.s
	@echo "regenerated asm/*.s (asm/demo.annotated.s is hand-authored, untouched)"

.PHONY: clean
clean:
	rm -f bfjit $(OBJS)
