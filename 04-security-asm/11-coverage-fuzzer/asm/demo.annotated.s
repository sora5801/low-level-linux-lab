# =============================================================================
# demo.annotated.s — the AFL coverage heart, clang -O1 output explained.
# =============================================================================
#
# HOW TO READ THIS FILE
# ---------------------
# This is the exact assembly clang emits for asm/demo.c at -O1 (see demo.s for
# the untouched original), with a comment on essentially every instruction.
# AT&T syntax throughout:
#
#     op   source, destination          # movl $1, %eax  =>  eax = 1
#     %reg                               # a register
#     $imm                               # an immediate (literal) constant
#     N(%base,%index)                    # memory at [base + index]   (scale 1)
#     symbol(%rip)                       # RIP-relative address of `symbol`
#
# Register widths are the SAME register: rax(64) / eax(32) / ax(16) / al(8).
# Writing a 32-bit name ZERO-EXTENDS into the 64-bit register, which is why the
# compiler prefers `movl $1,%eax` over `movq`.
#
# System V AMD64 ABI (what every function below obeys):
#   integer/pointer args:  rdi, rsi, rdx, rcx, r8, r9   (then the stack)
#   return value:          rax (eax for int)
#   callee-saved:          rbx, rbp, r12-r15  (a function MUST preserve these)
#   caller-saved:          rax, rcx, rdx, rsi, rdi, r8-r11
#
# THE BIG PICTURE
# ---------------
# demo.c has four functions. Three are the reusable coverage primitives —
# cov_update (the per-edge hot path), classify_count (hit-count bucketing),
# has_new_bits (the feedback decision). The fourth, demo_selftest, drives them.
# The single most instructive thing in this file: because demo_selftest calls
# cov_update with CONSTANT block ids, clang evaluated the entire AFL edge hash at
# COMPILE TIME and turned four function calls into four direct `map[k]++` stores
# at fixed offsets 10, 17, 30, 20 — the exact indices the hash produces. You can
# watch the algorithm run in the compiler. That is the payoff of reading asm.
# =============================================================================

	.file	"demo.c"
	.text

# =============================================================================
# cov_update(u8 *map, u32 prev_loc, u32 cur_loc) -> u32   [THE AFL HEART]
# -----------------------------------------------------------------------------
# ABI on entry:  rdi = map pointer
#                esi = prev_loc          (32-bit; only low bits matter)
#                edx = cur_loc           (this block's id)
# ABI on exit:   eax = cur_loc >> 1      (the caller threads this back in as the
#                                          next prev_loc)
#
# Computes  idx = (cur_loc ^ prev_loc) & 0xFFFF, saturating-increments map[idx],
# and returns cur_loc>>1. This runs on EVERY instrumented edge of a fuzz target,
# i.e. millions of times per second — every instruction here is on the hot path.
# =============================================================================
	.globl	cov_update
	.p2align	4                       # 16-byte align the entry for the I-cache
	.type	cov_update,@function
cov_update:
# ---- PROLOGUE (frame pointer kept at -O1 for legible backtraces) ------------
	pushq	%rbp                    # save caller's frame pointer
	movq	%rsp, %rbp              # rbp = frame base

# ---- idx = (cur_loc ^ prev_loc) & 0xFFFF ------------------------------------
	movl	%edx, %eax              # eax = cur_loc. Stash it now: the return
                                        #   value is cur_loc>>1, and edx is about
                                        #   to be reused as a scratch temporary.
	xorl	%edx, %esi              # esi = prev_loc ^ cur_loc  (esi ^= edx).
                                        #   This is the edge hash: XOR folds the
                                        #   (prev,cur) pair into one number.
	movzwl	%si, %ecx               # ecx = (esi & 0xFFFF), zero-extended.
                                        #   THE MASK IS FREE: "& (MAP_SIZE-1)" with
                                        #   MAP_SIZE=2^16 is just reading the low
                                        #   16-bit sub-register %si — no AND, no
                                        #   div. That is why the map is a power of
                                        #   two. ecx = idx, the bucket index.

