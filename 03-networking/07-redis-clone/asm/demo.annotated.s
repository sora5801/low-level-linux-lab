# =============================================================================
# demo.annotated.s — clang's -O1 output for demo.c, explained line by line.
# =============================================================================
#
# HOW TO READ THIS FILE
# ---------------------
# This is the exact assembly clang 20 emits for asm/demo.c at -O1 (see demo.s for
# the untouched original), with a comment on essentially every instruction. AT&T
# syntax: `op src, dst`. `%reg` is a register, `$imm` an immediate, `N(%reg)` is
# memory at [reg+N], and `(%base,%index,scale)` is [base + index*scale].
#
# Register widths are the SAME register: rax(64)/eax(32)/ax(16)/al(8). Writing
# eax zero-extends into rax, so clang uses `movl`/`shll`/`andl` (shorter
# encodings) whenever the top 32 bits should end up zero.
#
# THE TWO FUNCTIONS
# -----------------
#   u64   dict_hash(const u8 *key, u64 len, u64 seed);
#   usize rehash_target_index(u64 hash, u64 mask0, u64 mask1, long rehashidx);
#
# dict_hash is MurmurHash2-64A: mix the key 8 bytes at a time, fold the tail, and
# avalanche. rehash_target_index is the one arithmetic step incremental rehashing
# needs on every insert: pick the target table's mask, then `hash & mask`.
#
# SysV AMD64 ABI (the contract this code obeys):
#   integer/pointer args:  rdi, rsi, rdx, rcx, r8, r9   (then the stack)
#   return value:          rax
#   callee-saved:          rbx, rbp, r12-r15  (a function must preserve these)
#   caller-saved (scratch):rax, rcx, rdx, rsi, rdi, r8-r11
#   "red zone":            128 bytes below rsp a leaf may use without adjusting
#                          rsp. BOTH functions here are leaves and use NO stack
#                          locals — everything lives in registers.
#   stack alignment:       rsp % 16 == 0 at a `call` (neither function calls out,
#                          so this never binds here).
#
# THE BIG LESSONS
# ---------------
#   * The 64-bit Murmur multiplier 0xC6A4A7935BD1E995 is too wide to be an
#     instruction immediate, so it is loaded once with `movabsq` and reused from
#     %rax across every `imulq` — the whole hash is a chain of multiplies.
#   * The byte-by-byte little-endian word assembly in C (`p[0] | p[1]<<8 | ...`)
#     was recognized by the optimizer and fused into a SINGLE 64-bit load,
#     `movq (%rdi,%r9,8), %r10`. That is legal precisely because x86-64 is
#     little-endian, so the machine's native load already gives that byte order.
#   * The tail `switch (len & 7)`, compiled with -fno-jump-tables, became a
#     BINARY SEARCH of cmp/jcc that enters the fallthrough chain at the right
#     byte — a jump table would be one indirect jump instead.
#   * rehash_target_index has no branch at all: the `?:` is a `cmovns`.
# =============================================================================

	.file	"demo.c"
	.text
	.globl	dict_hash                       # export dict_hash for the linker
	.p2align	4                       # 16-byte align the entry (I-fetch)
	.type	dict_hash,@function
dict_hash:                              # u64 dict_hash(key=%rdi, len=%rsi, seed=%rdx)
# %bb.0: ---- PROLOGUE + hash seeding ------------------------------------------
	pushq	%rbp                            # save caller's frame pointer
	movq	%rsp, %rbp                      # rbp = frame base (kept for backtraces)
	movabsq	$-4132994306676758123, %rax     # rax = m = 0xC6A4A7935BD1E995, the
	                                        #   Murmur multiplier. 64-bit immediate
	                                        #   -> needs movabsq; lives in rax for
	                                        #   the whole function.
	movq	%rsi, %r8                       # r8 = len
	imulq	%rax, %r8                       # r8 = len * m
	xorq	%rdx, %r8                       # r8 = seed ^ (len*m) = h  (initial state)
	cmpq	$8, %rsi                        # len >= 8 ? (is there a full block?)
	jae	.LBB0_17                        #   yes -> run the block loop
# %bb.1: ---- no full blocks: carry h into rcx and skip straight to the tail ----
	movq	%r8, %rcx                       # rcx = h  (the loop's exit register)
	jmp	.LBB0_2                         # -> .Ltail_dispatch
# =============================================================================
# .LBB0_17  ==  .Lblock_loop_setup : prepare to consume full 8-byte blocks.
# =============================================================================
.LBB0_17:
	movq	%rsi, %rdx                      # rdx = len
	shrq	$3, %rdx                        # rdx = len / 8 = nblocks  (loop bound)
	xorl	%r9d, %r9d                      # r9  = i = 0  (block index)
	.p2align	4
