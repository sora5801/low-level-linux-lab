# =============================================================================
# demo.annotated.s — clang -O1 output for asm/demo.c, explained instruction by
#                    instruction. Genuine compiler output (see demo.s for the
#                    untouched original); only comments and blank lines added.
# =============================================================================
#
# HOW TO READ THIS FILE
# ---------------------
# AT&T syntax throughout:  op  source, destination.  So `movq 8(%rdi), %rax`
# means rax = *(u64*)(rdi + 8).  `%reg` is a register, `$imm` an immediate,
# `sym(%rip)` a RIP-relative address, and `N(%base,%index)` is memory at
# base + index + N.  A 32-bit write (e.g. `movl`, `incl`) ZERO-EXTENDS into the
# full 64-bit register, which is why the compiler freely mixes eax/rax.
#
# THE SYSTEM V AMD64 ABI (what every function here obeys)
# ------------------------------------------------------
#   integer/pointer ARGUMENTS, in order:  rdi, rsi, rdx, rcx, r8, r9   (then stack)
#   RETURN value:                         rax  (rdx:rax for 128-bit)
#   CALLEE-SAVED (a function must restore):  rbx, rbp, r12, r13, r14, r15, rsp
#   CALLER-SAVED (scratch, free to clobber): rax, rcx, rdx, rsi, rdi, r8-r11
#   RED ZONE: 128 bytes below rsp a leaf function may use without adjusting rsp.
#   STACK ALIGNMENT: rsp must be 16-byte aligned at the point of a `call`.
#
# NOTE ON THE *SYSCALL* ABI vs. the *FUNCTION* ABI: these demo functions are
# ordinary C functions, so their args come in rdi, rsi, rdx, rcx, r8, r9. That
# is DIFFERENT from the kernel syscall ABI these functions model, where arg4 is
# r10 (not rcx). syscall_args() below exists precisely to bridge that gap: it is
# handed a snapshot of a stopped tracee's registers and pulls arg4 out of r10.
#
# WHAT'S IN HERE
#   1. syscall_args  — the register->argument mapping. Tiny and clean: eleven
#                      instructions, no branches. The ideal "C statement -> asm"
#                      reading exercise.
#   2. decode_flags  — the flag-decode table walk. clang INLINED put_hex() into
#                      it and UNROLLED the name-copy into a do-while, so this is
#                      also a tour of what -O1 does to a real loop.
# =============================================================================

	.file	"demo.c"
	.text

# =============================================================================
# u64 syscall_args(const struct regs *r, u64 args[6])
#   r    -> rdi   (pointer to the saved register snapshot)
#   args -> rsi   (destination array of 6 u64)
#   returns the syscall number (orig_rax) in rax
#
# struct regs layout (all u64, 8 bytes each), by byte offset:
#   +0  orig_rax   +8  rdi   +16 rsi   +24 rdx   +32 r10   +40 r8   +48 r9   +56 rax
# So "the six argument registers" are the fields at offsets 8..48, which is
# exactly what this function streams into args[0..5].
# =============================================================================
	.globl	syscall_args
	.p2align	4                       # 16-byte-align the entry for the fetcher
	.type	syscall_args,@function
syscall_args:
# ---- PROLOGUE (frame pointer kept at -O1 for legible backtraces) ------------
	pushq	%rbp                    # save caller's frame pointer
	movq	%rsp, %rbp              # rbp = frame base

# ---- args[0] = r->rdi  (the tracee's arg1) ----------------------------------
	movq	8(%rdi), %rax           # rax = r->rdi        (struct offset +8)
	movq	%rax, (%rsi)            # args[0] = rax
# ---- args[1] = r->rsi  (arg2) -----------------------------------------------
	movq	16(%rdi), %rax          # rax = r->rsi        (+16)
	movq	%rax, 8(%rsi)           # args[1] = rax
