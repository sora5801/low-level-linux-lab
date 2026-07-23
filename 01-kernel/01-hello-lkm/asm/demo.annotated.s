# =============================================================================
# demo.annotated.s — clang's -O1 output for demo.c, explained line by line.
# =============================================================================
#
# HOW TO READ THIS FILE
# ---------------------
# This is the EXACT assembly clang 20 emits for asm/demo.c at -O1 (see demo.s
# for the untouched original), with a comment on essentially every instruction.
# It was produced with:
#
#   clang --target=x86_64-pc-linux-gnu -S -O1 \
#         -fno-asynchronous-unwind-tables -fno-jump-tables \
#         -fno-omit-frame-pointer asm/demo.c -o asm/demo.s
#
# AT&T syntax throughout, so the operand order is  SOURCE, DESTINATION:
#
#     movl $10, %ecx        #  ecx = 10      (dest is on the RIGHT)
#     %reg                  #  a register
#     $imm                  #  an immediate (literal) constant
#     N(%reg)               #  memory at address [reg + N]
#
# Register widths are views of ONE physical register:
#     rax(64) / eax(32) / ax(16) / al(8),  and  rcx/ecx/cx/cl, etc.
# Writing a 32-bit name (eax) ZERO-EXTENDS into the full 64-bit register — that
# is why the compiler prefers `movl $10, %ecx` over a 64-bit move: it is shorter
# and the top 32 bits become zero for free.
#
# THE SysV AMD64 ABI CONTRACT (the rules every function here obeys)
# ----------------------------------------------------------------
#   * Integer / pointer ARGUMENTS, in order:  rdi, rsi, rdx, rcx, r8, r9
#         - clamp_count(int count):   count arrives in %edi (low 32 of rdi)
#         - fnv1a32(const char *s):   s arrives in %rdi (a 64-bit pointer)
#   * RETURN value: %rax (or %eax for a 32-bit int).
#   * CALLEE-SAVED (a function must preserve these for its caller):
#         rbx, rbp, r12, r13, r14, r15, and rsp.
#   * CALLER-SAVED (free scratch — may be clobbered): rax, rcx, rdx, rsi, rdi,
#         r8, r9, r10, r11.  Note clamp_count/fnv1a32 use only rax/rcx/rdi, all
#         caller-saved, so they need to preserve nothing except the frame ptr.
#   * STACK: 16-byte aligned at the point of a `call`; grows DOWN.
#
# WHY THERE IS A FRAME POINTER AT ALL. We compiled with -fno-omit-frame-pointer
# for teaching clarity, so each function that does real work pushes %rbp and
# copies %rsp into it. These are LEAF functions (they call nothing), so the
# frame is not strictly required — but it makes a debugger backtrace work and
# shows the classic prologue/epilogue shape.
#
# THE BIG PICTURE
# ---------------
# Two pure-logic helpers, and what the optimizer did with each:
#   * clamp_count  -> completely BRANCHLESS: two compares feeding two `cmov`
#                     conditional moves. No jumps, so no branch misprediction.
#   * fnv1a32      -> a tight xor/imul loop, with a clever twist: the empty-
#                     string case returns WITHOUT setting up a stack frame.
# =============================================================================

	.file	"demo.c"
	.text                                   # executable code section

# =============================================================================
# int clamp_count(int count)   ->   max(COUNT_MIN=1, min(count, COUNT_MAX=10))
# -----------------------------------------------------------------------------
# The module's range check. The C is two `if` statements, but at -O1 clang
# proved it can be done with ZERO branches using conditional moves. A `cmovCC
# src, dst` copies src into dst ONLY IF the condition flags say so — otherwise
# dst is left unchanged. It reads both possibilities and picks one on the flags,
# which is faster than a branch the CPU might mispredict.
# =============================================================================
	.globl	clamp_count                     # export the symbol
	.p2align	4                       # 16-byte align the entry (fetch friendly)
	.type	clamp_count,@function
