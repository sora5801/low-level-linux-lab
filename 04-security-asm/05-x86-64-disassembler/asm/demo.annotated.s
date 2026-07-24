# =============================================================================
# demo.annotated.s — clang -O1 output for asm/demo.c, explained instruction by
# instruction. This mirrors demo.s block-for-block (same labels, same order) so
# you can diff the two; demo.s is the untouched compiler output. AT&T syntax:
# `op src, dst`.  decode_modrm is the ModR/M + SIB + displacement decoder.
# =============================================================================
#
# ABI CONTRACT (System V AMD64)
# -----------------------------
# `amode decode_modrm(const u8 *p, int n, int rexR, int rexX, int rexB)` returns
# a 64-byte struct BY VALUE. A struct that large is returned via the "sret"
# convention: the CALLER allocates the result and passes a hidden POINTER to it
# in %rdi, so every real argument shifts down one register:
#
#     %rdi = hidden pointer to the 64-byte result (sret); also the return value
#     %rsi = p       (bytes after the opcode)      %ecx = rexR
#     %edx = n       (bytes available)             %r8d = rexX
#                                                  %r9d = rexB
#
# STRUCT LAYOUT (byte offsets the stores below target; 64 bytes total):
#     0  is_reg     4  reg        8  rm_reg    12 has_base   16 base
#     20 has_index  24 index      28 scale     32 rip_rel    36 has_disp
#     40 disp_size  44 (pad)      48 disp(8B)  56 length
#
# BIG PICTURE: at -O1 clang INLINED read_disp() and its byte loop, VECTORIZED the
# struct zero-init with SSE, packed the adjacent has_disp+disp_size ints into one
# 8-byte store, and sign-extended the disp8 BRANCHLESSLY. The three "stolen"
# ModR/M encodings (SIB escape, RIP-relative, no-base/no-index) are precisely the
# branches that make this look busy — in the C they are three short `if`s.
# =============================================================================

	.file	"demo.c"
	.section	.rodata.cst16,"aM",@progbits,16
	.p2align	4, 0x0
.LCPI0_0:                               # a 16-byte constant clang materialized
	.long	1
	.long	0
	.long	0
	.long	0
	.section	.rodata.cst4,"aM",@progbits,4
	.p2align	2, 0x0
.LCPI0_1:
	.long	1                       # bit pattern used to seed xmm0 = {1,0,0,0}
	.text
	.globl	decode_modrm
	.p2align	4
	.type	decode_modrm,@function
decode_modrm:                           # amode decode_modrm(p,n,rexR,rexX,rexB)
# %bb.0:
	pushq	%rbp                    # PROLOGUE: save caller's frame pointer
	movq	%rsp, %rbp              # establish our frame
	pushq	%r14                    # save callee-saved r14 (ABI: must preserve)
	pushq	%rbx                    # save callee-saved rbx
	movq	%rdi, %rax              # rax = &result (sret ptr; also return value)

	# ---- zero-initialize the whole struct (all the "a.x = 0;" lines) --------
	xorps	%xmm0, %xmm0            # xmm0 = 16 zero bytes
	movups	%xmm0, 12(%rdi)         # zero has_base/base/has_index/index (12..27)
	movups	%xmm0, (%rdi)           # zero is_reg/reg/rm_reg/has_base (0..15)
	movss	.LCPI0_1(%rip), %xmm0   # xmm0 = {1,0,0,0}: seed for scale=1
	movups	%xmm0, 28(%rdi)         # scale=1(28), rip_rel=0, has_disp=0, disp_size=0
	movq	$0, 48(%rdi)            # disp = 0
	movl	$0, 56(%rdi)            # length = 0
	movl	$-1, %edi               # edi = -1: the "truncated" length, held in a reg

	# ---- guard: if (n < 1) return with length = -1 --------------------------
	testl	%edx, %edx              # n <= 0 ?
	jle	.LBB0_34                # yes -> store length(=-1) and return
# %bb.1:
	# ---- decode the ModR/M byte into mod / reg / rm -------------------------
	movzbl	(%rsi), %r11d           # r11d = p[0] = ModR/M byte
	movl	%r11d, %r10d
	shrl	$6, %r10d               # r10d = modrm >> 6         = mod
	movl	%r11d, %r14d
	shrl	$3, %r14d
	andl	$7, %r14d               # r14d = (modrm >> 3) & 7   = reg
	movl	%r11d, %ebx
	andl	$7, %ebx                # ebx  = modrm & 7          = rm
	leal	(%r14,%rcx,8), %ecx     # ecx  = reg + rexR*8
	movl	%ecx, 4(%rax)           # a.reg = reg | (rexR<<3)
	cmpl	$3, %r10d               # mod == 3 (register-direct) ?
	jne	.LBB0_3                 # no -> memory forms
