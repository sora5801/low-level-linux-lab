	.file	"demo.c"
	.text
	.globl	flow_hash                       # -- Begin function flow_hash
	.p2align	4
	.type	flow_hash,@function
flow_hash:                              # @flow_hash
# %bb.0:
	pushq	%rbp
	movq	%rsp, %rbp
	movq	%rdi, -8(%rbp)
	movl	$-2128831035, -12(%rbp)         # imm = 0x811C9DC5
	movl	$16777619, -16(%rbp)            # imm = 0x1000193
	movl	$-2128831035, -20(%rbp)         # imm = 0x811C9DC5
	movq	-8(%rbp), %rax
	movq	%rax, -32(%rbp)
	movl	-20(%rbp), %eax
	movq	-32(%rbp), %rcx
	movzbl	(%rcx), %ecx
	xorl	%ecx, %eax
	imull	$16777619, %eax, %eax           # imm = 0x1000193
	movl	%eax, -20(%rbp)
	movl	-20(%rbp), %eax
	movq	-32(%rbp), %rcx
	movzbl	1(%rcx), %ecx
	xorl	%ecx, %eax
	imull	$16777619, %eax, %eax           # imm = 0x1000193
	movl	%eax, -20(%rbp)
	movl	-20(%rbp), %eax
	movq	-32(%rbp), %rcx
	movzbl	2(%rcx), %ecx
	xorl	%ecx, %eax
	imull	$16777619, %eax, %eax           # imm = 0x1000193
	movl	%eax, -20(%rbp)
	movl	-20(%rbp), %eax
	movq	-32(%rbp), %rcx
	movzbl	3(%rcx), %ecx
	xorl	%ecx, %eax
	imull	$16777619, %eax, %eax           # imm = 0x1000193
	movl	%eax, -20(%rbp)
	movq	-8(%rbp), %rax
	addq	$4, %rax
	movq	%rax, -32(%rbp)
	movl	-20(%rbp), %eax
	movq	-32(%rbp), %rcx
	movzbl	(%rcx), %ecx
	xorl	%ecx, %eax
	imull	$16777619, %eax, %eax           # imm = 0x1000193
	movl	%eax, -20(%rbp)
	movl	-20(%rbp), %eax
	movq	-32(%rbp), %rcx
	movzbl	1(%rcx), %ecx
	xorl	%ecx, %eax
	imull	$16777619, %eax, %eax           # imm = 0x1000193
	movl	%eax, -20(%rbp)
	movl	-20(%rbp), %eax
	movq	-32(%rbp), %rcx
	movzbl	2(%rcx), %ecx
	xorl	%ecx, %eax
	imull	$16777619, %eax, %eax           # imm = 0x1000193
	movl	%eax, -20(%rbp)
	movl	-20(%rbp), %eax
	movq	-32(%rbp), %rcx
	movzbl	3(%rcx), %ecx
	xorl	%ecx, %eax
	imull	$16777619, %eax, %eax           # imm = 0x1000193
	movl	%eax, -20(%rbp)
	movq	-8(%rbp), %rax
	addq	$8, %rax
	movq	%rax, -32(%rbp)
	movl	-20(%rbp), %eax
	movq	-32(%rbp), %rcx
	movzbl	(%rcx), %ecx
	xorl	%ecx, %eax
	imull	$16777619, %eax, %eax           # imm = 0x1000193
	movl	%eax, -20(%rbp)
	movl	-20(%rbp), %eax
	movq	-32(%rbp), %rcx
	movzbl	1(%rcx), %ecx
	xorl	%ecx, %eax
	imull	$16777619, %eax, %eax           # imm = 0x1000193
	movl	%eax, -20(%rbp)
	movq	-8(%rbp), %rax
	addq	$10, %rax
	movq	%rax, -32(%rbp)
	movl	-20(%rbp), %eax
	movq	-32(%rbp), %rcx
	movzbl	(%rcx), %ecx
	xorl	%ecx, %eax
	imull	$16777619, %eax, %eax           # imm = 0x1000193
	movl	%eax, -20(%rbp)
	movl	-20(%rbp), %eax
	movq	-32(%rbp), %rcx
	movzbl	1(%rcx), %ecx
	xorl	%ecx, %eax
	imull	$16777619, %eax, %eax           # imm = 0x1000193
	movl	%eax, -20(%rbp)
	movl	-20(%rbp), %eax
	movq	-8(%rbp), %rcx
	movzbl	12(%rcx), %ecx
	xorl	%ecx, %eax
	imull	$16777619, %eax, %eax           # imm = 0x1000193
	movl	%eax, -20(%rbp)
	movl	-20(%rbp), %eax
	popq	%rbp
	retq
.Lfunc_end0:
	.size	flow_hash, .Lfunc_end0-flow_hash
                                        # -- End function
	.globl	flow_bucket                     # -- Begin function flow_bucket
	.p2align	4
	.type	flow_bucket,@function
flow_bucket:                            # @flow_bucket
# %bb.0:
	pushq	%rbp
	movq	%rsp, %rbp
	subq	$16, %rsp
	movq	%rdi, -8(%rbp)
	movl	%esi, -12(%rbp)
	movq	-8(%rbp), %rdi
	callq	flow_hash
	movl	-12(%rbp), %ecx
	subl	$1, %ecx
	andl	%ecx, %eax
	addq	$16, %rsp
	popq	%rbp
	retq
.Lfunc_end1:
	.size	flow_bucket, .Lfunc_end1-flow_bucket
                                        # -- End function
	.ident	"clang version 20.1.8"
	.section	".note.GNU-stack","",@progbits
	.addrsig
	.addrsig_sym flow_hash
