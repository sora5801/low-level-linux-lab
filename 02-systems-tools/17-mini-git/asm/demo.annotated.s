# =============================================================================
# demo.annotated.s — clang -O1 output for asm/demo.c, explained instruction by
#                     instruction. These are mini-git's two hottest pure-logic
#                     routines: the SHA-1 compression round and hex encoding.
# =============================================================================
#
# HOW TO READ THIS FILE
# ---------------------
# This is the EXACT assembly clang 20 emits for asm/demo.c at -O1 (see demo.s for
# the untouched original), with a comment on essentially every instruction. AT&T
# syntax throughout:  op  src, dst    (destination LAST). `%reg` is a register,
# `$imm` an immediate, `N(%base,%index,scale)` is memory at base+index*scale+N.
# Register widths are the same register: rax(64)/eax(32)/ax(16)/al(8); writing
# eax zero-extends into rax, which is why the compiler prefers `movl`.
#
# THE SYSTEM V AMD64 ABI (the contract every function here obeys)
# --------------------------------------------------------------
#   integer/pointer args, in order:  rdi, rsi, rdx, rcx, r8, r9, then the STACK
#   return value:                    rax  (rdx:rax for 128-bit)
#   callee-saved (we must preserve):  rbx, rbp, r12, r13, r14, r15
#   caller-saved (free to clobber):   rax, rcx, rdx, rsi, rdi, r8, r9, r10, r11
#   the "red zone":                  128 bytes below rsp a LEAF may use freely
#   stack alignment:                 rsp % 16 == 0 at the point of any `call`
#
# THREE OPTIMIZATIONS TO WATCH FOR (all visible below)
# ----------------------------------------------------
#   (1) BIG-ENDIAN LOAD -> `bswapl`. The C hand-assembles a word from four bytes
#       `(b0<<24)|(b1<<16)|(b2<<8)|b3`; clang recognizes this as "load a 32-bit
#       word and byte-swap it" and emits one `bswap` instead of four loads+shifts.
#   (2) ROTATE IDIOM -> `rol`. The C `(x<<n)|(x>>(32-n))` has no rotate operator,
#       but clang folds it back into a single `roll` (by 1, 5, or 30 here).
#   (3) CHOOSE FUNCTION algebra. `(b&c)|(~b&d)` is emitted as the cheaper,
#       equivalent `d ^ (b & (c^d))` — two xors and an and, no `not`.
# =============================================================================

	.file	"demo.c"
	.text
	.globl	sha1_block                      # export sha1_block for the linker
	.p2align	4                       # 16-byte align the entry (I-fetch)
	.type	sha1_block,@function
sha1_block:                                     # void sha1_block(u32 h[5], const u8 block[64])
                                                #   rdi = h (5 words, updated in place)
                                                #   rsi = block (64 input bytes)

# ---- PROLOGUE ---------------------------------------------------------------
# sha1_block keeps MANY long-lived values (the five working vars a..e, the five
# saved originals h0..h4, and the loop counter) all in registers across 80
# rounds, so it claims every callee-saved register and must preserve them.
	pushq	%rbp                    # save caller's frame pointer
	movq	%rsp, %rbp             # establish our frame; rbp = frame base
	pushq	%r15                    # save the five callee-saved regs we will use
	pushq	%r14                    #   as a..e / originals across the whole body
	pushq	%r13
	pushq	%r12
	pushq	%rbx
	subq	$200, %rsp             # reserve stack for the w[80] message schedule
                                        #   (320 bytes). 200 here + up to the 128-byte
                                        #   RED ZONE below rsp cover it: this function
                                        #   calls nothing, so it is a LEAF and may use
                                        #   the red zone. &w[0] sits at -368(%rbp);
                                        #   w[0..31] land in the red zone, w[32..79] in
                                        #   this reserved region.

