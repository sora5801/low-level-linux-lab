# =============================================================================
# demo.annotated.s — clang -O1 output for demo.c (ChaCha20), explained.
# The two functions are chacha_quarter_round (the ARX core, annotated in full)
# and chacha20_block (the 20-round block, annotated by construct). See asm/demo.s
# for the untouched compiler output this is a commentary on.
# =============================================================================
#
# HOW TO READ THIS FILE (AT&T syntax: `op src, dst`)
# --------------------------------------------------
#   movl $16, %eax       => eax = 16        $imm = literal    %reg = register
#   N(%base,%idx,scale)  => memory at base + idx*scale + N
#   roll $16, %edx       => rotate edx left by 16 bits (this IS ChaCha's rotate)
#   Writing a 32-bit name (eax) ZERO-EXTENDS into the 64-bit reg (rax). That is
#   why you see `movl %esi, %esi`: it clears the top 32 bits of rsi so a 32-bit
#   `unsigned` index can be used as a 64-bit address register.
#
# THE SysV AMD64 ABI CONTRACT
# ---------------------------
#   Integer/pointer ARGS, in order:  rdi, rsi, rdx, rcx, r8, r9  (then stack).
#   RETURN value: rax (both functions here return void).
#   CALLEE-SAVED (must be preserved for the caller): rbx, rbp, r12, r13, r14,
#     r15. chacha20_block uses all of them, so its prologue pushes each and its
#     epilogue pops them in reverse. CALLER-SAVED (free scratch): rax, rcx, rdx,
#     rsi, rdi, r8-r11.
#   Stack must be 16-byte aligned at a `call`. Both functions are LEAF (they call
#     nothing — rotl32 was inlined), so alignment is trivially kept.
#
# THE ONE LESSON OF THIS FILE
# ---------------------------
# ChaCha is "ARX": Add, Rotate, Xor, and nothing else. Scan the assembly and you
# will find ONLY addl / xorl / roll on registers — no table loads, no branches on
# data. That is the machine-level proof that ChaCha is constant-time: its running
# time and memory-access pattern are independent of the key and the plaintext.
# =============================================================================

	.file	"demo.c"
	.text
	.globl	chacha_quarter_round            # export for the linker
	.p2align	4                       # 16-byte align the entry (I-fetch)
	.type	chacha_quarter_round,@function
chacha_quarter_round:                   # void chacha_quarter_round(u32 *s,
                                        #        unsigned a, b, c, d)
# ---- PROLOGUE ---------------------------------------------------------------
# ABI mapping on entry: rdi = s (base pointer), esi = a, edx = b, ecx = c,
# r8d = d. The indices are 32-bit `unsigned`, so each must be zero-extended
# before it can index memory as a 64-bit register.
	pushq	%rbp                    # save caller's frame pointer
	movq	%rsp, %rbp              # rbp = base of our frame
	movl	%edx, %eax              # eax = b. Stash b's INDEX in rax; edx is
                                        #   about to become the data accumulator.
	movl	%esi, %esi              # zero-extend a into rsi so (%rdi,%rsi,4) is
                                        #   a valid 64-bit address of s[a].

# ---- STEP 1:  s[a] += s[b];  s[d] ^= s[a];  s[d] = rotl(s[d],16) ------------
	movl	(%rdi,%rsi,4), %edx     # edx = s[a]         (load word at s + a*4)
	addl	(%rdi,%rax,4), %edx     # edx = s[a] + s[b]  (rax holds index b)
	movl	%edx, (%rdi,%rsi,4)     # s[a] = s[a] + s[b]  (store back)
	movl	%r8d, %r8d              # zero-extend d into r8 for indexing
	xorl	(%rdi,%r8,4), %edx      # edx = s[d] ^ s[a]  (edx still = new s[a])
	roll	$16, %edx               # edx = rotl(s[d]^s[a], 16)  <-- ARX rotate
	movl	%edx, (%rdi,%r8,4)      # s[d] = that. Now edx tracks the live s[d].

