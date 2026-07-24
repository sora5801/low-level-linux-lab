# =============================================================================
# demo.annotated.s — clang -O1 output for demo.c, explained instruction by
# instruction. This is the collector's HOT PATH: the pointer-candidate test,
# the mark-bitmap bit-twiddling, the interior-pointer binary search, and the
# one function that fuses them all (mark_word).
# =============================================================================
#
# HOW TO READ THIS FILE
# ---------------------
# This is the EXACT assembly clang 20.1.8 emits for demo.c at -O1 (see demo.s for
# the untouched original), annotated. AT&T syntax throughout:
#
#     op   src, dst                      # movq %rsp,%rbp  =>  rbp = rsp
#     %reg      register     $imm immediate     sym(%rip) RIP-relative address
#     N(%reg)   memory at [reg+N]
#     (%b,%i,s) memory at [b + i*s]      # s (the "scale") is 1, 2, 4, or 8
#
# A register's narrow names are the SAME physical register: rax/eax/ax/al. Writing
# a 32-bit name (eax) ZERO-EXTENDS into the 64-bit register, which is why the
# compiler prefers `movl $1,%eax` (5 bytes) over `movq $1,%rax` (7) when the top
# 32 bits should be zero.
#
# THE SYSTEM V AMD64 ABI (the contract every function below obeys)
# ----------------------------------------------------------------
#   * Integer/pointer arguments, in order:  rdi, rsi, rdx, rcx, r8, r9  (then the
#     stack: the 7th arg and beyond are pushed by the caller, read at 16(%rbp)).
#   * Return value:                          rax  (rdx:rax for a 128-bit return)
#   * Caller-saved (scratch; a callee may clobber): rax, rcx, rdx, rsi, rdi, r8-r11
#   * Callee-saved (a callee MUST preserve):        rbx, rbp, r12-r15, rsp
#   * The RED ZONE: 128 bytes below rsp that a leaf function may use without moving
#     rsp. Every routine here is a leaf, so none allocate stack locals at all.
#   * Stack alignment: rsp must be 16-byte aligned at a `call`. Nothing here calls
#     out, so the only stack traffic is an optional frame-pointer push.
#
# WHAT THE COMPILER DID THAT IS WORTH SEEING
# ------------------------------------------
#   * mark_test / mark_word lower the bitmap test to `btq` (bit-test) and the set
#     to `btsq` (bit-test-and-set) — one instruction each, instead of shift+mask.
#   * The object descriptor is 24 bytes (base 8 + size 8 + atomic 4 + 4 pad), so
#     indexing objs[i] is `lea (%r,%r,2)` (i*3) followed by an x8 scale = i*24.
#   * mark_word is INLINED end to end: in_heap, obj_containing, granule_of,
#     mark_test and mark_set all vanish into one function.
#   * mark_word is SHRINK-WRAPPED: its fast exits run with NO stack frame at all
#     (just `retq`); the prologue (`push %rbp`) is emitted only on the one slow
#     path that must read the 7th argument off the stack. Watch for it at the end.
# =============================================================================

	.file	"demo.c"
	.text

# =============================================================================
# in_heap(uptr w /*rdi*/, uptr lo /*rsi*/, uptr hi /*rdx*/) -> int /*eax*/
#   return w >= lo && w < hi;
# The cheap range gate. Two unsigned compares, combined branchlessly with `and`
# so there is no misprediction on this extremely hot test.
# =============================================================================
	.globl	in_heap
	.p2align	4                       # 16-byte-align the entry (I-fetch friendly)
	.type	in_heap,@function
in_heap:
	pushq	%rbp                        # PROLOGUE: save caller's frame pointer
	movq	%rsp, %rbp                  #   establish our frame (debug only here)
	cmpq	%rsi, %rdi                  # compare w, lo  (sets CF = w < lo, unsigned)
	setae	%al                         # al = (w >= lo) ? 1 : 0   (CF==0)
	cmpq	%rdx, %rdi                  # compare w, hi
	setb	%cl                         # cl = (w <  hi) ? 1 : 0   (CF==1)
	andb	%al, %cl                    # cl = (w>=lo) & (w<hi)   -> the answer
	movzbl	%cl, %eax                   # zero-extend the byte to the full int in eax
	popq	%rbp                        # EPILOGUE
	retq                                # return the 0/1 result in eax
.Lfunc_end0:
	.size	in_heap, .Lfunc_end0-in_heap

# =============================================================================
# granule_of(char *p /*rdi*/, char *heap_lo /*rsi*/) -> usize /*rax*/
#   return (p - heap_lo) / 16;
# Object bases are 16-aligned, so the divide is an exact right shift by 4. This is
# how an address becomes a bit index into the mark bitmap.
# =============================================================================
	.globl	granule_of
	.p2align	4
	.type	granule_of,@function
