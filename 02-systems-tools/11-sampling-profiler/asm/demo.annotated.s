# =============================================================================
# demo.annotated.s — clang -O1 output for asm/demo.c, explained instruction by
# instruction. This is the profiler's two hot loops (frame-pointer stack walk +
# perf ring-buffer index math) plus the fold hash, laid bare.
# =============================================================================
#
# HOW TO READ THIS FILE
# ---------------------
# This is the EXACT assembly clang emits for demo.c at -O1 (see demo.s for the
# untouched original), with a comment on essentially every instruction. AT&T
# syntax throughout:
#
#     op   src, dst                     # movq %rsi, %rdi   =>  rdi = rsi
#     %reg                              # a register
#     $imm                              # an immediate (literal) constant
#     N(%reg)                           # memory at address [reg + N]
#     (%base,%idx,%scale)               # memory at [base + idx*scale]
#
# Register widths are windows onto the SAME register:
#     rax (64) / eax (32) / ax (16) / al (8).  Writing eax ZERO-EXTENDS into rax,
#     so `xorl %eax,%eax` clears all 64 bits — that is why clang uses the 32-bit
#     form to zero a 64-bit register (it is one byte shorter).
#
# THE SYSTEM V AMD64 ABI (the contract every function here obeys)
# --------------------------------------------------------------
#   * Integer/pointer ARGUMENTS, in order:  rdi, rsi, rdx, rcx, r8, r9, then stack
#   * RETURN value:                          rax  (eax for a 32-bit int like our
#                                            `int` returns)
#   * CALLEE-SAVED (a function must preserve): rbx, rbp, r12, r13, r14, r15, rsp
#   * CALLER-SAVED (scratch, free to clobber): rax, rcx, rdx, rsi, rdi, r8-r11
#   * THE RED ZONE: 128 bytes below rsp that a LEAF function may use without
#     moving rsp. fp_unwind and the ring helpers are leaves and make no `call`,
#     so they never touch rsp for locals — everything lives in registers.
#   * STACK ALIGNMENT: rsp must be 16-byte aligned at the point of a `call`.
#     demo_selftest, which does allocate a frame, keeps that invariant.
#
# WHY EVERY FUNCTION STILL HAS push %rbp / mov %rsp,%rbp
# -----------------------------------------------------
# We compiled with -fno-omit-frame-pointer (for teaching, and because this whole
# project is ABOUT frame pointers). So even leaf functions that need no frame
# emit the two-instruction prologue that maintains the %rbp chain — the very
# chain fp_unwind walks. Reading these prologues IS reading what makes the walk
# possible.
#
# WHAT THE OPTIMIZER DID (the payoff — watch for these below)
# -----------------------------------------------------------
#   * fp_unwind's `if (saved <= fp) break; fp = saved;` + loop-back became a
#     single `cmp` + `cmovaq` + `ja`: conditionally move, then branch, with no
#     separate break. (fp_unwind, .LBB0_5)
#   * `counter & (size - 1)` computes `size-1` with a `lea`, not a `sub`+`mov`.
#     (rb_offset)
#   * `min(need, to_end)` is branchless: `cmp` + `cmovbq`, no jump. (rb_first_chunk)
#   * clang INLINED fp_unwind into demo_selftest TWICE (the two callsites), so the
#     same walk loop appears three times in this file: once standalone, twice
#     inlined. Diffing them is a lesson in what inlining preserves. (demo_selftest)
# =============================================================================

	.file	"demo.c"
	.text                                   # executable code section

# =============================================================================
# fp_unwind(u64 fp, u64 lo, u64 hi, u64 *out, int max) -> int
# -----------------------------------------------------------------------------
# THE frame-pointer stack walk. Args land per SysV:
#     fp  in %rdi     lo  in %rsi     hi  in %rdx
#     out in %rcx     max in %r8d (32-bit int; sign-extended below)
# Return (frame count) in %eax. This is a leaf — no call, no stack frame beyond
# the teaching prologue; the loop counter `n` lives entirely in %rax.
# =============================================================================
	.globl	fp_unwind
	.p2align	4                       # 16-byte align the entry for fetch
	.type	fp_unwind,@function
