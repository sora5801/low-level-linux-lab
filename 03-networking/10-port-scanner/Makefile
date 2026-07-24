# =============================================================================
# Makefile — build the SYN port scanner and regenerate its teaching assembly.
# =============================================================================
# Platform: LINUX / WSL. synscan.c uses raw sockets + IP_HDRINCL (Linux raw(7)
# semantics), so it BUILDS and RUNS on Linux only. `make asm` works on ANY host
# because clang cross-targets Linux to emit the committed teaching assembly.
#
#   make            build ./synscan
#   make demo       unprivileged connect() scan of localhost (no root needed)
#   make cap        grant the binary CAP_NET_RAW so SYN scans run without sudo
#   make asm        regenerate asm/demo.{O0.s,s,O2.s} with the exact clang cmds
#   make clean
#
# ⚠  Only scan hosts you own or are explicitly authorized to test. See README.md.
# =============================================================================

CC      ?= clang
# -Wall -Wextra: warnings are errors in spirit. -O2: real codegen. -g: symbols.
CFLAGS  ?= -Wall -Wextra -O2 -g
# getaddrinfo/htons/inet_ntop all live in libc on Linux — no extra -l needed.
LDLIBS  +=

# Force the System V AMD64 (Linux) ABI for asm generation regardless of host OS.
ASM_TARGET ?= x86_64-pc-linux-gnu

.PHONY: all
all: synscan

# ---- the scanner ------------------------------------------------------------
synscan: synscan.c
	$(CC) $(CFLAGS) synscan.c -o $@ $(LDLIBS)
	@echo "built ./synscan — SYN scan needs root/CAP_NET_RAW; connect scan (-sT) does not"

# ---- unprivileged demo: connect() scan of your own loopback -----------------
# Uses -sT so it runs as an ordinary user. Whatever is listening locally shows
# "open"; the rest show "closed" (loopback sends a prompt RST) or "filtered".
.PHONY: demo
demo: synscan
	./synscan -sT -p 22,80,443,631,5432,8080 127.0.0.1

# ---- optional: run a real SYN (half-open) scan of loopback ------------------
# Needs privilege. Either `sudo make synscan-demo` or run `make cap` first.
.PHONY: synscan-demo
synscan-demo: synscan
	./synscan -sS -p 1-1024 127.0.0.1

# ---- grant the raw-socket capability so -sS works without sudo --------------
# CAP_NET_RAW is exactly the privilege a raw SOCK_RAW socket requires; this is
# far less than full root. Re-run after every rebuild (the file caps are per-inode).
.PHONY: cap
cap: synscan
	sudo setcap cap_net_raw+ep ./synscan
	@echo "granted CAP_NET_RAW — now:  ./synscan -sS -p 1-1024 127.0.0.1"

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
	rm -f synscan