clamp_count:                            # @clamp_count  (arg: count in %edi)
# %bb.0:                                # basic block 0 (the whole function)

# ---- PROLOGUE (frame pointer kept only because -fno-omit-frame-pointer) ----
	pushq	%rbp                            # save caller's frame pointer
	movq	%rsp, %rbp                      # rbp = base of our (empty) frame

# ---- STEP 1: ecx = min(count, 10) -------------------------------------------
	cmpl	$10, %edi                       # compute (count - 10), set flags only
	movl	$10, %ecx                       # ecx = 10  (the ceiling candidate)
	cmovll	%edi, %ecx                      # if count < 10 (SIGNED less), ecx = count.
                                                #   else ecx stays 10.  => ecx = min(count,10)
                                                #   'l' suffix = "less", the signed test,
                                                #   which is correct because count is a
                                                #   signed int and can be negative.

# ---- STEP 2: eax = max(1, ecx) ----------------------------------------------
# Source says "if (count < 1) return 1". clang rewrote it as "keep ecx only when
# ecx >= 2, else 1" — equivalent, because the sole differing input ecx==1 maps
# to the value 1 down BOTH paths (either the moved ecx=1 or the constant 1).
	cmpl	$2, %ecx                        # compute (ecx - 2), set flags only
	movl	$1, %eax                        # eax = 1  (the floor / return default)
	cmovgel	%ecx, %eax                      # if ecx >= 2 (SIGNED >=), eax = ecx.
                                                #   else eax stays 1.  => eax = max(1, ecx)
                                                #   Result now in eax, the ABI return reg.

# ---- EPILOGUE ---------------------------------------------------------------
	popq	%rbp                            # restore caller's frame pointer
	retq                                    # return; %eax holds the clamped value
.Lfunc_end0:
	.size	clamp_count, .Lfunc_end0-clamp_count   # symbol byte length (for tools)
                                        # -- End function

# =============================================================================
# unsigned int fnv1a32(const char *s)   ->   FNV-1a hash in %eax
# -----------------------------------------------------------------------------
# ABI: pointer s in %rdi; result u32 in %eax. Loop body per byte:
#         h = (h ^ (unsigned char)*s) * 16777619
# starting from h = 2166136261 (0x811C9DC5).
#
# NOTE ON THE CONSTANT: `movl $-2128831035` and 0x811C9DC5 are the SAME 32-bit
# value. 2166136261 > 2^31, so when printed as a SIGNED 32-bit int it shows as
# -2128831035. The bits loaded into eax are identical either way.
# =============================================================================
	.globl	fnv1a32
	.p2align	4
	.type	fnv1a32,@function
fnv1a32:                                # @fnv1a32  (arg: s in %rdi)
# %bb.0:                                # entry: peek at the first byte

# ---- FAST-PATH TEST: is the string empty? ----------------------------------
# clang peels the first iteration's load so it can skip the whole frame setup
# for the empty-string case. This is why the prologue appears LATER, not here.
	movzbl	(%rdi), %ecx                    # ecx = zero-extend *s  (byte -> 32 bits).
                                                #   movzbl clears the upper 24 bits, so a
                                                #   high byte (>= 0x80) does NOT sign-extend.
	testb	%cl, %cl                        # test the loaded byte (cl = low 8 of ecx)
	je	.LBB1_1                         # if *s == 0, take the no-frame fast path

# %bb.3:                                # the loop PROLOGUE (only reached if s != "")
	pushq	%rbp                            # save caller's frame pointer...
	movq	%rsp, %rbp                      #   ...and establish our frame. This runs
                                                #   ONLY on the non-empty path — the empty
                                                #   path below never pushes, so each return
                                                #   is still stack-balanced.
	incq	%rdi                            # s++  : advance past the byte we pre-loaded
	movl	$-2128831035, %eax              # h = 0x811C9DC5  (FNV-1a offset basis)
	.p2align	4                       # align the loop top for the fetch unit