fp_unwind:
# ---- PROLOGUE (frame-pointer bookkeeping only) ------------------------------
	pushq	%rbp                            # save caller's frame pointer...
	movq	%rsp, %rbp                      # ...and set ours (kept for the chain)
	movslq	%r8d, %r8                       # max: sign-extend int arg (r8d) to a
                                                #   64-bit r8, so the `n < max`
                                                #   compare below is a clean 64-bit
                                                #   signed compare.
	xorl	%eax, %eax                      # n = 0. Zeroing eax clears all of rax;
                                                #   rax IS the return value AND the
                                                #   live loop counter `n`.
	.p2align	4
# ---- LOOP HEADER: the four guards of `while (...)` ---------------------------
.LBB0_1:                                        # top of the while loop
	cmpq	%rsi, %rdi                      # compare fp (rdi) with lo (rsi)
	jb	.LBB0_6                         # if fp < lo (unsigned "below") -> exit.
                                                #   Guard 1: fp below the stack base.
	cmpq	%rdx, %rdi                      # compare fp with hi (rdx)
	jae	.LBB0_6                         # if fp >= hi (unsigned "above/equal")
                                                #   -> exit. Guard 2: fp past top.
	movl	%edi, %r9d                      # r9d = low 32 bits of fp (enough to
                                                #   test alignment — the bottom 3
                                                #   bits are all we need).
	andl	$7, %r9d                        # isolate fp & 7 (the low 3 bits)
	jne	.LBB0_6                         # if not 8-byte aligned -> exit.
                                                #   Guard 3: a real saved rbp is
                                                #   always 8-aligned; anything else
                                                #   is garbage we refuse to deref.
	cmpq	%r8, %rax                       # compare n (rax) with max (r8)
	jge	.LBB0_6                         # if n >= max -> exit. Guard 4: the
                                                #   depth cap that bounds the walk.
# ---- LOOP BODY: read the frame, record the return address, step up ----------
.LBB0_5:                                        # (all guards passed)
	movq	(%rdi), %r9                     # r9 = *(u64*)fp = saved caller rbp.
                                                #   THE pointer-chase load #1.
	movq	8(%rdi), %r10                   # r10 = *(u64*)(fp+8) = return address.
                                                #   THE pointer-chase load #2 — the
                                                #   value we actually want.
	movq	%r10, (%rcx,%rax,8)             # out[n] = ret. Address = out + n*8;
                                                #   scale-8 index addressing writes
                                                #   the 64-bit return address.
	incq	%rax                            # n++
	cmpq	%rdi, %r9                       # compare saved (r9) with fp (rdi)
	cmovaq	%r9, %rdi                       # if saved > fp (unsigned "above"),
                                                #   fp = saved. This is `fp = saved`
                                                #   FUSED with the terminator: the
                                                #   move only happens when the chain
                                                #   climbs. If saved <= fp, fp is
                                                #   left unchanged and...
	ja	.LBB0_1                         # ...if saved > fp, loop again; else
                                                #   fall through to exit. Together the
                                                #   cmov+ja ARE `if (saved<=fp) break;
                                                #   fp = saved;` with zero extra
                                                #   branches. Slickest line here.
# ---- EPILOGUE ---------------------------------------------------------------
.LBB0_6:
                                                # (clang notes eax is the result)
	popq	%rbp                            # restore caller's frame pointer
	retq                                    # return n in eax
.Lfunc_end0:
	.size	fp_unwind, .Lfunc_end0-fp_unwind

# =============================================================================
# rb_avail(u64 head, u64 tail) -> u64        head in %rdi, tail in %rsi
# -----------------------------------------------------------------------------
# The unconsumed-byte count of the perf ring: just head - tail, in unsigned
# 64-bit so it stays correct across the 2^64 counter wrap.
# =============================================================================
	.globl	rb_avail
	.p2align	4
	.type	rb_avail,@function
