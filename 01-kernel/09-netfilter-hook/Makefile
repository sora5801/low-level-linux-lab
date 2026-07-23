# =============================================================================
# Makefile — Kbuild out-of-tree build for the netfilter hook module.
# =============================================================================
#
# WHY IT LOOKS LIKE THIS
# ----------------------
# A kernel module is NOT built by calling clang/gcc directly: it must be built
# by the kernel's own build system (Kbuild), against the headers of the exact
# kernel you will load it into, so that struct layouts and CONFIG_* options
# match. The idiom below is the standard "out-of-tree module" recipe:
#
#   obj-m := <module>.o     tells Kbuild to build <module>.ko from <module>.c
#   make -C <kernel build tree> M=$(PWD) modules
#                           re-enters the kernel build system (-C) but compiles
#                           the module sources found in THIS directory (M=$(PWD)).
#
# $(shell uname -r) selects the running kernel's build tree, typically the
# symlink /lib/modules/<version>/build -> the matching kernel-headers package.
#
# THIS ONLY BUILDS ON LINUX. On any other host `make` here will fail because
# there is no kernel build tree — that is expected. The `asm` target below DOES
# run anywhere clang is installed (it cross-targets Linux), so you can still
# regenerate the teaching assembly from the standalone extract on this host.
# =============================================================================

# The name of the module object. Kbuild turns netfilter_hook.o -> netfilter_hook.ko.
obj-m := netfilter_hook.o

# Path to the build tree for the currently running kernel. Override on the make
# command line for cross/VM builds, e.g.  make KDIR=~/linux-6.9  .
KDIR ?= /lib/modules/$(shell uname -r)/build

# PWD is this project directory; Kbuild compiles the M= sources here.
PWD := $(shell pwd)

# --- Default target: build the .ko ------------------------------------------
# `modules` is Kbuild's target for out-of-tree modules.
.PHONY: all
all:
	$(MAKE) -C $(KDIR) M=$(PWD) modules

# --- Clean: remove all Kbuild build products (.ko/.o/.mod*/Module.symvers/…) -
.PHONY: clean
clean:
	$(MAKE) -C $(KDIR) M=$(PWD) clean

# --- Convenience: load/unload/inspect (need root; run inside a throwaway VM) --
# insmod loads the .ko; the pr_info() lines land in the kernel ring buffer,
# which `dmesg` prints. rmmod triggers our __exit and prints the final stats.
.PHONY: load
load: all
	sudo insmod netfilter_hook.ko

.PHONY: unload
unload:
	sudo rmmod netfilter_hook

.PHONY: log
log:
	dmesg | tail -n 30

# =============================================================================
# asm — regenerate the committed teaching assembly from the standalone extract.
#
# These are the EXACT commands the repo mandates. They compile asm/demo.c (which
# includes no kernel headers) to Linux SysV assembly at three optimization
# levels. The hand-written asm/demo.annotated.s is authored, never overwritten.
# Runs on ANY host with clang — clang is a cross-compiler by construction.
# =============================================================================
ASM_TARGET ?= x86_64-pc-linux-gnu
.PHONY: asm
asm:
	clang --target=$(ASM_TARGET) -S -O0 -fno-asynchronous-unwind-tables \
	      -fno-jump-tables -fno-omit-frame-pointer asm/demo.c -o asm/demo.O0.s
	clang --target=$(ASM_TARGET) -S -O1 -fno-asynchronous-unwind-tables \
	      -fno-jump-tables -fno-omit-frame-pointer asm/demo.c -o asm/demo.s
	clang --target=$(ASM_TARGET) -S -O2 -fno-asynchronous-unwind-tables \
	      -fno-jump-tables asm/demo.c -o asm/demo.O2.s
	@echo "regenerated asm/demo.{O0.s,s,O2.s} — demo.annotated.s is hand-written"
