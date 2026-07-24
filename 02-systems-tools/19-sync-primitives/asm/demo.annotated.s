# =============================================================================
# demo.annotated.s — clang's -O1 output for asm/demo.c, explained instruction
#                    by instruction. (The untouched original is asm/demo.s.)
# =============================================================================
#
# HOW TO READ THIS FILE
# ---------------------
# AT&T syntax, so the operand order is  op  source, destination :
#     movl $1, %eax            #  eax = 1
#     lock cmpxchgl %edx,(%rdi)#  atomic compare-exchange at memory [rdi]
# Register widths are the SAME physical register: rax(64) / eax(32) / ax(16) /
# al(8). Writing eax zero-extends into rax, so clang prefers the 32-bit form
# (`movl $1,%eax`, 5 bytes) whenever the high half should be zero.
#
# THE SysV AMD64 ABI (what every function here obeys)
# ---------------------------------------------------
#   * integer/pointer arguments, in order:  rdi, rsi, rdx, rcx, r8, r9
#       - so the sole `u32 *` argument of every routine below arrives in rdi.
#   * return value:                         rax  (eax for a 32-bit / int result)
#   * callee-saved (a function must restore): rbx, rbp, r12-r15
#   * caller-saved (free to clobber):         rax, rcx, rdx, rsi, rdi, r8-r11
#   * the RED ZONE: 128 bytes below rsp a leaf function may scribble on without
#     adjusting rsp. None of these routines spill, so none touch it.
#   * stack alignment: rsp must be 16-byte aligned at every `call`. These are
#     leaves (no calls), so they only push rbp for a tidy frame — not required.
#
# THE ONE IDEA WORTH TAKING AWAY
# ------------------------------
# A lock's fast path is not magic and is not a syscall: it is a SINGLE
# read-modify-write instruction carrying a `lock` prefix. `lock cmpxchgl`
# *is* the mutex acquire. `xchgl` *is* the spinlock acquire. Everything the
# kernel does (futex parking) happens only when these fail under contention.
# =============================================================================

	.file	"demo.c"
	.text                                   # executable code section

# =============================================================================
# demo_mutex_trylock(u32 *state)  ->  int (1 = acquired, 0 = was already held)
# -----------------------------------------------------------------------------
# The whole uncontended mutex acquire: attempt the atomic transition 0 -> 1.
# C: __atomic_compare_exchange_n(state, &expected(=0), 1, ACQUIRE, RELAXED)
# =============================================================================
	.globl	demo_mutex_trylock
	.p2align	4                       # 16-byte align the entry (fetch-friendly)
	.type	demo_mutex_trylock,@function
demo_mutex_trylock:
	pushq	%rbp                            # PROLOGUE: save caller's frame ptr
	movq	%rsp, %rbp                      #   establish our frame (cosmetic here)

	movl	$1, %edx                        # edx = 1 = the DESIRED new value
	xorl	%ecx, %ecx                      # ecx = 0 (a scratch for the sete result;
	                                        #   `xor r,r` is the 2-byte zero idiom)
	xorl	%eax, %eax                      # eax = 0 = EXPECTED value. cmpxchg uses
	                                        #   the accumulator (rax) as its
	                                        #   implicit "compare against" operand.

	# --- the atomic heart -------------------------------------------------
	lock	cmpxchgl	%edx, (%rdi)    # ATOMICALLY at [rdi]=*state:
	                                        #   if (*state == eax/*0*/) { *state = edx/*1*/; ZF=1 }
	                                        #   else                    { eax = *state;      ZF=0 }
	                                        # The `lock` prefix makes the whole
	                                        #   read-compare-write indivisible across
	                                        #   cores and acts as a full fence — this
	                                        #   is where ACQUIRE ordering is realized.
	sete	%cl                             # cl = ZF ? 1 : 0  — turn the flag into
	                                        #   the boolean "did the swap happen?"
	movl	%ecx, %eax                      # eax = result (the int we return)

	popq	%rbp                            # EPILOGUE: restore frame pointer
	retq                                    #   return; eax holds 1 (won) or 0 (lost)
