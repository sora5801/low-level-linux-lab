# =============================================================================
# demo.annotated.s — clang's -O1 output for demo.c, explained line by line.
# =============================================================================
#
# HOW TO READ THIS FILE
# ---------------------
# This is the EXACT assembly clang emits for asm/demo.c at -O1 (see demo.s for
# the untouched original), with a comment on essentially every instruction.
# AT&T syntax throughout:
#
#     op   source, destination          # movl $1, %eax   =>  eax = 1
#     %reg                               # a register
#     $imm                               # an immediate (literal) constant
#     N(%reg)                            # memory at address [reg + N]
#     (%reg)                             # memory at address [reg]  (N == 0)
#
# Register widths are views of ONE physical register:
#     rax(64) / eax(32) / ax(16) / al(8).  Writing a 32-bit name (eax) ZERO-
#     EXTENDS into the full 64-bit register — that is why `xorl %eax,%eax`
#     clears all 64 bits, and why movzwl/movzbl (move-zero-extend word/byte to
#     long) are used to widen a 16-/8-bit load without leaving stale high bits.
#
# THE SysV AMD64 ABI CONTRACT (what these functions rely on)
# ----------------------------------------------------------
#   * Integer/pointer ARGUMENTS, in order:  rdi, rsi, rdx, rcx, r8, r9, then stack.
#       - ip_checksum(const void *data, u32 len):  data -> rdi,  len -> esi.
#       - csum_fold(u32 sum):                      sum  -> edi.
#   * RETURN value:                          rax (here just ax/eax — a u16/int).
#   * CALLER-saved (scratch, a callee may trash): rax, rcx, rdx, rsi, rdi, r8-r11.
#   * CALLEE-saved (a callee must preserve):       rbx, rbp, r12-r15, rsp.
#   * Stack must be 16-byte aligned at a `call`; the `call` pushes the 8-byte
#     return address, so on entry rsp ≡ 8 (mod 16).
#   * The 128-byte "red zone" below rsp is scratch a leaf function may use
#     without adjusting rsp — relevant at -O2, not needed here.
#
# THE BIG PICTURE
# ---------------
# demo.c had three functions: csum_fold, ip_checksum, ip_checksum_valid. At -O1
# clang INLINED the tiny csum_fold into BOTH callers, so there is no `csum_fold`
# symbol here at all (compare demo.O0.s, where it is a real function reached by
# `call`). And in ip_checksum_valid the optimizer pulled off a genuinely clever
# trick — it never computes the final `~sum`; it compares the folded sum against
# 0xFFFF directly. Both are called out inline below. That collapse is the whole
# reason we read assembly: the source's structure is a suggestion, not a
# promise, and the .s file is the ground truth of what the CPU runs.
# =============================================================================

	.file	"demo.c"
	.text                           # executable code section

# =============================================================================
# ip_checksum(const void *data /*rdi*/, u32 len /*esi*/) -> u16 in ax
#
#   Sum 16-bit words into a 32-bit accumulator (eax), add a trailing odd byte,
#   fold the carries, return the ones-complement. The C had a separate csum_fold
#   call; here it is inlined as the second loop (.Lfold).
# =============================================================================
	.globl	ip_checksum
	.p2align	4                       # 16-byte-align the entry for the fetcher
	.type	ip_checksum,@function
ip_checksum:
# ---- PROLOGUE ---------------------------------------------------------------
# A frame pointer is kept only because we compiled with -fno-omit-frame-pointer
# (for legible backtraces); this leaf needs no stack frame otherwise.
	pushq	%rbp                    # save caller's frame pointer (callee-saved)
	movq	%rsp, %rbp              # rbp = base of our frame

# ---- sum = 0; enter the word loop -------------------------------------------
	xorl	%eax, %eax              # eax = 0  -> this is `sum`. xor r,r is the
                                        #   2-byte idiom to zero (and the CPU treats
                                        #   it as a dependency breaker).
	cmpl	$2, %esi                # compare len (esi) with 2...
	jb	.Ltail                  # ...if len < 2 (unsigned Below), no full word
                                        #   remains: skip straight to the odd-byte tail.
	xorl	%eax, %eax              # sum = 0 again (this copy dominates the loop
                                        #   entry the optimizer laid out; harmless).

