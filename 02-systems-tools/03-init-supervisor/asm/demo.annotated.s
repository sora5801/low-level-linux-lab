# =============================================================================
# demo.annotated.s — clang's -O1 output for demo.c, explained instruction by
# instruction. Generated form lives in demo.s; this is the hand-written twin.
# =============================================================================
#
# HOW TO READ THIS FILE
# ---------------------
# AT&T syntax (what clang emits by default) reads:
#
#     op   src, dst                     # e.g.  movq %rsi, %rax  =>  rax = rsi
#     %reg          a register          $imm    a literal constant
#     N(%reg)       memory at [reg+N]    sym(%rip)  RIP-relative address of sym
#     (%b,%i,%s)    memory at [b + i*s]  — base + index*scale addressing
#
# A register's names are widths of the SAME hardware register:
#     rax(64) / eax(32) / ax(16) / al(8).  Writing a 32-bit name (eax) ZERO-
#     EXTENDS into the full 64-bit register, which is why the compiler prefers
#     `movl $63,%ecx` (zeroing the top half for free) over a 64-bit move.
#
# THE SYSTEM V AMD64 ABI (the contract every function here obeys)
# --------------------------------------------------------------
#   * Integer/pointer ARGUMENTS, in order:  rdi, rsi, rdx, rcx, r8, r9, then stack
#   * RETURN value:                         rax  (rdx:rax for 128-bit)
#   * CALLER-saved (scratch; a call may clobber): rax, rcx, rdx, rsi, rdi, r8-r11
#   * CALLEE-saved (a function must preserve):    rbx, rbp, r12, r13, r14, r15
#       -> if we want a value to survive a `call`, it must live in one of these,
#          which is exactly why toposort parks `order`/`edges`/`n` in rbx/r14/r15.
#   * THE RED ZONE: 128 bytes below rsp that a LEAF function may scribble in
#       without moving rsp. A function that makes a `call` (like toposort, which
#       calls memset) CANNOT use it — the callee would trample it — so toposort
#       allocates real stack with `subq`. backoff_delay_ms is a leaf and needs
#       no locals, so it touches neither.
#   * STACK ALIGNMENT: at the moment a `call` executes, rsp must be 16-byte
#       aligned. `call` then pushes an 8-byte return address, so on entry to a
#       function rsp ≡ 8 (mod 16). The prologue math below restores 16-alignment
#       before toposort's own `call memset`.
#
# WHAT THE TWO FUNCTIONS ARE
# --------------------------
#   backoff_delay_ms — exponential restart backoff: min(base<<count, cap), with
#       the shift clamped so it can never be UB and the result saturated at cap.
#   toposort         — Kahn's algorithm: order services so each starts after its
#       prerequisites; return count < n signals a dependency cycle.
# =============================================================================

	.file	"demo.c"
	.text

# =============================================================================
# u64 backoff_delay_ms(unsigned restart_count, u64 base_ms, u64 cap_ms)
#     args:   edi = restart_count (32-bit unsigned)
#             rsi = base_ms        -> the compiler keeps the running `delay` here
#             rdx = cap_ms
#     result: rax
#
# The C loop `for (k=0;k<shift;k++){ if(delay>=cap) return cap; delay<<=1; }`
# becomes a countdown in ecx with a compare-and-double body. A LEAF function:
# no `call`, so just a frame-pointer prologue and no stack allocation at all.
# =============================================================================
	.globl	backoff_delay_ms
	.p2align	4                       # 16-byte-align the entry for the fetcher
	.type	backoff_delay_ms,@function
backoff_delay_ms:
# ---- PROLOGUE (frame pointer only; -O1 keeps it for legible backtraces) -----
	pushq	%rbp                    # save caller's frame pointer
	movq	%rsp, %rbp              # rbp = frame base for this call

# ---- set up the return register and clamp the shift count -------------------
	movq	%rdx, %rax              # rax = cap_ms. rax is the return register, and
                                        #   every "return" path yields either cap_ms
                                        #   or min(delay,cap), so seeding rax with
                                        #   cap_ms now saves work on both exits.
	cmpl	$63, %edi               # compare restart_count with 63 (sets flags)
	movl	$63, %ecx               # ecx = 63  (the clamp ceiling; tentative shift)
	cmovbl	%edi, %ecx             # if restart_count < 63 (unsigned "below"),
                                        #   ecx = restart_count; else ecx stays 63.
                                        #   => ecx = min(restart_count,63) = shift.
                                        #   CMOV avoids a branch the CPU might mispredict.
	testl	%edi, %edi              # restart_count == 0 ?  (test sets ZF if edi==0)
	je	.LBB0_3                 #   if 0, the loop body never runs: skip straight
                                        #   to the final clamp. (edi==0 iff shift==0.)