# ---- map[idx], saturating at 255 --------------------------------------------
	movzbl	(%rdi,%rcx), %edx       # edx = map[idx]; load the byte at
                                        #   [map + idx], zero-extended to 32 bits.
	cmpb	$-1, %dl                # compare the byte with 0xFF. $-1 IS 0xFF as
                                        #   a signed byte — clang's shorter encoding
                                        #   for "255". This is the saturation test.
	je	.Lcov_done              # if already 255, skip the increment so the
                                        #   counter never wraps 255->0 (AFL's wart;
                                        #   we deliberately saturate instead).
# %bb.1:  (count < 255)
	incb	%dl                     # dl = map[idx] + 1
	movb	%dl, (%rdi,%rcx)        # store the bumped count back to map[idx]
.Lcov_done:                             # (was .LBB0_2)

# ---- return cur_loc >> 1 ----------------------------------------------------
	shrl	%eax                    # eax = cur_loc >> 1. The >>1 is AFL's trick:
                                        #   it makes edge A->B differ from B->A
                                        #   (XOR alone is symmetric) and keeps
                                        #   self-loops A->A out of bucket 0.
	popq	%rbp                    # restore caller's frame pointer
	retq                            # return; eax = next prev_loc
.Lfunc_end0:
	.size	cov_update, .Lfunc_end0-cov_update

# =============================================================================
# classify_count(u8 count) -> u8
# -----------------------------------------------------------------------------
# ABI: edi = count (0..255) ; returns al = one-hot hit-count CLASS bit.
# Folds a raw hit count into a power-of-two bucket bit:
#   0->0  1->1  2->2  3->4  4..7->8  8..15->16  16..31->32  32..127->64  128+->128
# Notice the compiler collapsed the chain of `if`s into a few compares plus, for
# the top two buckets, a BRANCHLESS conditional move.
# =============================================================================
	.globl	classify_count
	.p2align	4
	.type	classify_count,@function
classify_count:
	pushq	%rbp                    # PROLOGUE
	movq	%rsp, %rbp

	cmpl	$3, %edi                # compare count with 3...
	jae	.Lcls_ge3               # if count >= 3, handle the wider buckets
# %bb.1:  count is 0, 1, or 2 -> the class bit EQUALS the count (0/1/2 map to
#         0/1/2), so no table needed:
	movl	%edi, %eax              # al = count  (returns 0, 1, or 2 directly)
.Lcls_ret:                              # (was .LBB1_8) common return for small al
	popq	%rbp
	retq

.Lcls_ge3:                              # (was .LBB1_2) count >= 3
	jne	.Lcls_gt3               # ZF from the cmp above: if count != 3, go on
# %bb.3:  count == 3
	movb	$4, %al                 # class(3) = 4
	popq	%rbp
	retq

.Lcls_gt3:                              # (was .LBB1_4) count >= 4
	movb	$8, %al                 # tentatively class = 8 (the 4..7 bucket)
	cmpb	$8, %dil                # if count < 8, that guess is right
	jb	.Lcls_ret               #   -> return 8
# %bb.5:  count >= 8
	movb	$16, %al                # tentatively class = 16 (8..15)
	cmpb	$16, %dil
	jb	.Lcls_ret               # count < 16 -> return 16
# %bb.6:  count >= 16
	movb	$32, %al                # tentatively class = 32 (16..31)
	cmpb	$32, %dil
	jb	.Lcls_ret               # count < 32 -> return 32
