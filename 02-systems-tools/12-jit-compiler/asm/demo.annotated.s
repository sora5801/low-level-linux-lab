# =============================================================================
# demo.annotated.s — the JIT's instruction ENCODER, compiler output explained.
# =============================================================================
#
# HOW TO READ THIS FILE
# ---------------------
# This is clang's -O1 output for asm/demo.c (see asm/demo.s for the untouched
# original), with a comment on essentially every instruction. AT&T syntax:
#
#     op   source, destination          # movb %sil, (%rcx)  =>  [rcx] = sil
#     %reg                              # a register
#     $imm                              # an immediate (literal); shown SIGNED,
#                                       #   so byte 0x81 prints as $-127
#     N(%reg)                           # memory at [reg + N]
#     (%base,%index)                    # memory at [base + index]  (scale 1)
#
# Register widths are windows onto the same register:
#     rax(64) / eax(32) / ax(16) / al(8, low) / ah(8, bits 15..8).
# Writing a 32-bit reg (eax) ZERO-EXTENDS into the 64-bit reg.
#
# SYSTEM V AMD64 ABI (what every function here obeys)
# ---------------------------------------------------
#   * integer/pointer ARGS, in order:  rdi, rsi, rdx, rcx, r8, r9  (then stack)
#   * RETURN value:                     rax  (al for a byte-returning function)
#   * CALLER-saved (scratch):           rax, rcx, rdx, rsi, rdi, r8-r11
#   * CALLEE-saved (must preserve):     rbx, rbp, r12-r15
#   * the RED ZONE:                     128 bytes below rsp a leaf may use freely
#   * STACK ALIGNMENT:                  rsp % 16 == 0 at the point of a `call`
#
# Every function below is a LEAF (calls nothing — emit_u8 was inlined into its
# callers), so none needs to align the stack for an outgoing call, and none
# touches a callee-saved register except rbp (kept only for a tidy frame at -O1).
#
# THE DATA STRUCTURE these functions manipulate — code_buf — has this layout
# (offsets are what the loads/stores below use):
#       offset 0  :  u8  *buf        (movq (%rdi), ...)      the output buffer
#       offset 8  :  u64  len        (movq 8(%rdi), ...)     next write index
#       offset 16 :  u64  cap        (cmpq 16(%rdi), ...)    capacity
#       offset 24 :  int  overflow   (movl $1, 24(%rdi))     sticky error flag
# =============================================================================

	.file	"demo.c"
	.text

# =============================================================================
# emit_u8(code_buf *c /*rdi*/, u8 b /*sil*/) — append one byte, bounds-checked.
#
#   if (c->len >= c->cap) { c->overflow = 1; return; }
#   c->buf[c->len++] = b;
#
# Teaching highlight: clang split the two paths so the common (in-bounds) path
# builds a stack frame while the rare overflow path returns WITHOUT one. The
# bounds test even runs BEFORE the `push %rbp`, so the failure path is a couple
# of instructions with no prologue at all.
# =============================================================================
	.globl	emit_u8
	.p2align	4
	.type	emit_u8,@function
emit_u8:
	movq	8(%rdi), %rax           # rax = c->len
	cmpq	16(%rdi), %rax          # compare len - cap  (sets flags)
	jae	.Loverflow0             # if len >= cap (unsigned) -> overflow path
# ---- in-bounds path (build a frame, do the store) ---------------------------
	pushq	%rbp                    # PROLOGUE (only on this hot path)
	movq	%rsp, %rbp              # frame base
	movq	(%rdi), %rcx            # rcx = c->buf   (base pointer)
	leaq	1(%rax), %rdx           # rdx = len + 1  (LEA = add without flags)
	movq	%rdx, 8(%rdi)           # c->len = len + 1   (the ++ )
	movb	%sil, (%rcx,%rax)       # buf[len] = b   (sil = low byte of arg `b`)
	popq	%rbp                    # EPILOGUE
	retq
.Loverflow0:                            # c->len >= c->cap
	movl	$1, 24(%rdi)            # c->overflow = 1   (note: no frame built)
	retq                            # return, dropping the write
