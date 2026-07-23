# =============================================================================
# Makefile — build the kprobe/ftrace tracer kernel modules, and (host-portable)
# regenerate the teaching assembly.
# =============================================================================
#
# HOW KERNEL MODULE BUILDS WORK
# -----------------------------
# You do not compile a module with a bare `clang foo.c`. A module must be built
# by the KERNEL's own build system (Kbuild) against the exact headers and config
# of the kernel it will load into — struct layouts, config options, and the
# module ABI must match or the module is rejected (or corrupts memory). So this
# Makefile plays a double role, selected by the $(KERNELRELEASE) variable:
#
#   * When you run `make` directly, KERNELRELEASE is empty. We fall to the
#     bottom rule, which re-invokes make inside the kernel build tree with
#     `-C <kdir> M=$(PWD)`. That is the canonical out-of-tree module invocation.
#   * Kbuild then re-enters THIS file with KERNELRELEASE set, reads the `obj-m`
#     lines below, and compiles each named object into a .ko.
#
# The two lines that matter to Kbuild are the `obj-m` assignments: each
# `name.o` tells Kbuild to build `name.c` into a loadable `name.ko`.
# =============================================================================

# Two independent modules: the int3-breakpoint tracer and the fentry tracer.
obj-m += kprobe_tracer.o
obj-m += ftrace_tracer.o

# Kernel build directory: the headers/build tree for the RUNNING kernel. On a
# real machine `uname -r` names it; override KDIR to cross-build for another.
KDIR ?= /lib/modules/$(shell uname -r)/build
PWD  := $(shell pwd)

# Default target: hand off to the kernel build system to build the modules.
.PHONY: all
all:
	$(MAKE) -C $(KDIR) M=$(PWD) modules

# Load/unload helpers (need root; run inside your QEMU guest). `dmesg` shows the
# pr_info output — the arg logs, the SLOW lines, and the histogram on unload.
.PHONY: load unload
load: all
	sudo insmod kprobe_tracer.ko
	sudo insmod ftrace_tracer.ko
	@echo "loaded. trigger some opens, then 'make unload' and read dmesg."
unload:
	-sudo rmmod ftrace_tracer
	-sudo rmmod kprobe_tracer
	@dmesg | tail -n 40

.PHONY: clean
clean:
	$(MAKE) -C $(KDIR) M=$(PWD) clean
	rm -f asm/demo.s asm/demo.O0.s asm/demo.O2.s

# -----------------------------------------------------------------------------
# Teaching assembly. Kernel C cannot be compiled standalone on any host (it
# needs the kernel headers/config), so the instructive PURE-LOGIC core — the
# log2 latency bucketing from latency_hist.h — is mirrored into asm/demo.c and
# THAT is what we turn into annotated assembly. clang cross-targets Linux, so
# this rule works on Windows/macOS/Linux alike; it needs no kernel tree.
# -----------------------------------------------------------------------------
ASM_TARGET ?= x86_64-pc-linux-gnu
.PHONY: asm
asm:
	clang --target=$(ASM_TARGET) -S -O0 -fno-asynchronous-unwind-tables \
		-fno-jump-tables -fno-omit-frame-pointer asm/demo.c -o asm/demo.O0.s
	clang --target=$(ASM_TARGET) -S -O1 -fno-asynchronous-unwind-tables \
		-fno-jump-tables -fno-omit-frame-pointer asm/demo.c -o asm/demo.s
	clang --target=$(ASM_TARGET) -S -O2 -fno-asynchronous-unwind-tables \
		-fno-jump-tables asm/demo.c -o asm/demo.O2.s
	@echo "regenerated asm/demo.{O0.s,s,O2.s}; asm/demo.annotated.s is hand-authored"