# ---- STEP 2:  s[c] += s[d];  s[b] ^= s[c];  s[b] = rotl(s[b],12) ------------
	movl	%ecx, %ecx              # zero-extend c into rcx
	addl	(%rdi,%rcx,4), %edx     # edx = s[c] + s[d]
	movl	%edx, (%rdi,%rcx,4)     # s[c] = s[c] + s[d]
	xorl	(%rdi,%rax,4), %edx     # edx = s[b] ^ s[c]
	roll	$12, %edx               # edx = rotl(..., 12)
	movl	%edx, (%rdi,%rax,4)     # s[b] = that. edx now tracks s[b].

# ---- STEP 3:  s[a] += s[b];  s[d] ^= s[a];  s[d] = rotl(s[d],8) -------------
	addl	(%rdi,%rsi,4), %edx     # edx = s[a] + s[b]
	movl	%edx, (%rdi,%rsi,4)     # s[a] = s[a] + s[b]
	xorl	(%rdi,%r8,4), %edx      # edx = s[d] ^ s[a]
	roll	$8, %edx                # edx = rotl(..., 8)
	movl	%edx, (%rdi,%r8,4)      # s[d] = that

# ---- STEP 4:  s[c] += s[d];  s[b] ^= s[c];  s[b] = rotl(s[b],7) -------------
	addl	(%rdi,%rcx,4), %edx     # edx = s[c] + s[d]
	movl	%edx, (%rdi,%rcx,4)     # s[c] = s[c] + s[d]
	xorl	(%rdi,%rax,4), %edx     # edx = s[b] ^ s[c]
	roll	$7, %edx                # edx = rotl(..., 7)
	movl	%edx, (%rdi,%rax,4)     # s[b] = that (final store)
# ---- EPILOGUE ---------------------------------------------------------------
	popq	%rbp                    # restore caller's frame pointer
	retq                            # return (void)
.Lfunc_end0:
	.size	chacha_quarter_round, .Lfunc_end0-chacha_quarter_round

# =============================================================================
# chacha20_block — one 64-byte keystream block.
# The optimizer keeps the state array on the stack but pulls the 16 working words
# `x[]` into registers (+ a handful of stack spills, since there are only 15 GP
# registers) and runs the double-round loop 10 times. Watch how it REASSIGNS
# which register holds which x[i] as the diagonals reach across the matrix.
# =============================================================================
	.section	.rodata.cst16,"aM",@progbits,16
	.p2align	4, 0x0
.LCPI1_0:                               # the four ChaCha constants, preloadable
	.long	1634760805                      # 0x61707865  "expa"
	.long	857760878                       # 0x3320646e  "nd 3"
	.long	2036477234                      # 0x79622d32  "2-by"
	.long	1797285236                      # 0x6b206574  "te k"
	.text
	.globl	chacha20_block
	.p2align	4
	.type	chacha20_block,@function
chacha20_block:                         # void chacha20_block(u8 out[64],
                                        #   const u8 key[32], u32 counter,
                                        #   const u8 nonce[12])
# ABI on entry: rdi = out, rsi = key, edx = counter, rcx = nonce.
# ---- PROLOGUE: save callee-saved regs, carve a frame -------------------------
	pushq	%rbp
	movq	%rsp, %rbp
	pushq	%r15                    # \ save every callee-saved register we will
	pushq	%r14                    #  \ use to hold x[] words during the rounds.
	pushq	%r13                    #  / The ABI promises the caller these survive.
	pushq	%r12                    # /
	pushq	%rbx
	subq	$56, %rsp               # reserve local space. Together with the red
                                        #   zone this frame holds s[] and x[].
	movq	%rdi, -160(%rbp)        # spill `out` (we need rdi/rax as scratch)

# ---- BUILD THE STATE s[] on the stack (base = -224(%rbp)) -------------------
# s[0..3] = the constants, loaded 16 bytes at once with an SSE move.
	movaps	.LCPI1_0(%rip), %xmm0   # xmm0 = the four constants
	movaps	%xmm0, -224(%rbp)       # s[0..3] = constants
	xorl	%eax, %eax              # i = 0
	.p2align	4