.Lfunc_end0:
	.size	emit_u8, .Lfunc_end0-emit_u8

# =============================================================================
# emit_u32(code_buf *c /*rdi*/, u32 v /*esi*/) — append v little-endian.
#
# The four emit_u8(c, byte) calls were INLINED, so you see the bounds-check /
# store pattern from emit_u8 repeated four times, once per byte of v, each
# extracting a different 8 bits of v:
#     byte 0 = v & 0xff        -> %al
#     byte 1 = (v>>8) & 0xff   -> %ah   (the high-8 sub-register — no shift!)
#     byte 2 = (v>>16)& 0xff   -> shr $16 then %dl
#     byte 3 = (v>>24)& 0xff   -> shr $24 then %al
# This is exactly why we always funnel multi-byte writes through emit_u8: the
# bounds check is preserved per byte even after inlining.
# =============================================================================
	.globl	emit_u32
	.p2align	4
	.type	emit_u32,@function
emit_u32:
	pushq	%rbp                    # PROLOGUE
	movq	%rsp, %rbp
	movl	%esi, %eax              # eax = v  (keep a working copy; esi is arg2)
# ---- emit_u8(c, v & 0xff) : byte 0 ------------------------------------------
	movq	8(%rdi), %rcx           # rcx = c->len
	cmpq	16(%rdi), %rcx          # len vs cap
	jae	.Lof32_0                # overflow -> skip store, set flag
	movq	(%rdi), %rdx            # rdx = c->buf
	leaq	1(%rcx), %rsi           # rsi = len + 1
	movq	%rsi, 8(%rdi)           # c->len++
	movb	%al, (%rdx,%rcx)        # buf[len] = v & 0xff   (%al = low 8 bits)
	jmp	.Lb32_1
.Lof32_0:
	movl	$1, 24(%rdi)            # c->overflow = 1
.Lb32_1:                                # ---- emit_u8(c, (v>>8)&0xff) : byte 1 --
	movq	8(%rdi), %rcx           # reload len (it changed above)
	cmpq	16(%rdi), %rcx
	jae	.Lof32_1
	movq	(%rdi), %rdx            # rdx = c->buf
	leaq	1(%rcx), %rsi
	movq	%rsi, 8(%rdi)           # c->len++
	movb	%ah, (%rdx,%rcx)        # buf[len] = bits 15..8 of v  (via %ah — the
	                                #   CPU exposes them as a byte register, so no
	                                #   shift instruction is needed for byte 1)
	jmp	.Lb32_2
.Lof32_1:
	movl	$1, 24(%rdi)
.Lb32_2:                                # ---- emit_u8(c, (v>>16)&0xff) : byte 2 -
	movq	8(%rdi), %rcx
	cmpq	16(%rdi), %rcx
	jae	.Lof32_2
	movl	%eax, %edx              # edx = v
	shrl	$16, %edx               # edx = v >> 16  (byte 2 now in dl)
	movq	(%rdi), %rsi            # rsi = c->buf
	leaq	1(%rcx), %r8            # r8 = len + 1
	movq	%r8, 8(%rdi)            # c->len++
	movb	%dl, (%rsi,%rcx)        # buf[len] = (v>>16)&0xff
	jmp	.Lb32_3
.Lof32_2:
	movl	$1, 24(%rdi)
.Lb32_3:                                # ---- emit_u8(c, (v>>24)&0xff) : byte 3 -
	movq	8(%rdi), %rcx
	cmpq	16(%rdi), %rcx
	jae	.Lof32_3
	shrl	$24, %eax               # eax = v >> 24  (top byte now in al)
	movq	(%rdi), %rdx            # rdx = c->buf
	leaq	1(%rcx), %rsi
	movq	%rsi, 8(%rdi)           # c->len++
	movb	%al, (%rdx,%rcx)        # buf[len] = v >> 24
	popq	%rbp                    # EPILOGUE (fall-through from the last byte)
	retq
.Lof32_3:
	movl	$1, 24(%rdi)
	popq	%rbp
	retq
