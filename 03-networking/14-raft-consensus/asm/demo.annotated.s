# =============================================================================
# demo.annotated.s — clang -O1 output for demo.c, explained instruction by
# instruction. These are Raft's two safety-critical pure-logic routines:
#   log_up_to_date       — the ELECTION RESTRICTION: may a voter back this
#                          candidate? A branchless lexicographic (term,index) test.
#   majority_match_index — the COMMIT-INDEX computation: the highest log index a
#                          MAJORITY of nodes store, via an insertion sort + median.
# Together they are the whole safety argument: (1) only an up-to-date candidate
# can win, so a new leader holds every committed entry, and (2) a leader only
# commits an index a majority holds — and any two majorities overlap.
# =============================================================================
#
# HOW TO READ THIS FILE
# ---------------------
# This is the EXACT assembly clang 20.1.8 emits for demo.c at -O1 (see demo.s for
# the untouched original), with a comment on essentially every instruction. AT&T
# syntax throughout, which means:
#
#     op   src, dst                      # movq %rsp,%rbp   =>  rbp = rsp
#     %reg      register       $imm immediate     sym(%rip) RIP-relative address
#     N(%reg)   memory at [reg+N]         N(%rb,%ri,S) memory at [rb + ri*S + N]
#
# A register's narrow names are the SAME physical register: rax/eax/ax/al. Writing
# a 32-bit name (eax) ZERO-EXTENDS into the full 64-bit register, which is why the
# compiler prefers `movl`/`xorl` when the top half should be zero. `cltq`
# SIGN-extends eax into rax. `movslq` sign-extends a 32-bit source into a 64-bit
# dest (used to turn a signed array index into a pointer offset).
#
# CONDITION CODES USED BELOW (set by cmp/test, which compute dst-src and discard
# the result, keeping only the flags):
#     seta/ja   : UNSIGNED  >     (CF=0 and ZF=0)      "above"
#     setae/jae : UNSIGNED  >=    (CF=0)               "above or equal"
#     jbe       : UNSIGNED  <=    (CF=1 or ZF=1)       "below or equal"
#     cmove     : move if ZF=1    (the two compared operands were EQUAL)
#     cmovge/jg : SIGNED  >=/>                          (for loop counters)
# Raft terms and indices are UNSIGNED 64-bit, so the value comparisons are
# unsigned (seta/setae/jbe); the loop counters are signed ints (jg/cmovge).
#
# THE SYSTEM V AMD64 ABI (the contract every function here obeys)
# --------------------------------------------------------------
#   * Integer/pointer args, in order:  rdi, rsi, rdx, rcx, r8, r9   (then stack)
#   * Return value:                    rax  (eax for a 32-bit int)
#   * Caller-saved (scratch): rax, rcx, rdx, rsi, rdi, r8-r11  — a called function
#     may trash these, so a value that must survive a `call` cannot live here.
#   * Callee-saved (preserved): rbx, rbp, r12-r15, rsp — a function that uses one
#     must push it on entry and pop it on exit. majority_match_index does exactly
#     that (rbx, r14) to keep `n` and the count alive across its memcpy call.
#   * The RED ZONE: 128 bytes below rsp a LEAF may use without moving rsp.
#     log_up_to_date is a leaf and needs no locals, so it touches no stack beyond
#     the debug-only rbp push. majority_match_index is NOT a leaf (it calls
#     memcpy), so it cannot use the red zone and allocates a real 128-byte frame.
#   * Stack alignment: rsp must be 16-byte aligned at the point of a `call`.
#
# ARGUMENTS OF EACH FUNCTION (so the register names below read as values)
#   log_up_to_date(u64 cand_term /rdi/, u64 cand_index /rsi/,
#                  u64 my_term /rdx/, u64 my_index /rcx/) -> int /eax/
#   majority_match_index(const u64 *match /rdi/, int n /esi/) -> u64 /rax/
#       (u64 is `unsigned long` = 64-bit on LP64; the tmp[] copy is 16*8 = 128 B)
# =============================================================================

	.file	"demo.c"
	.text

