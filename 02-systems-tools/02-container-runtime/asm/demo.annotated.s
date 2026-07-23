# =============================================================================
# demo.annotated.s — clang's -O1 output for demo.c, explained instruction by
# instruction. The seccomp decision core, as real x86-64 machine code.
# =============================================================================
#
# SOURCE OF TRUTH: this is a hand-annotated copy of asm/demo.s (clang 20, -O1,
# --target=x86_64-pc-linux-gnu, -fno-jump-tables -fno-omit-frame-pointer). Every
# instruction below appears in demo.s unchanged; only the comments are added.
#
# HOW TO READ AT&T SYNTAX
# -----------------------
#     op   src, dst              # dst = dst OP src   (destination is LAST)
#     movl $1, %eax              # eax = 1            ($ = immediate literal)
#     %reg                       # a register;  %eax is the low 32 bits of %rax
#     N(%base,%index,scale)      # memory at [base + index*scale + N]
#     symbol(%rip)               # RIP-relative address of `symbol` (PIC-friendly)
#   Writing a 32-bit register (eax) ZERO-EXTENDS into the 64-bit one (rax); that
#   is why `xorl %ecx,%ecx` is the idiom for "rcx = 0" (2 bytes, breaks the
#   dependency chain) and why 32-bit ops dominate this listing.
#
# THE SYSTEM V AMD64 ABI (what these functions obey)
# --------------------------------------------------
#   * integer/pointer ARGUMENTS, in order:  rdi, rsi, rdx, rcx, r8, r9, then stack
#   * RETURN value:                          rax (eax for 32-bit)
#   * CALLEE-SAVED (a function must preserve): rbx, rbp, r12, r13, r14, r15, rsp
#   * CALLER-SAVED (a call may clobber):       rax, rcx, rdx, rsi, rdi, r8-r11
#   * THE RED ZONE: 128 bytes below rsp a LEAF function may scribble on without
#     adjusting rsp. bpf_run is a leaf and uses no stack locals, so it never
#     touches rsp beyond the pushed frame pointer.
#   * STACK ALIGNMENT: at the instant a `call` executes, rsp must be 16-byte
#     aligned. `call` pushes the 8-byte return address, so on entry rsp ≡ 8
#     (mod 16); the `pushq %rbp` in each prologue brings it back to 16.
#
# THE THREE FUNCTIONS
# -------------------
#   bpf_run   — the classic-BPF interpreter (the star; annotated in full).
#   classify  — a thin wrapper: build a seccomp_data on the stack, call bpf_run.
#   main      — call bpf_run three times and pack the results into an exit code.
# We forced bpf_run to stay a real function (noinline in the C) so this is the
# GENERAL, data-driven interpreter — exactly the shape the kernel keeps.
# =============================================================================

	.file	"demo.c"
	.text                                   # executable code section

# =============================================================================
# u32 bpf_run(const struct binsn *prog, u32 len, const struct sdata *in)
#   prog -> rdi   (pointer to the instruction array; each binsn is 8 bytes)
#   len  -> esi   (instruction count)
#   in   -> rdx   (pointer to {u32 nr; u32 arch})
#   ret  -> eax   (the filter verdict word)
#
# Register roles the optimizer chose for the loop:
#   ecx = pc  (program counter / index)     r8d = A   (the BPF accumulator)
#   r9  = scratch: first the pc index for addressing, then the "keep looping?"
#         boolean in its low byte r9b        r11d = ins.code    r10d = ins.k
# =============================================================================
	.globl	bpf_run
	.p2align	4                       # 16-byte-align the entry for the fetcher
	.type	bpf_run,@function
bpf_run:
# %bb.0:  -- PROLOGUE ---------------------------------------------------------
	pushq	%rbp                    # save caller's frame pointer (callee-saved)
	movq	%rsp, %rbp              # establish our frame; rsp now 16-byte aligned
	xorl	%ecx, %ecx              # pc = 0
	xorl	%r8d, %r8d              # A  = 0   (accumulator starts cleared)
                                        # implicit-def: $eax  — eax is the return
                                        #   slot; every path sets it before use,
                                        #   so clang leaves it undefined here.
	jmp	.LBB0_3                 # jump straight to the loop's bounds check

# ---- default / unknown-opcode path: "return RET_KILL (0)" -------------------
# Reached by `jne .LBB0_1` when ins.code matched none of the 4 opcodes.
.LBB0_1:
	xorl	%r9d, %r9d              # keep-looping = 0  (we are about to return)
	xorl	%eax, %eax              # return value = 0 = RET_KILL
	.p2align	4
