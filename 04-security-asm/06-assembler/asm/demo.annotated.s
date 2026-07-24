# =============================================================================
# demo.annotated.s — clang's -O1 output for demo.c, explained line by line.
# =============================================================================
#
# HOW TO READ THIS FILE
# ---------------------
# This is the exact assembly clang emits for asm/demo.c at -O1 (see demo.s for
# the untouched original), with a comment on essentially every instruction.
# AT&T syntax throughout:
#
#     op   src, dst              # movl $1, %eax   =>  eax = 1
#     %reg                       # a register        $imm  a literal
#     N(%reg)                    # memory at [reg + N]
#     (%base,%index,scale)       # memory at [base + index*scale]  (an LEA/SIB)
#
# Register widths are the SAME register: rdx(64) / edx(32) / dx(16) / dl(8) /
# dh(bits 8..15). Writing a 32-bit name (edx) zero-extends into the 64-bit rdx.
#
# THE SysV ABI CONTRACT for the two functions below (integer args, in order):
#     rdi, rsi, rdx, rcx, r8, r9      ; return value in rax (eax for 32-bit)
#
#   encode_mov_rr(u8 *out,  int src, int dst)   ->  out=rdi, src=esi, dst=edx
#   backpatch_rel32(u8 *code, u32 field, u32 target) -> code=rdi, field=esi,
#                                                        target=edx  (void)
#
# THE BIG PICTURE
# ---------------
# demo.c is pure integer bit-twiddling, so the compiler needs no libc and no
# memory beyond the caller's buffer. Watch two things especially:
#   * encode_mov_rr builds the REX byte with setge/shl/or, and folds the whole
#     ModR/M byte into a single `lea` + `or` — a neat trick explained below.
#   * main was CONSTANT-FOLDED to `return 0`: clang evaluated both encoders on
#     their literal arguments, checked every expected byte, and proved the
#     self-test passes — so nothing is left but `xorl %eax,%eax`.
# =============================================================================

	.file	"demo.c"
	.text
	.globl	encode_mov_rr                   # export encode_mov_rr for the linker
	.p2align	4                       # 16-byte-align the entry (fetch-friendly)
	.type	encode_mov_rr,@function
# -----------------------------------------------------------------------------
# encode_mov_rr(u8 *out /*rdi*/, int src /*esi*/, int dst /*edx*/) -> int /*eax*/
#   Emit the 3 bytes of `mov %src, %dst`: REX(0x48|R|B), opcode 0x89, ModR/M.
# -----------------------------------------------------------------------------
encode_mov_rr:
# %bb.0:                                        # the function's single basic block
	pushq	%rbp                            # PROLOGUE: save caller's frame ptr
	movq	%rsp, %rbp                      # establish our frame (kept by -O1 +
                                                #   -fno-omit-frame-pointer)
                                                # (the two "kill" notes below are
                                                #  clang telling itself esi/edx will
                                                #  be reused at 64-bit width)

# ---- build the REX prefix:  0x48 | (src>=8?0x04:0) | (dst>=8?0x01:0) ---------
	cmpl	$8, %esi                        # compare src with 8 (is it r8..r15?)
	setge	%al                             # al = (src >= 8) ? 1 : 0  -> REX.R bit
	shlb	$2, %al                         # al <<= 2  -> 0x04 if set, else 0x00
                                                #   (REX.R is bit 2 of the REX byte)
	cmpl	$8, %edx                        # compare dst with 8
	setge	%cl                             # cl = (dst >= 8) ? 1 : 0  -> REX.B bit
	orb	%al, %cl                        # cl = REX.R | REX.B (bits 2 and 0)
	orb	$72, %cl                        # cl |= 0x48  (72 = REX with W=1 set):
                                                #   cl is now the complete REX byte
	movb	%cl, (%rdi)                     # out[0] = REX

# ---- opcode byte -------------------------------------------------------------
	movb	$-119, 1(%rdi)                  # out[1] = 0x89  (-119 as a signed byte
                                                #   is 0x89 = MOV r/m64, r64)