.LBB1_1:                                # for (i=0;i<8;i++) s[4+i]=load_le32(key+4i)
	movl	(%rsi,%rax,4), %edi     # edi = key word i (little-endian on x86, so a
                                        #   plain 32-bit load equals load_le32)
	movl	%edi, -208(%rbp,%rax,4) # s[4+i] = that  (-208 = &s[4])
	incq	%rax                    # i++
	cmpq	$8, %rax                # i < 8 ?
	jne	.LBB1_1
# %bb.2:  s[12]=counter; s[13..15]=nonce words
	movl	%edx, -176(%rbp)        # s[12] = counter   (-176 = &s[12])
	movl	(%rcx), %eax
	movl	%eax, -172(%rbp)        # s[13] = nonce[0..3]
	movl	4(%rcx), %eax
	movl	%eax, -168(%rbp)        # s[14] = nonce[4..7]
	movl	8(%rcx), %eax
	movl	%eax, -164(%rbp)        # s[15] = nonce[8..11]

# ---- COPY s[] -> x[] (working copy, base = -144(%rbp)) ----------------------
# The compiler moves 12 words as three 16-byte SSE copies plus the counter/nonce
# words individually. x[] is what the rounds mutate; s[] is kept for the final
# feed-forward add.
	movaps	-224(%rbp), %xmm0       # s[0..3]
	movaps	-208(%rbp), %xmm1       # s[4..7]
	movaps	-192(%rbp), %xmm2       # s[8..11]
	movaps	%xmm1, -128(%rbp)       # x[4..7]
	movaps	%xmm0, -144(%rbp)       # x[0..3]
	movl	-176(%rbp), %eax
	movl	%eax, -96(%rbp)         # x[12] = s[12]
	movl	-172(%rbp), %eax
	movl	%eax, -92(%rbp)         # x[13]
	movl	-168(%rbp), %eax
	movl	%eax, -88(%rbp)         # x[14]
	movl	-164(%rbp), %eax
	movl	%eax, -84(%rbp)         # x[15]
	movaps	%xmm2, -112(%rbp)       # x[8..11]

# ---- LOAD x[] INTO REGISTERS for the round loop -----------------------------
# 16 words do not fit in the GP registers, so the allocator keeps 10 in regs and
# 6 in named spill slots. The mapping used throughout the loop is:
#   esi=x4  eax=x5  r9d=x6  r14d=x7   edi=x12 edx=x13 r11d=x14 r12d=x15
#   r10d=x8                       (spills) -48=x0 -68=x1 -64=x2 -56=x3
#                                          -44=x9 -60=x10 -52=x11 -152=x8-temp
	movl	-128(%rbp), %esi        # esi  = x[4]
	movl	-124(%rbp), %eax        # eax  = x[5]
	movl	-144(%rbp), %ecx
	movl	%ecx, -48(%rbp)         # spill x[0] -> -48
	movl	-140(%rbp), %ecx
	movl	%ecx, -68(%rbp)         # spill x[1] -> -68
	movl	-96(%rbp), %edi         # edi  = x[12]
	movl	-92(%rbp), %edx         # edx  = x[13]
	movl	-112(%rbp), %r10d       # r10d = x[8]
	movl	-108(%rbp), %ecx
	movl	%ecx, -44(%rbp)         # spill x[9] -> -44
	movl	-120(%rbp), %r9d        # r9d  = x[6]
	movl	-136(%rbp), %ecx
	movl	%ecx, -64(%rbp)         # spill x[2] -> -64
	movl	-88(%rbp), %r11d        # r11d = x[14]
	movl	-104(%rbp), %ecx
	movl	%ecx, -60(%rbp)         # spill x[10] -> -60
	movl	-116(%rbp), %r14d       # r14d = x[7]
	movl	-132(%rbp), %ecx
	movl	%ecx, -56(%rbp)         # spill x[3] -> -56
	movl	-84(%rbp), %r12d        # r12d = x[15]
	movl	$10, %r8d               # r8d = round counter (10 double rounds)
	movl	-100(%rbp), %ecx
	movl	%ecx, -52(%rbp)         # spill x[11] -> -52

