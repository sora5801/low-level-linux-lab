# =============================================================================
# Makefile — builds the ramblk.ko kernel module, and (host-portably) the
# teaching assembly for the extracted arithmetic core.
# =============================================================================
#
# HOW KERNEL MODULE BUILDS WORK
# -----------------------------
# You cannot compile a kernel module with a bare `clang ramblk.c`. A module must
# be built by the *kernel's own build system* (Kbuild) against the exact headers
# and config of the running kernel, so that struct layouts and CONFIG_* options
# match. The idiom is to hand the work to the kernel tree:
#
#     make -C <kernel build dir> M=$(PWD) modules
#
# `-C` cd's into the kernel build tree (its Makefiles do the real work); `M=`
# points back here so it compiles OUR file. Kbuild re-invokes THIS Makefile with
# KERNELRELEASE set, and on that second pass only the `obj-m` line below matters.
#
# The two-pass trick: the same file is both the thing `make` runs first (with
# KERNELRELEASE empty -> the `else` branch) and the fragment Kbuild reads second
# (KERNELRELEASE set -> the `obj-m` branch). This is the standard out-of-tree
# module Makefile.

ifneq ($(KERNELRELEASE),)
# ---- PASS 2: invoked by Kbuild. Declare the module and its object. ----------
# obj-m = "build this as a loadable module (.ko)". ramblk.o is compiled from
# ramblk.c automatically; a multi-file module would list ramblk-objs instead.
obj-m := ramblk.o

else
# ---- PASS 1: invoked directly by the user on the command line. ---------------
# KDIR points at the build tree for the *running* kernel. `uname -r` is the
# release string; distros symlink the matching headers under /lib/modules/.
KDIR ?= /lib/modules/$(shell uname -r)/build
PWD  := $(shell pwd)

# Cross-target used ONLY for the teaching assembly, so its ABI is the Linux
# System V AMD64 one every project in this lab assumes, regardless of host OS.
ASM_TARGET ?= x86_64-pc-linux-gnu
CLANG      ?= clang

.PHONY: all modules clean asm help

all: modules

# Build ramblk.ko by delegating to the kernel build system.
modules:
	$(MAKE) -C $(KDIR) M=$(PWD) modules

# Remove build products (kernel tree provides the `clean` target too).
clean:
	$(MAKE) -C $(KDIR) M=$(PWD) clean
	rm -f asm/demo.O0.s asm/demo.s asm/demo.O2.s

# ---- Teaching assembly ------------------------------------------------------
# Regenerate the committed .s files from the self-contained arithmetic core in
# asm/demo.c. This works on ANY host (Windows/macOS/Linux) because clang cross-
# compiles to the Linux target without a Linux toolchain. The hand-written
# asm/demo.annotated.s is authored, NOT generated, and is never overwritten.
asm:
	$(CLANG) --target=$(ASM_TARGET) -S -O0 -fno-asynchronous-unwind-tables \
		-fno-jump-tables -fno-omit-frame-pointer asm/demo.c -o asm/demo.O0.s
	$(CLANG) --target=$(ASM_TARGET) -S -O1 -fno-asynchronous-unwind-tables \
		-fno-jump-tables -fno-omit-frame-pointer asm/demo.c -o asm/demo.s
	$(CLANG) --target=$(ASM_TARGET) -S -O2 -fno-asynchronous-unwind-tables \
		-fno-jump-tables asm/demo.c -o asm/demo.O2.s
	@echo "regenerated asm/demo.{O0.s,s,O2.s} — demo.annotated.s is hand-written"

help:
	@echo "make modules   build ramblk.ko (needs Linux kernel headers)"
	@echo "make asm       regenerate teaching assembly (any host)"
	@echo "make clean     remove build products"

endif