.Lfunc_end1:
	.size	emit_u32, .Lfunc_end1-emit_u32

# =============================================================================
# modrm(u8 mod /*dil*/, u8 reg /*sil*/, u8 rm /*dl*/) -> u8 in al
#
#   return ((mod&3)<<6) | ((reg&7)<<3) | (rm&7);
#
# Pure bit-twiddling — no memory touched. Watch the compiler pack three narrow
# fields into one byte. (It skips masking `mod` to 2 bits because the <<6 shifts
# any higher bits out of the byte anyway.)
# =============================================================================
	.globl	modrm
	.p2align	4
	.type	modrm,@function
modrm:
	pushq	%rbp                    # PROLOGUE
	movq	%rsp, %rbp
	                                # (clang note: esi is promoted to rsi for LEA)
	shlb	$6, %dil                # dil = mod << 6   -> the mod field (bits 7-6)
	leal	(,%rsi,8), %eax         # eax = reg * 8    == reg << 3  (bits 5-3),
	                                #   computed with LEA's scale-by-8 for free
	andb	$56, %al                # al &= 0b0011_1000  keep only the 3 reg bits
	orb	%dil, %al               # al |= mod field
	andb	$7, %dl                 # dl = rm & 7      -> the rm field (bits 2-0)
	orb	%dl, %al                # al |= rm field   -> finished ModRM byte
	                                # (result already in al = return register)
	popq	%rbp                    # EPILOGUE
	retq
.Lfunc_end2:
	.size	modrm, .Lfunc_end2-modrm

# =============================================================================
# emit_add_ptr(code_buf *c /*rdi*/, i32 delta /*esi*/)
#
#   emit `add $imm32, %rbx` = 48 81 C3 <imm32-LE>.
#
# modrm(3,0,3) was CONSTANT-FOLDED: the three fixed prefix/opcode/ModRM bytes
# appear as immediate stores 0x48, 0x81, 0xC3 (shown signed as 72, -127, -61),
# and only `delta` is emitted at run time via the inlined emit_u32. So the whole
# call to modrm() vanished — you are seeing the encoder specialize itself.
# =============================================================================
	.globl	emit_add_ptr
	.p2align	4
	.type	emit_add_ptr,@function
emit_add_ptr:
	pushq	%rbp                    # PROLOGUE
	movq	%rsp, %rbp
	movl	%esi, %eax              # eax = delta (saved; we clobber esi as scratch)
# ---- emit_u8(c, 0x48)  REX.W prefix -----------------------------------------
	movq	8(%rdi), %rcx           # rcx = len
	cmpq	16(%rdi), %rcx          # bounds check
	jae	.Lof_p0
	movq	(%rdi), %rdx            # rdx = buf
	leaq	1(%rcx), %rsi
	movq	%rsi, 8(%rdi)           # len++
	movb	$72, (%rdx,%rcx)        # buf[len] = 0x48  (REX.W = 64-bit operand)
	jmp	.Lp1
.Lof_p0:
	movl	$1, 24(%rdi)
.Lp1:                                   # ---- emit_u8(c, 0x81)  opcode add r/m64,imm32
	movq	8(%rdi), %rcx
	cmpq	16(%rdi), %rcx
	jae	.Lof_p1
	movq	(%rdi), %rdx
	leaq	1(%rcx), %rsi
	movq	%rsi, 8(%rdi)           # len++
	movb	$-127, (%rdx,%rcx)      # buf[len] = 0x81  (81 = -127 as signed byte)
	jmp	.Lp2
.Lof_p1:
	movl	$1, 24(%rdi)
.Lp2:                                   # ---- emit_u8(c, 0xC3)  ModRM mod11 /0 rbx
	movq	8(%rdi), %rcx
	cmpq	16(%rdi), %rcx
	jae	.Lof_p2
	movq	(%rdi), %rdx
	leaq	1(%rcx), %rsi
	movq	%rsi, 8(%rdi)           # len++
	movb	$-61, (%rdx,%rcx)       # buf[len] = 0xC3  (C3 = -61 as signed byte)
	jmp	.Lp3