# %bb.7:  count >= 32 -> either the 32..127 bucket (64) or 128+ bucket (128).
#         The split is exactly "is bit 7 set?", i.e. the sign of the byte:
	testb	%dil, %dil              # set flags from count; SF = bit 7 of count
	movl	$64, %ecx               # ecx = 64   (the 32..127 class)
	movl	$128, %eax              # eax = 128  (the 128..255 class)
	cmovnsl	%ecx, %eax              # if SF==0 (count < 128, "not sign"), eax=64;
                                        #   else keep 128. BRANCHLESS select — no
                                        #   mispredict on this final split.
	popq	%rbp
	retq
.Lfunc_end1:
	.size	classify_count, .Lfunc_end1-classify_count

# =============================================================================
# has_new_bits(u8 *virgin, const u8 *trace_raw, usize map_size) -> int
# -----------------------------------------------------------------------------
# ABI: rdi = virgin, rsi = trace_raw, rdx = map_size ; returns eax = 0/1.
# For each edge i: classify trace_raw[i]; if that class bit is still virgin in
# virgin[i], clear it (mark seen) and remember we found NEW coverage. classify's
# logic was INLINED into the loop body below (same compare-ladder as above).
# This is the decision that grows the corpus — the essence of "coverage-guided".
# =============================================================================
	.globl	has_new_bits
	.p2align	4
	.type	has_new_bits,@function
has_new_bits:
	pushq	%rbp                    # PROLOGUE
	movq	%rsp, %rbp
	xorl	%eax, %eax              # eax = is_new = 0 (also the return value)
	testq	%rdx, %rdx              # map_size == 0?
	je	.Lhnb_ret               # empty map -> return 0 immediately
# %bb.1:
	xorl	%ecx, %ecx              # rcx = i = 0 (loop counter)
	jmp	.Lhnb_body              # enter the loop at its header

	.p2align	4
.Lhnb_next:                             # (was .LBB2_14) loop latch / continue
	incq	%rcx                    # i++
	cmpq	%rcx, %rdx              # i == map_size?
	je	.Lhnb_ret               #   -> done, return is_new
.Lhnb_body:                             # (was .LBB2_2) TOP OF LOOP (per edge i)
	movzbl	(%rsi,%rcx), %r9d       # r9d = trace_raw[i] (this edge's raw count)

# ---- inlined classify_count(trace_raw[i]) into r8b -------------------------
	cmpl	$3, %r9d                # count >= 3?
	jae	.Lhnb_ge3
# %bb.3:  count 0/1/2 -> class == count
	movl	%r9d, %r8d              # r8b = count
	jmp	.Lhnb_have_class
	.p2align	4
.Lhnb_ge3:                              # (was .LBB2_4)
	jne	.Lhnb_gt3               # count != 3 -> wider buckets
# %bb.5:  count == 3
	movb	$4, %r8b                # class = 4
	jmp	.Lhnb_have_class
.Lhnb_gt3:                              # (was .LBB2_6)
	movb	$8, %r8b                # guess class = 8 (4..7)
	cmpb	$8, %r9b
	jb	.Lhnb_have_class        # count < 8 -> class 8
# %bb.7:
	movb	$16, %r8b               # class = 16 (8..15)
	cmpb	$16, %r9b
	jb	.Lhnb_have_class
# %bb.8:
	movb	$32, %r8b               # class = 32 (16..31)
	cmpb	$32, %r9b
	jb	.Lhnb_have_class
# %bb.9:
	movl	$64, %r8d               # class = 64 (32..127)
	testb	%r9b, %r9b              # bit 7 set (count >= 128)?
	jns	.Lhnb_have_class        #   no -> keep 64
# %bb.10:
	movl	$128, %r8d              # class = 128 (128..255)
	.p2align	4
.Lhnb_have_class:                       # (was .LBB2_11) r8b = class bit for edge i

# ---- the actual "is this new?" test ----------------------------------------
	testb	%r8b, %r8b              # class == 0? (edge not taken this run)
	je	.Lhnb_next              #   -> skip; nothing to record