# ---- THE DOUBLING LOOP: runs `shift` times, saturating at the cap -----------
	.p2align	4
.LBB0_1:                                # loop header  (one iteration per bit doubled)
	cmpq	%rax, %rsi              # compare delay(rsi) with cap_ms(rax): rsi - rax
	jae	.LBB0_4                 # if delay >= cap_ms (unsigned "above/equal"),
                                        #   return cap_ms — rax already holds it. This
                                        #   is the C `if (delay >= cap_ms) return cap_ms`.
# %bb.2:  (delay < cap: do one doubling)
	addq	%rsi, %rsi              # delay <<= 1, done as delay = delay+delay
	decl	%ecx                    # shift-- (consume one of the `shift` iterations)
	jne	.LBB0_1                 # if counter != 0, iterate again; else fall through

# ---- FINAL CLAMP: return min(delay, cap_ms) ---------------------------------
.LBB0_3:                                # reached when shift==0 or the loop finished
	cmpq	%rax, %rsi              # compare delay(rsi) with cap_ms(rax)
	cmovbq	%rsi, %rax             # if delay < cap_ms, rax = delay; else keep cap_ms.
                                        #   => rax = min(delay,cap), i.e. the C
                                        #   `if (delay>cap) delay=cap; return delay;`.

# ---- EPILOGUE ---------------------------------------------------------------
.LBB0_4:                                # the ">= cap" fast path also lands here
	popq	%rbp                    # restore caller's frame pointer
	retq                            # return; result (cap or min) is in rax
.Lfunc_end0:
	.size	backoff_delay_ms, .Lfunc_end0-backoff_delay_ms

# =============================================================================
# int toposort(int n, const unsigned char *edges, int *order)
#     args:   edi = n,  rsi = edges,  rdx = order
#     result: eax = number of nodes ordered (== n on success, < n means a cycle)
#
# Two on-stack byte arrays back the algorithm:
#     emitted[] at -112(%rbp)   (1 once a node has been output)
#     indeg[]   at -176(%rbp)   (count of a node's unmet prerequisites)
# The registers that must survive the `call memset` are parked in callee-saved
# registers, per the ABI:  rbx=order, r14=edges, r15=n(64-bit), r12d=n(32-bit).
# =============================================================================
	.globl	toposort
	.p2align	4
	.type	toposort,@function
toposort:
# ---- PROLOGUE: frame + save the five callee-saved regs we will use ----------
	pushq	%rbp                    # save caller frame pointer
	movq	%rsp, %rbp              # establish our frame
	pushq	%r15                    # \
	pushq	%r14                    #  |  preserve callee-saved registers: the ABI
	pushq	%r13                    #  |  promises the caller they come back intact,
	pushq	%r12                    #  |  and we need stable homes for values that
	pushq	%rbx                    # /   must outlive the memset call below.
	subq	$136, %rsp              # reserve locals: 64B indeg + 64B emitted = 128,
                                        #   plus 8 bytes of PADDING so that rsp is
                                        #   16-byte aligned at the `call memset`
                                        #   (entry rsp≡8; 6 pushes≡8; -136≡0 mod16).
                                        #   toposort is NOT a leaf, so it cannot use
                                        #   the 128-byte red zone — hence real stack.

# ---- stash the arguments into callee-saved registers ------------------------
	movq	%rdx, %rbx              # rbx = order  (survives the memset call)
	movq	%rsi, %r14              # r14 = edges  (survives)
	movl	%edi, %r12d             # r12d = n     (32-bit copy, used at phase-2 guard)
	movl	%edi, %r15d             # r15d = n     (writing r15d zero-extends into r15,
                                        #   giving a clean 64-bit n for pointer math)
	testl	%edi, %edi              # n <= 0 ?
	jle	.LBB1_5                 #   if so, skip phase 1 (nothing to count)

