	.file	"demo.c"
	.text
	.globl	chacha_quarter_round            # -- Begin function chacha_quarter_round
	.p2align	4
	.type	chacha_quarter_round,@function
chacha_quarter_round:                   # @chacha_quarter_round
# %bb.0:
	pushq	%rbp
	movq	%rsp, %rbp
	subq	$32, %rsp
	movq	%rdi, -8(%rbp)
	movl	%esi, -12(%rbp)
	movl	%edx, -16(%rbp)
	movl	%ecx, -20(%rbp)
	movl	%r8d, -24(%rbp)
	movq	-8(%rbp), %rax
	movl	-16(%rbp), %ecx
                                        # kill: def $rcx killed $ecx
	movl	(%rax,%rcx,4), %edx
	movq	-8(%rbp), %rax
	movl	-12(%rbp), %ecx
                                        # kill: def $rcx killed $ecx
	addl	(%rax,%rcx,4), %edx
	movl	%edx, (%rax,%rcx,4)
	movq	-8(%rbp), %rax
	movl	-12(%rbp), %ecx
                                        # kill: def $rcx killed $ecx
	movl	(%rax,%rcx,4), %edx
	movq	-8(%rbp), %rax
	movl	-24(%rbp), %ecx
                                        # kill: def $rcx killed $ecx
	xorl	(%rax,%rcx,4), %edx
	movl	%edx, (%rax,%rcx,4)
	movq	-8(%rbp), %rax
	movl	-24(%rbp), %ecx
                                        # kill: def $rcx killed $ecx
	movl	(%rax,%rcx,4), %edi
	movl	$16, %esi
	callq	rotl32
	movl	%eax, %edx
	movq	-8(%rbp), %rax
	movl	-24(%rbp), %ecx
                                        # kill: def $rcx killed $ecx
	movl	%edx, (%rax,%rcx,4)
	movq	-8(%rbp), %rax
	movl	-24(%rbp), %ecx
                                        # kill: def $rcx killed $ecx
	movl	(%rax,%rcx,4), %edx
	movq	-8(%rbp), %rax
	movl	-20(%rbp), %ecx
                                        # kill: def $rcx killed $ecx
	addl	(%rax,%rcx,4), %edx
	movl	%edx, (%rax,%rcx,4)
	movq	-8(%rbp), %rax
	movl	-20(%rbp), %ecx
                                        # kill: def $rcx killed $ecx
	movl	(%rax,%rcx,4), %edx
	movq	-8(%rbp), %rax
	movl	-16(%rbp), %ecx
                                        # kill: def $rcx killed $ecx
	xorl	(%rax,%rcx,4), %edx
	movl	%edx, (%rax,%rcx,4)
	movq	-8(%rbp), %rax
	movl	-16(%rbp), %ecx
                                        # kill: def $rcx killed $ecx
	movl	(%rax,%rcx,4), %edi
	movl	$12, %esi
	callq	rotl32
	movl	%eax, %edx
	movq	-8(%rbp), %rax
	movl	-16(%rbp), %ecx
                                        # kill: def $rcx killed $ecx
	movl	%edx, (%rax,%rcx,4)
	movq	-8(%rbp), %rax
	movl	-16(%rbp), %ecx
                                        # kill: def $rcx killed $ecx
	movl	(%rax,%rcx,4), %edx
	movq	-8(%rbp), %rax
	movl	-12(%rbp), %ecx
                                        # kill: def $rcx killed $ecx
	addl	(%rax,%rcx,4), %edx
	movl	%edx, (%rax,%rcx,4)
	movq	-8(%rbp), %rax
	movl	-12(%rbp), %ecx
                                        # kill: def $rcx killed $ecx
	movl	(%rax,%rcx,4), %edx
	movq	-8(%rbp), %rax
	movl	-24(%rbp), %ecx
                                        # kill: def $rcx killed $ecx
	xorl	(%rax,%rcx,4), %edx
	movl	%edx, (%rax,%rcx,4)
	movq	-8(%rbp), %rax
	movl	-24(%rbp), %ecx
                                        # kill: def $rcx killed $ecx
	movl	(%rax,%rcx,4), %edi
	movl	$8, %esi
	callq	rotl32
	movl	%eax, %edx
	movq	-8(%rbp), %rax
	movl	-24(%rbp), %ecx
                                        # kill: def $rcx killed $ecx
	movl	%edx, (%rax,%rcx,4)
	movq	-8(%rbp), %rax
	movl	-24(%rbp), %ecx
                                        # kill: def $rcx killed $ecx
	movl	(%rax,%rcx,4), %edx
	movq	-8(%rbp), %rax
	movl	-20(%rbp), %ecx
                                        # kill: def $rcx killed $ecx
	addl	(%rax,%rcx,4), %edx
	movl	%edx, (%rax,%rcx,4)
	movq	-8(%rbp), %rax
	movl	-20(%rbp), %ecx
                                        # kill: def $rcx killed $ecx
	movl	(%rax,%rcx,4), %edx
	movq	-8(%rbp), %rax
	movl	-16(%rbp), %ecx
                                        # kill: def $rcx killed $ecx
	xorl	(%rax,%rcx,4), %edx
	movl	%edx, (%rax,%rcx,4)
	movq	-8(%rbp), %rax
	movl	-16(%rbp), %ecx
                                        # kill: def $rcx killed $ecx
	movl	(%rax,%rcx,4), %edi
	movl	$7, %esi
	callq	rotl32
	movl	%eax, %edx
	movq	-8(%rbp), %rax
	movl	-16(%rbp), %ecx
                                        # kill: def $rcx killed $ecx
	movl	%edx, (%rax,%rcx,4)
	addq	$32, %rsp
	popq	%rbp
	retq
