# Makefile for the PathGuard LSM (teaching core).
#
# This is a Kbuild-style module Makefile. Kbuild is the kernel's own build
# system; when invoked as below it re-enters the running kernel's build tree so
# our file is compiled with the EXACT flags, headers, and config of that kernel.
# That is the only way to get correct `struct file` / hook-signature layouts.
#
# IMPORTANT (read the README): the resulting pathguard.ko will COMPILE but will
# NOT insmod, because a real LSM registers via the __init-only, unexported
# security_add_hooks(). `make` here is a genuine *type-check* against your
# kernel's headers; to actually RUN PathGuard you build it INTO the kernel (see
# README "Build & run"). Do that inside a QEMU VM.

# The object to build. Kbuild reads `obj-m` ("m" = as a module) and compiles the
# matching .c (pathguard.c) into pathguard.ko.
obj-m := pathguard.o

# KDIR points at the build tree of the *currently running* kernel. `uname -r` is
# e.g. 6.8.0-genericg; the build headers live under /lib/modules/<ver>/build.
KDIR ?= /lib/modules/$(shell uname -r)/build
PWD  := $(shell pwd)

# For the standalone teaching assembly we force the Linux SysV target so the
# output matches this repo's ABI regardless of the host OS (clang cross-targets).
ASM_TARGET ?= x86_64-pc-linux-gnu
ASM_FLAGS  := --target=$(ASM_TARGET) -fno-asynchronous-unwind-tables \
              -fno-jump-tables -fno-omit-frame-pointer

# Default target: compile-check the module against the running kernel's headers.
.PHONY: all
all:
	$(MAKE) -C $(KDIR) M=$(PWD) modules

# Regenerate the committed teaching assembly (asm/demo.{s,O0.s,O2.s}).
# The hand-written asm/demo.annotated.s is authored, never overwritten.
.PHONY: asm
asm:
	clang $(ASM_FLAGS) -S -O0 asm/demo.c -o asm/demo.O0.s
	clang $(ASM_FLAGS) -S -O1 asm/demo.c -o asm/demo.s
	clang --target=$(ASM_TARGET) -fno-asynchronous-unwind-tables \
	      -fno-jump-tables -S -O2 asm/demo.c -o asm/demo.O2.s
	@echo "regenerated asm/demo.{O0.s,s,O2.s}"

.PHONY: clean
clean:
	$(MAKE) -C $(KDIR) M=$(PWD) clean