rb_avail:
	pushq	%rbp                            # prologue (frame pointer kept)
	movq	%rsp, %rbp
	movq	%rdi, %rax                      # rax = head
	subq	%rsi, %rax                      # rax = head - tail  (the whole function)
	popq	%rbp                            # epilogue
	retq                                    # return backlog in rax
.Lfunc_end1:
	.size	rb_avail, .Lfunc_end1-rb_avail

# =============================================================================
# rb_offset(u64 counter, u64 size) -> u64    counter in %rdi, size in %rsi
# -----------------------------------------------------------------------------
# counter % size, where size is a power of two, done as counter & (size-1).
# =============================================================================
	.globl	rb_offset
	.p2align	4
	.type	rb_offset,@function
rb_offset:
	pushq	%rbp                            # prologue
	movq	%rsp, %rbp
	leaq	-1(%rsi), %rax                  # rax = size - 1 (the mask). LEA does the
                                                #   subtract-and-place in ONE op, no
                                                #   separate mov+dec — a common clang
                                                #   idiom for "compute an address-like
                                                #   value cheaply".
	andq	%rdi, %rax                      # rax = counter & (size-1) = counter % size
	popq	%rbp                            # epilogue
	retq
.Lfunc_end2:
	.size	rb_offset, .Lfunc_end2-rb_offset

# =============================================================================
# rb_first_chunk(u64 off, u64 need, u64 size) -> u64
#     off in %rdi, need in %rsi, size in %rdx
# -----------------------------------------------------------------------------
# min(need, size - off): how many bytes are contiguous before the ring wraps.
# Note it comes out BRANCHLESS — the ?: became a cmov.
# =============================================================================
	.globl	rb_first_chunk
	.p2align	4
	.type	rb_first_chunk,@function
rb_first_chunk:
	pushq	%rbp                            # prologue
	movq	%rsp, %rbp
	movq	%rdx, %rax                      # rax = size
	subq	%rdi, %rax                      # rax = size - off = to_end (bytes to wrap)
	cmpq	%rax, %rsi                      # compare need (rsi) with to_end (rax)
	cmovbq	%rsi, %rax                      # if need < to_end (unsigned below),
                                                #   rax = need. Otherwise rax stays
                                                #   to_end. That IS min(need,to_end),
                                                #   with no branch — the ternary
                                                #   compiled to a conditional move.
	popq	%rbp                            # epilogue
	retq
.Lfunc_end3:
	.size	rb_first_chunk, .Lfunc_end3-rb_first_chunk

# =============================================================================
# fnv1a(const u8 *p, u64 n) -> u64           p in %rdi, n in %rsi
# -----------------------------------------------------------------------------
# 64-bit FNV-1a hash: h starts at the offset basis; for each byte, XOR then
# multiply by the FNV prime. This is stackmap.c's bucket hash.
# =============================================================================
	.globl	fnv1a
	.p2align	4
	.type	fnv1a,@function
fnv1a:
	pushq	%rbp                            # prologue
	movq	%rsp, %rbp
	movabsq	$-3750763034362895579, %r8      # r8 = h = FNV offset basis. The value
                                                #   0xCBF29CE484222325 has bit 63 set,
                                                #   so as a signed literal it prints
                                                #   negative; it is the SAME 64 bits.
                                                #   movabsq loads a full 64-bit
                                                #   immediate (ordinary movq only
                                                #   takes a 32-bit sign-extended imm).
	testq	%rsi, %rsi                      # n == 0 ?
	je	.LBB4_1                         # if length 0, skip the loop, return basis
	xorl	%ecx, %ecx                      # i = 0 (the byte index)
	movabsq	$1099511628211, %rdx            # rdx = FNV prime 0x100000001B3, hoisted
                                                #   out of the loop (loop-invariant).
	.p2align	4
