# Makefile for the init / PID 1 / service supervisor.
#
# `make`      -> build ./supervisor (Linux/WSL only: uses signalfd + prctl)
# `make run`  -> build and run it against services.conf (subreaper mode)
# `make asm`  -> regenerate the committed teaching assembly (asm/*.s)
# `make clean`-> remove the binary
#
# The binary only builds on Linux, because signalfd(2) and PR_SET_CHILD_SUBREAPER
# are Linux-specific. The ASSEMBLY, however, regenerates on any host: clang is a
# cross-compiler, so `make asm` works from Windows/macOS too.

CC      ?= cc
# -D_GNU_SOURCE : expose signalfd(), SFD_CLOEXEC/SFD_NONBLOCK, and the prctl
#                 PR_SET_CHILD_SUBREAPER constant from glibc's headers.
# -Wall -Wextra : the house rule — warnings are errors in spirit.
CFLAGS  ?= -std=c11 -Wall -Wextra -O2 -D_GNU_SOURCE

SRCS    = supervisor.c config.c order.c
HDRS    = service.h config.h order.h
BIN     = supervisor

# clang cross-target used for the teaching assembly (System V AMD64 / Linux),
# regardless of the host OS. Matches every other project in the lab.
ASM_TARGET ?= x86_64-pc-linux-gnu
ASM_COMMON = --target=$(ASM_TARGET) -S -fno-asynchronous-unwind-tables -fno-jump-tables

$(BIN): $(SRCS) $(HDRS)
	$(CC) $(CFLAGS) $(SRCS) -o $@
	@echo "built ./$(BIN) — try:  ./$(BIN) services.conf   (Ctrl-C to shut down)"

# Run in "subreaper mode" as an ordinary process. It is NOT PID 1 here, but
# PR_SET_CHILD_SUBREAPER still makes it reap orphans, so you see the full
# lifecycle. See the README for running it as real PID 1 inside a container.
.PHONY: run
run: $(BIN)
	./$(BIN) services.conf

# Regenerate the committed teaching assembly with the EXACT flags the lab uses.
# We generate for BOTH the self-contained real source (order.c) and the didactic
# extraction (asm/demo.c). The hand-written asm/demo.annotated.s is authored and
# intentionally NOT overwritten here.
.PHONY: asm
asm:
	clang $(ASM_COMMON) -O0 -fno-omit-frame-pointer asm/demo.c -o asm/demo.O0.s
	clang $(ASM_COMMON) -O1 -fno-omit-frame-pointer asm/demo.c -o asm/demo.s
	clang $(ASM_COMMON) -O2 asm/demo.c -o asm/demo.O2.s
	clang $(ASM_COMMON) -O0 -fno-omit-frame-pointer order.c   -o asm/order.O0.s
	clang $(ASM_COMMON) -O1 -fno-omit-frame-pointer order.c   -o asm/order.s
	clang $(ASM_COMMON) -O2 order.c   -o asm/order.O2.s
	@echo "regenerated asm/{demo,order}.{O0.s,s,O2.s}; demo.annotated.s is hand-authored"

.PHONY: clean
clean:
	rm -f $(BIN)