# =============================================================================
# .LBB0_18  ==  .Lblock_loop : one MurmurHash block per iteration.
#   Invariant on entry: r8 = h, r9 = i < nblocks, rax = m.
# =============================================================================
.LBB0_18:                               # =>This Inner Loop Header: Depth=1
	movq	(%rdi,%r9,8), %r10              # r10 = k = *(u64*)(key + i*8). The C code
	                                        #   assembled this from 8 bytes; the
	                                        #   optimizer fused it into ONE little-
	                                        #   endian 64-bit load.
	imulq	%rax, %r10                      # k *= m                (scramble)
	movq	%r10, %rcx                      # rcx = k
	shrq	$47, %rcx                       # rcx = k >> 47
	xorq	%r10, %rcx                      # rcx = k ^ (k >> 47)   (avalanche fold)
	imulq	%rax, %rcx                      # rcx = (k ^ k>>47) * m  (scramble again)
	xorq	%r8, %rcx                       # rcx = h ^ k'          (mix into hash)
	imulq	%rax, %rcx                      # rcx = (h ^ k') * m    (diffuse) = new h
	incq	%r9                             # i++
	movq	%rcx, %r8                       # h = rcx  (write the new hash back)
	cmpq	%r9, %rdx                       # i == nblocks ?
	jne	.LBB0_18                        #   no -> next block
	                                        #   yes -> fall through with rcx = h
# =============================================================================
# .LBB0_2  ==  .Ltail_dispatch : handle the final (len % 8) bytes.
#   rcx = h here (from either the loop or the no-block path). The switch is a
#   binary search that jumps INTO a fallthrough chain, one byte per case.
# =============================================================================
.LBB0_2:
	movq	%rsi, %rdx                      # rdx = len
	andq	$-8, %rdx                       # rdx = len & ~7 = nblocks*8 = tail OFFSET
	                                        #   (the base index of the tail bytes)
	andl	$7, %esi                        # esi = len & 7 = number of tail bytes
	cmpq	$3, %rsi                        # (len&7) > 3 ?
	jg	.LBB0_9                         #   yes -> {4,5,6,7} subtree
# %bb.3:
	cmpq	$1, %rsi                        # (len&7) > 1 ?  (here it is 0..3)
	jg	.LBB0_6                         #   yes -> {2,3} subtree
# %bb.4:
	testq	%rsi, %rsi                      # (len&7) == 0 ?
	jne	.LBB0_8                         #   no (==1) -> case 1
	jmp	.LBB0_5                         #   yes (==0) -> NO tail: skip to avalanche
	                                        #   (note: the "h *= m" of case 1 is also
	                                        #    skipped, matching the C switch.)
# =============================================================================
# .LBB0_9  ==  tail length in {4,5,6,7}.
# =============================================================================
.LBB0_9:
	cmpq	$5, %rsi                        # (len&7) > 5 ?
	jg	.LBB0_13                        #   yes -> {6,7} subtree
# %bb.10:
	cmpl	$4, %esi                        # (len&7) == 4 ?
	jne	.LBB0_16                        #   no (==5) -> enter chain at case 5
	jmp	.LBB0_11                        #   yes      -> enter chain at case 4
# =============================================================================
# .LBB0_6  ==  tail length in {2,3}.
# =============================================================================
.LBB0_6:
	cmpl	$2, %esi                        # (len&7) == 2 ?
	jne	.LBB0_12                        #   no (==3) -> enter chain at case 3
	jmp	.LBB0_7                         #   yes      -> enter chain at case 2
# =============================================================================
# .LBB0_13  ==  tail length in {6,7}.
# =============================================================================
.LBB0_13:
	cmpl	$6, %esi                        # (len&7) == 6 ?
	je	.LBB0_15                        #   yes -> enter chain at case 6
# %bb.14: ---- case 7: h ^= tail[6] << 48 -------------------------------------
	movzbl	6(%rdi,%rdx), %esi             # esi = tail[6] = key[off+6] (zero-extend)
	shlq	$48, %rsi                       # rsi = tail[6] << 48
	xorq	%rsi, %rcx                      # h ^= that                (then fall through)
# ---- case 6: h ^= tail[5] << 40 ---------------------------------------------
.LBB0_15:
	movzbl	5(%rdi,%rdx), %esi             # esi = tail[5]
	shlq	$40, %rsi                       # << 40
	xorq	%rsi, %rcx                      # h ^= tail[5] << 40
# ---- case 5: h ^= tail[4] << 32 ---------------------------------------------
.LBB0_16:
	movzbl	4(%rdi,%rdx), %esi             # esi = tail[4]
	shlq	$32, %rsi                       # << 32
	xorq	%rsi, %rcx                      # h ^= tail[4] << 32