# ---- args[2] = r->rdx  (arg3) -----------------------------------------------
	movq	24(%rdi), %rax          # rax = r->rdx        (+24)
	movq	%rax, 16(%rsi)          # args[2] = rax
# ---- args[3] = r->r10  (arg4 — THE key line: r10 stands in for rcx) ---------
	movq	32(%rdi), %rax          # rax = r->r10        (+32)  <-- not rcx!
	movq	%rax, 24(%rsi)          # args[3] = rax
# ---- args[4] = r->r8   (arg5) -----------------------------------------------
	movq	40(%rdi), %rax          # rax = r->r8         (+40)
	movq	%rax, 32(%rsi)          # args[4] = rax
# ---- args[5] = r->r9   (arg6) -----------------------------------------------
	movq	48(%rdi), %rax          # rax = r->r9         (+48)
	movq	%rax, 40(%rsi)          # args[5] = rax

# ---- return r->orig_rax (the syscall number) --------------------------------
	movq	(%rdi), %rax            # rax = r->orig_rax   (+0) = the return value
	popq	%rbp                    # restore caller's frame pointer
	retq                            # return; rax already holds orig_rax
.Lfunc_end0:
	.size	syscall_args, .Lfunc_end0-syscall_args

# =============================================================================
# int decode_flags(u64 value, const struct flag *table, int n,
#                  char *out, int cap)
#   value -> rdi     table -> rsi     n -> edx     out -> rcx     cap -> r8d
#   returns pos (bytes written) in eax
#
# struct flag { u64 mask; const char *name; }  => 16 bytes: mask at +0, name +8.
#
# clang made three notable moves at -O1, all visible below:
#   (a) It hoisted `cap - 1` into r9d once and reuses it as the loop bound
#       everywhere (the C tests `pos < cap - 1` many times).
#   (b) It INLINED put_hex() — there is no `call`; the "0x", the nibble
#       extraction into a stack scratch buffer at -64(%rbp), and the reversed
#       emit are all right here.
#   (c) It turned the `while (nm[k])` name copy into a do-while with the
#       zero-check at the bottom, guarded by an entry test up top.
#
# Because it touches rbx and r12-r15, the prologue saves all five callee-saved
# registers and the epilogue restores them.
# =============================================================================
	.globl	decode_flags
	.p2align	4
	.type	decode_flags,@function
decode_flags:
# ---- PROLOGUE: frame + save every callee-saved register we will use ---------
	pushq	%rbp
	movq	%rsp, %rbp
	pushq	%r15
	pushq	%r14
	pushq	%r13
	pushq	%r12
	pushq	%rbx
                                        # (kill: r8d is both cap and, widened, r8)
# ---- Precompute the bound and the "skip the whole loop?" guard --------------
	leal	-1(%r8), %r9d           # r9d = cap - 1  (the reused loop bound)
	testl	%edx, %edx              # set flags from n
	setle	%r10b                   # r10b = (n <= 0)
	cmpl	$2, %r8d                # compare cap with 2
	setge	%bl                     # bl  = (cap >= 2)  == (pos < cap-1 when pos==0)
	setl	%r11b                   # r11b = (cap < 2)
	xorl	%eax, %eax              # eax = 0  => pos = 0
	orb	%r10b, %r11b            # r11b = (n<=0) | (cap<2)  => loop cannot run
	jne	.LBB1_1                 # ...so jump past the loop (to the tail code)

# ---- Loop setup (reached only when n>0 && cap>=2) ---------------------------
# %bb.16:
	movslq	%r9d, %r10              # r10 = (int64)(cap-1)  sign-extended bound
	movl	%edx, %edx              # zero-extend n into rdx (unsigned trip count)
	xorl	%r11d, %r11d            # r11 = 0  => i = 0 (loop index)
	xorl	%r14d, %r14d            # r14d = 0 => wrote_any = 0
	xorl	%eax, %eax              # eax = 0  => pos = 0
	.p2align	4

