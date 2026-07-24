# =============================================================================
# demo.annotated.s — clang -O1 output for demo.c, explained instruction by
# instruction. These are the debugger's two purest routines: the int3 byte-
# splice that plants/lifts a software breakpoint, and the sorted-table binary
# search that maps a program counter to a DWARF source-line row.
# =============================================================================
#
# HOW TO READ THIS FILE
# ---------------------
# This is the EXACT assembly clang 20.1.8 emits for demo.c at -O1 (see demo.s for
# the untouched original), annotated. AT&T syntax throughout:
#
#     op   src, dst                      # movq %rsp,%rbp  =>  rbp = rsp
#     %reg      register   $imm immediate   sym(%rip) RIP-relative address
#     N(%reg)   memory at [reg+N]        (%b,%i,%s) memory at [b + i*s]
#
# A register's narrow names are the SAME physical register: rax/eax/ax/al. Writing
# a 32-bit name (eax) ZERO-EXTENDS into the 64-bit register, which is why the
# compiler prefers `movl` (5 bytes) over `movq` (7) when the top 32 bits are zero.
#
# THE SYSTEM V AMD64 ABI (the contract every function below obeys)
# ----------------------------------------------------------------
#   * Integer/pointer arguments, in order:  rdi, rsi, rdx, rcx, r8, r9   (then stack)
#   * Return value:                          rax  (eax for an int)
#   * Caller-saved (scratch; a callee may clobber): rax, rcx, rdx, rsi, rdi, r8-r11
#   * Callee-saved (a callee MUST preserve):        rbx, rbp, r12-r15, rsp
#   * The RED ZONE: 128 bytes below rsp a leaf may use without moving rsp. Every
#     function here is a leaf (calls nobody), so none touch rsp beyond the frame.
#   * Stack alignment: rsp % 16 == 0 at a `call`. Nothing here calls, so the only
#     stack traffic is the frame-pointer push kept for debuggability.
#
# WHY THE CODE LOOKS LIKE THIS
# ----------------------------
# The four byte-splice routines collapse to 2-4 arithmetic instructions each. The
# star is addr_to_line: clang lowers the whole binary search into register-only
# index math, and turns the final `return end ? -1 : lo-1;` into a BRANCHLESS
# `sbb`/`or` idiom — called out in detail at the bottom of that function.
# =============================================================================

	.file	"demo.c"
	.text

# =============================================================================
# patch_int3(u64 word /*rdi*/) -> u64 /*rax*/
#   return (word & ~0xFF) | 0xCC;      // splice the int3 opcode into the low byte
# The elegant compiler trick: after clearing the low byte, "| 0xCC" is identical
# to "+ 0xCC" (no carry is possible into a zeroed byte), so clang folds the OR
# into an LEA displacement (0xCC == 204). One `and`, one `lea`.
# =============================================================================
	.globl	patch_int3
	.p2align	4                       # 16-byte-align the entry (I-fetch friendly)
	.type	patch_int3,@function
patch_int3:
	pushq	%rbp                        # PROLOGUE: save caller's frame pointer
	movq	%rsp, %rbp                  #   establish our frame (debug only here)
	andq	$-256, %rdi                 # rdi = word & 0xFF..F00 = word & ~0xFF.
	                                    #   -256 == 0xFFFFFFFFFFFFFF00: clears the
	                                    #   low byte, leaving the upper 7 intact.
	leaq	204(%rdi), %rax             # rax = rdi + 204. 204 == 0xCC. Because the
	                                    #   low byte is now 0, "+0xCC" == "|0xCC":
	                                    #   this IS the int3 splice, done with LEA.
	popq	%rbp                        # EPILOGUE: restore caller's frame pointer
	retq                                # return the patched word in rax
.Lfunc_end0:
	.size	patch_int3, .Lfunc_end0-patch_int3

# =============================================================================
# saved_byte_of(u64 word /*rdi*/) -> u8 /*al*/
#   return (u8)(word & 0xFF);          // the original opcode we are about to hide
# Returning a u8 means only AL matters; masking is implicit in the narrow return.
# =============================================================================
	.globl	saved_byte_of
	.p2align	4
	.type	saved_byte_of,@function
