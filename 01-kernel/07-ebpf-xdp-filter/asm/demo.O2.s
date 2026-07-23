	.file	"demo.c"
	.text
	.globl	flow_hash                       # -- Begin function flow_hash
	.p2align	4
	.type	flow_hash,@function
flow_hash:                              # @flow_hash
# %bb.0:
	movzbl	(%rdi), %eax
	xorl	$-2128831035, %eax              # imm = 0x811C9DC5
	imull	$16777619, %eax, %eax           # imm = 0x1000193
	movzbl	1(%rdi), %ecx
	xorl	%eax, %ecx
	imull	$16777619, %ecx, %eax           # imm = 0x1000193
	movzbl	2(%rdi), %ecx
	xorl	%eax, %ecx
	imull	$16777619, %ecx, %eax           # imm = 0x1000193
	movzbl	3(%rdi), %ecx
	xorl	%eax, %ecx
	imull	$16777619, %ecx, %eax           # imm = 0x1000193
	movzbl	4(%rdi), %ecx
	xorl	%eax, %ecx
	imull	$16777619, %ecx, %eax           # imm = 0x1000193
	movzbl	5(%rdi), %ecx
	xorl	%eax, %ecx
	imull	$16777619, %ecx, %eax           # imm = 0x1000193
	movzbl	6(%rdi), %ecx
	xorl	%eax, %ecx
	imull	$16777619, %ecx, %eax           # imm = 0x1000193
	movzbl	7(%rdi), %ecx
	xorl	%eax, %ecx
	imull	$16777619, %ecx, %eax           # imm = 0x1000193
	movzbl	8(%rdi), %ecx
	xorl	%eax, %ecx
	imull	$16777619, %ecx, %eax           # imm = 0x1000193
	movzbl	9(%rdi), %ecx
	xorl	%eax, %ecx
	imull	$16777619, %ecx, %eax           # imm = 0x1000193
	movzbl	10(%rdi), %ecx
	xorl	%eax, %ecx
	imull	$16777619, %ecx, %eax           # imm = 0x1000193
	movzbl	11(%rdi), %ecx
	xorl	%eax, %ecx
	imull	$16777619, %ecx, %eax           # imm = 0x1000193
	movzbl	12(%rdi), %ecx
	xorl	%eax, %ecx
	imull	$16777619, %ecx, %eax           # imm = 0x1000193
	retq
.Lfunc_end0:
	.size	flow_hash, .Lfunc_end0-flow_hash
                                        # -- End function
	.globl	flow_bucket                     # -- Begin function flow_bucket
	.p2align	4
	.type	flow_bucket,@function
flow_bucket:                            # @flow_bucket
# %bb.0:
                                        # kill: def $esi killed $esi def $rsi
	movzbl	(%rdi), %eax
	xorl	$-2128831035, %eax              # imm = 0x811C9DC5
	imull	$16777619, %eax, %eax           # imm = 0x1000193
	movzbl	1(%rdi), %ecx
	xorl	%eax, %ecx
	imull	$16777619, %ecx, %eax           # imm = 0x1000193
	movzbl	2(%rdi), %ecx
	xorl	%eax, %ecx
	imull	$16777619, %ecx, %eax           # imm = 0x1000193
	movzbl	3(%rdi), %ecx
	xorl	%eax, %ecx
	imull	$16777619, %ecx, %eax           # imm = 0x1000193
	movzbl	4(%rdi), %ecx
	xorl	%eax, %ecx
	imull	$16777619, %ecx, %eax           # imm = 0x1000193
	movzbl	5(%rdi), %ecx
	xorl	%eax, %ecx
	imull	$16777619, %ecx, %eax           # imm = 0x1000193
	movzbl	6(%rdi), %ecx
	xorl	%eax, %ecx
	imull	$16777619, %ecx, %eax           # imm = 0x1000193
	movzbl	7(%rdi), %ecx
	xorl	%eax, %ecx
	imull	$16777619, %ecx, %eax           # imm = 0x1000193
	movzbl	8(%rdi), %ecx
	xorl	%eax, %ecx
	imull	$16777619, %ecx, %eax           # imm = 0x1000193
	movzbl	9(%rdi), %ecx
	xorl	%eax, %ecx
	imull	$16777619, %ecx, %eax           # imm = 0x1000193
	movzbl	10(%rdi), %ecx
	xorl	%eax, %ecx
	imull	$16777619, %ecx, %eax           # imm = 0x1000193
	movzbl	11(%rdi), %ecx
	xorl	%eax, %ecx
	imull	$16777619, %ecx, %eax           # imm = 0x1000193
	movzbl	12(%rdi), %ecx
	xorl	%eax, %ecx
	imull	$16777619, %ecx, %ecx           # imm = 0x1000193
	leal	-1(%rsi), %eax
	andl	%ecx, %eax
	retq
.Lfunc_end1:
	.size	flow_bucket, .Lfunc_end1-flow_bucket
                                        # -- End function
	.ident	"clang version 20.1.8"
	.section	".note.GNU-stack","",@progbits
	.addrsig