# ===== OUTER LOOP: for (i = 0; i < n && pos < cap-1; i++) =====================
.LBB1_17:                               # loop_head:
	movq	%r11, %rbx              # rbx = i
	shlq	$4, %rbx                # rbx = i * 16 = byte offset of table[i]
	movq	(%rsi,%rbx), %r15       # r15 = table[i].mask        (field +0)
	testq	%r15, %r15              # mask == 0 ?
	je	.LBB1_27                # if so, skip this row (the `mask != 0` guard)
# %bb.18:                                #   (mask is nonzero)
	movq	%r15, %r12              # r12 = mask
	andq	%rdi, %r12              # r12 = value & mask
	cmpq	%r15, %r12              # (value & mask) == mask ?
	jne	.LBB1_27                # if not all bits present, skip this row
# %bb.19:                                #   (this flag matches)

# ---- if (wrote_any && pos < cap-1) out[pos++] = '|' -------------------------
	testl	%r14d, %r14d            # wrote_any == 0 ?
	je	.LBB1_21                # if first name, no separator
# %bb.20:
	movslq	%eax, %r14              # r14 = (int64)pos   (scratch; wrote_any reset below)
	incl	%eax                    # pos++
	movb	$124, (%rcx,%r14)       # out[pos] = '|'   (124 = '|')

.LBB1_21:                               # copy_name:
# ---- nm = table[i].name;  k = 0 --------------------------------------------
	addq	%rsi, %rbx              # rbx = &table[i]  (base + offset)
	movq	8(%rbx), %r14           # r14 = table[i].name = nm     (field +8)
	movzbl	(%r14), %r15d           # r15b = nm[0]
	testb	%r15b, %r15b            # nm[0] == 0 ?  (empty name)
	je	.LBB1_26                # if empty, nothing to copy
# %bb.22:
	cmpl	%r9d, %eax              # pos >= cap-1 ?
	jge	.LBB1_26                # if no room, stop before copying
# %bb.23:
	movslq	%eax, %r12              # r12 = pos
	incq	%r12                    # r12 = pos + 1  (we index out[r12-1] below)
	incq	%r14                    # r14 = &nm[1]   (nm[0] already loaded)
	.p2align	4

# ===== INNER LOOP: while (nm[k] && pos < cap-1) out[pos++] = nm[k++] ==========
.LBB1_24:                               # name_byte:
	movb	%r15b, -1(%rcx,%r12)    # out[pos] = current char  (r12 == pos+1)
	movzbl	(%r14), %r15d           # r15b = next char nm[k+1]
	incl	%eax                    # pos++
	testb	%r15b, %r15b            # next char == 0 ?
	je	.LBB1_26                # if terminator, name done
# %bb.25:
	leaq	1(%r12), %r13           # r13 = (pos+1)+1
	incq	%r14                    # advance nm pointer
	cmpq	%r10, %r12              # (pos+1) < (cap-1) ?  (r10 = cap-1)
	movq	%r13, %r12              # r12 = pos+1 (post-increment out index)
	jl	.LBB1_24                # keep copying while there is room

.LBB1_26:                               # consume_bits:
# ---- value &= ~table[i].mask;  wrote_any = 1 -------------------------------
	movq	(%rbx), %rbx            # rbx = table[i].mask  (rbx was &table[i])
	notq	%rbx                    # rbx = ~mask
	andq	%rbx, %rdi              # value &= ~mask   (CONSUME the named bits)
	movl	$1, %r14d               # wrote_any = 1

.LBB1_27:                               # loop_next:  (also the "skip row" target)
	incq	%r11                    # i++
	cmpl	%r9d, %eax              # pos < cap-1 ?
	setl	%bl                     # bl = (pos < cap-1)   (saved for the tail)
	cmpq	%rdx, %r11              # i >= n ?
	jae	.LBB1_2                 # if i reached n, leave the loop
# %bb.28:
	cmpl	%r9d, %eax              # pos < cap-1 ?
	jl	.LBB1_17                # both conditions hold -> next iteration

