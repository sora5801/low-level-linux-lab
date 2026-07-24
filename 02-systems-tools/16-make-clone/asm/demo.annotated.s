# =============================================================================
# demo.annotated.s — clang's -O1 output for demo.c, explained instruction by
# instruction. The untouched generated form is demo.s; this is its hand-written
# twin. Every committed .s here is REAL clang output (clang 20.1.8), not fabricated.
# =============================================================================
#
# HOW TO READ THIS FILE
# ---------------------
# AT&T syntax (clang's default) reads:
#
#     op   src, dst                     # e.g.  movq %rsi, %rax  =>  rax = rsi
#     %reg          a register          $imm    a literal constant
#     N(%reg)       memory at [reg+N]    sym(%rip)  RIP-relative address of sym
#     (%b,%i,%s)    memory at [b + i*s]  — base + index*scale addressing
#
# Crucial for the compare below: `cmp src, dst` computes dst - src and sets the
# flags from THAT. So `cmpq %rcx, (%r8,%rsi,8)` tests  mem - rcx, and `jg` is
# taken when mem > rcx (SIGNED). Getting the operand order backwards is the
# single most common AT&T reading error.
#
# A register's names are widths of the SAME hardware register:
#     rax(64) / eax(32) / ax(16) / al(8).  Writing a 32-bit name (eax) ZERO-
#     EXTENDS into the full 64-bit register, which is why the compiler prefers
#     `movl $1,%eax` (zeroing the top half for free) over a 64-bit move.
#
# THE SYSTEM V AMD64 ABI (the contract every function here obeys)
# --------------------------------------------------------------
#   * Integer/pointer ARGUMENTS, in order:  rdi, rsi, rdx, rcx, r8, r9, then the
#       STACK. mk_needs_rebuild has SEVEN integer args, so the 7th (nprereq)
#       does not fit in a register and is passed on the caller's stack — watch
#       it get read from 16(%rbp) below. This is the clearest specimen of stack
#       argument passing in the whole lab.
#   * RETURN value:                         rax  (eax for an int)
#   * CALLER-saved (scratch; a call may clobber): rax, rcx, rdx, rsi, rdi, r8-r11
#   * CALLEE-saved (a function must preserve):    rbx, rbp, r12, r13, r14, r15
#       -> to keep a value across a `call`, park it in one of these. That is
#          exactly why mk_toposort stashes order/edges/n in rbx/r14/r15/r12.
#   * THE RED ZONE: 128 bytes below rsp a LEAF function may use without moving
#       rsp. mk_needs_rebuild is a leaf and needs no locals, so it uses neither
#       the red zone nor real stack (its tiny frame exists ONLY to address the
#       stack argument). mk_toposort makes a `call` (memset) so it CANNOT use the
#       red zone and allocates real stack with `subq`.
#   * STACK ALIGNMENT: at a `call`, rsp must be 16-byte aligned; `call` then
#       pushes an 8-byte return address, so on entry rsp ≡ 8 (mod 16). The
#       prologue math below restores 16-alignment before mk_toposort's memset.
#
# WHAT THE TWO FUNCTIONS ARE
# --------------------------
#   mk_needs_rebuild — make's STALENESS test: rebuild if forced/phony/missing,
#       or if any prerequisite is newer or was itself rebuilt. A leaf; the star.
#   mk_toposort      — Kahn's algorithm: order targets so each is built after its
#       prerequisites; a return count < n means a dependency CYCLE.
# =============================================================================

	.file	"demo.c"
	.text

# =============================================================================
# int mk_needs_rebuild(int always, int is_phony, int target_missing,
#                      mk_time target_mtime, const mk_time *prereq_mtime,
#                      const int *prereq_rebuilt, int nprereq)
#
#   edi = always            esi = is_phony         edx = target_missing
#   rcx = target_mtime       r8 = prereq_mtime[]    r9 = prereq_rebuilt[]
#   [caller stack] = nprereq  (the 7th integer arg; read via 16(%rbp))
#   result: eax  (1 = rebuild, 0 = up to date)
#
# Shape: fold the three cheap boolean reasons into one OR and branch; only if
# ALL are false do we set up a frame, read nprereq off the stack, and scan the
# prerequisite arrays.
# =============================================================================
	.globl	mk_needs_rebuild
	.p2align	4
	.type	mk_needs_rebuild,@function
