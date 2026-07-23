# =============================================================================
# Makefile — build the allocator as an LD_PRELOAD shared object, plus benchmarks.
# =============================================================================
# Platform: Linux / WSL (the code calls brk/sbrk and mmap). `make asm` works on
# ANY host because clang cross-targets Linux to emit the teaching assembly.
#
#   make            build libmymalloc.so + bench_glibc + bench_mine
#   make bench      run all three ways (glibc, linked, LD_PRELOAD) side by side
#   make test       correctness smoke test + LD_PRELOAD a real program
#   make asm        regenerate asm/demo.{O0.s,s,O2.s} with the exact clang cmds
#   make clean
# =============================================================================

CC      ?= clang
# -O2 for a fair fight with glibc; -g keeps symbols for perf/gdb; warnings on.
CFLAGS  ?= -Wall -Wextra -O2 -g -fno-omit-frame-pointer
# The allocator is threaded (pthread mutex + a pthread_key thread-exit hook).
LDLIBS  += -pthread

# CRITICAL for an allocator: forbid the compiler from recognising our functions as
# the standard allocation builtins. Without this, an optimiser can rewrite (say) a
# malloc()+memset(0) pair back into a calloc() call — which would re-enter OUR
# calloc and recurse forever. We name our impl my_* to avoid this, but belt-and-
# braces on the interposer translation unit too.
NOBUILTIN := -fno-builtin-malloc -fno-builtin-calloc -fno-builtin-realloc -fno-builtin-free

# Force the System V AMD64 (Linux) ABI for asm generation regardless of host OS.
ASM_TARGET ?= x86_64-pc-linux-gnu

.PHONY: all
all: libmymalloc.so bench_glibc bench_mine

# ---- the shared object you LD_PRELOAD ---------------------------------------
# -fPIC: position-independent (required for a shared library).
# -shared: produce a .so, not an executable.
# Exporting strong `malloc`/`free`/... symbols is what lets the dynamic linker
# resolve a program's allocator calls to us instead of to libc.
libmymalloc.so: mymalloc.c mymalloc.h
	$(CC) $(CFLAGS) $(NOBUILTIN) -fPIC -shared mymalloc.c -o $@ $(LDLIBS)
	@echo "built ./libmymalloc.so — try:  LD_PRELOAD=./libmymalloc.so ls"

# ---- benchmark, built against glibc's allocator -----------------------------
bench_glibc: bench.c
	$(CC) $(CFLAGS) bench.c -o $@ $(LDLIBS)

# ---- benchmark, statically linked with OUR allocator ------------------------
# mymalloc.c's strong `malloc` replaces libc's at link time; -DWITH_MYMALLOC also
# enables the end-of-run stats + heap_check.
bench_mine: bench.c mymalloc.c mymalloc.h
	$(CC) $(CFLAGS) $(NOBUILTIN) -DWITH_MYMALLOC bench.c mymalloc.c -o $@ $(LDLIBS)

# ---- run the three-way comparison -------------------------------------------
.PHONY: bench
bench: all
	@echo "=================== glibc malloc ==================="; ./bench_glibc
	@echo "============ mymalloc (statically linked) =========="; ./bench_mine
	@echo "========= mymalloc via LD_PRELOAD (no relink) ======"; LD_PRELOAD=./libmymalloc.so ./bench_glibc

# ---- correctness smoke test -------------------------------------------------
# bench_mine ends by walking the whole heap (my_heap_check must print 0). Then we
# prove the .so really interposes a real program's allocator.
.PHONY: test
test: all
	./bench_mine 1
	@echo "--- LD_PRELOAD smoke test on a real binary ---"
	LD_PRELOAD=./libmymalloc.so /bin/echo "printed by echo, allocated by us"
	@LD_PRELOAD=./libmymalloc.so ls >/dev/null && echo "ls(1) under our malloc: OK"

# ---- regenerate the committed teaching assembly -----------------------------
# These are the EXACT commands the project spec mandates; the hand-written
# asm/demo.annotated.s is authored, NOT overwritten here.
.PHONY: asm
asm:
	clang --target=$(ASM_TARGET) -S -O0 -fno-asynchronous-unwind-tables -fno-jump-tables -fno-omit-frame-pointer asm/demo.c -o asm/demo.O0.s
	clang --target=$(ASM_TARGET) -S -O1 -fno-asynchronous-unwind-tables -fno-jump-tables -fno-omit-frame-pointer asm/demo.c -o asm/demo.s
	clang --target=$(ASM_TARGET) -S -O2 -fno-asynchronous-unwind-tables -fno-jump-tables asm/demo.c -o asm/demo.O2.s
	@echo "regenerated asm/demo.{O0.s,s,O2.s}; asm/demo.annotated.s is hand-written"

.PHONY: clean
clean:
	rm -f libmymalloc.so bench_glibc bench_mine
