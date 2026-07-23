# =============================================================================
# demo.annotated.s — clang's -O1 output for demo.c, explained instruction by
#                     instruction. Baseline source: demo.s (untouched -O1).
# =============================================================================
#
# HOW TO READ THIS FILE
# ---------------------
# AT&T syntax throughout:  op  source, destination     (e.g. movq %rsp,%rbp => rbp=rsp)
#   %reg          a register        $imm      an immediate (literal) constant
#   N(%reg)       memory at [reg+N]           N(%base,%index) => [base+index+N]
#   movabsq       load a full 64-bit immediate into a register
# Register widths are the SAME register: rax(64) / eax(32) / ax(16) / al(8).
# Writing eax ZERO-EXTENDS into rax, so clang prefers `movl $1,%eax` (5 bytes)
# over `movq $1,%rax` (7 bytes) whenever the top 32 bits should be zero.
#
# THE SysV AMD64 ABI CONTRACT (what every function below obeys)
# ------------------------------------------------------------
#   integer/pointer ARGS, in order:  rdi, rsi, rdx, rcx, r8, r9   (then stack)
#   RETURN value:                    rax  (32-bit results in eax)
#   CALLEE-SAVED (we must preserve):  rbx, rbp, r12, r13, r14, r15, rsp
#   CALLER-SAVED (free scratch):      rax, rcx, rdx, rsi, rdi, r8, r9, r10, r11
#   STACK: 16-byte aligned at the point of a `call`. The `call` pushes the
#          8-byte return address, so on entry rsp % 16 == 8; a function that
#          itself calls must re-align (that is what the odd-looking `sub $56`
#          numbers below are partly doing).
#
# THE STAR OF THIS FILE: DIVISION BY 10 WITHOUT A `div`
# ----------------------------------------------------
# u64_to_dec divides by 10 every iteration. A hardware `divq` is ~20-40 cycles.
# The C constant 10 is known at compile time, so clang replaces `v / 10` with a
# MULTIPLY by the magic reciprocal 0xCCCCCCCCCCCCCCCD followed by a shift — the
# textbook "division by invariant integers using multiplication" (Granlund &
# Montgomery). You will see it below as:
#       movabsq $-3689348814741910323, %r8   # = 0xCCCCCCCCCCCCCCCD  (magic)
#       mulq    %r8                           # rdx:rax = v * magic  (128-bit)
#       shrq    $3, %rdx                      # rdx = high64 >> 3    = v / 10
# and the remainder v%10 is then reconstructed as v - (v/10)*10 with two LEAs.
# Compare:  demo.O0.s uses a literal `divq` here;  demo.O2.s keeps the magic
# multiply AND vectorizes the digit-reversal loop with SSE (punpcklbw/packuswb).
# =============================================================================

	.file	"demo.c"
	.text

# =============================================================================
# unsigned u64_to_dec(u64 v /*rdi*/, char *out /*rsi*/)   -> length in eax
# -----------------------------------------------------------------------------
# Convert v to decimal ASCII in `out`; return the digit count. Digits are
# produced least-significant-first into a stack temp, then reversed into out[].
# This is the exact core beneath every "%llu" the kernel module prints.
# =============================================================================
	.globl	u64_to_dec
	.p2align	4
	.type	u64_to_dec,@function
u64_to_dec:
# %bb.0:  entry — handle the v==0 special case before setting up a frame.
	testq	%rdi, %rdi              # set flags from v (rdi). testq r,r == "is v 0?"
	je	.LBB0_8                 # if v==0 jump to the "emit a single '0'" tail

# %bb.1:  v != 0 — establish a frame; the digit temp lives below rbp.
	pushq	%rbp                    # PROLOGUE: save caller's frame pointer
	movq	%rsp, %rbp              # rbp = frame base; tmp[] is at -32(%rbp)..
	xorl	%ecx, %ecx              # ecx = 0  = n, the digit counter (index into tmp)
	movabsq	$-3689348814741910323, %r8  # r8 = 0xCCCCCCCCCCCCCCCD = magic 1/10
	.p2align	4