granule_of:
	pushq	%rbp                        # PROLOGUE
	movq	%rsp, %rbp
	movq	%rdi, %rax                  # rax = p
	subq	%rsi, %rax                  # rax = p - heap_lo   (byte offset into heap)
	shrq	$4, %rax                    # rax = offset / 16   (÷16 == >>4, no divide)
	popq	%rbp                        # EPILOGUE
	retq                                # return granule index
.Lfunc_end1:
	.size	granule_of, .Lfunc_end1-granule_of

# =============================================================================
# mark_set(u64 *bitmap /*rdi*/, usize g /*rsi*/)
#   bitmap[g >> 6] |= 1UL << (g & 63);
# Set the mark bit for granule g. The word index is g>>6 (64 bits per word); the
# bit within the word is g&63. Note the shift instruction (`shlq %cl`) uses only
# the low 6 bits of the count, which IS g&63 for free — so no explicit mask.
# =============================================================================
	.globl	mark_set
	.p2align	4
	.type	mark_set,@function
mark_set:
	pushq	%rbp                        # PROLOGUE
	movq	%rsp, %rbp
	movq	%rsi, %rcx                  # rcx = g  (shift count must live in cl)
	movl	$1, %eax                    # rax = 1  (the bit we will shift into place)
	shlq	%cl, %rax                   # rax = 1 << (g mod 64)  = the bit mask
	shrq	$6, %rcx                    # rcx = g >> 6           = the word index
	orq	%rax, (%rdi,%rcx,8)             # bitmap[word] |= mask  (x8: 8 bytes/u64)
	popq	%rbp                        # EPILOGUE
	retq
.Lfunc_end2:
	.size	mark_set, .Lfunc_end2-mark_set

# =============================================================================
# mark_test(u64 *bitmap /*rdi*/, usize g /*rsi*/) -> int /*eax*/
#   return (bitmap[g >> 6] >> (g & 63)) & 1;
# The compiler turns "shift down and AND 1" into a single `btq` (bit test): it
# reads bit number (g mod 64) of the word straight into CF.
# =============================================================================
	.globl	mark_test
	.p2align	4
	.type	mark_test,@function
mark_test:
	pushq	%rbp                        # PROLOGUE
	movq	%rsp, %rbp
	movq	%rsi, %rax                  # rax = g
	shrq	$6, %rax                    # rax = g >> 6  = word index
	movq	(%rdi,%rax,8), %rcx          # rcx = bitmap[word]
	xorl	%eax, %eax                  # eax = 0  (clear the result register)
	btq	%rsi, %rcx                     # CF = bit (g mod 64) of rcx  (BT masks count)
	setb	%al                         # al = CF  -> the 0/1 mark state
	popq	%rbp                        # EPILOGUE
	retq
.Lfunc_end3:
	.size	mark_test, .Lfunc_end3-mark_test

# =============================================================================
# obj_containing(uptr w /*rdi*/, const gc_obj *objs /*rsi*/, usize n /*rdx*/)
#                                                            -> long /*rax*/
#   binary-search for the greatest base <= w in [0,n); confirm w < base+size;
#   return that index, else -1. INTERIOR-pointer aware.
#
# Register roles in the loop:  rcx = lo,  rdx = hi (starts at n),  rax = (hi-lo)/2,
#   r8 = mid,  r9 = mid*3 (for the x8 scale => mid*24-byte stride).
# The descriptor is 24 bytes, so objs[mid].base is at (%rsi + mid*3 * 8).
# =============================================================================
	.globl	obj_containing
	.p2align	4
	.type	obj_containing,@function
obj_containing:
	pushq	%rbp                        # PROLOGUE
	movq	%rsp, %rbp
	xorl	%ecx, %ecx                  # lo = 0
	testq	%rdx, %rdx                  # n == 0 ?
	jne	.LBB4_1                        # n>0: enter the search loop
	jmp	.LBB4_5                        # n==0: skip to the post-loop (lo stays 0)
	.p2align	4
# ---- loop tail for the "w >= base" case: lo = mid + 1 -----------------------
.LBB4_3:
	addq	%rax, %rcx                  # rcx = lo + (hi-lo)/2 = mid   (rax holds ÷2)
	incq	%rcx                        # rcx = mid + 1 = new lo
	cmpq	%rdx, %rcx                  # new lo vs hi
	jae	.LBB4_5                        # lo >= hi: search done
# ---- loop header: compute mid, compare w to objs[mid].base ------------------
.LBB4_1:
	movq	%rdx, %rax                  # rax = hi
	subq	%rcx, %rax                  # rax = hi - lo
	shrq	%rax                        # rax = (hi - lo) / 2   (shr by 1)
	leaq	(%rax,%rcx), %r8            # r8  = lo + (hi-lo)/2 = mid
	leaq	(%r8,%r8,2), %r9            # r9  = mid * 3   (so r9*8 = mid*24 = stride)
	cmpq	(%rsi,%r9,8), %rdi          # compare w, objs[mid].base   (base at off 0)
	jae	.LBB4_3                        # w >= base: go raise lo (search upper half)
