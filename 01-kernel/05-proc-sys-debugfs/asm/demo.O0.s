	.file	"demo.c"
	.text
	.globl	u64_to_dec                      # -- Begin function u64_to_dec
	.p2align	4
	.type	u64_to_dec,@function
u64_to_dec:                             # @u64_to_dec
# %bb.0:
	pushq	%rbp
	movq	%rsp, %rbp
	movq	%rdi, -16(%rbp)
	movq	%rsi, -24(%rbp)
	movl	$0, -52(%rbp)
	cmpq	$0, -16(%rbp)
	jne	.LBB0_2
# %bb.1:
	movq	-24(%rbp), %rax
	movb	$48, (%rax)
	movq	-24(%rbp), %rax
	movb	$0, 1(%rax)
	movl	$1, -4(%rbp)
	jmp	.LBB0_10
.LBB0_2:
	jmp	.LBB0_3
.LBB0_3:                                # =>This Inner Loop Header: Depth=1
	cmpq	$0, -16(%rbp)
	je	.LBB0_5
# %bb.4:                                #   in Loop: Header=BB0_3 Depth=1
	movq	-16(%rbp), %rax
	movl	$10, %ecx
	xorl	%edx, %edx
                                        # kill: def $rdx killed $edx
	divq	%rcx
	movq	%rdx, -64(%rbp)
	movq	-64(%rbp), %rax
	addq	$48, %rax
	movb	%al, %cl
	movl	-52(%rbp), %eax
	movl	%eax, %edx
	addl	$1, %edx
	movl	%edx, -52(%rbp)
	movl	%eax, %eax
                                        # kill: def $rax killed $eax
	movb	%cl, -48(%rbp,%rax)
	movq	-16(%rbp), %rax
	movl	$10, %ecx
	xorl	%edx, %edx
                                        # kill: def $rdx killed $edx
	divq	%rcx
	movq	%rax, -16(%rbp)
	jmp	.LBB0_3
.LBB0_5:
	movl	$0, -56(%rbp)
.LBB0_6:                                # =>This Inner Loop Header: Depth=1
	movl	-56(%rbp), %eax
	cmpl	-52(%rbp), %eax
	jae	.LBB0_9
# %bb.7:                                #   in Loop: Header=BB0_6 Depth=1
	movl	-52(%rbp), %eax
	subl	$1, %eax
	subl	-56(%rbp), %eax
	movl	%eax, %eax
                                        # kill: def $rax killed $eax
	movb	-48(%rbp,%rax), %dl
	movq	-24(%rbp), %rax
	movl	-56(%rbp), %ecx
                                        # kill: def $rcx killed $ecx
	movb	%dl, (%rax,%rcx)
# %bb.8:                                #   in Loop: Header=BB0_6 Depth=1
	movl	-56(%rbp), %eax
	addl	$1, %eax
	movl	%eax, -56(%rbp)
	jmp	.LBB0_6
.LBB0_9:
	movq	-24(%rbp), %rax
	movl	-52(%rbp), %ecx
                                        # kill: def $rcx killed $ecx
	movb	$0, (%rax,%rcx)
	movl	-52(%rbp), %eax
	movl	%eax, -4(%rbp)
.LBB0_10:
	movl	-4(%rbp), %eax
	popq	%rbp
	retq
.Lfunc_end0:
	.size	u64_to_dec, .Lfunc_end0-u64_to_dec
                                        # -- End function
	.globl	format_field                    # -- Begin function format_field
	.p2align	4
	.type	format_field,@function
format_field:                           # @format_field
# %bb.0:
	pushq	%rbp
	movq	%rsp, %rbp
	subq	$80, %rsp
	movq	%rdi, -8(%rbp)
	movl	%esi, -12(%rbp)
	movq	%rdx, -24(%rbp)
	movq	-8(%rbp), %rdi
	leaq	-48(%rbp), %rsi
	callq	u64_to_dec
	movl	%eax, -52(%rbp)
	movl	$0, -64(%rbp)
	movl	-12(%rbp), %eax
	cmpl	-52(%rbp), %eax
	jbe	.LBB1_2
# %bb.1:
	movl	-12(%rbp), %eax
	subl	-52(%rbp), %eax
	movl	%eax, -68(%rbp)                 # 4-byte Spill
	jmp	.LBB1_3
.LBB1_2:
	xorl	%eax, %eax
	movl	%eax, -68(%rbp)                 # 4-byte Spill
	jmp	.LBB1_3
.LBB1_3:
	movl	-68(%rbp), %eax                 # 4-byte Reload
	movl	%eax, -56(%rbp)
	movl	$0, -60(%rbp)
.LBB1_4:                                # =>This Inner Loop Header: Depth=1
	movl	-60(%rbp), %eax
	cmpl	-56(%rbp), %eax
	jae	.LBB1_7
