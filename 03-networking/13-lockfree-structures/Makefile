# =============================================================================
# Makefile — lock-free data structures (Treiber stack, MS queue, hash map).
# =============================================================================
#   make            build the stress-test binary (./stress)
#   make run        build and run the concurrency stress tests
#   make tsan       rebuild with ThreadSanitizer and run (the real validation)
#   make asm        regenerate the teaching assembly in asm/ (demo.c)
#   make clean      remove build artifacts
#
# Platform: Linux (pthreads + C11 atomics). clang is the reference compiler.
# =============================================================================

CC      ?= clang

# -mcx16 : emit the `lock cmpxchg16b` instruction inline for the Treiber stack's
#          128-bit tagged-pointer CAS. WITHOUT it the compiler falls back to a
#          libatomic call (a hidden lock), defeating the whole exercise. If your
#          toolchain still emits __atomic_compare_exchange_16 calls, add
#          `LDLIBS=-latomic` on the make command line.
# -pthread : link libpthread and set the right preprocessor flags for threads.
# _POSIX_C_SOURCE : expose pthread_barrier_* under strict -std=c11.
CFLAGS  ?= -std=c11 -Wall -Wextra -O2 -g -pthread -mcx16 -D_POSIX_C_SOURCE=200809L
LDFLAGS ?= -pthread
LDLIBS  ?=

SRCS = treiber_stack.c ms_queue.c hashmap.c hazard.c main.c
OBJS = $(SRCS:.c=.o)
HDRS = lockfree.h treiber_stack.h ms_queue.h hashmap.h hazard.h
BIN  = stress

# Flags for the didactic assembly. These MUST match CONVENTIONS.md / the project
# spec exactly so the committed .s files are reproducible.
ASM_TARGET := x86_64-pc-linux-gnu
ASM_COMMON := --target=$(ASM_TARGET) -S -fno-asynchronous-unwind-tables -fno-jump-tables

.PHONY: all run demo tsan asm clean

all: $(BIN)

$(BIN): $(OBJS)
	$(CC) $(LDFLAGS) $^ -o $@ $(LDLIBS)

$(OBJS): $(HDRS)

%.o: %.c
	$(CC) $(CFLAGS) -c $< -o $@

run demo: $(BIN)
	./$(BIN)

# ThreadSanitizer is the honest way to validate a lock-free structure: it models
# the C11 happens-before graph and flags any data race or missing synchronization
# a stress test might otherwise pass by luck. Run it with many threads.
tsan: CFLAGS := $(filter-out -O2,$(CFLAGS)) -O1 -fsanitize=thread
tsan: LDFLAGS += -fsanitize=thread
tsan: clean $(BIN)
	./$(BIN) 8 100000

# Regenerate the three optimization levels of the demo's assembly. demo.c is
# self-contained (no headers), so this works on any host — clang cross-targets
# Linux. The hand-annotated asm/demo.annotated.s is written from demo.s.
asm:
	$(CC) $(ASM_COMMON) -O0 -fno-omit-frame-pointer asm/demo.c -o asm/demo.O0.s
	$(CC) $(ASM_COMMON) -O1 -fno-omit-frame-pointer asm/demo.c -o asm/demo.s
	$(CC) $(ASM_COMMON) -O2 asm/demo.c -o asm/demo.O2.s
	@echo "regenerated asm/demo.{O0.s,s,O2.s}"

clean:
	rm -f $(OBJS) $(BIN)