# ---- digit-extraction loop:  while (v != 0) { tmp[n++]='0'+v%10; v/=10; } -----
.LBB0_2:                                # label: .LBB0_2 == "digit_loop"
	movq	%rdi, %rax              # rax = v  (mulq multiplies rax by its operand)
	mulq	%r8                     # rdx:rax = v * magic  (unsigned 128-bit product)
	shrq	$3, %rdx                # rdx = (v*magic) >> 67 effectively = v / 10
	                                #   (the >>64 is implicit in taking rdx; +3 more)
	leal	(%rdx,%rdx), %eax       # eax = (v/10)*2                 } compute
	leal	(%rax,%rax,4), %eax     # eax = ((v/10)*2)*5 = (v/10)*10 } quotient*10
	movl	%edi, %r9d              # r9d = low 32 bits of v (the digit fits in 32b)
	subl	%eax, %r9d              # r9d = v - (v/10)*10 = v % 10  (the digit 0..9)
	orb	$48, %r9b               # r9b |= 0x30 : ASCII-ify. '0'==48; digits are
	                                #   <10 so OR and ADD are equivalent, OR is 1 byte
	movl	%ecx, %eax              # eax = n (current write index, zero-extended)
	incl	%ecx                    # n++  (post-increment: index used, then bumped)
	movb	%r9b, -32(%rbp,%rax)    # tmp[n] = the ASCII digit   (store 1 byte)
	cmpq	$10, %rdi               # compare OLD v with 10 to decide loop exit
	movq	%rdx, %rdi              # v = v / 10   (advance for next iteration)
	jae	.LBB0_2                 # if old v >= 10 there are more digits -> loop
	                                #   (clang tests before overwriting; classic
	                                #    do-while shape — the C `while` ran once
	                                #    already because we entered with v!=0)

# %bb.3:  digits done; n is in ecx. Prepare to reverse tmp[] into out[].
	movl	%ecx, %edx              # edx = n  (will walk DOWN: source index)
	movl	%ecx, %eax              # eax = n  (this is also the RETURN value)
	subl	$1, %edx                # edx = n-1 = index of last (most-significant)
	                                #   digit in tmp; sets CF if n==0 (can't here)
	jb	.LBB0_6                 # if n-1 underflowed (n==0) skip reverse (dead
	                                #   path here since v!=0 guarantees n>=1)

# %bb.4:  set up the reverse loop counter.
	xorl	%edi, %edi              # edi = 0 = i, the destination index into out[]
	.p2align	4

# ---- reverse loop:  for (i=0;i<n;i++) out[i] = tmp[n-1-i]; ---------------------
.LBB0_5:                                # label: .LBB0_5 == "reverse_loop"
	movl	%edx, %r8d              # r8d = current source index (n-1, n-2, ...)
	movzbl	-32(%rbp,%r8), %r8d     # r8d = tmp[src]  (zero-extend the byte)
	movb	%r8b, (%rsi,%rdi)       # out[i] = that byte  (rsi = out base, rdi = i)
	incq	%rdi                    # i++
	decl	%edx                    # src--  (walk tmp from the top down)
	cmpq	%rdi, %rax              # compare n (rax) with i (rdi)
	jne	.LBB0_5                 # loop until i == n

.LBB0_6:                                # label: "epilogue_nonzero"
	popq	%rbp                    # EPILOGUE: restore caller's frame pointer
	jmp	.LBB0_7                 # jump to the shared NUL-terminate + return tail

# ---- v==0 fast path: no frame was set up, write "0" directly ------------------
.LBB0_8:                                # label: "zero_case"
	movb	$48, (%rsi)             # out[0] = '0'
	movl	$1, %ecx                # n = 1  (unused past here, kept for parity)
	movl	$1, %eax                # rax = 1 = length to return AND the NUL offset