.Lfunc_end0:
	.size	chacha_quarter_round, .Lfunc_end0-chacha_quarter_round
                                        # -- End function
	.p2align	4                               # -- Begin function rotl32
	.type	rotl32,@function
rotl32:                                 # @rotl32
# %bb.0:
	pushq	%rbp
	movq	%rsp, %rbp
	movl	%edi, -4(%rbp)
	movl	%esi, -8(%rbp)
	movl	-4(%rbp), %eax
	movl	-8(%rbp), %ecx
                                        # kill: def $cl killed $ecx
	shll	%cl, %eax
	movl	-4(%rbp), %edx
	movl	$32, %ecx
	subl	-8(%rbp), %ecx
                                        # kill: def $cl killed $ecx
	shrl	%cl, %edx
	movl	%edx, %ecx
	orl	%ecx, %eax
	popq	%rbp
	retq
.Lfunc_end1:
	.size	rotl32, .Lfunc_end1-rotl32
                                        # -- End function
	.globl	chacha20_block                  # -- Begin function chacha20_block
	.p2align	4
	.type	chacha20_block,@function
chacha20_block:                         # @chacha20_block
# %bb.0:
	pushq	%rbp
	movq	%rsp, %rbp
	subq	$176, %rsp
	movq	%rdi, -8(%rbp)
	movq	%rsi, -16(%rbp)
	movl	%edx, -20(%rbp)
	movq	%rcx, -32(%rbp)
	movl	$1634760805, -96(%rbp)          # imm = 0x61707865
	movl	$857760878, -92(%rbp)           # imm = 0x3320646E
	movl	$2036477234, -88(%rbp)          # imm = 0x79622D32
	movl	$1797285236, -84(%rbp)          # imm = 0x6B206574
	movl	$0, -164(%rbp)
.LBB2_1:                                # =>This Inner Loop Header: Depth=1
	cmpl	$8, -164(%rbp)
	jge	.LBB2_4
