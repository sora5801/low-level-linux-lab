# =============================================================================
# demo.annotated.s — clang -O1 output for asm/demo.c, explained line by line.
# =============================================================================
#
# HOW TO READ THIS FILE
# ---------------------
# This is the exact assembly clang emits for asm/demo.c at -O1 (see demo.s for
# the untouched original), with a comment on essentially every instruction. It
# is x86-64, AT&T syntax:
#
#     op   source, destination           # e.g.  movl $1, %eax  =>  eax = 1
#     %reg                                # a register
#     $imm                                # an immediate (literal) constant
#     N(%reg)                             # memory at address [reg + N]
#
# Register widths are views of ONE register: rax(64) / eax(32) / ax(16) / al(8).
# Writing a 32-bit name (eax) ZERO-EXTENDS into the full 64-bit rax, which is why
# byte loads use `movzbl` (move-zero-extend byte -> long): it clears the upper
# bits so no stale garbage pollutes the arithmetic.
#
# THE SysV AMD64 ABI CONTRACT (what a caller and callee agree on)
# ---------------------------------------------------------------
#   * Integer/pointer ARGUMENTS, in order:  rdi, rsi, rdx, rcx, r8, r9, then the
#     stack. So here flow_hash's one argument `const struct flow_key *k` arrives
#     in %rdi, and flow_bucket's second argument `u32 nbuckets` arrives in %esi.
#   * RETURN value: rax (eax for a 32-bit u32). Both functions return in eax.
#   * CALLEE-SAVED (a function must restore these before returning): rbx, rbp,
#     r12, r13, r14, r15, and rsp. Everything else (rax, rcx, rdx, rsi, rdi,
#     r8-r11) is CALLER-saved / scratch — a callee may clobber it freely.
#   * A leaf function that makes no `call` may use the 128-byte "red zone" below
#     rsp as scratch without adjusting rsp. Both functions here are leaves.
#
# THE BIG PICTURE
# ---------------
# demo.c has two functions: flow_hash (the FNV-1a hash over the 13 meaningful
# bytes of the 5-tuple) and flow_bucket (flow_hash & (nbuckets-1)). Two lessons
# jump out of the assembly:
#
#   1. FNV-1a is a perfectly regular instruction chain: for each of the 13 bytes,
#      { load byte, XOR into the running hash, multiply by the FNV prime }. The
#      whole hash is 13 identical 3-instruction groups. No loop, no table, no
#      branches — which is exactly why it is cheap enough to run per-packet at
#      tens of millions of packets per second.
#
#   2. clang INLINED flow_hash into flow_bucket (there is no `call flow_hash` in
#      flow_bucket — the 13 groups reappear verbatim), then implemented the
#      power-of-two modulo `hash & (nbuckets-1)` as a single `leal`/`andl` pair.
#      The standalone flow_hash is still emitted because it is globally visible.
#
# The only thing -O2 changes (see demo.O2.s) is dropping the two frame-pointer
# instructions below — we forced them here with -fno-omit-frame-pointer so the
# frame is visible; at -O2, without that flag, these leaves run prologue-free.
# =============================================================================

	.file	"demo.c"
	.text                                   # executable code section

# =============================================================================
# flow_hash(const struct flow_key *k)  ->  u32
#   arg:    %rdi = k (pointer to the 16-byte flow_key)
#   result: %eax = 32-bit FNV-1a hash
# =============================================================================
	.globl	flow_hash                       # export so the linker/other TUs see it
	.p2align	4                       # 16-byte align the entry (I-fetch friendly)
	.type	flow_hash,@function
flow_hash:                                      # @flow_hash

# ---- PROLOGUE ---------------------------------------------------------------
# This leaf needs no frame at all, but -fno-omit-frame-pointer makes clang keep
# one so a debugger can unwind. It costs exactly two instructions.
	pushq	%rbp                            # save caller's frame pointer
	movq	%rsp, %rbp                      # rbp = base of our (empty) frame

