# =============================================================================
# Makefile — build the crackme, its keygen, the LD_PRELOAD bypass, and the asm.
# =============================================================================
# Targets:
#   make            -> crackme, keygen, libfakeptrace.so           (Linux/WSL)
#   make test       -> keygen a serial and prove the crackme accepts it
#   make bypass     -> show the LD_PRELOAD ptrace hook neutralizing gate 1
#   make demo       -> native self-test of the transform (asm/demo.c)
#   make asm        -> regenerate the committed teaching assembly (any host)
#   make crackme-stripped -> "hard mode": no symbols, for realistic RE practice
#   make clean
#
# Everything except `make asm` needs Linux (or WSL): the crackme calls ptrace(2)
# and uses the System V ABI. `make asm` works anywhere because clang cross-
# targets Linux with -S (no Linux linker/libc needed just to SEE the assembly).
# =============================================================================

CC        ?= clang
ASM_TARGET ?= x86_64-pc-linux-gnu

# --- Mitigation flags for the crackme (deliberately toggled — you own it) -----
# This binary is meant to be reverse-engineered, so we make static analysis
# REPRODUCIBLE rather than hardened. Each flag is a mitigation knob; we say what
# turning it off costs, so the lesson lands:
#
#   -no-pie              Build a non-PIE (fixed-load-address) executable. PIE +
#                        ASLR normally randomize the base every run, so addresses
#                        in the writeup would not match your machine. Disabling
#                        PIE pins them. COST TO AN ATTACKER (in a real target):
#                        PIE+ASLR forces an info-leak before any hardcoded
#                        address is usable — a large hurdle we are waiving here.
#   -fno-stack-protector No stack canary. Canaries detect linear stack-buffer
#                        overflows by placing a random guard before the saved
#                        return address and checking it in the epilogue; without
#                        one the epilogue is clean, so the disassembly is easier
#                        to read. This crackme has no overflow, so it changes
#                        nothing but noise.
#   -O1                  Optimized enough to be realistic (inlining, the folded
#                        constant-time compare) yet still legible. -O0 would be
#                        even easier but unrealistically verbose.
# We KEEP symbols by default so objdump shows `key_from_name`, `validate`, etc.
# `make crackme-stripped` removes them for authentic practice.
CRACKME_CFLAGS ?= -Wall -Wextra -O1 -no-pie -fno-stack-protector

# keygen is an ordinary program; optimize it and let it be small.
KEYGEN_CFLAGS  ?= -Wall -Wextra -O2

.PHONY: all
all: crackme keygen libfakeptrace.so

# --- The target you reverse ---------------------------------------------------
crackme: crackme.c serial.h
	$(CC) $(CRACKME_CFLAGS) crackme.c -o $@
	@echo "built ./crackme  (run: ./crackme <user> <serial>)"

# --- The solution: mints valid serials ---------------------------------------
keygen: keygen.c serial.h
	$(CC) $(KEYGEN_CFLAGS) keygen.c -o $@
	@echo "built ./keygen   (run: ./keygen <user>)"

# --- The anti-debug bypass: an LD_PRELOAD shim that fakes ptrace() ------------
libfakeptrace.so: libfakeptrace.c
	$(CC) -shared -fPIC -Wall libfakeptrace.c -o $@
	@echo "built ./libfakeptrace.so  (use: LD_PRELOAD=./libfakeptrace.so ./crackme ...)"

# --- Hard mode: stripped, no symbol names in objdump -------------------------
.PHONY: crackme-stripped
crackme-stripped: crackme.c serial.h
	$(CC) $(CRACKME_CFLAGS) crackme.c -o crackme-stripped
	strip --strip-all crackme-stripped
	@echo "built ./crackme-stripped  (no symbols — reverse it the hard way)"

# --- Prove the solution: keygen a serial, feed it back, expect exit 0 --------
.PHONY: test
test: crackme keygen
	@set -e; \
	for U in alice bob "Ada Lovelace"; do \
	  S=$$(./keygen "$$U"); \
	  printf 'keygen(%-12s) = %s  -> ' "$$U" "$$S"; \
	  if ./crackme "$$U" "$$S" >/dev/null; then echo "ACCEPTED"; \
	  else echo "REJECTED (BUG!)"; exit 1; fi; \
	done; \
	echo "all serials accepted."

# --- Demonstrate the LD_PRELOAD anti-debug bypass -----------------------------
# Under gdb the ptrace gate would abort; the preloaded fake ptrace() returns 0,
# so the crackme proceeds. (Requires Linux; run under gdb to see the contrast.)
.PHONY: bypass
bypass: crackme keygen libfakeptrace.so
	@S=$$(./keygen alice); \
	echo "# normal run:"; ./crackme alice "$$S"; \
	echo "# with fake ptrace preloaded (would also survive gdb):"; \
	LD_PRELOAD=./libfakeptrace.so ./crackme alice "$$S"

# --- Native self-test of the pure transform (asm/demo.c) ---------------------
# demo.c is header-free (so its generated asm is clean); under DEMO_MAIN it grows
# a tiny main that prints a serial via a raw write(2) syscall and checks that
# validate() agrees. Built with libc's _start (no -nostdlib) so main is called.
.PHONY: demo
demo: asm/demo.c
	$(CC) -Wall -Wextra -O2 -DDEMO_MAIN asm/demo.c -o demo_selftest
	@echo "built ./demo_selftest  (run: ./demo_selftest alice  -> prints its serial)"

# --- Regenerate the committed teaching assembly ------------------------------
# These are the exact commands the lab's CONVENTIONS require; they write the
# three .s files next to the hand-annotated demo.annotated.s (which is authored,
# never overwritten). Works on any host — clang cross-targets Linux.
.PHONY: asm
asm:
	$(CC) --target=$(ASM_TARGET) -S -O0 -fno-asynchronous-unwind-tables -fno-jump-tables -fno-omit-frame-pointer asm/demo.c -o asm/demo.O0.s
	$(CC) --target=$(ASM_TARGET) -S -O1 -fno-asynchronous-unwind-tables -fno-jump-tables -fno-omit-frame-pointer asm/demo.c -o asm/demo.s
	$(CC) --target=$(ASM_TARGET) -S -O2 -fno-asynchronous-unwind-tables -fno-jump-tables asm/demo.c -o asm/demo.O2.s
	@echo "regenerated asm/demo.{O0.s,s,O2.s}; asm/demo.annotated.s is hand-authored."

.PHONY: clean
clean:
	rm -f crackme keygen crackme-stripped libfakeptrace.so demo_selftest
