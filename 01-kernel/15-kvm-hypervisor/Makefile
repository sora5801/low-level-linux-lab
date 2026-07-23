# =============================================================================
# Makefile — build the /dev/kvm Type-2 hypervisor (a USERSPACE program).
# =============================================================================
#
# Unlike most projects in 01-kernel/, this is NOT a kernel module: a Type-2
# hypervisor is an ordinary Linux process that drives KVM through ioctls on
# /dev/kvm. So there is no Kbuild recursion here — just a normal userspace
# compile of two translation units into one binary. (For contrast, the Kbuild
# `obj-m` pattern lives in the neighboring module projects, e.g.
# 02-char-device-ioctl/Makefile.)
#
# LINUX ONLY to BUILD AND RUN: vmm.c/guest.c include <linux/kvm.h> and open
# /dev/kvm, neither of which exists on the Windows host this repo may live on.
# The `asm` target is the exception — clang cross-compiles asm/demo.c to Linux
# assembly on any host, so the teaching assembly regenerates anywhere.
# =============================================================================

CC      ?= cc
# -Wall -Wextra: warnings are errors in spirit (see CONVENTIONS.md).
# -O2:  optimize; the run loop is a real hot path.
# -g:   keep debug info — single-stepping the KVM_RUN loop in gdb is the best
#       way to watch VM exits arrive.
CFLAGS  ?= -Wall -Wextra -O2 -g
OBJS    := vmm.o guest.o

.PHONY: all run run-real run-long asm clean

# Default: build the hypervisor binary from both source files.
all: vmm

# vmm depends on the shared header, so editing vmm.h triggers a rebuild.
vmm.o: vmm.c vmm.h
guest.o: guest.c vmm.h

vmm: $(OBJS)
	$(CC) $(CFLAGS) -o $@ $(OBJS)
	@echo "built ./vmm — run it on Linux with KVM:  ./vmm both"

# Convenience run targets. Running needs read/write access to /dev/kvm, i.e.
# membership in the 'kvm' group (or root). No sudo is needed if you are in kvm.
run: vmm
	./vmm both
run-real: vmm
	./vmm real
run-long: vmm
	./vmm long

# Regenerate the committed teaching assembly from the extracted pure-logic core
# (asm/demo.c). Works on ANY host; clang cross-targets the Linux SysV ABI. The
# hand-written asm/demo.annotated.s is authored, never overwritten by this.
asm:
	../../tools/gen_asm.sh asm/demo.c

clean:
	rm -f vmm $(OBJS)
