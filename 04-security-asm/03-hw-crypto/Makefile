# =============================================================================
# Makefile — hardware crypto (AES-NI / SHA-NI) with a constant-time fallback.
# =============================================================================
#
# Build on Linux/WSL (or macOS) with clang or gcc. The key idea is PER-FILE ISA
# flags: only the files that use the special instructions are compiled with the
# matching -m flag, so the rest of the program stays portable baseline x86-64.
#
#   aes_ni.c    needs  -maes -msse4.1        (aesenc/aesenclast/aeskeygenassist)
#   sha256_ni.c needs  -msha -mssse3 -msse4.1(sha256rnds2/msg1/msg2 + pshufb/blend)
#
# Compiling those TUs with the ISA flag lets clang EMIT those instructions; it is
# still safe to run on an old CPU because main.c only CALLS them after `cpuid`
# says they exist (see cpuid.h). Everything else — the constant-time software
# AES, the software SHA-256, the driver, main — is plain baseline code.
# =============================================================================

CC       ?= clang
CFLAGS   ?= -Wall -Wextra -O2 -std=c11
ASM_TARGET ?= x86_64-pc-linux-gnu

# Per-file instruction-set flags (see above).
AESNI_FLAGS ?= -maes -msse4.1
SHANI_FLAGS ?= -msha -mssse3 -msse4.1

BIN     = hwcrypto
# Object files. Baseline objects use $(CFLAGS) only; the two *_ni objects add
# their ISA flags via the explicit rules below.
OBJS    = main.o aes_ct.o sha256.o sha256_soft.o aes_ni.o sha256_ni.o

.PHONY: all run test asm clean
all: $(BIN)

$(BIN): $(OBJS)
	$(CC) $(CFLAGS) $^ -o $@
	@echo "built ./$(BIN) — run it with:  make test"

# Baseline (portable) translation units.
main.o:        main.c        cpuid.h aes.h sha256.h ; $(CC) $(CFLAGS) -c $< -o $@
aes_ct.o:      aes_ct.c      aes.h                  ; $(CC) $(CFLAGS) -c $< -o $@
sha256.o:      sha256.c      sha256.h               ; $(CC) $(CFLAGS) -c $< -o $@
sha256_soft.o: sha256_soft.c sha256.h               ; $(CC) $(CFLAGS) -c $< -o $@

# ISA-specific translation units (extra -m flags, still only invoked after cpuid).
aes_ni.o:      aes_ni.c      aes.h    ; $(CC) $(CFLAGS) $(AESNI_FLAGS) -c $< -o $@
sha256_ni.o:   sha256_ni.c   sha256.h ; $(CC) $(CFLAGS) $(SHANI_FLAGS) -c $< -o $@

# Run the full known-answer test suite; exit status = number of failures.
run test: $(BIN)
	./$(BIN)

# -----------------------------------------------------------------------------
# Regenerate the committed teaching assembly. These are the exact commands from
# the project brief; the hand-written *.annotated.s files are NOT overwritten.
#
# aes_round.c pulls in <wmmintrin.h>, which transitively includes <stdlib.h>. On
# Linux that is present; if you regenerate on a host WITHOUT Linux headers, add
#   -isystem /path/to/stub   (a dir with a stdlib.h declaring malloc/free/
#    posix_memalign) so the unused inline _mm_malloc parses. The emitted asm does
# not reference any of it.
# -----------------------------------------------------------------------------
ASMFLAGS = --target=$(ASM_TARGET) -S -fno-asynchronous-unwind-tables -fno-jump-tables
asm:
	$(CC) $(ASMFLAGS) -O0 -fno-omit-frame-pointer asm/demo.c -o asm/demo.O0.s
	$(CC) $(ASMFLAGS) -O1 -fno-omit-frame-pointer asm/demo.c -o asm/demo.s
	$(CC) $(ASMFLAGS) -O2                         asm/demo.c -o asm/demo.O2.s
	$(CC) $(ASMFLAGS) -O1 -fno-omit-frame-pointer $(AESNI_FLAGS) asm/aes_round.c -o asm/aes_round.s
	$(CC) $(ASMFLAGS) -O0 -fno-omit-frame-pointer $(AESNI_FLAGS) asm/aes_round.c -o asm/aes_round.O0.s
	$(CC) $(ASMFLAGS) -O2                         $(AESNI_FLAGS) asm/aes_round.c -o asm/aes_round.O2.s
	@echo "regenerated asm/*.s (annotated files left untouched)"

clean:
	rm -f $(BIN) $(OBJS)
