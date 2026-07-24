# =============================================================================
# demo.annotated.s — clang -O1 output for asm/demo.c, explained line by line.
# =============================================================================
#
# HOW TO READ THIS FILE
# ---------------------
# This is the exact assembly clang 20 emits for asm/demo.c at -O1 (see demo.s for
# the untouched original), annotated on essentially every instruction. AT&T
# syntax throughout:
#
#     op   source, destination        # movq 8(%rdi), %rax  =>  rax = *(rdi+8)
#     %reg                             # a register
#     $imm                             # an immediate constant
#     N(%reg)                          # memory at address reg + N
#     (%base,%idx,scale)               # memory at base + idx*scale
#
# Register widths are views of one register: rax(64) / eax(32) / ax(16) / al(8).
# A write to eax zero-extends into rax, so clang uses `xorl %eax,%eax` (2 bytes)
# to zero the full 64-bit rax.
#
# THE SysV AMD64 ABI (what every function here obeys)
# ---------------------------------------------------
#   * integer/pointer ARGUMENTS, in order:  rdi, rsi, rdx, rcx, r8, r9   (then stack)
#   * RETURN value:                         rax
#   * CALLEE-SAVED (a function must preserve):  rbx, rbp, r12-r15, rsp
#   * CALLER-SAVED (scratch, freely clobbered): rax, rcx, rdx, rsi, rdi, r8-r11
#   * the RED ZONE: 128 bytes below rsp a leaf function may use without adjusting
#     rsp. All three functions here are leaves and touch NO stack locals, so they
#     never even sub from rsp — the pushq/popq of rbp is the whole frame.
#   * STACK ALIGNMENT: rsp must be 16-byte aligned at a `call`. Irrelevant here:
#     none of these functions call anything.
#
# THE STRUCT LAYOUT the offsets below decode (LP64, natural alignment):
#   deque {  i64 top;      // offset 0    <- CAS target for thieves
#            i64 bottom;   // offset 8    <- owner's index
#            array *arr; } // offset 16
#   array {  i64 cap;      // offset 0
#            i64 mask;     // offset 8
#            item *slot; } // offset 16   <- base of the circular buffer
# So in every function: rdi = deque*, (%rdi)=top, 8(%rdi)=bottom, 16(%rdi)=arr.
#
# THE ONE BIG LESSON — x86-TSO turns C memory orders into ASYMMETRIC cost:
#   * The RELEASE fence in dq_push emits NO instruction, just a `#MEMBARRIER`
#     marker, because x86's Total Store Order already keeps stores in program
#     order (a store can never be reordered after a later store or an earlier
#     load) — release is free.
#   * The SEQ_CST fences in dq_take / dq_steal emit a real `mfence`, because the
#     ONE reordering x86 DOES allow is StoreLoad: a store can be delayed past a
#     later load to a *different* address. Chase-Lev depends on the owner's
#     `bottom` store being ordered before its `top` load (and symmetrically for
#     the thief), so that StoreLoad must be fenced. `mfence` is that fence.
#   * The CAS is `lock cmpxchgq`: the `lock` prefix makes it atomic AND a full
#     barrier. It is the only expensive op on the steal fast path.
# =============================================================================

	.file	"demo.c"
	.text

# =============================================================================
# dq_push(deque *d /rdi/, item x /rsi/)  ->  void
# -----------------------------------------------------------------------------
# Owner-only append at the bottom. No CAS, no mfence: the fast path of the whole
# structure is just loads + one store + a store, which is the point of Chase-Lev.
# =============================================================================
	.globl	dq_push
	.p2align	4
	.type	dq_push,@function
dq_push:
# ---- PROLOGUE (frame pointer kept at -O1 for debuggability; no locals) ------
	pushq	%rbp                    # save caller's frame pointer
	movq	%rsp, %rbp              # rbp = frame base