# ===== AFTER THE LOOP ========================================================
.LBB1_2:                                # after_loop:
	testq	%rdi, %rdi              # value == 0 ?  (any unnamed bits left?)
	je	.LBB1_29                # if none, take the else-if branch

.LBB1_3:                                # have_leftover:  (value != 0)
# ---- if (wrote_any && pos < cap-1) out[pos++] = '|' -------------------------
	testl	%r14d, %r14d            # wrote_any != 0 ?
	setne	%dl                     # dl = (wrote_any != 0)
	andb	%bl, %dl                # dl = wrote_any && (pos < cap-1)
	cmpb	$1, %dl                 # both true ?
	je	.LBB1_4                 # if so, emit '|' before the hex
# %bb.5:
	cmpl	%r9d, %eax              # pos < cap-1 ?   (bound for the '0' of "0x")
	jl	.LBB1_6                 # room -> write '0'
.LBB1_7:                                # hex_x:
	cmpl	%r9d, %eax              # pos < cap-1 ?   (bound for the 'x')
	jge	.LBB1_9                 # no room -> skip 'x'
.LBB1_8:                                # write_x:
	movslq	%eax, %rdx
	incl	%eax                    # pos++
	movb	$120, (%rcx,%rdx)       # out[pos] = 'x'   (120 = 'x')

.LBB1_9:                                # hex_digits:  (put_hex, inlined; value != 0)
	xorl	%esi, %esi              # rsi = 0 => n (digit count); table ptr now dead
	leaq	put_hex.digits(%rip), %r10   # r10 = &"0123456789abcdef"
	.p2align	4
# ---- extract nibbles low-to-high into tmp[] at -64(%rbp) --------------------
.LBB1_10:                               # nibble:
	movl	%edi, %edx              # edx = value (low 32 bits enough per step)
	andl	$15, %edx               # edx = value & 0xf   (one nibble)
	movzbl	(%rdx,%r10), %r11d      # r11b = digits[nibble]
	leaq	1(%rsi), %rdx           # rdx = n + 1
	movb	%r11b, -64(%rbp,%rsi)   # tmp[n] = digit char
	cmpq	$16, %rdi               # value < 16 ?  (last nibble?)
	jb	.LBB1_12                # if so, stop extracting
# %bb.11:
	shrq	$4, %rdi                # value >>= 4
	cmpq	$15, %rsi               # n < 15 ?  (tmp has room)
	movq	%rdx, %rsi              # n++
	jb	.LBB1_10                # keep extracting nibbles

.LBB1_12:                               # emit_hex:
# ---- emit tmp[] high-to-low into out (the reversal) ------------------------
	cmpl	%r9d, %eax              # pos < cap-1 ?
	jge	.LBB1_31                # no room -> go terminate
# %bb.13:
	movslq	%eax, %rdi              # rdi = pos
	movslq	%r9d, %rsi              # rsi = cap-1  (bound)
	incq	%rdi                    # rdi = pos + 1  (index out[rdi-1])
	.p2align	4
.LBB1_14:                               # emit_byte:
	movzbl	-65(%rbp,%rdx), %r10d   # r10b = tmp[rdx-1]  (-65 = -64 - 1, reversed)
	movb	%r10b, -1(%rcx,%rdi)    # out[pos] = digit
	incl	%eax                    # pos++
	cmpq	$2, %rdx                # more than one digit left ?
	jl	.LBB1_31                # if this was the last, terminate
# %bb.15:
	decq	%rdx                    # step tmp index toward 0
	leaq	1(%rdi), %r10           # r10 = (pos+1)+1
	cmpq	%rsi, %rdi              # (pos+1) < (cap-1) ?
	movq	%r10, %rdi              # advance out index
	jl	.LBB1_14                # keep emitting while room
	jmp	.LBB1_31                # then terminate