# %bb.2:                                #   in Loop: Header=BB2_1 Depth=1
	movq	-16(%rbp), %rdi
	movl	-164(%rbp), %eax
	shll	$2, %eax
	cltq
	addq	%rax, %rdi
	callq	load_le32
	movl	%eax, %ecx
	movl	-164(%rbp), %eax
	addl	$4, %eax
	cltq
	movl	%ecx, -96(%rbp,%rax,4)
# %bb.3:                                #   in Loop: Header=BB2_1 Depth=1
	movl	-164(%rbp), %eax
	addl	$1, %eax
	movl	%eax, -164(%rbp)
	jmp	.LBB2_1
.LBB2_4:
	movl	-20(%rbp), %eax
	movl	%eax, -48(%rbp)
	movq	-32(%rbp), %rdi
	callq	load_le32
	movl	%eax, -44(%rbp)
	movq	-32(%rbp), %rdi
	addq	$4, %rdi
	callq	load_le32
	movl	%eax, -40(%rbp)
	movq	-32(%rbp), %rdi
	addq	$8, %rdi
	callq	load_le32
	movl	%eax, -36(%rbp)
	movl	$0, -168(%rbp)
.LBB2_5:                                # =>This Inner Loop Header: Depth=1
	cmpl	$16, -168(%rbp)
	jge	.LBB2_8
# %bb.6:                                #   in Loop: Header=BB2_5 Depth=1
	movslq	-168(%rbp), %rax
	movl	-96(%rbp,%rax,4), %ecx
	movslq	-168(%rbp), %rax
	movl	%ecx, -160(%rbp,%rax,4)
# %bb.7:                                #   in Loop: Header=BB2_5 Depth=1
	movl	-168(%rbp), %eax
	addl	$1, %eax
	movl	%eax, -168(%rbp)
	jmp	.LBB2_5
.LBB2_8:
	movl	$0, -172(%rbp)
.LBB2_9:                                # =>This Inner Loop Header: Depth=1
	cmpl	$10, -172(%rbp)
	jge	.LBB2_28
# %bb.10:                               #   in Loop: Header=BB2_9 Depth=1
	jmp	.LBB2_11
.LBB2_11:                               #   in Loop: Header=BB2_9 Depth=1
	movl	-144(%rbp), %eax
	addl	-160(%rbp), %eax
	movl	%eax, -160(%rbp)
	movl	-160(%rbp), %eax
	xorl	-112(%rbp), %eax
	movl	%eax, -112(%rbp)
	movl	-112(%rbp), %edi
	movl	$16, %esi
	callq	rotl32
	movl	%eax, -112(%rbp)
	movl	-112(%rbp), %eax
	addl	-128(%rbp), %eax
	movl	%eax, -128(%rbp)
	movl	-128(%rbp), %eax
	xorl	-144(%rbp), %eax
	movl	%eax, -144(%rbp)
	movl	-144(%rbp), %edi
	movl	$12, %esi
	callq	rotl32
	movl	%eax, -144(%rbp)
	movl	-144(%rbp), %eax
	addl	-160(%rbp), %eax
	movl	%eax, -160(%rbp)
	movl	-160(%rbp), %eax
	xorl	-112(%rbp), %eax
	movl	%eax, -112(%rbp)
	movl	-112(%rbp), %edi
	movl	$8, %esi
	callq	rotl32
	movl	%eax, -112(%rbp)
	movl	-112(%rbp), %eax
	addl	-128(%rbp), %eax
	movl	%eax, -128(%rbp)
	movl	-128(%rbp), %eax
	xorl	-144(%rbp), %eax
	movl	%eax, -144(%rbp)
	movl	-144(%rbp), %edi
	movl	$7, %esi
	callq	rotl32
	movl	%eax, -144(%rbp)
# %bb.12:                               #   in Loop: Header=BB2_9 Depth=1
	jmp	.LBB2_13