mk_needs_rebuild:
# ---- the three scalar reasons: return 1 if (always | is_phony | target_missing)
	orl	%esi, %edi              # edi = always | is_phony
	movl	$1, %eax                # eax = 1  — preload the "rebuild" answer so
                                        #   every early-exit path is just `retq`.
	orl	%edx, %edi              # edi = (always|is_phony) | target_missing
	je	.LBB0_1                 # if the combined OR is 0 (all three false),
                                        #   go scan the prerequisites; ...
.LBB0_7:
	retq                            #   ...else fall here and return eax (=1).
                                        #   The prereq loop's two "stale" exits
                                        #   also jump here — one shared `ret 1`.

# ---- all three false: build a frame just to reach the 7th (stack) argument ---
.LBB0_1:
	pushq	%rbp                    # PROLOGUE. rsp was ≡8 (mod16) on entry; this
                                        #   push makes rbp addressable and rsp ≡0.
	movq	%rsp, %rbp              # rbp = frame base.
	movl	16(%rbp), %edx          # edx = nprereq. Layout from rbp: [rbp]=saved
                                        #   rbp, [rbp+8]=return addr, [rbp+16]=the
                                        #   7th arg the caller pushed. THIS is
                                        #   stack-argument passing, made visible.
	testl	%edx, %edx              # nprereq <= 0 ?  (sets SF/ZF from nprereq)
	popq	%rbp                    # EPILOGUE now — the frame's only job (reading
                                        #   the stack arg) is done, so restore rbp
                                        #   before either branch. Both paths leave
                                        #   with a balanced stack.
	jle	.LBB0_6                 # nprereq <= 0: no prereqs to check -> up to date

# ---- set up the prerequisite scan ------------------------------------------
# %bb.2:
	movl	%edx, %edx              # zero-extend nprereq (32b) into rdx (64b) so it
                                        #   can be the loop's 64-bit end bound. `mov
                                        #   eDX,eDX` clears rdx's top half for free.
	xorl	%esi, %esi              # esi = i = 0  (reuse rsi as the index register)
	.p2align	4
.LBB0_3:                                # for (i = 0; i < nprereq; i++)
	cmpl	$0, (%r9,%rsi,4)        # prereq_rebuilt[i] — r9 is the int* base, and
                                        #   the index is scaled by 4 = sizeof(int).
	jne	.LBB0_7                 # prereq_rebuilt[i] != 0 -> a prereq will be
                                        #   rebuilt -> return 1 (jump to shared ret).
# %bb.4:
	cmpq	%rcx, (%r8,%rsi,8)      # compare prereq_mtime[i] with target_mtime.
                                        #   r8 is the mk_time* base; scale is 8 =
                                        #   sizeof(long long). cmp computes
                                        #   mem - rcx, i.e. prereq_mtime[i] - target.
	jg	.LBB0_7                 # SIGNED greater: prereq_mtime[i] > target_mtime
                                        #   -> a newer prerequisite -> return 1. It
                                        #   is `jg`, not `ja`, because mk_time is
                                        #   SIGNED — the MK_TIME_MISSING = -1
                                        #   sentinel depends on signed ordering.
# %bb.5:
	incq	%rsi                    # i++
	cmpq	%rsi, %rdx              # compare nprereq(rdx) with i(rsi)
	jne	.LBB0_3                 # i != nprereq: keep scanning

# ---- nothing was newer/rebuilt: up to date ---------------------------------
.LBB0_6:
	xorl	%eax, %eax              # eax = 0  (do NOT rebuild). The ONLY path that
                                        #   overwrites the preloaded 1.
	retq
.Lfunc_end0:
	.size	mk_needs_rebuild, .Lfunc_end0-mk_needs_rebuild

