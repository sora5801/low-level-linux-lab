# =============================================================================
# demo.annotated.s — clang -O1 output for demo.c (dns_decode_name), explained.
# =============================================================================
#
# HOW TO READ THIS FILE
# ---------------------
# This is the exact assembly clang 20 emits for asm/demo.c at -O1 (see demo.s
# for the untouched original), with a comment on essentially every instruction.
# AT&T syntax throughout:  op  src, dst   (e.g. `movl $1,%eax` => eax = 1).
#   %reg  register   $imm immediate   N(%r) memory at [r+N]   (%b,%i) [b+i]
# Register widths are the same register: rax(64)/eax(32)/ax(16)/al(8). Writing a
# 32-bit name ZERO-EXTENDS into the 64-bit register, so `movl` is used freely to
# clear the top half cheaply.
#
# THE SysV AMD64 ABI (what this function obeys)
# ---------------------------------------------
#   integer/pointer args, in order:  rdi, rsi, rdx, rcx, r8, r9   (then stack)
#   return value:                    rax  (eax for an int)
#   callee-saved (we must preserve): rbx, rbp, r12, r13, r14, r15
#   caller-saved (free scratch):     rax, rcx, rdx, rsi, rdi, r8, r9, r10, r11
#   red zone:                        128 bytes below rsp usable by leaf funcs
#   stack alignment:                 rsp % 16 == 0 at a `call`
#
# For dns_decode_name(const u8 *msg, u32 msg_len, u32 start, char *out,
#                     u32 out_cap):
#   rdi = msg      esi = msg_len   edx = start(=pos)   rcx = out   r8d = out_cap
#
# THE BIG PICTURE — a compiler-flattened state machine
# ----------------------------------------------------
# The C source is one `for(;;)` loop whose body ends in `continue`, `break`
# (root label), or `return -1` (malformed). At -O1 clang did NOT keep those as
# jumps; it SYNTHESISED a small integer "what to do next" code in %r14d and
# dispatches on it:
#
#     r14d == 0 or 3  ->  CONTINUE the loop      (fall to loop-condition recheck)
#     r14d == 2       ->  BREAK: name complete   (write NUL, return out_len)
#     r14d == 1/other ->  ERROR: return -1
#
# Watching that transformation — structured control flow becoming a data value —
# is exactly why we keep the assembly open. Persistent registers across the loop:
#     edx = pos (walk cursor)     ebx = out_len     r11d = hops
#     r9d = max_hops (= msg_len+1, spilled during the byte-copy)   r10d = const 1
# Labels are clang's `.LBB0_n`; the comments give each a meaningful name.
# =============================================================================

	.file	"demo.c"
	.text
	.globl	dns_decode_name                 # export the symbol (global function)
	.p2align	4                       # 16-byte align the entry for the I-fetch
	.type	dns_decode_name,@function
dns_decode_name:                        # (rdi,esi,edx,rcx,r8d) per the ABI above

# ---- CHEAP EARLY-OUTS before building a stack frame -------------------------
# These two `return -1` cases are leaf-simple, so clang handles them with NO
# prologue (note: rbp is pushed only later, at the loop entry).
	movl	$-1, %eax               # eax = -1 : the default/error return value
	testl	%r8d, %r8d              # out_cap == 0 ?  (test sets ZF if it is)
	je	.LBB0_30                # if out_cap == 0 -> return -1  (bare `ret`)
# %bb.1:
	movb	$0, (%rcx)              # out[0] = '\0'  (the C `out[0]='\0';`)
	cmpl	%esi, %edx              # compare pos(edx) with msg_len(esi)
	jae	.LBB0_30                # first-iteration `if (pos>=msg_len) return -1`
                                        #   eax is still -1, so this returns -1.

# ---- PROLOGUE: we are committed to the loop; save callee-saved regs ---------
# %bb.2:
	pushq	%rbp                    # save caller's frame pointer
	movq	%rsp, %rbp              # establish our frame (rbp = frame base)
	pushq	%r15                    # \
	pushq	%r14                    #  | preserve the callee-saved registers we
	pushq	%r13                    #  | are about to use as loop state / scratch
	pushq	%r12                    #  |
	pushq	%rbx                    # /  (rbx will hold out_len)
	leal	1(%rsi), %r9d           # r9d = msg_len + 1 = max_hops (loop guard cap)
	xorl	%ebx, %ebx              # ebx = 0 = out_len
	movl	$1, %r10d               # r10d = 1 : a reusable constant for cmov below
	xorl	%r11d, %r11d            # r11d = 0 = hops (pointers followed so far)
	jmp	.LBB0_4                 # jump into the loop body (skip the dispatch)

# ============================================================================
# THE STATE DISPATCH (targets of the synthesised r14d code).
# Every path through the loop body lands here to decide continue / break / error.
# ============================================================================
.LBB0_21:                               # ERROR-with-cleanup entry: name too long
	movl	%r15d, %ebx             # ebx = r15d (write offset); dead on error but
                                        #   clang keeps out_len coherent. Falls
                                        #   through into the error code below.
	.p2align	4
