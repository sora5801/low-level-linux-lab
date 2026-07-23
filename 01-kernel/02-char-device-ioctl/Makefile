# =============================================================================
# Makefile — Kbuild-style build for the foo char device.
# =============================================================================
#
# This is the standard "recursive" kernel-module Makefile. It is read TWICE:
#
#   1. When you run `make` at the shell, KERNELRELEASE is empty, so the `else`
#      branch runs. It re-invokes make INSIDE the kernel's own build system:
#          make -C <kernel build tree> M=$(PWD) modules
#      -C changes into the kernel source/headers tree (which has all the kbuild
#      machinery); M=$(PWD) tells it "the module to build lives back over here".
#
#   2. The kernel build system then re-reads THIS file with KERNELRELEASE set,
#      so the `ifneq` branch runs and hands kbuild the one line it actually
#      needs: `obj-m := foo.o` = "build foo.ko from foo.c as a loadable module".
#
# Everything here is Linux-only: a kernel module can only be compiled against a
# configured kernel build tree, which exists on Linux/WSL/QEMU, not on Windows.
# The teaching ASSEMBLY (make asm) is host-portable — see the asm/ target.
# =============================================================================

ifneq ($(KERNELRELEASE),)
# ---- Pass 2: invoked by kbuild. Just declare the module object. -------------
	obj-m := foo.o

else
# ---- Pass 1: invoked from the shell. Drive kbuild, build userspace + asm. ----

# The build tree for the RUNNING kernel. Override on the command line to build
# against a different tree, e.g.  make KDIR=~/linux-6.9  (needed in a QEMU VM).
KDIR ?= /lib/modules/$(shell uname -r)/build
PWD  := $(shell pwd)

# Host compiler for the userspace test program (NOT the kernel toolchain).
CC       ?= cc
CFLAGS   ?= -Wall -Wextra -O2

.PHONY: all modules test asm clean

# Default: build the kernel module AND the userspace tester.
all: modules test

# Recurse into the kernel build system to produce foo.ko.
modules:
	$(MAKE) -C $(KDIR) M=$(PWD) modules

# The userspace exerciser. It includes the SAME foo.h the module does, so the
# ioctl numbers are guaranteed identical.
test: test_foo
test_foo: test_foo.c foo.h
	$(CC) $(CFLAGS) -o $@ test_foo.c

# Regenerate the committed teaching assembly from the extracted pure-logic core.
# Works on ANY host: clang cross-targets the Linux SysV ABI (see the script).
asm:
	../../tools/gen_asm.sh asm/demo.c

# Clean kernel build artifacts (via kbuild) plus our userspace binary.
clean:
	$(MAKE) -C $(KDIR) M=$(PWD) clean
	rm -f test_foo

endif