.Lfunc_end0:
	.size	demo_mutex_trylock, .Lfunc_end0-demo_mutex_trylock

# =============================================================================
# demo_mutex_lock_fast(u32 *state)  ->  u32 (0 = acquired, else observed state)
# -----------------------------------------------------------------------------
# Same CAS, but note the optimization: on FAILURE, cmpxchg has already loaded the
# current value into eax — which is exactly our return register. So the compiler
# emits NO extra load and NO branch: the value that comes back in eax is either 0
# (we succeeded, eax was left 0) or the real state (we failed, cmpxchg wrote it).
# =============================================================================
	.globl	demo_mutex_lock_fast
	.p2align	4
	.type	demo_mutex_lock_fast,@function
demo_mutex_lock_fast:
	pushq	%rbp                            # PROLOGUE
	movq	%rsp, %rbp

	movl	$1, %ecx                        # ecx = 1 = desired new value
	xorl	%eax, %eax                      # eax = 0 = expected (and, cleverly, the
	                                        #   "return 0 on success" value already)
	lock	cmpxchgl	%ecx, (%rdi)    # CAS 0 -> 1 at *state. On success ZF=1 and
	                                        #   eax stays 0. On failure eax := *state.
	                                        #   Either way eax is precisely what C
	                                        #   returns — the `if/return c` collapsed
	                                        #   into "just return eax". No branch!

	popq	%rbp                            # EPILOGUE
	retq                                    #   eax = 0 (locked) or 1/2 (contended)
.Lfunc_end1:
	.size	demo_mutex_lock_fast, .Lfunc_end1-demo_mutex_lock_fast

# =============================================================================
# demo_mutex_unlock_fast(u32 *state)  ->  int (1 = must FUTEX_WAKE, 0 = clean)
# -----------------------------------------------------------------------------
# C: if (__atomic_fetch_sub(state,1,RELEASE) != 1) { *state = 0; return 1; }
#    return 0;
# `fetch_sub` of 1 with a discarded-but-tested result becomes `lock decl`, whose
# ZF tells us whether the NEW value is zero (i.e. the old value was 1).
# =============================================================================
	.globl	demo_mutex_unlock_fast
	.p2align	4
	.type	demo_mutex_unlock_fast,@function
demo_mutex_unlock_fast:
	pushq	%rbp                            # PROLOGUE
	movq	%rsp, %rbp

	xorl	%eax, %eax                      # eax = 0 = the "clean unlock" return value
	lock	decl	(%rdi)                  # ATOMIC *state -= 1 (RELEASE publishes the
	                                        #   critical section). Sets ZF iff the
	                                        #   result is 0, i.e. old value was 1.
	je	.LBB2_2                         # old==1 (no waiter) -> jump to return 0
# --- old value was 2 (contended): fully free the lock and demand a wake -------
	movl	$0, (%rdi)                      # *state = 0  (plain store; the atomic
	                                        #   RMW above already ordered everything)
	movl	$1, %eax                        # eax = 1 = "caller must FUTEX_WAKE(1)"
.LBB2_2:                                    # return:
	popq	%rbp                            # EPILOGUE
	retq                                    #   eax = 0 (clean) or 1 (wake a waiter)
.Lfunc_end2:
	.size	demo_mutex_unlock_fast, .Lfunc_end2-demo_mutex_unlock_fast

# =============================================================================
# demo_spin_lock(u32 *locked)  ->  void
# -----------------------------------------------------------------------------
# Test-and-test-and-set. The FIRST attempt is an unconditional `xchg`; only if
# that loses do we enter the read-only spin (cache-friendly) with PAUSE, and re-
# try the `xchg` each time the word looks free. Note clang hoists the whole frame
# setup PAST the fast path: if we win immediately we never even push rbp.
# =============================================================================
	.globl	demo_spin_lock
	.p2align	4
	.type	demo_spin_lock,@function