# ---- PHASE 1 setup: clear emitted[0..n) with one memset ---------------------
# %bb.1:
	leaq	-112(%rbp), %rdi        # rdi = &emitted[0]           (memset arg1: dst)
	xorl	%r13d, %r13d            # r13 = 0  = i (phase-1 outer index); also reused
                                        #   as the running &emitted write cursor below.
	xorl	%esi, %esi              # esi = 0                     (memset arg2: fill=0)
	movq	%r15, %rdx              # rdx = n                     (memset arg3: length)
	callq	memset@PLT              # emitted[0..n) = 0. Only emitted needs clearing;
                                        #   indeg is fully overwritten in the loop below.
	movq	%r14, %rax              # rax = edges = &row(0). rax walks the matrix one
                                        #   ROW (n bytes) at a time as i advances.

# ---- PHASE 1: indeg[i] = number of prerequisites of i -----------------------
	.p2align	4
.LBB1_2:                                # outer loop over i  (row pointer = rax)
	xorl	%edx, %edx              # edx = 0 = j (inner column index)
	xorl	%ecx, %ecx              # ecx = 0 = deg (prerequisite accumulator)
	.p2align	4
.LBB1_3:                                # inner loop over j: count nonzero row entries
	cmpb	$1, (%rax,%rdx)         # compute row[j] - 1. For an unsigned byte this
                                        #   sets the borrow flag CF iff row[j] < 1,
                                        #   i.e. iff row[j] == 0.
	sbbl	$-1, %ecx               # ecx = ecx - (-1) - CF = ecx + 1 - CF.
                                        #   row[j]==0 -> CF=1 -> +0;  row[j]!=0 -> CF=0
                                        #   -> +1.  This is the BRANCHLESS form of the
                                        #   C `deg += (row[j] != 0)`.
	incq	%rdx                    # j++
	cmpq	%rdx, %r15              # compare n with j
	jne	.LBB1_3                 #   loop while j != n
# %bb.4:  (row finished: commit deg)
	movb	%cl, -176(%rbp,%r13)    # indeg[i] = (byte)deg
	incq	%r13                    # i++  (also advances the emitted/indeg cursor)
	addq	%r15, %rax              # row pointer += n  -> &row(i) = edges + i*n
	cmpq	%r15, %r13              # compare n with i
	jne	.LBB1_2                 #   loop while i != n

# ---- PHASE 2: repeatedly emit a ready node, relax its dependents ------------
.LBB1_5:
	testl	%r12d, %r12d            # n <= 0 ?  (re-checked using the 32-bit copy)
	jle	.LBB1_6                 #   n<=0 -> return 0
# %bb.7:
	xorl	%ecx, %ecx              # ecx = 0 = i (round counter, 0..n-1)
	xorl	%eax, %eax              # eax = 0 = out (nodes emitted so far = result)
	.p2align	4
.LBB1_8:                                # top of a round: find the lowest ready node
	xorl	%edx, %edx              # edx = 0 = j (scan index); edx also ends as `pick`
	jmp	.LBB1_9
	.p2align	4
.LBB1_11:                               # scan: advance to next j
	incq	%rdx                    # j++
	cmpq	%rdx, %r15              # j == n ?
	je	.LBB1_12                #   ran off the end with nothing ready -> pick=-1
.LBB1_9:                                # scan body: is node j ready (unemitted, indeg 0)?
	cmpb	$0, -112(%rbp,%rdx)     # emitted[j] != 0 ?
	jne	.LBB1_11                #   already emitted -> skip
# %bb.10:
	cmpb	$0, -176(%rbp,%rdx)     # indeg[j] != 0 ?
	jne	.LBB1_11                #   still has prerequisites -> skip
# %bb.13:  (j is ready: pick = j)
	testl	%edx, %edx              # j >= 0 ? (always true here; the compiler shares
	jns	.LBB1_14                #   this test with the pick=-1 path). j>=0 -> commit.
	jmp	.LBB1_20                #   (unreachable for j>=0; here for the shared exit)
	.p2align	4
.LBB1_12:                               # nothing ready this round
	movl	$-1, %edx               # pick = -1
	testl	%edx, %edx              # pick < 0 ?
	js	.LBB1_20                #   yes -> LBB1_20 will break the round loop