# ---- shared tail: out[len] = '\0'; return len --------------------------------
.LBB0_7:                                # label: "nul_terminate_and_return"
	movb	$0, (%rsi,%rax)         # out[len] = '\0'  (rax holds len == n)
	movl	%ecx, %eax              # return value = n  (eax); for the zero case
	                                #   ecx==1==rax already, so this is consistent
	retq                            # return; result (digit count) is in eax
.Lfunc_end0:
	.size	u64_to_dec, .Lfunc_end0-u64_to_dec

# =============================================================================
# unsigned format_field(u64 v /*rdi*/, unsigned width /*esi*/, char *out /*rdx*/)
# -----------------------------------------------------------------------------
# Right-justify v's decimal form in a width-wide column (like "%20llu"), padding
# on the LEFT with spaces. u64_to_dec was INLINED here (same magic-multiply loop
# reappears), and the pad fill uses a `memset` call, so this function is a
# non-leaf: it saves callee-saved regs and re-aligns the stack.
# =============================================================================
	.globl	format_field
	.p2align	4
	.type	format_field,@function
format_field:
# %bb.0:  prologue — this function CALLS memset, so it is a non-leaf.
	pushq	%rbp                    # save frame pointer
	movq	%rsp, %rbp              # establish frame
	pushq	%r15                    # save callee-saved regs we will use as
	pushq	%r14                    #   long-lived locals across the memset call
	pushq	%rbx                    #   (caller-saved regs would be clobbered by it)
	subq	$56, %rsp               # reserve locals: digits[] at -48(%rbp), the
	                                #   inlined u64_to_dec temp at -80(%rbp); the
	                                #   size also restores 16-byte alignment for
	                                #   the upcoming `call memset`.
	movq	%rdx, %rbx              # rbx = out  (preserved across memset in a
	                                #   callee-saved reg — memset may trash rdx)
	movl	%esi, %ecx              # ecx = width (stash; esi is caller-saved and
	                                #   memset's 2nd arg, so move it out of the way)
	testq	%rdi, %rdi              # v == 0 ?
	je	.LBB1_14                # yes -> inlined "emit '0'" path

# %bb.1..3:  INLINED u64_to_dec digit loop -> writes into digits temp at -80(%rbp)
# (identical shape to u64_to_dec above; comments abbreviated)
	xorl	%r14d, %r14d            # r14d = 0 = ndigits
	movabsq	$-3689348814741910323, %rsi  # rsi = magic 1/10 constant
	.p2align	4
.LBB1_2:                                # "inlined_digit_loop"
	movq	%rdi, %rax              # rax = v
	mulq	%rsi                    # rdx:rax = v * magic
	shrq	$3, %rdx                # rdx = v / 10
	leal	(%rdx,%rdx), %eax       # \
	leal	(%rax,%rax,4), %eax     #  } eax = (v/10)*10
	movl	%edi, %r8d              # r8d = v (low 32)
	subl	%eax, %r8d              # r8d = v % 10
	orb	$48, %r8b               # ASCII-ify
	movl	%r14d, %eax             # eax = ndigits
	incl	%r14d                   # ndigits++
	movb	%r8b, -80(%rbp,%rax)    # tmp[ndigits] = digit
	cmpq	$10, %rdi               # more digits?
	movq	%rdx, %rdi              # v /= 10
	jae	.LBB1_2                 # loop
# %bb.3:
	movl	%r14d, %edx             # edx = ndigits (source cursor)
	movl	%r14d, %eax             # eax = ndigits
	subl	$1, %edx                # edx = ndigits-1
	jb	.LBB1_6                 # (n==0 guard; unreachable when v!=0)
# %bb.4..5:  INLINED reversal -> digits[] at -48(%rbp)
	xorl	%esi, %esi              # esi = 0 = i
	.p2align	4