.Lof_p2:
	movl	$1, 24(%rdi)
.Lp3:                                   # ---- emit_u32(c, delta) byte 0 = delta&0xff
	movq	8(%rdi), %rcx
	cmpq	16(%rdi), %rcx
	jae	.Lof_p3
	movq	(%rdi), %rdx
	leaq	1(%rcx), %rsi
	movq	%rsi, 8(%rdi)           # len++
	movb	%al, (%rdx,%rcx)        # buf[len] = delta & 0xff  (al)
	jmp	.Lp4
.Lof_p3:
	movl	$1, 24(%rdi)
.Lp4:                                   # ---- emit_u32 byte 1 = (delta>>8)&0xff
	movq	8(%rdi), %rcx
	cmpq	16(%rdi), %rcx
	jae	.Lof_p4
	movq	(%rdi), %rdx
	leaq	1(%rcx), %rsi
	movq	%rsi, 8(%rdi)           # len++
	movb	%ah, (%rdx,%rcx)        # buf[len] = bits 15..8 of delta (via %ah)
	jmp	.Lp5
.Lof_p4:
	movl	$1, 24(%rdi)
.Lp5:                                   # ---- emit_u32 byte 2 = (delta>>16)&0xff
	movq	8(%rdi), %rcx
	cmpq	16(%rdi), %rcx
	jae	.Lof_p5
	movl	%eax, %edx              # edx = delta
	shrl	$16, %edx               # edx = delta >> 16
	movq	(%rdi), %rsi            # rsi = buf
	leaq	1(%rcx), %r8
	movq	%r8, 8(%rdi)           # len++
	movb	%dl, (%rsi,%rcx)        # buf[len] = (delta>>16)&0xff
	jmp	.Lp6
.Lof_p5:
	movl	$1, 24(%rdi)
.Lp6:                                   # ---- emit_u32 byte 3 = (delta>>24)&0xff
	movq	8(%rdi), %rcx
	cmpq	16(%rdi), %rcx
	jae	.Lof_p6
	shrl	$24, %eax               # eax = delta >> 24  (top byte in al)
	movq	(%rdi), %rdx
	leaq	1(%rcx), %rsi
	movq	%rsi, 8(%rdi)           # len++
	movb	%al, (%rdx,%rcx)        # buf[len] = delta >> 24
	popq	%rbp                    # EPILOGUE
	retq
.Lof_p6:
	movl	$1, 24(%rdi)
	popq	%rbp
	retq
.Lfunc_end3:
	.size	emit_add_ptr, .Lfunc_end3-emit_add_ptr

# =============================================================================
# emit_add_cell(code_buf *c /*rdi*/, u8 delta /*sil*/)
#
#   emit `add $imm8, (%rbx)` = 80 03 <imm8>.
#
# Three inlined emit_u8s: the fixed opcode 0x80 (shown as -128), the fixed ModRM
# 0x03, and the run-time `delta` (sil). Only three bytes — the cheapest op the
# JIT emits, matching Brainfuck's most common instruction, '+'/'-'.
# =============================================================================
	.globl	emit_add_cell
	.p2align	4
	.type	emit_add_cell,@function
emit_add_cell:
	pushq	%rbp                    # PROLOGUE
	movq	%rsp, %rbp
# ---- emit_u8(c, 0x80)  opcode add r/m8, imm8 --------------------------------
	movq	8(%rdi), %rax           # rax = len
	cmpq	16(%rdi), %rax
	jae	.Lof_c0
	movq	(%rdi), %rcx            # rcx = buf
	leaq	1(%rax), %rdx
	movq	%rdx, 8(%rdi)           # len++
	movb	$-128, (%rcx,%rax)      # buf[len] = 0x80
	jmp	.Lc1
.Lof_c0:
	movl	$1, 24(%rdi)
