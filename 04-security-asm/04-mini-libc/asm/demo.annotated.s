# =============================================================================
# demo.annotated.s — u64_to_dec / i64_to_dec (printf's %u/%d core), explained.
# =============================================================================
#
# HOW TO READ THIS FILE
# ---------------------
# This is clang's -O1 output for asm/demo.c (see the untouched demo.s), with a
# comment on essentially every instruction. AT&T syntax: `op src, dst`, %reg is
# a register, $imm a literal, N(%base,%index) is memory at base+index+N. Writing
# a 32-bit name (eax, ecx) zero-extends into the 64-bit register (rax, rcx).
#
# THE ONE TRICK WORTH THE WHOLE FILE
# ----------------------------------
# The C source divides by 10 every iteration. The optimizer NEVER emits the slow
# hardware `div` for a constant divisor. Instead it multiplies by a precomputed
# "magic" reciprocal and shifts:
#
#     q = v / 10   becomes   q = (v * 0xCCCCCCCCCCCCCCCD) >> 67
#
# 0xCCCCCCCCCCCCCCCD is ceil(2^67 / 10). `mulq` forms the full 128-bit product
# in rdx:rax (rdx = product >> 64); one more `shr $3` makes it >> 67 total.
#
# Then the remainder is recovered WITHOUT a second divide, using a byte-width
# identity: 246 == 256 - 10, so 246 ≡ -10 (mod 256). Therefore
#     (q*246 + v) mod 256  ≡  (v - 10*q) mod 256  =  r        (since 0<=r<=9)
# and the low byte `al` already holds the digit. Watch for `imull $246` below —
# that is the remainder, computed in one multiply-add.
# =============================================================================

	.file	"demo.c"
	.text
	.globl	u64_to_dec              # export: extern linkage so it is emitted
	.p2align	4               # 16-byte align the entry (fetch efficiency)
	.type	u64_to_dec,@function

# -----------------------------------------------------------------------------
# u32 u64_to_dec(u64 v, char *out)
#   SysV ABI in:  rdi = v (the value),  rsi = out (destination buffer)
#             out: eax = number of digits written
#   Clobbers rax, rcx, rdx, r8, r9 (all caller-saved); preserves rbp.
#   tmp[20] scratch lives in the 128-byte "red zone" below rsp — a leaf function
#   may use it without a `sub $N,%rsp`, which is why you see no stack allocation.
# -----------------------------------------------------------------------------
u64_to_dec:
# ---- guard: v == 0 has no digits from the divide loop, handle it separately --
	testq	%rdi, %rdi              # ZF = (v == 0)?  (AND without storing)
	je	.Lzero                  # if v == 0 -> emit a single '0' at the end

# ---- PROLOGUE ---------------------------------------------------------------
	pushq	%rbp                    # save caller's frame pointer
	movq	%rsp, %rbp              # rbp = frame base; tmp[] addressed as -32(%rbp)
	xorl	%ecx, %ecx              # i = 0  (ecx = digit counter / index)
	movabsq	$-3689348814741910323, %r8   # r8 = 0xCCCCCCCCCCCCCCCD, the /10 magic
	                                #   (a 64-bit immediate needs movabs). Hoisted
	                                #   out of the loop: it is loop-invariant.
	.p2align	4
# ---- DIGIT-EXTRACTION LOOP: while (v) { tmp[i++] = '0' + v%10; v /= 10; } -----
.Ldigits:                               # (clang's .LBB0_2)
	movq	%rdi, %rax              # rax = v  (dividend for the wide multiply)
	mulq	%r8                     # rdx:rax = v * magic  (unsigned 128-bit product)
	shrq	$3, %rdx                # rdx = product >> 67 = v / 10 = q
	imull	$246, %edx, %eax        # eax = q * 246 ; 246 ≡ -10 (mod 256)
	addl	%edi, %eax              # eax = q*246 + v ; low byte ≡ v - 10q = r
	addb	$48, %al                # al = r + '0' (0x30) -> the ASCII digit byte
	movl	%ecx, %r9d              # r9 = i (zero-extended index for the store)
	incl	%ecx                    # i++
	movb	%al, -32(%rbp,%r9)      # tmp[i] = digit  (store one byte on the stack)
	cmpq	$10, %rdi               # compare the CURRENT v with 10 (sets flags)...
	movq	%rdx, %rdi              # ...then advance v = q (does not touch flags)
	jae	.Ldigits                # if old v >= 10 there are more digits -> loop
	                                #   (when old v was a single digit, q==0 and we fall through)

