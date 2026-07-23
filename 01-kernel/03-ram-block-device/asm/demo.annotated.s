# =============================================================================
# demo.annotated.s — clang's -O1 output for demo.c, explained instruction by
# instruction. The un-annotated original is demo.s; -O0 and -O2 sit beside it.
# =============================================================================
#
# HOW TO READ THIS FILE (AT&T syntax, the clang/gas default)
# ----------------------------------------------------------
#     op   source, destination         # movq %rdi,%rax  =>  rax = rdi
#     %reg          a register          $imm  an immediate (literal)
#     N(%reg)       memory at [reg+N]   (%reg) memory at [reg]
#
# Register widths are windows on the SAME register:
#     rax(64) / eax(32) / ax(16) / al(8).  Writing eax ZERO-EXTENDS into rax,
#     so `movl %esi,%eax` both moves the 32-bit arg AND clears rax's top half —
#     the idiomatic way to widen a u32 to a u64.
#
# THE SysV AMD64 ABI CONTRACT (what every function here obeys)
# -----------------------------------------------------------
#   Integer/pointer ARGUMENTS, in order:  rdi, rsi, rdx, rcx, r8, r9
#       (a `u32` arg arrives in the 32-bit view, e.g. esi, with top bits unset)
#   RETURN value:                          rax  (eax for 32-bit `int`)
#   CALLEE-SAVED (must preserve):          rbx, rbp, rsp, r12, r13, r14, r15
#   CALLER-SAVED (scratch, free to clobber): rax, rcx, rdx, rsi, rdi, r8-r11
#   The FLAGS register carries CF (carry/borrow), ZF (zero), etc. between a
#   subtract/compare and the conditional that reads it — the crux of this file.
#
# WHAT THESE THREE FUNCTIONS ARE
# ------------------------------
#   sector_to_bytes  off = sector << 9          the block<->byte conversion
#   ramblk_range_ok  the overflow-safe bounds check from ramblk_transfer()
#   ramblk_advance   pos += nbytes              the per-segment cursor step
# The middle one is the point: watch the compiler fold two source-level checks
# into one subtract-and-borrow, and see how it stays correct on overflow.
# =============================================================================

	.file	"demo.c"
	.text                                   # executable code section

# =============================================================================
# u64 sector_to_bytes(u64 sector)   —   return sector << 9
#   arg:  rdi = sector      ret: rax = byte offset
# =============================================================================
	.globl	sector_to_bytes
	.p2align	4                       # 16-byte align the entry (fetch-friendly)
	.type	sector_to_bytes,@function
sector_to_bytes:
	pushq	%rbp                            # PROLOGUE: save caller's frame pointer
	movq	%rsp, %rbp                      #   rbp = rsp: establish this frame
	movq	%rdi, %rax                      # rax = sector (move arg into the return reg)
	shlq	$9, %rax                        # rax <<= 9  ==  sector * 512. A shift, not
	                                        #   a multiply: 512 is 2^9, so one `shl`
	                                        #   does it in a single cycle. This is the
	                                        #   whole "sector -> byte offset" step.
	popq	%rbp                            # EPILOGUE: restore caller's rbp
	retq                                    # return; result already in rax
.Lfunc_end0:
	.size	sector_to_bytes, .Lfunc_end0-sector_to_bytes

# =============================================================================
# int ramblk_range_ok(u64 sector, u32 nbytes, u64 store_bytes, u64 *out_off)
#   args: rdi = sector   esi = nbytes   rdx = store_bytes   rcx = out_off
#   ret:  eax = 1 if [off, off+nbytes) fits in store_bytes (and *out_off set),
#         else 0.
#
# The C did two guarded checks:
#     off = sector << 9;
#     if (off > store_bytes)              return 0;   // (1)
#     if (nbytes > store_bytes - off)     return 0;   // (2)
#     *out_off = off; return 1;
# clang computes `store_bytes - off` ONCE and reuses its borrow flag for check
# (1) and its value for check (2), then ORs the two failure bits into a single
# branch. Follow the FLAGS carefully — that is where the cleverness lives.
# =============================================================================
	.globl	ramblk_range_ok
	.p2align	4
	.type	ramblk_range_ok,@function