saved_byte_of:
	pushq	%rbp                        # PROLOGUE
	movq	%rsp, %rbp
	movq	%rdi, %rax                  # rax = word. The caller reads only AL (the
	                                    #   low byte), so no explicit & 0xFF is
	                                    #   needed — the u8 return type discards the
	                                    #   upper bits (see the `kill: def $al` note
	                                    #   the compiler leaves in demo.s).
	popq	%rbp                        # EPILOGUE
	retq                                # return byte in al
.Lfunc_end1:
	.size	saved_byte_of, .Lfunc_end1-saved_byte_of

# =============================================================================
# unpatch_byte(u64 word /*rdi*/, u8 saved /*sil*/) -> u64 /*rax*/
#   return (word & ~0xFF) | (u64)saved;   // restore the real opcode byte
# Symmetric with patch_int3, but here the low byte is a runtime value (`saved`),
# so the compiler cannot fold it into a displacement — it uses a real OR.
# =============================================================================
	.globl	unpatch_byte
	.p2align	4
	.type	unpatch_byte,@function
unpatch_byte:
	pushq	%rbp                        # PROLOGUE
	movq	%rsp, %rbp
	andq	$-256, %rdi                 # rdi = word & ~0xFF  (clear low byte)
	movl	%esi, %eax                  # rax = saved, zero-extended from esi. `saved`
	                                    #   is a u8 the caller placed in esi/sil; the
	                                    #   32-bit move zeroes rax's upper 32 bits.
	orq	%rdi, %rax                      # rax = (word & ~0xFF) | saved  = restored word
	popq	%rbp                        # EPILOGUE
	retq                                # return the restored word
.Lfunc_end2:
	.size	unpatch_byte, .Lfunc_end2-unpatch_byte

# =============================================================================
# rewind_rip(u64 rip /*rdi*/) -> u64 /*rax*/
#   return rip - 1;    // after a 1-byte int3 fires, RIP is addr+1; step it back
# LEA does the subtract without touching flags — the canonical "rax = rdi - 1".
# =============================================================================
	.globl	rewind_rip
	.p2align	4
	.type	rewind_rip,@function
rewind_rip:
	pushq	%rbp                        # PROLOGUE
	movq	%rsp, %rbp
	leaq	-1(%rdi), %rax              # rax = rip - 1  (the breakpoint's true addr)
	popq	%rbp                        # EPILOGUE
	retq
.Lfunc_end3:
	.size	rewind_rip, .Lfunc_end3-rewind_rip

# =============================================================================
# addr_to_line(const line_row *rows /*rdi*/, int n /*esi*/, u64 pc /*rdx*/) -> int /*eax*/
#
#   int lo = 0, hi = n;
#   while (lo < hi) {
#       int mid = lo + (hi - lo)/2;
#       if (rows[mid].addr <= pc) lo = mid + 1;   // answer is in the upper half
#       else                      hi = mid;       // answer is in the lower half
#   }
#   if (lo == 0)          return -1;              // pc below the whole table
#   if (rows[lo-1].end)   return -1;              // pc fell in an end_sequence gap
#   return lo - 1;                                // the covering row
#
# sizeof(line_row) == 24 (u64 addr @0, u32 file @8, u32 line @12, int end @16, pad
# to a multiple of 8). So &rows[i] == rows + i*24, and clang forms i*24 as
# (i*3) with the ,8 index scale — watch for `leaq (%r,%r,2)` (×3) and `,8`.
#
# REGISTER MAP inside the loop:  eax = lo,  esi = hi (reuses the n argument),
#                                rdx = pc,  rdi = rows base.
# =============================================================================
	.globl	addr_to_line
	.p2align	4
	.type	addr_to_line,@function
addr_to_line:
	pushq	%rbp                        # PROLOGUE
	movq	%rsp, %rbp
	xorl	%eax, %eax                  # lo = 0  (xor is the 2-byte zeroing idiom)
	testl	%esi, %esi                  # test n
	jg	.LBB_loop                       # if n > 0 enter the search loop...
	jmp	.LBB_after                      # ...else skip it entirely (lo stays 0)

	.p2align	4
# ---- lo = mid + 1 branch (rejoined at loop top) -----------------------------
.LBB_lo_up:                             # rows[mid].addr <= pc : raise the floor
	addl	%ecx, %eax                  # eax = lo + (hi-lo)/2   (ecx holds (hi-lo)/2)
	incl	%eax                        # eax = mid + 1  ->  lo = mid + 1
	cmpl	%esi, %eax                  # compare new lo with hi
	jge	.LBB_after                      # if lo >= hi the search is done
	                                    #   (fall through into the loop header)
