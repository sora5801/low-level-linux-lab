# =============================================================================
# demo.annotated.s — the Internet Checksum in x86-64 assembly, explained.
# =============================================================================
#
# This is clang's -O1 output for asm/demo.c (see demo.s for the untouched
# original), with a comment on essentially every instruction. It is the RFC 1071
# ones'-complement checksum that IP / ICMP / UDP / TCP all rely on.
#
# HOW TO READ THIS FILE
# ---------------------
# AT&T syntax:  op  src, dst           # movl $1, %eax   =>   eax = 1
#     %reg = register   $imm = literal   N(%reg) = memory at [reg + N]
# Register widths are windows onto the SAME physical register:
#     rax(64) / eax(32) / ax(16) / al(8).  Writing a 32-bit dst (e.g. `movl`)
#     ZERO-EXTENDS into the full 64-bit register — the compiler leans on this.
#
# THE SYSTEM V AMD64 ABI (what every function here obeys)
# ------------------------------------------------------
#   * Integer/pointer ARGS, in order:  rdi, rsi, rdx, rcx, r8, r9  (then stack).
#   * RETURN value:                     rax  (16-bit results come back in ax).
#   * CALLEE-SAVED (a function must preserve): rbx, rbp, r12-r15.  Everything
#     else (rax, rcx, rdx, rsi, rdi, r8-r11) is CALLER-saved / scratch.
#   * THE RED ZONE: 128 bytes below rsp that a leaf function may scribble on
#     without adjusting rsp. All three routines here are leaves that keep their
#     locals in registers, so none of them touches the stack at all beyond the
#     frame-pointer push.
#   * STACK ALIGNMENT: rsp must be 16-byte aligned at every `call`. These leaves
#     make no calls, so alignment is moot — the pushq %rbp is kept only because
#     -fno-omit-frame-pointer asked for a walkable frame, not because it's needed.
#
# The three functions map to the three C routines. At -O1 they are NOT inlined
# into each other except inside inet_checksum, which absorbs both helpers.
# =============================================================================

	.file	"demo.c"
	.text

# =============================================================================
# u32 csum_accumulate(const void *data, usize len, u32 sum)
#   args:  rdi = data (pointer)      rsi = len (byte count)   edx = sum (seed)
#   ret:   eax = updated 32-bit ones'-complement running sum
# =============================================================================
	.globl	csum_accumulate
	.p2align	4
	.type	csum_accumulate,@function
csum_accumulate:
# ---- PROLOGUE (frame pointer only; no locals need the stack) ----------------
	pushq	%rbp                    # save caller's frame pointer
	movq	%rsp, %rbp              # rbp = frame base (for a clean backtrace)

	movl	%edx, %eax              # eax = sum:  seed the accumulator with the
	                                #   `sum` argument. eax is also the return
	                                #   register, so we build the result in place.
	cmpq	$2, %rsi                # compare len (rsi) with 2 ...
	jb	.LBB0_2                 # ... if len < 2 (unsigned Below), skip the
	                                #   word loop straight to the tail-byte check.
	.p2align	4               # align the hot loop head for the fetch unit

# ---- WORD LOOP: while (len >= 2) sum += (p[0]<<8) | p[1]; p+=2; len-=2; ------
.LBB0_1:                                # "word loop"
	movzwl	(%rdi), %ecx            # load 2 bytes at [data] as a 16-bit word,
	                                #   zero-extended to 32. On little-endian x86
	                                #   this puts p[0] in the LOW byte of cx and
	                                #   p[1] in the HIGH byte:  cx = p[0]|p[1]<<8.
	rolw	$8, %cx                 # rotate the 16-bit cx left by 8 => SWAP its
	                                #   two bytes => cx = p[1]|p[0]<<8 = the
	                                #   big-endian word (p[0]<<8)|p[1]. This 2-op
	                                #   "load + rotate" is how clang realized our
	                                #   byte-assembly expression as one memory read.
	movzwl	%cx, %ecx               # re-zero the top 16 bits of ecx (rolw only
	                                #   wrote cx; the high half was stale).
	addl	%ecx, %eax              # sum += word. Carry out of bit 15 lands in
	                                #   bit 16+ of the 32-bit eax and is kept for
	                                #   the later fold — the whole point of using
	                                #   a 32-bit accumulator.
	addq	$2, %rdi                # data += 2  (advance the byte pointer)
	addq	$-2, %rsi               # len  -= 2
	cmpq	$1, %rsi                # compare len with 1 ...
	ja	.LBB0_1                 # ... loop while len > 1 (unsigned Above), i.e.
	                                #   while at least one more full word remains.

# ---- TAIL BYTE: if (len == 1) sum += p[0] << 8; ----------------------------
.LBB0_2:                                # "check trailing byte"
	testq	%rsi, %rsi              # len == 0 ?  (test sets ZF when rsi is zero)
	je	.LBB0_4                 # if no leftover byte, done.
# %bb.3:                                #   (len is exactly 1 here)
	movzbl	(%rdi), %ecx            # load the single leftover byte into ecx
	shll	$8, %ecx                # << 8: it is the HIGH byte of a final word
	                                #   whose low byte is an implicit 0 (RFC 1071).
	addl	%ecx, %eax              # fold that half-word into the sum.

# ---- EPILOGUE ---------------------------------------------------------------
.LBB0_4:                                # "return"
	popq	%rbp                    # restore caller's frame pointer
	retq                            # return; eax holds the running sum
.Lfunc_end0:
	.size	csum_accumulate, .Lfunc_end0-csum_accumulate

# =============================================================================
# u16 csum_fold(u32 sum)
#   arg:  edi = sum (32-bit accumulator)      ret: ax = folded 16-bit checksum
# =============================================================================
	.globl	csum_fold
	.p2align	4
	.type	csum_fold,@function