# %bb.12:
	movzbl	(%rdi,%rcx), %r9d       # r9d = virgin[i]
	testb	%r8b, %r9b              # (virgin[i] & class) — still-unseen bit set?
	je	.Lhnb_next              #   zero -> already seen; skip
# %bb.13:  first sighting of this class for this edge
	notb	%r8b                    # r8b = ~class
	andb	%r8b, %r9b              # virgin[i] &= ~class  (clear the class bit)
	movb	%r9b, (%rdi,%rcx)       # store the updated virgin byte
	movl	$1, %eax                # is_new = 1
	jmp	.Lhnb_next              # continue scanning (a run can open many)
.Lhnb_ret:                              # (was .LBB2_15)
	popq	%rbp
	retq                            # eax = is_new (0 or 1)
.Lfunc_end2:
	.size	has_new_bits, .Lfunc_end2-has_new_bits

# =============================================================================
# demo_selftest(void) -> int
# -----------------------------------------------------------------------------
# Drives the three primitives on a simulated 4-edge trace 10->20->20->30 and
# returns a checksum. The HEADLINE lesson lives here: cov_update was called with
# constant ids, so clang folded the whole edge hash at compile time. Watch the
# four calls become four fixed-offset increments.
#
# The hash, by hand (idx = (cur ^ prev) & 0xFFFF ; prev' = cur>>1, prev0 = 0):
#   edge (prev=0,  cur=10): idx = 10 ^ 0  = 10 ;  prev -> 10>>1 = 5
#   edge (prev=5,  cur=20): idx = 20 ^ 5  = 17 ;  prev -> 20>>1 = 10
#   edge (prev=10, cur=20): idx = 20 ^ 10 = 30 ;  prev -> 20>>1 = 10   (self-loop!)
#   edge (prev=10, cur=30): idx = 30 ^ 10 = 20 ;  prev -> 30>>1 = 15
# So the four buckets touched are 10, 17, 30, 20 — precisely the offsets below.
# =============================================================================
	.globl	demo_selftest
	.p2align	4
	.type	demo_selftest,@function
demo_selftest:
# ---- PROLOGUE: save the callee-saved regs we use as long-lived pointers -----
	pushq	%rbp
	movq	%rsp, %rbp
	pushq	%r14                    # will hold &virgin (callee-saved: survives calls)
	pushq	%rbx                    # will hold &map

# ---- memset(map, 0, 65536) --------------------------------------------------
	leaq	demo_selftest.map(%rip), %rbx   # rbx = &map (RIP-relative address)
	movl	$65536, %edx            # arg3 len = 65536
	movq	%rbx, %rdi              # arg1 = &map
	xorl	%esi, %esi              # arg2 = 0 (fill byte). xor is the 2-byte zero.
	callq	memset@PLT              # map[] = {0}

# ---- memset(virgin, 0xFF, 65536) -------------------------------------------
	leaq	demo_selftest.virgin(%rip), %r14  # r14 = &virgin
	movl	$65536, %edx            # len
	movq	%r14, %rdi              # arg1 = &virgin
	movl	$255, %esi              # arg2 = 0xFF (all class bits still virgin)
	callq	memset@PLT              # virgin[] = {0xFF}

# ---- the four cov_update() calls, CONSTANT-FOLDED to direct increments ------
# Each block below is one inlined cov_update whose idx the compiler already
# computed. Note it even reordered them (10,17,30,20) — data-independent stores.
	movzbl	demo_selftest.map+10(%rip), %eax   # edge (0,10)  -> map[10]
	cmpb	$-1, %al                # saturate check (==255?)
	je	.Lst_e1
	incb	%al
	movb	%al, demo_selftest.map+10(%rip)     # map[10]++
.Lst_e1:                                # (was .LBB3_2)
	movzbl	demo_selftest.map+17(%rip), %eax   # edge (5,20)  -> map[17]
	cmpb	$-1, %al
	je	.Lst_e2
	incb	%al
	movb	%al, demo_selftest.map+17(%rip)     # map[17]++
