# =============================================================================
# demo.annotated.s — clang's -O1 output for demo.c, explained instruction by
#                    instruction. (The untouched original is demo.s.)
# =============================================================================
#
# HOW TO READ THIS FILE
# ---------------------
# AT&T syntax throughout:  op  source, destination      (e.g. movq %rsp,%rbp
# means rbp = rsp). `%reg` is a register, `$imm` an immediate, `N(%r)` is the
# memory at [r+N], and `(%rb,%ri)` is [rb + ri] (base + index).
#
# Register widths are views of ONE physical register:
#     rax(64) / eax(32) / ax(16) / al(8).  Writing a 32-bit name ZERO-EXTENDS
#     into the top 32 bits, which is why `xorl %ecx,%ecx` is the idiomatic way
#     to set the full 64-bit rcx to 0 in two bytes.
#
# THE SysV AMD64 ABI CONTRACT (what every function here obeys)
# -----------------------------------------------------------
#   * Integer/pointer ARGUMENTS, in order:  rdi, rsi, rdx, rcx, r8, r9, then stack
#   * RETURN value:                          rax
#   * CALLEE-SAVED (a function must restore): rbx, rbp, r12, r13, r14, r15, rsp
#   * CALLER-SAVED (free scratch):            rax, rcx, rdx, rsi, rdi, r8-r11
#   * rsp must be 16-byte aligned at a `call`; the red zone is 128 bytes below
#     rsp that a leaf may scribble on without adjusting rsp.
#
# All three functions here are LEAVES (they call nothing), so they need no stack
# locals at -O1 — every value lives in a register. The only stack traffic is the
# frame-pointer prologue/epilogue, which -O1 keeps for legible backtraces (it is
# strictly optional for a leaf and -O2 would drop it).
#
# THE MAPPING (which C became which label)
#   fnv1a64            -> the multiply-accumulate hash loop  (.LBB0_*)
#   region_first_diff  -> the compare loop with early exit   (.LBB1_*)
#   table_fingerprint  -> shift-left-3 then the same hash    (.LBB2_*)
# =============================================================================

	.file	"demo.c"
	.text                                   # executable code section

# =============================================================================
# u64 fnv1a64(const u8 *data /*rdi*/, usize len /*rsi*/)   ->  hash in rax
# -----------------------------------------------------------------------------
# The FNV-1a inner step is  hash = (hash ^ byte) * PRIME.  clang keeps the
# running hash in a register (rdx, then rax each iteration) and never touches
# memory except to read the next input byte — a tight, allocation-free loop,
# which is exactly why the kernel detector can afford to call it.
# =============================================================================
	.globl	fnv1a64
	.p2align	4                       # 16-byte align the entry for the fetcher
	.type	fnv1a64,@function
fnv1a64:
# ---- PROLOGUE ---------------------------------------------------------------
	pushq	%rbp                            # save caller's frame pointer
	movq	%rsp, %rbp                      # rbp = base of our frame (leaf: cosmetic)

# ---- seed the hash + guard the empty region ---------------------------------
	movabsq	$-3750763034362895579, %rdx     # rdx = hash = 0xCBF29CE484222325, the
                                                #   FNV-64 offset basis. It prints as
                                                #   a negative decimal only because
                                                #   the assembler shows the 64-bit
                                                #   pattern as signed; the bits are
                                                #   the unsigned basis. movabsq is the
                                                #   10-byte form needed for a full
                                                #   64-bit immediate.
	testq	%rsi, %rsi                      # set flags from len & len (i.e. len==0?)
	je	.LBB0_1                         # if len == 0, skip the loop and return
                                                #   the untouched basis (hash of "").

# ---- loop setup -------------------------------------------------------------
# %bb.2:
	xorl	%ecx, %ecx                      # rcx = i = 0 (index). xor zero-extends,
                                                #   so all 64 bits of rcx are cleared.
	movabsq	$1099511628211, %r8             # r8 = FNV prime 0x100000001B3, hoisted
                                                #   OUT of the loop (loop-invariant) so
                                                #   the multiply reuses a register, not
                                                #   a re-loaded immediate each pass.
	.p2align	4                       # align the loop top — a hot branch target

