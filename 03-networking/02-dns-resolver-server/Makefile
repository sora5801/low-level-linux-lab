# =============================================================================
# Makefile — DNS resolver + authoritative server (teaching build).
# =============================================================================
# `make`        builds both binaries: ./resolve and ./dnsd
# `make run`    starts the authoritative server on port 5353 (Ctrl-C to stop)
# `make trace`  runs the iterative resolver against example.com with +trace
# `make asm`    regenerates the committed teaching assembly for asm/demo.c
#
# PLATFORM: Linux (or WSL). The code uses POSIX sockets (<sys/socket.h>,
# <netinet/in.h>, <arpa/inet.h>, poll(2)), so it does NOT build on native
# Windows. The `asm` target works on ANY host — clang cross-targets Linux.
# =============================================================================

CC      ?= clang
# -Wall -Wextra: warnings are errors in spirit (see CONVENTIONS.md §7).
# -O2:           realistic optimisation; the code is written to be correct there.
# -g:            keep debug info so gdb/objdump line up with the source.
CFLAGS  ?= -Wall -Wextra -O2 -g -std=c11
# _POSIX_C_SOURCE exposes poll(2), clock_gettime, etc. from glibc.
CFLAGS  += -D_POSIX_C_SOURCE=200809L

# Assembly generation always targets the Linux System V AMD64 ABI, regardless
# of the host OS, so the committed .s matches what every project here assumes.
ASM_TARGET ?= x86_64-pc-linux-gnu

# Shared wire-format / cache / transport objects.
COMMON_SRC = src/wire.c src/net.c src/cache.c
RESOLVE_SRC = src/resolver.c $(COMMON_SRC)
DNSD_SRC    = src/server.c src/zone.c src/wire.c

all: resolve dnsd

# The iterative resolver / dig-like CLI.
resolve: $(RESOLVE_SRC) src/dns.h src/net.h src/cache.h
	$(CC) $(CFLAGS) $(RESOLVE_SRC) -o $@
	@echo "built ./resolve  — try:  ./resolve +trace www.example.com A"

# The authoritative server.
dnsd: $(DNSD_SRC) src/dns.h src/zone.h
	$(CC) $(CFLAGS) $(DNSD_SRC) -o $@
	@echo "built ./dnsd     — try:  ./dnsd zones/example.com.zone 5353"

# Run the authoritative server (unprivileged port 5353).
.PHONY: run
run: dnsd
	./dnsd zones/example.com.zone 5353

# Query our own server (in another terminal, after `make run`):
#   dig @127.0.0.1 -p 5353 www.example.com A
.PHONY: query
query:
	dig @127.0.0.1 -p 5353 www.example.com A

# Watch the iterative resolver walk from the roots (needs outbound UDP/53).
.PHONY: trace
trace: resolve
	./resolve +trace www.example.com A

# Regenerate asm/demo.{O0.s,s,O2.s}. The hand-written demo.annotated.s is NOT
# overwritten — it is authored. Uses the exact flags from CONVENTIONS.md.
.PHONY: asm
asm:
	../../tools/gen_asm.sh asm/demo.c

.PHONY: clean
clean:
	rm -f resolve dnsd