.LBB1_5:                                # "inlined_reverse_loop"
	movl	%edx, %edi              # edi = src index
	movzbl	-80(%rbp,%rdi), %edi    # load tmp[src]
	movb	%dil, -48(%rbp,%rsi)    # digits[i] = tmp[src]
	incq	%rsi                    # i++
	decl	%edx                    # src--
	cmpq	%rsi, %rax              # i < ndigits ?
	jne	.LBB1_5                 # loop
	jmp	.LBB1_6                 # fall through to padding math
.LBB1_14:                               # "inlined_zero_case"
	movb	$48, -48(%rbp)          # digits[0] = '0'
	movl	$1, %r14d               # ndigits = 1
	movl	$1, %eax                # length-so-far = 1

# ---- pad = (width > ndigits) ? width-ndigits : 0;  then memset spaces ----------
.LBB1_6:                                # "compute_pad"
	movb	$0, -48(%rbp,%rax)      # NUL-terminate the local digits[] string
	xorl	%eax, %eax              # eax = 0 : running output position `pos`
	movl	%ecx, %r15d             # r15d = width
	subl	%r14d, %r15d            # r15d = width - ndigits  (may underflow...)
	cmovbl	%eax, %r15d             # ...if it borrowed (width<ndigits), pad = 0.
	                                #   cmovb = conditional move if CF: the branch-
	                                #   free encoding of the C ternary. THIS is why
	                                #   the C wrote `(width>ndigits)?...:0` — to keep
	                                #   the unsigned subtraction from wrapping huge.
	jbe	.LBB1_9                 # if width<=ndigits there is no padding to write
# %bb.7:  pad > 0 — fill `pad` spaces at out[0..] via memset(out, ' ', pad)
	movl	%r14d, %eax             # eax = ndigits
	notl	%eax                    # eax = ~ndigits = -ndigits-1
	addl	%eax, %ecx              # ecx = width - ndigits - 1
	incq	%rcx                    # rcx = width - ndigits = pad  (the memset count)
	movq	%rbx, %rdi              # arg0 = out            (rbx preserved earlier)
	movl	$32, %esi               # arg1 = 32 = ' '       (fill byte)
	movq	%rcx, %rdx              # arg2 = pad             (byte count)
	callq	memset@PLT              # out[0..pad) = spaces. @PLT: resolved lazily via
	                                #   the Procedure Linkage Table (dynamic link).
	xorl	%eax, %eax              # pos = 0, recomputed as an index below
	.p2align	4
.LBB1_8:                                # "count_pad" — recompute pos = pad
	incl	%eax                    # pos++
	cmpl	%eax, %r15d             # until pos == pad (r15d)
	ja	.LBB1_8                 # (a tiny loop the optimizer left to derive pos;
	                                #   it mirrors the C `for` that wrote spaces)
.LBB1_9:                                # "copy_digits" — append the digit chars
	testl	%r14d, %r14d            # ndigits == 0 ? (never, but guarded)
	je	.LBB1_13
# %bb.10:
	movl	%r14d, %ecx             # ecx = ndigits (loop trip count)
	movl	%eax, %esi              # esi = pos (destination base offset = pad)
	xorl	%edx, %edx              # edx = 0 = j (index into digits[])
	.p2align	4
.LBB1_11:                               # "copy_loop": out[pad+j] = digits[j]
	leal	(%rsi,%rdx), %edi       # edi = pos + j  (destination index)
	movzbl	-48(%rbp,%rdx), %r8d    # r8d = digits[j]
	movb	%r8b, (%rbx,%rdi)       # out[pos+j] = digit
	incq	%rdx                    # j++
	cmpq	%rdx, %rcx              # j < ndigits ?
	jne	.LBB1_11                # loop
# %bb.12:
	addl	%edx, %eax              # pos += ndigits  (total chars written)
.LBB1_13:                               # "finish"
	movl	%eax, %ecx              # ecx = pos
	movb	$0, (%rbx,%rcx)         # out[pos] = '\0'
	addq	$56, %rsp               # EPILOGUE: free locals
	popq	%rbx                    # restore callee-saved registers in reverse
	popq	%r14                    #   order of the pushes
	popq	%r15
	popq	%rbp
	retq                            # return pos in eax