# ---- the hash loop:  for (i=0; i<len; i++) hash=(hash^data[i])*PRIME ---------
.LBB0_3:                                        # loop header (label .LBB0_3)
	movzbl	(%rdi,%rcx), %eax               # eax = data[i]: load ONE byte at
                                                #   [rdi + rcx] and zero-extend it to
                                                #   32 bits (hence movzBL). Top bits of
                                                #   rax are cleared, so rax == the byte.
	xorq	%rdx, %rax                      # rax = hash ^ byte   (the "1a" mix step)
	imulq	%r8, %rax                       # rax = (hash ^ byte) * PRIME. Signed vs
                                                #   unsigned imul is irrelevant here: the
                                                #   low 64 bits of the product are the
                                                #   same bit pattern either way, and we
                                                #   only keep the low 64.
	incq	%rcx                            # i++
	movq	%rax, %rdx                      # hash = rax: carry the new hash forward
                                                #   in rdx for the next iteration.
	cmpq	%rcx, %rsi                      # compare len (rsi) with i (rcx)
	jne	.LBB0_3                         # if i != len, loop again. Because we
                                                #   already know len>0 on entry, testing
                                                #   at the BOTTOM runs the body the right
                                                #   number of times with one branch/pass.

# ---- EPILOGUE (fall-through: rax already holds the final hash) ---------------
# %bb.4:
	popq	%rbp                            # restore caller's frame pointer
	retq                                    # return; rax = final hash

# ---- empty-region path ------------------------------------------------------
.LBB0_1:
	movq	%rdx, %rax                      # rax = the basis (len==0 => hash of "")
	popq	%rbp                            # restore frame pointer
	retq                                    # return that basis
.Lfunc_end0:
	.size	fnv1a64, .Lfunc_end0-fnv1a64    # symbol byte-length (for objdump)

# =============================================================================
# long region_first_diff(const u8 *a /*rdi*/, const u8 *b /*rsi*/,
#                        usize len /*rdx*/)     ->  first-diff index, else -1
# -----------------------------------------------------------------------------
# This is the routine that turns "the table hash changed" into "syscall N was
# hooked": the first differing BYTE at offset K means table entry K/8. Note how
# the -1 sentinel is pre-loaded so the "all equal" fall-through path is free.
# =============================================================================
	.globl	region_first_diff
	.p2align	4
	.type	region_first_diff,@function
region_first_diff:
# ---- PROLOGUE ---------------------------------------------------------------
	pushq	%rbp                            # save caller's frame pointer
	movq	%rsp, %rbp                      # establish frame

# ---- preload the "equal" answer + guard empty -------------------------------
	movq	$-1, %rax                       # rax = -1: the return value IF we never
                                                #   find a mismatch. Setting it up front
                                                #   means the fall-through path needs no
                                                #   extra instruction.
	testq	%rdx, %rdx                      # len == 0 ?
	je	.LBB1_5                         # empty regions are "equal" -> return -1

# ---- loop setup -------------------------------------------------------------
# %bb.1:
	xorl	%ecx, %ecx                      # rcx = i = 0
	.p2align	4                       # align the hot loop head
.LBB1_2:                                        # loop header
	movzbl	(%rdi,%rcx), %r8d               # r8d = a[i] (load+zero-extend one byte)
	cmpb	(%rsi,%rcx), %r8b               # compare a[i] (r8b) with b[i] in memory.
                                                #   cmpb does an 8-bit subtract for flags
                                                #   only; neither operand is modified.
	jne	.LBB1_3                         # a[i] != b[i]  ->  we found the diff

# ---- bytes matched: advance ------------------------------------------------
# %bb.4:                                        #   still inside the loop
	incq	%rcx                            # i++
	cmpq	%rcx, %rdx                      # i == len ?
	jne	.LBB1_2                         # not yet -> keep scanning

# ---- all bytes equal --------------------------------------------------------
.LBB1_5:                                        # (also the len==0 target)
	popq	%rbp                            # restore frame pointer
	retq                                    # return rax, still -1  => "identical"

# ---- mismatch path ----------------------------------------------------------
.LBB1_3:
	movq	%rcx, %rax                      # rax = i: hand back the FIRST diff index
	popq	%rbp                            # restore frame pointer
	retq                                    # return that offset (>= 0)
