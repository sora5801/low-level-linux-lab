# =============================================================================
# demo.annotated.s — asm/demo.c at -O1, explained instruction by instruction.
# =============================================================================
#
# HOW TO READ THIS FILE
# ---------------------
# This is the exact assembly clang emits for asm/demo.c at -O1 (see demo.s for
# the untouched original), annotated on essentially every instruction. AT&T
# syntax throughout:
#
#     op   source, destination          # movl $1, %eax   =>  eax = 1
#     %reg                               # a register
#     $imm                               # an immediate (literal) constant
#     N(%reg)                            # memory at [reg + N]
#     (%b,%i,%s)                         # memory at [base + index*scale]
#
# Register widths are the SAME physical register: rax(64)/eax(32)/ax(16)/al(8).
# Writing a 32-bit name (eax) ZERO-EXTENDS into the 64-bit register, which is
# why clang uses `movl $1,%eax` (5 bytes) rather than `movq` (7) when the top
# 32 bits should be zero.
#
# THE SysV AMD64 ABI CONTRACT (what every function here obeys)
# -----------------------------------------------------------
#   integer/pointer ARGUMENTS, in order:  rdi, rsi, rdx, rcx, r8, r9   (then stack)
#   RETURN value:                         rax
#   CALLEE-SAVED (a function must preserve): rbx, rbp, r12, r13, r14, r15, rsp
#   CALLER-SAVED (free to clobber):       rax, rcx, rdx, rsi, rdi, r8-r11
#   STACK: 16-byte aligned at a `call`; the 128 bytes below rsp are the red zone.
#
# So for our four functions the arguments land as:
#   lat_bucket(u64 v)              -> v in rdi                 ret in rax
#   bucket_low(u32 b)              -> b in edi                 ret in rax
#   bucket_high(u32 b)             -> b in edi                 ret in rax
#   hist_record(u64 *slots, u64 v) -> slots in rdi, v in rsi   (void)
#
# THE BIG LESSON
# --------------
# In the C, lat_bucket() is a "shift right until zero, counting bits" LOOP. At
# -O0 (see demo.O0.s) that loop is emitted literally, and hist_record() makes a
# real `call lat_bucket`. But already at -O1, shown here, clang:
#   (1) recognizes the loop as "count significant bits" and replaces the ENTIRE
#       loop with ONE `bsr` (bit-scan-reverse) instruction — exactly what the
#       kernel's fls64() intrinsic would emit; and
#   (2) INLINES lat_bucket into hist_record (no call survives).
# It also turns every `if` into a branchless compute-then-`cmov` select, and in
# bucket_high it PROVES the `if (b==0)` special case is redundant and deletes it.
# Reading this file is how you SEE all of that happen.
# =============================================================================

	.file	"demo.c"
	.text                                   # executable code section

# =============================================================================
# u32 lat_bucket(u64 v)   —   number of significant bits of v (0 if v==0).
#   v arrives in rdi; result leaves in eax/rax.
#   The C was a loop; clang collapsed it to bit-scan + a branchless select.
# =============================================================================
	.globl	lat_bucket
	.p2align	4                       # 16-byte align the entry (fetch-friendly)
	.type	lat_bucket,@function
lat_bucket:
# ---- PROLOGUE (frame pointer kept at -O1 for legible backtraces) ------------
	pushq	%rbp                            # save caller's rbp (callee-saved)
	movq	%rsp, %rbp                      # rbp = frame base for this call
# ---- THE WHOLE LOOP, AS ONE INSTRUCTION -------------------------------------
	bsrq	%rdi, %rcx                      # rcx = Bit-Scan-Reverse(rdi) = index
	                                        #   of the highest set bit (0..63).
	                                        #   NOTE: bsr's result is UNDEFINED if
	                                        #   rdi==0 — that case is fixed up below,
	                                        #   so the garbage in rcx is harmless.
	incl	%ecx                            # ecx = bsr+1 = count of significant
	                                        #   bits = the bucket, for v != 0.
# ---- BRANCHLESS "v==0 ? 0 : ecx" --------------------------------------------
	xorl	%eax, %eax                      # eax = 0  (the answer when v==0);
	                                        #   xor-self is the 2-byte zero idiom
	                                        #   and breaks the dependency chain.
	testq	%rdi, %rdi                      # set ZF = (rdi == 0) without altering rdi
	cmovnel	%ecx, %eax                      # if v != 0 (ZF clear) eax = ecx;
	                                        #   else eax stays 0. A conditional MOVE:
	                                        #   no branch, so no misprediction. This
	                                        #   is how the compiler expressed the C
	                                        #   `while`/return without a jump.
# ---- EPILOGUE ---------------------------------------------------------------
	popq	%rbp                            # restore caller's rbp
	retq                                    # return; result already in eax
.Lfunc_end0:
	.size	lat_bucket, .Lfunc_end0-lat_bucket

# =============================================================================
# u64 bucket_low(u32 b)   —   0 if b==0, else 2^(b-1).   b in edi, ret in rax.
#   Shows a variable-count left shift (shl by %cl) and another cmov select.
# =============================================================================
	.globl	bucket_low
	.p2align	4
	.type	bucket_low,@function
