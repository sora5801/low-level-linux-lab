# =============================================================================
# demo.annotated.s — clang -O1 output for demo.c (wc_count), explained.
# =============================================================================
#
# HOW TO READ THIS FILE
# ---------------------
# This is the exact assembly clang emits for asm/demo.c at -O1 (see demo.s for
# the untouched original), with a comment on essentially every instruction.
# AT&T syntax throughout:
#
#     op   source, destination          # movl $1, %eax   =>   eax = 1
#     %reg                               # a register
#     $imm                               # an immediate (literal) constant
#     N(%base,%index)                    # memory at [base + index + N]
#
# Register widths are the SAME register: rax(64) / eax(32) / ax(16) / al(8).
# Writing eax ZERO-EXTENDS into rax, so `xorl %r9d,%r9d` clears all 64 bits of
# r9 in 3 bytes — the idiomatic "set to 0".
#
# THE SYSTEM V AMD64 ABI (what this function obeys)
# -------------------------------------------------
#   * integer/pointer ARGS, in order:  rdi, rsi, rdx, rcx, r8, r9   (then stack)
#       - here:  rdi = st (struct wc_state*),  rsi = buf,  rdx = n
#   * RETURN value:                    rax   (this function returns void)
#   * CALLEE-SAVED (must be preserved): rbx, rbp, r12, r13, r14, r15, rsp
#       - the prologue pushes every one it uses and the epilogue pops them.
#   * CALLER-SAVED (scratch):          rax, rcx, rdx, rsi, rdi, r8, r9, r10, r11
#   * the RED ZONE: 128 bytes below rsp a LEAF function may use without moving
#       rsp. wc_count calls nothing (wc_is_space was inlined), so it is a leaf
#       and parks its one spill slot at -48(%rbp) — below rsp — for free.
#   * STACK ALIGNMENT: rsp must be 16-byte aligned at a `call`. Irrelevant here
#       (no calls), which is exactly why there is no `sub $N,%rsp`.
#
# THE ALGORITHM (mirrors wc.c's wc_count)
# ---------------------------------------
# For each byte c of buf[0..n): lines += (c=='\n'); chars += (c is not a UTF-8
# continuation byte 10xxxxxx); and a word-boundary state machine bumps `words`
# on every transition from whitespace into non-whitespace. `in_word` is loaded
# from st at entry and stored back at exit so words spanning chunk boundaries
# count once. The compiler keeps every running total in a register and touches
# memory only twice: the initial load of in_word and the final write-back.
#
# REGISTER MAP the optimizer chose for the hot loop
# -------------------------------------------------
#   rdi -> st (spilled to -48(%rbp); reloaded once at the end)
#   rsi -> buf          rdx -> n            r11 -> i (loop counter)
#   eax -> in_word (carried)                r10b -> c (current byte)
#   r9  -> lines        rcx -> chars        r8  -> words
#   r13b-> (c=='\n')    r12b-> (c is not a continuation byte)   bl -> (c is NOT space)
# =============================================================================

	.file	"demo.c"
	.text
	.globl	wc_count                        # export wc_count for the linker
	.p2align	4                       # 16-byte-align the entry (fetch friendly)
	.type	wc_count,@function
wc_count:                               # void wc_count(st=rdi, buf=rsi, n=rdx)
# %bb.0:  ---- PROLOGUE ---------------------------------------------------------
	pushq	%rbp                            # save caller's frame pointer...
	movq	%rsp, %rbp                      # ...and establish our own frame base
	pushq	%r15                            # preserve the 5 callee-saved regs we
	pushq	%r14                            #   are about to use as loop state
	pushq	%r13                            #   (r15,r14,r13,r12,rbx). The ABI
	pushq	%r12                            #   promises the caller they survive.
	pushq	%rbx
	movq	%rdi, -48(%rbp)                 # SPILL st to the red zone: the loop
                                        #   needs rdi as a scratch temp (dil),
                                        #   so stash the pointer and reload later.
	movl	32(%rdi), %eax                  # eax = st->in_word  (offset 32: the
                                        #   struct is lines,words,chars,bytes at
                                        #   0,8,16,24 — four u64 — then int@32)
	testq	%rdx, %rdx                      # n == 0 ?
	je	.LBB0_1                         #   if so, skip the loop (empty-chunk path)

# %bb.3:  ---- LOOP SET-UP (n > 0) -----------------------------------------------
	xorl	%r9d, %r9d                      # lines = 0
	xorl	%r11d, %r11d                    # i     = 0   (the loop index)
	xorl	%ecx, %ecx                      # chars = 0
	xorl	%r8d, %r8d                      # words = 0
	jmp	.LBB0_4                         # enter the loop at its header
	.p2align	4                       # align the hot loop's back-edge target