demo_spin_lock:
	# --- fast path: one locked exchange, no frame -------------------------
	movl	$1, %eax                        # eax = 1 = value to swap in (LOCKED)
	xchgl	%eax, (%rdi)                    # ATOMIC swap: eax <-> *locked. `xchg`
	                                        #   with a memory operand is IMPLICITLY
	                                        #   locked (no prefix needed) and carries
	                                        #   ACQUIRE ordering. eax now = old value.
	testl	%eax, %eax                      # was the old value 0 (was it free)?
	je	.LBB3_6                         # yes -> jump straight to ret: acquired,
	                                        #   and we never built a stack frame.

# --- contended: build a frame, then spin ------------------------------------
	pushq	%rbp                            # (only reached under contention)
	movq	%rsp, %rbp
	jmp	.LBB3_3                         # enter the TEST loop at its load

	.p2align	4
.LBB3_4:                                    # spin_backoff: (in the inner loop)
	pause                                   # PAUSE: spin-wait hint — throttle the
	                                        #   pipeline, cede the SMT sibling, and
	                                        #   dodge the memory-order mis-speculation
	                                        #   flush when the release store lands.
.LBB3_3:                                    # spin_test:  =>inner loop header
	movl	(%rdi), %eax                    # PLAIN relaxed load of *locked (NOT a
	                                        #   locked op — this is the "test" that
	                                        #   keeps the line in Shared state, so
	                                        #   waiters don't fight over ownership).
	testl	%eax, %eax                      # still held (non-zero)?
	jne	.LBB3_4                         # yes -> back off (PAUSE) and re-read
# --- looks free: re-attempt the expensive locked exchange -------------------
	movl	$1, %eax                        # eax = 1 again
	xchgl	%eax, (%rdi)                    # ATOMIC test-and-set retry
	testl	%eax, %eax                      # did WE win the race for it?
	jne	.LBB3_3                         # no (someone beat us) -> back to test loop
# %bb.5: acquired on retry
	popq	%rbp                            # EPILOGUE (only the contended path has one)
.LBB3_6:                                    # done:
	retq                                    #   lock held; return void
.Lfunc_end3:
	.size	demo_spin_lock, .Lfunc_end3-demo_spin_lock

# =============================================================================
# demo_spin_unlock(u32 *locked)  ->  void
# -----------------------------------------------------------------------------
# RELEASE store of 0. On x86's Total-Store-Order model a release store is just a
# plain `mov`; the "release" is enforced against the COMPILER (it may not sink
# critical-section writes below this), while the hardware already guarantees
# prior stores are visible before this one.
# =============================================================================
	.globl	demo_spin_unlock
	.p2align	4
	.type	demo_spin_unlock,@function
demo_spin_unlock:
	pushq	%rbp                            # PROLOGUE (cosmetic; leaf, no spill)
	movq	%rsp, %rbp
	movl	$0, (%rdi)                      # *locked = 0 — release the lock
	popq	%rbp                            # EPILOGUE
	retq
.Lfunc_end4:
	.size	demo_spin_unlock, .Lfunc_end4-demo_spin_unlock

	.ident	"clang version 20.1.8"          # toolchain stamp (metadata)
	.section	".note.GNU-stack","",@progbits  # non-executable stack: a security
	                                        #   default recorded for the linker
# =============================================================================
# WHAT TO TAKE AWAY
#   * `lock cmpxchgl` is the mutex fast path; `xchgl` is the spinlock fast path.
#     Both are single instructions — the "no syscall on the uncontended path"
#     promise is literally one bus-locked op.
#   * The `lock` prefix (and the implicitly-locked `xchg`) is the full-barrier
#     that realizes ACQUIRE/RELEASE on x86; the C memory_order arguments mostly
#     constrain the COMPILER here, and would emit real fences on ARM/RISC-V.
#   * cmpxchg communicates two things at once: ZF = success, and eax = the value
#     it saw — which the compiler reuses to avoid a reload (see lock_fast).
#   * clang split the spinlock so the winning fast path skips the stack frame
#     entirely: optimizers optimize for the common (uncontended) case.
#   * Compare with demo.O0.s (every value spilled to the stack, each function a
#     literal statement-by-statement mapping) and demo.O2.s (tighter still).
# =============================================================================