.LBB4_4:                                        # for each byte:
	movzbl	(%rdi,%rcx), %eax               # eax = p[i], zero-extended from 8 bits
                                                #   (movzbl clears the upper bits so
                                                #   the 64-bit xor next is clean).
	xorq	%r8, %rax                       # rax = p[i] ^ h   (the "a": xor FIRST)
	imulq	%rdx, %rax                      # rax = (p[i]^h) * prime  (then multiply)
	incq	%rcx                            # i++
	movq	%rax, %r8                       # h = rax (carry the running hash forward)
	cmpq	%rcx, %rsi                      # i == n ?
	jne	.LBB4_4                         # loop until all n bytes mixed
	popq	%rbp                            # epilogue (h already in... rax)
	retq                                    # return h in rax
.LBB4_1:                                        # n == 0 fast path
	movq	%r8, %rax                       # move the untouched basis into rax
	popq	%rbp
	retq
.Lfunc_end4:
	.size	fnv1a, .Lfunc_end4-fnv1a

# =============================================================================
# demo_selftest() -> int   —  SCAFFOLDING, annotated lightly.
# -----------------------------------------------------------------------------
# This proves the routines above with constant inputs. It is NOT project core
# logic, so it is not walked line by line. Two things ARE worth seeing:
#
#   (1) clang INLINED fp_unwind here — twice. The loop at .LBB5_1 is the first
#       fp_unwind call (walk the synthetic chain); the loop at .LBB5_10 is the
#       second call (the misaligned-fp rejection test). Both are the exact
#       cmp/cmov/ja shape you just read in fp_unwind proper. Inlining copied the
#       whole loop rather than emitting a `call`, because -O1 judged it cheap.
#
#   (2) The `subq $432, %rsp` below is this function allocating a REAL stack
#       frame (unlike the leaves above): it needs room for the synthetic `stk`
#       array and the `got` output buffer. That is the red zone being too small,
#       so rsp actually moves.
# =============================================================================
	.globl	demo_selftest
	.p2align	4
	.type	demo_selftest,@function
demo_selftest:
	pushq	%rbp                            # prologue
	movq	%rsp, %rbp
	subq	$432, %rsp                      # reserve 432 bytes of locals (stk[6] +
                                                #   got[64] + slack). rsp MOVES here —
                                                #   this frame is too big for the red
                                                #   zone.
# --- build the synthetic frame chain stk[] (see demo.c) ---
	leaq	-48(%rbp), %rax                 # rax = &stk[0] (the initial fp)
	leaq	-32(%rbp), %rcx                 # rcx = &stk[2]
	movq	%rcx, -48(%rbp)                 # stk[0] = &stk[2]   (frame 0's saved rbp)
	movq	$43690, -40(%rbp)               # stk[1] = 0xAAAA    (frame 0's ret addr)
	leaq	-16(%rbp), %rcx                 # rcx = &stk[4]
	movq	%rcx, -32(%rbp)                 # stk[2] = &stk[4]   (frame 1's saved rbp)
	movq	$48059, -24(%rbp)               # stk[3] = 0xBBBB    (frame 1's ret addr)
	movq	$0, -16(%rbp)                   # stk[4] = 0         (terminator saved rbp)
	movq	$52428, -8(%rbp)                # stk[5] = 0xCCCC    (frame 2's ret addr)
	leaq	(%rbp), %rcx                    # rcx = hi bound = &stk[6] (one past end)
	xorl	%edx, %edx                      # n = 0 for the inlined walk
	.p2align	4
