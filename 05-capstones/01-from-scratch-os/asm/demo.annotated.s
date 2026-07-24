# =============================================================================
# demo.annotated.s — asm/demo.s (-O1) explained instruction by instruction.
# =============================================================================
#
# HOW TO READ THIS FILE
# ---------------------
# This is the EXACT assembly clang emits for asm/demo.c at -O1 (see demo.s for
# the untouched original), annotated line by line. AT&T syntax throughout:
#
#     op   source, destination        # movl %edi, %eax  =>  eax = edi
#     %reg                             # a register
#     $imm                             # an immediate constant
#     N(%reg)                          # memory at address reg + N
#     (%r1,%r2,s)                      # memory/address at r1 + r2*s (s = scale)
#
# Register widths are the SAME register: rax(64)/eax(32)/ax(16)/al(8). Writing
# a 32-bit sub-register (e.g. `movl`) ZERO-EXTENDS into the full 64-bit register,
# which is why the compiler freely uses `eax` when it only needs 32 bits.
#
# THE SysV AMD64 ABI THIS CODE OBEYS
# ----------------------------------
#   integer/pointer args, in order:  rdi, rsi, rdx, rcx, r8, r9   (then stack)
#   return value:                    rax  (al/ax/eax for narrower types)
#   callee-saved (must preserve):    rbx, rbp, r12-r15, rsp
#   caller-saved (free to clobber):  rax, rcx, rdx, rsi, rdi, r8-r11
#
# Every function here is a LEAF (it calls nothing), so it needs no stack frame
# at all — yet at -O1 clang still emits `push %rbp / mov %rsp,%rbp / pop %rbp`
# purely so a debugger can walk the stack. That frame is pure debuggability tax;
# at -O2 (see demo.O2.s) most of it disappears.
#
# THE BIG PICTURE
# ---------------
# These are the console driver's pure computations: where a character cell lives,
# how a cell is packed, how the cursor position is split for the CRTC, which I/O
# port a device register sits on, and the timer-divisor division. The single
# most instructive line in the file is the `lea`+`shl` that multiplies by 80
# without ever emitting an `imul` — read on.
# =============================================================================

	.file	"demo.c"
	.text

# =============================================================================
# u32 vga_cell_offset(u32 row, u32 col)        row -> edi, col -> esi, ret -> eax
# -----------------------------------------------------------------------------
# Returns row*80 + col, the linear index of a character cell. The lesson is how
# the compiler multiplies by 80 with NO multiply instruction: 80 = 5 * 16, so it
# computes row*5 with a scaled `lea`, then row*5*16 with a shift-by-4. Shifts and
# `lea` are cheaper and lower-latency than a general `imul`; this is "strength
# reduction," and you will see it constantly in real optimized code.
# =============================================================================
	.globl	vga_cell_offset
	.p2align	4
	.type	vga_cell_offset,@function
vga_cell_offset:
	pushq	%rbp                    # PROLOGUE: save caller's frame pointer
	movq	%rsp, %rbp              #   establish our frame (debug-only here)
	# kill: def $edi killed $edi def $rdi   <- compiler note: edi's upper bits are
	#                                          now defined (zero) in rdi; no code.
	leal	(%rdi,%rdi,4), %eax     # eax = rdi + rdi*4 = row*5   (scaled lea, no imul)
	shll	$4, %eax                # eax <<= 4  => (row*5)*16 = row*80
	addl	%esi, %eax              # eax += col  => row*80 + col   (the result)
	popq	%rbp                    # EPILOGUE: restore caller's frame pointer
	retq                            # return; result already in eax
.Lfunc_end0:
	.size	vga_cell_offset, .Lfunc_end0-vga_cell_offset

# =============================================================================
# u32 vga_byte_offset(u32 row, u32 col)        row -> edi, col -> esi, ret -> eax
# -----------------------------------------------------------------------------
# The same cell index, doubled, because each VGA cell is 2 bytes. Note the
# compiler INLINED vga_cell_offset (same three instructions) and then did the
# `*2` as `add %eax,%eax` — adding a value to itself is the cheapest possible
# doubling (one micro-op, no shifter, no immediate).
# =============================================================================
	.globl	vga_byte_offset
	.p2align	4
	.type	vga_byte_offset,@function