# ---- prepare to reverse tmp[0..i) into out[0..i) ----------------------------
	movl	%ecx, %eax              # eax = i (digit count)
	subl	$1, %eax                # eax = i-1 = index of the most-significant digit
	jb	.Lret                   # if i was 0 (unsigned borrow) skip — unreachable
	                                #   on this path (we had at least one digit)
	movl	%ecx, %edx              # edx = i = loop bound (number of digits)
	xorl	%edi, %edi              # j = 0 ; rdi is now reused as the output index
	.p2align	4
# ---- REVERSE LOOP: for (j=0; j<i; j++) out[j] = tmp[i-1-j]; -------------------
.Lreverse:                              # (clang's .LBB0_5)
	movl	%eax, %r8d              # r8 = source index (i-1-j), counts DOWN
	movzbl	-32(%rbp,%r8), %r8d     # r8 = tmp[src], zero-extended byte -> dword
	movb	%r8b, (%rsi,%rdi)       # out[j] = that byte  (rsi = out base, rdi = j)
	incq	%rdi                    # j++
	decl	%eax                    # src--  (walk tmp high -> low)
	cmpq	%rdi, %rdx              # j == count ?
	jne	.Lreverse               # loop until every digit is copied out

# ---- EPILOGUE ---------------------------------------------------------------
.Lret:                                  # (clang's .LBB0_6)
	popq	%rbp                    # restore caller's frame pointer
	movl	%ecx, %eax              # return value = i (number of digits)
	retq

# ---- v == 0 special case ----------------------------------------------------
.Lzero:                                 # (clang's .LBB0_7) — note: no frame set up,
	movb	$48, (%rsi)             #   out[0] = '0'  (0x30). This path never pushed
	movl	$1, %eax                #   return 1      rbp, so there is nothing to pop.
	retq
.Lfunc_end0:
	.size	u64_to_dec, .Lfunc_end0-u64_to_dec

# =============================================================================
# u32 i64_to_dec(i64 v, char *out)   —  signed wrapper around the same loop.
#   in:  rdi = v (signed),  rsi = out      out: eax = digits written (incl. '-')
#
# clang expanded this into THREE code paths and, in each, INLINED the exact
# digit+reverse loops annotated above rather than calling u64_to_dec:
#     * v  > 0 : convert straight (LBB1_9 / LBB1_12)   — identical to u64_to_dec
#     * v == 0 : emit '0'                              (LBB1_14)
#     * v  < 0 : store '-', negate, convert into out+1 (LBB1_2 / LBB1_5), +1 count
# The inlined loops are byte-for-byte the ones above; comments there apply, so
# below we annotate only the sign/zero scaffolding and the negative path's
# distinguishing instructions.
# =============================================================================
	.globl	i64_to_dec
	.p2align	4
	.type	i64_to_dec,@function
i64_to_dec:
	pushq	%rbp                    # PROLOGUE (shared by all three paths)
	movq	%rsp, %rbp
	testq	%rdi, %rdi              # inspect the sign of v
	js	.Lneg                   # SF set  -> v < 0  : negative path
	je	.Lizero                 # ZF set  -> v == 0 : zero path
# ---- positive path: identical digit loop, then reverse, then return ---------
	xorl	%ecx, %ecx              # i = 0
	movabsq	$-3689348814741910323, %r8   # same /10 magic constant
	.p2align	4
