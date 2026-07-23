# =============================================================================
# demo.annotated.s — clang's -O1 output for asm/demo.c, explained line by line.
# =============================================================================
#
# HOW TO READ THIS FILE
# ---------------------
# This is the exact assembly clang emits for asm/demo.c at -O1 (see demo.s for
# the untouched original), with a comment on essentially every instruction. It
# is AT&T syntax:
#
#     op   source, destination          # e.g.  imull $71, %edi, %eax
#     %reg                               # a register
#     $imm                               # an immediate (literal) constant
#     N(%reg)                            # memory at [reg + N]
#
# Register widths are views of ONE register:
#     rax (64) / eax (32) / ax (16) / al (8).  Writing eax ZERO-EXTENDS into the
#     full rax, which is why 32-bit ops appear even though C `unsigned int` math
#     conceptually lives in a 64-bit register.
#
# THE SysV AMD64 ABI CONTRACT (what every function here obeys)
# -----------------------------------------------------------
#   - integer/pointer args, in order:  rdi, rsi, rdx, rcx, r8, r9  (then stack)
#       => our single `u32 key` arg arrives in EDI (the 32-bit view of rdi).
#   - return value:                    rax  (EAX for a 32-bit result)
#   - callee-saved (a function MUST preserve): rbx, rbp, r12, r13, r14, r15, rsp
#   - caller-saved (free to clobber):  rax, rcx, rdx, rsi, rdi, r8, r9, r10, r11
#   - stack must be 16-byte aligned at the point of a `call`.
#   - the "red zone": 128 bytes below rsp a leaf function may scribble on
#     without adjusting rsp. Every function here is a leaf, so none touch the
#     stack for locals at all — the whole computation lives in registers.
#
# WHY THE PROLOGUE/EPILOGUE IS HERE AT ALL
# ----------------------------------------
# These functions make no `call`, so they need no frame. At -O1 with
# -fno-omit-frame-pointer clang still emits `push %rbp; mov %rsp,%rbp` ... `pop
# %rbp` purely so a debugger can unwind. It is two instructions of overhead with
# no functional effect; -O2 (see demo.O2.s) drops it and the functions become
# just "compute and ret".
#
# THE RCU CONNECTION (what the asm does NOT show — read this)
# ----------------------------------------------------------
# Every function below is pure arithmetic on a value passed in a register. There
# is NOT ONE memory access to shared state, and therefore NOT ONE memory
# barrier. That is the whole point of extracting it: in rcu_hashtable.c the RCU
# publish/consume barriers (rcu_assign_pointer's store-release, rcu_dereference's
# dependent-load) wrap the BUCKET ACCESS that consumes this index — the loads of
# bucket->first and node->value — never the index computation itself. Hashing is
# branch-free, memory-free arithmetic; RCU ordering is a property of the loads
# and stores that happen AFTER you hold the bucket number. Seeing this clean,
# barrier-free asm is what makes that separation concrete.
# =============================================================================

	.file	"demo.c"
	.text                           # executable code section

# =============================================================================
# u32 ht_hash(u32 key)   —   return key * GOLDEN_RATIO_32;
# The multiply in isolation. This is Fibonacci hashing's one essential step.
# =============================================================================
	.globl	ht_hash
	.p2align	4               # 16-byte align the entry (fetch-friendly)
	.type	ht_hash,@function
ht_hash:
# ---- PROLOGUE (debug-only frame; this is a leaf) ----------------------------
	pushq	%rbp                    # save caller's frame pointer
	movq	%rsp, %rbp              # rbp = frame base (for the debugger only)
# ---- BODY -------------------------------------------------------------------
	imull	$1640531527, %edi, %eax # eax = edi * 0x61C88647.  key is in edi
                                        #   (SysV arg0); the golden-ratio constant
                                        #   1640531527 == 0x61C88647. This 3-operand
                                        #   `imul imm, src, dst` writes the low 32
                                        #   bits of the product — exactly u32 wrap-
                                        #   around multiply, which is all a hash
                                        #   wants. Result parked in eax = return reg.
# ---- EPILOGUE ---------------------------------------------------------------
	popq	%rbp                    # restore caller's rbp
	retq                            # return; eax holds the scrambled key
.Lfunc_end0:
	.size	ht_hash, .Lfunc_end0-ht_hash

# =============================================================================
# u32 ht_bucket_highbits(u32 key)   —   return ht_hash(key) >> (32 - HT_BITS);
# The "keep the TOP bits" index scheme (what the kernel's hash_32 returns).
# ht_hash was INLINED — there is no call, just the multiply then a shift.
# =============================================================================
	.globl	ht_bucket_highbits
	.p2align	4
	.type	ht_bucket_highbits,@function
ht_bucket_highbits:
	pushq	%rbp                    # PROLOGUE (debug frame)
	movq	%rsp, %rbp
	imull	$1640531527, %edi, %eax # eax = key * 0x61C88647  (ht_hash inlined)
	shrl	$24, %eax               # eax >>= 24.  32 - HT_BITS = 32 - 8 = 24.
                                        #   A LOGICAL right shift brings the top 8
                                        #   bits (the best-mixed ones in Fibonacci
                                        #   hashing) down to bit 0. Those 8 bits ARE
                                        #   the bucket index, 0..255 — no mask needed
                                        #   because the shift already zeroed bits 8+.
	popq	%rbp                    # EPILOGUE
	retq