# ---- THE HASH LOOP ----------------------------------------------------------
.LBB1_4:                                # do { ... } while (*s)   [current byte in ecx]
	movzbl	%cl, %ecx                       # re-narrow to (unsigned char): the source
                                                #   cast. Redundant with the movzbl load
                                                #   that produced ecx, but harmless.
	xorl	%eax, %ecx                      # ecx = byte ^ h     (h currently in eax)
	imull	$16777619, %ecx, %eax           # eax = ecx * 16777619  => new h.
                                                #   3-operand imul: dst, src1, imm. clang did
                                                #   NOT strength-reduce this to shifts/adds —
                                                #   one `imul` by a constant is ~3 cycles and
                                                #   beats a multi-instruction LEA chain.
	movzbl	(%rdi), %ecx                    # load the NEXT byte *s into ecx
	incq	%rdi                            # s++
	testb	%cl, %cl                        # is that next byte the NUL terminator?
	jne	.LBB1_4                         # if not zero, iterate again

# %bb.5:                                # loop done: h (final) is in eax
	popq	%rbp                            # restore caller's frame pointer
	retq                                    # return hash in %eax

# ---- FAST PATH: empty string ------------------------------------------------
.LBB1_1:                                # reached when the very first byte was 0
	movl	$-2128831035, %eax              # h = basis (loop body never executed)
	retq                                    # return WITHOUT a pop: we never pushed
.Lfunc_end1:
	.size	fnv1a32, .Lfunc_end1-fnv1a32
                                        # -- End function

# =============================================================================
# int main(void)
# -----------------------------------------------------------------------------
# The demo driver. Because clamp_count(5) and fnv1a32("world") are both
# compile-time constant, the optimizer evaluated the ENTIRE function at compile
# time: clamp_count(5)=5, fnv1a32("world")=0x37A3E893, then (h ^ 5) & 0x7f = 22.
# The whole thing collapses to "return 22" — a vivid demonstration of constant
# folding across function boundaries.
# =============================================================================
	.globl	main
	.p2align	4
	.type	main,@function
main:                                   # @main
# %bb.0:
	pushq	%rbp                            # prologue (kept for frame-pointer builds)
	movq	%rsp, %rbp
	movl	$22, %eax                       # eax = 22  <-- the fully precomputed result
	popq	%rbp                            # epilogue
	retq                                    # exit status 22
.Lfunc_end2:
	.size	main, .Lfunc_end2-main
                                        # -- End function

	.ident	"clang version 20.1.8"          # toolchain stamp (metadata)
	.section	".note.GNU-stack","",@progbits  # we do NOT need an executable
                                                        #   stack — a security default the
                                                        #   linker records.
	.addrsig                                # address-significance table (LTO hint)
# =============================================================================
# WHAT TO TAKE AWAY
#   * clamp_count is BRANCHLESS: `cmp` sets flags, `cmov` picks a value without
#     jumping. The module's range check costs a handful of cycles, no branch
#     prediction involved.
#   * The signed suffixes matter: `cmovl`/`cmovge` are SIGNED tests, correct for
#     a signed int that could be negative. At -O2 clang gets cheekier and uses an
#     UNSIGNED compare (`cmovb`) to fold the "< 1" and "> 10" checks — compare
#     demo.O2.s to see it.
#   * fnv1a32 shows a real optimization decision: peel the first byte so the
#     empty-string path returns with NO stack frame at all.
#   * main proves the optimizer folds pure functions across call boundaries: a
#     hash of a literal string became the single immediate `22`.
#   * The in-kernel machine code for hello_lkm.c's clamp_count/fnv1a32 has this
#     exact shape — the module just cannot be compiled to .s standalone, which is
#     the whole reason demo.c exists.
# =============================================================================