.LBB5_1:                                        # inlined fp_unwind #1 (walk stk)
	cmpq	%rcx, %rax                      # fp >= hi ?  (lo check folded away:
                                                #   the compiler proved fp >= lo here)
	jae	.LBB5_5
	movl	%eax, %esi                      # alignment guard, as in fp_unwind
	andl	$7, %esi
	jne	.LBB5_5
	cmpq	$63, %rdx                       # depth-cap guard (max = 64 -> n <= 63)
	ja	.LBB5_5
	movq	(%rax), %rsi                    # saved = *fp
	movq	8(%rax), %rdi                   # ret   = *(fp+8)
	movq	%rdi, -560(%rbp,%rdx,8)         # got[n] = ret
	incq	%rdx                            # n++
	cmpq	%rax, %rsi                      # saved > fp ?
	cmovaq	%rsi, %rax                      # fp = saved (if climbing)
	ja	.LBB5_1                         # loop — identical shape to fp_unwind
.LBB5_5:
	movl	$1, %eax                        # prepare return code 1 (n != 3 fail)
	cmpl	$3, %edx                        # got exactly 3 frames?
	jne	.LBB5_15
	movl	$2, %eax                        # code 2: got[0] != 0xAAAA
	cmpq	$43690, -560(%rbp)              # check got[0] == 0xAAAA
	jne	.LBB5_15
	movl	$3, %eax                        # code 3: got[1] != 0xBBBB
	cmpq	$48059, -552(%rbp)              # check got[1]
	jne	.LBB5_15
	movl	$4, %eax                        # code 4: got[2] != 0xCCCC
	cmpq	$52428, -544(%rbp)              # check got[2]
	jne	.LBB5_15
	leaq	-47(%rbp), %rdx                 # &stk[0] + 1 = a deliberately MISALIGNED
                                                #   fp for the next test
	xorl	%eax, %eax                      # n = 0
	.p2align	4
.LBB5_10:                                       # inlined fp_unwind #2 (must return 0)
	cmpq	%rcx, %rdx                      # fp >= hi ?
	jae	.LBB5_14
	movl	%edx, %esi
	andl	$7, %esi                        # this alignment guard fires immediately
	jne	.LBB5_14                        #   because fp = base+1 is misaligned ->
                                                #   the walk records zero frames.
	cmpq	$63, %rax
	ja	.LBB5_14
	movq	(%rdx), %rsi
	movq	8(%rdx), %rdi
	movq	%rdi, -560(%rbp,%rax,8)
	incq	%rax
	cmpq	%rdx, %rsi
	cmovaq	%rsi, %rdx
	ja	.LBB5_10
.LBB5_14:
	xorl	%ecx, %ecx                      # compute return code for the align test:
	testl	%eax, %eax                      #   n == 0 ?
	setne	%cl                             # cl = (n != 0)
	leal	(%rcx,%rcx,4), %eax             # eax = 5*cl -> 5 if the test FAILED
                                                #   (n!=0), 0 if it passed. (The
                                                #   remaining rb_*/fnv1a checks were
                                                #   constant-folded away by -O1, so
                                                #   only this branch remains before
                                                #   the function's tail.)
.LBB5_15:
	addq	$432, %rsp                      # deallocate the frame (undo the subq)
	popq	%rbp                            # restore caller's frame pointer
	retq                                    # return the first failing code (or 0)
.Lfunc_end5:
	.size	demo_selftest, .Lfunc_end5-demo_selftest

	.ident	"clang version 20.1.8"          # toolchain stamp (harmless metadata)
	.section	".note.GNU-stack","",@progbits  # we do NOT need an executable
                                                        #   stack (a security default)
# =============================================================================
# WHAT TO TAKE AWAY
#   * The frame-pointer walk is a tiny pointer-chasing loop: two loads per frame
#     (saved rbp, return addr), a monotonic-increase guard to terminate, and a
#     depth cap. That is the ENTIRE mechanism behind a PERF_SAMPLE_CALLCHAIN and
#     behind our SIGPROF handler.
#   * Ring-buffer addressing is `head - tail` for the backlog and `& (size-1)`
#     for the slot — one sub and one and. The power-of-two size is what makes the
#     modulo a mask.
#   * -O1 fused the walk's break+advance into cmov+ja, made min() branchless, and
#     inlined fp_unwind twice. Keep demo.O0.s open next to this file to see the
#     same source before any of that happened.
# =============================================================================
