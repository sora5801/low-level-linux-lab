# =============================================================================
# Makefile — mini-WireGuard VPN.
# =============================================================================
# `make`        builds ./mini-wg   (LINUX ONLY: uses /dev/net/tun + UDP sockets).
# `make asm`    regenerates the teaching assembly in asm/ (works on ANY host —
#               clang cross-targets Linux; asm/demo.c is self-contained).
# `make keygen` builds, then prints a fresh base64 keypair (no privileges).
# `make run`    prints the two-peer quickstart you paste into two shells.
# `make clean`  removes build artifacts (never the committed .s files).
# =============================================================================

CC      ?= clang
# Warnings are errors in spirit (CONVENTIONS §7). -O2 keeps the ChaCha/Poly and
# ladder inner loops tight; -g keeps symbols for perf/gdb while you learn.
CFLAGS  ?= -Wall -Wextra -O2 -g
LDFLAGS ?=

# Eight translation units (see README "How it works").
OBJS = vpn.o noise.o x25519.o blake2s.o chacha20poly1305.o replay.o tun.o util.o
BIN  = mini-wg

# For asm generation we force the Linux System V AMD64 target so the output is
# the ABI every project in this repo assumes, regardless of build host.
ASM_TARGET ?= x86_64-pc-linux-gnu
ASM_FLAGS  ?= --target=$(ASM_TARGET) -S \
              -fno-asynchronous-unwind-tables -fno-jump-tables

$(BIN): $(OBJS)
	$(CC) $(CFLAGS) $(OBJS) -o $@ $(LDFLAGS)
	@echo "built ./$(BIN) — see 'make run' for the two-peer quickstart"

# Header dependencies (each object recompiles when a header it includes changes).
vpn.o:                vpn.c wg.h noise.h x25519.h tun.h
noise.o:              noise.c noise.h wg.h replay.h x25519.h blake2s.h chacha20poly1305.h
x25519.o:             x25519.c x25519.h wg.h
blake2s.o:            blake2s.c blake2s.h wg.h
chacha20poly1305.o:   chacha20poly1305.c chacha20poly1305.h wg.h
replay.o:             replay.c replay.h wg.h
tun.o:                tun.c tun.h wg.h
util.o:               util.c wg.h

%.o: %.c
	$(CC) $(CFLAGS) -c $< -o $@

# ---------------------------------------------------------------------------
# Teaching assembly for the ChaCha20 core (asm/demo.c). These are the EXACT
# commands CONVENTIONS mandates; the committed .s files are their output. The
# hand-written asm/demo.annotated.s is authored, NOT regenerated here.
# ---------------------------------------------------------------------------
.PHONY: asm
asm:
	$(CC) $(ASM_FLAGS) -O0 -fno-omit-frame-pointer asm/demo.c -o asm/demo.O0.s
	$(CC) $(ASM_FLAGS) -O1 -fno-omit-frame-pointer asm/demo.c -o asm/demo.s
	$(CC) $(ASM_FLAGS) -O2                          asm/demo.c -o asm/demo.O2.s
	@echo "regenerated asm/demo.{O0.s,s,O2.s}; demo.annotated.s is hand-written"

# Convenience: print a keypair (32 random bytes + its Curve25519 public key).
.PHONY: keygen
keygen: $(BIN)
	./$(BIN) -g

# Print the copy-paste quickstart for a local two-peer loopback tunnel.
.PHONY: run
run: $(BIN)
	@echo "Generate two keypairs:      ./$(BIN) -g   (run twice)"
	@echo "Peer A (responder):  sudo ./$(BIN) -i wg0 -l 51820 -k <A_priv> -R <B_pub>"
	@echo "                     sudo ip addr add 10.9.0.1/24 dev wg0"
	@echo "Peer B (initiator):  sudo ./$(BIN) -i wg1 -l 51821 -k <B_priv> -R <A_pub> -e 127.0.0.1:51820"
	@echo "                     sudo ip addr add 10.9.0.2/24 dev wg1"
	@echo "Then:                ping 10.9.0.1   (from B's namespace)"

.PHONY: clean
clean:
	rm -f $(OBJS) $(BIN)