# ---- LOAD 16 BIG-ENDIAN WORDS: w[0..15] = big-endian block ------------------
	xorl	%eax, %eax             # rax = 0  == t, the load-loop counter
	.p2align	4
.LBB0_1:                                        # for (t = 0; t < 16; t++)
	movl	(%rsi,%rax,4), %ecx    # ecx = *(u32*)&block[t*4]  (a little-endian load)
	bswapl	%ecx                    # byte-swap -> the BIG-ENDIAN word SHA-1 wants.
                                        #   This ONE instruction replaced the four
                                        #   shift-and-or operations in the C source.
	movl	%ecx, -368(%rbp,%rax,4) # w[t] = ecx        (&w[0] == -368(%rbp))
	incq	%rax                    # t++
	cmpq	$16, %rax              # t == 16 ?
	jne	.LBB0_1                #   no -> next word

# ---- EXTEND TO 80 WORDS: w[t] = ROTL(w[t-3]^w[t-8]^w[t-14]^w[t-16], 1) -------
# %bb.2:
	movl	$16, %eax              # t = 16
	.p2align	4
.LBB0_3:                                        # for (t = 16; t < 80; t++)
	movl	-400(%rbp,%rax,4), %ecx # ecx = w[t-8]   (-400 = -368 - 8*4)
	xorl	-380(%rbp,%rax,4), %ecx #     ^= w[t-3]   (-380 = -368 - 3*4)
	xorl	-424(%rbp,%rax,4), %ecx #     ^= w[t-14]  (-424 = -368 - 14*4)
	xorl	-432(%rbp,%rax,4), %ecx #     ^= w[t-16]  (-432 = -368 - 16*4)
	roll	%ecx                    # rotate left by 1  == ROTL32(x, 1). The
                                        #   `(x<<1)|(x>>31)` idiom folded to one `rol`.
	movl	%ecx, -368(%rbp,%rax,4) # w[t] = ecx
	incq	%rax                    # t++
	cmpq	$80, %rax              # t == 80 ?
	jne	.LBB0_3                #   no -> next word

# ---- INITIALIZE WORKING VARS from the running state -------------------------
# %bb.4:
	movl	(%rdi), %r8d           # r8  = h[0]   (KEPT untouched for the final fold)
	movl	4(%rdi), %esi          # esi = h[1]        "         "
	movl	8(%rdi), %edx          # edx = h[2]        "         "
	movl	12(%rdi), %ecx         # ecx = h[3]        "         "
	movl	16(%rdi), %eax         # eax = h[4]        "         "
	xorl	%r15d, %r15d           # r15 = 0  == t, the ROUND counter
	movl	%r8d, %r14d            # a = h[0]   (a is r14 throughout the rounds)
	movl	%esi, %ebx             # b = h[1]   (b is ebx)
	movl	%edx, %r11d            # c = h[2]   (c is r11)
	movl	%ecx, %r10d            # d = h[3]   (d is r10)
	movl	%eax, %r9d             # e = h[4]   (e is r9)

# ---- ROUNDS 0..19  (f = "choose", K = 0x5A827999) ---------------------------
# Each round does, in the C:  T = ROTL(a,5)+f+e+w[t]+K; e=d; d=c; c=ROTL(b,30);
# b=a; a=T.  clang implements the five-way variable rotation as a chain of movs
# at the top of the loop, reusing the SAME five registers every iteration.
	.p2align	4
