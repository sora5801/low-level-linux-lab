/* ===========================================================================
 * isr.S — the interrupt stubs: the thin asm shim between the CPU and C.
 * ===========================================================================
 *
 * WHAT THE CPU DOES FOR US, AND WHAT IT DOES NOT
 * ----------------------------------------------
 * When an interrupt or exception fires, the CPU looks up our IDT gate, switches
 * to the handler, and pushes a small frame: SS, RSP, RFLAGS, CS, RIP (and, for
 * a FEW exceptions, an error code). That is ALL it does — it does not save the
 * general-purpose registers. If our C handler clobbered them, we could never
 * return to the interrupted code. So each vector needs a tiny assembly stub
 * that (1) makes the on-stack frame UNIFORM across every vector, (2) saves all
 * 15 GP registers, (3) calls the C dispatcher with a pointer to that frame, and
 * (4) restores everything and `iretq`s back.
 *
 * TWO KINDS OF EXCEPTION
 * ----------------------
 * A handful of exceptions (8,10,11,12,13,14,17,21) push a hardware ERROR CODE;
 * the rest, and all hardware IRQs, do not. To give the C side ONE struct shape,
 * the no-error stubs push a dummy 0 in the error-code slot. Then every stub
 * pushes its own vector number. After that the stack from low to high address
 * is EXACTLY `struct regs` (see idt.h) — the ordering here and there must agree.
 * =========================================================================== */
	.text
	.code64

/* ---- macros that stamp out one stub per vector ---------------------------- */

/* No-error-code exception / software vector: push a fake 0, then the number. */
.macro ISR_NOERR num
isr\num:
	pushq $0                 /* dummy error code -> keeps the frame uniform     */
	pushq $\num              /* the vector number (becomes regs.int_no)         */
	jmp   isr_common
.endm

/* Error-code exception: the CPU ALREADY pushed a real error code, so we push
 * ONLY the vector number. Pushing a dummy here would misalign the whole frame. */
.macro ISR_ERR num
isr\num:
	pushq $\num              /* vector number; CPU's error code already below it */
	jmp   isr_common
.endm

/* Hardware IRQ: never carries an error code, so it looks like a no-error stub.
 * `num` is the IRQ line (0..15); `vec` is the remapped CPU vector (32..47). */
.macro IRQ num, vec
irq\num:
	pushq $0                 /* no error code from an IRQ -> dummy               */
	pushq $\vec              /* the remapped vector (regs.int_no)               */
	jmp   isr_common
.endm

/* ---- the 32 CPU exception vectors (0..31) -------------------------------- */
	ISR_NOERR 0     /* #DE  divide error                                       */
	ISR_NOERR 1     /* #DB  debug                                              */
	ISR_NOERR 2     /* NMI  non-maskable interrupt                             */
	ISR_NOERR 3     /* #BP  breakpoint (int3)                                  */
	ISR_NOERR 4     /* #OF  overflow                                           */
	ISR_NOERR 5     /* #BR  bound range exceeded                               */
	ISR_NOERR 6     /* #UD  invalid opcode                                     */
	ISR_NOERR 7     /* #NM  device not available (no FPU)                      */
	ISR_ERR   8     /* #DF  double fault           (error code = 0, always)    */
	ISR_NOERR 9     /* (reserved: coprocessor segment overrun)                 */
	ISR_ERR   10    /* #TS  invalid TSS            (error code)                */
	ISR_ERR   11    /* #NP  segment not present    (error code)                */
	ISR_ERR   12    /* #SS  stack-segment fault    (error code)                */
	ISR_ERR   13    /* #GP  general protection     (error code)                */
	ISR_ERR   14    /* #PF  page fault             (error code; CR2 = addr)    */
	ISR_NOERR 15    /* (reserved)                                              */
	ISR_NOERR 16    /* #MF  x87 floating-point                                 */
	ISR_ERR   17    /* #AC  alignment check        (error code)                */
	ISR_NOERR 18    /* #MC  machine check                                      */
	ISR_NOERR 19    /* #XM  SIMD floating-point                                */
	ISR_NOERR 20    /* #VE  virtualization                                     */
	ISR_ERR   21    /* #CP  control protection     (error code)                */
	ISR_NOERR 22
	ISR_NOERR 23
	ISR_NOERR 24
	ISR_NOERR 25
	ISR_NOERR 26
	ISR_NOERR 27
	ISR_NOERR 28
	ISR_NOERR 29
	ISR_NOERR 30
	ISR_NOERR 31