.LBB2_13:                               #   in Loop: Header=BB2_9 Depth=1
	movl	-140(%rbp), %eax
	addl	-156(%rbp), %eax
	movl	%eax, -156(%rbp)
	movl	-156(%rbp), %eax
	xorl	-108(%rbp), %eax
	movl	%eax, -108(%rbp)
	movl	-108(%rbp), %edi
	movl	$16, %esi
	callq	rotl32
	movl	%eax, -108(%rbp)
	movl	-108(%rbp), %eax
	addl	-124(%rbp), %eax
	movl	%eax, -124(%rbp)
	movl	-124(%rbp), %eax
	xorl	-140(%rbp), %eax
	movl	%eax, -140(%rbp)
	movl	-140(%rbp), %edi
	movl	$12, %esi
	callq	rotl32
	movl	%eax, -140(%rbp)
	movl	-140(%rbp), %eax
	addl	-156(%rbp), %eax
	movl	%eax, -156(%rbp)
	movl	-156(%rbp), %eax
	xorl	-108(%rbp), %eax
	movl	%eax, -108(%rbp)
	movl	-108(%rbp), %edi
	movl	$8, %esi
	callq	rotl32
	movl	%eax, -108(%rbp)
	movl	-108(%rbp), %eax
	addl	-124(%rbp), %eax
	movl	%eax, -124(%rbp)
	movl	-124(%rbp), %eax
	xorl	-140(%rbp), %eax
	movl	%eax, -140(%rbp)
	movl	-140(%rbp), %edi
	movl	$7, %esi
	callq	rotl32
	movl	%eax, -140(%rbp)
# %bb.14:                               #   in Loop: Header=BB2_9 Depth=1
	jmp	.LBB2_15
.LBB2_15:                               #   in Loop: Header=BB2_9 Depth=1
	movl	-136(%rbp), %eax
	addl	-152(%rbp), %eax
	movl	%eax, -152(%rbp)
	movl	-152(%rbp), %eax
	xorl	-104(%rbp), %eax
	movl	%eax, -104(%rbp)
	movl	-104(%rbp), %edi
	movl	$16, %esi
	callq	rotl32
	movl	%eax, -104(%rbp)
	movl	-104(%rbp), %eax
	addl	-120(%rbp), %eax
	movl	%eax, -120(%rbp)
	movl	-120(%rbp), %eax
	xorl	-136(%rbp), %eax
	movl	%eax, -136(%rbp)
	movl	-136(%rbp), %edi
	movl	$12, %esi
	callq	rotl32
	movl	%eax, -136(%rbp)
	movl	-136(%rbp), %eax
	addl	-152(%rbp), %eax
	movl	%eax, -152(%rbp)
	movl	-152(%rbp), %eax
	xorl	-104(%rbp), %eax
	movl	%eax, -104(%rbp)
	movl	-104(%rbp), %edi
	movl	$8, %esi
	callq	rotl32
	movl	%eax, -104(%rbp)
	movl	-104(%rbp), %eax
	addl	-120(%rbp), %eax
	movl	%eax, -120(%rbp)
	movl	-120(%rbp), %eax
	xorl	-136(%rbp), %eax
	movl	%eax, -136(%rbp)
	movl	-136(%rbp), %edi
	movl	$7, %esi
	callq	rotl32
	movl	%eax, -136(%rbp)
# %bb.16:                               #   in Loop: Header=BB2_9 Depth=1
	jmp	.LBB2_17
