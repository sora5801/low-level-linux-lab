	.file	"demo.c"
	.text
	.globl	log_up_to_date                  # -- Begin function log_up_to_date
	.p2align	4
	.type	log_up_to_date,@function
log_up_to_date:                         # @log_up_to_date
# %bb.0:
	pushq	%rbp
	movq	%rsp, %rbp
	movq	%rdi, -16(%rbp)
	movq	%rsi, -24(%rbp)
	movq	%rdx, -32(%rbp)
	movq	%rcx, -40(%rbp)
	movq	-16(%rbp), %rax
	cmpq	-32(%rbp), %rax
	je	.LBB0_2
# %bb.1:
	movq	-16(%rbp), %rax
	cmpq	-32(%rbp), %rax
	seta	%al
	andb	$1, %al
	movzbl	%al, %eax
	movl	%eax, -4(%rbp)
	jmp	.LBB0_3
.LBB0_2:
	movq	-24(%rbp), %rax
	cmpq	-40(%rbp), %rax
	setae	%al
	andb	$1, %al
	movzbl	%al, %eax
	movl	%eax, -4(%rbp)
.LBB0_3:
	movl	-4(%rbp), %eax
	popq	%rbp
	retq
.Lfunc_end0:
	.size	log_up_to_date, .Lfunc_end0-log_up_to_date
                                        # -- End function
	.globl	majority_match_index            # -- Begin function majority_match_index
	.p2align	4
	.type	majority_match_index,@function
majority_match_index:                   # @majority_match_index
# %bb.0:
	pushq	%rbp
	movq	%rsp, %rbp
	subq	$64, %rsp
	movq	%rdi, -16(%rbp)
	movl	%esi, -20(%rbp)
	cmpl	$0, -20(%rbp)
	jg	.LBB1_2
# %bb.1:
	movq	$0, -8(%rbp)
	jmp	.LBB1_18
.LBB1_2:
	cmpl	$16, -20(%rbp)
	jle	.LBB1_4
# %bb.3:
	movl	$16, -20(%rbp)
.LBB1_4:
	movl	$0, -164(%rbp)
.LBB1_5:                                # =>This Inner Loop Header: Depth=1
	movl	-164(%rbp), %eax
	cmpl	-20(%rbp), %eax
	jge	.LBB1_8
# %bb.6:                                #   in Loop: Header=BB1_5 Depth=1
	movq	-16(%rbp), %rax
	movslq	-164(%rbp), %rcx
	movq	(%rax,%rcx,8), %rcx
	movslq	-164(%rbp), %rax
	movq	%rcx, -160(%rbp,%rax,8)
# %bb.7:                                #   in Loop: Header=BB1_5 Depth=1
	movl	-164(%rbp), %eax
	addl	$1, %eax
	movl	%eax, -164(%rbp)
	jmp	.LBB1_5
.LBB1_8:
	movl	$1, -168(%rbp)
.LBB1_9:                                # =>This Loop Header: Depth=1
                                        #     Child Loop BB1_11 Depth 2
	movl	-168(%rbp), %eax
	cmpl	-20(%rbp), %eax
	jge	.LBB1_17
# %bb.10:                               #   in Loop: Header=BB1_9 Depth=1
	movslq	-168(%rbp), %rax
	movq	-160(%rbp,%rax,8), %rax
	movq	%rax, -176(%rbp)
	movl	-168(%rbp), %eax
	subl	$1, %eax
	movl	%eax, -180(%rbp)
.LBB1_11:                               #   Parent Loop BB1_9 Depth=1
                                        # =>  This Inner Loop Header: Depth=2
	xorl	%eax, %eax
                                        # kill: def $al killed $al killed $eax
	cmpl	$0, -180(%rbp)
	movb	%al, -185(%rbp)                 # 1-byte Spill
	jl	.LBB1_13
# %bb.12:                               #   in Loop: Header=BB1_11 Depth=2
	movslq	-180(%rbp), %rax
	movq	-160(%rbp,%rax,8), %rax
	cmpq	-176(%rbp), %rax
	seta	%al
	movb	%al, -185(%rbp)                 # 1-byte Spill
.LBB1_13:                               #   in Loop: Header=BB1_11 Depth=2
	movb	-185(%rbp), %al                 # 1-byte Reload
	testb	$1, %al
	jne	.LBB1_14
	jmp	.LBB1_15
.LBB1_14:                               #   in Loop: Header=BB1_11 Depth=2
	movslq	-180(%rbp), %rax
	movq	-160(%rbp,%rax,8), %rcx
	movl	-180(%rbp), %eax
	addl	$1, %eax
	cltq
	movq	%rcx, -160(%rbp,%rax,8)
	movl	-180(%rbp), %eax
	addl	$-1, %eax
	movl	%eax, -180(%rbp)
	jmp	.LBB1_11
.LBB1_15:                               #   in Loop: Header=BB1_9 Depth=1
	movq	-176(%rbp), %rcx
	movl	-180(%rbp), %eax
	addl	$1, %eax
	cltq
	movq	%rcx, -160(%rbp,%rax,8)
# %bb.16:                               #   in Loop: Header=BB1_9 Depth=1
	movl	-168(%rbp), %eax
	addl	$1, %eax
	movl	%eax, -168(%rbp)
	jmp	.LBB1_9
.LBB1_17:
	movl	-20(%rbp), %eax
	movl	$2, %ecx
	cltd
	idivl	%ecx
	addl	$1, %eax
	movl	%eax, -184(%rbp)
	movl	-20(%rbp), %eax
	subl	-184(%rbp), %eax
	cltq
	movq	-160(%rbp,%rax,8), %rax
	movq	%rax, -8(%rbp)
.LBB1_18:
	movq	-8(%rbp), %rax
	addq	$64, %rsp
	popq	%rbp
	retq
.Lfunc_end1:
	.size	majority_match_index, .Lfunc_end1-majority_match_index
                                        # -- End function
	.ident	"clang version 20.1.8"
	.section	".note.GNU-stack","",@progbits
	.addrsig