# ---- build the ModR/M byte:  0xC0 | ((src&7)<<3) | (dst&7) -------------------
	andl	$7, %edx                        # dst &= 7  (low 3 bits -> rm field)
	leal	(%rdx,%rsi,8), %eax             # eax = (dst&7) + src*8. src*8 puts
                                                #   src's low 3 bits into positions
                                                #   3..5 (the reg field); src's bit 3
                                                #   lands in bit 6 but is about to be
                                                #   overwritten by the mod bits.
	orb	$-64, %al                       # al |= 0xC0: set mod=11 (register-
                                                #   direct). Bit 6 is forced to 1 here,
                                                #   which is exactly why the stray
                                                #   src-bit-3 above does not matter.
	movb	%al, 2(%rdi)                    # out[2] = ModR/M

# ---- return n = 3 ------------------------------------------------------------
	movl	$3, %eax                        # eax = 3 (bytes written)
	popq	%rbp                            # EPILOGUE: restore caller's frame ptr
	retq                                    # return; result (3) is in eax
.Lfunc_end0:
	.size	encode_mov_rr, .Lfunc_end0-encode_mov_rr

# -----------------------------------------------------------------------------
	.globl	backpatch_rel32
	.p2align	4
	.type	backpatch_rel32,@function
# backpatch_rel32(u8 *code /*rdi*/, u32 field /*esi*/, u32 target /*edx*/) -> void
#   Fill the 4-byte rel32 hole at code[field] with target-(field+4), LE.
# -----------------------------------------------------------------------------
backpatch_rel32:
# %bb.0:
	pushq	%rbp                            # PROLOGUE
	movq	%rsp, %rbp

# ---- rel = target - (field + 4) ---------------------------------------------
	subl	%esi, %edx                      # edx = target - field
	addl	$-4, %edx                       # edx = target - field - 4 = rel32
                                                #   (-4 because a rel32 is measured
                                                #    from the END of the branch)

# ---- store rel little-endian into code[field .. field+3] --------------------
	movl	%esi, %eax                      # eax = field (index of byte 0)
	movb	%dl,  (%rdi,%rax)               # code[field+0] = rel bits  0..7  (dl)
	leal	1(%rsi), %eax                   # eax = field + 1
	movb	%dh,  (%rdi,%rax)               # code[field+1] = rel bits  8..15 (dh)
	movl	%edx, %eax                      # copy rel...
	shrl	$16, %eax                       # ...and shift down 16
	leal	2(%rsi), %ecx                   # ecx = field + 2
	movb	%al,  (%rdi,%rcx)               # code[field+2] = rel bits 16..23
	shrl	$24, %edx                       # edx = rel >> 24
	addl	$3, %esi                        # esi = field + 3
	movb	%dl,  (%rdi,%rsi)               # code[field+3] = rel bits 24..31
	popq	%rbp                            # EPILOGUE
	retq
.Lfunc_end1:
	.size	backpatch_rel32, .Lfunc_end1-backpatch_rel32

# -----------------------------------------------------------------------------
	.globl	main
	.p2align	4
	.type	main,@function
# main() -> int : the self-check. Constant-folded to `return 0`, because clang
#   evaluated encode_mov_rr(...,6,7)=48 89 f7, (...,8,15)=4d 89 c7, and the
#   backpatch(1,10)=05 00 00 00 at COMPILE TIME and confirmed every byte.
# -----------------------------------------------------------------------------
main:
# %bb.0:
	pushq	%rbp                            # PROLOGUE
	movq	%rsp, %rbp
	xorl	%eax, %eax                      # eax = 0: the proof succeeded, so the
                                                #   whole test collapses to "return 0"
	popq	%rbp                            # EPILOGUE
	retq
.Lfunc_end2:
	.size	main, .Lfunc_end2-main

	.ident	"clang version 20.1.8"
	.section	".note.GNU-stack","",@progbits  # stack is NON-executable (a
                                                        #   security default: no code
                                                        #   may run from the stack)
	.addrsig                                # address-significance table (LTO hint)
# =============================================================================
# WHAT TO TAKE AWAY
#   * An instruction encoder is just bit assembly: setge/shl/or lay out the REX
#     flags; a single `lea (%rdx,%rsi,8)` + `or $0xC0` builds the ModR/M byte.
#   * A rel32 backpatch is target-(field+4) written low byte first — the exact
#     little-endian store you see as movb %dl / %dh / (rel>>16) / (rel>>24).
#   * The optimizer can execute your pure functions at compile time. `main`
#     proving itself correct and vanishing to `return 0` is that in action —
#     and it is also how the -O1/-O2 files verify demo.c is genuinely correct.
#   * Compare with demo.O0.s (every value spilled to the stack, nothing folded)
#     to see the same logic before the optimizer touches it.
# =============================================================================