# ---- loop tail: decide whether to iterate again -----------------------------
.LBB0_2:
	testb	%r9b, %r9b              # test the keep-looping flag (r9b)
	je	.LBB0_21                # 0 => fall out to the epilogue (return eax)
# ---- loop header: the `if (pc >= len) return KILL;` bounds check ------------
.LBB0_3:                                # top of `for (;;)`
	cmpl	%esi, %ecx              # compute pc - len (unsigned)
	jae	.LBB0_20                # pc >= len  => out of range, return 0
# %bb.4:  fetch the current instruction: ins = prog[pc]
	movl	%ecx, %r9d              # r9  = pc, zero-extended to 64 bits for use
                                        #   as an array index (addresses are 64-bit)
	movzwl	(%rdi,%r9,8), %r11d     # r11d = prog[pc].code — load the u16 at
                                        #   [prog + pc*8 + 0], zero-extended. The
                                        #   scale 8 = sizeof(struct binsn).
	movl	4(%rdi,%r9,8), %r10d     # r10d = prog[pc].k — the u32 at offset +4
	cmpl	$20, %r11d              # split the switch: is code > 20 ?
	jg	.LBB0_8                 #   yes -> {21=JEQ, 32=LD_ABS} handled at _8
# %bb.5:  low opcodes: code is 5 (JA) or 6 (RET) or unknown
	cmpl	$5, %r11d               # code == 5 (OP_JA)?
	je	.LBB0_13                #   yes -> unconditional jump
# %bb.6:
	cmpl	$6, %r11d               # code == 6 (OP_RET_K)?
	jne	.LBB0_1                 #   no  -> unknown opcode -> KILL
# %bb.7:  OP_RET_K — `return ins.k;`
	xorl	%r9d, %r9d              # keep-looping = 0 (terminate the loop)
	movl	%r10d, %eax             # return value = ins.k
	jmp	.LBB0_2                 # go to the tail; r9b==0 -> will return eax
	.p2align	4
# ---- high opcodes: code is 21 (JEQ_K) or 32 (LD_ABS) ------------------------
.LBB0_8:
	cmpl	$21, %r11d              # code == 21 (OP_JEQ_K)?
	je	.LBB0_14                #   yes -> conditional branch
# %bb.9:
	cmpl	$32, %r11d              # code == 32 (OP_LD_ABS)?
	jne	.LBB0_1                 #   no  -> unknown opcode -> KILL
# %bb.10:  OP_LD_ABS — `A = load(in, ins.k)`
	cmpl	$4, %r10d               # is the load offset k == 4 (OFF_ARCH)?
	je	.LBB0_18                #   yes -> load in->arch
# %bb.11:  offset is 0 (nr) or out of range
	xorl	%r8d, %r8d              # A = 0 (the out-of-range default)
	testl	%r10d, %r10d            # is k == 0 (OFF_NR)?
	jne	.LBB0_19                #   k != 0 -> leave A = 0, advance pc
# %bb.12:  k == 0 (OFF_NR): `A = in->nr`
	movl	(%rdx), %r8d            # A = *(u32 *)(in + 0)  = in->nr
	jmp	.LBB0_19                # advance pc, continue
# ---- OP_JA — `pc = pc + 1 + ins.k;` -----------------------------------------
.LBB0_13:
	addl	%r10d, %ecx             # pc += k
	incl	%ecx                    # pc += 1
	movb	$1, %r9b                # keep-looping = 1
	jmp	.LBB0_2                 # iterate
# ---- OP_JEQ_K — `pc += 1 + (A==k ? jt : jf);` -------------------------------
.LBB0_14:
	incl	%ecx                    # pc += 1  (the "+1", applied before jt/jf)
	cmpl	%r10d, %r8d             # compare A (r8d) with k (r10d)
	jne	.LBB0_16                # not equal -> take the jf branch
# %bb.15:  A == k: add jt
	movzbl	2(%rdi,%r9,8), %r9d     # r9d = prog[pc].jt — the u8 at offset +2,
                                        #   using the ORIGINAL pc index still in r9
	jmp	.LBB0_17
.LBB0_16:  # A != k: add jf
	movzbl	3(%rdi,%r9,8), %r9d     # r9d = prog[pc].jf — the u8 at offset +3
.LBB0_17:
	addl	%r9d, %ecx              # pc += (jt or jf)
	movb	$1, %r9b                # keep-looping = 1 (r9b overwrites the low
                                        #   byte of r9; the index use above is done)
	jmp	.LBB0_2                 # iterate
# ---- OP_LD_ABS, arch case — `A = in->arch` ----------------------------------
.LBB0_18:
	movl	4(%rdx), %r8d           # A = *(u32 *)(in + 4) = in->arch