# =============================================================================
# THE DOUBLE-ROUND LOOP (runs 10x). Each pass is 8 quarter rounds: 4 on columns,
# 4 on diagonals. Every quarter round is the SAME add/xor/roll ×4 you saw fully
# annotated above; here we label each with its (a,b,c,d) words and comment the
# ARX steps compactly. The only "clever" bits are the spill/reload shuffles that
# free a register (r8) for the loop counter and move x8 between reg and stack.
# =============================================================================
	.p2align	4
.LBB1_3:                                # =>This Inner Loop Header
	movl	%r8d, -148(%rbp)        # spill the loop counter (free r8 as scratch)
# -- QR column 0: a=x0 b=x4 c=x8 d=x12 --
	movl	-48(%rbp), %r8d         # r8d = x0 (reload a)
	addl	%esi, %r8d              # a += b        (x0 += x4)
	xorl	%r8d, %edi              # d ^= a        (x12)
	roll	$16, %edi               # d = rotl(d,16)
	addl	%edi, %r10d             # c += d        (x8)
	xorl	%r10d, %esi             # b ^= c        (x4)
	roll	$12, %esi               # b = rotl(b,12)
	addl	%esi, %r8d              # a += b
	xorl	%r8d, %edi              # d ^= a
	roll	$8, %edi                # d = rotl(d,8)
	addl	%edi, %r10d             # c += d
	movl	%r10d, -152(%rbp)       # spill c (x8) -> -152 (frees r10 next)
	xorl	%r10d, %esi             # b ^= c
	roll	$7, %esi                # b = rotl(b,7) -> esi now = new x4
# -- QR column 1: a=x1 b=x5 c=x9 d=x13 --
	movl	-68(%rbp), %r10d        # r10d = x1 (reload a; r10 is free now)
	addl	%eax, %r10d             # a += b        (x1 += x5)
	xorl	%r10d, %edx             # d ^= a        (x13)
	roll	$16, %edx
	movl	-44(%rbp), %ecx         # ecx = x9 (reload c)
	addl	%edx, %ecx              # c += d
	xorl	%ecx, %eax              # b ^= c        (x5)
	roll	$12, %eax
	addl	%eax, %r10d             # a += b
	xorl	%r10d, %edx             # d ^= a
	roll	$8, %edx
	addl	%edx, %ecx              # c += d
	movl	%ecx, -44(%rbp)         # spill c (x9)
	xorl	%ecx, %eax              # b ^= c
	roll	$7, %eax                # -> eax = new x5
# -- QR column 2: a=x2 b=x6 c=x10 d=x14 --
	movl	-64(%rbp), %ebx         # ebx = x2 (reload a)
	addl	%r9d, %ebx              # a += b        (x2 += x6)
	xorl	%ebx, %r11d             # d ^= a        (x14)
	roll	$16, %r11d
	movl	-60(%rbp), %r15d        # r15d = x10 (reload c)
	addl	%r11d, %r15d            # c += d
	xorl	%r15d, %r9d             # b ^= c        (x6)
	roll	$12, %r9d
	addl	%r9d, %ebx              # a += b
	xorl	%ebx, %r11d             # d ^= a
	roll	$8, %r11d
	addl	%r11d, %r15d            # c += d
	xorl	%r15d, %r9d             # b ^= c
	roll	$7, %r9d                # -> r9d = new x6  (a=x2 in ebx, c=x10 in r15d)
# -- QR column 3: a=x3 b=x7 c=x11 d=x15 --
	movl	-56(%rbp), %ecx         # ecx = x3 (reload a)
	addl	%r14d, %ecx             # a += b        (x3 += x7)
	xorl	%ecx, %r12d             # d ^= a        (x15)
	roll	$16, %r12d
	movl	-52(%rbp), %r13d        # r13d = x11 (reload c)
	addl	%r12d, %r13d            # c += d
	xorl	%r13d, %r14d            # b ^= c        (x7)
	roll	$12, %r14d
	addl	%r14d, %ecx             # a += b
	xorl	%ecx, %r12d             # d ^= a
	roll	$8, %r12d
	addl	%r12d, %r13d            # c += d
	xorl	%r13d, %r14d            # b ^= c
	roll	$7, %r14d               # -> r14d = new x7 (a=x3 in ecx, c=x11 in r13d,
                                        #    d=x15 in r12d) — COLUMNS DONE