# %bb.2:
	# ---- mod==11: r/m is a register; no SIB, no displacement ----------------
	movl	$1, (%rax)              # a.is_reg = 1
	leal	(%rbx,%r9,8), %ecx      # ecx = rm + rexB*8
	movl	%ecx, 8(%rax)           # a.rm_reg = rm | (rexB<<3)
	movl	$1, %edi                # length = 1 (only the ModR/M byte)
.LBB0_34:                               # .Lstore_len_and_return
	movl	%edi, 56(%rax)          # a.length = edi
.LBB0_35:                               # .Lepilogue
	popq	%rbx                    # restore callee-saved regs
	popq	%r14
	popq	%rbp                    # restore frame pointer
	retq                            # rax = &result (ABI return)
.LBB0_3:                                # .Lnot_reg (mod != 3)
	# ---- is there a SIB byte? (rule A: rm == 100) ---------------------------
	cmpl	$4, %ebx                # rm == 4 ?
	jne	.LBB0_18                # no -> .Lno_sib (plain base or RIP-relative)
# %bb.4:
	cmpl	$1, %edx                # only 1 byte available but we need the SIB ?
	je	.LBB0_34                # truncated -> length = -1
# %bb.5:
	# ---- decode the SIB byte into scale-exp / idx3 / base3 ------------------
	movzbl	1(%rsi), %ecx           # ecx = p[1] = SIB byte
	movl	%ecx, %r14d
	shrl	$3, %r14d
	andl	$7, %r14d               # r14d = (sib >> 3) & 7 = idx3 (index field)
	movl	%ecx, %ebx
	andl	$7, %ebx                # ebx  = sib & 7        = base3 (base field)
	testl	%r8d, %r8d              # rexX != 0 ?  (REX.X==1 makes idx3==100 = r12)
	jne	.LBB0_8                 #   -> index present
# %bb.6:
	cmpl	$4, %r14d               # idx3 == 4 (RSP slot) ?
	jne	.LBB0_8                 #   != 4 -> index present
# %bb.7:  rule C(index): idx3==100 && REX.X==0  =>  NO index register
	movl	$0, 20(%rax)            # a.has_index = 0
	jmp	.LBB0_9                 # go decide the base
.LBB0_18:                               # .Lno_sib (rm != 4)
	# ---- RIP-relative? (rule B: mod==0 && rm==101) --------------------------
	cmpb	$64, %r11b              # modrm >= 0x40 ?  (mod != 0)
	setae	%cl                     # cl  = (mod != 0)
	cmpl	$5, %ebx                # rm == 5 ?
	setne	%r8b                    # r8b = (rm != 5)
	orb	%cl, %r8b               # r8b = (mod != 0) || (rm != 5)
	jne	.LBB0_23                #   true -> plain [reg] base (.Lplain_base)
# %bb.19:  rule B: mod==0 && rm==5  =>  RIP-relative disp32, no base register
	movl	$1, 32(%rax)            # a.rip_rel = 1
	cmpl	$5, %edx                # fewer than 5 bytes (ModR/M + 4 disp) ?
	jl	.LBB0_34                # truncated -> length = -1
# %bb.20:  read the RIP-relative disp32 (inlined read_disp(p+1, 4))
	incq	%rsi                    # skip the ModR/M byte -> point at disp
	xorl	%ecx, %ecx              # ecx = shift count = 0
	xorl	%edx, %edx              # edx = little-endian accumulator = 0
	.p2align	4
.LBB0_21:                               # 4-iteration byte-assembly loop
	movzbl	(%rsi), %edi            # edi = next byte
	shlq	%cl, %rdi               # shift into position (0,8,16,24)
	orq	%rdi, %rdx              # OR into accumulator
	addq	$8, %rcx                # next byte 8 bits higher
	incq	%rsi                    # advance pointer
	cmpq	$32, %rcx               # assembled 32 bits ?
	jne	.LBB0_21