.Lfunc_end1:
	.size	format_field, .Lfunc_end1-format_field

# =============================================================================
# u32 checksum8(const unsigned char *buf /*rdi*/, u32 len /*esi*/) -> eax
# -----------------------------------------------------------------------------
# sum of bytes, masked to 8 bits. A deliberately trivial accumulate loop, here
# to contrast with the division trick: at -O2 clang VECTORIZES this loop with
# SSE (see demo.O2.s: pxor/movdqu/paddw). At -O1 it stays a simple scalar loop,
# which is the easiest possible thing to read.
# =============================================================================
	.globl	checksum8
	.p2align	4
	.type	checksum8,@function
checksum8:
# %bb.0:  a leaf function (no calls), but -O1 still keeps a frame pointer.
	pushq	%rbp                    # save frame pointer
	movq	%rsp, %rbp              # establish frame
	testl	%esi, %esi              # len == 0 ?
	je	.LBB2_1                 # empty buffer -> return 0
# %bb.2:  set up the accumulate loop.
	movl	%esi, %eax              # eax = len  (used as the loop bound)
	xorl	%ecx, %ecx              # ecx = 0 = i (byte index)
	xorl	%edx, %edx              # edx = 0 = sum accumulator
	.p2align	4
.LBB2_3:                                # "sum_loop": sum += buf[i];
	movzbl	(%rdi,%rcx), %esi       # esi = buf[i]  (zero-extend one byte to 32b)
	addl	%esi, %edx              # sum += buf[i]
	incq	%rcx                    # i++
	cmpq	%rcx, %rax              # i < len ?
	jne	.LBB2_3                 # loop
# %bb.4:  mask to 8 bits and return.
	movzbl	%dl, %eax               # eax = sum & 0xFF  (movzbl of the low byte dl
	                                #   is how the compiler realizes `& 0xffu`)
	popq	%rbp                    # restore frame pointer
	retq                            # return the byte checksum in eax
.LBB2_1:                                # "empty": len==0
	xorl	%eax, %eax              # return 0
	popq	%rbp
	retq
.Lfunc_end2:
	.size	checksum8, .Lfunc_end2-checksum8

# =============================================================================
# unsigned render_line(u32 idx /*edi*/, u64 value /*rsi*/, char *out /*rdx*/)
# -----------------------------------------------------------------------------
# Compose one "/proc row": u64_to_dec(idx) + ' ' + format_field(value,20).
# BOTH helpers were inlined, so you see the magic-multiply digit loop TWICE
# (once for idx, once for value) and the memset padding once — the clearest
# demonstration in this file of what "inlining" buys and costs. Reading this and
# diffing it against demo.O0.s (where these are real `call`s) is the exercise.
# =============================================================================
	.globl	render_line
	.p2align	4
	.type	render_line,@function
render_line:
# %bb.0:  prologue — non-leaf (calls memset), saves several callee-saved regs.
	pushq	%rbp
	movq	%rsp, %rbp
	pushq	%r15                    # r15 = pad count, preserved across memset
	pushq	%r14                    # r14 = length of the idx part (running total)
	pushq	%r12                    # r12 = ndigits of the value part
	pushq	%rbx                    # rbx = out pointer (preserved across memset)
	subq	$64, %rsp               # locals: two digit temps at -64 and -96(%rbp)
	movq	%rdx, %rbx              # rbx = out
	movabsq	$-3689348814741910323, %r8   # r8 = magic 1/10, reused by BOTH loops
	testl	%edi, %edi              # idx == 0 ?
	je	.LBB3_19                # -> emit '0' for the index

# ---- FIRST inlined u64_to_dec: format idx into out[] directly ------------------
	movl	%edi, %ecx              # ecx = idx (working copy)
	xorl	%r14d, %r14d            # r14d = 0 = idx digit count
	.p2align	4