.LBB2_17:                               #   in Loop: Header=BB2_9 Depth=1
	movl	-132(%rbp), %eax
	addl	-148(%rbp), %eax
	movl	%eax, -148(%rbp)
	movl	-148(%rbp), %eax
	xorl	-100(%rbp), %eax
	movl	%eax, -100(%rbp)
	movl	-100(%rbp), %edi
	movl	$16, %esi
	callq	rotl32
	movl	%eax, -100(%rbp)
	movl	-100(%rbp), %eax
	addl	-116(%rbp), %eax
	movl	%eax, -116(%rbp)
	movl	-116(%rbp), %eax
	xorl	-132(%rbp), %eax
	movl	%eax, -132(%rbp)
	movl	-132(%rbp), %edi
	movl	$12, %esi
	callq	rotl32
	movl	%eax, -132(%rbp)
	movl	-132(%rbp), %eax
	addl	-148(%rbp), %eax
	movl	%eax, -148(%rbp)
	movl	-148(%rbp), %eax
	xorl	-100(%rbp), %eax
	movl	%eax, -100(%rbp)
	movl	-100(%rbp), %edi
	movl	$8, %esi
	callq	rotl32
	movl	%eax, -100(%rbp)
	movl	-100(%rbp), %eax
	addl	-116(%rbp), %eax
	movl	%eax, -116(%rbp)
	movl	-116(%rbp), %eax
	xorl	-132(%rbp), %eax
	movl	%eax, -132(%rbp)
	movl	-132(%rbp), %edi
	movl	$7, %esi
	callq	rotl32
	movl	%eax, -132(%rbp)
# %bb.18:                               #   in Loop: Header=BB2_9 Depth=1
	jmp	.LBB2_19
.LBB2_19:                               #   in Loop: Header=BB2_9 Depth=1
	movl	-140(%rbp), %eax
	addl	-160(%rbp), %eax
	movl	%eax, -160(%rbp)
	movl	-160(%rbp), %eax
	xorl	-100(%rbp), %eax
	movl	%eax, -100(%rbp)
	movl	-100(%rbp), %edi
	movl	$16, %esi
	callq	rotl32
	movl	%eax, -100(%rbp)
	movl	-100(%rbp), %eax
	addl	-120(%rbp), %eax
	movl	%eax, -120(%rbp)
	movl	-120(%rbp), %eax
	xorl	-140(%rbp), %eax
	movl	%eax, -140(%rbp)
	movl	-140(%rbp), %edi
	movl	$12, %esi
	callq	rotl32
	movl	%eax, -140(%rbp)
	movl	-140(%rbp), %eax
	addl	-160(%rbp), %eax
	movl	%eax, -160(%rbp)
	movl	-160(%rbp), %eax
	xorl	-100(%rbp), %eax
	movl	%eax, -100(%rbp)
	movl	-100(%rbp), %edi
	movl	$8, %esi
	callq	rotl32
	movl	%eax, -100(%rbp)
	movl	-100(%rbp), %eax
	addl	-120(%rbp), %eax
	movl	%eax, -120(%rbp)
	movl	-120(%rbp), %eax
	xorl	-140(%rbp), %eax
	movl	%eax, -140(%rbp)
	movl	-140(%rbp), %edi
	movl	$7, %esi
	callq	rotl32
	movl	%eax, -140(%rbp)
# %bb.20:                               #   in Loop: Header=BB2_9 Depth=1
	jmp	.LBB2_21
.LBB2_21:                               #   in Loop: Header=BB2_9 Depth=1
	movl	-136(%rbp), %eax
	addl	-156(%rbp), %eax
	movl	%eax, -156(%rbp)
	movl	-156(%rbp), %eax
	xorl	-112(%rbp), %eax
	movl	%eax, -112(%rbp)
	movl	-112(%rbp), %edi
	movl	$16, %esi
	callq	rotl32
	movl	%eax, -112(%rbp)
	movl	-112(%rbp), %eax
	addl	-116(%rbp), %eax
	movl	%eax, -116(%rbp)
	movl	-116(%rbp), %eax
	xorl	-136(%rbp), %eax
	movl	%eax, -136(%rbp)
	movl	-136(%rbp), %edi
	movl	$12, %esi
	callq	rotl32
	movl	%eax, -136(%rbp)
	movl	-136(%rbp), %eax
	addl	-156(%rbp), %eax
	movl	%eax, -156(%rbp)
	movl	-156(%rbp), %eax
	xorl	-112(%rbp), %eax
	movl	%eax, -112(%rbp)
	movl	-112(%rbp), %edi
	movl	$8, %esi
	callq	rotl32
	movl	%eax, -112(%rbp)
	movl	-112(%rbp), %eax
	addl	-116(%rbp), %eax
	movl	%eax, -116(%rbp)
	movl	-116(%rbp), %eax
	xorl	-136(%rbp), %eax
	movl	%eax, -136(%rbp)
	movl	-136(%rbp), %edi
	movl	$7, %esi
	callq	rotl32
	movl	%eax, -136(%rbp)