# ---- b = load(bottom, RELAXED);  a = d->arr;  compute &slot[b & mask] -------
	movq	8(%rdi), %rax           # rax = d->bottom          (RELAXED load: a
                                        #   plain mov; no fence needed to read the
                                        #   owner's own index)
	movq	16(%rdi), %rcx          # rcx = d->arr             (array* pointer)
	movq	16(%rcx), %rdx          # rdx = a->slot            (buffer base)
	movq	8(%rcx), %rcx           # rcx = a->mask            (cap-1)
	andq	%rax, %rcx              # rcx = b & mask           (index % cap, since
                                        #   cap is a power of two — the reason cap
                                        #   must be a power of two)

# ---- slot[b & mask] = x   (RELAXED store, while the slot is still private) --
	movq	%rsi, (%rdx,%rcx,8)     # *(slot + (b&mask)*8) = x. scale 8 = sizeof
                                        #   (item). Bottom has NOT moved yet, so no
                                        #   thief can legally look at this slot.

# ---- atomic_thread_fence(RELEASE) ------------------------------------------
	#MEMBARRIER                     # RELEASE fence — emits NO instruction on x86.
                                        #   TSO already orders this store before the
                                        #   bottom store below, which is exactly the
                                        #   publish guarantee release provides. The
                                        #   marker is only for the compiler's own
                                        #   scheduling; the CPU needs nothing.

# ---- store(bottom, b + 1, RELAXED)  — publishes the item -------------------
	incq	%rax                    # rax = b + 1
	movq	%rax, 8(%rdi)           # d->bottom = b+1. Now bottom-top is one
                                        #   larger; the item is visible to thieves.

# ---- EPILOGUE ---------------------------------------------------------------
	popq	%rbp                    # restore caller's frame pointer
	retq                            # return (void)
.Lfunc_end0:
	.size	dq_push, .Lfunc_end0-dq_push

# =============================================================================
# dq_take(deque *d /rdi/)  ->  item /rax/
# -----------------------------------------------------------------------------
# Owner-only LIFO pop from the bottom. Speculatively decrements bottom, fences,
# then reads top; only the last-element case needs the CAS to arbitrate with a
# racing thief.
#   Register map for the body:  rdx = original bottom (b_old)
#                               rsi = b = b_old - 1  (the claimed index)
#                               rax = d->arr, later the return value
#                               rcx = t = top
# =============================================================================
	.globl	dq_take
	.p2align	4
	.type	dq_take,@function
dq_take:
	pushq	%rbp                    # PROLOGUE
	movq	%rsp, %rbp

# ---- b = load(bottom, RELAXED) - 1;  a = d->arr;  store(bottom, b) ----------
	movq	8(%rdi), %rdx           # rdx = b_old = d->bottom
	leaq	-1(%rdx), %rsi          # rsi = b = b_old - 1   (LEA does the subtract
                                        #   without touching flags)
	movq	16(%rdi), %rax          # rax = d->arr  (loaded now, before the fence,
                                        #   so it is ready if we take the item)
	movq	%rsi, 8(%rdi)           # d->bottom = b   — speculatively claim the
                                        #   bottom slot by lowering bottom.

# ---- atomic_thread_fence(SEQ_CST) — THE linchpin ---------------------------
	mfence                          # Full barrier. Forces the bottom STORE above
                                        #   to complete before the top LOAD below.
                                        #   Without it, x86 StoreLoad reordering
                                        #   could let this thread read a stale top
                                        #   while a thief reads a stale bottom, and
                                        #   BOTH would think they own the last item.

# ---- t = load(top, RELAXED);  if (t <= b) ... else empty -------------------
	movq	(%rdi), %rcx            # rcx = t = d->top
	cmpq	%rdx, %rcx              # compare t (rcx) with b_old (rdx)...
	jge	.LBB1_1                 # ...NOTE: clang tests t >= b_old, i.e. t > b
                                        #   (since b = b_old-1). If so the deque was
                                        #   empty -> go restore bottom & return EMPTY.