# %bb.22:  sign-extend the 32-bit value to 64 and store the RIP result
	movslq	%edx, %rcx              # rcx = sign-extend(low 32 bits)
	movabsq	$-4294967296, %rsi      # rsi = 0xFFFFFFFF00000000 (high-half mask)
	andq	%rcx, %rsi              # keep the sign-extended high 32 bits
	orq	%rdx, %rsi              # merge with the original low 32 bits
	movq	%rsi, 48(%rax)          # a.disp = (i64)(int32)disp32
	movabsq	$17179869185, %rcx      # 0x0000000400000001 : two int fields at once
	movq	%rcx, 36(%rax)          #   a.has_disp = 1 (off 36), a.disp_size = 4 (off 40)
	movl	$5, %edi                # length = 5 (ModR/M + 4 disp bytes)
	jmp	.LBB0_34                # store length and return
.LBB0_23:                               # .Lplain_base : [reg] with rm as the base
	movl	$1, 12(%rax)            # a.has_base = 1
	leal	(%rbx,%r9,8), %ecx      # ecx = rm + rexB*8
	movl	%ecx, 16(%rax)          # a.base = rm | (rexB<<3)
	movl	$1, %ecx                # ecx = pos = 1 (only ModR/M consumed so far)
	jmp	.LBB0_24                # go pick displacement by mod
.LBB0_8:                                # .Lsib_has_index
	shrl	$6, %ecx                # ecx = sib >> 6 = ss (scale exponent)
	movl	$1, 20(%rax)            # a.has_index = 1
	leal	(%r14,%r8,8), %r8d      # r8d = idx3 + rexX*8 = full index register
	movl	%r8d, 24(%rax)          # a.index = idx3 | (rexX<<3)
	movl	$1, %r8d
	shll	%cl, %r8d               # r8d = 1 << ss = 1/2/4/8
	movl	%r8d, 28(%rax)          # a.scale = 1 << ss
.LBB0_9:                                # .Lsib_decide_base
	# ---- rule C(base): base3==101 && mod==00  =>  no base, disp32 stands alone
	cmpl	$5, %ebx                # base3 == 5 ?
	setne	%cl                     # cl  = (base3 != 5)
	cmpb	$64, %r11b              # mod != 0 ?
	setae	%r8b                    # r8b = (mod != 0)
	orb	%cl, %r8b               # r8b = (base3!=5) || (mod!=0)
	je	.LBB0_10                #   false&false -> base3==5 && mod==0 -> no base
# %bb.15:  normal SIB base present
	movl	$1, 12(%rax)            # a.has_base = 1
	leal	(%rbx,%r9,8), %ecx      # ecx = base3 + rexB*8
	movl	%ecx, 16(%rax)          # a.base = base3 | (rexB<<3)
	jmp	.LBB0_16
.LBB0_10:                               # .Lsib_nobase : rule C(base)
	movl	$0, 12(%rax)            # a.has_base = 0
	cmpl	$5, %edx                # more than 5 bytes ? (pos=2 + 4 disp => n>=6)
	jg	.LBB0_12                # enough -> read disp32
# %bb.11:  truncated
	movl	$-1, 56(%rax)           # a.length = -1
.LBB0_16:                               # shared bridge to the mod-based disp code
	movl	$2, %ecx                # ecx = pos = 2 (ModR/M + SIB)
	jmp	.LBB0_17
.LBB0_12:                               # read disp32 for the no-base SIB case
	leaq	2(%rsi), %r11           # r11 = p + 2 (skip ModR/M + SIB)
	xorl	%ecx, %ecx              # shift count = 0
	xorl	%r9d, %r9d              # accumulator = 0  (kept in r9 on this copy)
	.p2align	4
.LBB0_13:                               # 4-iteration byte-assembly loop
	movzbl	(%r11), %ebx            # next byte
	shlq	%cl, %rbx               # position it
	orq	%rbx, %r9               # OR into accumulator
	addq	$8, %rcx
	incq	%r11
	cmpq	$32, %rcx
	jne	.LBB0_13
# %bb.14:  sign-extend + store; length = 6 (ModR/M + SIB + 4 disp)
	movslq	%r9d, %rcx              # sign-extend low 32 bits
	movabsq	$-4294967296, %r11      # 0xFFFFFFFF00000000
	andq	%rcx, %r11
	orq	%r9, %r11
	movq	%r11, 48(%rax)          # a.disp = (i64)(int32)disp32
	movabsq	$17179869185, %rcx      # 0x400000001 -> has_disp=1, disp_size=4
	movq	%rcx, 36(%rax)
	movl	$6, 56(%rax)            # a.length = 6
	movl	$6, %ecx                # (ecx would be pos, but we return next)