# ---- LOOP TAIL / ACCUMULATE  (label .LBB0_7) ---------------------------------
# Reached from the loop header once c's three flags (r13b/r12b/bl) are ready.
# Folds this byte into lines/chars/words and updates in_word, then loops.
.LBB0_7:                                #   in Loop: Header=BB0_4 Depth=1
	movb	%r13b, %r15b                    # r15b = (c=='\n')  [r15 was zeroed]
	addq	%r15, %r9                       # lines += newline flag
	movb	%r12b, %r14b                    # r14b = (c is not a continuation byte)
	addq	%r14, %rcx                      # chars += that flag
	testl	%eax, %eax                      # in_word == 0 ?
	sete	%dil                            # dil = !in_word   (1 if we were OUT)
	cmpl	$1, %eax                        # set CF = (in_word == 0)
	adcl	$0, %eax                        # eax = in_word + CF  => eax = 1 here.
                                        #   Net effect for in_word∈{0,1}: tentat-
                                        #   ively mark "inside a word" (=1); the
                                        #   cmov below reverts it to 0 for spaces.
	andb	%bl, %dil                       # dil = !in_word & (c is NOT space)
                                        #   == the RISING EDGE: a new word starts
	movzbl	%dil, %edi                      # zero-extend the 0/1 edge flag
	addq	%rdi, %r8                       # words += rising-edge
	testb	%bl, %bl                        # was c a space?  (bl == 0 means space)
	movl	$0, %edi                        # edi = 0  (the "leave the word" value)
	cmovel	%edi, %eax                      # if c was space (ZF): in_word = 0;
                                        #   else keep eax==1 (we are inside a word)
	incq	%r11                            # i++
	cmpq	%r11, %rdx                      # i == n ?
	je	.LBB0_2                         #   if done, go write the totals back

# ---- LOOP HEADER  (label .LBB0_4): classify byte c into three flags ----------
.LBB0_4:                                # =>This Inner Loop Header: Depth=1
	movzbl	(%rsi,%r11), %r10d              # c = buf[i]   (zero-extended byte load)
	xorl	%r15d, %r15d                    # pre-zero the newline temp
	cmpb	$10, %r10b                      # c == '\n' (10) ?
	sete	%r13b                           # r13b = newline flag
	xorl	%r14d, %r14d                    # pre-zero the chars temp
	cmpb	$-64, %r10b                     # compare c with 0xC0 as a SIGNED byte
	setge	%r12b                           # r12b = (c >=s -64). Continuation bytes
                                        #   0x80..0xBF are signed -128..-65 (< -64),
                                        #   so `>= -64` is TRUE for every byte that
                                        #   is NOT a continuation byte — precisely
                                        #   (c & 0xC0) != 0x80.  One compare, no AND.
	xorl	%ebx, %ebx                      # bl = 0  (assume c IS whitespace)
	leal	-9(%r10), %edi                  # edi = c - 9   (LEA as a free subtract)
	cmpl	$5, %edi                        # (unsigned)(c-9) < 5 ?  i.e. c in 9..13
	jb	.LBB0_7                         #   => c is \t\n\v\f\r : whitespace, bl=0
# %bb.5:
	cmpl	$32, %r10d                      # c == ' ' (32) ?
	je	.LBB0_7                         #   => space: whitespace, bl stays 0
# %bb.6:
	movb	$1, %bl                         # c is NON-space: set the in-word flag
	jmp	.LBB0_7                         # fall into the accumulate block

# ---- EMPTY-CHUNK PATH (n == 0): all three loop sums are zero -----------------
.LBB0_1:
	xorl	%r8d, %r8d                      # words = 0
	xorl	%ecx, %ecx                      # chars = 0
	xorl	%r9d, %r9d                      # lines = 0
                                        #   (eax still holds the untouched in_word)

# ---- WRITE-BACK / EPILOGUE  (label .LBB0_2) ----------------------------------
.LBB0_2:
	movq	-48(%rbp), %rsi                 # reload st (we spilled it in prologue)
	addq	%r9,  (%rsi)                    # st->lines += lines   (offset 0)
	addq	%r8,  8(%rsi)                   # st->words += words   (offset 8)
	addq	%rcx, 16(%rsi)                  # st->chars += chars   (offset 16)
	addq	%rdx, 24(%rsi)                  # st->bytes += n       (offset 24)
                                        #   note: bytes just adds the FULL n — no
                                        #   per-byte work, the compiler hoisted it
	movl	%eax, 32(%rsi)                  # st->in_word = in_word (carry out)
	popq	%rbx                            # restore callee-saved regs in exact
	popq	%r12                            #   reverse order of the pushes...
	popq	%r13
	popq	%r14
	popq	%r15
	popq	%rbp                            # ...and the caller's frame pointer
	retq                                    # return (void); rax is dead
.Lfunc_end0:
	.size	wc_count, .Lfunc_end0-wc_count  # record byte length for tools

	.ident	"clang version 20.1.8"          # toolchain stamp (harmless metadata)
	.section	".note.GNU-stack","",@progbits  # we do NOT need an executable
                                                #   stack — a security default
	.addrsig
# =============================================================================
# WHAT TO TAKE AWAY
#   * The whole state machine runs in REGISTERS. Memory is touched exactly
#     twice: `movl 32(%rdi),%eax` to load in_word, and the five `addq/movl`
#     stores at the end. That is what "keep the hot data in registers" looks
#     like in the generated code.
#   * `(c & 0xC0) != 0x80` — the UTF-8 continuation-byte test — became a single
#     SIGNED compare `cmpb $-64; setge`. Recognising that a bit-mask test can be
#     a range test is a classic optimizer move; reading asm is how you catch it.
#   * The `words` increment is branchless: `sete`/`andb`/`addq` turn the
#     "rising edge" boolean into a 0/1 you simply add — no mispredict on random
#     whitespace. Compare with demo.O0.s, where every `if` is a real branch and
#     every variable lives on the stack.
#   * -O2 (demo.O2.s) goes further: it drops the frame pointer, reuses %rbp as a
#     data register (%bpl), and sinks the whole prologue INSIDE the n>0 branch so
#     the empty-chunk path never pushes a single register or spills st — it just
#     zeroes, adds n to st->bytes, and returns.
# =============================================================================
