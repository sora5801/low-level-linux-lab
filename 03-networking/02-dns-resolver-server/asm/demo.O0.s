	.file	"demo.c"
	.text
	.globl	dns_decode_name                 # -- Begin function dns_decode_name
	.p2align	4
	.type	dns_decode_name,@function
dns_decode_name:                        # @dns_decode_name
# %bb.0:
	pushq	%rbp
	movq	%rsp, %rbp
	movq	%rdi, -16(%rbp)
	movl	%esi, -20(%rbp)
	movl	%edx, -24(%rbp)
	movq	%rcx, -32(%rbp)
	movl	%r8d, -36(%rbp)
	movl	-24(%rbp), %eax
	movl	%eax, -40(%rbp)
	movl	$0, -44(%rbp)
	movl	$0, -48(%rbp)
	movl	-20(%rbp), %eax
	addl	$1, %eax
	movl	%eax, -52(%rbp)
	cmpl	$0, -36(%rbp)
	jne	.LBB0_2
# %bb.1:
	movl	$-1, -4(%rbp)
	jmp	.LBB0_35
.LBB0_2:
	movq	-32(%rbp), %rax
	movb	$0, (%rax)
.LBB0_3:                                # =>This Loop Header: Depth=1
                                        #     Child Loop BB0_30 Depth 2
	movl	-40(%rbp), %eax
	cmpl	-20(%rbp), %eax
	jb	.LBB0_5
# %bb.4:
	movl	$-1, -4(%rbp)
	jmp	.LBB0_35
.LBB0_5:                                #   in Loop: Header=BB0_3 Depth=1
	movq	-16(%rbp), %rax
	movl	-40(%rbp), %ecx
                                        # kill: def $rcx killed $ecx
	movb	(%rax,%rcx), %al
	movb	%al, -53(%rbp)
	movzbl	-53(%rbp), %eax
	andl	$192, %eax
	cmpl	$192, %eax
	jne	.LBB0_13
# %bb.6:                                #   in Loop: Header=BB0_3 Depth=1
	movl	-40(%rbp), %eax
	addl	$2, %eax
	cmpl	-20(%rbp), %eax
	jbe	.LBB0_8
# %bb.7:
	movl	$-1, -4(%rbp)
	jmp	.LBB0_35
.LBB0_8:                                #   in Loop: Header=BB0_3 Depth=1
	movzbl	-53(%rbp), %eax
	andl	$63, %eax
	shll	$8, %eax
	movq	-16(%rbp), %rcx
	movl	-40(%rbp), %edx
	addl	$1, %edx
	movl	%edx, %edx
                                        # kill: def $rdx killed $edx
	movzbl	(%rcx,%rdx), %ecx
	orl	%ecx, %eax
	movl	%eax, -60(%rbp)
	movl	-48(%rbp), %eax
	addl	$1, %eax
	movl	%eax, -48(%rbp)
	cmpl	-52(%rbp), %eax
	jbe	.LBB0_10
# %bb.9:
	movl	$-1, -4(%rbp)
	jmp	.LBB0_35
.LBB0_10:                               #   in Loop: Header=BB0_3 Depth=1
	movl	-60(%rbp), %eax
	cmpl	-20(%rbp), %eax
	jb	.LBB0_12
# %bb.11:
	movl	$-1, -4(%rbp)
	jmp	.LBB0_35
.LBB0_12:                               #   in Loop: Header=BB0_3 Depth=1
	movl	-60(%rbp), %eax
	movl	%eax, -40(%rbp)
	jmp	.LBB0_3
.LBB0_13:                               #   in Loop: Header=BB0_3 Depth=1
	movzbl	-53(%rbp), %eax
	andl	$192, %eax
	cmpl	$0, %eax
	je	.LBB0_15
# %bb.14:
	movl	$-1, -4(%rbp)
	jmp	.LBB0_35
.LBB0_15:                               #   in Loop: Header=BB0_3 Depth=1
	movzbl	-53(%rbp), %eax
	cmpl	$0, %eax
	jne	.LBB0_17
# %bb.16:
	jmp	.LBB0_34
.LBB0_17:                               #   in Loop: Header=BB0_3 Depth=1
	movzbl	-53(%rbp), %eax
	cmpl	$63, %eax
	jle	.LBB0_19
