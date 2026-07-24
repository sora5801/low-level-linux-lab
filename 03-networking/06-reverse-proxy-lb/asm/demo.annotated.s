# =============================================================================
# demo.annotated.s — clang -O1 output for demo.c, explained instruction by
# instruction. These are the load balancer's three backend-selection routines:
#   fnv1a_32        — the hash used for client IPs AND ring virtual-node labels
#   ring_lookup     — consistent-hash ring: a binary search over sorted vnodes
#   least_conn_pick — least-connections policy: a linear min-scan
# This is the decision logic the proxy runs for every incoming connection.
# =============================================================================
#
# HOW TO READ THIS FILE
# ---------------------
# This is the EXACT assembly clang 20.1.8 emits for demo.c at -O1 (see demo.s for
# the untouched original), with a comment on essentially every instruction. AT&T
# syntax throughout, which means:
#
#     op   src, dst                      # movl %esp,%ebp   =>  ebp = esp
#     %reg      register       $imm immediate     sym(%rip) RIP-relative address
#     N(%reg)   memory at [reg+N]         (%rb,%ri,S) memory at [rb + ri*S]
#
# A register's narrow names are the SAME physical register: rax/eax/ax/al. Writing
# a 32-bit name (eax) ZERO-EXTENDS into the full 64-bit register, which is why the
# compiler prefers `movl`/`xorl` when the top half should be zero. `movslq`
# SIGN-extends a 32-bit value into 64 bits (used to turn a signed array index into
# a pointer offset). `movzbl` zero-extends a single byte into 32 bits.
#
# THE SYSTEM V AMD64 ABI (the contract every function below obeys)
# ----------------------------------------------------------------
#   * Integer/pointer arguments, in order:  rdi, rsi, rdx, rcx, r8, r9  (then stack)
#   * Return value:                          rax  (eax for a 32-bit int/u32 return)
#   * Caller-saved (scratch; callee may clobber): rax, rcx, rdx, rsi, rdi, r8-r11
#   * Callee-saved (callee MUST preserve):        rbx, rbp, r12-r15, rsp
#   * The RED ZONE: 128 bytes below rsp a leaf function may use without moving rsp.
#     All three functions here are leaves (they call nobody), so none allocates a
#     stack frame beyond the debug-only rbp push, and none touches the red zone.
#   * Stack alignment: rsp must be 16-byte aligned at a `call`. Nothing here calls,
#     so the only stack traffic is the frame-pointer push/pop kept by -O1 for
#     legible backtraces.
#
# THE ARGUMENTS OF EACH FUNCTION (so the register names below read as values)
#   fnv1a_32(const unsigned char *data /rdi/, usize len /rsi/) -> u32 /eax/
#   ring_lookup(const struct rnode *nodes /rdi/, int n /esi/, u32 key /edx/)
#                                                             -> int /eax/
#       struct rnode { u32 hash; int idx; }  is 8 bytes: .hash at +0, .idx at +4,
#       so nodes[i] is addressed as (%rdi + i*8), a scale-8 index.
#   least_conn_pick(const int *conns /rdi/, const unsigned char *eligible /rsi/,
#                   int n /edx/) -> int /eax/
# =============================================================================

	.file	"demo.c"
	.text

# =============================================================================
# fnv1a_32(const unsigned char *data /rdi/, usize len /rsi/) -> u32 /eax/
#   u32 h = 2166136261;
#   for (i=0; i<len; i++) { h ^= data[i]; h *= 16777619; }
#   return h;
# The whole hash lives in eax (which is also the return register), so the loop
# body is just "xor a byte in, multiply by the prime" with nothing spilled.
# =============================================================================
	.globl	fnv1a_32
	.p2align	4                       # 16-byte-align the entry (I-fetch friendly)
	.type	fnv1a_32,@function
fnv1a_32:
	pushq	%rbp                        # PROLOGUE: save caller's frame pointer
	movq	%rsp, %rbp                  #   establish our frame (debug only; leaf)
	movl	$-2128831035, %eax          # h = 0x811C9DC5 = 2166136261, the FNV offset
                                        #   basis. The literal prints signed, but the
                                        #   bit pattern is the unsigned seed. eax = h.
	testq	%rsi, %rsi                  # len == 0 ?  (test sets ZF if rsi is zero)
	je	.LBB0_3                         #   if so, skip the loop and return the seed
# ---- loop setup -------------------------------------------------------------
	xorl	%ecx, %ecx                  # i = 0  (rcx is the byte index / cursor)
	.p2align	4                       # align the hot loop head for the fetcher
