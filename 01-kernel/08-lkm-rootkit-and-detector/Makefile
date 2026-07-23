# =============================================================================
# Makefile — builds BOTH kernel modules (rootkit.ko + detector.ko) and can
#            regenerate the teaching assembly for the extracted core.
# =============================================================================
#
# This is the standard DUAL-MODE kernel-module Makefile. It is read twice:
#
#   1. When you run `make` yourself, KERNELRELEASE is empty, so the `else`
#      branch runs. It re-invokes make INSIDE the kernel build tree
#      (/lib/modules/$(uname -r)/build) with M=$(PWD), which is how out-of-tree
#      modules are built: the kernel's own Kbuild drives the compile so the
#      module is built with the EXACT flags/headers of the running kernel (this
#      is why a .ko is tied to one kernel version — struct layouts must match).
#
#   2. Kbuild then re-reads this file with KERNELRELEASE set (the `ifneq` is
#      true), and sees only the `obj-m` lines telling it which objects to build.
#
# You MUST build against real kernel headers on Linux, inside a throwaway VM.
# There is no way to build or load these on this Windows host; see README.md.
# =============================================================================

ifneq ($(KERNELRELEASE),)
# ---- Kbuild view: just declare the modules --------------------------------
# Each names a single .c translation unit -> .o -> .ko. rootkit.c #includes the
# header-only ftrace_helper.h, so there is no separate object for it.
	obj-m += rootkit.o
	obj-m += detector.o

else
# ---- Normal `make` view: drive the kernel build tree ----------------------
# KDIR points at the build tree for the CURRENTLY RUNNING kernel. Override on
# the command line to cross-build:  make KDIR=/path/to/other/kernel/build
KDIR ?= /lib/modules/$(shell uname -r)/build
PWD  := $(shell pwd)

# clang flags used ONLY for the standalone teaching-asm (never for the modules).
ASM_TARGET ?= x86_64-pc-linux-gnu

.PHONY: all
all:
	$(MAKE) -C $(KDIR) M=$(PWD) modules
	@echo
	@echo "Built rootkit.ko and detector.ko."
	@echo "Load the DETECTOR first on a clean throwaway VM:"
	@echo "    sudo insmod detector.ko && cat /proc/rkdetect"
	@echo "Then, to see it catch a hook:"
	@echo "    sudo insmod rootkit.ko hide_prefix=rk_ && cat /proc/rkdetect"

.PHONY: clean
clean:
	$(MAKE) -C $(KDIR) M=$(PWD) clean

# Regenerate asm/demo.{O0.s,s,O2.s} from asm/demo.c with the EXACT commands the
# lab specifies. The hand-annotated asm/demo.annotated.s is authored, not
# generated, so it is never overwritten here. Works on ANY host (clang cross-
# targets Linux); no kernel tree needed.
.PHONY: asm
asm:
	clang --target=$(ASM_TARGET) -S -O0 -fno-asynchronous-unwind-tables -fno-jump-tables -fno-omit-frame-pointer asm/demo.c -o asm/demo.O0.s
	clang --target=$(ASM_TARGET) -S -O1 -fno-asynchronous-unwind-tables -fno-jump-tables -fno-omit-frame-pointer asm/demo.c -o asm/demo.s
	clang --target=$(ASM_TARGET) -S -O2 -fno-asynchronous-unwind-tables -fno-jump-tables asm/demo.c -o asm/demo.O2.s
	@echo "regenerated asm/demo.{O0.s,s,O2.s}; asm/demo.annotated.s is hand-written"

endif