vga_byte_offset:
	pushq	%rbp                    # PROLOGUE
	movq	%rsp, %rbp
	leal	(%rdi,%rdi,4), %eax     # row*5           (inlined vga_cell_offset)
	shll	$4, %eax                # *16 -> row*80
	addl	%esi, %eax              # + col -> cell index
	addl	%eax, %eax              # *2 -> byte offset from 0xB8000
	popq	%rbp                    # EPILOGUE
	retq
.Lfunc_end1:
	.size	vga_byte_offset, .Lfunc_end1-vga_byte_offset

# =============================================================================
# u16 vga_entry(u8 ch, u8 fg, u8 bg)      ch->edi, fg->esi, bg->edx, ret->ax
# -----------------------------------------------------------------------------
# Pack a cell:  attr = (bg<<4)|(fg&0x0F);  cell = (attr<<8)|ch. The compiler
# builds the attribute in %sil (the low byte of rsi, where fg arrived), then
# widens and shifts into place. Watch it operate on 8-bit sub-registers (%dl,
# %sil, %dil) to match the u8 types exactly, only widening at the end.
# =============================================================================
	.globl	vga_entry
	.p2align	4
	.type	vga_entry,@function
vga_entry:
	pushq	%rbp                    # PROLOGUE
	movq	%rsp, %rbp
	shlb	$4, %dl                 # dl = bg << 4        (background into high nibble)
	andb	$15, %sil               # sil = fg & 0x0F     (foreground, low nibble only)
	orb	%dl, %sil               # sil = (bg<<4)|fg    = the attribute byte
	movzbl	%sil, %eax              # eax = zero-extend(attr)  (clear the upper bits)
	shll	$8, %eax                # eax = attr << 8     (attribute into the high byte)
	orl	%edi, %eax              # eax = (attr<<8)|ch  = the 16-bit cell value
	# kill: def $ax killed $ax killed $eax  <- only the low 16 bits (ax) are the u16 result
	popq	%rbp                    # EPILOGUE
	retq
.Lfunc_end2:
	.size	vga_entry, .Lfunc_end2-vga_entry

# =============================================================================
# u8 vga_cursor_hi(u16 pos)                    pos -> edi, ret -> al
# -----------------------------------------------------------------------------
# The high byte of the cursor position, for CRTC register 0x0E. Just pos>>8.
# =============================================================================
	.globl	vga_cursor_hi
	.p2align	4
	.type	vga_cursor_hi,@function
vga_cursor_hi:
	pushq	%rbp                    # PROLOGUE
	movq	%rsp, %rbp
	movl	%edi, %eax              # eax = pos           (copy arg to the return reg)
	shrl	$8, %eax                # eax >>= 8           (bits 8..15 drop to 0..7)
	# kill: def $al killed $al killed $eax  <- the byte we want is now in al
	popq	%rbp                    # EPILOGUE
	retq
.Lfunc_end3:
	.size	vga_cursor_hi, .Lfunc_end3-vga_cursor_hi

# =============================================================================
# u8 vga_cursor_lo(u16 pos)                    pos -> edi, ret -> al
# -----------------------------------------------------------------------------
# The low byte, for CRTC register 0x0F. The C says `pos & 0xFF`, but the return
# type is u8, so the caller only ever looks at %al — the low 8 bits of the copy.
# The compiler therefore drops the AND entirely: `mov %edi,%eax` and done. A neat
# reminder that the ABI's narrow return type can make a masking op free.
# =============================================================================
	.globl	vga_cursor_lo
	.p2align	4
	.type	vga_cursor_lo,@function
vga_cursor_lo:
	pushq	%rbp                    # PROLOGUE
	movq	%rsp, %rbp
	movl	%edi, %eax              # eax = pos ; the u8 result is simply its low byte al
	# kill: def $al killed $al killed $eax
	popq	%rbp                    # EPILOGUE
	retq
.Lfunc_end4:
	.size	vga_cursor_lo, .Lfunc_end4-vga_cursor_lo

# =============================================================================
# u16 port_reg(u16 base, u16 off)         base -> edi, off -> esi, ret -> ax
# -----------------------------------------------------------------------------
# base + off — the address of a device register in the I/O port space. One `lea`
# does the add without touching the flags (unlike `add`), and without a separate
# `mov` to the result register: `lea` computes an address expression straight
# into %eax. In the real driver an `outb`/`inb` to this port would follow.
# =============================================================================
	.globl	port_reg
	.p2align	4
	.type	port_reg,@function
