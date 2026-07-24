# =============================================================================
# Makefile — build the userspace TCP/IP stack and (re)generate teaching asm.
# =============================================================================
# Platform: LINUX ONLY. The stack opens /dev/net/tun and uses Linux ioctls, so
# it does not build or run on macOS/Windows. The `asm` target, however, is
# host-portable: clang cross-targets Linux to emit the committed assembly.
#
#   make            # build ./tcpip
#   make run        # build, then run it (needs root/CAP_NET_ADMIN for the tap)
#   make asm        # regenerate asm/demo.{O0.s,s,O2.s} (demo.annotated.s is
#                   #   hand-written and never overwritten)
#   make clean
# =============================================================================

CC      ?= clang
# -Wall -Wextra: warnings are errors in spirit (CONVENTIONS.md).
# -std=gnu11    : C11 plus the POSIX/GNU extensions we use (clock_gettime,
#                 sigaction, poll, the linux/if_tun ioctls).
# -O2 -g        : optimized but debuggable; the stack is I/O-bound anyway.
CFLAGS  ?= -Wall -Wextra -std=gnu11 -O2 -g
LDFLAGS ?=

TARGET  := tcpip
SRCS    := main.c ether.c arp.c ip.c icmp.c udp.c tcp.c tap.c checksum.c
OBJS    := $(SRCS:.c=.o)
HDRS    := $(wildcard *.h)

all: $(TARGET)

$(TARGET): $(OBJS)
	$(CC) $(CFLAGS) -o $@ $(OBJS) $(LDFLAGS)

# Every object depends on every header — small project, keep it simple/correct.
%.o: %.c $(HDRS)
	$(CC) $(CFLAGS) -c $< -o $@

# Run it. The tap device requires privilege; see the README for the host-side
# `ip addr`/`ip link` commands that make ping/nc reach 10.0.0.2.
run: $(TARGET)
	sudo ./$(TARGET)

# -----------------------------------------------------------------------------
# Teaching assembly. These are the EXACT commands the project spec pins, so the
# committed .s files are reproducible. demo.annotated.s is written by hand from
# demo.s (-O1) and is intentionally NOT a target here.
# -----------------------------------------------------------------------------
ASM_TGT    := x86_64-pc-linux-gnu
ASM_COMMON := --target=$(ASM_TGT) -S -fno-asynchronous-unwind-tables -fno-jump-tables

asm:
	clang $(ASM_COMMON) -O0 -fno-omit-frame-pointer asm/demo.c -o asm/demo.O0.s
	clang $(ASM_COMMON) -O1 -fno-omit-frame-pointer asm/demo.c -o asm/demo.s
	clang $(ASM_COMMON) -O2                          asm/demo.c -o asm/demo.O2.s
	@echo "regenerated asm/demo.{O0.s,s,O2.s}; demo.annotated.s is hand-written"

clean:
	rm -f $(OBJS) $(TARGET)

.PHONY: all run asm clean
