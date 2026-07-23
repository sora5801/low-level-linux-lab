# =============================================================================
# demo.annotated.s — clang -O1 output for demo.c, explained instruction by
# instruction. This is the allocator's pure arithmetic core: alignment rounding,
# the size-class bit trick, block splitting, and boundary-tag coalescing.
# =============================================================================
#
# HOW TO READ THIS FILE
# ---------------------
# This is the EXACT assembly clang 20.1.8 emits for demo.c at -O1 (see demo.s for
# the untouched original), annotated. AT&T syntax throughout:
#
#     op   src, dst                      # movq %rsp,%rbp  =>  rbp = rsp
#     %reg      register   $imm immediate   sym(%rip) RIP-relative address
#     N(%reg)   memory at [reg+N]        (%r1,%r2) memory at [r1+r2]
#
# A register's narrow names are the SAME physical register: rax/eax/ax/al. Writing
# a 32-bit name (eax) ZERO-EXTENDS into the 64-bit register, which is why the
# compiler prefers `movl $48,%eax` (5 bytes) over `movq $48,%rax` (7) when the top
# 32 bits should be zero.
#
# THE SYSTEM V AMD64 ABI (the contract every function below obeys)
# ----------------------------------------------------------------
#   * Integer/pointer arguments, in order:  rdi, rsi, rdx, rcx, r8, r9   (then stack)
#   * Return value:                          rax  (rdx:rax for a 128-bit return)
#   * Caller-saved (scratch; a callee may clobber): rax, rcx, rdx, rsi, rdi, r8-r11
#   * Callee-saved (a callee MUST preserve):        rbx, rbp, r12-r15, rsp
#   * The RED ZONE: 128 bytes below rsp that a leaf function may freely use without
#     moving rsp — the kernel/signal delivery promises not to clobber it. Every
#     function here is a leaf (calls nobody), so none of them touch rsp for locals.
#   * Stack alignment: rsp must be 16-byte aligned at the point of a `call`. Since
#     nothing here makes a call, the only stack traffic is the frame-pointer push.
#
# WHY THE CODE LOOKS LIKE THIS
# ----------------------------
# Every routine is a LEAF made of pure arithmetic, so at -O1 clang:
#   * keeps a frame pointer (push rbp / mov rsp,rbp / pop rbp) purely for
#     debuggability — `-fno-omit-frame-pointer` asked for it; it is otherwise dead.
#   * spills NOTHING to the stack — every value lives in a register.
#   * replaces branches with `cmov` (conditional move) for the min/max clamps.
#   * lowers __builtin_clzll(x) with `bsr` (bit-scan-reverse) because
#     63 - clz(x) == bsr(x) == floor(log2 x).
#   * proves one whole `if` is dead and deletes it (called out at bin_index).
# =============================================================================

	.file	"demo.c"
	.text

# =============================================================================
# align_up(usize n /*rdi*/, usize a /*rsi*/) -> usize /*rax*/
#   return (n + (a-1)) & ~(a-1);
# The whole body is four arithmetic instructions. The elegant part is that the
# mask ~(a-1) is computed as -a, using the two's-complement identity
#   -a == ~a + 1 == ~(a-1).
# =============================================================================
	.globl	align_up
	.p2align	4                       # 16-byte-align the entry (I-fetch friendly)
	.type	align_up,@function
align_up:
	pushq	%rbp                        # PROLOGUE: save caller's frame pointer
	movq	%rsp, %rbp                  #   establish our frame (debug only here)
	leaq	(%rdi,%rsi), %rax           # rax = n + a   (LEA does the add for free,
	                                    #   without touching flags)
	decq	%rax                        # rax = n + a - 1   (= n + (a-1))
	negq	%rsi                        # rsi = -a  == ~(a-1)  -> this IS the mask
	andq	%rsi, %rax                  # rax = (n+a-1) & ~(a-1)  = rounded-up value
	popq	%rbp                        # EPILOGUE: restore caller's frame pointer
	retq                                # return rax
.Lfunc_end0:
	.size	align_up, .Lfunc_end0-align_up

# =============================================================================
# bin_index(usize total /*rdi*/) -> int /*eax*/
#   clamp total to >=48; idx = (63 - clz(total)) - 5; clamp idx to [0,15].
# Two branchless clamps (cmov) bracket one bit-scan. Note the OPTIMIZATION: the
# source also had `if (idx < 0) idx = 0;`, but because total is clamped to >=48
# the compiler PROVES floor_log2 >= 5 so idx >= 0 always — and emits nothing for
# that check. Reading the asm is how you SEE the optimizer erase dead code.
# =============================================================================
	.globl	bin_index
	.p2align	4
	.type	bin_index,@function
