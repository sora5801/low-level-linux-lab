	.file	"demo.c"
	.text
	.globl	fnv1a_32                        # -- Begin function fnv1a_32
	.p2align	4
	.type	fnv1a_32,@function
fnv1a_32:                               # @fnv1a_32
# %bb.0:
	pushq	%rbp
	movq	%rsp, %rbp
	movl	$-2128831035, %eax              # imm = 0x811C9DC5
	testq	%rsi, %rsi
	je	.LBB0_3
# %bb.1:
	xorl	%ecx, %ecx
	.p2align	4
.LBB0_2:                                # =>This Inner Loop Header: Depth=1
	movzbl	(%rdi,%rcx), %edx
	xorl	%eax, %edx
	imull	$16777619, %edx, %eax           # imm = 0x1000193
	incq	%rcx
	cmpq	%rcx, %rsi
	jne	.LBB0_2
.LBB0_3:
	popq	%rbp
	retq
.Lfunc_end0:
	.size	fnv1a_32, .Lfunc_end0-fnv1a_32
                                        # -- End function
	.globl	ring_lookup                     # -- Begin function ring_lookup
	.p2align	4
	.type	ring_lookup,@function
ring_lookup:                            # @ring_lookup
# %bb.0:
	pushq	%rbp
	movq	%rsp, %rbp
	testl	%esi, %esi
	jle	.LBB1_1
# %bb.2:
	xorl	%eax, %eax
	movl	%esi, %ecx
	jmp	.LBB1_3
	.p2align	4
.LBB1_5:                                #   in Loop: Header=BB1_3 Depth=1
	movl	%r9d, %ecx
	cmpl	%ecx, %eax
	jge	.LBB1_7
.LBB1_3:                                # =>This Inner Loop Header: Depth=1
	movl	%ecx, %r8d
	subl	%eax, %r8d
	sarl	%r8d
	leal	(%r8,%rax), %r9d
	movslq	%r9d, %r10
	cmpl	%edx, (%rdi,%r10,8)
	jae	.LBB1_5
# %bb.4:                                #   in Loop: Header=BB1_3 Depth=1
	addl	%r8d, %eax
	incl	%eax
	cmpl	%ecx, %eax
	jl	.LBB1_3
.LBB1_7:
	xorl	%ecx, %ecx
	cmpl	%esi, %eax
	cmovnel	%eax, %ecx
	movslq	%ecx, %rax
	movl	4(%rdi,%rax,8), %eax
	popq	%rbp
	retq
.LBB1_1:
	movl	$-1, %eax
	popq	%rbp
	retq
.Lfunc_end1:
	.size	ring_lookup, .Lfunc_end1-ring_lookup
                                        # -- End function
	.globl	least_conn_pick                 # -- Begin function least_conn_pick
	.p2align	4
	.type	least_conn_pick,@function
least_conn_pick:                        # @least_conn_pick
# %bb.0:
	pushq	%rbp
	movq	%rsp, %rbp
	testl	%edx, %edx
	jle	.LBB2_1
# %bb.3:
	movl	%edx, %ecx
	movl	$-1, %eax
	xorl	%edx, %edx
	xorl	%r8d, %r8d
	jmp	.LBB2_4
	.p2align	4
.LBB2_7:                                #   in Loop: Header=BB2_4 Depth=1
	movl	(%rdi,%rdx,4), %r8d
	movl	%edx, %eax
.LBB2_8:                                #   in Loop: Header=BB2_4 Depth=1
	incq	%rdx
	cmpq	%rdx, %rcx
	je	.LBB2_2
.LBB2_4:                                # =>This Inner Loop Header: Depth=1
	cmpb	$0, (%rsi,%rdx)
	je	.LBB2_8
# %bb.5:                                #   in Loop: Header=BB2_4 Depth=1
	testl	%eax, %eax
	js	.LBB2_7
# %bb.6:                                #   in Loop: Header=BB2_4 Depth=1
	cmpl	%r8d, (%rdi,%rdx,4)
	jl	.LBB2_7
	jmp	.LBB2_8
.LBB2_1:
	movl	$-1, %eax
.LBB2_2:
	popq	%rbp
	retq
.Lfunc_end2:
	.size	least_conn_pick, .Lfunc_end2-least_conn_pick
                                        # -- End function
	.ident	"clang version 20.1.8"
	.section	".note.GNU-stack","",@progbits
	.addrsig
