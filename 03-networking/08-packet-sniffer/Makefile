# =============================================================================
# Makefile — tcpdump-lite packet sniffer.
# =============================================================================
# `make`        builds ./sniffer (LINUX ONLY: uses AF_PACKET/PACKET_MMAP/BPF).
# `make asm`    regenerates the teaching assembly in asm/ (works on ANY host —
#               clang cross-targets Linux; nothing Linux-specific is needed to
#               emit the .s for the self-contained asm/demo.c).
# `make demo`   builds, then runs a short capture (needs root / CAP_NET_RAW).
# `make dump`   shows what a filter expression compiles to (no privileges).
# `make clean`  removes build artifacts (never the committed .s files).
# =============================================================================

CC      ?= clang
# Warnings are errors in spirit (CONVENTIONS §7). -O2 so the ring-drain loop is
# tight; -g keeps symbols for `perf`/gdb while you learn.
CFLAGS  ?= -Wall -Wextra -O2 -g
LDFLAGS ?=

# The capture program is four translation units (see README "How it works").
OBJS = sniffer.o filter.o decode.o ring.o
BIN  = sniffer

# For asm generation we force the Linux System V AMD64 target so the output is
# the ABI every project in this repo assumes, regardless of the build host.
ASM_TARGET ?= x86_64-pc-linux-gnu
ASM_FLAGS  ?= --target=$(ASM_TARGET) -S \
              -fno-asynchronous-unwind-tables -fno-jump-tables

$(BIN): $(OBJS)
	$(CC) $(CFLAGS) $(OBJS) -o $@ $(LDFLAGS)
	@echo "built ./$(BIN) — try:  sudo ./$(BIN) -i eth0 tcp port 80"

# Each object depends on its own headers plus the shared ones it includes.
sniffer.o: sniffer.c filter.h decode.h ring.h
filter.o:  filter.c filter.h
decode.o:  decode.c decode.h
ring.o:    ring.c ring.h

%.o: %.c
	$(CC) $(CFLAGS) -c $< -o $@

# ---------------------------------------------------------------------------
# Teaching assembly for the classic-BPF interpreter (asm/demo.c). These are the
# EXACT commands CONVENTIONS mandates; the committed .s files are their output.
# The hand-written asm/demo.annotated.s is authored, NOT regenerated here.
# ---------------------------------------------------------------------------
.PHONY: asm
asm:
	$(CC) $(ASM_FLAGS) -O0 -fno-omit-frame-pointer asm/demo.c -o asm/demo.O0.s
	$(CC) $(ASM_FLAGS) -O1 -fno-omit-frame-pointer asm/demo.c -o asm/demo.s
	$(CC) $(ASM_FLAGS) -O2                          asm/demo.c -o asm/demo.O2.s
	@echo "regenerated asm/demo.{O0.s,s,O2.s}; demo.annotated.s is hand-written"

# Convenience: capture 20 packets on the first non-loopback interface. Requires
# privileges for the AF_PACKET socket.
.PHONY: demo
demo: $(BIN)
	@echo "capturing 20 packets (needs root); Ctrl-C to stop early"
	sudo ./$(BIN) -c 20

# Show the compiled BPF for an expression — no capture, no privileges needed.
.PHONY: dump
dump: $(BIN)
	./$(BIN) -d tcp port 80

.PHONY: clean
clean:
	rm -f $(OBJS) $(BIN)