.LBB0_19:                               # shared "advance and continue" tail
	incl	%ecx                    # pc += 1
	movb	$1, %r9b                # keep-looping = 1
	jmp	.LBB0_2                 # iterate
# ---- out-of-range exit — `return RET_KILL (0)` ------------------------------
.LBB0_20:
	xorl	%eax, %eax              # return value = 0 = RET_KILL
# ---- EPILOGUE ---------------------------------------------------------------
.LBB0_21:
	popq	%rbp                    # restore caller's frame pointer
	retq                            # return; eax holds the verdict
.Lfunc_end0:
	.size	bpf_run, .Lfunc_end0-bpf_run

# =============================================================================
# u32 classify(u32 sysno)   — build a seccomp_data on the stack, run the filter.
#   sysno -> edi         ret -> eax
# This shows STRUCT-ON-STACK construction and the ABI of a real `call`.
# =============================================================================
	.globl	classify
	.p2align	4
	.type	classify,@function
classify:
# %bb.0:
	pushq	%rbp                    # PROLOGUE: save frame pointer
	movq	%rsp, %rbp              # set up frame
	subq	$16, %rsp               # reserve 16 bytes of locals for `struct
                                        #   sdata ev` (8 bytes, rounded up to keep
                                        #   rsp 16-aligned for the upcoming call)
	movl	%edi, -8(%rbp)          # ev.nr   = sysno            (offset 0 of ev)
	movl	$-1073741762, -4(%rbp)  # ev.arch = 0xC000003E       (offset 4 of ev)
                                        #   (-1073741762 is 0xC000003E as signed)
	leaq	allowlist(%rip), %rdi   # arg0 = &allowlist   (the compiled filter)
	leaq	-8(%rbp), %rdx          # arg2 = &ev          (address of our struct)
	movl	$9, %esi                # arg1 = 9            (ALLOWLIST_LEN)
	callq	bpf_run                 # eax = bpf_run(prog, 9, &ev). `call` pushed
                                        #   the return address; rsp was 16-aligned
                                        #   just before, per the ABI.
	addq	$16, %rsp               # free the locals
	popq	%rbp                    # EPILOGUE
	retq                            # return bpf_run's verdict (already in eax)
.Lfunc_end1:
	.size	classify, .Lfunc_end1-classify

# =============================================================================
# int main(void) — probe read/write/ptrace and pack a 3-bit result (expect 7).
#
# Watch the compiler build each `struct sdata` with ONE 64-bit store: the struct
# is 8 bytes, nr in the low dword, arch in the high dword. On little-endian
# x86-64 the immediate 0xC000003E_00000000 therefore means {nr=0, arch=X86_64}.
# rbx is used as a callee-saved cache of &allowlist across all three calls, so it
# must be (and is) saved and restored.
# =============================================================================
	.globl	main
	.p2align	4
	.type	main,@function
main:
# %bb.0:  -- PROLOGUE --
	pushq	%rbp                    # save frame pointer
	movq	%rsp, %rbp              # set up frame
	pushq	%r14                    # save r14 (callee-saved; holds running result)
	pushq	%rbx                    # save rbx (callee-saved; will cache &allowlist)
	subq	$16, %rsp               # reserve space for the reused `struct sdata`

# ---- probe 1: classify(NR_READ = 0) ----------------------------------------
	movabsq	$-4611685752139415552, %rax     # rax = 0xC000003E00000000
	movq	%rax, -24(%rbp)         # ev = {nr=0 (READ), arch=X86_64} in one store
	leaq	allowlist(%rip), %rbx   # rbx = &allowlist (cached for all 3 calls)
	leaq	-24(%rbp), %rdx         # arg2 = &ev
	movq	%rbx, %rdi              # arg0 = &allowlist
	movl	$9, %esi                # arg1 = 9
	callq	bpf_run                 # eax = verdict for read
	xorl	%r14d, %r14d            # result = 0
	cmpl	$2147418112, %eax       # verdict == 0x7FFF0000 (RET_ALLOW)?
	sete	%r14b                   # result bit0 = (read allowed) -> the "1" bit

# ---- probe 2: classify(NR_WRITE = 1) ---------------------------------------
	movabsq	$-4611685752139415551, %rax     # rax = 0xC000003E00000001
	movq	%rax, -24(%rbp)         # ev = {nr=1 (WRITE), arch=X86_64}
	leaq	-24(%rbp), %rdx         # arg2 = &ev
	movq	%rbx, %rdi              # arg0 = &allowlist (from the cache)
	movl	$9, %esi                # arg1 = 9
	callq	bpf_run                 # eax = verdict for write
	xorl	%ecx, %ecx              # tmp = 0
	cmpl	$2147418112, %eax       # verdict == RET_ALLOW?
	sete	%cl                     # cl = (write allowed)
	leal	(%r14,%rcx,2), %r14d    # result = result + cl*2  -> OR in the "2" bit