# =============================================================================
# log_up_to_date — the election restriction, computed WITHOUT a branch.
#
#   if (cand_term != my_term) return cand_term > my_term;   // newer term wins
#   return cand_index >= my_index;                          // tie: longer wins
#
# The trick: evaluate BOTH possible answers up front —
#     A = (cand_index >= my_index)   into al
#     B = (cand_term  >  my_term)    into cl
# then use whether the terms were EQUAL to pick between them with a `cmove`. No
# conditional jump means no branch misprediction — safety in ~10 straight-line
# instructions.
# =============================================================================
	.globl	log_up_to_date
	.p2align	4                       # 16-byte-align the entry (I-fetch friendly)
	.type	log_up_to_date,@function
log_up_to_date:
	pushq	%rbp                        # PROLOGUE: save caller's frame pointer
	movq	%rsp, %rbp                  #   establish our frame (debug only; leaf)

# ---- answer for the EQUAL-terms case: cand_index >= my_index ----------------
	xorl	%eax, %eax                  # eax = 0 (clear before the setcc byte-set)
	cmpq	%rcx, %rsi                  # compute cand_index(rsi) - my_index(rcx);
	                                    #   sets CF/ZF for an UNSIGNED comparison
	setae	%al                         # al = (rsi >= rcx, unsigned) = (cand_index
	                                    #   >= my_index). This is the tie-breaker
	                                    #   answer, parked in eax for now.

# ---- answer for the DIFFERENT-terms case: cand_term > my_term ---------------
	xorl	%ecx, %ecx                  # ecx = 0 (its flag effects are irrelevant:
	                                    #   the next cmpq re-sets the flags)
	cmpq	%rdx, %rdi                  # compute cand_term(rdi) - my_term(rdx);
	                                    #   ZF=1 here means the terms are EQUAL
	seta	%cl                         # cl = (rdi > rdx, unsigned) = (cand_term
	                                    #   > my_term). This is the newer-term answer.

# ---- select: if terms are equal, use the index answer; else the term answer -
	cmovel	%eax, %ecx                  # if ZF (cand_term == my_term): ecx = eax
	                                    #   (the >= index result); else keep cl
	                                    #   (the > term result). Branchless pick.
	movzbl	%cl, %eax                   # zero-extend the chosen 0/1 byte into eax,
	                                    #   the return register (an `int` 0 or 1)
	popq	%rbp                        # EPILOGUE
	retq                                # return the boolean in eax
.Lfunc_end0:
	.size	log_up_to_date, .Lfunc_end0-log_up_to_date

# =============================================================================
# majority_match_index(const u64 *match /rdi/, int n /esi/) -> u64 /rax/
#
#   if (n <= 0) return 0;
#   if (n > 16) n = 16;
#   u64 tmp[16]; memcpy(tmp, match, n*8);      // work on a copy
#   insertion_sort_ascending(tmp, n);
#   return tmp[n - (n/2 + 1)];                 // highest index a majority reaches
#
# Register roles after setup:  ebx = clamped n,  r14d = original n (across memcpy),
#   rcx = outer index i,  rax = outer bound n,  rsi = inner cursor (j+1),
#   rdx = key = tmp[i],  rdi = tmp[j]. The sorted array lives at -144(%rbp).
# =============================================================================
	.globl	majority_match_index
	.p2align	4
	.type	majority_match_index,@function
majority_match_index:
# ---- guard: empty input returns 0 (checked BEFORE building a frame) ----------
	testl	%esi, %esi                  # n <= 0 ?  (signed test on n)
	jle	.LBB1_1                         #   yes -> return 0 with no frame at all