.Lfunc_end1:
	.size	region_first_diff, .Lfunc_end1-region_first_diff

# =============================================================================
# u64 table_fingerprint(const u64 *entries /*rdi*/, usize count /*rsi*/) -> rax
# -----------------------------------------------------------------------------
# C body was literally `return fnv1a64((const u8*)entries, count*sizeof(u64));`.
# The optimizer INLINED fnv1a64 (no `call` survives) and turned the multiply by
# sizeof(u64)==8 into a shift. The pointer cast to (u8*) is invisible: same
# address in rdi, just read a byte at a time.
# =============================================================================
	.globl	table_fingerprint
	.p2align	4
	.type	table_fingerprint,@function
table_fingerprint:
# ---- PROLOGUE ---------------------------------------------------------------
	pushq	%rbp                            # save caller's frame pointer
	movq	%rsp, %rbp                      # establish frame

# ---- count (words) -> byte length, seed the inlined hash --------------------
	movabsq	$-3750763034362895579, %rcx     # rcx = FNV basis 0xCBF29CE484222325
                                                #   (fnv1a64 inlined; its `hash` lives
                                                #   in rcx here instead of rdx).
	shlq	$3, %rsi                        # rsi = count << 3 = count * 8 = byte
                                                #   length. This is `count*sizeof(u64)`
                                                #   strength-reduced from a multiply to
                                                #   a single shift — sizeof(u64)==8.
	testq	%rsi, %rsi                      # zero-length table? (count == 0)
	je	.LBB2_1                         # yes -> return the bare basis

# ---- loop setup (identical shape to fnv1a64) --------------------------------
# %bb.2:
	xorl	%edx, %edx                      # rdx = byte index i = 0
	movabsq	$1099511628211, %r8             # r8 = FNV prime, hoisted out of the loop
	.p2align	4
.LBB2_3:                                        # loop header
	movzbl	(%rdi,%rdx), %eax               # eax = ((u8*)entries)[i]  (one byte)
	xorq	%rcx, %rax                      # rax = hash ^ byte
	imulq	%r8, %rax                       # rax = (hash ^ byte) * PRIME
	incq	%rdx                            # i++
	movq	%rax, %rcx                      # hash = rax (carry forward in rcx)
	cmpq	%rdx, %rsi                      # i == byte_len ?
	jne	.LBB2_3                         # loop until the whole table is hashed

# ---- EPILOGUE (rax holds the fingerprint) -----------------------------------
# %bb.4:
	popq	%rbp                            # restore frame pointer
	retq                                    # return rax = table fingerprint

# ---- empty-table path -------------------------------------------------------
.LBB2_1:
	movq	%rcx, %rax                      # rax = basis (count==0 => empty hash)
	popq	%rbp
	retq
.Lfunc_end2:
	.size	table_fingerprint, .Lfunc_end2-table_fingerprint

	.ident	"clang version 20.1.8"          # toolchain stamp (metadata)
	.section	".note.GNU-stack","",@progbits  # mark stack NON-executable (W^X)
	.addrsig                                # address-significance table (LTO hint)
# =============================================================================
# WHAT TO TAKE AWAY
#   * All three functions are leaves: no `call`, no stack locals — the entire
#     computation happens in caller-saved scratch registers. The push/pop %rbp is
#     the only memory traffic besides the input reads.
#   * The FNV loop is a textbook multiply-accumulate: load byte, xor, imul, carry.
#     The prime is hoisted OUT of the loop; the hash never spills.
#   * `count * sizeof(u64)` became `shlq $3` — always expect the optimizer to turn
#     a multiply-by-power-of-two into a shift, and an inlined tiny function into
#     no `call` at all (table_fingerprint has zero calls despite the C source).
#   * region_first_diff preloads -1 so its common "identical" answer is free, and
#     the early `jne` out of the loop makes its cost O(first difference), not
#     O(len) — the property the detector relies on to localize a single hook fast.
#   * Compare with demo.O0.s (every value spilled to the stack, one C line at a
#     time) and demo.O2.s (the loop gets unrolled/vectorized) to SEE the optimizer.
# =============================================================================
