	.file	"demo.c"
	.text
	.globl	vga_cell_offset                 # -- Begin function vga_cell_offset
	.p2align	4
	.type	vga_cell_offset,@function
vga_cell_offset:                        # @vga_cell_offset
# %bb.0:
	pushq	%rbp
	movq	%rsp, %rbp
	movl	%edi, -4(%rbp)
	movl	%esi, -8(%rbp)
	imull	$80, -4(%rbp), %eax
	addl	-8(%rbp), %eax
	popq	%rbp
	retq
.Lfunc_end0:
	.size	vga_cell_offset, .Lfunc_end0-vga_cell_offset
                                        # -- End function
	.globl	vga_byte_offset                 # -- Begin function vga_byte_offset
	.p2align	4
	.type	vga_byte_offset,@function
vga_byte_offset:                        # @vga_byte_offset
# %bb.0:
	pushq	%rbp
	movq	%rsp, %rbp
	subq	$16, %rsp
	movl	%edi, -4(%rbp)
	movl	%esi, -8(%rbp)
	movl	-4(%rbp), %edi
	movl	-8(%rbp), %esi
	callq	vga_cell_offset
	shll	%eax
	addq	$16, %rsp
	popq	%rbp
	retq
.Lfunc_end1:
	.size	vga_byte_offset, .Lfunc_end1-vga_byte_offset
                                        # -- End function
	.globl	vga_entry                       # -- Begin function vga_entry
	.p2align	4
	.type	vga_entry,@function
vga_entry:                              # @vga_entry
# %bb.0:
	pushq	%rbp
	movq	%rsp, %rbp
	movb	%dl, %al
	movb	%sil, %cl
	movb	%dil, %dl
	movb	%dl, -1(%rbp)
	movb	%cl, -2(%rbp)
	movb	%al, -3(%rbp)
	movzbl	-3(%rbp), %eax
	shll	$4, %eax
	movzbl	-2(%rbp), %ecx
	andl	$15, %ecx
	orl	%ecx, %eax
                                        # kill: def $al killed $al killed $eax
	movb	%al, -4(%rbp)
	movzbl	-4(%rbp), %eax
                                        # kill: def $ax killed $ax killed $eax
	movzwl	%ax, %eax
	shll	$8, %eax
	movzbl	-1(%rbp), %ecx
                                        # kill: def $cx killed $cx killed $ecx
	movzwl	%cx, %ecx
	orl	%ecx, %eax
                                        # kill: def $ax killed $ax killed $eax
	popq	%rbp
	retq
.Lfunc_end2:
	.size	vga_entry, .Lfunc_end2-vga_entry
                                        # -- End function
	.globl	vga_cursor_hi                   # -- Begin function vga_cursor_hi
	.p2align	4
	.type	vga_cursor_hi,@function
vga_cursor_hi:                          # @vga_cursor_hi
# %bb.0:
	pushq	%rbp
	movq	%rsp, %rbp
	movw	%di, %ax
	movw	%ax, -2(%rbp)
	movzwl	-2(%rbp), %eax
	sarl	$8, %eax
	andl	$255, %eax
                                        # kill: def $al killed $al killed $eax
	popq	%rbp
	retq
.Lfunc_end3:
	.size	vga_cursor_hi, .Lfunc_end3-vga_cursor_hi
                                        # -- End function
	.globl	vga_cursor_lo                   # -- Begin function vga_cursor_lo
	.p2align	4
	.type	vga_cursor_lo,@function
vga_cursor_lo:                          # @vga_cursor_lo
# %bb.0:
	pushq	%rbp
	movq	%rsp, %rbp
	movw	%di, %ax
	movw	%ax, -2(%rbp)
	movzwl	-2(%rbp), %eax
	andl	$255, %eax
                                        # kill: def $al killed $al killed $eax
	popq	%rbp
	retq
.Lfunc_end4:
	.size	vga_cursor_lo, .Lfunc_end4-vga_cursor_lo
                                        # -- End function
	.globl	port_reg                        # -- Begin function port_reg
	.p2align	4
	.type	port_reg,@function
port_reg:                               # @port_reg
# %bb.0:
	pushq	%rbp
	movq	%rsp, %rbp
	movw	%si, %ax
	movw	%di, %cx
	movw	%cx, -2(%rbp)
	movw	%ax, -4(%rbp)
	movzwl	-2(%rbp), %eax
	movzwl	-4(%rbp), %ecx
	addl	%ecx, %eax
                                        # kill: def $ax killed $ax killed $eax
	popq	%rbp
	retq
.Lfunc_end5:
	.size	port_reg, .Lfunc_end5-port_reg
                                        # -- End function
	.globl	serial_thr_empty                # -- Begin function serial_thr_empty
	.p2align	4
	.type	serial_thr_empty,@function
serial_thr_empty:                       # @serial_thr_empty
# %bb.0:
	pushq	%rbp
	movq	%rsp, %rbp
	movb	%dil, %al
	movb	%al, -1(%rbp)
	movzbl	-1(%rbp), %eax
	sarl	$5, %eax
	andl	$1, %eax
	popq	%rbp
	retq
