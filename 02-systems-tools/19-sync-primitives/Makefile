# Makefile — hand-rolled futex synchronization primitives.
#
# `make`        builds ./syncdemo (the self-checking stress harness).
# `make run`    builds and runs it; it prints PASS/FAIL and exits non-zero on failure.
# `make test`   alias for run (CI-friendly: non-zero exit == broken lock).
# `make asm`    regenerates the committed teaching assembly (does NOT touch the
#               hand-written asm/demo.annotated.s).
#
# Platform: the BUILD/RUN targets are Linux or WSL2 only — they need futex(2)
# and pthreads. The `asm` target works on ANY host, because clang cross-targets
# the Linux System V ABI without needing a Linux linker or libc.

CC      ?= cc
# -pthread : link libpthread and set _REENTRANT; the harness spawns OS threads.
# We compile the core and the harness with the same warnings-as-conscience flags.
CFLAGS  ?= -Wall -Wextra -O2 -pthread
LDFLAGS ?= -pthread

# Force the Linux System V AMD64 target for asm generation so the output matches
# the ABI every project in this repo assumes, regardless of the host OS.
ASM_TARGET ?= x86_64-pc-linux-gnu

OBJS = syncprim.o main.o

syncdemo: $(OBJS)
	$(CC) $(CFLAGS) $(OBJS) -o $@ $(LDFLAGS)
	@echo "built ./syncdemo — run it with:  make run"

# syncprim.c is freestanding (no system headers); main.c pulls in pthread/stdio.
syncprim.o: syncprim.c syncprim.h
	$(CC) $(CFLAGS) -c syncprim.c -o $@

main.o: main.c syncprim.h
	$(CC) $(CFLAGS) -c main.c -o $@

.PHONY: run test asm clean
run: syncdemo
	./syncdemo

test: run

# Regenerate asm/*.s for BOTH the self-contained real source (syncprim.c) and
# the focused extraction (asm/demo.c). The exact clang invocations are spelled
# out so the generated files are reproducible byte-for-byte.
asm:
	clang --target=$(ASM_TARGET) -S -O0 -fno-asynchronous-unwind-tables -fno-jump-tables -fno-omit-frame-pointer asm/demo.c -o asm/demo.O0.s
	clang --target=$(ASM_TARGET) -S -O1 -fno-asynchronous-unwind-tables -fno-jump-tables -fno-omit-frame-pointer asm/demo.c -o asm/demo.s
	clang --target=$(ASM_TARGET) -S -O2 -fno-asynchronous-unwind-tables -fno-jump-tables asm/demo.c -o asm/demo.O2.s
	clang --target=$(ASM_TARGET) -S -O0 -fno-asynchronous-unwind-tables -fno-jump-tables -fno-omit-frame-pointer syncprim.c -o asm/syncprim.O0.s
	clang --target=$(ASM_TARGET) -S -O1 -fno-asynchronous-unwind-tables -fno-jump-tables -fno-omit-frame-pointer syncprim.c -o asm/syncprim.s
	clang --target=$(ASM_TARGET) -S -O2 -fno-asynchronous-unwind-tables -fno-jump-tables syncprim.c -o asm/syncprim.O2.s
	@echo "regenerated asm/ (hand-written asm/demo.annotated.s left untouched)"

clean:
	rm -f syncdemo $(OBJS)