# ---- PROLOGUE + frame (only reached when n >= 1) ----------------------------
	pushq	%rbp                        # save caller's frame pointer
	movq	%rsp, %rbp                  #   rbp = frame base
	pushq	%r14                        # save callee-saved r14 (will hold `n`
	                                    #   across the memcpy call — caller-saved
	                                    #   regs would be clobbered by memcpy)
	pushq	%rbx                        # save callee-saved rbx (will hold clamped n)
	subq	$128, %rsp                  # reserve 128 bytes = tmp[16] (16 * 8). After
	                                    #   3 pushes (24B) + 128B, rsp is 16-byte
	                                    #   aligned at the call below, per the ABI.

# ---- clamp n to [.., 16] and compute the byte count for memcpy --------------
	movq	%rdi, %rax                  # rax = match pointer (save; rdi is reused
	                                    #   as the memcpy DEST argument next)
	cmpl	$16, %esi                   # n vs 16
	movl	$16, %ebx                   # ebx = 16 (the cap)
	cmovll	%esi, %ebx                  # if n < 16 (signed), ebx = n; else ebx = 16.
	                                    #   ebx = min(n,16) = the clamped count.
	leal	-1(%rbx), %ecx              # ecx = ebx - 1
	leaq	8(,%rcx,8), %rdx            # rdx = 8 + (ebx-1)*8 = ebx*8  (bytes to copy).
	                                    #   LEA does the multiply-add with no flags.
	leaq	-144(%rbp), %rdi            # rdi = &tmp[0]  (memcpy arg1, dest). tmp sits
	                                    #   at rbp-144 .. rbp-16 (below the saved regs)
	movl	%esi, %r14d                 # r14d = original n, preserved across the call
	movq	%rax, %rsi                  # rsi = match (memcpy arg2, source)
	callq	memcpy@PLT                  # memcpy(tmp, match, n*8). The compiler turned
	                                    #   the C copy loop into a libc memcpy call;
	                                    #   @PLT routes through the procedure linkage
	                                    #   table for the dynamic symbol.

# ---- if n == 1 the copy is already sorted; skip straight to the pick ---------
	cmpl	$1, %r14d                   # original n == 1 ?
	jne	.LBB1_3                         #   no (n >= 2) -> run the insertion sort
	                                    #   yes -> fall through to .LBB1_9 (pick)

# ---- .LBB1_9: return tmp[n - (n/2 + 1)] -------------------------------------
.LBB1_9:
	movl	%ebx, %eax                  # eax = n (clamped)
	shrl	%eax                        # eax = n >> 1 = n/2  (logical: n >= 0)
	notl	%eax                        # eax = ~(n/2) = -(n/2) - 1   (two's complement)
	addl	%ebx, %eax                  # eax = n + (-(n/2)-1) = n - n/2 - 1
	                                    #     = n - (n/2 + 1) = n - majority.
	                                    #   `not`+`add` computes the subtract in two
	                                    #   cheap ops with no extra register.
	cltq                                # sign-extend eax into rax for use as an index
	movq	-144(%rbp,%rax,8), %rax     # rax = tmp[n - majority] — the return value:
	                                    #   the highest index a MAJORITY of nodes hold
# ---- EPILOGUE: tear down the frame and restore callee-saved regs -------------
	addq	$128, %rsp                  # pop the tmp[] buffer
	popq	%rbx                        # restore rbx
	popq	%r14                        # restore r14
	popq	%rbp                        # restore caller's frame pointer
	retq                                # return tmp[n-majority] in rax

# ---- n <= 0: the early guard lands here (no frame was built) ----------------
.LBB1_1:
	xorl	%eax, %eax                  # return 0
	retq

# ---- .LBB1_3: set up the insertion sort (only when n >= 2) ------------------
.LBB1_3:
	cmpl	$3, %ebx                    # n vs 3  ...
	movl	$2, %eax                    # eax = 2 ...
	cmovgel	%ebx, %eax                  # eax = (n >= 3) ? n : 2. Since n >= 2 here,
	                                    #   this just materializes eax = n as the
	                                    #   OUTER-loop bound (i runs until i == n).
	movl	$1, %ecx                    # rcx = i = 1  (outer index; prefix [0,1) sorted)
	jmp	.LBB1_4                         # enter the outer loop header

	.p2align	4