# ---- BYTE 0 of saddr: seed the hash -----------------------------------------
# The first group is special: instead of XORing byte[0] into the FNV offset
# basis with a separate mov, clang folds the constant in as an immediate XOR.
	movzbl	(%rdi),  %eax                   # eax = k->[0]   (zero-extended byte)
	xorl	$-2128831035, %eax              # eax ^= 0x811C9DC5 (FNV_OFFSET basis).
                                                #   -2128831035 is 0x811C9DC5 read as
                                                #   a signed 32-bit int — same bits.
	imull	$16777619, %eax, %eax           # eax *= 0x01000193 (FNV_PRIME).
                                                #   `imul` here is a plain 32-bit
                                                #   multiply; we only keep the low 32
                                                #   bits, so signed vs unsigned is moot.

# ---- BYTES 1..12: the repeating FNV-1a step ---------------------------------
# Every remaining byte is the identical 3-instruction idiom:
#     movzbl N(%rdi), %ecx     load the next byte
#     xorl   %eax, %ecx        ecx = byte XOR running_hash        (the "1a" XOR)
#     imull  $prime, %ecx,%eax eax = ecx * FNV_PRIME              (fold into hash)
# The running hash ping-pongs between eax (after each imul) and ecx (the XOR
# temp). Offsets 1..3 finish saddr; 4..7 are daddr; 8..9 sport; 10..11 dport;
# 12 is proto. Offset 12 (not 13..15) is the last because the struct's padding
# bytes are deliberately NOT hashed — the C hashes named fields only.

	movzbl	1(%rdi), %ecx                   # saddr byte 1
	xorl	%eax, %ecx                      #   ^ running hash
	imull	$16777619, %ecx, %eax           #   * prime

	movzbl	2(%rdi), %ecx                   # saddr byte 2
	xorl	%eax, %ecx
	imull	$16777619, %ecx, %eax

	movzbl	3(%rdi), %ecx                   # saddr byte 3  (source IP complete)
	xorl	%eax, %ecx
	imull	$16777619, %ecx, %eax

	movzbl	4(%rdi), %ecx                   # daddr byte 0
	xorl	%eax, %ecx
	imull	$16777619, %ecx, %eax

	movzbl	5(%rdi), %ecx                   # daddr byte 1
	xorl	%eax, %ecx
	imull	$16777619, %ecx, %eax

	movzbl	6(%rdi), %ecx                   # daddr byte 2
	xorl	%eax, %ecx
	imull	$16777619, %ecx, %eax

	movzbl	7(%rdi), %ecx                   # daddr byte 3  (dest IP complete)
	xorl	%eax, %ecx
	imull	$16777619, %ecx, %eax

	movzbl	8(%rdi), %ecx                   # sport byte 0  (offset 8: struct has
	xorl	%eax, %ecx                      #   no gap before sport because two u32
	imull	$16777619, %ecx, %eax           #   are 4-aligned and 8 is a multiple)

	movzbl	9(%rdi), %ecx                   # sport byte 1  (source port complete)
	xorl	%eax, %ecx
	imull	$16777619, %ecx, %eax

	movzbl	10(%rdi), %ecx                  # dport byte 0
	xorl	%eax, %ecx
	imull	$16777619, %ecx, %eax

	movzbl	11(%rdi), %ecx                  # dport byte 1  (dest port complete)
	xorl	%eax, %ecx
	imull	$16777619, %ecx, %eax

	movzbl	12(%rdi), %ecx                  # proto byte    (offset 12, the 13th byte)
	xorl	%eax, %ecx
	imull	$16777619, %ecx, %eax           # eax = final hash (stays in the return reg)

# ---- EPILOGUE ---------------------------------------------------------------
	popq	%rbp                            # restore caller's frame pointer
	retq                                    # return; result already in eax
.Lfunc_end0:
	.size	flow_hash, .Lfunc_end0-flow_hash

# =============================================================================
# flow_bucket(const struct flow_key *k, u32 nbuckets)  ->  u32
#   args:   %rdi = k,  %esi = nbuckets
#   result: %eax = flow_hash(k) & (nbuckets - 1)
#
# The source is a one-liner that CALLS flow_hash, but the optimizer INLINED it:
# below you see the same 13 FNV groups again (no `call`), then the mask. This is
# the whole fast path of a hash-based L4 load balancer in ~40 instructions.
# =============================================================================
	.globl	flow_bucket
	.p2align	4
	.type	flow_bucket,@function
