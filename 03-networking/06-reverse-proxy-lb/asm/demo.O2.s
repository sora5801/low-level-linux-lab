	.file	"demo.c"
	.text
	.globl	fnv1a_32                        # -- Begin function fnv1a_32
	.p2align	4
	.type	fnv1a_32,@function
fnv1a_32:                               # @fnv1a_32
# %bb.0:
	testq	%rsi, %rsi
	je	.LBB0_1
# %bb.2:
	movl	%esi, %ecx
	andl	$3, %ecx
	cmpq	$4, %rsi
	jae	.LBB0_8
# %bb.3:
	movl	$-2128831035, %eax              # imm = 0x811C9DC5
	xorl	%edx, %edx
	jmp	.LBB0_4
.LBB0_1:
	movl	$-2128831035, %eax              # imm = 0x811C9DC5
	retq
.LBB0_8:
	andq	$-4, %rsi
	movl	$-2128831035, %eax              # imm = 0x811C9DC5
	xorl	%edx, %edx
	.p2align	4
.LBB0_9:                                # =>This Inner Loop Header: Depth=1
	movzbl	(%rdi,%rdx), %r8d
	xorl	%eax, %r8d
	imull	$16777619, %r8d, %eax           # imm = 0x1000193
	movzbl	1(%rdi,%rdx), %r8d
	xorl	%eax, %r8d
	imull	$16777619, %r8d, %eax           # imm = 0x1000193
	movzbl	2(%rdi,%rdx), %r8d
	xorl	%eax, %r8d
	imull	$16777619, %r8d, %eax           # imm = 0x1000193
	movzbl	3(%rdi,%rdx), %r8d
	xorl	%eax, %r8d
	imull	$16777619, %r8d, %eax           # imm = 0x1000193
	addq	$4, %rdx
	cmpq	%rdx, %rsi
	jne	.LBB0_9
.LBB0_4:
	testq	%rcx, %rcx
	je	.LBB0_7
# %bb.5:
	addq	%rdx, %rdi
	xorl	%edx, %edx
	.p2align	4
.LBB0_6:                                # =>This Inner Loop Header: Depth=1
	movzbl	(%rdi,%rdx), %esi
	xorl	%eax, %esi
	imull	$16777619, %esi, %eax           # imm = 0x1000193
	incq	%rdx
	cmpq	%rdx, %rcx
	jne	.LBB0_6
.LBB0_7:
	retq
.Lfunc_end0:
	.size	fnv1a_32, .Lfunc_end0-fnv1a_32
                                        # -- End function
	.globl	ring_lookup                     # -- Begin function ring_lookup
	.p2align	4
	.type	ring_lookup,@function
ring_lookup:                            # @ring_lookup
# %bb.0:
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
	retq
.LBB1_1:
	movl	$-1, %eax
	retq
.Lfunc_end1:
	.size	ring_lookup, .Lfunc_end1-ring_lookup
                                        # -- End function
	.globl	least_conn_pick                 # -- Begin function least_conn_pick
	.p2align	4
	.type	least_conn_pick,@function
least_conn_pick:                        # @least_conn_pick
# %bb.0:
	testl	%edx, %edx
	jle	.LBB2_1
# %bb.2:
	movl	%edx, %r8d
	cmpl	$1, %edx
	jne	.LBB2_8
# %bb.3:
	movl	$-1, %eax
	xorl	%ecx, %ecx
	xorl	%r9d, %r9d
	testb	$1, %r8b
	jne	.LBB2_5
.LBB2_7:
	retq
.LBB2_1:
	movl	$-1, %eax
	retq
.LBB2_8:
	pushq	%rbx
	movl	%r8d, %edx
	andl	$2147483646, %edx               # imm = 0x7FFFFFFE
	movl	$-1, %eax
	xorl	%ecx, %ecx
	xorl	%r9d, %r9d
	jmp	.LBB2_9
	.p2align	4
.LBB2_15:                               #   in Loop: Header=BB2_9 Depth=1
	addq	$2, %rcx
	cmpq	%rcx, %rdx
	je	.LBB2_16
.LBB2_9:                                # =>This Inner Loop Header: Depth=1
	cmpb	$0, (%rsi,%rcx)
	je	.LBB2_12
# %bb.10:                               #   in Loop: Header=BB2_9 Depth=1
	testl	%eax, %eax
	sets	%r11b
	movl	(%rdi,%rcx,4), %r10d
	cmpl	%r9d, %r10d
	setl	%bl
	orb	%r11b, %bl
	je	.LBB2_12
# %bb.11:                               #   in Loop: Header=BB2_9 Depth=1
	movl	%ecx, %eax
	movl	%r10d, %r9d
.LBB2_12:                               #   in Loop: Header=BB2_9 Depth=1
	cmpb	$0, 1(%rsi,%rcx)
	je	.LBB2_15
# %bb.13:                               #   in Loop: Header=BB2_9 Depth=1
	testl	%eax, %eax
	sets	%r11b
	movl	4(%rdi,%rcx,4), %r10d
	cmpl	%r9d, %r10d
	setl	%bl
	orb	%r11b, %bl
	je	.LBB2_15
# %bb.14:                               #   in Loop: Header=BB2_9 Depth=1
	leal	1(%rcx), %eax
	movl	%r10d, %r9d
	jmp	.LBB2_15
.LBB2_16:
	popq	%rbx
	testb	$1, %r8b
	je	.LBB2_7
.LBB2_5:
	cmpb	$0, (%rsi,%rcx)
	je	.LBB2_7
# %bb.6:
	cmpl	%r9d, (%rdi,%rcx,4)
	movl	%eax, %edx
	cmovll	%ecx, %edx
	testl	%eax, %eax
	cmovsl	%ecx, %edx
	movl	%edx, %eax
	retq
.Lfunc_end2:
	.size	least_conn_pick, .Lfunc_end2-least_conn_pick
                                        # -- End function
	.ident	"clang version 20.1.8"
	.section	".note.GNU-stack","",@progbits
	.addrsig
