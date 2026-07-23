# Makefile for the nolibc exemplar.
#
# Everything here is deliberately explicit so you can see exactly which flags
# strip the C runtime away. Run `make` on Linux (or WSL). On other hosts you can
# still regenerate the *teaching assembly* with `make asm`, because clang cross-
# compiles to the Linux target without needing a Linux linker.

CC      ?= clang
# -nostdlib : do not link crt1.o/crti.o/crtn.o or libc — we supply _start.
# -static   : no dynamic loader (ld.so) is involved; the binary is self-contained.
# -no-pie   : load at a fixed address so no startup relocation is needed.
# -fno-stack-protector : the canary setup would call __stack_chk_fail in libc,
#             which we don't have; turn it off.
CFLAGS  ?= -Wall -Wextra -O2 -nostdlib -static -no-pie -fno-stack-protector -ffreestanding

# For asm generation we force the Linux System V target so the output is the
# ABI every project in this repo assumes, regardless of the host OS.
ASM_TARGET ?= x86_64-pc-linux-gnu

hello: hello.c
	$(CC) $(CFLAGS) $< -o $@
	@echo "built ./hello — run it with:  ./hello ; echo exit=\$$?"

# Regenerate the committed teaching assembly (asm/hello.{s,O0.s,O2.s}).
# The hand-written asm/hello.annotated.s is NOT overwritten — it is authored.
.PHONY: asm
asm:
	../../tools/gen_asm.sh hello.c -ffreestanding -fno-stack-protector

.PHONY: run
run: hello
	./hello ; echo "exit=$$?"

.PHONY: clean
clean:
	rm -f hello