.LBB0_11:                               # ERROR: set state=1 (=> return -1)
	movl	$1, %r14d               # r14d = 1  (error code)
	testl	%r14d, %r14d           # r14d != 0 ? (always true here)
	je	.LBB0_3                 # (never taken) would mean "continue"
.LBB0_26:                               # DISPATCH: is the code CONTINUE(3)?
	cmpl	$3, %r14d               # r14d == 3 ?
	jne	.LBB0_27                # if not 3, test for BREAK(2) next
.LBB0_3:                                # LOOP-CONDITION RECHECK (top of `for`)
	cmpl	%esi, %edx              # pos(edx) >= msg_len(esi) ?
	jae	.LBB0_29                # if out of range -> return (eax=-1)  [`if(pos>=len)`]
.LBB0_4:                                # === LOOP HEADER (Depth 1) ============
	movl	%edx, %r14d             # r14d = pos (32->cleared-top index)
	movzbl	(%rdi,%r14), %r14d      # r14d = msg[pos] = lenb  (zero-extended byte)
	movl	%r14d, %r15d            # r15d = lenb (copy for the mask test)
	andl	$192, %r15d             # r15d = lenb & 0xC0   (192 = 0xC0)
	je	.LBB0_8                 # if top two bits clear -> ordinary label/root

# ---- a pointer, or the reserved 0b01/0b10 length-byte forms -----------------
# %bb.5:
	cmpl	$192, %r15d             # (lenb & 0xC0) == 0xC0 ?  (a compression ptr)
	jne	.LBB0_11                # else 0x40/0x80 are reserved -> ERROR (state=1)
# %bb.6:                                # it IS a pointer: bounds-check 2 bytes
	leal	2(%rdx), %r15d          # r15d = pos + 2
	cmpl	%esi, %r15d             # (pos+2) > msg_len ?
	ja	.LBB0_11                # pointer's 2nd byte out of range -> ERROR
# %bb.12:                               # decode + follow the 14-bit pointer
	andl	$63, %r14d              # r14d = lenb & 0x3F  (high 6 bits of offset)
	shll	$8, %r14d               # r14d <<= 8          (=> target high byte)
	leal	1(%rdx), %r15d          # r15d = pos + 1
	movzbl	(%rdi,%r15), %r15d      # r15d = msg[pos+1]   (offset low 8 bits)
	orl	%r14d, %r15d            # r15d = target = (hi<<8)|lo
	incl	%r11d                   # ++hops   (r11d)
	xorl	%r14d, %r14d            # r14d = 0  (prepare the in-bounds flag)
	cmpl	%esi, %r15d             # target < msg_len ?
	setb	%r14b                   # r14b = (target <  msg_len) ? 1 : 0
	cmovael	%edx, %r15d            # if target >= msg_len, keep pos (target unused)
	cmpl	%r9d, %r11d             # hops vs max_hops (sets flags for the cmov's)
	leal	1(%r14,%r14), %r14d     # r14d = 2*inbounds + 1  => 3 if in-bounds else 1
	cmoval	%r10d, %r14d           # if hops > max_hops: r14d = 1  (loop! => ERROR)
	cmoval	%edx, %r15d            # if hops > max_hops: keep pos (ignore target)
	movl	%r15d, %edx             # pos = target (or unchanged on error)
	testl	%r14d, %r14d           # state != 0 ? (it is 1 or 3 here)
	jne	.LBB0_26                # dispatch: 3 -> continue, 1 -> error
	jmp	.LBB0_3                 # (unreached; symmetry with other paths)

# ---- ordinary label (or root) : (lenb & 0xC0) == 0 --------------------------
	.p2align	4
.LBB0_8:
	testl	%r14d, %r14d           # lenb == 0 ?
	je	.LBB0_13                # yes -> root label -> BREAK (state=2)
# %bb.9:
	cmpb	$63, %r14b              # lenb > 63 ?  (label length limit)
	ja	.LBB0_11                # over-long label -> ERROR (state=1)
# %bb.14:                               # bounds-check the label body
	leal	(%rdx,%r14), %r15d      # r15d = pos + lenb
	incl	%r15d                   # r15d = pos + 1 + lenb  (end of the label)
	cmpl	%esi, %r15d             # (pos+1+lenb) > msg_len ?
	ja	.LBB0_11                # label body runs past the buffer -> ERROR
# %bb.16:                               # write a '.' separator unless first label
	testl	%ebx, %ebx              # out_len == 0 ?
	je	.LBB0_19                # first label: skip the dot
# %bb.17:
	leal	1(%rbx), %r15d          # r15d = out_len + 1
	cmpl	%r8d, %r15d             # (out_len+1) >= out_cap ?
	jae	.LBB0_11                # no room even for the '.' -> ERROR
# %bb.18:
	movl	%ebx, %ebx              # zero-extend out_len into rbx (for the index)
	movb	$46, (%rcx,%rbx)        # out[out_len] = '.'   (46 = '.')
	jmp	.LBB0_20                # r15d = out_len+1 = write offset after the dot

