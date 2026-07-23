# =============================================================================
# demo.annotated.s — clang's -O1 output for demo.c, explained instruction by
#                     instruction. (The untouched original is demo.s.)
# =============================================================================
#
# HOW TO READ THIS FILE
# ---------------------
# AT&T syntax throughout:  op  source, destination.   So `subl %esi, %eax`
# computes  eax = eax - esi.  Notation:
#
#     %reg          a register              $imm      an immediate constant
#     (%rdi)        memory at [rdi]         N(%rcx)   memory at [rcx + N]
#     eax/rax       the SAME register: eax is the low 32 bits of rax. Writing a
#                   32-bit register ALWAYS zero-extends into the full 64-bit reg,
#                   which is why 32-bit `u32` math never needs the `q` (64-bit)
#                   forms here.
#
# THE SysV AMD64 ABI (the contract every function below obeys)
# ------------------------------------------------------------
#   * Integer/pointer ARGUMENTS come in, left to right, in:
#         rdi, rsi, rdx, rcx, r8, r9      (further args on the stack)
#     At 32-bit width those are: edi, esi, edx, ecx, r8d, r9d.
#   * The RETURN value goes in rax (eax for a 32-bit int).
#   * CALLEE-SAVED (a function must preserve these for its caller):
#         rbx, rbp, r12, r13, r14, r15, rsp
#     CALLER-SAVED / scratch (free to clobber): rax, rcx, rdx, rsi, rdi, r8-r11.
#   * These are leaf functions: they call nothing, so they touch only scratch
#     registers and never have to save rbx/r12-r15.
#
# ABOUT THE PROLOGUE YOU SEE ON EVERY FUNCTION
# --------------------------------------------
#     pushq %rbp ; movq %rsp, %rbp
# This is the frame-pointer prologue. These leaves need NO stack frame at all —
# the -O2 output (demo.O2.s) drops it entirely. It survives here only because we
# compiled with -fno-omit-frame-pointer, which keeps rbp as a walkable frame
# pointer so a debugger/`perf` can unwind. It costs two instructions and buys a
# clean backtrace; that trade is exactly why kernels are often built with it.
#
# THE ONE LESSON THAT REPEATS
# ---------------------------
# Every "modulo RING_SIZE" in the C is a power-of-two AND in the asm: you will
# see `andl $63, ...` and never a `div`. Every "occupancy" is one `subl`. The
# free-running-counter ring compiles to arithmetic a CPU retires in a cycle.
# =============================================================================

	.file	"demo.c"
	.text                                   # executable code section

# =============================================================================
# u32 rb_count(u32 head /*edi*/, u32 tail /*esi*/)   ->  head - tail  in eax
# -----------------------------------------------------------------------------
# Live occupancy of the ring. Unsigned modular subtraction: correct even when
# `head` has wrapped past 2^32 and `tail` has not, because both are truncated to
# 32 bits and the difference is taken mod 2^32.
# =============================================================================
	.globl	rb_count
	.p2align	4                       # 16-byte-align the entry (fetch-friendly)
	.type	rb_count,@function
rb_count:
	pushq	%rbp                            # PROLOGUE: save caller's frame pointer
	movq	%rsp, %rbp                      #   rbp = current stack top = frame base
	movl	%edi, %eax                      # eax = head            (arg0 -> return reg)
	subl	%esi, %eax                      # eax = head - tail     (the whole function)
	popq	%rbp                            # EPILOGUE: restore caller's frame pointer
	retq                                    # return; result already in eax
.Lfunc_end0:
	.size	rb_count, .Lfunc_end0-rb_count

# =============================================================================
# i32 rb_is_full(u32 head /*edi*/, u32 tail /*esi*/)  ->  (head-tail) >= 64
# -----------------------------------------------------------------------------
# "Full" == occupancy has reached capacity (RING_SIZE == 64). Note there is NO
# branch: the compiler realizes a bool is just the flag from one compare, and
# materializes it with `setae` (set-if-above-or-equal, the UNSIGNED >=).
# =============================================================================
	.globl	rb_is_full
	.p2align	4
	.type	rb_is_full,@function