# %bb.18:
	movl	$-1, -4(%rbp)
	jmp	.LBB0_35
.LBB0_19:                               #   in Loop: Header=BB0_3 Depth=1
	movl	-40(%rbp), %eax
	addl	$1, %eax
	movzbl	-53(%rbp), %ecx
	addl	%ecx, %eax
	cmpl	-20(%rbp), %eax
	jbe	.LBB0_21
# %bb.20:
	movl	$-1, -4(%rbp)
	jmp	.LBB0_35
.LBB0_21:                               #   in Loop: Header=BB0_3 Depth=1
	cmpl	$0, -44(%rbp)
	je	.LBB0_25
# %bb.22:                               #   in Loop: Header=BB0_3 Depth=1
	movl	-44(%rbp), %eax
	addl	$1, %eax
	cmpl	-36(%rbp), %eax
	jb	.LBB0_24
# %bb.23:
	movl	$-1, -4(%rbp)
	jmp	.LBB0_35
.LBB0_24:                               #   in Loop: Header=BB0_3 Depth=1
	movq	-32(%rbp), %rax
	movl	-44(%rbp), %ecx
	movl	%ecx, %edx
	addl	$1, %edx
	movl	%edx, -44(%rbp)
	movl	%ecx, %ecx
                                        # kill: def $rcx killed $ecx
	movb	$46, (%rax,%rcx)
.LBB0_25:                               #   in Loop: Header=BB0_3 Depth=1
	movl	-44(%rbp), %eax
	movzbl	-53(%rbp), %ecx
	addl	%ecx, %eax
	cmpl	-36(%rbp), %eax
	jb	.LBB0_27
# %bb.26:
	movl	$-1, -4(%rbp)
	jmp	.LBB0_35
.LBB0_27:                               #   in Loop: Header=BB0_3 Depth=1
	movl	-44(%rbp), %eax
	movzbl	-53(%rbp), %ecx
	addl	%ecx, %eax
	cmpl	$255, %eax
	jbe	.LBB0_29
# %bb.28:
	movl	$-1, -4(%rbp)
	jmp	.LBB0_35
.LBB0_29:                               #   in Loop: Header=BB0_3 Depth=1
	movl	$0, -64(%rbp)
.LBB0_30:                               #   Parent Loop BB0_3 Depth=1
                                        # =>  This Inner Loop Header: Depth=2
	movl	-64(%rbp), %eax
	movzbl	-53(%rbp), %ecx
	cmpl	%ecx, %eax
	jae	.LBB0_33
# %bb.31:                               #   in Loop: Header=BB0_30 Depth=2
	movq	-16(%rbp), %rax
	movl	-40(%rbp), %ecx
	addl	$1, %ecx
	addl	-64(%rbp), %ecx
	movl	%ecx, %ecx
                                        # kill: def $rcx killed $ecx
	movb	(%rax,%rcx), %dl
	movq	-32(%rbp), %rax
	movl	-44(%rbp), %ecx
	addl	-64(%rbp), %ecx
	movl	%ecx, %ecx
                                        # kill: def $rcx killed $ecx
	movb	%dl, (%rax,%rcx)
# %bb.32:                               #   in Loop: Header=BB0_30 Depth=2
	movl	-64(%rbp), %eax
	addl	$1, %eax
	movl	%eax, -64(%rbp)
	jmp	.LBB0_30
.LBB0_33:                               #   in Loop: Header=BB0_3 Depth=1
	movzbl	-53(%rbp), %eax
	addl	-44(%rbp), %eax
	movl	%eax, -44(%rbp)
	movzbl	-53(%rbp), %eax
	addl	$1, %eax
	addl	-40(%rbp), %eax
	movl	%eax, -40(%rbp)
	jmp	.LBB0_3
.LBB0_34:
	movq	-32(%rbp), %rax
	movl	-44(%rbp), %ecx
                                        # kill: def $rcx killed $ecx
	movb	$0, (%rax,%rcx)
	movl	-44(%rbp), %eax
	movl	%eax, -4(%rbp)
.LBB0_35:
	movl	-4(%rbp), %eax
	popq	%rbp
	retq
.Lfunc_end0:
	.size	dns_decode_name, .Lfunc_end0-dns_decode_name
                                        # -- End function
	.ident	"clang version 20.1.8"
	.section	".note.GNU-stack","",@progbits
	.addrsig