# =============================================================================
# int mk_toposort(int n, const unsigned char *edges, int *order)
#     args:   edi = n,  rsi = edges,  rdx = order
#     result: eax = number of nodes ordered (== n on success, < n means a cycle)
#
# Two on-stack byte arrays back the algorithm:
#     emitted[] at -112(%rbp)   (1 once a node has been placed in `order`)
#     indeg[]   at -176(%rbp)   (count of a node's not-yet-built prerequisites)
# Values that must survive the `call memset` are parked in callee-saved regs:
#     rbx = order,  r14 = edges,  r15 = n (64-bit),  r12d = n (32-bit copy).
# =============================================================================
	.globl	mk_toposort
	.p2align	4
	.type	mk_toposort,@function
mk_toposort:
# ---- PROLOGUE: frame + save the five callee-saved regs we will use ----------
	pushq	%rbp                    # save caller's frame pointer
	movq	%rsp, %rbp              # establish our frame
	pushq	%r15                    # \
	pushq	%r14                    #  |  preserve callee-saved registers: the ABI
	pushq	%r13                    #  |  promises the caller gets them back intact,
	pushq	%r12                    #  |  and we need stable homes for values that
	pushq	%rbx                    # /   must outlive the memset call below.
	subq	$136, %rsp              # reserve locals: 64B indeg + 64B emitted = 128,
                                        #   + 8 bytes PADDING so rsp is 16-byte
                                        #   aligned at `call memset` (entry rsp≡8;
                                        #   6 pushes≡8; -136≡0 mod 16). Not a leaf,
                                        #   so the red zone is unavailable — real
                                        #   stack it is.

# ---- stash arguments into callee-saved registers ----------------------------
	movq	%rdx, %rbx              # rbx = order  (survives the memset call)
	movq	%rsi, %r14              # r14 = edges  (survives)
	movl	%edi, %r12d             # r12d = n     (32-bit copy for the phase-2 guard)
	movl	%edi, %r15d             # r15d = n     (writing r15d zero-extends into
                                        #   r15, giving a clean 64-bit n for indexing)
	testl	%edi, %edi              # n <= 0 ?
	jle	.LBB1_5                 #   if so, skip phase 1 entirely

# ---- PHASE 1 setup: clear emitted[0..n) with one memset ---------------------
# %bb.1:
	leaq	-112(%rbp), %rdi        # rdi = &emitted[0]           (memset arg1: dst)
	xorl	%r13d, %r13d            # r13 = 0 = i (phase-1 index; also the emitted/
                                        #   indeg write cursor as it advances)
	xorl	%esi, %esi              # esi = 0                     (memset arg2: fill=0)
	movq	%r15, %rdx              # rdx = n                     (memset arg3: length)
	callq	memset@PLT              # emitted[0..n) = 0. Only emitted needs clearing;
                                        #   indeg is fully overwritten in the loop.
	movq	%r14, %rax              # rax = edges = &row(0). rax walks the matrix one
                                        #   ROW (n bytes) at a time as i advances.

# ---- PHASE 1: indeg[i] = number of prerequisites of target i ----------------
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
                                        #   row[j]==0 -> CF=1 -> +0; row[j]!=0 -> CF=0
                                        #   -> +1. Branchless form of the C
                                        #   `deg += (row[j] != 0)`.
	incq	%rdx                    # j++
	cmpq	%rdx, %r15              # compare n with j
	jne	.LBB1_3                 #   loop while j != n
# %bb.4:  (row finished: commit deg)
	movb	%cl, -176(%rbp,%r13)    # indeg[i] = (byte)deg
	incq	%r13                    # i++  (advances the indeg/emitted cursor too)
	addq	%r15, %rax              # row pointer += n  -> &row(i) = edges + i*n
	cmpq	%r15, %r13              # compare n with i
	jne	.LBB1_2                 #   loop while i != n

# ---- PHASE 2: repeatedly emit a ready node, relax its dependents ------------
.LBB1_5:
	testl	%r12d, %r12d            # n <= 0 ?  (re-checked with the 32-bit copy)
	jle	.LBB1_6                 #   n<=0 -> return 0
# %bb.7:
	xorl	%ecx, %ecx              # ecx = 0 = i (round counter, 0..n-1)
	xorl	%eax, %eax              # eax = 0 = out (nodes emitted so far = result)
	.p2align	4
