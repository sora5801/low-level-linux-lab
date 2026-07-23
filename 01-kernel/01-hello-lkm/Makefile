# =============================================================================
# Makefile — build the hello_lkm kernel module (Kbuild), and the teaching asm.
# =============================================================================
#
# THIS RUNS ON LINUX ONLY. A kernel module must be compiled against the exact
# headers of the kernel it will load into, so there is no cross-build here (the
# asm target below is the only host-portable part). Use a Linux box, WSL2 with
# kernel headers, or — best for a module you might crash — a throwaway QEMU VM.
#
# HOW KERNEL MODULES BUILD: `obj-m`.
# ---------------------------------
# You do NOT invoke clang/gcc directly on hello_lkm.c. Instead you ask the
# KERNEL's own build system (Kbuild) to build your object *for* you, so it uses
# the kernel's flags, include paths, and module post-processing (symbol
# versioning, .modinfo, the .ko wrapper). The single line
#
#     obj-m += hello_lkm.o
#
# tells Kbuild "produce a loadable module from hello_lkm.o". Kbuild finds
# hello_lkm.c automatically, compiles it, then links hello_lkm.ko.
#
# This Makefile is "recursive": when you type `make`, the default target
# re-invokes make inside the kernel build tree (-C) but pointing the module
# search path back here (M=$(PWD)). That two-directory dance is the standard
# out-of-tree module idiom.
# =============================================================================

# The single most important line: register our object as a module ("-m").
# If we wanted it built INTO the kernel image instead, this would be obj-y.
obj-m += hello_lkm.o

# KDIR = where the running kernel's build tree / headers live. `uname -r` is the
# running kernel's version string, so this resolves to something like
# /lib/modules/6.8.0-generic/build (a symlink into the headers package). Install
# it with `apt install linux-headers-$(uname -r)` (Debian/Ubuntu) or the distro
# equivalent before building.
KDIR ?= /lib/modules/$(shell uname -r)/build

# PWD = this project directory; passed to Kbuild as M= so it knows where the
# out-of-tree module source lives.
PWD  := $(shell pwd)

# ---- Default target: build the .ko --------------------------------------
# -C $(KDIR)      : chdir into the kernel build tree and use ITS Makefiles.
# M=$(PWD)        : but build the module found in THIS directory.
# modules         : the Kbuild goal that produces hello_lkm.ko.
.PHONY: all
all:
	$(MAKE) -C $(KDIR) M=$(PWD) modules

# ---- Clean: remove all Kbuild intermediates (.o .ko .mod.c .cmd ...) --------
.PHONY: clean
clean:
	$(MAKE) -C $(KDIR) M=$(PWD) clean

# ---- Convenience: load, exercise, and unload in one shot --------------------
# Requires root (loading code into the kernel is privileged). `insmod` inserts
# THIS .ko exactly; `modprobe` would additionally resolve dependencies from the
# module database (not needed for a standalone module like this one).
.PHONY: load
load: all
	sudo insmod hello_lkm.ko who="lab" count=3
	@echo "--- dmesg tail ---"; sudo dmesg | tail -n 8

.PHONY: unload
unload:
	sudo rmmod hello_lkm
	@echo "--- dmesg tail ---"; sudo dmesg | tail -n 4

# ---- Inspect the module's metadata without loading it -----------------------
.PHONY: info
info: all
	modinfo hello_lkm.ko

# =============================================================================
# TEACHING ASSEMBLY (host-portable; needs only clang, no Linux kernel tree).
# -----------------------------------------------------------------------------
# hello_lkm.c cannot be compiled to .s on its own (it needs kernel headers), so
# we compile asm/demo.c — the extracted pure-logic core — with a Linux-targeting
# clang. These are the EXACT commands the lab convention prescribes; the hand-
# written asm/demo.annotated.s is authored, never overwritten here.
# =============================================================================
ASM_TARGET ?= x86_64-pc-linux-gnu
ASM_COMMON  = --target=$(ASM_TARGET) -S -fno-asynchronous-unwind-tables -fno-jump-tables

.PHONY: asm
asm:
	clang $(ASM_COMMON) -O0 -fno-omit-frame-pointer asm/demo.c -o asm/demo.O0.s
	clang $(ASM_COMMON) -O1 -fno-omit-frame-pointer asm/demo.c -o asm/demo.s
	clang $(ASM_COMMON) -O2                         asm/demo.c -o asm/demo.O2.s
	@echo "wrote asm/demo.O0.s, asm/demo.s (-O1 baseline), asm/demo.O2.s"