.LBB0_2:                                # do { ... } while (i != len)
	movzbl	(%rdi,%rcx), %edx           # edx = (u32)data[i]  — zero-extend one byte
	xorl	%eax, %edx                  # edx = h ^ data[i]    — FNV-1a XORs FIRST ...
	imull	$16777619, %edx, %eax       # eax = edx * 16777619 — ... THEN multiplies by
                                        #   the FNV prime. The 32-bit imul keeps the low
                                        #   32 bits, i.e. the multiply wraps mod 2^32 —
                                        #   that wraparound is what makes h a ring coord.
	incq	%rcx                        # i++
	cmpq	%rcx, %rsi                  # compare len, i  (computes len - i, sets flags)
	jne	.LBB0_2                        #   loop while i != len
.LBB0_3:
	popq	%rbp                        # EPILOGUE: restore caller's frame pointer
	retq                                # return h in eax
.Lfunc_end0:
	.size	fnv1a_32, .Lfunc_end0-fnv1a_32

# =============================================================================
# ring_lookup(const struct rnode *nodes /rdi/, int n /esi/, u32 key /edx/) -> int
#   if (n <= 0) return -1;
#   lo = 0; hi = n;
#   while (lo < hi) {
#       mid = lo + ((hi - lo) >> 1);
#       if (nodes[mid].hash < key) lo = mid + 1;  else hi = mid;
#   }
#   if (lo == n) lo = 0;              // wrap around the ring
#   return nodes[lo].idx;
#
# This is a left-most binary search (lower_bound): find the first vnode whose
# hash >= key — the next node CLOCKWISE from the key — then wrap to node 0 if the
# key sat past the top of the ring. clang peels the first iteration and lets each
# branch re-test the loop condition, so there are two `cmp lo,hi` exits below.
# Register map inside the loop:  eax = lo,  ecx = hi,  edx = key,
#                                r8d = (hi-lo)/2,  r9d = mid,  r10 = mid (64-bit).
# =============================================================================
	.globl	ring_lookup
	.p2align	4
	.type	ring_lookup,@function
ring_lookup:
	pushq	%rbp                        # PROLOGUE: save frame pointer
	movq	%rsp, %rbp                  #   establish frame (debug only; leaf)
	testl	%esi, %esi                  # n <= 0 ?  (signed test on n)
	jle	.LBB1_1                        #   empty ring -> return -1 (tail below)
# ---- loop init --------------------------------------------------------------
	xorl	%eax, %eax                  # lo = 0
	movl	%esi, %ecx                  # hi = n
	jmp	.LBB1_3                        # enter the loop at the header (first mid calc)
	.p2align	4
# ---- else branch: nodes[mid].hash >= key, so hi = mid ----------------------
.LBB1_5:
	movl	%r9d, %ecx                  # hi = mid
	cmpl	%ecx, %eax                  # compare hi, lo  (computes lo - hi)
	jge	.LBB1_7                        #   if lo >= hi, the range is empty -> exit loop
.LBB1_3:                                # LOOP HEADER: recompute the midpoint
	movl	%ecx, %r8d                  # r8d = hi
	subl	%eax, %r8d                  # r8d = hi - lo   (>= 0 while lo < hi)
	sarl	%r8d                        # r8d = (hi - lo) >> 1  (halve the range)
	leal	(%r8,%rax), %r9d            # r9d = mid = lo + (hi-lo)/2   (LEA = add, no flags)
	movslq	%r9d, %r10                  # r10 = (int64)mid — sign-extend for addressing
	cmpl	%edx, (%rdi,%r10,8)         # compare nodes[mid].hash (at rdi + mid*8, +0)
                                        #   against key(edx). UNSIGNED compare because
                                        #   .hash is u32 and the ring coord is unsigned.
	jae	.LBB1_5                        #   if nodes[mid].hash >= key -> hi = mid branch
# ---- then branch: nodes[mid].hash < key, so lo = mid + 1 -------------------
	addl	%r8d, %eax                  # lo += (hi-lo)/2      \  together: lo = mid + 1
	incl	%eax                        # lo += 1              /
	cmpl	%ecx, %eax                  # compare hi, lo  (lo - hi)
	jl	.LBB1_3                        #   if lo < hi, keep searching; else fall through
# ---- loop exit: eax = lo = first index with hash >= key --------------------
.LBB1_7:
	xorl	%ecx, %ecx                  # tentative result index = 0 (the wrap target)
	cmpl	%esi, %eax                  # compare n, lo  (lo == n ?)
	cmovnel	%eax, %ecx                  # if lo != n, index = lo; else keep 0 (WRAP: the
                                        #   key was past the last vnode, so it maps to
                                        #   node 0 — a branchless conditional move).
	movslq	%ecx, %rax                  # rax = (int64)index — sign-extend for addressing
	movl	4(%rdi,%rax,8), %eax        # return nodes[index].idx  (offset +4 within the
                                        #   8-byte struct; the backend this arc routes to)
	popq	%rbp                        # EPILOGUE
	retq
