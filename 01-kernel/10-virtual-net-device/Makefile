# Makefile for the virtual network device driver (a Linux kernel module).
#
# TWO WORLDS, ONE FILE
# --------------------
#  * `make` (default) builds the kernel module by invoking the KERNEL'S build
#    system (Kbuild). It MUST run on Linux with the matching kernel headers
#    installed. It will NOT build on the host that generated the asm/ files.
#  * `make asm` regenerates the committed teaching assembly from asm/demo.c.
#    That uses a cross-targeting clang and works on ANY host, because demo.c is
#    self-contained pure C with no kernel headers.
#
# Kbuild is recursive: when you run `make` here, we re-invoke make inside the
# kernel source tree (`-C .../build`) pointing it back at this directory
# (`M=$(PWD)`). Kbuild then reads the `obj-m` line below to know what to build.

# ---- Kbuild object list (read when Kbuild includes this file) --------------
# obj-m := vnetdev.o  ->  build vnetdev.ko as a loadable module from vnetdev.c.
obj-m += vnetdev.o

# ---- Normal `make` from a shell: drive the kernel build system -------------
# KDIR points at the build tree for the RUNNING kernel. Override on the command
# line to cross-build:  make KDIR=/path/to/linux
KDIR ?= /lib/modules/$(shell uname -r)/build
PWD  := $(shell pwd)

# For asm generation we pin the Linux System V x86-64 target so the output is
# the ABI this repo assumes, regardless of the host OS.
ASM_TARGET ?= x86_64-pc-linux-gnu

.PHONY: all
all:
	$(MAKE) -C $(KDIR) M=$(PWD) modules

.PHONY: clean
clean:
	$(MAKE) -C $(KDIR) M=$(PWD) clean

# ---- Regenerate the teaching assembly (asm/demo.{s,O0.s,O2.s}) -------------
# The hand-written asm/demo.annotated.s is authored, never overwritten.
# These are the EXACT commands the lab convention prescribes.
.PHONY: asm
asm:
	clang --target=$(ASM_TARGET) -S -O0 -fno-asynchronous-unwind-tables \
		-fno-jump-tables -fno-omit-frame-pointer asm/demo.c -o asm/demo.O0.s
	clang --target=$(ASM_TARGET) -S -O1 -fno-asynchronous-unwind-tables \
		-fno-jump-tables -fno-omit-frame-pointer asm/demo.c -o asm/demo.s
	clang --target=$(ASM_TARGET) -S -O2 -fno-asynchronous-unwind-tables \
		-fno-jump-tables asm/demo.c -o asm/demo.O2.s
	@echo "regenerated asm/demo.{O0.s,s,O2.s} — demo.annotated.s is hand-authored"

# ---- Convenience: load / unload / watch (run on your Linux test box) -------
# These are here as documentation of the manual test flow; see README.md.
.PHONY: load
load: all
	sudo insmod vnetdev.ko
	sudo ip link set vnet0 up
	ip -d link show vnet0

.PHONY: unload
unload:
	-sudo ip link set vnet0 down
	sudo rmmod vnetdev