bucket_low:
	pushq	%rbp                            # PROLOGUE
	movq	%rsp, %rbp
	                                        # (clang note: edi is already zero-extended
	                                        #  into rdi by the caller's 32-bit write)
	leal	-1(%rdi), %ecx                  # ecx = b - 1 = the shift amount. LEA does
	                                        #   the subtract as an address calc without
	                                        #   touching flags or needing a temp.
	movl	$1, %edx                        # edx = 1 (the value we will shift left)
	shlq	%cl, %rdx                       # rdx = 1 << (b-1). x86 shifts read the
	                                        #   count from CL (low 8 bits) and MASK it
	                                        #   mod 64, so a huge count wraps — fine
	                                        #   here because the result is discarded
	                                        #   whenever b==0 (see cmov below).
	xorl	%eax, %eax                      # eax = 0  (the answer when b==0)
	cmpl	$1, %edi                        # compare b with 1: sets CF if b < 1,
	                                        #   i.e. CF set exactly when b==0 (unsigned)
	cmovaeq	%rdx, %rax                      # if b >= 1 (Above-or-Equal, CF clear)
	                                        #   rax = rdx = 2^(b-1); else rax stays 0.
	popq	%rbp                            # EPILOGUE
	retq
.Lfunc_end1:
	.size	bucket_low, .Lfunc_end1-bucket_low

# =============================================================================
# u64 bucket_high(u32 b)  —   0 if b==0, else 2^b - 1.   b in edi, ret in rax.
#   The interesting case: clang DELETED the `if (b==0)` branch, because the
#   general formula (1<<b)-1 already evaluates to 0 at b==0. Fewer instructions,
#   no branch at all.
# =============================================================================
	.globl	bucket_high
	.p2align	4
	.type	bucket_high,@function
bucket_high:
	pushq	%rbp                            # PROLOGUE
	movq	%rsp, %rbp
	movl	%edi, %ecx                      # ecx = b (the shift count)
	movq	$-1, %rax                       # rax = 0xFFFFFFFFFFFFFFFF (all ones)
	shlq	%cl, %rax                       # rax = -1 << b  = ...1110000 (b low zeros).
	                                        #   At b==0 this shifts by 0, leaving all
	                                        #   ones, which the `not` turns into 0 —
	                                        #   THAT is why the b==0 branch was safe to
	                                        #   drop. (Caveat: b==64 would mask to a
	                                        #   shift of 0 and give the wrong answer;
	                                        #   the C `1<<64` is UB, and this diagnostic
	                                        #   helper is only ever called with b<64.)
	notq	%rax                            # rax = ~(-1 << b) = (1<<b) - 1: the low b
	                                        #   bits set = the inclusive upper bound.
	popq	%rbp                            # EPILOGUE
	retq
.Lfunc_end2:
	.size	bucket_high, .Lfunc_end2-bucket_high

# =============================================================================
# void hist_record(u64 *slots, u64 value)
#   slots in rdi, value in rsi. Increment slots[ clamp(lat_bucket(value)) ].
#   Note: lat_bucket was INLINED (no `call`), and the source clamp
#   `if (b >= 65) b = 64` became a branchless `min(b, 64)` because clang knows
#   b (= bsr+1) can be at most 64 for a 64-bit input.
# =============================================================================
	.globl	hist_record
	.p2align	4
	.type	hist_record,@function
hist_record:
	pushq	%rbp                            # PROLOGUE
	movq	%rsp, %rbp
# ---- inlined lat_bucket(value) ----------------------------------------------
	bsrq	%rsi, %rax                      # rax = highest-set-bit index of value
	                                        #   (the inlined bit-scan; no call made)
	incl	%eax                            # eax = bsr+1 = bucket b (for value != 0)
# ---- clamp: ecx = min(b, 64) ------------------------------------------------
	cmpl	$64, %eax                       # compare b with 64
	movl	$64, %ecx                       # ecx = 64 (the ceiling / default)
	cmovbl	%eax, %ecx                      # if b < 64 (Below) ecx = b; else keep 64.
	                                        #   This is the array-bounds clamp: index
	                                        #   can never exceed HIST_SLOTS-1 (=64), so
	                                        #   the increment below cannot run off the
	                                        #   array — an out-of-bounds write here would
	                                        #   be kernel memory corruption.
# ---- branchless value==0 -> index 0 -----------------------------------------
	xorl	%eax, %eax                      # eax = 0 (index for the value==0 bucket)
	testq	%rsi, %rsi                      # ZF = (value == 0)
	cmovnel	%ecx, %eax                      # if value != 0, eax = clamped bucket;
	                                        #   else eax stays 0.
# ---- slots[index]++ ---------------------------------------------------------
	incq	(%rdi,%rax,8)                   # ++*(u64*)(slots + index). The (base,
	                                        #   index, 8) addressing scales by 8 = the
	                                        #   size of a u64 slot; one memory RMW.
	                                        #   In the real kernel this slot is an
	                                        #   atomic64_t and the increment is a
	                                        #   `lock incq` / `lock xadd` so concurrent
	                                        #   CPUs cannot lose a count; here, extracted
	                                        #   without the atomics header, it is a plain
	                                        #   increment — the addressing/indexing math,
	                                        #   which is the didactic part, is identical.
	popq	%rbp                            # EPILOGUE
	retq
.Lfunc_end3:
	.size	hist_record, .Lfunc_end3-hist_record

	.ident	"clang version 20.1.8"          # toolchain stamp (harmless metadata)
	.section	".note.GNU-stack","",@progbits  # mark the stack non-executable (security default)
	.addrsig                                # address-significance table (LTO metadata)
# =============================================================================
# WHAT TO TAKE AWAY
#   * A counting-bits LOOP became a single `bsr`. The kernel spells that fls64();
#     writing the readable loop and letting the compiler find the instruction is
#     usually as fast and far clearer. Compare demo.O0.s (the real loop) here.
#   * `if` conditions became branchless `cmov` selects — no branch to mispredict.
#   * The optimizer inlined lat_bucket into hist_record and PROVED away a dead
#     `if (b==0)` in bucket_high. Reading asm is how you confirm what it did.
#   * `bsr` is undefined on input 0 and `shl`'s count is masked mod 64: two
#     hardware corner cases the surrounding test/cmov code carefully sidesteps.
# =============================================================================
