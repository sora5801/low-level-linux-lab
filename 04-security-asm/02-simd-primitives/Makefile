# =============================================================================
# Makefile — SIMD primitives (strlen / memchr / memcpy / UTF-8) 🟧
# =============================================================================
#
# PLATFORM: `make`, `make test`, `make bench` build and RUN the full program,
# which links the hand-written simd_asm.S. That file uses the System V AMD64
# ABI, so the runnable binary is LINUX / WSL x86-64 ONLY. On any host you can
# still regenerate the teaching assembly with `make asm` (clang cross-targets
# Linux) and read every committed .s.
#
# SECURITY / DEFENSE POSTURE (see README "Defense"): these primitives are the
# exact functions memory-corruption bugs abuse (an off-by-one in a memcpy, a
# missing NUL for strlen). So unlike the deliberately-vulnerable projects in
# this section, we build this one HARDENED — every standard mitigation ON — and
# the assembly declares a non-executable stack. The flags below are the lesson.
# =============================================================================

CC      ?= clang

# Correctness + hardening. We do NOT pass a global -mavx2: each intrinsic
# function opts into its ISA with __attribute__((target(...))) so the binary
# still runs on pre-AVX2 CPUs and only executes AVX2 after CPUID clears it.
CFLAGS  ?= -std=c11 -Wall -Wextra -O2 \
           -fstack-protector-strong -D_FORTIFY_SOURCE=2 \
           -fPIE
LDFLAGS ?= -pie -Wl,-z,relro,-z,now -Wl,-z,noexecstack

# For asm generation we force the Linux System V target, matching every project
# in this repo, regardless of the host OS.
ASM_TARGET ?= x86_64-pc-linux-gnu

OBJS = cpuid.o scalar.o simd_intrin.o dispatch.o simd_asm.o bench.o
BIN  = simd_bench

.PHONY: all
all: $(BIN)

# The hand-written assembly is assembled straight to an object by the C driver.
simd_asm.o: simd_asm.S
	$(CC) -c $< -o $@

%.o: %.c simd_primitives.h
	$(CC) $(CFLAGS) -c $< -o $@

$(BIN): $(OBJS)
	$(CC) $(CFLAGS) $(OBJS) -o $@ $(LDFLAGS)
	@echo "built ./$(BIN) — 'make test' checks correctness, 'make bench' times it"

# Correctness gate: differential-test every SIMD variant vs scalar AND glibc.
.PHONY: test
test: $(BIN)
	./$(BIN)

# Throughput: same tests, then print GB/s next to glibc for each primitive.
.PHONY: bench
bench: $(BIN)
	./$(BIN) bench

# -----------------------------------------------------------------------------
# Regenerate the committed teaching assembly for asm/demo.c. The hand-written
# asm/demo.annotated.s and simd_asm.S are authored, NOT overwritten here.
# These are the exact three commands the project ships the output of.
# -----------------------------------------------------------------------------
.PHONY: asm
asm:
	cd asm && $(CC) --target=$(ASM_TARGET) -S -O0 -fno-asynchronous-unwind-tables -fno-jump-tables -fno-omit-frame-pointer demo.c -o demo.O0.s
	cd asm && $(CC) --target=$(ASM_TARGET) -S -O1 -fno-asynchronous-unwind-tables -fno-jump-tables -fno-omit-frame-pointer demo.c -o demo.s
	cd asm && $(CC) --target=$(ASM_TARGET) -S -O2 -fno-asynchronous-unwind-tables -fno-jump-tables demo.c -o demo.O2.s
	@echo "regenerated asm/demo.{O0.s,s,O2.s}; annotated files are hand-authored"

# Disassemble the hand-written .S to prove the committed encodings are genuine.
.PHONY: disasm
disasm: simd_asm.o
	llvm-objdump -d simd_asm.o || objdump -d simd_asm.o

.PHONY: clean
clean:
	rm -f $(OBJS) $(BIN)
