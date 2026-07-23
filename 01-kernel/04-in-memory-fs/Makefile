# =============================================================================
# Makefile — build the tinyfs kernel module, and regenerate its teaching asm.
# =============================================================================
#
# THIS IS A KBUILD MAKEFILE. A kernel module is NOT built by compiling the .c
# directly; it is built by re-entering the *kernel's own* build system so the
# module is compiled with the EXACT flags, config, and headers of the running
# kernel (an object built with mismatched config will refuse to load, or crash).
# The one line that matters:
#
#     make -C $(KDIR) M=$(PWD) modules
#          |         |        |
#          |         |        +-- target: build the module(s) named in obj-m
#          |         +----------- M=... : "external module" — sources live HERE,
#          |                       not in the kernel tree; write outputs here too
#          +--------------------- -C $(KDIR): cd into the kernel build tree first
#
# `obj-m += tinyfs.o` tells Kbuild to build tinyfs.ko from tinyfs.c.
#
# LINUX ONLY. This will not build on the Windows host the repo ships from — you
# need a Linux box (or WSL2, or a VM) with the matching kernel headers. Test the
# module in a THROWAWAY VM (see README): a buggy filesystem module can wedge the
# whole machine. On any host you can still regenerate the didactic assembly with
# `make asm`, because clang cross-compiles the standalone asm/demo.c to Linux.
# =============================================================================

# The single object Kbuild should turn into tinyfs.ko.
obj-m += tinyfs.o

# KDIR points at the build tree for the *running* kernel. Override on the command
# line to cross-build against another tree:  make KDIR=~/linux-6.6
KDIR ?= /lib/modules/$(shell uname -r)/build
PWD  := $(shell pwd)

# ---- default target: build the module -------------------------------------
.PHONY: all
all:
	$(MAKE) -C $(KDIR) M=$(PWD) modules

# ---- clean: remove build artifacts via Kbuild -----------------------------
.PHONY: clean
clean:
	$(MAKE) -C $(KDIR) M=$(PWD) modules clean

# ---- regenerate the committed teaching assembly ---------------------------
# The kernel C in tinyfs.c cannot be compiled standalone (it needs kernel
# headers), so the assembly deliverable lives in asm/demo.c — the extracted,
# self-contained pure-logic core (the dcache name hash + the inode-number
# allocator). These are the EXACT commands the repo mandates; each forces the
# Linux System V AMD64 target so the output is host-independent.
# The hand-written asm/demo.annotated.s is authored, never overwritten here.
ASM_TARGET ?= x86_64-pc-linux-gnu
.PHONY: asm
asm:
	clang --target=$(ASM_TARGET) -S -O0 -fno-asynchronous-unwind-tables -fno-jump-tables -fno-omit-frame-pointer asm/demo.c -o asm/demo.O0.s
	clang --target=$(ASM_TARGET) -S -O1 -fno-asynchronous-unwind-tables -fno-jump-tables -fno-omit-frame-pointer asm/demo.c -o asm/demo.s
	clang --target=$(ASM_TARGET) -S -O2 -fno-asynchronous-unwind-tables -fno-jump-tables asm/demo.c -o asm/demo.O2.s
	@echo "regenerated asm/demo.{O0.s,s,O2.s} — asm/demo.annotated.s is hand-authored"

# ---- convenience: the full load/test cycle, for reference -----------------
# Run these BY HAND inside your VM, not from this Makefile — they need root and
# a live kernel. Listed here so the recipe is in one place.
.PHONY: help
help:
	@echo "make            build tinyfs.ko against $(KDIR)"
	@echo "make clean      remove build artifacts"
	@echo "make asm        regenerate asm/demo.{O0.s,s,O2.s} (any host with clang)"
	@echo ""
	@echo "In a throwaway Linux VM, as root:"
	@echo "  insmod ./tinyfs.ko"
	@echo "  mkdir -p /mnt/tiny && mount -t tinyfs none /mnt/tiny"
	@echo "  echo hello > /mnt/tiny/f && cat /mnt/tiny/f && ls -l /mnt/tiny"
	@echo "  umount /mnt/tiny && rmmod tinyfs"