# ---- n <= 0: empty ring ----------------------------------------------------
.LBB1_1:
	movl	$-1, %eax                   # return -1 (no eligible backend)
	popq	%rbp
	retq
.Lfunc_end1:
	.size	ring_lookup, .Lfunc_end1-ring_lookup

# =============================================================================
# least_conn_pick(const int *conns /rdi/, const unsigned char *eligible /rsi/,
#                 int n /edx/) -> int /eax/
#   best = -1; bestc = 0;
#   for (i=0; i<n; i++) {
#       if (!eligible[i]) continue;
#       if (best < 0 || conns[i] < bestc) { best = i; bestc = conns[i]; }
#   }
#   return best;
# A straight linear min-scan. Register map: eax = best, r8d = bestc, rdx = i,
# rcx = n. The `best < 0` seed guarantees the first eligible backend is taken
# unconditionally (the `js` below), after which only a strictly-smaller count wins.
# =============================================================================
	.globl	least_conn_pick
	.p2align	4
	.type	least_conn_pick,@function
least_conn_pick:
	pushq	%rbp                        # PROLOGUE: save frame pointer
	movq	%rsp, %rbp                  #   establish frame (debug only; leaf)
	testl	%edx, %edx                  # n <= 0 ?
	jle	.LBB2_1                        #   nothing to scan -> return -1
# ---- loop init --------------------------------------------------------------
	movl	%edx, %ecx                  # rcx = n  (save the bound; edx is about to be i)
	movl	$-1, %eax                   # best = -1  ("nothing chosen yet")
	xorl	%edx, %edx                  # i = 0
	xorl	%r8d, %r8d                  # bestc = 0  (unused until best >= 0)
	jmp	.LBB2_4                        # enter at the eligibility test
	.p2align	4
# ---- take this candidate: best = i, bestc = conns[i] -----------------------
.LBB2_7:
	movl	(%rdi,%rdx,4), %r8d         # bestc = conns[i]   (stride 4 = sizeof(int))
	movl	%edx, %eax                  # best  = i
.LBB2_8:                                # loop tail: advance i and test the bound
	incq	%rdx                        # i++
	cmpq	%rdx, %rcx                  # compare n, i  (i == n ?)
	je	.LBB2_2                        #   done -> return best
.LBB2_4:                                # LOOP HEADER
	cmpb	$0, (%rsi,%rdx)             # eligible[i] == 0 ?  (one byte per backend)
	je	.LBB2_8                        #   down/draining -> skip (continue)
# ---- eligible: is it better than the current best? -------------------------
	testl	%eax, %eax                  # best < 0 ?  (test the sign of best)
	js	.LBB2_7                        #   yes: first eligible backend, take it outright
	cmpl	%r8d, (%rdi,%rdx,4)         # compare conns[i], bestc  (conns[i] - bestc)
	jl	.LBB2_7                        #   conns[i] < bestc -> strictly emptier, take it
	jmp	.LBB2_8                        #   else keep current best, continue
# ---- n <= 0 fell straight here with best still -1 --------------------------
.LBB2_1:
	movl	$-1, %eax                   # return -1 (no eligible backend)
.LBB2_2:
	popq	%rbp                        # EPILOGUE
	retq
.Lfunc_end2:
	.size	least_conn_pick, .Lfunc_end2-least_conn_pick

	.ident	"clang version 20.1.8"          # toolchain stamp (harmless metadata)
	.section	".note.GNU-stack","",@progbits  # mark the stack non-executable
	.addrsig                                # address-significance table (linker ICF hint)
# =============================================================================
# WHAT TO TAKE AWAY
#   * fnv1a_32's whole state is one register (eax): xor a byte, imul by the prime.
#     The 32-bit imul is the modular arithmetic that makes the hash a ring coord.
#   * ring_lookup is lower_bound in ~11 instructions of loop body; the WRAP that
#     closes the ring is a single branchless `cmovne` at the end, and the struct
#     field access is just a +0 (hash) vs +4 (idx) displacement on a scale-8 index.
#   * least_conn_pick shows how `best < 0` seeds a min-scan so the first candidate
#     is taken via one `js`, with no separate "is this the first?" bookkeeping.
#   * Compare with demo.O0.s (every variable spilled to the stack, one C statement
#     per clump of instructions) and demo.O2.s (the loops unrolled/reassociated).
# =============================================================================
