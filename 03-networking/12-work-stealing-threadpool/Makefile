# Makefile — work-stealing thread pool.
#
# The program is LINUX-ONLY: it uses the raw futex(2) syscall, sched_setaffinity,
# pthreads and C11 atomics. Build it on Linux or WSL with `make`. On any host you
# can still regenerate the committed teaching assembly with `make asm`, because
# clang cross-compiles the self-contained asm/demo.c to the Linux target.

CC      ?= clang                 # gcc works too; clang is what the asm assumes
# -std=gnu11: C11 + the POSIX/GNU symbols (clock_gettime, sched_setaffinity, ...)
# exposed by default. threadpool.c also defines _GNU_SOURCE for cpu_set_t.
CFLAGS  ?= -Wall -Wextra -O2 -std=gnu11 -pthread
LDLIBS  ?= -pthread              # pthread_create/join live here (and futex is a raw syscall)

SRCS = main.c threadpool.c chase_lev.c
HDRS = threadpool.h chase_lev.h

# ---- the demo binary --------------------------------------------------------
demo: $(SRCS) $(HDRS)
	$(CC) $(CFLAGS) $(SRCS) -o $@ $(LDLIBS)
	@echo "built ./demo — run it with:  ./demo [N] [workers]"

# ---- run the fork-join demo -------------------------------------------------
.PHONY: run
run: demo
	./demo

# A larger, more visibly-parallel run.
.PHONY: demo-big
demo-big: demo
	./demo 400000000 0

# ---- ThreadSanitizer build --------------------------------------------------
# The whole point of the explicit memory orders is that this must come back
# CLEAN. TSan understands C11 atomics and will flag any genuine data race.
.PHONY: tsan
tsan: $(SRCS) $(HDRS)
	$(CC) -Wall -Wextra -O1 -g -std=c11 -pthread -fsanitize=thread \
	      $(SRCS) -o demo.tsan $(LDLIBS)
	@echo "built ./demo.tsan — run e.g.:  ./demo.tsan 2000000 4"

# ---- teaching assembly for asm/demo.c ---------------------------------------
# These are the EXACT commands the lab mandates; the .s files are committed.
# The hand-written asm/demo.annotated.s is authored, never overwritten here.
ASM_TARGET ?= x86_64-pc-linux-gnu
CLANG      ?= clang
.PHONY: asm
asm:
	$(CLANG) --target=$(ASM_TARGET) -S -O0 -fno-asynchronous-unwind-tables \
	         -fno-jump-tables -fno-omit-frame-pointer asm/demo.c -o asm/demo.O0.s
	$(CLANG) --target=$(ASM_TARGET) -S -O1 -fno-asynchronous-unwind-tables \
	         -fno-jump-tables -fno-omit-frame-pointer asm/demo.c -o asm/demo.s
	$(CLANG) --target=$(ASM_TARGET) -S -O2 -fno-asynchronous-unwind-tables \
	         -fno-jump-tables asm/demo.c -o asm/demo.O2.s
	@echo "regenerated asm/demo.{O0.s,s,O2.s}  (annotated.s is hand-authored)"

# ---- housekeeping -----------------------------------------------------------
.PHONY: clean
clean:
	rm -f demo demo.tsan
