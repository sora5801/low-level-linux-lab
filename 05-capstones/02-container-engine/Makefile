# =============================================================================
# Makefile — build `ceng`, run it, build/pull a rootfs, and (re)generate the
# teaching assembly.
# =============================================================================
# The engine is LINUX-ONLY: it calls clone(CLONE_NEW*), overlay mount(2),
# pivot_root, capset, seccomp, rtnetlink, and writes cgroupfs. `make` therefore
# only produces a working binary on Linux/WSL2. The `asm` target works on ANY
# host, because clang cross-compiles the self-contained asm/demo.c to Linux
# assembly without a Linux toolchain.
# =============================================================================

CC      ?= clang
# -Wall -Wextra : warnings are errors in spirit (see CONVENTIONS.md).
# -O2           : real code, built optimized.
# _GNU_SOURCE is set per-file in the sources (needed for clone/CLONE_NEW*).
CFLAGS  ?= -Wall -Wextra -O2 -std=gnu11
LDFLAGS ?=

# The engine, split so the container "story" reads across files (see README).
SRCS = main.c child.c overlay.c cgroup.c caps.c seccomp.c network.c util.c
OBJS = $(SRCS:.c=.o)
BIN  = ceng

# Force the Linux System V AMD64 target for asm so the output is the ABI every
# project in this repo assumes, regardless of the host OS.
ASM_TARGET ?= x86_64-pc-linux-gnu
ASM_COMMON  = --target=$(ASM_TARGET) -S -fno-asynchronous-unwind-tables -fno-jump-tables

# ---- default: build the engine ---------------------------------------------
$(BIN): $(OBJS)
	$(CC) $(CFLAGS) $(OBJS) -o $@ $(LDFLAGS)
	@echo "built ./$(BIN) — full run needs root:  sudo ./$(BIN) --image ./image -- /bin/sh"

%.o: %.c engine.h util.h
	$(CC) $(CFLAGS) -c $< -o $@

# ---- rootfs: build a busybox image (overlay lowerdir) ----------------------
.PHONY: rootfs
rootfs:
	sh scripts/build-rootfs.sh busybox ./image

# ---- pull: fetch a real image from a registry (needs curl/jq/tar) ----------
# Override IMAGE=<repo>:<tag> to pull something else, e.g. make pull IMAGE=alpine:3.19
IMAGE ?= alpine:3.19
.PHONY: pull
pull:
	sh scripts/oci-pull.sh $(IMAGE) ./image

# ---- run: start a shell in a fully-featured container ----------------------
# Needs root for the overlay mount, cgroup writes, and veth/NAT. Rootless runs
# work too but the overlay needs kernel >= 5.11 and networking/cgroups are
# skipped with a warning (see README).
.PHONY: run
run: $(BIN)
	sudo ./$(BIN) --image ./image --state ./state -- /bin/sh

# ---- test: non-interactive smoke test --------------------------------------
# Prints the isolated view: hostname, PID (should be 1), its own /proc, and the
# container-side network interface (proves the veth landed in the netns).
.PHONY: test
test: $(BIN)
	@echo ">> container should report hostname=container, PID 1, its own /proc, a veth:"
	sudo ./$(BIN) --image ./image --state ./state -- /bin/sh -c \
	  'echo host=$$(hostname); echo pid=$$$$; echo "--- ip ---"; ip addr 2>/dev/null | head; echo "--- proc ---"; ls /proc | head'

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

# ---- demo: build + run the asm/demo.c logic probe (expects exit 7) ---------
.PHONY: demo
demo:
	$(CC) $(CFLAGS) asm/demo.c -o demo_probe
	@./demo_probe; echo "demo probe exit status = $$? (expect 7)"

.PHONY: clean
clean:
	rm -f $(OBJS) $(BIN) demo_probe

# Note: `make clean` does not delete ./image (expensive to rebuild), ./state
# (the container's writable layer — inspect it!), nor the committed asm/*.s
# (teaching artifacts). Remove those by hand.
