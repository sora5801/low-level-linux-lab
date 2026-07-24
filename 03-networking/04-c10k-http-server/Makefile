# Makefile for the C10k HTTP/1.1 server.
#
# Build on Linux (or WSL): the server uses epoll, accept4, sendfile, and
# SO_REUSEPORT, which are Linux-only. On any host you can still regenerate the
# teaching assembly with `make asm`, because clang cross-compiles to the Linux
# target without needing a Linux toolchain to link.

CC      ?= clang
# -Wall -Wextra : warnings are errors in spirit (this repo's rule 7).
# -O2           : the server is I/O bound, but let the parser be fast.
# -g            : keep symbols so `perf`/gdb can map hot spots back to source.
# _GNU_SOURCE is set in the source itself (must precede the first include).
CFLAGS  ?= -Wall -Wextra -O2 -g

# The System V AMD64 (Linux) target used for the committed teaching assembly.
ASM_TARGET ?= x86_64-pc-linux-gnu

BIN     := server
SRCS    := server.c http_parser.c
HDRS    := http_parser.h

.PHONY: all
all: $(BIN)

$(BIN): $(SRCS) $(HDRS)
	$(CC) $(CFLAGS) $(SRCS) -o $@
	@echo "built ./$(BIN) — try:  ./$(BIN) 8080 ./www 4"

# ---- run/demo -------------------------------------------------------------
# Start the server on :8080 with 4 worker processes serving ./www, then fetch
# the index page from another terminal:  curl -v http://localhost:8080/
.PHONY: run
run: $(BIN)
	./$(BIN) 8080 ./www 4

# A quick smoke test: keep-alive request pair over one connection with curl.
.PHONY: demo
demo: $(BIN)
	@echo "In another shell:  curl -v --http1.1 http://localhost:8080/"
	@echo "Load test:         wrk -t4 -c10000 -d10s http://localhost:8080/"
	./$(BIN) 8080 ./www 4

# ---- teaching assembly ----------------------------------------------------
# Regenerates asm/demo.{s,O0.s,O2.s} AND asm/http_parser.{s,O0.s,O2.s}, since
# both demo.c and http_parser.c are self-contained (no system headers). The
# hand-written asm/demo.annotated.s is authored, never overwritten.
.PHONY: asm
asm:
	clang --target=$(ASM_TARGET) -S -O0 -fno-asynchronous-unwind-tables \
	      -fno-jump-tables -fno-omit-frame-pointer asm/demo.c -o asm/demo.O0.s
	clang --target=$(ASM_TARGET) -S -O1 -fno-asynchronous-unwind-tables \
	      -fno-jump-tables -fno-omit-frame-pointer asm/demo.c -o asm/demo.s
	clang --target=$(ASM_TARGET) -S -O2 -fno-asynchronous-unwind-tables \
	      -fno-jump-tables asm/demo.c -o asm/demo.O2.s
	clang --target=$(ASM_TARGET) -S -O0 -fno-asynchronous-unwind-tables \
	      -fno-jump-tables -fno-omit-frame-pointer http_parser.c -o asm/http_parser.O0.s
	clang --target=$(ASM_TARGET) -S -O1 -fno-asynchronous-unwind-tables \
	      -fno-jump-tables -fno-omit-frame-pointer http_parser.c -o asm/http_parser.s
	clang --target=$(ASM_TARGET) -S -O2 -fno-asynchronous-unwind-tables \
	      -fno-jump-tables http_parser.c -o asm/http_parser.O2.s
	@echo "regenerated asm/*.s — asm/demo.annotated.s is hand-written, left alone"

.PHONY: clean
clean:
	rm -f $(BIN)