csum_fold:
	pushq	%rbp                    # PROLOGUE: save/establish the frame pointer
	movq	%rsp, %rbp

	movl	%edi, %eax              # eax = sum  (work in the return register)
	cmpl	$65536, %edi            # sum < 0x10000 ?  (i.e. is the high half 0?)
	jb	.LBB1_2                 # if nothing to fold, skip to the complement.
	.p2align	4

# ---- FOLD LOOP: while (sum >> 16) sum = (sum & 0xffff) + (sum >> 16); --------
.LBB1_1:                                # "fold loop" (the end-around carry)
	movl	%eax, %ecx              # ecx = sum
	shrl	$16, %ecx               # ecx = sum >> 16   (the accumulated carries)
	movzwl	%ax, %eax               # eax = sum & 0xffff (keep only the low half)
	addl	%ecx, %eax              # sum = low16 + high16 : carries fold back in.
	cmpl	$65535, %eax            # did that addition itself overflow 16 bits?
	ja	.LBB1_1                 # if sum > 0xFFFF, fold once more (at most one
	                                #   extra pass is ever needed).

# ---- ONES'-COMPLEMENT and return -------------------------------------------
.LBB1_2:                                # "complement"
	notl	%eax                    # eax = ~sum : the stored checksum value.
	                                # kill: def $ax ... — clang's note that only
	                                #   the low 16 bits (ax) are the u16 result.
	popq	%rbp                    # EPILOGUE
	retq                            # return; ax = checksum
.Lfunc_end1:
	.size	csum_fold, .Lfunc_end1-csum_fold

# =============================================================================
# u16 inet_checksum(const void *data, usize len)
#   args: rdi = data   rsi = len        ret: ax = full checksum of the buffer
#
# At -O1 clang INLINED csum_accumulate (seed sum = 0) and csum_fold into one
# body: first the word loop + tail byte, then the fold loop + complement. This
# is the routine to read to "see" the whole algorithm as straight-line code.
# =============================================================================
	.globl	inet_checksum
	.p2align	4
	.type	inet_checksum,@function
inet_checksum:
	pushq	%rbp                    # PROLOGUE
	movq	%rsp, %rbp

	xorl	%eax, %eax              # sum = 0  (xor r,r is the 2-byte idiom for
	                                #   zeroing; also a CPU dependency-breaker).
	cmpq	$2, %rsi                # len < 2 ?
	jb	.LBB2_3                 # if so, jump past the word loop.
# %bb.1:
	xorl	%eax, %eax              # sum = 0 again on this path (redundant, but
	                                #   harmless — the two entry paths were merged).
	.p2align	4

# ---- inlined csum_accumulate: WORD LOOP ------------------------------------
.LBB2_2:                                # "word loop"
	movzwl	(%rdi), %ecx            # word = 16-bit load at [data] (see above)
	rolw	$8, %cx                 # byte-swap to big-endian order
	movzwl	%cx, %ecx               # zero-extend to 32 bits
	addl	%ecx, %eax              # sum += word
	addq	$2, %rdi                # data += 2
	addq	$-2, %rsi               # len  -= 2
	cmpq	$1, %rsi                # while len > 1 ...
	ja	.LBB2_2                 # ... keep summing full words.

# ---- inlined csum_accumulate: TAIL BYTE ------------------------------------
.LBB2_3:                                # "check trailing byte"
	testq	%rsi, %rsi              # len == 0 ?
	je	.LBB2_5                 # no leftover -> go fold.
# %bb.4:
	movzbl	(%rdi), %ecx            # load the odd final byte
	shll	$8, %ecx                # it is the high byte of the last word
	addl	%ecx, %eax              # sum += (byte << 8)

# ---- inlined csum_fold: FOLD LOOP + COMPLEMENT -----------------------------
.LBB2_5:                                # "fold check"
	cmpl	$65536, %eax            # high half nonzero?
	jb	.LBB2_7                 # if not, skip straight to the complement.
	.p2align	4
.LBB2_6:                                # "fold loop"
	movl	%eax, %ecx              # ecx = sum
	shrl	$16, %ecx               # ecx = sum >> 16
	movzwl	%ax, %eax               # eax = sum & 0xffff
	addl	%ecx, %eax              # sum = low16 + high16 (end-around carry)
	cmpl	$65535, %eax            # overflowed again?
	ja	.LBB2_6                 # fold once more if so.
.LBB2_7:                                # "complement"
	notl	%eax                    # eax = ~sum  -> the 16-bit checksum in ax
	                                # kill: def $ax ... (only ax is the result)
	popq	%rbp                    # EPILOGUE
	retq                            # return; ax = checksum
.Lfunc_end2:
	.size	inet_checksum, .Lfunc_end2-inet_checksum

# =============================================================================
# WHAT TO TAKE AWAY
#   * The 32-bit accumulator is load-bearing: every `addl` can carry out of bit
#     15, and those carries MUST survive to be folded back (end-around carry).
#   * clang compiled our `(p[0]<<8)|p[1]` byte assembly into a 16-bit LOAD plus
#     a `rolw $8` byte-swap — one memory access, not two. Reading the asm is how
#     you catch that the "obvious" C and the machine code diverge in shape.
#   * The fold loop runs at most twice for any real packet, so the checksum is
#     effectively branch-free straight-line arithmetic on the hot path.
#   * These are leaf functions: no `call`, no stack locals, no red-zone use —
#     the frame-pointer push is bookkeeping, and every value lives in a register.
#   * Compare with demo.O0.s (every C statement spilled to the stack, real
#     `call`s between the helpers) and demo.O2.s (the loop unrolled/vectorized).
# =============================================================================
	.section	".note.GNU-stack","",@progbits