.LBB0_5:                                        # =>rounds 0..19
	movl	%r9d, %r12d            # r12 = e           (stash old e for the +e below)
	movl	%r10d, %r9d            # e = d             (r9  <- old d)
	movl	%r11d, %r10d           # d = c             (r10 <- old c)
	movl	%ebx, %r11d            # c = b             (r11 <- old b; rotated later)
	movl	%r14d, %ebx            # b = a             (ebx <- old a)
	# f = Ch(b,c,d) computed as d ^ (b & (c ^ d))  — see header note (3):
	movl	%r10d, %r14d           # r14 = c   (old c, now in r10)
	xorl	%r9d, %r14d            # r14 = c ^ d
	andl	%r11d, %r14d           # r14 = (c ^ d) & b        (b = old b in r11)
	xorl	%r9d, %r14d            # r14 = ((c^d)&b) ^ d  == Ch(b,c,d) == f
	movl	%ebx, %r13d            # r13 = b(new) == old a == a
	roll	$5, %r13d             # r13 = ROTL(a, 5)
	addl	%r12d, %r13d           # r13 = ROTL(a,5) + e
	addl	%r14d, %r13d           # r13 = ROTL(a,5) + e + f
	movl	-368(%rbp,%r15,4), %r14d# r14 = w[t]
	addl	%r13d, %r14d           # r14 = w[t] + ROTL(a,5)+e+f
	addl	$1518500249, %r14d     # r14 += 0x5A827999 (K0)  == T == the new a
	roll	$30, %r11d            # c = ROTL(b, 30)   (finish the c=ROTL(b,30) step)
	incq	%r15                    # t++
	cmpq	$20, %r15             # t == 20 ?
	jne	.LBB0_5                #   no -> next round

# ---- ROUNDS 20..39  (f = "parity" b^c^d, K = 0x6ED9EBA1) --------------------
# Identical skeleton; only the f computation and the constant change. Parity is
# just two xors, so this quarter is a touch shorter than the others.
# %bb.6:
	movl	$20, %r15d             # t = 20
	.p2align	4
.LBB0_7:                                        # =>rounds 20..39
	movl	%r9d, %r12d            # stash old e
	movl	%r10d, %r9d            # e = d
	movl	%r11d, %r10d           # d = c
	movl	%ebx, %r11d            # c = b (rotate later)
	movl	%r14d, %ebx            # b = a
	movl	%r10d, %r14d           # r14 = c
	xorl	%r9d, %r14d            # r14 = c ^ d
	xorl	%r11d, %r14d           # r14 = c ^ d ^ b == Parity == f
	movl	%ebx, %r13d            # r13 = a
	roll	$5, %r13d             # r13 = ROTL(a,5)
	addl	%r12d, %r14d           # r14 = f + e
	addl	%r14d, %r13d           # r13 = ROTL(a,5) + f + e
	movl	-368(%rbp,%r15,4), %r14d# r14 = w[t]
	addl	%r13d, %r14d           # r14 = w[t] + ...
	addl	$1859775393, %r14d     # r14 += 0x6ED9EBA1 (K1) == T
	roll	$30, %r11d            # c = ROTL(b,30)
	incq	%r15                    # t++
	cmpq	$40, %r15             # t == 40 ?
	jne	.LBB0_7

# ---- ROUNDS 40..59  (f = "majority" (b&c)|(b&d)|(c&d), K = 0x8F1BBCDC) ------
# Majority is the most expensive f: three ands and two ors.
# %bb.8:
	movl	$40, %r15d             # t = 40
	.p2align	4
.LBB0_9:                                        # =>rounds 40..59
	movl	%r9d, %r12d            # stash old e
	movl	%r10d, %r9d            # e = d
	movl	%r11d, %r10d           # d = c
	movl	%ebx, %r11d            # c = b (rotate later)
	movl	%r14d, %ebx            # b = a
	movl	%r10d, %r14d           # r14 = c
	orl	%r9d, %r14d            # r14 = c | d
	andl	%r11d, %r14d           # r14 = (c|d) & b
	movl	%r10d, %r13d           # r13 = c
	andl	%r9d, %r13d            # r13 = c & d
	orl	%r14d, %r13d           # r13 = ((c|d)&b) | (c&d) == Maj(b,c,d) == f
                                        #   (an algebraic form of (b&c)|(b&d)|(c&d))
	movl	%ebx, %r14d            # r14 = a
	roll	$5, %r14d             # r14 = ROTL(a,5)
	addl	%r12d, %r14d           # r14 = ROTL(a,5) + e
	addl	%r13d, %r14d           # r14 = ROTL(a,5) + e + f
	movl	-368(%rbp,%r15,4), %r12d# r12 = w[t]
	addl	%r12d, %r14d           # r14 += w[t]
	addl	$-1894007588, %r14d    # r14 += 0x8F1BBCDC (K2, as a signed imm) == T
	roll	$30, %r11d            # c = ROTL(b,30)
	incq	%r15                    # t++
	cmpq	$60, %r15             # t == 60 ?
	jne	.LBB0_9