# ---- else (w < base): hi = mid ---------------------------------------------
	movq	%r8, %rdx                   # hi = mid
	cmpq	%rdx, %rcx                  # lo < hi ?
	jb	.LBB4_1                        # yes: keep searching
	                                    #   (falls through to post-loop otherwise)
# ---- post-loop: lo is the count of bases <= w; check the candidate ----------
.LBB4_5:
	testq	%rcx, %rcx                  # lo == 0 ?  (no base was <= w)
	je	.LBB4_6                        # yes: nothing contains w -> return -1
	decq	%rcx                        # i = lo - 1 = index of greatest base <= w
	leaq	(%rcx,%rcx,2), %rax         # rax = i * 3   (24-byte stride helper)
	movq	8(%rsi,%rax,8), %rdx        # rdx = objs[i].size   (size at offset +8)
	addq	(%rsi,%rax,8), %rdx         # rdx = objs[i].base + size = one-past-end
	cmpq	%rdx, %rdi                  # compare w, end   (CF = w < end)
	movq	$-1, %rax                   # default result = -1 (w is past the object)
	cmovbq	%rcx, %rax                  # if w < end (interior/at base) result = i
	popq	%rbp                        # EPILOGUE
	retq                                # return i, or -1
.LBB4_6:
	movq	$-1, %rax                   # return -1 (w below every object)
	popq	%rbp
	retq
.Lfunc_end4:
	.size	obj_containing, .Lfunc_end4-obj_containing

# =============================================================================
# mark_word(uptr w /*rdi*/, char *heap_lo /*rsi*/, char *heap_hi /*rdx*/,
#           const gc_obj *objs /*rcx*/, usize n /*r8*/, u64 *bitmap /*r9*/,
#           long *out_index /*7th arg -> 16(%rbp) on the slow path*/) -> int /*eax*/
#
# The whole conservative-marking decision, inlined:
#   if (!in_heap(w)) return 0;                 // range gate
#   i = obj_containing(w, objs, n);            // interior-aware locate
#   if (i < 0) return 0;                       // gap / free block
#   g = (objs[i].base - heap_lo) >> 4;         // granule_of
#   if (mark_test(bitmap,g)) return 0;         // already marked
#   mark_set(bitmap,g); *out_index = i; return 1;
#
# THE SHRINK-WRAP: notice mark_word has NO prologue at entry and its `return 0`
# exits are a bare `retq` (frameless — it is a leaf using the red zone). Only the
# final "newly marked" path pushes a frame, because only it needs the 7th argument
# (out_index), which lives on the caller's stack and is reached via rbp.
# =============================================================================
	.globl	mark_word
	.p2align	4
	.type	mark_word,@function
mark_word:
# ---- inlined in_heap(w, heap_lo, heap_hi): return 0 fast if out of range ----
	cmpq	%rsi, %rdi                  # compare w, heap_lo   (CF = w < heap_lo)
	setb	%r10b                       # r10b = (w < heap_lo)
	cmpq	%rdx, %rdi                  # compare w, heap_hi
	setae	%dl                         # dl = (w >= heap_hi)
	xorl	%eax, %eax                  # eax = 0  (the "not marked" return value)
	orb	%r10b, %dl                     # dl = (w<lo) | (w>=hi)  = "outside heap"
	jne	.LBB5_12                       # outside: return 0 (frameless exit)
# ---- inlined obj_containing(w, objs, n): binary search (rcx=objs, r8=n) ------
	xorl	%eax, %eax                  # lo = 0  (eax reused as lo here)
	testq	%r8, %r8                    # n == 0 ?
	jne	.LBB5_2                        # n>0: enter loop
	jmp	.LBB5_6                        # n==0: post-loop
	.p2align	4
.LBB5_4:                                # "w >= base": lo = mid + 1
	addq	%rdx, %rax                  # rax = lo + (hi-lo)/2 = mid
	incq	%rax                        # rax = mid + 1 = new lo
	cmpq	%r8, %rax                   # new lo vs hi (r8 = hi, shrinks toward lo)
	jae	.LBB5_6                        # lo >= hi: done