# ---- loop header: compute mid, load rows[mid].addr, compare to pc ----------
.LBB_loop:
	movl	%esi, %r8d                  # r8d = hi
	subl	%eax, %r8d                  # r8d = hi - lo   (>= 0 here)
	movl	%r8d, %ecx                  # ---- ecx = (hi-lo)/2, computed as a signed
	shrl	$31, %ecx                   #      divide: grab the sign bit,
	addl	%r8d, %ecx                  #      bias the dividend,
	sarl	%ecx                        #      arithmetic shift right by 1.
	                                    #   (hi-lo>=0, so this is just (hi-lo)/2.)
	leal	(%rcx,%rax), %r8d           # r8d = lo + (hi-lo)/2 = mid
	movslq	%r8d, %r9                   # r9 = (int64)mid  (sign-extend the index)
	leaq	(%r9,%r9,2), %r9            # r9 = mid*3   (so r9*8 below == mid*24 bytes)
	cmpq	%rdx, (%rdi,%r9,8)          # compare rows[mid].addr (at rdi + mid*24)
	                                    #   with pc: sets flags for addr <= pc test
	jbe	.LBB_lo_up                      # if rows[mid].addr <= pc (unsigned) -> lo up
# ---- hi = mid branch --------------------------------------------------------
	movl	%r8d, %esi                  # hi = mid
	cmpl	%esi, %eax                  # compare lo with new hi
	jl	.LBB_loop                       # if lo < hi keep searching
	                                    #   (else fall through: loop finished)

# ---- post-loop: lo == count of rows with addr <= pc; covering row is lo-1 ---
.LBB_after:
	testl	%eax, %eax                  # test lo
	je	.LBB_none                       # if lo == 0 -> pc is below row 0 -> return -1
# ---- return rows[lo-1].end ? -1 : lo-1   (done BRANCHLESSLY) ----------------
	decl	%eax                        # eax = lo - 1  (candidate return value)
	cltq                                # sign-extend eax -> rax for address math
	leaq	(%rax,%rax,2), %rcx         # rcx = (lo-1)*3  -> (lo-1)*24 bytes with ,8
	xorl	%edx, %edx                  # edx = 0
	cmpl	16(%rdi,%rcx,8), %edx       # compute 0 - rows[lo-1].end (.end is @ +16).
	                                    #   Sets CF iff rows[lo-1].end != 0.
	sbbl	%edx, %edx                  # edx = 0 - 0 - CF = -(CF):
	                                    #   end!=0 -> edx = 0xFFFFFFFF (-1)
	                                    #   end==0 -> edx = 0
	orl	%edx, %eax                      # eax = (lo-1) | edx:
	                                    #   end!=0 -> (lo-1)|-1 = -1   (gap: no line)
	                                    #   end==0 -> (lo-1)|0  = lo-1 (the answer)
	popq	%rbp                        # EPILOGUE
	retq                                # return the row index (or -1)
.LBB_none:
	movl	$-1, %eax                   # return -1  (pc below the whole table)
	popq	%rbp                        # EPILOGUE
	retq
.Lfunc_end4:
	.size	addr_to_line, .Lfunc_end4-addr_to_line

	.ident	"clang version 20.1.8"
	.section	".note.GNU-stack","",@progbits   # non-executable stack (security default)
	.addrsig                            # address-significance table (LTO metadata)
# =============================================================================
# WHAT TO TAKE AWAY
#   * The int3 splice is a masked read-modify-write: `(w & ~0xFF) | 0xCC`. Because
#     the low byte is zeroed first, clang turns the OR into an LEA +0xCC. (patch_int3)
#   * RIP rewind is a single `lea -1`: the whole reason a breakpoint is invisible.
#   * The binary search is the classic "upper_bound, then step back one", lowered
#     to register-only index math (mid*24 built as mid*3 with an ×8 scale).
#   * `return end ? -1 : lo-1;` became BRANCHLESS via `cmp`/`sbb`/`or`: the sbb
#     idiom smears the carry flag into a 0 or -1 mask. Reading the asm is how you
#     SEE the optimizer erase a branch. (addr_to_line)
#   * Compare with demo.O0.s (every variable spilled to the stack, every branch
#     real) and demo.O2.s (the same logic, laid out for the branch predictor).
# =============================================================================