# -- QR diagonal 0: a=x0 b=x5 c=x10 d=x15 --
	addl	%eax, %r8d              # a += b   (x0 in r8d += x5 in eax)
	xorl	%r8d, %r12d             # d ^= a   (x15)
	roll	$16, %r12d
	addl	%r12d, %r15d            # c += d   (x10 in r15d)
	xorl	%r15d, %eax             # b ^= c   (x5)
	roll	$12, %eax
	addl	%eax, %r8d              # a += b
	movl	%r8d, -48(%rbp)         # spill a (x0) back to its home -48
	xorl	%r8d, %r12d             # d ^= a
	movl	-148(%rbp), %r8d        # reload the LOOP COUNTER into r8 (x0 parked)
	roll	$8, %r12d
	addl	%r12d, %r15d            # c += d
	movl	%r15d, -60(%rbp)        # spill c (x10) -> -60
	xorl	%r15d, %eax             # b ^= c
	roll	$7, %eax                # -> eax = new x5
# -- QR diagonal 1: a=x1 b=x6 c=x11 d=x12 --
	addl	%r9d, %r10d             # a += b   (x1 in r10d += x6 in r9d)
	xorl	%r10d, %edi             # d ^= a   (x12)
	roll	$16, %edi
	addl	%edi, %r13d             # c += d   (x11 in r13d)
	xorl	%r13d, %r9d             # b ^= c   (x6)
	roll	$12, %r9d
	addl	%r9d, %r10d             # a += b
	movl	%r10d, -68(%rbp)        # spill a (x1) -> -68
	xorl	%r10d, %edi             # d ^= a
	movl	-152(%rbp), %r10d       # reload x8 (parked at -152) into r10d
	roll	$8, %edi
	addl	%edi, %r13d             # c += d
	movl	%r13d, -52(%rbp)        # spill c (x11) -> -52
	xorl	%r13d, %r9d             # b ^= c
	roll	$7, %r9d                # -> r9d = new x6
# -- QR diagonal 2: a=x2 b=x7 c=x8 d=x13 --
	addl	%r14d, %ebx             # a += b   (x2 in ebx += x7 in r14d)
	xorl	%ebx, %edx              # d ^= a   (x13)
	roll	$16, %edx
	addl	%edx, %r10d             # c += d   (x8 in r10d)
	xorl	%r10d, %r14d            # b ^= c   (x7)
	roll	$12, %r14d
	addl	%r14d, %ebx             # a += b
	movl	%ebx, -64(%rbp)         # spill a (x2) -> -64
	xorl	%ebx, %edx              # d ^= a
	roll	$8, %edx
	addl	%edx, %r10d             # c += d
	xorl	%r10d, %r14d            # b ^= c
	roll	$7, %r14d               # -> r14d = new x7 (x8 stays in r10d)
# -- QR diagonal 3: a=x3 b=x4 c=x9 d=x14 --
	addl	%esi, %ecx              # a += b   (x3 in ecx += x4 in esi)
	xorl	%ecx, %r11d             # d ^= a   (x14)
	roll	$16, %r11d
	movl	-44(%rbp), %ebx         # ebx = x9 (reload c)
	addl	%r11d, %ebx             # c += d
	xorl	%ebx, %esi              # b ^= c   (x4)
	roll	$12, %esi
	addl	%esi, %ecx              # a += b
	movl	%ecx, -56(%rbp)         # spill a (x3) -> -56
	xorl	%ecx, %r11d             # d ^= a
	roll	$8, %r11d
	addl	%r11d, %ebx             # c += d
	movl	%ebx, -44(%rbp)         # spill c (x9) -> -44
	xorl	%ebx, %esi              # b ^= c
	roll	$7, %esi                # -> esi = new x4  — DIAGONALS DONE
	decl	%r8d                    # round counter--
	jne	.LBB1_3                 # 10 double rounds total