# ---- ROUNDS 60..79  (f = "parity" again, K = 0xCA62C1D6) --------------------
# %bb.10:
	movl	$60, %r15d             # t = 60
	.p2align	4
.LBB0_11:                                       # =>rounds 60..79
	movl	%r9d, %r12d            # stash old e
	movl	%r10d, %r9d            # e = d
	movl	%r11d, %r10d           # d = c
	movl	%ebx, %r11d            # c = b (rotate later)
	movl	%r14d, %ebx            # b = a
	movl	%r10d, %r14d           # r14 = c
	xorl	%r9d, %r14d            # r14 = c ^ d
	xorl	%r11d, %r14d           # r14 = c ^ d ^ b == Parity == f
	movl	%ebx, %r13d            # r13 = a
	roll	$5, %r13d             # r13 = ROTL(a,5)
	addl	%r12d, %r14d           # r14 = f + e
	addl	%r14d, %r13d           # r13 = ROTL(a,5) + f + e
	movl	-368(%rbp,%r15,4), %r14d# r14 = w[t]
	addl	%r13d, %r14d           # r14 = w[t] + ...
	addl	$-899497514, %r14d     # r14 += 0xCA62C1D6 (K3) == T
	roll	$30, %r11d            # c = ROTL(b,30)
	incq	%r15                    # t++
	cmpq	$80, %r15             # t == 80 ?
	jne	.LBB0_11

# ---- FOLD the working vars back into the running state (mod 2^32) -----------
# The saved originals h0..h4 (r8/esi/edx/ecx/eax) are added to the final a..e
# (r14/ebx/r11/r10/r9) and written back through rdi. This feedback is what makes
# SHA-1 one-way.
# %bb.12:
	addl	%r8d, %r14d            # a += h[0]
	movl	%r14d, (%rdi)          # h[0] = a
	addl	%esi, %ebx            # b += h[1]
	movl	%ebx, 4(%rdi)          # h[1] = b
	addl	%edx, %r11d            # c += h[2]
	movl	%r11d, 8(%rdi)         # h[2] = c
	addl	%ecx, %r10d            # d += h[3]
	movl	%r10d, 12(%rdi)        # h[3] = d
	addl	%eax, %r9d            # e += h[4]
	movl	%r9d, 16(%rdi)         # h[4] = e

# ---- EPILOGUE: undo the frame, restore callee-saved regs (reverse order) ----
	addq	$200, %rsp            # release the w[] reservation
	popq	%rbx                    # restore rbx ...
	popq	%r12
	popq	%r13
	popq	%r14
	popq	%r15                    #   ... r15
	popq	%rbp                    # restore caller's frame pointer
	retq                            # return (void)
.Lfunc_end0:
	.size	sha1_block, .Lfunc_end0-sha1_block

# =============================================================================
# hex_encode — 20 raw bytes -> 40 hex chars. Branch-free table lookup.
# =============================================================================
	.globl	hex_encode
	.p2align	4
	.type	hex_encode,@function
hex_encode:                                     # void hex_encode(const u8 *raw, int n, char *out)
                                                #   rdi = raw, esi = n, rdx = out