# ---- word loop: while (len > 1) sum += p[0] | (p[1]<<8) ---------------------
	.p2align	4               # align the hot loop top for the fetch unit
.Lword_loop:                            # (clang's .LBB0_2)
	movzwl	(%rdi), %ecx            # ecx = *(u16*)p, zero-extended to 32 bits.
                                        #   ONE 16-bit load does the job of the C's
                                        #   `p[0] | (p[1]<<8)`: on this little-endian
                                        #   target the two source bytes already sit in
                                        #   low-then-high order, so the compiler proved
                                        #   the byte-assembly equals a single word load.
	addl	%ecx, %eax              # sum += word   (32-bit add: carries accumulate
                                        #   in the high half of eax, never lost)
	addq	$2, %rdi                # p += 2  (advance the byte cursor by one word)
	addl	$-2, %esi               # len -= 2
	cmpl	$1, %esi                # compare len with 1...
	ja	.Lword_loop             # ...loop while len > 1 (unsigned Above). Using
                                        #   `> 1` here matches `while (len > 1)` exactly.

# ---- odd trailing byte: if (len == 1) sum += p[0] ---------------------------
.Ltail:                                 # (clang's .LBB0_3)
	testl	%esi, %esi              # set ZF if len == 0 (test = AND, discards result)
	je	.Lfold                  # if len == 0, nothing left: go fold.
	movzbl	(%rdi), %ecx            # ecx = *(u8*)p, zero-extended (implicit 0 high byte)
	addl	%ecx, %eax              # sum += last byte

# ---- fold: while (sum >> 16) sum = (sum & 0xffff) + (sum >> 16) --------------
# This is the INLINED csum_fold. In demo.O0.s it was a `call csum_fold`; at -O1
# it is these few instructions with no call overhead.
.Lfold:                                 # (clang's .LBB0_5)
	cmpl	$65536, %eax            # is sum >= 0x10000 (any carry in bits 16..31)?
	jb	.Ldone                  # if sum < 0x10000, already folded: done.
	.p2align	4
.Lfold_loop:                            # (clang's .LBB0_6)
	movl	%eax, %ecx              # ecx = sum
	shrl	$16, %ecx               # ecx = sum >> 16   (the high half / carry)
	movzwl	%ax, %eax               # eax = sum & 0xffff (low half; movzwl ax->eax
                                        #   zero-extends the low 16 bits, clearing top)
	addl	%ecx, %eax              # sum = (sum & 0xffff) + (sum >> 16)  <-- the
                                        #   end-around carry that defines a ones-
                                        #   complement sum.
	cmpl	$65535, %eax            # did that add itself carry past 0xFFFF?
	ja	.Lfold_loop             # if sum > 0xFFFF, fold once more (at most twice)

# ---- return (u16)~sum -------------------------------------------------------
.Ldone:                                 # (clang's .LBB0_7)
	notl	%eax                    # eax = ~sum  — the ones' complement IS the
                                        #   checksum you store in the header.
                                        # (only ax is returned; the high bits of eax
                                        #   are ignored by the u16 return type.)
	popq	%rbp                    # EPILOGUE: restore caller's frame pointer
	retq                            # return; result in ax
.Lfunc_end0:
	.size	ip_checksum, .Lfunc_end0-ip_checksum

# =============================================================================
# ip_checksum_valid(const void *data /*rdi*/, u32 len /*esi*/) -> int in eax
#
#   Returns 1 iff the ones-complement sum over the buffer is 0 (i.e. the stored
#   checksum verifies). The body is a COPY of ip_checksum's summation — the
#   compiler inlined ip_checksum too — but watch the ending: it does NOT emit
#   `notl` and compare against 0. Instead it compares the FOLDED sum against
#   0xFFFF, because  (~x == 0)  ==  (x == 0xFFFF).  One instruction saved, and a
#   perfect little illustration of the optimizer reasoning about ones-complement.
# =============================================================================
	.globl	ip_checksum_valid
	.p2align	4
	.type	ip_checksum_valid,@function
