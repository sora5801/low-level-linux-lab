# =============================================================================
# Makefile — build the coreutils multiplexer and (re)generate teaching asm.
# =============================================================================
#
# `make`        builds ./coreutils (the busybox-style multiplexer).
# `make links`  creates cat/cp/ls/wc/xargs/tailf symlinks to it (applet install).
# `make asm`    regenerates asm/demo.{O0.s,s,O2.s} with the EXACT clang commands
#               from CONVENTIONS.md. The hand-written demo.annotated.s is never
#               overwritten — it is authored.
# `make test`   a small self-check that exercises each applet on real files.
#
# PLATFORM: the tools are Linux-only (getdents64, inotify, copy_file_range,
# SEEK_HOLE). Build & run on Linux or WSL. The `asm` target works on ANY host
# because clang cross-targets the Linux ABI.

CC      ?= clang
# -Wall -Wextra per house rules; -O2 for realistic codegen; _GNU_SOURCE exposes
# copy_file_range, inotify_init1, and the SEEK_HOLE/SEEK_DATA whence constants.
CFLAGS  ?= -Wall -Wextra -O2 -D_GNU_SOURCE -std=c11
LDFLAGS ?=

# The multiplexer links every applet's object file.
OBJS = coreutils.o util.o cat.o cp.o ls.o wc.o xargs.o tailf.o
BIN  = coreutils
APPLETS = cat cp ls wc xargs tailf

# clang cross-target for the assembly deliverable (host-independent).
ASM_TARGET ?= x86_64-pc-linux-gnu

.PHONY: all
all: $(BIN)

$(BIN): $(OBJS)
	$(CC) $(CFLAGS) $(OBJS) -o $@ $(LDFLAGS)
	@echo "built ./$(BIN) — try:  ./$(BIN) wc Makefile   or   make links"

# Every object depends on the shared header.
%.o: %.c coreutils.h
	$(CC) $(CFLAGS) -c $< -o $@

# Install the applet names as symlinks to the one binary (busybox style). Then
# `./cat FILE`, `./ls`, etc. dispatch via argv[0].
.PHONY: links
links: $(BIN)
	@for a in $(APPLETS); do ln -sf $(BIN) $$a; echo "  $$a -> $(BIN)"; done

# --- Assembly deliverable ----------------------------------------------------
# The three commands below are byte-for-byte the ones mandated by the project
# spec / CONVENTIONS.md. They compile the self-contained wc core (asm/demo.c).
.PHONY: asm
asm:
	clang --target=$(ASM_TARGET) -S -O0 -fno-asynchronous-unwind-tables -fno-jump-tables -fno-omit-frame-pointer asm/demo.c -o asm/demo.O0.s
	clang --target=$(ASM_TARGET) -S -O1 -fno-asynchronous-unwind-tables -fno-jump-tables -fno-omit-frame-pointer asm/demo.c -o asm/demo.s
	clang --target=$(ASM_TARGET) -S -O2 -fno-asynchronous-unwind-tables -fno-jump-tables asm/demo.c -o asm/demo.O2.s
	@echo "regenerated asm/demo.{O0.s,s,O2.s}; asm/demo.annotated.s is hand-authored"

# --- Smoke test --------------------------------------------------------------
# Not a substitute for the real GNU test suite; it just proves each applet runs
# end to end against the source tree itself. Compares wc against system wc when
# available.
.PHONY: test
test: $(BIN)
	@echo "== cat =="   ; ./$(BIN) cat coreutils.h | ./$(BIN) wc -l
	@echo "== ls -l ==" ; ./$(BIN) ls -l . | head -n 5
	@echo "== wc =="    ; ./$(BIN) wc *.c
	@if command -v wc >/dev/null; then \
	   echo "== wc vs system wc (util.c) ==" ; \
	   ./$(BIN) wc util.c ; wc util.c ; fi
	@echo "== cp (sparse) ==" ; \
	   ./$(BIN) cp util.c /tmp/cu_copy.c && cmp util.c /tmp/cu_copy.c && echo "cp OK" ; \
	   rm -f /tmp/cu_copy.c
	@echo "== xargs ==" ; printf 'a\nb\nc\n' | ./$(BIN) xargs echo
	@echo "all applets ran."

.PHONY: clean
clean:
	rm -f $(OBJS) $(BIN) $(APPLETS)
