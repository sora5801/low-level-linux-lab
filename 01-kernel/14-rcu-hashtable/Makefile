# =============================================================================
# Makefile — RCU-protected hash table kernel module.
# =============================================================================
#
# This is a KERNEL MODULE, so the build is driven by the kernel's own Kbuild
# system, not by us compiling directly. The line
#
#     obj-m := rcu_hashtable.o
#
# tells Kbuild "build rcu_hashtable.ko as an out-of-tree module." We then invoke
# the kernel build system with -C pointing at the configured kernel tree and
# M=$(PWD) pointing back here, so it compiles our file against the EXACT headers
# and config of the running kernel. Mismatched headers = a module that won't
# load (vermagic/CRC mismatch), which is why we always use the running kernel's
# build tree via `uname -r`.
#
# BUILD/RUN THIS ONLY ON LINUX, INSIDE A THROWAWAY VM (QEMU). A bug in an RCU
# reclaim path is a kernel oops, not a userspace segfault. See README.md.
# =============================================================================

# When invoked by Kbuild (KERNELRELEASE is set in that recursive pass), just
# declare the module object. Kbuild reads this branch during its own build.
ifneq ($(KERNELRELEASE),)

obj-m := rcu_hashtable.o

# -------------------------------------------------------------------------
# Normal `make` from a shell lands here (KERNELRELEASE is empty). We bounce
# into the kernel build system.
# -------------------------------------------------------------------------
else

# Path to the build tree for the currently running kernel. On Debian/Ubuntu
# this comes from the linux-headers-$(uname -r) package; on Fedora, kernel-devel.
KDIR ?= /lib/modules/$(shell uname -r)/build
PWD  := $(shell pwd)

# ASM teaching target uses a Linux-targeting clang so the output is the System V
# AMD64 ABI every project in this repo assumes, regardless of the build host.
ASM_TARGET ?= x86_64-pc-linux-gnu
ASM_FLAGS  := --target=$(ASM_TARGET) -S -fno-asynchronous-unwind-tables -fno-jump-tables

.PHONY: all
all:
	$(MAKE) -C $(KDIR) M=$(PWD) modules
	@echo "built rcu_hashtable.ko — load in a VM with:  sudo insmod rcu_hashtable.ko"

# Load, watch the stress test run, then unload — all with dmesg evidence.
# Run this INSIDE A VM. `modinfo` first shows the module metadata.
.PHONY: load
load: all
	sudo insmod rcu_hashtable.ko
	@echo "loaded. Watch it with:  sudo dmesg -w   (reader/writer kthreads are racing)"

.PHONY: unload
unload:
	sudo rmmod rcu_hashtable
	@echo "unloaded. Check dmesg for the exit stats line."

# -------------------------------------------------------------------------
# Regenerate the committed teaching assembly from the extracted pure-logic core
# asm/demo.c. These are the EXACT commands the repo's convention prescribes; the
# hand-written asm/demo.annotated.s is authored and NOT overwritten here.
# Works on any host — clang cross-targets Linux without a Linux toolchain.
# -------------------------------------------------------------------------
.PHONY: asm
asm:
	clang $(ASM_FLAGS) -O0 -fno-omit-frame-pointer asm/demo.c -o asm/demo.O0.s
	clang $(ASM_FLAGS) -O1 -fno-omit-frame-pointer asm/demo.c -o asm/demo.s
	clang $(ASM_FLAGS) -O2 asm/demo.c -o asm/demo.O2.s
	@echo "regenerated asm/demo.{O0.s,s,O2.s}; asm/demo.annotated.s is hand-authored"

.PHONY: clean
clean:
	-$(MAKE) -C $(KDIR) M=$(PWD) clean
	rm -f *.o *.ko *.mod *.mod.c *.mod.o modules.order Module.symvers .*.cmd
	rm -rf .tmp_versions

endif