# %bb.0:
	pushq	%rbp                    # PROLOGUE (frame pointer kept at -O1 for debug)
	movq	%rsp, %rbp
	testl	%esi, %esi             # n <= 0 ?
	jle	.LBB1_3                #   yes -> skip straight to writing the NUL
# %bb.1:
	movl	%esi, %eax             # eax = n  (the trip count, zero-extended below)
	xorl	%ecx, %ecx             # rcx = 0  == i, the byte index
	leaq	hex_encode.d(%rip), %r8 # r8 = &"0123456789abcdef"  (RIP-relative)
	.p2align	4
.LBB1_2:                                        # for (i = 0; i < n; i++)
	movzbl	(%rdi,%rcx), %r9d      # r9 = raw[i]  (zero-extend byte -> 32 bits)
	shrl	$4, %r9d              # r9 = raw[i] >> 4        (the HIGH nibble)
	movzbl	(%r9,%r8), %r9d        # r9 = d[high]  (table[base + nibble])
	movb	%r9b, (%rdx,%rcx,2)    # out[2*i] = that hex char
	movzbl	(%rdi,%rcx), %r9d      # reload raw[i]
	andl	$15, %r9d             # r9 = raw[i] & 0x0f      (the LOW nibble)
	movzbl	(%r9,%r8), %r9d        # r9 = d[low]
	movb	%r9b, 1(%rdx,%rcx,2)   # out[2*i+1] = that hex char
	incq	%rcx                    # i++
	cmpq	%rcx, %rax             # i == n ?
	jne	.LBB1_2                #   no -> next byte
.LBB1_3:
	movslq	%esi, %rax             # rax = (long)n  (sign-extend for the address)
	movb	$0, (%rdx,%rax,2)      # out[2*n] = '\0'  (terminate the string)
	popq	%rbp                    # EPILOGUE
	retq
.Lfunc_end1:
	.size	hex_encode, .Lfunc_end1-hex_encode

# =============================================================================
# demo_run — build the padded "abc" block, run ONE sha1_block, hex the digest.
# Notice clang CONSTANT-FOLDS the block construction (it stores the literal word
# 0x80636261 = 'a','b','c',0x80 and the length byte 24 directly) but it does NOT
# fold sha1_block itself — the 80-round loop-carried dependency is too much to
# evaluate at compile time, so a real `call` remains. That contrast is the
# lesson: the optimizer runs what it can prove and calls the rest.
# =============================================================================
	.globl	demo_run
	.p2align	4
	.type	demo_run,@function
demo_run:
# %bb.0:
	pushq	%rbp                    # PROLOGUE
	movq	%rsp, %rbp
	subq	$176, %rsp             # frame for: block[64], h[5], raw[20], hex[41]
	xorps	%xmm0, %xmm0           # xmm0 = 0 : used to zero the block in 16-byte...
	movaps	%xmm0, -96(%rbp)        # ...chunks. block[] is at -96(%rbp).
	movaps	%xmm0, -48(%rbp)
	movaps	%xmm0, -64(%rbp)
	movaps	%xmm0, -80(%rbp)        # block[0..63] now all zero
	movl	$-2140970399, -96(%rbp) # block[0..3] = 0x80636261 (LE) = 'a''b''c'0x80
	movb	$24, -33(%rbp)         # block[63] = 24  (message length in BITS)
	movaps	.L__const.demo_run.h(%rip), %xmm0  # load h[0..3] init constants (16 B)
	movaps	%xmm0, -32(%rbp)        # h[0..3] = {0x67452301,..,0x10325476}
	movl	$-1009589776, -16(%rbp) # h[4] = 0xC3D2E1F0
	leaq	-32(%rbp), %rdi         # arg0 = &h
	leaq	-96(%rbp), %rsi         # arg1 = &block
	callq	sha1_block             # h = compress(h, block)  — the real call
	xorl	%eax, %eax             # i = 0
	.p2align	4