.LBB3_2:                                # "idx_digit_loop"
	movq	%rcx, %rax              # rax = idx
	mulq	%r8                     # rdx:rax = idx * magic
	shrq	$3, %rdx                # rdx = idx / 10
	leal	(%rdx,%rdx), %eax       # \
	leal	(%rax,%rax,4), %eax     #  } (idx/10)*10
	movl	%ecx, %edi              # edi = idx
	subl	%eax, %edi              # edi = idx % 10
	orb	$48, %dil               # ASCII-ify
	movl	%r14d, %eax             # eax = count
	incl	%r14d                   # count++
	movb	%dil, -64(%rbp,%rax)    # idxtmp[count] = digit
	cmpq	$10, %rcx               # more?
	movq	%rdx, %rcx              # idx /= 10
	jae	.LBB3_2
# %bb.3..5:  reverse idxtmp into out[]
	movl	%r14d, %ecx
	movl	%r14d, %eax
	subl	$1, %ecx
	jb	.LBB3_6
	xorl	%edx, %edx              # i = 0
	.p2align	4
.LBB3_5:                                # "idx_reverse_loop": out[i] = idxtmp[n-1-i]
	movl	%ecx, %edi
	movzbl	-64(%rbp,%rdi), %edi
	movb	%dil, (%rbx,%rdx)       # write straight into the caller's out[]
	incq	%rdx
	decl	%ecx
	cmpq	%rdx, %rax
	jne	.LBB3_5
	jmp	.LBB3_6
.LBB3_19:                               # idx == 0
	movb	$48, (%rbx)             # out[0] = '0'
	movl	$1, %r14d               # count = 1
	movl	$1, %eax

# ---- write the single separating space:  out[pos++] = ' ' ---------------------
.LBB3_6:                                # "after_idx"
	movb	$0, (%rbx,%rax)         # (transient NUL from the inlined helper)
	movl	%r14d, %eax             # eax = idx length
	incl	%r14d                   # r14 now counts idx length + 1 (the space)
	movb	$32, (%rbx,%rax)        # out[idxlen] = ' '   (the C `out[pos++]=' '`)
	testq	%rsi, %rsi              # value == 0 ?
	je	.LBB3_20

# ---- SECOND inlined u64_to_dec: format `value` into a temp at -96(%rbp) --------
	xorl	%r12d, %r12d            # r12d = 0 = value digit count
	.p2align	4
.LBB3_8:                                # "value_digit_loop"
	movq	%rsi, %rax              # rax = value
	mulq	%r8                     # rdx:rax = value * magic  (same r8 magic const)
	shrq	$3, %rdx                # rdx = value / 10
	leal	(%rdx,%rdx), %eax       # \
	leal	(%rax,%rax,4), %eax     #  } (value/10)*10
	movl	%esi, %ecx              # ecx = value
	subl	%eax, %ecx              # ecx = value % 10
	orb	$48, %cl                # ASCII-ify
	movl	%r12d, %eax             # eax = count
	incl	%r12d                   # count++
	movb	%cl, -96(%rbp,%rax)     # valtmp[count] = digit
	cmpq	$10, %rsi               # more?
	movq	%rdx, %rsi              # value /= 10
	jae	.LBB3_8
# %bb.9..11:  reverse valtmp into the scratch at -64(%rbp) (reused idxtmp space)
	movl	%r12d, %ecx
	movl	%r12d, %eax
	subl	$1, %ecx
	jb	.LBB3_12
	xorl	%edx, %edx
	.p2align	4
.LBB3_11:                               # "value_reverse_loop"
	movl	%ecx, %esi
	movzbl	-96(%rbp,%rsi), %esi
	movb	%sil, -64(%rbp,%rdx)    # digits[] (the field to right-justify)
	incq	%rdx
	decl	%ecx
	cmpq	%rdx, %rax
	jne	.LBB3_11
	jmp	.LBB3_12