/* ---- the 16 hardware IRQ vectors, remapped to 32..47 --------------------- */
	IRQ 0,  32      /* PIT timer                                               */
	IRQ 1,  33      /* keyboard                                                */
	IRQ 2,  34      /* cascade (never raised directly)                         */
	IRQ 3,  35
	IRQ 4,  36
	IRQ 5,  37
	IRQ 6,  38
	IRQ 7,  39
	IRQ 8,  40
	IRQ 9,  41
	IRQ 10, 42
	IRQ 11, 43
	IRQ 12, 44
	IRQ 13, 45
	IRQ 14, 46
	IRQ 15, 47

/* Catch-all for the 208 unused vectors (48..255). They are masked/absent in
 * this kernel, but a stray vector should be handled, not triple-fault the box. */
isr_default:
	pushq $0
	pushq $255              /* sentinel vector number                          */
	jmp   isr_common

/* ---- the shared body: save state, call C, restore state, return ---------- */
isr_common:
	/* Save the 15 GP registers. Order matters: we push rax FIRST (it lands at
	 * the HIGHEST address) down to r15 LAST (the LOWEST address). Reading the
	 * stack from low to high therefore yields r15,r14,...,rax — precisely the
	 * field order of `struct regs`. Change one, change the other. */
	push %rax
	push %rbx
	push %rcx
	push %rdx
	push %rsi
	push %rdi
	push %rbp
	push %r8
	push %r9
	push %r10
	push %r11
	push %r12
	push %r13
	push %r14
	push %r15

	cld                     /* SysV ABI: direction flag must be clear on C entry*/
	mov  %rsp, %rdi         /* arg0 = &regs (the frame we just built) -> rdi    */
	call isr_dispatch       /* into C: classify + service the interrupt         */

	/* Restore in exact reverse order so each register gets its own value back. */
	pop %r15
	pop %r14
	pop %r13
	pop %r12
	pop %r11
	pop %r10
	pop %r9
	pop %r8
	pop %rbp
	pop %rdi
	pop %rsi
	pop %rdx
	pop %rcx
	pop %rbx
	pop %rax

	add $16, %rsp           /* pop the pushed int_no + err_code (2 x 8 bytes)   */
	iretq                   /* atomically restore RIP,CS,RFLAGS,RSP,SS -> resume*/

/* ---------------------------------------------------------------------------
 * isr_stub_table — 256 code addresses, one per vector, in vector order. idt.c
 * walks this to fill every IDT gate. Placed in .rodata: it is constant address
 * data the CPU never executes directly (it executes what the gates point AT).
 * ------------------------------------------------------------------------- */
	.section .rodata
	.align 8
	.global isr_stub_table
isr_stub_table:
	.quad isr0,  isr1,  isr2,  isr3,  isr4,  isr5,  isr6,  isr7
	.quad isr8,  isr9,  isr10, isr11, isr12, isr13, isr14, isr15
	.quad isr16, isr17, isr18, isr19, isr20, isr21, isr22, isr23
	.quad isr24, isr25, isr26, isr27, isr28, isr29, isr30, isr31
	.quad irq0,  irq1,  irq2,  irq3,  irq4,  irq5,  irq6,  irq7
	.quad irq8,  irq9,  irq10, irq11, irq12, irq13, irq14, irq15
	/* vectors 48..255 all route to the catch-all stub */
	.rept 256 - 48
	.quad isr_default
	.endr

/* Mark the stack as non-executable metadata-wise; harmless in a flat binary but
 * keeps the assembler from warning and documents intent. */
	.section .note.GNU-stack,"",@progbits