# %bb.22:                               #   in Loop: Header=BB2_9 Depth=1
	jmp	.LBB2_23
.LBB2_23:                               #   in Loop: Header=BB2_9 Depth=1
	movl	-132(%rbp), %eax
	addl	-152(%rbp), %eax
	movl	%eax, -152(%rbp)
	movl	-152(%rbp), %eax
	xorl	-108(%rbp), %eax
	movl	%eax, -108(%rbp)
	movl	-108(%rbp), %edi
	movl	$16, %esi
	callq	rotl32
	movl	%eax, -108(%rbp)
	movl	-108(%rbp), %eax
	addl	-128(%rbp), %eax
	movl	%eax, -128(%rbp)
	movl	-128(%rbp), %eax
	xorl	-132(%rbp), %eax
	movl	%eax, -132(%rbp)
	movl	-132(%rbp), %edi
	movl	$12, %esi
	callq	rotl32
	movl	%eax, -132(%rbp)
	movl	-132(%rbp), %eax
	addl	-152(%rbp), %eax
	movl	%eax, -152(%rbp)
	movl	-152(%rbp), %eax
	xorl	-108(%rbp), %eax
	movl	%eax, -108(%rbp)
	movl	-108(%rbp), %edi
	movl	$8, %esi
	callq	rotl32
	movl	%eax, -108(%rbp)
	movl	-108(%rbp), %eax
	addl	-128(%rbp), %eax
	movl	%eax, -128(%rbp)
	movl	-128(%rbp), %eax
	xorl	-132(%rbp), %eax
	movl	%eax, -132(%rbp)
	movl	-132(%rbp), %edi
	movl	$7, %esi
	callq	rotl32
	movl	%eax, -132(%rbp)
# %bb.24:                               #   in Loop: Header=BB2_9 Depth=1
	jmp	.LBB2_25
.LBB2_25:                               #   in Loop: Header=BB2_9 Depth=1
	movl	-144(%rbp), %eax
	addl	-148(%rbp), %eax
	movl	%eax, -148(%rbp)
	movl	-148(%rbp), %eax
	xorl	-104(%rbp), %eax
	movl	%eax, -104(%rbp)
	movl	-104(%rbp), %edi
	movl	$16, %esi
	callq	rotl32
	movl	%eax, -104(%rbp)
	movl	-104(%rbp), %eax
	addl	-124(%rbp), %eax
	movl	%eax, -124(%rbp)
	movl	-124(%rbp), %eax
	xorl	-144(%rbp), %eax
	movl	%eax, -144(%rbp)
	movl	-144(%rbp), %edi
	movl	$12, %esi
	callq	rotl32
	movl	%eax, -144(%rbp)
	movl	-144(%rbp), %eax
	addl	-148(%rbp), %eax
	movl	%eax, -148(%rbp)
	movl	-148(%rbp), %eax
	xorl	-104(%rbp), %eax
	movl	%eax, -104(%rbp)
	movl	-104(%rbp), %edi
	movl	$8, %esi
	callq	rotl32
	movl	%eax, -104(%rbp)
	movl	-104(%rbp), %eax
	addl	-124(%rbp), %eax
	movl	%eax, -124(%rbp)
	movl	-124(%rbp), %eax
	xorl	-144(%rbp), %eax
	movl	%eax, -144(%rbp)
	movl	-144(%rbp), %edi
	movl	$7, %esi
	callq	rotl32
	movl	%eax, -144(%rbp)