.Lst_e2:                                # (was .LBB3_4)
	movzbl	demo_selftest.map+30(%rip), %eax   # edge (10,20) -> map[30]  (self-loop)
	cmpb	$-1, %al
	je	.Lst_e3
	incb	%al
	movb	%al, demo_selftest.map+30(%rip)     # map[30]++
.Lst_e3:                                # (was .LBB3_6)
	movzbl	demo_selftest.map+20(%rip), %eax   # edge (10,30) -> map[20]
	cmpb	$-1, %al
	je	.Lst_e4
	incb	%al
	movb	%al, demo_selftest.map+20(%rip)     # map[20]++
.Lst_e4:                                # (was .LBB3_8)

# ---- first = has_new_bits(virgin, map, 65536) : INLINED loop #1 -------------
# This is the same compare-ladder + virgin-clear as the standalone has_new_bits,
# unrolled inline. It scans all 65536 edges; only offsets 10/17/20/30 are nonzero
# so only those clear a virgin bit. Result (is_new = 1) accumulates in %eax.
	xorl	%eax, %eax              # first  = 0  (kept in eax across the loop)
	xorl	%ecx, %ecx              # i = 0
	jmp	.Lst_l1_body
	.p2align	4
.Lst_l1_next:                           # (was .LBB3_21)
	incq	%rcx                    # i++
	cmpq	$65536, %rcx            # i == MAP_SIZE?
	je	.Lst_l1_done
.Lst_l1_body:                           # (was .LBB3_9) per-edge (classify inlined)
	movzbl	(%rcx,%rbx), %esi       # esi = map[i]
	cmpl	$3, %esi
	jae	.Lst_l1_ge3
	movl	%esi, %edx              # class = count (0/1/2)
	jmp	.Lst_l1_have
	.p2align	4
.Lst_l1_ge3:                            # (was .LBB3_11)
	jne	.Lst_l1_gt3
	movb	$4, %dl                 # class(3)=4
	jmp	.Lst_l1_have
.Lst_l1_gt3:                            # (was .LBB3_13)
	movb	$8, %dl
	cmpb	$8, %sil
	jb	.Lst_l1_have
	movb	$16, %dl
	cmpb	$16, %sil
	jb	.Lst_l1_have
	movb	$32, %dl
	cmpb	$32, %sil
	jb	.Lst_l1_have
	movl	$64, %edx
	testb	%sil, %sil
	jns	.Lst_l1_have
	movl	$128, %edx
	.p2align	4
.Lst_l1_have:                           # (was .LBB3_18) dl = class bit
	testb	%dl, %dl
	je	.Lst_l1_next            # class 0 -> nothing new
	movzbl	(%rcx,%r14), %esi       # esi = virgin[i]
	testb	%dl, %sil               # class bit still virgin?
	je	.Lst_l1_next
	notb	%dl                     # clear the class bit in virgin[i]
	andb	%dl, %sil
	movb	%sil, (%rcx,%r14)       # store virgin[i]
	movl	$1, %eax                # first = 1
	jmp	.Lst_l1_next
.Lst_l1_done:                           # (was .LBB3_22)

# ---- second = has_new_bits(virgin, map, 65536) : INLINED loop #2 ------------
# Identical to loop #1, but now virgin already had those four bits cleared, so
# NO bit is newly cleared and `second` stays 0. Result accumulates in %ecx.
	xorl	%ecx, %ecx              # second = 0 (kept in ecx)
	xorl	%edx, %edx              # i = 0
	jmp	.Lst_l2_body
	.p2align	4
.Lst_l2_next:                           # (was .LBB3_35)
	incq	%rdx
	cmpq	$65536, %rdx
	je	.Lst_l2_done
