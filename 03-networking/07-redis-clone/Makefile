# =============================================================================
# Makefile for the teaching Redis clone.
#
# PLATFORM: Linux or WSL2. The server uses epoll(7), accept4(2), and fork(2)
# copy-on-write — all Linux facilities — so `make` (which builds the server)
# only works there. On ANY host, `make asm` still regenerates the committed
# teaching assembly, because clang cross-compiles to the Linux target without a
# Linux toolchain to link.
# =============================================================================

CC      ?= clang
# -Wall -Wextra : warnings are errors in spirit (this repo's rule 7).
# -O2           : the hot path is byte-shuffling and hashing; let it be fast.
# -g            : keep symbols so perf/gdb can map hot spots back to source.
# -std=gnu11    : C11 plus the GNU feature-test defaults glibc needs for POSIX.
#                 (server.c additionally #defines _GNU_SOURCE for accept4/epoll.)
CFLAGS  ?= -Wall -Wextra -O2 -g -std=gnu11

# The System V AMD64 (Linux) target used for the committed teaching assembly.
ASM_TARGET ?= x86_64-pc-linux-gnu

BIN  := redis-clone
SRCS := zmalloc.c sds.c dict.c object.c db.c resp.c commands.c pubsub.c \
        persist.c server.c
OBJS := $(SRCS:.c=.o)
HDRS := zmalloc.h sds.h dict.h server.h

.PHONY: all
all: $(BIN)

# Link the server. Every .o depends on the headers so an edit to server.h
# rebuilds the world (these are teaching sources; correctness over speed).
$(BIN): $(OBJS)
	$(CC) $(CFLAGS) $(OBJS) -o $@
	@echo "built ./$(BIN) — try:  ./$(BIN) 6379   (then: redis-cli -p 6379 ping)"

%.o: %.c $(HDRS)
	$(CC) $(CFLAGS) -c $< -o $@

# ---- run / test -----------------------------------------------------------
# Start the server in the foreground on :6379. In another terminal:
#     redis-cli -p 6379 set foo bar
#     redis-cli -p 6379 get foo
#     redis-cli -p 6379 lpush mylist a b c
#     redis-cli -p 6379 lrange mylist 0 -1
.PHONY: run
run: $(BIN)
	./$(BIN) 6379

# A minimal, dependency-free smoke test: start the server, drive it with a raw
# RESP/inline script over /dev/tcp, and print the replies. Requires bash and a
# free port 6399. Purely optional — it exercises the real event loop end to end.
.PHONY: test
test: SHELL := /bin/bash
test: $(BIN)
	@echo ">> starting server on :6399"; \
	./$(BIN) 6399 >/tmp/redis-clone-test.log 2>&1 & pid=$$!; sleep 1; \
	exec 3<>/dev/tcp/127.0.0.1/6399; \
	printf 'SET foo bar\r\nGET foo\r\nINCR n\r\nINCR n\r\nLPUSH l a b c\r\nLRANGE l 0 -1\r\nHSET h f1 v1 f2 v2\r\nHGET h f1\r\nDBSIZE\r\nPING\r\n' >&3; \
	sleep 1; timeout 1 cat <&3 || true; exec 3<&-; exec 3>&-; \
	kill $$pid 2>/dev/null; \
	echo; echo ">> server log:"; cat /tmp/redis-clone-test.log

# ---- teaching assembly ----------------------------------------------------
# Regenerate asm/demo.{s,O0.s,O2.s} with the EXACT flags from the project spec.
# asm/demo.annotated.s is hand-written and never overwritten by this target.
.PHONY: asm
asm:
	clang --target=$(ASM_TARGET) -S -O0 -fno-asynchronous-unwind-tables \
	      -fno-jump-tables -fno-omit-frame-pointer asm/demo.c -o asm/demo.O0.s
	clang --target=$(ASM_TARGET) -S -O1 -fno-asynchronous-unwind-tables \
	      -fno-jump-tables -fno-omit-frame-pointer asm/demo.c -o asm/demo.s
	clang --target=$(ASM_TARGET) -S -O2 -fno-asynchronous-unwind-tables \
	      -fno-jump-tables asm/demo.c -o asm/demo.O2.s
	@echo "regenerated asm/demo.*.s — asm/demo.annotated.s is hand-written"

.PHONY: clean
clean:
	rm -f $(OBJS) $(BIN)