ip_checksum_valid:
	pushq	%rbp                    # PROLOGUE (frame pointer kept, as above)
	movq	%rsp, %rbp

	xorl	%ecx, %ecx              # ecx = 0  -> `sum` lives in ecx here (eax is
                                        #   reserved for the boolean return value)
	cmpl	$2, %esi                # len < 2 ?
	jb	.Lv_tail                # if so, skip the word loop
	xorl	%ecx, %ecx              # sum = 0 (loop-entry copy, as before)
	.p2align	4
.Lv_word_loop:                          # (clang's .LBB1_2) — same loop as ip_checksum
	movzwl	(%rdi), %eax            # eax = *(u16*)p  (scratch load; eax reused)
	addl	%eax, %ecx              # sum += word
	addq	$2, %rdi                # p += 2
	addl	$-2, %esi               # len -= 2
	cmpl	$1, %esi
	ja	.Lv_word_loop           # while (len > 1)
.Lv_tail:                               # (clang's .LBB1_3)
	testl	%esi, %esi              # len == 0 ?
	je	.Lv_fold
	movzbl	(%rdi), %eax            # eax = last byte
	addl	%eax, %ecx              # sum += last byte
.Lv_fold:                               # (clang's .LBB1_5) — inlined csum_fold again
	cmpl	$65536, %ecx            # any carry above bit 15?
	jb	.Lv_check
	.p2align	4
.Lv_fold_loop:                          # (clang's .LBB1_6)
	movl	%ecx, %eax              # eax = sum
	shrl	$16, %eax               # eax = sum >> 16
	movzwl	%cx, %ecx               # ecx = sum & 0xffff
	addl	%eax, %ecx              # sum = low + high  (end-around carry)
	cmpl	$65535, %ecx
	ja	.Lv_fold_loop           # fold until no carry remains

# ---- the clever ending: return (sum == 0xFFFF) instead of (~sum == 0) -------
.Lv_check:                              # (clang's .LBB1_7)
	xorl	%eax, %eax              # eax = 0 — pre-clear the return value. Must
                                        #   come BEFORE sete, which writes only al and
                                        #   would leave stale bits 8..31 otherwise.
	cmpl	$65535, %ecx            # compare folded sum with 0xFFFF. Note: NO
                                        #   `notl` was emitted — the compiler replaced
                                        #   "complement then test ==0" with "test the
                                        #   pre-complement value ==0xFFFF". Equivalent,
                                        #   one instruction cheaper.
	sete	%al                     # al = (sum == 0xFFFF) ? 1 : 0  (set-if-equal
                                        #   reads ZF from the cmp). eax is now the int.
	popq	%rbp                    # EPILOGUE
	retq                            # return the 0/1 in eax
.Lfunc_end1:
	.size	ip_checksum_valid, .Lfunc_end1-ip_checksum_valid

	.ident	"clang version 20.1.8"
	.section	".note.GNU-stack","",@progbits  # non-executable stack: a
                                                        #   security default the linker records
# =============================================================================
# WHAT TO TAKE AWAY
#   * The ones-complement sum is a handful of adds + an end-around-carry fold +
#     a NOT. There is nothing magic about the "Internet checksum."
#   * -O1 INLINED csum_fold into both callers — the `csum_fold` symbol and its
#     `call` (visible in demo.O0.s) simply vanished.
#   * ip_checksum_valid shows the optimizer doing algebra on your behalf:
#     (~x == 0) became (x == 0xFFFF), dropping the `notl`. Reading asm is how you
#     catch these rewrites.
#   * At -O2 (see demo.O2.s) the word loop is AUTO-VECTORIZED with SSE2:
#     `movq`+`punpcklwd` widen four 16-bit words to 32-bit lanes, `paddd` sums
#     four at a time in xmm registers, and a `pshufd`/`paddd` cascade does the
#     horizontal reduction back to one scalar. Same math, 4-wide.
# =============================================================================