port_reg:
	pushq	%rbp                    # PROLOGUE
	movq	%rsp, %rbp
	leal	(%rdi,%rsi), %eax       # eax = base + off   (add via lea, result in eax)
	# kill: def $ax killed $ax killed $eax  <- low 16 bits = the u16 port number
	popq	%rbp                    # EPILOGUE
	retq
.Lfunc_end5:
	.size	port_reg, .Lfunc_end5-port_reg

# =============================================================================
# u32 serial_thr_empty(u8 lsr)                 lsr -> edi, ret -> eax
# -----------------------------------------------------------------------------
# Extract bit 5 of the UART Line Status Register: (lsr>>5)&1. Done in the 8-bit
# sub-register %dil, then zero-extended to the u32 return. This is the exact
# predicate serial.c spins on before sending a byte, minus the `inb`.
# =============================================================================
	.globl	serial_thr_empty
	.p2align	4
	.type	serial_thr_empty,@function
serial_thr_empty:
	pushq	%rbp                    # PROLOGUE
	movq	%rsp, %rbp
	shrb	$5, %dil                # dil >>= 5           (bit 5 becomes bit 0)
	andb	$1, %dil                # dil &= 1            (isolate that one bit -> 0/1)
	movzbl	%dil, %eax              # eax = zero-extend(dil)   (u32 result, 0 or 1)
	popq	%rbp                    # EPILOGUE
	retq
.Lfunc_end6:
	.size	serial_thr_empty, .Lfunc_end6-serial_thr_empty

# =============================================================================
# u32 pit_divisor(u32 hz)                      hz -> edi, ret -> eax
# -----------------------------------------------------------------------------
# 1193182 / hz. Because hz is a RUNTIME value (not a constant), the compiler
# cannot use the multiply-by-magic-reciprocal trick and must emit a real
# hardware `divl`. `divl %edi` divides the 64-bit value EDX:EAX by edi; the ABI
# for unsigned 32-bit division therefore requires EDX be zeroed first, which is
# what `xor %edx,%edx` does. Quotient lands in eax (our result); remainder in edx.
# =============================================================================
	.globl	pit_divisor
	.p2align	4
	.type	pit_divisor,@function
pit_divisor:
	pushq	%rbp                    # PROLOGUE
	movq	%rsp, %rbp
	movl	$1193182, %eax          # eax = dividend low half (imm 0x1234DE = base clock)
	xorl	%edx, %edx              # edx = 0 : the high half of the 64-bit dividend
	divl	%edi                    # EDX:EAX / hz -> quotient in eax, remainder in edx
	popq	%rbp                    # EPILOGUE  (quotient already in eax = the divisor)
	retq
.Lfunc_end7:
	.size	pit_divisor, .Lfunc_end7-pit_divisor

# =============================================================================
# int demo_selftest(void)                      no args, ret -> eax
# -----------------------------------------------------------------------------
# THE PAYOFF. The C body is a dozen `if` checks over CONSTANT inputs. At -O1 the
# optimizer evaluated every one at compile time, proved they all pass, and
# collapsed the entire function to "return 0". So all that survives is the
# idiomatic zeroing of the return register:
# =============================================================================
	.globl	demo_selftest
	.p2align	4
	.type	demo_selftest,@function
demo_selftest:
	pushq	%rbp                    # PROLOGUE
	movq	%rsp, %rbp
	xorl	%eax, %eax              # eax = 0 : every check folded to true -> return 0
	popq	%rbp                    # EPILOGUE
	retq
.Lfunc_end8:
	.size	demo_selftest, .Lfunc_end8-demo_selftest

	.ident	"clang version 20.1.8"
	.section	".note.GNU-stack","",@progbits    # stack need not be executable
	.addrsig                                          # address-significance table (LTO aid)

# =============================================================================
# WHAT TO TAKE AWAY
#   * Multiplying by 80 needed NO `imul`: `lea (%rdi,%rdi,4)` + `shl $4` is the
#     strength-reduced form the compiler prefers. Spotting these turns "why is it
#     doing THAT?" into "oh, that's row*80."
#   * A narrow return type (u8) can delete work: vga_cursor_lo's `& 0xFF` vanished
#     because the caller only reads %al.
#   * Division by a VARIABLE emits a real `divl` and forces `xor %edx,%edx` first;
#     division by a CONSTANT would instead become a multiply — compare in your head.
#   * The optimizer constant-folded a whole test suite to `xor %eax,%eax`. Open
#     demo.O0.s to see the same source written the naive way (every check a real
#     compare-and-branch), and demo.O2.s to confirm the folding holds.
# =============================================================================