# ---- case 4: h ^= tail[3] << 24 ---------------------------------------------
.LBB0_11:
	movzbl	3(%rdi,%rdx), %esi             # esi = tail[3]
	shll	$24, %esi                       # << 24 (32-bit shift; result zero-extends)
	xorq	%rsi, %rcx                      # h ^= tail[3] << 24
# ---- case 3: h ^= tail[2] << 16 ---------------------------------------------
.LBB0_12:
	movzbl	2(%rdi,%rdx), %esi             # esi = tail[2]
	shll	$16, %esi                       # << 16
	xorq	%rsi, %rcx                      # h ^= tail[2] << 16
# ---- case 2: h ^= tail[1] << 8 ----------------------------------------------
.LBB0_7:
	movzbl	1(%rdi,%rdx), %esi             # esi = tail[1]
	shll	$8, %esi                        # << 8
	xorq	%rsi, %rcx                      # h ^= tail[1] << 8
# ---- case 1: h ^= tail[0]; h *= m -------------------------------------------
.LBB0_8:
	movzbl	(%rdi,%rdx), %edx              # edx = tail[0] = key[off+0]
	xorq	%rcx, %rdx                      # rdx = h ^ tail[0]
	imulq	%rax, %rdx                      # rdx = (h ^ tail[0]) * m   (the case-1 "*m")
	movq	%rdx, %rcx                      # h = rdx
# =============================================================================
# .LBB0_5  ==  .Lavalanche : the final mix. rcx = h.
#   h ^= h >> 47;  h *= m;  h ^= h >> 47;  return h;
# =============================================================================
.LBB0_5:
	movq	%rcx, %rdx                      # rdx = h
	shrq	$47, %rdx                       # rdx = h >> 47
	xorq	%rcx, %rdx                      # rdx = h ^ (h >> 47)
	imulq	%rax, %rdx                      # rdx = that * m
	movq	%rdx, %rax                      # rax = h  (build the result here)
	shrq	$47, %rax                       # rax = h >> 47
	xorq	%rdx, %rax                      # rax = h ^ (h >> 47)      = return value
	popq	%rbp                            # restore caller's frame pointer
	retq                                    # return the 64-bit hash in rax
.Lfunc_end0:
	.size	dict_hash, .Lfunc_end0-dict_hash

# =============================================================================
# rehash_target_index — the incremental-rehash index step, branchless.
#   usize rehash_target_index(hash=%rdi, mask0=%rsi, mask1=%rdx, rehashidx=%rcx)
# =============================================================================
	.globl	rehash_target_index
	.p2align	4
	.type	rehash_target_index,@function
rehash_target_index:                    # @rehash_target_index
# %bb.0:
	pushq	%rbp                            # PROLOGUE: save frame pointer
	movq	%rsp, %rbp                      # rbp = frame base
	movq	%rsi, %rax                      # rax = mask0  (the "not rehashing" mask)
	testq	%rcx, %rcx                      # set flags from rehashidx (check its sign)
	cmovnsq	%rdx, %rax                      # if rehashidx >= 0 (sign flag clear):
	                                        #   rax = mask1. This BRANCHLESS conditional
	                                        #   move IS the `(rehashidx>=0)?mask1:mask0`.
	andq	%rdi, %rax                      # rax = hash & mask  == hash % size
	                                        #   (valid because size is a power of two)
	popq	%rbp                            # EPILOGUE
	retq                                    # return the bucket index in rax
.Lfunc_end1:
	.size	rehash_target_index, .Lfunc_end1-rehash_target_index

	.ident	"clang version 20.1.8"          # toolchain stamp (harmless metadata)
	.section	".note.GNU-stack","",@progbits  # non-executable stack (security)
	.addrsig
# =============================================================================
# WHAT TO TAKE AWAY
#   * A wide constant (the 64-bit Murmur multiplier) is materialized ONCE with
#     movabsq and reused; the hash is literally a chain of `imulq %rax, ...`.
#   * `p[0] | p[1]<<8 | ... | p[7]<<56` compiled to a single `movq` load: the
#     optimizer proved that hand-rolled little-endian assembly equals a native
#     x86-64 64-bit load. Endianness is why that is correct.
#   * `switch (len & 7)` with -fno-jump-tables became a cmp/jcc binary search
#     that jumps into a fallthrough chain — a jump table would be one indirect
#     branch (predictable vs. table-lookup is the trade-off).
#   * The `len&7 == 0` path deliberately skips the case-1 `h *= m`, exactly as
#     the C switch (whose body never runs) would.
#   * rehash_target_index shows the two ideas behind fast hashing tables: a
#     branchless `cmov` to choose the table during a resize, and `and` (not a
#     costly `div`) for the modulo because bucket counts are powers of two.
#   * Compare with demo.O0.s (every value spilled to the stack, one block per C
#     statement) and demo.O2.s (the loop and tail mixed even more aggressively).
# =============================================================================