.Lpos_digits:                           # == .Ldigits above (unsigned convert of v)
	movq	%rdi, %rax
	mulq	%r8
	shrq	$3, %rdx
	imull	$246, %edx, %eax        # remainder via 246 ≡ -10 (mod 256)
	addl	%edi, %eax
	addb	$48, %al
	movl	%ecx, %r9d
	incl	%ecx
	movb	%al, -32(%rbp,%r9)
	cmpq	$10, %rdi
	movq	%rdx, %rdi
	jae	.Lpos_digits
	movl	%ecx, %eax
	subl	$1, %eax
	jb	.Lidone                 # (unreachable: at least one digit)
	movl	%ecx, %edx
	xorl	%edi, %edi
	.p2align	4
.Lpos_rev:                              # == .Lreverse above
	movl	%eax, %r8d
	movzbl	-32(%rbp,%r8), %r8d
	movb	%r8b, (%rsi,%rdi)
	incq	%rdi
	decl	%eax
	cmpq	%rdi, %rdx
	jne	.Lpos_rev
	jmp	.Lidone                 # done -> shared return

# ---- negative path ----------------------------------------------------------
.Lneg:                                  # (clang's .LBB1_1)
	movb	$45, (%rsi)             # out[0] = '-' (0x2D): write the sign first
	negq	%rdi                    # v = -v : magnitude. Well-defined here because
	                                #   the C computed -(u64)v; INT64_MIN would wrap
	                                #   to itself, still the correct bit pattern.
	xorl	%ecx, %ecx              # i = 0
	movabsq	$-3689348814741910323, %r8   # /10 magic again
	.p2align	4
.Lneg_digits:                           # == .Ldigits, converting the magnitude
	movq	%rdi, %rax
	mulq	%r8
	shrq	$3, %rdx
	imull	$246, %edx, %eax
	addl	%edi, %eax
	addb	$48, %al
	movl	%ecx, %r9d
	incl	%ecx
	movb	%al, -32(%rbp,%r9)
	cmpq	$10, %rdi
	movq	%rdx, %rdi
	jae	.Lneg_digits
	movl	%ecx, %eax
	subl	$1, %eax
	jb	.Lneg_fix
	movl	%ecx, %edx
	xorl	%edi, %edi
	.p2align	4
.Lneg_rev:                              # == .Lreverse, but destination is offset...
	movl	%eax, %r8d
	movzbl	-32(%rbp,%r8), %r8d
	movb	%r8b, 1(%rsi,%rdi)      # out[1 + j] : digits go AFTER the '-' at out[0]
	incq	%rdi
	decl	%eax
	cmpq	%rdi, %rdx
	jne	.Lneg_rev
.Lneg_fix:                              # (clang's .LBB1_6)
	incl	%ecx                    # count += 1 to include the '-' we wrote
	jmp	.Lidone

# ---- v == 0 --------------------------------------------------------------
.Lizero:                                # (clang's .LBB1_14)
	movb	$48, (%rsi)             # out[0] = '0'
	movl	$1, %ecx                # count = 1

# ---- shared return ----------------------------------------------------------
.Lidone:                                # (clang's .LBB1_13)
	movl	%ecx, %eax              # return value = digit count
	popq	%rbp                    # EPILOGUE
	retq
.Lfunc_end1:
	.size	i64_to_dec, .Lfunc_end1-i64_to_dec

	.ident	"clang version 20.1.8"
	.section	".note.GNU-stack","",@progbits   # vote for a NON-executable stack
# =============================================================================
# WHAT TO TAKE AWAY
#   * printf's %u is "repeatedly divide by 10, print the remainders backwards."
#   * A compiler turns constant division into a magic multiply + shift — you
#     will see 0xCCCCCCCCCCCCCCCD every time something divides by 10. Recognize it.
#   * The remainder can fall out of the SAME product via 246 ≡ -10 (mod 256):
#     no second `div` is needed. This is the kind of trick objdump reveals and
#     source never would — which is exactly why we keep the asm open.
# =============================================================================