rb_is_full:
	pushq	%rbp                            # PROLOGUE
	movq	%rsp, %rbp
	subl	%esi, %edi                      # edi = head - tail  (occupancy; edi is scratch)
	xorl	%eax, %eax                      # eax = 0. Zeroing BEFORE the compare preserves
	                                        #   the flags that setae will read; xor is the
	                                        #   idiomatic 2-byte zero and breaks the dep chain.
	cmpl	$64, %edi                       # compare occupancy with capacity (sets CF/ZF)
	setae	%al                             # al = (occupancy >= 64) ? 1 : 0  (unsigned test)
	                                        #   -> eax is 0 or 1; that is our i32 bool.
	popq	%rbp                            # EPILOGUE
	retq
.Lfunc_end1:
	.size	rb_is_full, .Lfunc_end1-rb_is_full

# =============================================================================
# i32 rb_is_empty(u32 head /*edi*/, u32 tail /*esi*/)  ->  head == tail
# -----------------------------------------------------------------------------
# The NAPI poll loop's stop condition. Same branchless bool pattern, but with
# `sete` (set-if-equal, reads ZF) instead of setae.
# =============================================================================
	.globl	rb_is_empty
	.p2align	4
	.type	rb_is_empty,@function
rb_is_empty:
	pushq	%rbp                            # PROLOGUE
	movq	%rsp, %rbp
	xorl	%eax, %eax                      # eax = 0 (pre-zero the result register)
	cmpl	%esi, %edi                      # compare head (edi) with tail (esi) -> sets ZF
	sete	%al                             # al = (head == tail) ? 1 : 0
	popq	%rbp                            # EPILOGUE
	retq
.Lfunc_end2:
	.size	rb_is_empty, .Lfunc_end2-rb_is_empty

# =============================================================================
# u32 rb_slot(u32 index /*edi*/)  ->  index & 63   (== index % 64)
# -----------------------------------------------------------------------------
# THE power-of-two payoff, in one instruction. `index % RING_SIZE` becomes a
# single `andl $63` because 63 == 0b111111 masks off exactly the low 6 bits.
# No division unit, no branch. This is why the ring size MUST be a power of two.
# =============================================================================
	.globl	rb_slot
	.p2align	4
	.type	rb_slot,@function
rb_slot:
	pushq	%rbp                            # PROLOGUE
	movq	%rsp, %rbp
	movl	%edi, %eax                      # eax = index         (arg0 -> return reg)
	andl	$63, %eax                       # eax = index & 63    (the "modulo")
	popq	%rbp                            # EPILOGUE
	retq
.Lfunc_end3:
	.size	rb_slot, .Lfunc_end3-rb_slot

# =============================================================================
# i32 rb_reserve(u32 *head /*rdi*/, u32 tail /*esi*/)   -- THE CENTREPIECE
# -----------------------------------------------------------------------------
# The producer's atomic-looking decision (the driver runs this under a spinlock):
#     h = *head;
#     if ((h - tail) >= RING_SIZE) return -1;   // ring full -> drop the frame
#     *head = h + 1;                            // commit: advance the cursor
#     return h & RING_MASK;                     // physical slot for this frame
#
# Register map on entry:  rdi = &head (a POINTER, so 64-bit),  esi = tail.
# The "-1 = full" sentinel is prepared up front, then overwritten on success —
# a branch-light idiom: the failure value costs nothing on the success path.
#
# A neat optimization to notice: the C says `>= 64`, but the asm tests `> 63`
# with `ja` (jump-if-above, unsigned). For integers `x >= 64` and `x > 63` are
# identical, and `ja` pairs with the immediate 63 the compiler already needed.
# =============================================================================
	.globl	rb_reserve
	.p2align	4
	.type	rb_reserve,@function