.LBB0_13:                               # root label reached: name is complete
	movl	$2, %r14d               # r14d = 2  (BREAK/success code)
	testl	%r14d, %r14d           # != 0
	jne	.LBB0_26                # dispatch -> falls through 3?no ->27 ->2 yes
	jmp	.LBB0_3

.LBB0_19:                               # first label: no separator
	xorl	%r15d, %r15d            # r15d = 0 = write offset (label starts at out[0])

.LBB0_20:                               # common tail: enforce name/capacity caps
	leal	(%r15,%r14), %ebx       # ebx = write_off + lenb = out_len AFTER copy
	cmpl	%r8d, %ebx              # new_out_len >= out_cap ?
	setae	%r12b                   # r12b = capacity-overflow flag
	cmpl	$256, %ebx              # imm 0x100 : new_out_len >= 256  (i.e. > 255)
	setae	%r13b                   # r13b = name-length (>255) overflow flag
	orb	%r12b, %r13b            # either overflow?
	jne	.LBB0_21                # if so -> ERROR (via the cleanup entry)
# %bb.22:                               # copy the label bytes; set up trip count
	movl	%r9d, -44(%rbp)         # spill max_hops to the stack: r9 becomes the
                                        #   copy's scratch dst-index below (4-byte
                                        #   slot in our frame, per the red zone/
                                        #   local-storage discipline)
	cmpl	$1, %r14d               # sets CF = (lenb < 1); lenb>=1 here so CF=0
	movl	%r14d, %r12d           # r12d = lenb
	adcl	$0, %r12d               # r12d += CF (=0) => trip count = lenb
	leal	1(%rdx), %r13d          # r13d = pos + 1 = source index (first label byte)
	.p2align	4
.LBB0_23:                               # === INNER COPY LOOP (Depth 2) ========
	movl	%r13d, %r10d            # r10d = src index
	movzbl	(%rdi,%r10), %r10d      # r10d = msg[src]
	movl	%r15d, %r9d             # r9d = dst offset (out_len position)
	movb	%r10b, (%rcx,%r9)       # out[dst] = msg[src]   (copy one byte)
	incl	%r15d                   # dst++
	incl	%r13d                   # src++
	decq	%r12                    # trip--   (64-bit dec to also clear high bits)
	jne	.LBB0_23                # loop until all `lenb` bytes copied
# %bb.24:                               # advance the cursor; resume the outer loop
	addl	%r14d, %edx             # pos += lenb
	incl	%edx                    # pos += 1  => pos = pos + 1 + lenb
	xorl	%r14d, %r14d            # r14d = 0  (state = CONTINUE)
	movl	-44(%rbp), %r9d         # reload max_hops from the spill slot
	movl	$1, %r10d               # restore the constant 1 (clobbered by copy)
	testl	%r14d, %r14d           # state == 0 -> continue
	jne	.LBB0_26                # (not taken) dispatch if nonzero
	jmp	.LBB0_3                 # continue: recheck the loop condition

# ---- DISPATCH tail: distinguish BREAK(2) from ERROR(other) ------------------
.LBB0_27:
	cmpl	$2, %r14d               # state == 2 (BREAK / success) ?
	jne	.LBB0_29                # not 2 -> ERROR path: return eax (still -1)
# %bb.28:                               # success: terminate and return the length
	movl	%ebx, %eax              # eax = out_len
	movb	$0, (%rcx,%rax)         # out[out_len] = '\0'   (C `out[out_len]=0;`)
	movl	%ebx, %eax              # eax = out_len  (the return value)

# ---- EPILOGUE: restore callee-saved registers, return -----------------------
.LBB0_29:
	popq	%rbx                    # \
	popq	%r12                    #  | pop in REVERSE of the push order so each
	popq	%r13                    #  | callee-saved register gets its value back
	popq	%r14                    #  |
	popq	%r15                    # /
	popq	%rbp                    # restore caller's frame pointer
.LBB0_30:                               # the two early-outs (out_cap==0, pos>=len)
	retq                            # return: eax = out_len on success, else -1
.Lfunc_end0:
	.size	dns_decode_name, .Lfunc_end0-dns_decode_name

	.ident	"clang version 20.1.8"          # toolchain stamp (harmless metadata)
	.section	".note.GNU-stack","",@progbits  # mark the stack non-executable
	.addrsig                                        # address-significance table
# =============================================================================
# WHAT TO TAKE AWAY
#   * The compiler turned `continue`/`break`/`return` into a small integer state
#     in %r14d and a 3-way dispatch — structured control flow became DATA. When
#     you can't map C lines to jumps 1:1, look for a synthesised selector like
#     this; the -O0 file (demo.O0.s) still shows the naive, literal mapping.
#   * `cmov`/`setb`/`adc` replace branches: the pointer-bounds and loop-guard
#     decisions are computed branchlessly, then folded into the state code.
#   * The loop guard (`++hops > max_hops`) and every `pos`/label bounds check
#     survive optimisation intact — that is the security-relevant core, and you
#     can SEE it is still there in the machine code.
#   * Callee-saved rbx/r12-r15 are pushed once and popped in reverse; the frame
#     exists only because the loop needs more live values than the caller-saved
#     scratch set provides.
# =============================================================================