.LBB5_2:                                # loop header (rax=lo, r8=hi, rdx scratch)
	movq	%r8, %rdx                   # rdx = hi
	subq	%rax, %rdx                  # rdx = hi - lo
	shrq	%rdx                        # rdx = (hi - lo)/2
	leaq	(%rdx,%rax), %r10           # r10 = lo + (hi-lo)/2 = mid
	leaq	(%r10,%r10,2), %r11         # r11 = mid * 3   (x8 => mid*24 stride)
	cmpq	(%rcx,%r11,8), %rdi         # compare w, objs[mid].base
	jae	.LBB5_4                        # w >= base: raise lo
	movq	%r10, %r8                   # else hi = mid
	cmpq	%r8, %rax                   # lo < hi ?
	jb	.LBB5_2                        # keep searching
.LBB5_6:                                # post-loop: rax = lo
	testq	%rax, %rax                  # lo == 0 ?
	je	.LBB5_7                        # yes: i = -1
	decq	%rax                        # i = lo - 1
	leaq	(%rax,%rax,2), %rdx         # rdx = i*3
	movq	8(%rcx,%rdx,8), %r8         # r8 = objs[i].size
	addq	(%rcx,%rdx,8), %r8          # r8 = base + size = end
	cmpq	%r8, %rdi                   # w < end ?
	movq	$-1, %rdx                   # default i = -1
	cmovbq	%rax, %rdx                  # if w<end, i = lo-1 ; else -1
	jmp	.LBB5_9
.LBB5_7:
	movq	$-1, %rdx                   # i = -1 (no containing object)
.LBB5_9:                                # rdx = i (object index or -1)
	xorl	%eax, %eax                  # eax = 0  (return value if we bail)
	testq	%rdx, %rdx                  # i < 0 ?
	js	.LBB5_12                       # yes (sign bit set): return 0, frameless
# ---- granule_of + mark_test, done with combined shifts ----------------------
	leaq	(%rdx,%rdx,2), %rdi         # rdi = i*3   (reload objs[i] address parts)
	movq	(%rcx,%rdi,8), %rcx         # rcx = objs[i].base
	subq	%rsi, %rcx                  # rcx = base - heap_lo   (byte offset)
	movl	%ecx, %esi                  # esi = low 32 bits of the offset
	shrl	$4, %esi                    # esi = offset>>4 = g  (low bits -> bit index)
	shrq	$10, %rcx                   # rcx = offset>>10 = g>>6 = bitmap word index
	                                    #   (>>4 to get g, then >>6 for the word, fused)
	movq	(%r9,%rcx,8), %rdi          # rdi = bitmap[g>>6]  (the word holding our bit)
	btq	%rsi, %rdi                     # CF = current mark bit (btq masks count to 6b)
	jae	.LBB5_11                       # bit clear: it is NEWLY reachable -> go set it
.LBB5_12:                               # ---- FRAMELESS return 0 (already marked, or
	retq                                #      not in heap, or gap). eax already 0.
# ---- slow path: first time we reach this object. Set the bit, record index. -
.LBB5_11:
	pushq	%rbp                        # PROLOGUE (deferred to here!): we now need
	movq	%rsp, %rbp                  #   the stack to reach the 7th argument.
	movq	16(%rbp), %rax              # rax = out_index. Layout after push: [rbp]=old
	                                    #   rbp, [rbp+8]=return addr, [rbp+16]=arg7.
	andl	$63, %esi                   # esi = g & 63  (exact bit index for btsq)
	btsq	%rsi, %rdi                  # set bit (g mod 64) in the loaded word rdi
	movq	%rdi, (%r9,%rcx,8)          # write the word back: bitmap[g>>6] |= bit
	movq	%rdx, (%rax)                # *out_index = i   (hand the object to caller)
	movl	$1, %eax                    # return 1 = "newly marked, push and scan it"
	popq	%rbp                        # EPILOGUE
	retq
.Lfunc_end5:
	.size	mark_word, .Lfunc_end5-mark_word

	.ident	"clang version 20.1.8"
	.section	".note.GNU-stack","",@progbits   # non-executable stack (security default)
	.addrsig
# =============================================================================
# WHAT TO TAKE AWAY
#   * The pointer test is astonishingly cheap: in_heap is two compares + an `and`,
#     and it rejects nearly every stack/register word before the expensive part.
#   * A mark bit is one `btq` to test and one `btsq` to set — the compiler picked
#     the x86 bit-string instructions over shift+mask on its own.
#   * Interior pointers cost nothing extra: the same binary search that finds an
#     exact base also finds the object a mid-pointer lands inside (the `w < end`
#     check after locating the greatest base <= w).
#   * Descriptor indexing is `lea (%r,%r,2)` + scale-8 = *24, the sizeof(gc_obj).
#   * SHRINK WRAPPING: mark_word runs frameless on every fast path and only builds
#     a stack frame on the rare "newly marked" path that needs the 7th argument.
#     You can only SEE that decision in the asm — it is invisible in the C.
#   * Compare with demo.O0.s (every value spilled to the stack, every branch real)
#     and demo.O2.s (the same logic, scheduled and laid out for the predictor).
# =============================================================================