# ---- non-empty: x = load(slot[b & mask], RELAXED) --------------------------
# %bb.2:
	movq	16(%rax), %r8           # r8  = a->slot   (buffer base)
	movq	8(%rax), %rax           # rax = a->mask
	andq	%rsi, %rax              # rax = b & mask  (the index)
	movq	(%r8,%rax,8), %rax      # rax = slot[b & mask] = x  (candidate return)

# ---- if (t == b) we are taking the LAST element — must CAS against thieves -
	cmpq	%rsi, %rcx              # compare t (rcx) with b (rsi)
	jne	.LBB1_5                 # t != b  => more than one element remained;
                                        #   no thief can be after THIS slot, so just
                                        #   return x (bottom already lowered). Jump
                                        #   straight to epilogue.

# ---- single-element race: CAS top from t to t+1 (SEQ_CST) ------------------
# %bb.3:  clang marshals cmpxchg's fixed operands: it compares against rax and,
#         on success, stores the source; so t must be in rax and t+1 elsewhere.
	leaq	1(%rcx), %r8            # r8  = t + 1        (desired new top)
	xorl	%r9d, %r9d              # r9  = 0            (the EMPTY sentinel, for
                                        #   the "lost the race" cmov below)
	movq	%rax, %rsi              # rsi = x            (save candidate item; rax
                                        #   is about to be commandeered by cmpxchg)
	movq	%rcx, %rax              # rax = t            (cmpxchg's "expected")
	lock	cmpxchgq	%r8, (%rdi)   # ATOMIC: if d->top == rax(t) then
                                        #   d->top = r8(t+1), ZF=1; else load the
                                        #   current top into rax, ZF=0. The `lock`
                                        #   prefix makes it atomic and a full fence.
                                        #   This is the owner claiming the last item.
	movq	%rsi, %rax              # rax = x  (assume we won: return the item)
	cmovneq	%r9, %rax              # if CAS FAILED (ZF=0, a thief beat us), overwrite
                                        #   rax with 0 = EMPTY. Branchless select.
	jmp	.LBB1_4                 # go reset bottom to the empty state

# ---- empty path: t > b, deque was already empty ----------------------------
.LBB1_1:                                # (label .LBB1_1 = "was empty")
	xorl	%eax, %eax              # rax = 0 = EMPTY (zero-extends to full rax)

# ---- reset bottom to canonical empty (bottom = b_old), then fall to epilogue-
.LBB1_4:                                # reached from empty path AND single-elem path
	movq	%rdx, 8(%rdi)           # d->bottom = b_old (= b+1): undo/settle so
                                        #   bottom == top and the deque reads empty.

# ---- EPILOGUE (also the direct target for the multi-element return) --------
.LBB1_5:                                # (multi-element case jumps straight here with
                                        #   x already in rax and bottom already lowered)
	popq	%rbp
	retq                            # return x (or EMPTY)
.Lfunc_end1:
	.size	dq_take, .Lfunc_end1-dq_take

# =============================================================================
# dq_steal(deque *d /rdi/)  ->  item /rax/
# -----------------------------------------------------------------------------
# Any-thief FIFO steal from the top. Reads top, fences, reads bottom, and if the
# deque looks non-empty, reads the top slot and commits with one CAS.
#   Register map:  rax = t = top (also cmpxchg's "expected" and the return value)
#                  rcx = b = bottom, then reused as slot value
#                  rdx = t + 1 (desired new top)
# =============================================================================
	.globl	dq_steal
	.p2align	4
	.type	dq_steal,@function
dq_steal:
	pushq	%rbp                    # PROLOGUE
	movq	%rsp, %rbp

# ---- t = load(top, ACQUIRE) ------------------------------------------------
	movq	(%rdi), %rax            # rax = t = d->top. ACQUIRE on x86 is a plain
                                        #   mov (loads are never reordered with each
                                        #   other under TSO); the acquire is free.