.LBB1_8:                                # top of a round: find the lowest ready node
	xorl	%edx, %edx              # edx = 0 = j (scan index); edx also ends as pick
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
	jmp	.LBB1_20                #   (shared exit for the pick<0 case)
	.p2align	4
.LBB1_12:                               # nothing ready this round
	movl	$-1, %edx               # pick = -1
	testl	%edx, %edx              # pick < 0 ?
	js	.LBB1_20                #   yes -> LBB1_20 breaks the round loop (a CYCLE)

# ---- commit pick: order[out++] = pick; emitted[pick] = 1 --------------------
.LBB1_14:                               # edx = pick (a valid node index)
	movslq	%eax, %rsi              # rsi = sign-extend out to 64 bits (array index)
	incl	%eax                    # out++   (the result register bumps now)
	movl	%edx, (%rbx,%rsi,4)     # order[old_out] = pick  (int store, scale 4).
                                        #   rbx=order base; rsi=old out.
	movl	%edx, %esi              # esi = pick
	movb	$1, -112(%rbp,%rsi)     # emitted[pick] = 1  ("remove pick from the graph")
	addq	%r14, %rsi              # rsi = edges + pick. Adding n each iteration
                                        #   below walks column `pick` down the rows,
                                        #   so (%rsi) reads edges[j*n + pick] =
                                        #   "does j depend on pick".
	xorl	%edi, %edi              # edi = 0 = j (relax index)
	jmp	.LBB1_15
	.p2align	4
.LBB1_19:                               # relax: advance to next dependent candidate
	incq	%rdi                    # j++
	addq	%r15, %rsi              # column cursor += n -> edges + pick + j*n
	cmpq	%rdi, %r15              # j == n ?
	je	.LBB1_20                #   done relaxing this pick
.LBB1_15:                               # relax body: decrement indeg[j] if j needs pick
	cmpb	$0, -112(%rbp,%rdi)     # emitted[j] != 0 ?
	jne	.LBB1_19                #   already emitted -> skip
# %bb.16:
	cmpb	$0, (%rsi)              # edges[j*n + pick] != 0 ?  (does j depend on pick?)
	je	.LBB1_19                #   no edge -> skip
# %bb.17:
	movzbl	-176(%rbp,%rdi), %r8d   # r8 = indeg[j]  (zero-extended byte load)
	testb	%r8b, %r8b              # indeg[j] != 0 ?  (guard against underflow)
	je	.LBB1_19                #   already 0 -> skip the decrement
# %bb.18:
	decb	%r8b                    # indeg[j]--
	movb	%r8b, -176(%rbp,%rdi)   # store the decremented in-degree back
	jmp	.LBB1_19                # continue the relax scan

# ---- end of a round: break on cycle, else loop until n rounds are done ------
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
	.size	mk_toposort, .Lfunc_end1-mk_toposort

# =============================================================================
# WHAT TO TAKE AWAY
#   * mk_needs_rebuild is a LEAF, yet it still sets up a one-instruction frame —
#     purely to reach its SEVENTH argument at 16(%rbp). Six integer args go in
#     registers; the rest ride the caller's stack. That is the ABI made visible.
#   * The staleness compare is SIGNED (`jg`, `cmpq`) and uses two different index
#     SCALES — 4 for the int prereq_rebuilt[], 8 for the long long prereq_mtime[]
#     — a direct readout of the C element types.
#   * mk_toposort is NOT a leaf (it calls memset): it parks live values in
#     callee-saved rbx/r14/r15/r12 and allocates real stack (padding 128 -> 136)
#     to keep rsp 16-byte aligned at the call, since the red zone is off-limits.
#   * The `cmpb $1,mem ; sbb $-1,%ecx` pair is the optimizer turning the boolean
#     `deg += (row[j] != 0)` into branchless carry-flag arithmetic.
#   * Compare with demo.O0.s (every variable spilled to the stack, each loop a
#     literal compare-and-jump) and demo.O2.s (heavier scheduling/unrolling).
# =============================================================================
	.ident	"clang version 20.1.8"
	.section	".note.GNU-stack","",@progbits