flow_bucket:                                    # @flow_bucket
# %bb.0:
	pushq	%rbp                            # PROLOGUE (frame pointer, as above)
	movq	%rsp, %rbp
                                                # the assembler notes esi is defined
                                                #   here as the 32-bit nbuckets arg;
                                                #   it is used once, at the mask below.

	# ---- inlined flow_hash(k): identical 13-group chain over bytes 0..12 ----
	movzbl	(%rdi),  %eax                   # byte 0: seed
	xorl	$-2128831035, %eax              #   ^ FNV_OFFSET
	imull	$16777619, %eax, %eax           #   * FNV_PRIME
	movzbl	1(%rdi), %ecx
	xorl	%eax, %ecx
	imull	$16777619, %ecx, %eax
	movzbl	2(%rdi), %ecx
	xorl	%eax, %ecx
	imull	$16777619, %ecx, %eax
	movzbl	3(%rdi), %ecx
	xorl	%eax, %ecx
	imull	$16777619, %ecx, %eax
	movzbl	4(%rdi), %ecx
	xorl	%eax, %ecx
	imull	$16777619, %ecx, %eax
	movzbl	5(%rdi), %ecx
	xorl	%eax, %ecx
	imull	$16777619, %ecx, %eax
	movzbl	6(%rdi), %ecx
	xorl	%eax, %ecx
	imull	$16777619, %ecx, %eax
	movzbl	7(%rdi), %ecx
	xorl	%eax, %ecx
	imull	$16777619, %ecx, %eax
	movzbl	8(%rdi), %ecx
	xorl	%eax, %ecx
	imull	$16777619, %ecx, %eax
	movzbl	9(%rdi), %ecx
	xorl	%eax, %ecx
	imull	$16777619, %ecx, %eax
	movzbl	10(%rdi), %ecx
	xorl	%eax, %ecx
	imull	$16777619, %ecx, %eax
	movzbl	11(%rdi), %ecx
	xorl	%eax, %ecx
	imull	$16777619, %ecx, %eax
	movzbl	12(%rdi), %ecx
	xorl	%eax, %ecx
	imull	$16777619, %ecx, %ecx           # NOTE: last product lands in ECX, not
                                                #   eax — because eax is about to be
                                                #   reused for the mask value. The hash
                                                #   now lives in ecx.

	# ---- fold into a bucket: hash & (nbuckets - 1) --------------------------
	leal	-1(%rsi), %eax                  # eax = nbuckets - 1. `lea` does the
                                                #   subtract as an address calculation,
                                                #   avoiding a separate `dec` and not
                                                #   touching the flags register.
	andl	%ecx, %eax                      # eax = hash & (nbuckets-1). This equals
                                                #   hash % nbuckets ONLY because nbuckets
                                                #   is a power of two — the reason the
                                                #   real program fixes LB_BUCKETS = 8.
                                                #   A general modulo would need a costly
                                                #   `div`; the mask is one cycle.
	popq	%rbp                            # EPILOGUE
	retq                                    # return bucket in eax
.Lfunc_end1:
	.size	flow_bucket, .Lfunc_end1-flow_bucket

	.ident	"clang version 20.1.8"           # toolchain stamp (harmless metadata)
	.section	".note.GNU-stack","",@progbits  # mark stack non-executable (W^X)
	.addrsig                                 # address-significance table (LTO icf aid)
# =============================================================================
# WHAT TO TAKE AWAY
#   * A per-packet hash is just a straight instruction chain: 13 * { load, xor,
#     mul }. No branches means no misprediction — ideal for the XDP fast path.
#   * `hash & (N-1)` is the power-of-two modulo trick; it is why load balancers
#     size their backend/bucket tables to powers of two.
#   * The optimizer inlined flow_hash into flow_bucket and shuffled which
#     register holds the final product (ecx vs eax) purely to free eax for the
#     mask — a tiny glimpse of register allocation at work.
#   * Compare with demo.O0.s (every byte spilled to the stack, flow_bucket makes
#     a real `call flow_hash`) and demo.O2.s (identical math, but no frame
#     pointer — the prologue/epilogue vanish on these leaves).
# =============================================================================