bin_index:
	pushq	%rbp                        # PROLOGUE
	movq	%rsp, %rbp
	# ---- total = (total < 48) ? 48 : total   (i.e. max(total,48)) ------------
	cmpq	$49, %rdi                   # compare total to 49 (sets CF if total<49)
	movl	$48, %eax                   # eax = 48  (the clamp floor, MIN_BLOCK)
	cmovaeq	%rdi, %rax                  # if total>=49 (unsigned) rax=total, else 48.
	                                    #   total==48 keeps 48; total<48 becomes 48.
	# ---- floor_log2 = 63 - clz(total) = bsr(total) --------------------------
	bsrq	%rax, %rcx                  # rcx = index of highest set bit of total
	                                    #   = floor(log2 total). This one instruction
	                                    #   is the whole __builtin_clzll trick.
	# ---- idx = floor_log2 - 5, then cap at 15 -------------------------------
	addl	$-5, %ecx                   # ecx = floor_log2 - 5  (>=0, proven above)
	cmpl	$15, %ecx                   # compare idx to 15
	movl	$15, %eax                   # eax = 15  (the clamp ceiling, NUM_BINS-1)
	cmovbl	%ecx, %eax                  # if idx<15 (unsigned) rax=idx, else 15.
	                                    #   (the idx<0 branch was deleted as dead)
	popq	%rbp                        # EPILOGUE
	retq                                # return idx in eax
.Lfunc_end1:
	.size	bin_index, .Lfunc_end1-bin_index

# =============================================================================
# round_total(usize n /*rdi*/) -> usize /*rax*/
#   t = align_up(16 + n + 8, 16) = (n + 39) & ~15;  return t<48 ? 48 : t;
# align_up was INLINED and constant-folded: HDR(16)+FTR(8)=24 plus the (a-1)=15
# from the alignment rounding collapse to the single literal +39.
# =============================================================================
	.globl	round_total
	.p2align	4
	.type	round_total,@function
round_total:
	pushq	%rbp                        # PROLOGUE
	movq	%rsp, %rbp
	leaq	39(%rdi), %rcx              # rcx = n + 39  (= n + 24 + 15, folded)
	andq	$-16, %rcx                  # rcx = (n+39) & ~15  = align_up(n+24,16) = t
	cmpq	$49, %rcx                   # compare t to 49
	movl	$48, %eax                   # eax = 48  (MIN_BLOCK floor)
	cmovaeq	%rcx, %rax                  # if t>=49 rax=t, else 48. t is a multiple of
	                                    #   16, so t==48 stays, t==32 clamps to 48.
	popq	%rbp                        # EPILOGUE
	retq                                # return t
.Lfunc_end2:
	.size	round_total, .Lfunc_end2-round_total

# =============================================================================
# split_block(block *b /*rdi*/, usize total /*rsi*/) -> block* /*rax*/
#   whole = b->size & ~15;  extra = whole - total;
#   if (extra < 48) { b->size = whole|1; return NULL; }
#   b->size = total|1;  rem = (char*)b + total;  rem->size = extra;  return rem;
# One branch: "is the leftover a usable block?". FLAG_ALLOC is bit 0, set with `or`.
# =============================================================================
	.globl	split_block
	.p2align	4
	.type	split_block,@function
split_block:
	pushq	%rbp                        # PROLOGUE
	movq	%rsp, %rbp
	movq	(%rdi), %rax                # rax = b->size  (load the header word)
	andq	$-16, %rax                  # rax = whole = size & ~15 (strip flag bits)
	movq	%rax, %rcx                  # rcx = whole
	subq	%rsi, %rcx                  # rcx = whole - total = extra (leftover bytes)
	cmpq	$47, %rcx                   # extra vs 47
	ja	.LBB_split_do                   # if extra > 47 (i.e. >=48=MIN_BLOCK) -> split
# ---- no-split path: leftover too small, hand out the whole block ------------
	orq	$1, %rax                        # rax = whole | FLAG_ALLOC
	movq	%rax, (%rdi)                # b->size = whole | 1   (mark allocated)
	xorl	%eax, %eax                  # rax = 0  (return NULL: no remainder)
	popq	%rbp                        # EPILOGUE
	retq
.LBB_split_do:                          # ---- split path: carve the tail off ----
	movq	%rsi, %rax                  # rax = total
	orq	$1, %rax                        # rax = total | FLAG_ALLOC
	movq	%rax, (%rdi)                # b->size = total | 1  (b now exactly `total`)
	leaq	(%rdi,%rsi), %rax           # rax = (char*)b + total = &remainder (return)
	movq	%rcx, (%rdi,%rsi)           # rem->size = extra  (free: flags=0). Stored at
	                                    #   [b+total], which is the remainder's header.
	popq	%rbp                        # EPILOGUE
	retq                                # return rem
.Lfunc_end3:
	.size	split_block, .Lfunc_end3-split_block

# =============================================================================
# coalesce_next(block *b /*rdi*/, const void *heap_end /*rsi*/) -> block* /*rax*/
#   nx = (char*)b + blk_size(b);
#   if (nx >= heap_end)      return b;   // at the top of the heap
#   if (nx->size & 1)        return b;   // neighbour is allocated
#   b->size = blk_size(b) + blk_size(nx);  return b;   // absorb neighbour, stay free
# rax is preloaded with b so both early-outs just fall to the shared epilogue.
# =============================================================================
	.globl	coalesce_next
	.p2align	4
	.type	coalesce_next,@function