# ---- Skipped-loop entry: n<=0 or cap<2. pos==0, wrote_any not yet set -------
.LBB1_1:                                # no_loop:
	xorl	%r14d, %r14d            # wrote_any = 0
	testq	%rdi, %rdi              # value == 0 ?
	jne	.LBB1_3                 # nonzero -> emit hex (bl already = cap>=2)

.LBB1_29:                               # else_if:  else if (!wrote_any && pos<cap-1)
	testl	%r14d, %r14d            # wrote_any == 0 ?
	sete	%dl                     # dl = (wrote_any == 0)
	andb	%bl, %dl                # dl = !wrote_any && (pos < cap-1)
	cmpb	$1, %dl                 # both true ?
	jne	.LBB1_31                # if not, nothing to add
# %bb.30:
	movslq	%eax, %rdx
	incl	%eax                    # pos++
	movb	$48, (%rcx,%rdx)        # out[pos] = '0'   (48 = '0'): "nothing set" case

.LBB1_31:                               # terminate:  NUL-terminate within cap
	movl	%eax, %edx              # edx = pos
	cmpl	%r8d, %eax              # pos < cap ?
	jl	.LBB1_33                # if so, write out[pos] = 0
# %bb.32:
	movl	%r9d, %edx              # else index = cap-1
	testl	%r8d, %r8d              # cap <= 0 ?
	jle	.LBB1_34                # if cap<=0, do not write at all
.LBB1_33:
	movslq	%edx, %rdx
	movb	$0, (%rcx,%rdx)         # out[idx] = '\0'

.LBB1_34:                               # EPILOGUE: restore callee-saved, return
	popq	%rbx
	popq	%r12
	popq	%r13
	popq	%r14
	popq	%r15
	popq	%rbp
	retq                            # eax already holds pos (the return value)

# ---- The '|' + first-'0' blocks, placed out-of-line by the scheduler --------
.LBB1_4:                                # emit_bar_before_hex:
	movslq	%eax, %rdx
	incl	%eax                    # pos++
	movb	$124, (%rcx,%rdx)       # out[pos] = '|'
	cmpl	%r9d, %eax              # pos < cap-1 ?
	jge	.LBB1_7                 # no room for '0' -> try 'x'
.LBB1_6:                                # write_hex_0:
	movslq	%eax, %rdx
	incl	%eax                    # pos++
	movb	$48, (%rcx,%rdx)        # out[pos] = '0'  (the '0' of the "0x" prefix)
	cmpl	%r9d, %eax              # pos < cap-1 ?
	jl	.LBB1_8                 # room -> write 'x'
	jmp	.LBB1_9                 # else jump to the digit emit
.Lfunc_end1:
	.size	decode_flags, .Lfunc_end1-decode_flags

# ---- READ-ONLY DATA: put_hex's digit table (inlined, but its data remains) --
	.type	put_hex.digits,@object
	.section	.rodata.str1.16,"aMS",@progbits,1
	.p2align	4, 0x0
put_hex.digits:
	.asciz	"0123456789abcdef"      # the nibble -> ASCII lookup used above
	.size	put_hex.digits, 17

	.ident	"clang version 20.1.8"
	.section	".note.GNU-stack","",@progbits   # non-executable stack (security)
	.addrsig
# =============================================================================
# WHAT TO TAKE AWAY
#   * syscall_args is the ABI mapping in the clear: six loads, and the one that
#     matters reads arg4 from r10 (offset +32), the kernel's substitute for rcx.
#   * decode_flags shows -O1 at work: cap-1 hoisted into r9d, put_hex fully
#     inlined (no `call`, a stack scratch buffer, a reversed emit), and the name
#     copy lowered to a do-while. The AND / compare-equal / AND-NOT trio
#     (`andq;cmpq;...;notq;andq`) IS the bit "consume" that makes the leftover
#     hex correct.
#   * Compare with demo.O0.s to see the same code with every variable spilled to
#     the stack and put_hex left as a real call — the naive, literal mapping.
# =============================================================================