# ---- atomic_thread_fence(SEQ_CST) — thief side of the take() fence ---------
	mfence                          # Order the top LOAD above before the bottom
                                        #   LOAD below against the owner's interleaved
                                        #   store. Same StoreLoad hazard as take(),
                                        #   fenced from the other side.

# ---- b = load(bottom, ACQUIRE);  if (t < b) ... else EMPTY -----------------
	movq	8(%rdi), %rcx           # rcx = b = d->bottom (ACQUIRE = plain mov)
	cmpq	%rcx, %rax              # compare t (rax) with b (rcx)
	jge	.LBB2_1                 # t >= b  => empty (or owner is mid-take) ->
                                        #   return EMPTY. Otherwise fall through.

# ---- non-empty: x = load(slot[t & mask], RELAXED) --------------------------
# %bb.2:
	movq	16(%rdi), %rcx          # rcx = d->arr
	movq	16(%rcx), %rdx          # rdx = a->slot   (buffer base)
	movq	8(%rcx), %rcx           # rcx = a->mask
	andq	%rax, %rcx              # rcx = t & mask  (the top index)
	movq	(%rdx,%rcx,8), %rcx     # rcx = slot[t & mask] = x  (SPECULATIVE read:
                                        #   valid only if the CAS below wins; the
                                        #   ACQUIRE on bottom paired with push's
                                        #   release fence guarantees it is a fully
                                        #   published item, not a torn write)

# ---- commit: CAS top from t to t+1 (SEQ_CST) -------------------------------
	leaq	1(%rax), %rdx          # rdx = t + 1  (desired new top).  rax already
                                        #   holds t = cmpxchg's "expected" operand.
	lock	cmpxchgq	%rdx, (%rdi)   # ATOMIC: if d->top == rax(t) then
                                        #   d->top = rdx(t+1), ZF=1 (we won the item);
                                        #   else rax = current top, ZF=0 (a thief or
                                        #   the owner beat us). Single `lock cmpxchg`.

# ---- select return: won -> x, lost -> ABORT(-1) ----------------------------
	movq	$-1, %rax              # rax = -1 = ABORT (assume we lost)
	cmoveq	%rcx, %rax             # if CAS SUCCEEDED (ZF=1), rax = x = rcx. This
                                        #   branchless cmov distinguishes "got item"
                                        #   from "lost the race" without a branch.
	popq	%rbp                    # EPILOGUE
	retq                            # return x (won) or ABORT (lost)

# ---- empty path: t >= b ----------------------------------------------------
.LBB2_1:                                # (label = "empty")
	xorl	%eax, %eax              # rax = 0 = EMPTY
	popq	%rbp
	retq
.Lfunc_end2:
	.size	dq_steal, .Lfunc_end2-dq_steal

	.ident	"clang version 20.1.8"      # toolchain stamp (metadata)
	.section	".note.GNU-stack","",@progbits  # non-executable stack (security default)
# =============================================================================
# WHAT TO TAKE AWAY
#   * Owner fast paths (push, and the common multi-element take) use only mov +
#     mov — no atomics. That is the entire performance case for Chase-Lev.
#   * A C `atomic_thread_fence(release)` compiled to NOTHING on x86 (#MEMBARRIER),
#     while `atomic_thread_fence(seq_cst)` compiled to a real `mfence`. Same
#     source, wildly different cost — because x86-TSO only reorders StoreLoad.
#   * The two contended operations — take's last-element path and every steal —
#     each reduce to ONE `lock cmpxchgq`, and clang picks results branchlessly
#     with `cmov`. Reading this asm is how you SEE the memory model, not just
#     read about it. Compare with demo.O0.s (every value spilled to the stack)
#     and demo.O2.s (identical logic, frame pointer omitted).
# =============================================================================
