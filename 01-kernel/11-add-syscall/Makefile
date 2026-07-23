# =============================================================================
# Makefile — "Add a real system call"
# =============================================================================
#
# This project has TWO halves that are built very differently, and that split is
# itself a lesson:
#
#   1. The KERNEL half (kernel/sys_hello.c + patches/) is NOT built here. A
#      syscall is compiled into the kernel image, not as a module, so it is
#      built by the KERNEL's own Kbuild when you recompile the patched tree:
#          make -j$(nproc) bzImage            # inside the kernel source tree
#      There is deliberately no `obj-m` target here — see patches/0003 for why a
#      syscall can't be a loadable module.
#
#   2. The USERSPACE half (user/hello_user.c) is an ordinary program that calls
#      the syscall by number. That is what THIS Makefile builds, plus the
#      teaching assembly for asm/demo.c.
#
# Targets:
#   make            build the userspace caller (user/hello_user)
#   make run        build and run it (expect ENOSYS on an unpatched kernel)
#   make asm        regenerate asm/demo.{O0.s,s,O2.s} (runs on any host)
#   make clean      remove build products
# =============================================================================

CC      ?= clang
# -Wall -Wextra: warnings are errors in spirit (CONVENTIONS.md).
# No special flags are needed: this is a plain libc program using syscall(2).
CFLAGS  ?= -Wall -Wextra -O2

# For asm generation we force the Linux System V AMD64 target so the output is
# the ABI every project in this repo assumes, regardless of the host OS. clang
# cross-compiles by construction, so this works on Windows/macOS/Linux alike.
ASM_TARGET ?= x86_64-pc-linux-gnu

.PHONY: all run asm clean help

all: user/hello_user

user/hello_user: user/hello_user.c
	$(CC) $(CFLAGS) $< -o $@
	@echo "built user/hello_user — run on a PATCHED kernel (QEMU); see README."

run: user/hello_user
	./user/hello_user

# Regenerate the committed teaching assembly for asm/demo.c using the EXACT
# commands documented in the README. The hand-written asm/demo.annotated.s is
# authored, never overwritten by this target.
asm:
	$(CC) --target=$(ASM_TARGET) -S -O0 -fno-asynchronous-unwind-tables -fno-jump-tables -fno-omit-frame-pointer asm/demo.c -o asm/demo.O0.s
	$(CC) --target=$(ASM_TARGET) -S -O1 -fno-asynchronous-unwind-tables -fno-jump-tables -fno-omit-frame-pointer asm/demo.c -o asm/demo.s
	$(CC) --target=$(ASM_TARGET) -S -O2 -fno-asynchronous-unwind-tables -fno-jump-tables asm/demo.c -o asm/demo.O2.s
	@echo "wrote asm/demo.O0.s (naive), asm/demo.s (-O1 baseline), asm/demo.O2.s (-O2)"

clean:
	rm -f user/hello_user

help:
	@echo "make       - build user/hello_user (the userspace caller)"
	@echo "make run   - run it (ENOSYS unless booted on the patched kernel)"
	@echo "make asm   - regenerate teaching assembly from asm/demo.c"
	@echo "make clean - remove build products"
	@echo "Kernel side: apply patches/, drop in kernel/sys_hello.c, rebuild kernel."
