# =============================================================================
# Makefile — reverse-proxy / L4 load balancer
# =============================================================================
# Targets:
#   make            build ./lb                        (Linux only)
#   make run        build, then run a sample config    (needs backends up)
#   make backends   start two throwaway HTTP backends on :9001 and :9002
#   make asm        regenerate the teaching assembly in asm/ (any host)
#   make clean      remove build artefacts
#
# The proxy uses splice(2), epoll(7), timerfd, signalfd, and accept4 — all
# Linux-specific — so `make`/`make run` require Linux (or WSL2). The `asm`
# target cross-targets Linux via clang and works on any host.
# =============================================================================

CC      ?= cc
CFLAGS  ?= -Wall -Wextra -O2 -g -std=c11
LDFLAGS ?=

BIN     := lb
SRC     := lb.c hashring.c
HDR     := lb.h
OBJ     := $(SRC:.c=.o)

# ---- toolchain for the assembly deliverable (matches tools/gen_asm.sh) ------
ASM_CC     ?= clang
ASM_TARGET ?= x86_64-pc-linux-gnu
ASM_COMMON := --target=$(ASM_TARGET) -S -fno-asynchronous-unwind-tables -fno-jump-tables

.PHONY: all run backends asm clean

all: $(BIN)

$(BIN): $(OBJ)
	$(CC) $(CFLAGS) -o $@ $(OBJ) $(LDFLAGS)

%.o: %.c $(HDR)
	$(CC) $(CFLAGS) -c -o $@ $<

# Run against two local backends on :9001 and :9002 with consistent hashing,
# a warm pool of 4 connections each, and a 1s health interval. Start the
# backends first with `make backends` in another terminal.
run: $(BIN)
	./$(BIN) -l 0.0.0.0:8080 -b 127.0.0.1:9001 -b 127.0.0.1:9002 -p hash -w 4 -i 1000

# Two disposable HTTP backends so you can watch load balancing / health / drain.
# Kill them with Ctrl-C; each labels its responses with its port.
backends:
	@echo "starting backends on :9001 and :9002 (Ctrl-C to stop)"
	@python3 -m http.server 9001 --bind 127.0.0.1 & \
	 python3 -m http.server 9002 --bind 127.0.0.1 & \
	 wait

# ---- teaching assembly: EXACT commands from the project brief ---------------
# Regenerates asm/demo.{O0.s,s,O2.s}. asm/demo.annotated.s is hand-written.
asm:
	$(ASM_CC) $(ASM_COMMON) -O0 -fno-omit-frame-pointer asm/demo.c -o asm/demo.O0.s
	$(ASM_CC) $(ASM_COMMON) -O1 -fno-omit-frame-pointer asm/demo.c -o asm/demo.s
	$(ASM_CC) $(ASM_COMMON) -O2 asm/demo.c -o asm/demo.O2.s
	@echo "regenerated asm/demo.O0.s, asm/demo.s, asm/demo.O2.s"

clean:
	rm -f $(BIN) $(OBJ)