.LBB2_1:                                        # serialize h[i] big-endian into raw[]
	movl	-32(%rbp,%rax,4), %ecx # ecx = h[i]
	bswapl	%ecx                    # to big-endian (same trick as the loader)
	movl	%ecx, -128(%rbp,%rax,4) # raw[i*4..] = ecx   (raw[] at -128(%rbp))
	incq	%rax                    # i++
	cmpq	$5, %rax              # i == 5 ?
	jne	.LBB2_1
# %bb.2:
	xorl	%eax, %eax             # i = 0
	leaq	hex_encode.d(%rip), %rcx# rcx = &hex digit table (hex_encode inlined here)
	.p2align	4
.LBB2_3:                                        # hex-encode raw[0..19] -> hex[0..39]
	movzbl	-128(%rbp,%rax), %edx  # edx = raw[i]
	movl	%edx, %esi             # copy for the low-nibble path
	shrl	$4, %esi              # esi = raw[i] >> 4  (high nibble)
	movzbl	(%rsi,%rcx), %esi      # table[high]
	movb	%sil, -176(%rbp,%rax,2)# hex[2*i]   (hex[] at -176(%rbp))
	andl	$15, %edx             # edx = raw[i] & 15  (low nibble)
	movzbl	(%rdx,%rcx), %edx      # table[low]
	movb	%dl, -175(%rbp,%rax,2) # hex[2*i+1]
	incq	%rax                    # i++
	cmpq	$20, %rax             # i == 20 ?
	jne	.LBB2_3
# %bb.4:
	movzbl	-176(%rbp), %eax       # return (int)hex[0]  — 'a' (97) for "abc"
	addq	$176, %rsp            # EPILOGUE
	popq	%rbp
	retq
.Lfunc_end2:
	.size	demo_run, .Lfunc_end2-demo_run

# ---- READ-ONLY DATA ---------------------------------------------------------
	.type	hex_encode.d,@object
	.section	.rodata.cst16,"aM",@progbits,16
	.p2align	4, 0x0
hex_encode.d:
	.ascii	"0123456789abcdef"              # the 16-entry nibble->char table
	.size	hex_encode.d, 16

	.type	.L__const.demo_run.h,@object
	.section	.rodata,"a",@progbits
	.p2align	4, 0x0
.L__const.demo_run.h:                           # the SHA-1 init constants, as data
	.long	1732584193                      # 0x67452301
	.long	4023233417                      # 0xefcdab89
	.long	2562383102                      # 0x98badcfe
	.long	271733878                       # 0x10325476
	.long	3285377520                      # 0xc3d2e1f0
	.size	.L__const.demo_run.h, 20

	.ident	"clang version 20.1.8"
	.section	".note.GNU-stack","",@progbits  # non-executable stack (security)
	.addrsig                                # address-significance table (LTO aid)

# =============================================================================
# WHAT TO TAKE AWAY
#   * The whole SHA round is INTEGER arithmetic: rol / add / and / xor / or. No
#     loads except the message schedule, no branches inside a round. That is why
#     SHA-1 is fast and why it makes such legible assembly.
#   * clang recognized THREE C idioms and rewrote them: the big-endian byte
#     assembly became `bswap`, the rotate idiom became `rol`, and the "choose"
#     function became `d ^ (b&(c^d))`. Reading asm is how you SEE those.
#   * The 80-round variable rotation e=d;d=c;c=ROTL(b,30);b=a;a=T is just a chain
#     of `mov`s reusing five fixed registers — no memory traffic per round.
#   * sha1_block is a LEAF, so it borrows the 128-byte red zone for part of the
#     w[80] schedule instead of reserving all 320 bytes with `subq`.
#   * Compare with demo.O0.s (every variable spilled to the stack, the rotate
#     idiom emitted as two shifts + or) and demo.O2.s (the round loops partially
#     unrolled and the schedule software-pipelined) to watch the optimizer work.
# =============================================================================