# %bb.5:                                #   in Loop: Header=BB1_4 Depth=1
	movq	-24(%rbp), %rax
	movl	-64(%rbp), %ecx
	movl	%ecx, %edx
	addl	$1, %edx
	movl	%edx, -64(%rbp)
	movl	%ecx, %ecx
                                        # kill: def $rcx killed $ecx
	movb	$32, (%rax,%rcx)
# %bb.6:                                #   in Loop: Header=BB1_4 Depth=1
	movl	-60(%rbp), %eax
	addl	$1, %eax
	movl	%eax, -60(%rbp)
	jmp	.LBB1_4
.LBB1_7:
	movl	$0, -60(%rbp)
.LBB1_8:                                # =>This Inner Loop Header: Depth=1
	movl	-60(%rbp), %eax
	cmpl	-52(%rbp), %eax
	jae	.LBB1_11
# %bb.9:                                #   in Loop: Header=BB1_8 Depth=1
	movl	-60(%rbp), %eax
                                        # kill: def $rax killed $eax
	movb	-48(%rbp,%rax), %dl
	movq	-24(%rbp), %rax
	movl	-64(%rbp), %ecx
	movl	%ecx, %esi
	addl	$1, %esi
	movl	%esi, -64(%rbp)
	movl	%ecx, %ecx
                                        # kill: def $rcx killed $ecx
	movb	%dl, (%rax,%rcx)
# %bb.10:                               #   in Loop: Header=BB1_8 Depth=1
	movl	-60(%rbp), %eax
	addl	$1, %eax
	movl	%eax, -60(%rbp)
	jmp	.LBB1_8
.LBB1_11:
	movq	-24(%rbp), %rax
	movl	-64(%rbp), %ecx
                                        # kill: def $rcx killed $ecx
	movb	$0, (%rax,%rcx)
	movl	-64(%rbp), %eax
	addq	$80, %rsp
	popq	%rbp
	retq
.Lfunc_end1:
	.size	format_field, .Lfunc_end1-format_field
                                        # -- End function
	.globl	checksum8                       # -- Begin function checksum8
	.p2align	4
	.type	checksum8,@function
checksum8:                              # @checksum8
# %bb.0:
	pushq	%rbp
	movq	%rsp, %rbp
	movq	%rdi, -8(%rbp)
	movl	%esi, -12(%rbp)
	movl	$0, -16(%rbp)
	movl	$0, -20(%rbp)
.LBB2_1:                                # =>This Inner Loop Header: Depth=1
	movl	-20(%rbp), %eax
	cmpl	-12(%rbp), %eax
	jae	.LBB2_4
# %bb.2:                                #   in Loop: Header=BB2_1 Depth=1
	movq	-8(%rbp), %rax
	movl	-20(%rbp), %ecx
                                        # kill: def $rcx killed $ecx
	movzbl	(%rax,%rcx), %eax
	addl	-16(%rbp), %eax
	movl	%eax, -16(%rbp)
# %bb.3:                                #   in Loop: Header=BB2_1 Depth=1
	movl	-20(%rbp), %eax
	addl	$1, %eax
	movl	%eax, -20(%rbp)
	jmp	.LBB2_1
.LBB2_4:
	movl	-16(%rbp), %eax
	andl	$255, %eax
	popq	%rbp
	retq
.Lfunc_end2:
	.size	checksum8, .Lfunc_end2-checksum8
                                        # -- End function
	.globl	render_line                     # -- Begin function render_line
	.p2align	4
	.type	render_line,@function
render_line:                            # @render_line
# %bb.0:
	pushq	%rbp
	movq	%rsp, %rbp
	subq	$32, %rsp
	movl	%edi, -4(%rbp)
	movq	%rsi, -16(%rbp)
	movq	%rdx, -24(%rbp)
	movl	-4(%rbp), %eax
	movl	%eax, %edi
	movq	-24(%rbp), %rsi
	callq	u64_to_dec
	movl	%eax, -28(%rbp)
	movq	-24(%rbp), %rax
	movl	-28(%rbp), %ecx
	movl	%ecx, %edx
	addl	$1, %edx
	movl	%edx, -28(%rbp)
	movl	%ecx, %ecx
                                        # kill: def $rcx killed $ecx
	movb	$32, (%rax,%rcx)
	movq	-16(%rbp), %rdi
	movq	-24(%rbp), %rdx
	movl	-28(%rbp), %eax
                                        # kill: def $rax killed $eax
	addq	%rax, %rdx
	movl	$20, %esi
	callq	format_field
	addl	-28(%rbp), %eax
	movl	%eax, -28(%rbp)
	movl	-28(%rbp), %eax
	addq	$32, %rsp
	popq	%rbp
	retq
.Lfunc_end3:
	.size	render_line, .Lfunc_end3-render_line
                                        # -- End function
	.ident	"clang version 20.1.8"
	.section	".note.GNU-stack","",@progbits
	.addrsig
	.addrsig_sym u64_to_dec
	.addrsig_sym format_field
