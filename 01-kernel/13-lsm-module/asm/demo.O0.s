	.file	"demo.c"
	.text
	.globl	pg_path_has_prefix              # -- Begin function pg_path_has_prefix
	.p2align	4
	.type	pg_path_has_prefix,@function
pg_path_has_prefix:                     # @pg_path_has_prefix
# %bb.0:
	pushq	%rbp
	movq	%rsp, %rbp
	movq	%rdi, -16(%rbp)
	movq	%rsi, -24(%rbp)
	movq	$0, -32(%rbp)
.LBB0_1:                                # =>This Inner Loop Header: Depth=1
	movq	-24(%rbp), %rax
	movq	-32(%rbp), %rcx
	movsbl	(%rax,%rcx), %eax
	cmpl	$0, %eax
	je	.LBB0_5
# %bb.2:                                #   in Loop: Header=BB0_1 Depth=1
	movq	-16(%rbp), %rax
	movq	-32(%rbp), %rcx
	movsbl	(%rax,%rcx), %eax
	movq	-24(%rbp), %rcx
	movq	-32(%rbp), %rdx
	movsbl	(%rcx,%rdx), %ecx
	cmpl	%ecx, %eax
	je	.LBB0_4
# %bb.3:
	movl	$0, -4(%rbp)
	jmp	.LBB0_9
.LBB0_4:                                #   in Loop: Header=BB0_1 Depth=1
	movq	-32(%rbp), %rax
	addq	$1, %rax
	movq	%rax, -32(%rbp)
	jmp	.LBB0_1
.LBB0_5:
	movq	-16(%rbp), %rax
	movq	-32(%rbp), %rcx
	movsbl	(%rax,%rcx), %eax
	cmpl	$0, %eax
	je	.LBB0_7
# %bb.6:
	movq	-16(%rbp), %rax
	movq	-32(%rbp), %rcx
	movsbl	(%rax,%rcx), %eax
	cmpl	$47, %eax
	jne	.LBB0_8
.LBB0_7:
	movl	$1, -4(%rbp)
	jmp	.LBB0_9
.LBB0_8:
	movl	$0, -4(%rbp)
.LBB0_9:
	movl	-4(%rbp), %eax
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
	pushq	%rbp
	movq	%rsp, %rbp
	subq	$48, %rsp
	movq	%rdi, -16(%rbp)
	movq	%rsi, -24(%rbp)
	movl	%edx, -28(%rbp)
	movl	%ecx, -32(%rbp)
	movl	$0, -36(%rbp)
.LBB1_1:                                # =>This Inner Loop Header: Depth=1
	movl	-36(%rbp), %eax
	cmpl	-28(%rbp), %eax
	jge	.LBB1_6
# %bb.2:                                #   in Loop: Header=BB1_1 Depth=1
	movq	-16(%rbp), %rdi
	movq	-24(%rbp), %rax
	movslq	-36(%rbp), %rcx
	shlq	$4, %rcx
	addq	%rcx, %rax
	movq	(%rax), %rsi
	callq	pg_path_has_prefix
	cmpl	$0, %eax
	je	.LBB1_4
# %bb.3:
	movq	-24(%rbp), %rax
	movslq	-36(%rbp), %rcx
	shlq	$4, %rcx
	addq	%rcx, %rax
	movl	8(%rax), %eax
	movl	%eax, -4(%rbp)
	jmp	.LBB1_7
.LBB1_4:                                #   in Loop: Header=BB1_1 Depth=1
	jmp	.LBB1_5
.LBB1_5:                                #   in Loop: Header=BB1_1 Depth=1
	movl	-36(%rbp), %eax
	addl	$1, %eax
	movl	%eax, -36(%rbp)
	jmp	.LBB1_1
.LBB1_6:
	movl	-32(%rbp), %eax
	movl	%eax, -4(%rbp)
.LBB1_7:
	movl	-4(%rbp), %eax
	addq	$48, %rsp
	popq	%rbp
	retq
.Lfunc_end1:
	.size	pg_policy_lookup, .Lfunc_end1-pg_policy_lookup
                                        # -- End function
	.ident	"clang version 20.1.8"
	.section	".note.GNU-stack","",@progbits
	.addrsig
	.addrsig_sym pg_path_has_prefix
