	.file	"demo.c"
	.text
	.globl	log_up_to_date                  # -- Begin function log_up_to_date
	.p2align	4
	.type	log_up_to_date,@function
log_up_to_date:                         # @log_up_to_date
# %bb.0:
	pushq	%rbp
	movq	%rsp, %rbp
	xorl	%eax, %eax
	cmpq	%rcx, %rsi
	setae	%al
	xorl	%ecx, %ecx
	cmpq	%rdx, %rdi
	seta	%cl
	cmovel	%eax, %ecx
	movzbl	%cl, %eax
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
	testl	%esi, %esi
	jle	.LBB1_1
# %bb.2:
	pushq	%rbp
	movq	%rsp, %rbp
	pushq	%r14
	pushq	%rbx
	subq	$128, %rsp
	movq	%rdi, %rax
	cmpl	$16, %esi
	movl	$16, %ebx
	cmovll	%esi, %ebx
	leal	-1(%rbx), %ecx
	leaq	8(,%rcx,8), %rdx
	leaq	-144(%rbp), %rdi
	movl	%esi, %r14d
	movq	%rax, %rsi
	callq	memcpy@PLT
	cmpl	$1, %r14d
	jne	.LBB1_3
.LBB1_9:
	movl	%ebx, %eax
	shrl	%eax
	notl	%eax
	addl	%ebx, %eax
	cltq
	movq	-144(%rbp,%rax,8), %rax
	addq	$128, %rsp
	popq	%rbx
	popq	%r14
	popq	%rbp
	retq
.LBB1_1:
	xorl	%eax, %eax
	retq
.LBB1_3:
	cmpl	$3, %ebx
	movl	$2, %eax
	cmovgel	%ebx, %eax
	movl	$1, %ecx
	jmp	.LBB1_4
	.p2align	4
.LBB1_7:                                #   in Loop: Header=BB1_4 Depth=1
	xorl	%esi, %esi
.LBB1_8:                                #   in Loop: Header=BB1_4 Depth=1
	movslq	%esi, %rsi
	movq	%rdx, -144(%rbp,%rsi,8)
	incq	%rcx
	cmpq	%rax, %rcx
	je	.LBB1_9
.LBB1_4:                                # =>This Loop Header: Depth=1
                                        #     Child Loop BB1_5 Depth 2
	movq	-144(%rbp,%rcx,8), %rdx
	movq	%rcx, %rsi
	.p2align	4
.LBB1_5:                                #   Parent Loop BB1_4 Depth=1
                                        # =>  This Inner Loop Header: Depth=2
	movq	-152(%rbp,%rsi,8), %rdi
	cmpq	%rdx, %rdi
	jbe	.LBB1_8
# %bb.6:                                #   in Loop: Header=BB1_5 Depth=2
	movq	%rdi, -144(%rbp,%rsi,8)
	decq	%rsi
	leaq	1(%rsi), %rdi
	cmpq	$1, %rdi
	jg	.LBB1_5
	jmp	.LBB1_7
.Lfunc_end1:
	.size	majority_match_index, .Lfunc_end1-majority_match_index
                                        # -- End function
	.ident	"clang version 20.1.8"
	.section	".note.GNU-stack","",@progbits
	.addrsig