# ---- commit pick: order[out++] = pick; emitted[pick] = 1 --------------------
.LBB1_14:                               # edx = pick (a valid node index)
	movslq	%eax, %rsi              # rsi = sign-extend out to 64 bits (array index)
	incl	%eax                    # out++   (result register bumps now...)
	movl	%edx, (%rbx,%rsi,4)     # order[old_out] = pick  (int store: scale 4).
                                        #   rbx=order base; rsi=old out -> order[out++]=pick.
	movl	%edx, %esi              # esi = pick
	movb	$1, -112(%rbp,%rsi)     # emitted[pick] = 1  ("remove pick from the graph")
	addq	%r14, %rsi              # rsi = edges + pick. Adding n each iteration below
                                        #   walks column `pick` down the rows, so (%rsi)
                                        #   reads edges[j*n + pick] = "does j depend on pick".
	xorl	%edi, %edi              # edi = 0 = j (relax index)
	jmp	.LBB1_15
	.p2align	4
.LBB1_19:                               # relax: advance to next dependent candidate
	incq	%rdi                    # j++
	addq	%r15, %rsi              # column cursor += n -> edges + pick + j*n
	cmpq	%rdi, %r15              # j == n ?
	je	.LBB1_20                #   done relaxing this pick
.LBB1_15:                               # relax body: decrement indeg[j] if j depends on pick
	cmpb	$0, -112(%rbp,%rdi)     # emitted[j] != 0 ?
	jne	.LBB1_19                #   already emitted -> skip
# %bb.16:
	cmpb	$0, (%rsi)              # edges[j*n + pick] != 0 ?  (j depends on pick?)
	je	.LBB1_19                #   no edge -> skip
# %bb.17:
	movzbl	-176(%rbp,%rdi), %r8d   # r8 = indeg[j]  (zero-extended load)
	testb	%r8b, %r8b              # indeg[j] != 0 ?  (guards against underflow)
	je	.LBB1_19                #   already 0 -> skip the decrement
# %bb.18:
	decb	%r8b                    # indeg[j]--
	movb	%r8b, -176(%rbp,%rdi)   # store the decremented in-degree back
	jmp	.LBB1_19                # continue the relax scan

# ---- end of a round: break on cycle, else loop until we've done n rounds ----
.LBB1_20:
	testl	%edx, %edx              # pick < 0 ?  (edx still holds pick)
	js	.LBB1_22                #   pick==-1 -> nothing was ready -> CYCLE: stop,
                                        #   returning out < n so the caller detects it.
# %bb.21:
	incl	%ecx                    # round++  (i++)
	cmpl	%r15d, %ecx             # i == n ?
	jne	.LBB1_8                 #   more rounds to run
	jmp	.LBB1_22                # did n rounds: fall through to return
.LBB1_6:                                # n <= 0 entry point
	xorl	%eax, %eax              # out = 0

# ---- EPILOGUE: tear down frame, restore callee-saved regs, return `out` -----
.LBB1_22:
	addq	$136, %rsp              # release the local arrays
	popq	%rbx                    # \
	popq	%r12                    #  |  restore callee-saved registers in REVERSE
	popq	%r13                    #  |  push order, so each returns to the value the
	popq	%r14                    #  |  caller left in it.
	popq	%r15                    # /
	popq	%rbp                    # restore caller frame pointer
	retq                            # return; eax = number of nodes ordered
.Lfunc_end1:
	.size	toposort, .Lfunc_end1-toposort

# =============================================================================
# WHAT TO TAKE AWAY
#   * backoff_delay_ms is a LEAF: no `call`, so no stack frame beyond the saved
#     rbp, and it leans on CMOV to clamp/saturate without a single data branch.
#   * toposort is NOT a leaf (it calls memset), so it (a) parks the values that
#     must survive the call in callee-saved rbx/r14/r15/r12 and (b) allocates
#     real stack instead of using the red zone, padding 128 -> 136 to keep rsp
#     16-byte aligned at the call.
#   * The `cmpb $1,mem ; sbb $-1,%ecx` pair is the optimizer turning the boolean
#     `deg += (row[j] != 0)` into branchless carry-flag arithmetic — the kind of
#     trick you only *see* by reading the asm next to the C.
#   * Compare with demo.O0.s (every variable spilled to the stack, every loop a
#     literal compare-and-jump) and demo.O2.s (heavier unrolling/reassociation).
# =============================================================================
	.ident	"clang version 20.1.8"
	.section	".note.GNU-stack","",@progbits