.LBB3_20:                               # value == 0
	movb	$48, -64(%rbp)          # digits[0] = '0'
	movl	$1, %r12d               # count = 1
	movl	$1, %eax

# ---- format_field body: pad `value` to width 20, then copy after the space -----
.LBB3_12:                               # "field_pad"
	addq	%r14, %rbx              # out += (idxlen+1): advance past "idx " so the
	                                #   value column is written at the right offset
	movb	$0, -64(%rbp,%rax)      # NUL-terminate the local digits string
	xorl	%r15d, %r15d            # pad = 0
	cmpl	$19, %r12d              # value digits > 19  (i.e. >= width 20)?
	ja	.LBB3_14                # then no padding (field already full)
# %bb.13:  pad = 20 - ndigits; memset that many spaces
	movl	$20, %r15d              # r15d = 20 (the column width, from the C literal)
	subl	%r12d, %r15d            # r15d = 20 - ndigits = pad
	movq	%rbx, %rdi              # arg0 = out (already advanced past "idx ")
	movl	$32, %esi               # arg1 = ' '
	movq	%r15, %rdx              # arg2 = pad
	callq	memset@PLT              # write the leading spaces
.LBB3_14:                               # "field_copy"
	testl	%r12d, %r12d            # ndigits == 0 ?
	je	.LBB3_18
# %bb.15:
	movl	%r12d, %eax             # trip count = ndigits
	movl	%r15d, %edx             # base = pad
	xorl	%ecx, %ecx              # j = 0
	.p2align	4
.LBB3_16:                               # "field_copy_loop": out[pad+j]=digits[j]
	leal	(%rdx,%rcx), %esi       # esi = pad + j
	movzbl	-64(%rbp,%rcx), %edi    # digits[j]
	movb	%dil, (%rbx,%rsi)       # store into the value column
	incq	%rcx                    # j++
	cmpq	%rcx, %rax              # j < ndigits ?
	jne	.LBB3_16
# %bb.17:
	addl	%ecx, %r15d             # pad += ndigits  => field length written
.LBB3_18:                               # "done"
	movl	%r15d, %eax             # eax = field length
	movb	$0, (%rbx,%rax)         # NUL-terminate the whole line
	addl	%r15d, %r14d            # total = (idxlen+1) + fieldlen
	movl	%r14d, %eax             # return total length in eax
	addq	$64, %rsp               # EPILOGUE: free locals
	popq	%rbx                    # restore callee-saved regs (reverse order)
	popq	%r12
	popq	%r14
	popq	%r15
	popq	%rbp
	retq
.Lfunc_end3:
	.size	render_line, .Lfunc_end3-render_line

	.ident	"clang version 20.1.8"
	.section	".note.GNU-stack","",@progbits  # non-executable stack: a
	                                                #   security default the linker
	                                                #   records (W^X for the stack)
# =============================================================================
# WHAT TO TAKE AWAY
#   * Division by a compile-time constant becomes a MULTIPLY by a magic
#     reciprocal (0xCCCCCCCCCCCCCCCD) + shift — never a `div`. That is the one
#     line worth memorizing:  x/10  ==  (x * 0xCCC...CCD) >> 67.
#   * `x % 10` is then just  x - (x/10)*10, built from two `lea`s (no `mul`).
#   * The SysV ABI is visible in every prologue: args arrive in rdi/rsi/rdx,
#     results leave in eax/rax, and any function that CALLS another (memset here)
#     must save callee-saved regs (rbx/r14/r15) and re-align rsp to 16 bytes.
#   * Inlining is real: render_line contains the digit loop TWICE with zero
#     `call`s to u64_to_dec/format_field. Diff against demo.O0.s to see the same
#     code as honest function calls, and against demo.O2.s to watch the reversal
#     loop turn into SSE shuffles (punpcklbw/pshufd/packuswb).
# =============================================================================
