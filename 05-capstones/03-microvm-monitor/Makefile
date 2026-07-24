# =============================================================================
# Makefile — build the KVM microVM monitor (a USERSPACE program).
# =============================================================================
#
# Like ../../01-kernel/15-kvm-hypervisor and unlike the kernel MODULES in
# 01-kernel/, this is an ordinary Linux process that drives KVM through ioctls on
# /dev/kvm — so there is no Kbuild recursion, just a normal userspace compile of
# three translation units (vmm.c, virtio.c, guest.c) into one binary.
#
# LINUX ONLY to BUILD AND RUN: every .c here includes <linux/kvm.h> and the
# monitor opens /dev/kvm, neither of which exists on the Windows host this repo
# may live on. The `asm` target is the exception — clang cross-compiles
# asm/demo.c to Linux assembly on any host, so the teaching assembly regenerates
# anywhere (that is the piece verified in CI).
# =============================================================================

CC      ?= cc
# -Wall -Wextra: warnings are errors in spirit (see CONVENTIONS.md).
# -O2:  the KVM_RUN / virtqueue path is a real hot loop.
# -g:   keep debug info — single-stepping the run loop in gdb is the best way to
#       watch VM exits and virtqueue processing arrive.
CFLAGS  ?= -Wall -Wextra -O2 -g
OBJS    := vmm.o virtio.o guest.o
BIN     := microvm

.PHONY: all run asm clean

all: $(BIN)

# All three units share the headers; edit a header and everything rebuilds.
vmm.o:    vmm.c    vmm.h virtio.h boot.h
virtio.o: virtio.c vmm.h virtio.h
guest.o:  guest.c  vmm.h virtio.h boot.h

$(BIN): $(OBJS)
	$(CC) $(CFLAGS) -o $@ $(OBJS)
	@echo "built ./$(BIN) — run it on Linux with KVM:  ./$(BIN)"

# Running needs read/write access to /dev/kvm (membership in the 'kvm' group, or
# root). No sudo is needed if you are in the kvm group.
run: $(BIN)
	./$(BIN)

# Regenerate the committed teaching assembly from the extracted pure-logic core
# (asm/demo.c). Works on ANY host; clang cross-targets the Linux SysV ABI. The
# hand-written asm/demo.annotated.s is authored, never overwritten by this.
asm:
	../../tools/gen_asm.sh asm/demo.c

clean:
	rm -f $(BIN) $(OBJS)
