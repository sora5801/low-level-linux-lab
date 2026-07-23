# =============================================================================
# Makefile — build `minic`, run it, and (re)generate the teaching assembly.
# =============================================================================
# The runtime is LINUX-ONLY: it calls clone(CLONE_NEW*), pivot_root, capset,
# seccomp, and writes cgroupfs. `make` therefore only produces a binary on
# Linux/WSL. The `asm` target works on ANY host, because clang cross-compiles
# the self-contained asm/demo.c to Linux assembly without a Linux toolchain.
# =============================================================================

CC      ?= clang
# -Wall -Wextra : warnings are errors in spirit (see CONVENTIONS.md).
# -O2           : the runtime is real code, so build it optimized.
# _GNU_SOURCE is set per-file in the sources (needed for clone/CLONE_NEW*).
CFLAGS  ?= -Wall -Wextra -O2 -std=gnu11
LDFLAGS ?=

# The runtime, split so the container "story" reads across files (see README).
SRCS = container.c child.c cgroup.c seccomp.c util.c
OBJS = $(SRCS:.c=.o)
BIN  = minic

# Force the Linux System V AMD64 target for asm so the output is the ABI every
# project in this repo assumes, regardless of the host OS.
ASM_TARGET ?= x86_64-pc-linux-gnu
ASM_COMMON  = --target=$(ASM_TARGET) -S -fno-asynchronous-unwind-tables -fno-jump-tables

# ---- default: build the runtime --------------------------------------------
$(BIN): $(OBJS)
	$(CC) $(CFLAGS) $(OBJS) -o $@ $(LDFLAGS)
	@echo "built ./$(BIN) — try (rootless):  ./$(BIN) --rootfs ./rootfs -- /bin/sh"

%.o: %.c container.h util.h
	$(CC) $(CFLAGS) -c $< -o $@

# ---- run: start a shell in a container using ./rootfs ----------------------
# Rootless by default (thanks to the user namespace). If your distro disabled
# unprivileged user namespaces, run under sudo instead.
.PHONY: run
run: $(BIN)
	./$(BIN) --rootfs ./rootfs -- /bin/sh

# ---- test: non-interactive smoke test --------------------------------------
# Runs a canned command inside the container and prints the isolated view:
# hostname, PID (should be 1), and mounts. Proves the namespaces took effect.
.PHONY: test
test: $(BIN)
	@echo ">> container should report hostname=container, PID 1, its own /proc:"
	./$(BIN) --rootfs ./rootfs -- /bin/sh -c \
	  'echo host=$$(hostname); echo pid=$$$$; echo "--- /proc ---"; ls /proc | head'

# ---- rootfs: build a minimal busybox root filesystem for testing -----------
# Needs a static busybox on the host (apt-get install busybox-static). Creates
# ./rootfs with /bin/sh + coreutils applets and the mountpoints minic expects.
.PHONY: rootfs
rootfs:
	@command -v busybox >/dev/null || { echo "install busybox first (busybox-static)"; exit 1; }
	mkdir -p rootfs/bin rootfs/proc rootfs/dev rootfs/etc rootfs/tmp
	cp "$$(command -v busybox)" rootfs/bin/busybox
	cd rootfs/bin && for a in sh ls cat echo ps mount id uname sleep hostname; do ln -sf busybox $$a; done
	@echo "rootfs ready at ./rootfs"

# ---- asm: regenerate the committed teaching assembly -----------------------
# Uses the EXACT commands the project spec pins, so the .s files are reproducible
# clang output. The hand-written asm/demo.annotated.s is authored, not generated,
# and is intentionally NOT overwritten here.
.PHONY: asm
asm:
	$(CC) $(ASM_COMMON) -O0 -fno-omit-frame-pointer asm/demo.c -o asm/demo.O0.s
	$(CC) $(ASM_COMMON) -O1 -fno-omit-frame-pointer asm/demo.c -o asm/demo.s
	$(CC) $(ASM_COMMON) -O2 asm/demo.c -o asm/demo.O2.s
	@echo "regenerated asm/demo.{O0.s,s,O2.s} (annotated.s is hand-written)"

.PHONY: clean
clean:
	rm -f $(OBJS) $(BIN)

# Note: `make clean` does not delete ./rootfs (it is expensive to rebuild) nor
# the committed asm/*.s (they are teaching artifacts). Remove those by hand.