.Lfunc_end6:
	.size	serial_thr_empty, .Lfunc_end6-serial_thr_empty
                                        # -- End function
	.globl	pit_divisor                     # -- Begin function pit_divisor
	.p2align	4
	.type	pit_divisor,@function
pit_divisor:                            # @pit_divisor
# %bb.0:
	pushq	%rbp
	movq	%rsp, %rbp
	movl	%edi, -4(%rbp)
	movl	$1193182, %eax                  # imm = 0x1234DE
	xorl	%edx, %edx
	divl	-4(%rbp)
	popq	%rbp
	retq
.Lfunc_end7:
	.size	pit_divisor, .Lfunc_end7-pit_divisor
                                        # -- End function
	.globl	demo_selftest                   # -- Begin function demo_selftest
	.p2align	4
	.type	demo_selftest,@function
demo_selftest:                          # @demo_selftest
# %bb.0:
	pushq	%rbp
	movq	%rsp, %rbp
	subq	$16, %rsp
	xorl	%esi, %esi
	movl	%esi, %edi
	callq	vga_cell_offset
	cmpl	$0, %eax
	je	.LBB8_2
# %bb.1:
	movl	$1, -4(%rbp)
	jmp	.LBB8_23
.LBB8_2:
	movl	$1, %edi
	xorl	%esi, %esi
	callq	vga_cell_offset
	cmpl	$80, %eax
	je	.LBB8_4
# %bb.3:
	movl	$2, -4(%rbp)
	jmp	.LBB8_23
.LBB8_4:
	movl	$24, %edi
	movl	$79, %esi
	callq	vga_cell_offset
	cmpl	$1999, %eax                     # imm = 0x7CF
	je	.LBB8_6
# %bb.5:
	movl	$3, -4(%rbp)
	jmp	.LBB8_23
.LBB8_6:
	movl	$1, %edi
	xorl	%esi, %esi
	callq	vga_byte_offset
	cmpl	$160, %eax
	je	.LBB8_8
# %bb.7:
	movl	$4, -4(%rbp)
	jmp	.LBB8_23
.LBB8_8:
	movl	$65, %edi
	movl	$15, %esi
	movl	$1, %edx
	callq	vga_entry
	movzwl	%ax, %eax
	cmpl	$8001, %eax                     # imm = 0x1F41
	je	.LBB8_10
# %bb.9:
	movl	$5, -4(%rbp)
	jmp	.LBB8_23
.LBB8_10:
	movl	$1999, %edi                     # imm = 0x7CF
	callq	vga_cursor_hi
	movzbl	%al, %eax
	cmpl	$7, %eax
	je	.LBB8_12
# %bb.11:
	movl	$6, -4(%rbp)
	jmp	.LBB8_23
.LBB8_12:
	movl	$1999, %edi                     # imm = 0x7CF
	callq	vga_cursor_lo
	movzbl	%al, %eax
	cmpl	$207, %eax
	je	.LBB8_14
# %bb.13:
	movl	$7, -4(%rbp)
	jmp	.LBB8_23
.LBB8_14:
	movl	$1016, %edi                     # imm = 0x3F8
	movl	$5, %esi
	callq	port_reg
	movzwl	%ax, %eax
	cmpl	$1021, %eax                     # imm = 0x3FD
	je	.LBB8_16
# %bb.15:
	movl	$8, -4(%rbp)
	jmp	.LBB8_23
.LBB8_16:
	movl	$96, %edi
	callq	serial_thr_empty
	cmpl	$0, %eax
	jne	.LBB8_18
# %bb.17:
	movl	$9, -4(%rbp)
	jmp	.LBB8_23
.LBB8_18:
	movl	$1, %edi
	callq	serial_thr_empty
	cmpl	$0, %eax
	je	.LBB8_20
# %bb.19:
	movl	$10, -4(%rbp)
	jmp	.LBB8_23
.LBB8_20:
	movl	$100, %edi
	callq	pit_divisor
	cmpl	$11931, %eax                    # imm = 0x2E9B
	je	.LBB8_22
# %bb.21:
	movl	$11, -4(%rbp)
	jmp	.LBB8_23
.LBB8_22:
	movl	$0, -4(%rbp)
.LBB8_23:
	movl	-4(%rbp), %eax
	addq	$16, %rsp
	popq	%rbp
	retq
.Lfunc_end8:
	.size	demo_selftest, .Lfunc_end8-demo_selftest
                                        # -- End function
	.ident	"clang version 20.1.8"
	.section	".note.GNU-stack","",@progbits
	.addrsig
	.addrsig_sym vga_cell_offset
	.addrsig_sym vga_byte_offset
	.addrsig_sym vga_entry
	.addrsig_sym vga_cursor_hi
	.addrsig_sym vga_cursor_lo
	.addrsig_sym port_reg
	.addrsig_sym serial_thr_empty
	.addrsig_sym pit_divisor