.Lst_l2_body:                           # (was .LBB3_23)  [same ladder as loop #1]
	movzbl	(%rdx,%rbx), %edi       # edi = map[i]
	cmpl	$3, %edi
	jae	.Lst_l2_ge3
	movl	%edi, %esi
	jmp	.Lst_l2_have
	.p2align	4
.Lst_l2_ge3:                            # (was .LBB3_25)
	jne	.Lst_l2_gt3
	movb	$4, %sil
	jmp	.Lst_l2_have
.Lst_l2_gt3:                            # (was .LBB3_27)
	movb	$8, %sil
	cmpb	$8, %dil
	jb	.Lst_l2_have
	movb	$16, %sil
	cmpb	$16, %dil
	jb	.Lst_l2_have
	movb	$32, %sil
	cmpb	$32, %dil
	jb	.Lst_l2_have
	movl	$64, %esi
	testb	%dil, %dil
	jns	.Lst_l2_have
	movl	$128, %esi
	.p2align	4
.Lst_l2_have:                           # (was .LBB3_32)
	testb	%sil, %sil
	je	.Lst_l2_next
	movzbl	(%rdx,%r14), %edi       # virgin[i]
	testb	%sil, %dil
	je	.Lst_l2_next
	notb	%sil
	andb	%sil, %dil
	movb	%dil, (%rdx,%r14)
	movl	$1, %ecx                # (never taken this pass -> second stays 0)
	jmp	.Lst_l2_next
.Lst_l2_done:                           # (was .LBB3_36)

# ---- return (first<<1) | second | (prev<<2) --------------------------------
# eax = first (1), ecx = second (0), and prev after the walk = 30>>1 = 15, so
# prev<<2 = 60 was CONSTANT-FOLDED. The three fields occupy disjoint bits, so the
# compiler proved the OR equals an ADD and used one LEA + one ADD:
	leal	(%rcx,%rax,2), %eax     # eax = second + first*2 = (first<<1)|second
	addl	$60, %eax               # + (prev<<2) = +60   -> final checksum 62
	popq	%rbx                    # EPILOGUE: restore callee-saved regs...
	popq	%r14
	popq	%rbp
	retq                            # eax = 62 (0b111110): first=1, second=0, prev=15
.Lfunc_end3:
	.size	demo_selftest, .Lfunc_end3-demo_selftest

# =============================================================================
# STATIC DATA — the two 64 KiB maps demo_selftest scans.
# .comm reserves zero-initialised storage in .bss (COMMON); the 16 is alignment.
# =============================================================================
	.type	demo_selftest.map,@object
	.local	demo_selftest.map
	.comm	demo_selftest.map,65536,16      # u8 map[65536], 16-byte aligned
	.type	demo_selftest.virgin,@object
	.local	demo_selftest.virgin
	.comm	demo_selftest.virgin,65536,16   # u8 virgin[65536]

	.ident	"clang version 20.1.8"          # toolchain stamp (harmless)
	.section	".note.GNU-stack","",@progbits  # we do NOT need an exec stack
# =============================================================================
# WHAT TO TAKE AWAY
#   * The AFL edge hash is five instructions: xor, mask-by-subregister, load,
#     saturating inc, shift. "& (MAP_SIZE-1)" is FREE because MAP_SIZE is 2^16 —
#     it is just reading %si. That is the whole reason for the power-of-two map.
#   * `prev = cur >> 1` (one `shr`) is what gives the map DIRECTIONAL edges and
#     visible self-loops; without it a fuzzer would be half-blind.
#   * classify_count's final 64-vs-128 split compiles to a branchless `cmovns` —
#     the sign bit IS "count >= 128".
#   * With constant block ids, clang ran the entire coverage algorithm at compile
#     time (four `map[k]++` at 10/17/30/20) and folded the checksum's disjoint OR
#     into LEA+ADD. Reading asm is how you SEE the optimizer do your algebra.
#   * Compare with demo.O0.s to watch the same code written the naive way: every
#     call a real `call`, every value spilled to the stack.
# =============================================================================