coalesce_next:
	pushq	%rbp                        # PROLOGUE
	movq	%rsp, %rbp
	movq	%rdi, %rax                  # rax = b  (default return value)
	movq	(%rdi), %rcx                # rcx = b->size
	andq	$-16, %rcx                  # rcx = blk_size(b)  (mask off flags)
	leaq	(%rdi,%rcx), %rdx           # rdx = b + blk_size(b) = nx (next header)
	cmpq	%rsi, %rdx                  # nx vs heap_end
	jae	.LBB_cn_ret                     # if nx >= heap_end -> nothing above, return b
	movq	(%rdx), %rdx                # rdx = nx->size (load neighbour's header)
	testb	$1, %dl                     # test FLAG_ALLOC (bit 0) of nx->size
	jne	.LBB_cn_ret                     # if neighbour allocated -> return b unchanged
# ---- merge: b grows to cover nx --------------------------------------------
	andq	$-16, %rdx                  # rdx = blk_size(nx)
	addq	%rcx, %rdx                  # rdx = blk_size(b) + blk_size(nx) = merged
	movq	%rdx, (%rax)                # b->size = merged (flags=0 => stays free)
.LBB_cn_ret:
	popq	%rbp                        # EPILOGUE
	retq                                # return b (rax)
.Lfunc_end4:
	.size	coalesce_next, .Lfunc_end4-coalesce_next

# =============================================================================
# coalesce_prev(block *b /*rdi*/, const void *heap_lo /*rsi*/) -> block* /*rax*/
#   if (b <= heap_lo)                 return b;   // at the bottom of the heap
#   pf = *(usize*)((char*)b - 8);                 // the PREVIOUS block's footer tag
#   if (pf & 1)                       return b;   // previous block is allocated
#   psize = pf & ~15;  pv = (char*)b - psize;     // step back to its header
#   pv->size = blk_size(pv) + blk_size(b);  return pv;
# This is the payoff of boundary tags: reading (b-8) gives the previous block's
# size in O(1), with no list walk, so backward coalescing is constant time.
# =============================================================================
	.globl	coalesce_prev
	.p2align	4
	.type	coalesce_prev,@function
coalesce_prev:
	pushq	%rbp                        # PROLOGUE
	movq	%rsp, %rbp
	movq	%rdi, %rax                  # rax = b  (default return value)
	cmpq	%rsi, %rdi                  # b vs heap_lo
	jbe	.LBB_cp_ret                     # if b <= heap_lo -> nothing below, return b
	movq	-8(%rax), %rcx              # rcx = *(b-8) = previous block's FOOTER tag
	testb	$1, %cl                     # test FLAG_ALLOC of that footer
	jne	.LBB_cp_ret                     # if previous block allocated -> return b
# ---- merge: previous block grows forward over b ----------------------------
	andq	$-16, %rcx                  # rcx = psize = footer & ~15 (prev block size)
	movq	%rax, %rdx                  # rdx = b
	subq	%rcx, %rdx                  # rdx = b - psize = pv (previous header)
	negq	%rcx                        # rcx = -psize  (used as an index below)
	movq	(%rdx), %rsi                # rsi = pv->size  (reuse rsi; heap_lo is dead)
	andq	$-16, %rsi                  # rsi = blk_size(pv)
	movq	(%rax), %rdi                # rdi = b->size  (reuse rdi)
	andq	$-16, %rdi                  # rdi = blk_size(b)
	addq	%rsi, %rdi                  # rdi = blk_size(pv) + blk_size(b) = merged
	movq	%rdi, (%rax,%rcx)           # store merged at [b + (-psize)] = pv->size.
	                                    #   Same address as pv, reached via (b,-psize)
	                                    #   so the compiler needn't keep pv in a 2nd reg.
	movq	%rdx, %rax                  # rax = pv  (the merged block's new start)
.LBB_cp_ret:
	popq	%rbp                        # EPILOGUE
	retq                                # return pv (or b if we bailed early)
.Lfunc_end5:
	.size	coalesce_prev, .Lfunc_end5-coalesce_prev

	.ident	"clang version 20.1.8"
	.section	".note.GNU-stack","",@progbits   # non-executable stack (security default)
# =============================================================================
# WHAT TO TAKE AWAY
#   * The mask ~(a-1) is just -a: two's complement turns an alignment mask into a
#     single `neg`. (align_up)
#   * A size-class index is one `bsr`: 63 - clz(x) == floor(log2 x). (bin_index)
#   * The optimizer used the >=48 clamp to PROVE `idx < 0` can't happen and deleted
#     that branch — visible only in the asm. (bin_index)
#   * Boundary tags make coalescing pure pointer math: forward via our own size,
#     backward via the neighbour's footer at (b-8), each O(1). (coalesce_*)
#   * Compare with demo.O0.s (every value spilled to the stack, every branch real)
#     and demo.O2.s (the same code, laid out for the branch predictor).
# =============================================================================
