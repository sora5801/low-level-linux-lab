/* ===========================================================================
 * entry.S — the kernel image's first byte: set up a stack, then call C.
 * ===========================================================================
 *
 * The boot sector loads this whole kernel image to physical 0x10000 and, after
 * switching the CPU into 64-bit long mode, does `jmp 0x10000`. So the VERY FIRST
 * instruction the kernel runs is `_start` below — which means `_start` MUST be
 * at offset 0 of the linked image. The linker script (linker.ld) guarantees
 * that by placing this file's `.text.boot` section first.
 *
 * Why a stack setup is the first thing we do: C cannot run without one. The
 * boot code left RSP pointing into low memory that we are about to reuse for
 * page tables and the BIOS data area; before the first `call` (which pushes a
 * return address) we must point RSP at memory WE own. We reserve that stack in
 * .bss below and load its top here.
 * =========================================================================== */
	.section .text.boot,"ax",@progbits
	.code64
	.global _start
_start:
	/* RSP must point at the TOP of our stack because the stack grows DOWNWARD;
	 * the first push will predecrement it into our reserved region. We use a
	 * RIP-relative LEA so this works regardless of where the image is loaded
	 * (here load address == link address, but RIP-relative is the correct habit). */
	lea   stack_top(%rip), %rsp

	/* Zero RBP so a stack unwinder / debugger sees a clean "bottom of stack"
	 * sentinel rather than a garbage frame-pointer chain. */
	xor   %rbp, %rbp

	/* ---- Zero the .bss ------------------------------------------------------
	 * C guarantees that objects with no initializer (or an all-zero one) start
	 * at zero. The compiler puts them in .bss, which occupies NO bytes in our
	 * on-disk image — so nothing zeroed them yet. We rely on that guarantee for
	 * g_ticks, the VGA cursor, and more, so clear [__bss_start, __bss_end) here,
	 * BEFORE any C runs. The linker script exports the two boundary symbols. */
	cld                            /* stosb must count UP: direction flag = 0       */
	lea   __bss_start(%rip), %rdi  /* RDI = first byte of .bss                       */
	lea   __bss_end(%rip), %rcx    /* RCX = one past the last byte                   */
	sub   %rdi, %rcx               /* RCX = byte count to clear                      */
	xor   %eax, %eax               /* AL = 0, the fill byte                          */
	rep   stosb                    /* while (rcx--) *rdi++ = 0                       */

	/* Into C. kmain() never returns, but if it ever did we fall through to the
	 * halt loop below rather than executing whatever bytes follow in memory. */
	call  kmain

	/* Safety net: park the CPU forever. `cli` so no interrupt can wake us into
	 * undefined territory; `hlt` sleeps the core; the `jmp` re-arms `hlt` in the
	 * (impossible on x86) event it returns spuriously. */
.hang:
	cli
	hlt
	jmp   .hang

/* ---------------------------------------------------------------------------
 * The kernel stack. 16 KiB is ample for this non-recursive, interrupt-driven
 * kernel. Living in .bss means it occupies NO space in the on-disk image (the
 * loader just reserves zeroed RAM), and 16-byte alignment satisfies the SysV
 * ABI requirement that RSP be 16-aligned at a `call` boundary.
 * ------------------------------------------------------------------------- */
	.section .bss
	.align 16
stack_bottom:
	.skip 16384          /* 16 KiB of stack                                     */
stack_top:

	.section .note.GNU-stack,"",@progbits