ramblk_range_ok:
	pushq	%rbp                            # PROLOGUE (kept because -O1 leaves the
	movq	%rsp, %rbp                      #   frame pointer in for debuggability)

	shlq	$9, %rdi                        # rdi = sector << 9 = off. From here on
	                                        #   rdi holds the byte offset; note the
	                                        #   arg register is reused as a temporary.

	subq	%rdi, %rdx                      # rdx = store_bytes - off.
	                                        #   This subtraction sets CF (the borrow
	                                        #   flag) IFF store_bytes < off, i.e. IFF
	                                        #   off > store_bytes -- which is exactly
	                                        #   check (1). One instruction yields both
	                                        #   the value we need for (2) AND the flag
	                                        #   for (1).
	setb	%r8b                            # r8b = CF = (off > store_bytes)
	                                        #   = "check (1) FAILED". Stash it; we will
	                                        #   OR it in at the end. (If (1) failed,
	                                        #   rdx just wrapped to a huge value, but
	                                        #   that poisoned rdx can only make (2)
	                                        #   *pass*, never wrongly reject -- and the
	                                        #   OR below forces rejection anyway, so
	                                        #   correctness holds. This is why the
	                                        #   source ORDER of the two ifs is safe to
	                                        #   collapse.)

	movl	%esi, %eax                      # eax = nbytes, zero-extended into rax
	                                        #   (u32 -> u64). Needed so the 64-bit
	                                        #   compare below is apples-to-apples.
	cmpq	%rax, %rdx                      # compute (store_bytes-off) - nbytes,
	                                        #   discarding the result, keeping flags.
	                                        #   CF set IFF (store_bytes-off) < nbytes,
	                                        #   i.e. nbytes > store_bytes-off = check(2).
	setb	%dl                             # dl = CF = "check (2) FAILED".

	xorl	%eax, %eax                      # eax = 0: preload the "reject" return
	                                        #   value. xor-self is the 2-byte, zero-
	                                        #   latency idiom for clearing a register.

	orb	%r8b, %dl                       # dl = fail(1) OR fail(2). Nonzero IFF the
	                                        #   range is bad. Sets ZF for the branch.
	jne	.LBB1_2                         # if (dl != 0) -> reject: skip the store,
	                                        #   return with eax already 0.
                                                #   (.LBB1_2 == "return" label below.)

# ---- both checks passed: commit the result ---------------------------------
	movq	%rdi, (%rcx)                    # *out_off = off  (rcx is the out_off ptr).
	                                        #   Only reached when the range is valid,
	                                        #   so the caller never sees a stale offset.
	movl	$1, %eax                        # return 1 ("OK").

.LBB1_2:                                        # <- "return" (reject falls straight here)
	popq	%rbp                            # EPILOGUE
	retq                                    # eax holds 0 (reject) or 1 (ok)
.Lfunc_end1:
	.size	ramblk_range_ok, .Lfunc_end1-ramblk_range_ok

# =============================================================================
# u64 ramblk_advance(u64 pos, u32 nbytes)   —   return pos + nbytes
#   args: rdi = pos   esi = nbytes      ret: rax = pos + nbytes
# =============================================================================
	.globl	ramblk_advance
	.p2align	4
	.type	ramblk_advance,@function
ramblk_advance:
	pushq	%rbp                            # PROLOGUE
	movq	%rsp, %rbp
	movl	%esi, %eax                      # eax = nbytes, zero-extended to rax (u32->u64)
	addq	%rdi, %rax                      # rax = nbytes + pos. Commutative, so the
	                                        #   compiler adds pos into the already-widened
	                                        #   nbytes rather than the other way round.
	popq	%rbp                            # EPILOGUE
	retq
.Lfunc_end2:
	.size	ramblk_advance, .Lfunc_end2-ramblk_advance

	.ident	"clang version 20.1.8"          # toolchain stamp (harmless metadata)
	.section	".note.GNU-stack","",@progbits  # mark the stack non-executable (security
	                                                #   default the linker records)
# =============================================================================
# WHAT TO TAKE AWAY
#   * sector<->byte is a shift by 9, never a multiply — 512 == 2^9.
#   * The overflow-safe check (compare AFTER subtracting, read the borrow flag)
#     compiles to fewer branches than the naive `off+nbytes > size`, AND is the
#     version that cannot be tricked by a wrapping offset. Faster and safer.
#   * clang reused one `sub` for two source checks and merged their failure bits
#     with `or` into a single `jne`. Compare with demo.O0.s, where each `if` is
#     its own compare-and-branch with every value spilled to the stack — the
#     same logic, written the slow, literal way.
#   * demo.O2.s is identical to this minus the `push/pop %rbp` frame (that frame
#     only survives here because -O1 was built with -fno-omit-frame-pointer).
# =============================================================================