rb_reserve:
	pushq	%rbp                            # PROLOGUE
	movq	%rsp, %rbp
	movl	(%rdi), %ecx                    # ecx = *head   (load the producer cursor h.
	                                        #   rdi holds the POINTER; one deref reads h.)
	movl	%ecx, %edx                      # edx = h       (copy; we still need h intact
	                                        #   for the slot AND and the +1 below)
	subl	%esi, %edx                      # edx = h - tail   (occupancy)
	movl	$-1, %eax                       # eax = -1      (pre-load the "full" return value;
	                                        #   this is what we return if we take the branch)
	cmpl	$63, %edx                       # compare occupancy with 63
	ja	.LBB4_2                         # if occupancy > 63 (i.e. >= 64, FULL) skip to
	                                        #   the epilogue leaving eax = -1. Unsigned `ja`.
# ---- not full: reserve and commit the slot (the fall-through, success path) --
	leal	1(%rcx), %eax                   # eax = h + 1.  LEA does the add without touching
	                                        #   flags and lands the result straight in eax.
	movl	%eax, (%rdi)                    # *head = h + 1   (publish the advance to memory)
	andl	$63, %ecx                       # ecx = h & 63    (physical slot = h % RING_SIZE)
	movl	%ecx, %eax                      # eax = slot      (this becomes the return value)
.LBB4_2:                                        # common exit for both paths (label was .LBB4_2)
	popq	%rbp                            # EPILOGUE
	retq                                    # return eax: either the slot (0..63) or -1
.Lfunc_end4:
	.size	rb_reserve, .Lfunc_end4-rb_reserve

# =============================================================================
# i32 skb_has_headroom(u32 head /*edi*/, u32 data /*esi*/, u32 need /*edx*/)
#                                            ->  (data - head) >= need
# -----------------------------------------------------------------------------
# sk_buff geometry:  head <= data <= tail <= end.  Headroom == data - head, the
# free space in FRONT of the payload that skb_push() consumes when a lower layer
# prepends a header. Same branchless `setae` bool as rb_is_full, but this time
# the threshold is a runtime argument (edx = need), not a constant.
# =============================================================================
	.globl	skb_has_headroom
	.p2align	4
	.type	skb_has_headroom,@function
skb_has_headroom:
	pushq	%rbp                            # PROLOGUE
	movq	%rsp, %rbp
	subl	%edi, %esi                      # esi = data - head   (available headroom)
	xorl	%eax, %eax                      # eax = 0 (pre-zero result; keeps flags for setae)
	cmpl	%edx, %esi                      # compare headroom (esi) with need (edx)
	setae	%al                             # al = (headroom >= need) ? 1 : 0  (unsigned)
	popq	%rbp                            # EPILOGUE
	retq
.Lfunc_end5:
	.size	skb_has_headroom, .Lfunc_end5-skb_has_headroom

	.ident	"clang version 20.1.8"          # toolchain stamp (harmless metadata)
	.section	".note.GNU-stack","",@progbits  # we do NOT need an executable stack
	.addrsig                                # address-significance table (LLVM ICF hint)
# =============================================================================
# WHAT TO TAKE AWAY
#   * A power-of-two ring turns every "% size" into one `andl` and every
#     "occupancy" into one `subl`. That is the entire performance argument for
#     requiring a power-of-two capacity (and why kfifo demands it).
#   * Free-running unsigned counters make full/empty unambiguous WITHOUT wasting
#     a slot: empty is head==tail, full is (head-tail)==SIZE. Modular subtraction
#     stays correct across the 2^32 wrap.
#   * Booleans compile to `cmp` + `setCC` with no branch; only rb_reserve needs a
#     real branch (`ja`) because it has two different side effects to choose from.
#   * Compare this file with demo.O2.s to watch the frame-pointer prologue/
#     epilogue vanish once -fno-omit-frame-pointer is gone.
# =============================================================================