# ---- STORE the 16 working words x[] back to the stack -----------------------
# %bb.4:  (mirror of the register->x load, now reg/spill -> x[] home slots)
	movl	%esi, -128(%rbp)        # x[4]  = esi
	movl	-48(%rbp), %ecx
	movl	%ecx, -144(%rbp)        # x[0]  (from spill)
	movl	%edi, -96(%rbp)         # x[12] = edi
	movl	%r10d, -112(%rbp)       # x[8]  = r10d
	movl	%eax, -124(%rbp)        # x[5]  = eax
	movl	-68(%rbp), %eax
	movl	%eax, -140(%rbp)        # x[1]
	movl	%edx, -92(%rbp)         # x[13] = edx
	movl	-44(%rbp), %eax
	movl	%eax, -108(%rbp)        # x[9]
	movl	%r9d, -120(%rbp)        # x[6]  = r9d
	movl	-64(%rbp), %eax
	movl	%eax, -136(%rbp)        # x[2]
	movl	%r11d, -88(%rbp)        # x[14] = r11d
	movl	-60(%rbp), %eax
	movl	%eax, -104(%rbp)        # x[10]
	movl	%r14d, -116(%rbp)       # x[7]  = r14d
	movl	-56(%rbp), %eax
	movl	%eax, -132(%rbp)        # x[3]
	movl	%r12d, -84(%rbp)        # x[15] = r12d
	movl	-52(%rbp), %eax
	movl	%eax, -100(%rbp)        # x[11]

# ---- FEED-FORWARD + SERIALISE: out[i] = x[i] + s[i], little-endian ----------
	xorl	%eax, %eax              # i = 0
	movq	-160(%rbp), %rdx        # reload the `out` pointer we spilled
	.p2align	4
.LBB1_5:                                # for (i=0;i<16;i++)
	movl	-224(%rbp,%rax,4), %ecx # ecx = s[i]      (original state)
	addl	-144(%rbp,%rax,4), %ecx # ecx = s[i] + x[i]  <-- the one-way add
	movl	%ecx, (%rdx,%rax,4)     # out[i] = that (32-bit store == store_le32 on x86)
	incq	%rax                    # i++
	cmpq	$16, %rax               # i < 16 ?
	jne	.LBB1_5
# ---- EPILOGUE: unwind the frame, restore callee-saved regs, return ----------
# %bb.6:
	addq	$56, %rsp               # drop the local frame
	popq	%rbx                    # \ restore callee-saved regs in REVERSE push
	popq	%r12                    #  \ order (LIFO), so each caller value is
	popq	%r13                    #  / intact.
	popq	%r14                    # /
	popq	%r15
	popq	%rbp
	retq
.Lfunc_end1:
	.size	chacha20_block, .Lfunc_end1-chacha20_block

	.ident	"clang version 20.1.8"
	.section	".note.GNU-stack","",@progbits   # non-executable stack
	.addrsig
# =============================================================================
# WHAT TO TAKE AWAY
#   * ChaCha is ARX and nothing else: the whole loop is add / xor / roll on
#     registers. No data-dependent branch or memory address appears -> the code
#     is constant-time by construction, the property AEAD security relies on.
#   * `roll $16/$12/$8/$7` is literally the C `rotl32(...)` idiom compiled to one
#     instruction — the optimizer recognised (x<<n)|(x>>(32-n)).
#   * With only 15 GP registers for 16 state words, the allocator keeps a working
#     set in registers and rotates a few through named stack spills — watch x0,
#     x8, x9, x10, x11 move between register and stack as the diagonals demand.
#   * The final `s[i] + x[i]` feed-forward is what makes the 20-round permutation
#     one-way; without it ChaCha would be trivially invertible.
#   Compare demo.O0.s (every C statement spilled, quarter round is a real `call`)
#   with demo.O2.s (the loop vectorised into SSE/AVX lanes) to see the optimizer
#   climb from literal translation to four-lane SIMD.
# =============================================================================