.Lc1:                                   # ---- emit_u8(c, 0x03)  ModRM mod00 /0 [rbx]
	movq	8(%rdi), %rax
	cmpq	16(%rdi), %rax
	jae	.Lof_c1
	movq	(%rdi), %rcx
	leaq	1(%rax), %rdx
	movq	%rdx, 8(%rdi)           # len++
	movb	$3, (%rcx,%rax)         # buf[len] = 0x03  ([rbx], no displacement)
	jmp	.Lc2
.Lof_c1:
	movl	$1, 24(%rdi)
.Lc2:                                   # ---- emit_u8(c, delta) ---------------------
	movq	8(%rdi), %rax
	cmpq	16(%rdi), %rax
	jae	.Lof_c2
	movq	(%rdi), %rcx
	leaq	1(%rax), %rdx
	movq	%rdx, 8(%rdi)           # len++
	movb	%sil, (%rcx,%rax)       # buf[len] = delta  (sil, the imm8)
	popq	%rbp                    # EPILOGUE
	retq
.Lof_c2:
	movl	$1, 24(%rdi)
	popq	%rbp
	retq
.Lfunc_end4:
	.size	emit_add_cell, .Lfunc_end4-emit_add_cell

# =============================================================================
# patch_rel32(code_buf *c /*rdi*/, u64 at /*rsi*/, u64 target /*rdx*/)
#
#   i32 disp = target - (at + 4);
#   buf[at+0..3] = disp bytes (little-endian);
#
# The interesting bits:
#   * The whole computation is done in 32 bits (subl/addl on %edx), because disp
#     is an i32 and the high halves of `at`/`target` don't matter to the result.
#   * disp = target - at - 4 is formed as `subl %esi,%edx; addl $-4,%edx`.
#   * clang RELOADS `c->buf` (movq (%rdi),...) before each of the four stores.
#     It cannot prove the stores don't alias the code_buf struct itself, so it
#     conservatively re-reads the base pointer each time — a visible cost of
#     pointer aliasing that a `restrict` qualifier could remove.
# =============================================================================
	.globl	patch_rel32
	.p2align	4
	.type	patch_rel32,@function
patch_rel32:
	pushq	%rbp                    # PROLOGUE
	movq	%rsp, %rbp
	subl	%esi, %edx              # edx = target - at            (low 32 bits)
	addl	$-4, %edx               # edx = target - at - 4  = disp
	movq	(%rdi), %rax            # rax = c->buf   (reload #1)
	movb	%dl, (%rax,%rsi)        # buf[at+0] = disp & 0xff        (dl)
	movq	(%rdi), %rax            # reload #2 (aliasing-conservative)
	movb	%dh, 1(%rax,%rsi)       # buf[at+1] = (disp>>8)&0xff     (dh)
	movl	%edx, %eax              # eax = disp
	shrl	$16, %eax               # eax = disp >> 16
	movq	(%rdi), %rcx            # reload #3
	movb	%al, 2(%rcx,%rsi)       # buf[at+2] = (disp>>16)&0xff
	shrl	$24, %edx               # edx = disp >> 24
	movq	(%rdi), %rax            # reload #4
	movb	%dl, 3(%rax,%rsi)       # buf[at+3] = (disp>>24)&0xff
	popq	%rbp                    # EPILOGUE
	retq
.Lfunc_end5:
	.size	patch_rel32, .Lfunc_end5-patch_rel32

# =============================================================================
# WHAT TO TAKE AWAY
#   * A JIT's "assembler" is just these functions: append prefix/opcode/ModRM
#     bytes, then the immediate little-endian. Nothing more mysterious than that.
#   * At -O1 clang constant-folded modrm(3,0,3) into the literal bytes 48/81/C3
#     and inlined every emit_u8, so emit_add_ptr became a straight-line
#     "store these three constants then splat delta." The optimizer specialized
#     our generic encoder into the exact instruction it emits.
#   * The per-byte bounds check survives inlining — that is the safety property
#     that stops a code generator from scribbling past its buffer and then
#     executing the wreckage. Compare with asm/demo.O2.s to see the checks and
#     stores get reordered and tightened further.
# =============================================================================
	.section	".note.GNU-stack","",@progbits