# ---- probe 3: classify(101 = ptrace), which must be DENIED ------------------
	movabsq	$-4611685752139415451, %rax     # rax = 0xC000003E00000065  (0x65 = 101)
	movq	%rax, -24(%rbp)         # ev = {nr=101 (ptrace), arch=X86_64}
	leaq	-24(%rbp), %rdx         # arg2 = &ev
	movq	%rbx, %rdi              # arg0 = &allowlist
	movl	$9, %esi                # arg1 = 9
	callq	bpf_run                 # eax = verdict for ptrace
	xorl	%ecx, %ecx              # tmp = 0
	cmpl	$2147418112, %eax       # verdict == RET_ALLOW?
	setne	%cl                     # cl = (ptrace NOT allowed) — note setNE
	leal	(%r14,%rcx,4), %eax     # return = result + cl*4  -> OR in the "4" bit
                                        #   => 1 + 2 + 4 = 7 on a correct build

# ---- EPILOGUE: unwind the saved registers in REVERSE push order -------------
	addq	$16, %rsp               # free the locals
	popq	%rbx                    # restore rbx
	popq	%r14                    # restore r14
	popq	%rbp                    # restore frame pointer
	retq                            # exit status = eax (7)
.Lfunc_end2:
	.size	main, .Lfunc_end2-main

# =============================================================================
# READ-ONLY DATA: the compiled allowlist, laid out byte-for-byte.
#
# Each row is one `struct binsn` = 8 bytes:  .short code | .byte jt | .byte jf |
# (1 pad byte) | .long k.  This is the exact program seccomp.c would hand the
# kernel, and the exact bytes bpf_run walks above.
# =============================================================================
	.type	allowlist,@object
	.section	.rodata,"a",@progbits   # "a" = allocatable, read-only
	.p2align	4, 0x0                  # 16-byte align the table
allowlist:
	.short	32                              # [0] code=0x20 LD_ABS
	.byte	0                               #     jt
	.byte	0                               #     jf
	.long	4                               #     k=4    -> load in->arch
	.short	21                              # [1] code=0x15 JEQ_K
	.byte	1                               #     jt=1   -> if equal, skip insn [2]
	.byte	0                               #     jf=0
	.long	3221225534                      #     k=0xC000003E (AUDIT_ARCH_X86_64)
	.short	6                               # [2] code=0x06 RET_K
	.byte	0
	.byte	0
	.long	0                               #     k=0    -> RET_KILL (wrong arch)
	.short	32                              # [3] code=0x20 LD_ABS
	.byte	0
	.byte	0
	.long	0                               #     k=0    -> load in->nr
	.short	21                              # [4] code=0x15 JEQ_K
	.byte	0                               #     jt=0   -> if equal, fall to [5]
	.byte	1                               #     jf=1   -> else skip [5]
	.long	0                               #     k=0    (__NR_read)
	.short	6                               # [5] code=0x06 RET_K
	.byte	0
	.byte	0
	.long	2147418112                      #     k=0x7FFF0000 RET_ALLOW
	.short	21                              # [6] code=0x15 JEQ_K
	.byte	0
	.byte	1                               #     jf=1
	.long	1                               #     k=1    (__NR_write)
	.short	6                               # [7] code=0x06 RET_K
	.byte	0
	.byte	0
	.long	2147418112                      #     k=0x7FFF0000 RET_ALLOW
	.short	6                               # [8] code=0x06 RET_K
	.byte	0
	.byte	0
	.long	327681                          #     k=0x00050001 ERRNO|EPERM (default)
	.size	allowlist, 72                   # 9 instructions * 8 bytes = 72

	.ident	"clang version 20.1.8"          # toolchain stamp (metadata)
	.section	".note.GNU-stack","",@progbits  # non-executable stack: security
                                                        #   default the linker records
# =============================================================================
# WHAT TO TAKE AWAY
#   * A BPF filter decision is just: load a field, compare, branch, return — the
#     same handful of instructions the kernel runs on EVERY syscall you make.
#   * -fno-jump-tables turned the switch into the compare-chain at .LBB0_5/_8;
#     with jump tables it would be an indirect jump through a .rodata table.
#   * classify shows a struct built on the stack and passed by address; main
#     shows the optimizer folding that 8-byte struct into a single movabsq store.
#   * Compare with demo.O0.s (every value spilled to the stack, nothing cached in
#     registers) and demo.O2.s (tighter still) to see the optimizer at work.
# =============================================================================
