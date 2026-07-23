	.file	"demo.c"
	.text
	.globl	pg_path_has_prefix              # -- Begin function pg_path_has_prefix
	.p2align	4
	.type	pg_path_has_prefix,@function
pg_path_has_prefix:                     # @pg_path_has_prefix
# %bb.0:
	pushq	%rbp
	movq	%rsp, %rbp
	movzbl	(%rsi), %edx
	testb	%dl, %dl
	je	.LBB0_1
# %bb.2:
	xorl	%eax, %eax
	xorl	%ecx, %ecx
	.p2align	4
.LBB0_3:                                # =>This Inner Loop Header: Depth=1
	cmpb	%dl, (%rdi,%rcx)
	jne	.LBB0_6
# %bb.4:                                #   in Loop: Header=BB0_3 Depth=1
	movzbl	1(%rsi,%rcx), %edx
	incq	%rcx
	testb	%dl, %dl
	jne	.LBB0_3
	jmp	.LBB0_5
.LBB0_1:
	xorl	%ecx, %ecx
.LBB0_5:
	movzbl	(%rdi,%rcx), %eax
	testb	%al, %al
	sete	%cl
	cmpb	$47, %al
	sete	%al
	orb	%cl, %al
	movzbl	%al, %eax
.LBB0_6:
                                        # kill: def $eax killed $eax killed $rax
	popq	%rbp
	retq
.Lfunc_end0:
	.size	pg_path_has_prefix, .Lfunc_end0-pg_path_has_prefix
                                        # -- End function
	.globl	pg_policy_lookup                # -- Begin function pg_policy_lookup
	.p2align	4
	.type	pg_policy_lookup,@function
pg_policy_lookup:                       # @pg_policy_lookup
# %bb.0:
	movl	%ecx, %eax
	testl	%edx, %edx
	jle	.LBB1_11
# %bb.1:
	pushq	%rbp
	movq	%rsp, %rbp
	movl	%edx, %ecx
	xorl	%edx, %edx
	jmp	.LBB1_2
	.p2align	4
.LBB1_9:                                #   in Loop: Header=BB1_2 Depth=1
	incq	%rdx
	cmpq	%rcx, %rdx
	je	.LBB1_10
.LBB1_2:                                # =>This Loop Header: Depth=1
                                        #     Child Loop BB1_4 Depth 2
	movq	%rdx, %r8
	shlq	$4, %r8
	movq	(%rsi,%r8), %r9
	movzbl	(%r9), %r11d
	xorl	%r10d, %r10d
	testb	%r11b, %r11b
	je	.LBB1_6
	.p2align	4
.LBB1_4:                                #   Parent Loop BB1_2 Depth=1
                                        # =>  This Inner Loop Header: Depth=2
	cmpb	%r11b, (%rdi,%r10)
	jne	.LBB1_9
# %bb.5:                                #   in Loop: Header=BB1_4 Depth=2
	movzbl	1(%r9,%r10), %r11d
	incq	%r10
	testb	%r11b, %r11b
	jne	.LBB1_4
.LBB1_6:                                #   in Loop: Header=BB1_2 Depth=1
	movzbl	(%rdi,%r10), %r9d
	cmpl	$47, %r9d
	je	.LBB1_8
# %bb.7:                                #   in Loop: Header=BB1_2 Depth=1
	testl	%r9d, %r9d
	jne	.LBB1_9
.LBB1_8:
	addq	%r8, %rsi
	movl	8(%rsi), %eax
.LBB1_10:
	popq	%rbp
.LBB1_11:
	retq
.Lfunc_end1:
	.size	pg_policy_lookup, .Lfunc_end1-pg_policy_lookup
                                        # -- End function
	.ident	"clang version 20.1.8"
	.section	".note.GNU-stack","",@progbits
	.addrsig