# %bb.26:                               #   in Loop: Header=BB2_9 Depth=1
	jmp	.LBB2_27
.LBB2_27:                               #   in Loop: Header=BB2_9 Depth=1
	movl	-172(%rbp), %eax
	addl	$1, %eax
	movl	%eax, -172(%rbp)
	jmp	.LBB2_9
.LBB2_28:
	movl	$0, -176(%rbp)
.LBB2_29:                               # =>This Inner Loop Header: Depth=1
	cmpl	$16, -176(%rbp)
	jge	.LBB2_32
# %bb.30:                               #   in Loop: Header=BB2_29 Depth=1
	movq	-8(%rbp), %rdi
	movl	-176(%rbp), %eax
	shll	$2, %eax
	cltq
	addq	%rax, %rdi
	movslq	-176(%rbp), %rax
	movl	-160(%rbp,%rax,4), %esi
	movslq	-176(%rbp), %rax
	addl	-96(%rbp,%rax,4), %esi
	callq	store_le32
# %bb.31:                               #   in Loop: Header=BB2_29 Depth=1
	movl	-176(%rbp), %eax
	addl	$1, %eax
	movl	%eax, -176(%rbp)
	jmp	.LBB2_29
.LBB2_32:
	addq	$176, %rsp
	popq	%rbp
	retq
.Lfunc_end2:
	.size	chacha20_block, .Lfunc_end2-chacha20_block
                                        # -- End function
	.p2align	4                               # -- Begin function load_le32
	.type	load_le32,@function
load_le32:                              # @load_le32
# %bb.0:
	pushq	%rbp
	movq	%rsp, %rbp
	movq	%rdi, -8(%rbp)
	movq	-8(%rbp), %rax
	movzbl	(%rax), %eax
	movq	-8(%rbp), %rcx
	movzbl	1(%rcx), %ecx
	shll	$8, %ecx
	orl	%ecx, %eax
	movq	-8(%rbp), %rcx
	movzbl	2(%rcx), %ecx
	shll	$16, %ecx
	orl	%ecx, %eax
	movq	-8(%rbp), %rcx
	movzbl	3(%rcx), %ecx
	shll	$24, %ecx
	orl	%ecx, %eax
	popq	%rbp
	retq
.Lfunc_end3:
	.size	load_le32, .Lfunc_end3-load_le32
                                        # -- End function
	.p2align	4                               # -- Begin function store_le32
	.type	store_le32,@function
store_le32:                             # @store_le32
# %bb.0:
	pushq	%rbp
	movq	%rsp, %rbp
	movq	%rdi, -8(%rbp)
	movl	%esi, -12(%rbp)
	movl	-12(%rbp), %eax
	movb	%al, %cl
	movq	-8(%rbp), %rax
	movb	%cl, (%rax)
	movl	-12(%rbp), %eax
	shrl	$8, %eax
	movb	%al, %cl
	movq	-8(%rbp), %rax
	movb	%cl, 1(%rax)
	movl	-12(%rbp), %eax
	shrl	$16, %eax
	movb	%al, %cl
	movq	-8(%rbp), %rax
	movb	%cl, 2(%rax)
	movl	-12(%rbp), %eax
	shrl	$24, %eax
	movb	%al, %cl
	movq	-8(%rbp), %rax
	movb	%cl, 3(%rax)
	popq	%rbp
	retq
.Lfunc_end4:
	.size	store_le32, .Lfunc_end4-store_le32
                                        # -- End function
	.ident	"clang version 20.1.8"
	.section	".note.GNU-stack","",@progbits
	.addrsig
	.addrsig_sym rotl32
	.addrsig_sym load_le32
	.addrsig_sym store_le32