.Lfunc_end1:
	.size	ht_bucket_highbits, .Lfunc_end1-ht_bucket_highbits

# =============================================================================
# u32 ht_bucket_mask(u32 key)   —   return ht_hash(key) & HT_MASK;   (& 0xFF)
# The naive "just mask the LOW bits" scheme. Watch the optimizer be clever:
# =============================================================================
	.globl	ht_bucket_mask
	.p2align	4
	.type	ht_bucket_mask,@function
ht_bucket_mask:
	pushq	%rbp                    # PROLOGUE (debug frame)
	movq	%rsp, %rbp
	imull	$71, %edi, %eax         # eax = key * 71  (!!).  We wrote `key *
                                        #   0x61C88647`, so why 71? Because the
                                        #   result is immediately masked to its low
                                        #   8 bits, and the low 8 bits of a product
                                        #   depend ONLY on the low 8 bits of each
                                        #   factor. 0x61C88647 & 0xFF == 0x47 == 71.
                                        #   clang proved multiplying by the full
                                        #   constant is wasted work here and shrank
                                        #   the immediate. A neat lesson: masking the
                                        #   output lets the optimizer narrow the input.
	movzbl	%al, %eax               # eax = (zero-extend) al.  `movzbl %al,%eax`
                                        #   keeps the low byte and zeroes bits 8..31
                                        #   — this IS the `& 0xFF` mask (HT_MASK).
	popq	%rbp                    # EPILOGUE
	retq
.Lfunc_end2:
	.size	ht_bucket_mask, .Lfunc_end2-ht_bucket_mask

# =============================================================================
# u32 ht_bucket(u32 key)  — THE routine the module actually uses:
#     u32 h = key * GOLDEN_RATIO_32;   // scatter
#     h ^= h >> 16;                    // avalanche high half into low half
#     return h & HT_MASK;              // fold to [0, 256)
# Four arithmetic ops, no branch, no memory, no division. Note: here clang keeps
# the FULL constant (unlike ht_bucket_mask) because the xor-shift consumes the
# high bits before the mask, so those high bits genuinely matter.
# =============================================================================
	.globl	ht_bucket
	.p2align	4
	.type	ht_bucket,@function
ht_bucket:
	pushq	%rbp                    # PROLOGUE (debug frame)
	movq	%rsp, %rbp
	imull	$1640531527, %edi, %eax # eax = h = key * 0x61C88647   (scatter)
	movl	%eax, %ecx              # ecx = copy of h (we need h twice: as the
                                        #   left operand of xor AND, shifted, as the
                                        #   right operand). x86 shifts are
                                        #   destructive, so keep an untouched copy.
	shrl	$16, %ecx               # ecx = h >> 16.  Logical shift moves the
                                        #   well-mixed HIGH half down over the LOW
                                        #   half so the xor can fold entropy inward.
	xorl	%eax, %ecx              # ecx = h ^ (h >> 16).  The avalanche: now
                                        #   the low bits carry information from the
                                        #   high bits, so masking low bits is safe.
	movzbl	%cl, %eax               # eax = (h ^ (h>>16)) & 0xFF.  Zero-extend the
                                        #   low byte (cl) into the return reg eax —
                                        #   this is the HT_MASK fold to [0,255].
	popq	%rbp                    # EPILOGUE
	retq                            # return the bucket index in eax
.Lfunc_end3:
	.size	ht_bucket, .Lfunc_end3-ht_bucket

# =============================================================================
# int main(void)   —   sum ht_bucket(0..7), return (sum & 0xff).
# The loop and every helper call were CONSTANT-FOLDED away entirely: the inputs
# 0..7 are compile-time constants, so clang evaluated the whole thing and emitted
# the answer as a literal. This is the same collapse you see in the reference
# nolibc example — the optimizer running the program at build time.
# =============================================================================
	.globl	main
	.p2align	4
	.type	main,@function
main:
	pushq	%rbp                    # PROLOGUE (debug frame)
	movq	%rsp, %rbp
	movl	$164, %eax              # eax = 164.  = (ht_bucket(0)+...+ht_bucket(7))
                                        #   & 0xFF, computed AT COMPILE TIME. The loop,
                                        #   the adds, and all eight ht_bucket calls
                                        #   vanished — nothing is left to run.
	popq	%rbp                    # EPILOGUE
	retq                            # return 164 to the shell ($?)
.Lfunc_end4:
	.size	main, .Lfunc_end4-main

	.ident	"clang version 20.1.8"          # toolchain stamp
	.section	".note.GNU-stack","",@progbits  # non-executable stack (security default)
# =============================================================================
# WHAT TO TAKE AWAY
#   * key -> bucket is imul (scatter) + shift/xor (avalanche) + mask (fold):
#     branch-free, memory-free, division-free arithmetic. That is the hot path.
#   * The optimizer narrowed `*0x61C88647 & 0xFF` to `*71` because masking the
#     output means only the low bits of the input matter — read your asm and you
#     catch tricks like this.
#   * ht_bucket touches NO shared memory, so it needs NO barriers. The RCU
#     publish (rcu_assign_pointer) and consume (rcu_dereference) ordering in
#     rcu_hashtable.c guards the bucket/node LOADS that use this index, not the
#     arithmetic here. Hashing is math; RCU safety is about the loads that follow.
#   * Compare with demo.O0.s (every value spilled to the stack, the loop in main
#     actually emitted) and demo.O2.s (frame pointers gone, pure compute + ret).
# =============================================================================