.LBB0_17:                               # bridge: continue to disp, or return?
	testb	%r8b, %r8b              # r8b==0 on the no-base paths (already stored)
	je	.LBB0_35                #   -> just return (length already written)
.LBB0_24:                               # .Ldisp_by_mod  (ecx = pos)
	# ---- displacement dictated by mod: 2 -> disp32, 1 -> disp8, 0 -> none ----
	cmpl	$2, %r10d               # mod == 2 ?
	je	.LBB0_29                #   -> disp32
# %bb.25:
	cmpl	$1, %r10d               # mod == 1 ?
	jne	.LBB0_26                #   mod == 0 -> no displacement
# %bb.27:  mod==1: disp8
	cmpl	%edx, %ecx              # pos >= n ? (need one more byte)
	jge	.LBB0_34                # truncated -> length = -1
# %bb.28:  read one byte and BRANCHLESSLY sign-extend it
	movl	%ecx, %edx              # edx = pos
	movzbl	(%rsi,%rdx), %edx       # edx = p[pos] (0..255, zero-extended)
	incl	%ecx                    # pos++
	xorl	%esi, %esi
	testb	%dl, %dl                # look at the byte's sign bit
	setns	%sil                    # sil = 1 if byte >= 0, else 0
	shll	$8, %esi                # esi = 256 (positive) or 0 (negative)
	addq	%rdx, %rsi              # rsi = byte + (positive ? 256 : 0)
	addq	$-256, %rsi             # rsi = sext8(byte)  (0xFF -> -1, 0x10 -> 16)
	movl	$1, %edx                # disp_size = 1
	movl	%ecx, %r8d              # length = pos
	jmp	.LBB0_33
.LBB0_29:                               # mod==2: disp32
	leal	4(%rcx), %r8d           # r8d = pos + 4 = prospective length
	cmpl	%edx, %r8d              # pos+4 > n ?
	jg	.LBB0_34                # truncated -> length = -1
# %bb.30:
	movl	%ecx, %ecx              # zero-extend pos
	addq	%rcx, %rsi              # rsi = p + pos
	xorl	%ecx, %ecx              # shift count = 0
	xorl	%edx, %edx              # accumulator = 0
	.p2align	4
.LBB0_31:                               # 4-iteration byte-assembly loop
	movzbl	(%rsi), %edi
	shlq	%cl, %rdi
	orq	%rdi, %rdx
	addq	$8, %rcx
	incq	%rsi
	cmpq	$32, %rcx
	jne	.LBB0_31
# %bb.32:  sign-extend to 64
	movslq	%edx, %rcx
	movabsq	$-4294967296, %rsi      # 0xFFFFFFFF00000000
	andq	%rcx, %rsi
	orq	%rdx, %rsi              # rsi = sext32(value)
	movl	$4, %edx                # disp_size = 4
.LBB0_33:                               # .Lstore_disp (shared by disp8 and disp32)
	movq	%rsi, 48(%rax)          # a.disp      = sign-extended displacement
	movl	%edx, 40(%rax)          # a.disp_size = 1 or 4
	movl	$1, 36(%rax)            # a.has_disp  = 1
	movl	%r8d, %edi              # length = pos (final)
	jmp	.LBB0_34                # store length and return
.LBB0_26:                               # .Lno_disp (mod==0 with a base register)
	movl	%ecx, %edi              # length = pos (ModR/M [+ SIB])
	jmp	.LBB0_34                # store length and return
.Lfunc_end0:
	.size	decode_modrm, .Lfunc_end0-decode_modrm

# =============================================================================
# WHAT TO TAKE AWAY
#   * sret: a >16-byte struct return becomes a hidden pointer in %rdi; the real
#     args shift to rsi/edx/ecx/r8d/r9d. Miss this and every arg is off by one.
#   * The optimizer zero-inits the struct with three SSE stores and writes the
#     two adjacent int fields has_disp+disp_size as ONE 8-byte movabsq.
#   * Sign-extension appears two ways: movslq+high-mask for disp32; a branchless
#     setns trick for disp8. Both reconstruct a two's-complement value.
#   * The three "stolen" encodings are the branches at .LBB0_3 (SIB escape),
#     .LBB0_18 (RIP-relative), and .LBB0_9/.LBB0_10 (no-base / no-index). Learn
#     ModR/M by correlating those with the three short `if`s in demo.c.
#   * Compare with demo.O0.s (naive, everything spilled) and demo.O2.s.
	.ident	"clang version 20.1.8"
	.section	".note.GNU-stack","",@progbits
