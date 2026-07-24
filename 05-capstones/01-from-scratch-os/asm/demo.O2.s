	.file	"demo.c"
	.text
	.globl	vga_cell_offset                 # -- Begin function vga_cell_offset
	.p2align	4
	.type	vga_cell_offset,@function
vga_cell_offset:                        # @vga_cell_offset
# %bb.0:
                                        # kill: def $edi killed $edi def $rdi
	leal	(%rdi,%rdi,4), %eax
	shll	$4, %eax
	addl	%esi, %eax
	retq
.Lfunc_end0:
	.size	vga_cell_offset, .Lfunc_end0-vga_cell_offset
                                        # -- End function
	.globl	vga_byte_offset                 # -- Begin function vga_byte_offset
	.p2align	4
	.type	vga_byte_offset,@function
vga_byte_offset:                        # @vga_byte_offset
# %bb.0:
                                        # kill: def $edi killed $edi def $rdi
	leal	(%rdi,%rdi,4), %eax
	shll	$4, %eax
	addl	%esi, %eax
	addl	%eax, %eax
	retq
.Lfunc_end1:
	.size	vga_byte_offset, .Lfunc_end1-vga_byte_offset
                                        # -- End function
	.globl	vga_entry                       # -- Begin function vga_entry
	.p2align	4
	.type	vga_entry,@function
vga_entry:                              # @vga_entry
# %bb.0:
	shlb	$4, %dl
	andb	$15, %sil
	orb	%dl, %sil
	movzbl	%sil, %eax
	shll	$8, %eax
	orl	%edi, %eax
                                        # kill: def $ax killed $ax killed $eax
	retq
.Lfunc_end2:
	.size	vga_entry, .Lfunc_end2-vga_entry
                                        # -- End function
	.globl	vga_cursor_hi                   # -- Begin function vga_cursor_hi
	.p2align	4
	.type	vga_cursor_hi,@function
vga_cursor_hi:                          # @vga_cursor_hi
# %bb.0:
	movl	%edi, %eax
	shrl	$8, %eax
                                        # kill: def $al killed $al killed $eax
	retq
.Lfunc_end3:
	.size	vga_cursor_hi, .Lfunc_end3-vga_cursor_hi
                                        # -- End function
	.globl	vga_cursor_lo                   # -- Begin function vga_cursor_lo
	.p2align	4
	.type	vga_cursor_lo,@function
vga_cursor_lo:                          # @vga_cursor_lo
# %bb.0:
	movl	%edi, %eax
                                        # kill: def $al killed $al killed $eax
	retq
.Lfunc_end4:
	.size	vga_cursor_lo, .Lfunc_end4-vga_cursor_lo
                                        # -- End function
	.globl	port_reg                        # -- Begin function port_reg
	.p2align	4
	.type	port_reg,@function
port_reg:                               # @port_reg
# %bb.0:
                                        # kill: def $esi killed $esi def $rsi
                                        # kill: def $edi killed $edi def $rdi
	leal	(%rdi,%rsi), %eax
                                        # kill: def $ax killed $ax killed $eax
	retq
.Lfunc_end5:
	.size	port_reg, .Lfunc_end5-port_reg
                                        # -- End function
	.globl	serial_thr_empty                # -- Begin function serial_thr_empty
	.p2align	4
	.type	serial_thr_empty,@function
serial_thr_empty:                       # @serial_thr_empty
# %bb.0:
	shrb	$5, %dil
	andb	$1, %dil
	movzbl	%dil, %eax
	retq
.Lfunc_end6:
	.size	serial_thr_empty, .Lfunc_end6-serial_thr_empty
                                        # -- End function
	.globl	pit_divisor                     # -- Begin function pit_divisor
	.p2align	4
	.type	pit_divisor,@function
pit_divisor:                            # @pit_divisor
# %bb.0:
	movl	$1193182, %eax                  # imm = 0x1234DE
	xorl	%edx, %edx
	divl	%edi
	retq
.Lfunc_end7:
	.size	pit_divisor, .Lfunc_end7-pit_divisor
                                        # -- End function
	.globl	demo_selftest                   # -- Begin function demo_selftest
	.p2align	4
	.type	demo_selftest,@function
demo_selftest:                          # @demo_selftest
# %bb.0:
	xorl	%eax, %eax
	retq
.Lfunc_end8:
	.size	demo_selftest, .Lfunc_end8-demo_selftest
                                        # -- End function
	.ident	"clang version 20.1.8"
	.section	".note.GNU-stack","",@progbits
	.addrsig