# ---- inner-loop exit when we shifted all the way down to slot 0 -------------
.LBB1_7:                                #   in Loop: Header=BB1_4 Depth=1
	xorl	%esi, %esi                  # insertion slot = 0 (key is the new minimum)

# ---- place the key, then advance the outer loop -----------------------------
.LBB1_8:                                #   in Loop: Header=BB1_4 Depth=1
	movslq	%esi, %rsi                  # sign-extend the slot index to 64 bits
	movq	%rdx, -144(%rbp,%rsi,8)     # tmp[slot] = key  (drop key into its place)
	incq	%rcx                        # i++
	cmpq	%rax, %rcx                  # i == n ?  (rax = n, the outer bound)
	je	.LBB1_9                         #   done sorting -> go compute the pick

# ---- .LBB1_4: OUTER loop header — grab the next key -------------------------
.LBB1_4:                                # => outer loop, Depth 1
	movq	-144(%rbp,%rcx,8), %rdx     # rdx = key = tmp[i]
	movq	%rcx, %rsi                  # rsi = i  (the inner cursor starts at i and
	                                    #   names the DESTINATION slot j+1; tmp[rsi-1]
	                                    #   is tmp[j])
	.p2align	4
# ---- .LBB1_5: INNER loop — shift elements > key one slot up ------------------
.LBB1_5:                                # => inner loop, Depth 2
	movq	-152(%rbp,%rsi,8), %rdi     # rdi = tmp[rsi-1] = tmp[j]   (-152 = -144 - 8,
	                                    #   i.e. one u64 below &tmp[rsi]). rsi >= 1 on
	                                    #   every entry here, so this never underflows.
	cmpq	%rdx, %rdi                  # compare tmp[j](rdi) with key(rdx), unsigned
	jbe	.LBB1_8                         #   tmp[j] <= key -> stop; insert key at rsi
# %bb.6: tmp[j] > key, so slide it up and keep scanning down
	movq	%rdi, -144(%rbp,%rsi,8)     # tmp[rsi] = tmp[j]   (shift the larger up)
	decq	%rsi                        # rsi-- (move down: rsi now = j)
	leaq	1(%rsi), %rdi              # rdi = rsi + 1
	cmpq	$1, %rdi                    # (rsi+1) > 1 ?  i.e. is there still a tmp[rsi-1]
	jg	.LBB1_5                         #   yes (rsi >= 1) -> compare the next one down
	jmp	.LBB1_7                         #   no  (rsi == 0) -> key is the new minimum
.Lfunc_end1:
	.size	majority_match_index, .Lfunc_end1-majority_match_index

	.ident	"clang version 20.1.8"          # toolchain stamp (harmless metadata)
	.section	".note.GNU-stack","",@progbits  # mark the stack non-executable
	.addrsig                                # address-significance table (linker ICF hint)
# =============================================================================
# WHAT TO TAKE AWAY
#   * log_up_to_date is a BRANCHLESS lexicographic compare: compute both the
#     term-answer (seta) and the index-answer (setae), then `cmove` on term-
#     equality to pick. That is the entire election restriction — the rule that
#     guarantees a new leader already holds every committed entry — in ~10
#     straight-line instructions, no jumps to mispredict.
#   * majority_match_index shows a NON-leaf function: it calls memcpy, so it
#     builds a real 128-byte frame (no red zone) and keeps its live values (`n`,
#     the clamped count) in CALLEE-SAVED rbx/r14 across the call, because memcpy
#     is free to clobber every caller-saved register.
#   * The commit index is `tmp[n - (n/2 + 1)]` — the sorted median — and clang
#     computes `n - n/2 - 1` with a `shr`/`not`/`add` instead of a divide+sub.
#   * Compare with demo.O0.s (every variable spilled to the stack, one C statement
#     per clump of instructions) and demo.O2.s (the same, more aggressively
#     scheduled). The -O1 form here is the sweet spot for reading intent.
# =============================================================================
