	.file	"demo.c"
	.text
	.globl	align_up                        # -- Begin function align_up
	.p2align	4
	.type	align_up,@function
align_up:                               # @align_up
# %bb.0:
	leaq	(%rdi,%rsi), %rax
	decq	%rax
	negq	%rsi
	andq	%rsi, %rax
	retq
.Lfunc_end0:
	.size	align_up, .Lfunc_end0-align_up
                                        # -- End function
	.globl	runq_push                       # -- Begin function runq_push
	.p2align	4
	.type	runq_push,@function
runq_push:                              # @runq_push
# %bb.0:
	movq	$0, 16(%rsi)
	movq	8(%rdi), %rax
	testq	%rax, %rax
	je	.LBB1_2
# %bb.1:
	movq	%rsi, 16(%rax)
	movq	%rsi, 8(%rdi)
	retq
.LBB1_2:
	movq	%rsi, (%rdi)
	movq	%rsi, 8(%rdi)
	retq
.Lfunc_end1:
	.size	runq_push, .Lfunc_end1-runq_push
                                        # -- End function
	.globl	runq_pop                        # -- Begin function runq_pop
	.p2align	4
	.type	runq_pop,@function
runq_pop:                               # @runq_pop
# %bb.0:
	movq	(%rdi), %rax
	testq	%rax, %rax
	je	.LBB2_4
# %bb.1:
	movq	16(%rax), %rcx
	movq	%rcx, (%rdi)
	testq	%rcx, %rcx
	jne	.LBB2_3
# %bb.2:
	movq	$0, 8(%rdi)
.LBB2_3:
	movq	$0, 16(%rax)
.LBB2_4:
	retq
.Lfunc_end2:
	.size	runq_pop, .Lfunc_end2-runq_pop
                                        # -- End function
	.globl	pick_next                       # -- Begin function pick_next
	.p2align	4
	.type	pick_next,@function
pick_next:                              # @pick_next
# %bb.0:
	movq	(%rdi), %rax
	testq	%rax, %rax
	je	.LBB3_1
# %bb.3:
	cmpl	$0, 8(%rax)
	je	.LBB3_4
# %bb.12:
	movq	%rax, %rdx
	.p2align	4
.LBB3_13:                               # =>This Inner Loop Header: Depth=1
	movq	16(%rdx), %rax
	testq	%rax, %rax
	je	.LBB3_1
# %bb.5:                                #   in Loop: Header=BB3_13 Depth=1
	cmpl	$0, 8(%rax)
	movq	%rdx, %rcx
	movq	%rax, %rdx
	jne	.LBB3_13
	jmp	.LBB3_6
.LBB3_1:
	xorl	%eax, %eax
	retq
.LBB3_4:
	xorl	%ecx, %ecx
.LBB3_6:
	movq	16(%rax), %rdx
	testq	%rcx, %rcx
	je	.LBB3_8
# %bb.7:
	movq	%rdx, 16(%rcx)
	cmpq	%rax, 8(%rdi)
	je	.LBB3_10
.LBB3_11:
	movq	$0, 16(%rax)
	retq
.LBB3_8:
	movq	%rdx, (%rdi)
	cmpq	%rax, 8(%rdi)
	jne	.LBB3_11
.LBB3_10:
	movq	%rcx, 8(%rdi)
	movq	$0, 16(%rax)
	retq
.Lfunc_end3:
	.size	pick_next, .Lfunc_end3-pick_next
                                        # -- End function
	.globl	frame_init                      # -- Begin function frame_init
	.p2align	4
	.type	frame_init,@function
frame_init:                             # @frame_init
# %bb.0:
	leaq	-56(%rdi), %rax
	xorps	%xmm0, %xmm0
	movups	%xmm0, -56(%rdi)
	movq	%rdx, -40(%rdi)
	movq	%rsi, -32(%rdi)
	movups	%xmm0, -24(%rdi)
	movq	%rcx, -8(%rdi)
	retq
.Lfunc_end4:
	.size	frame_init, .Lfunc_end4-frame_init
                                        # -- End function
	.globl	demo_main                       # -- Begin function demo_main
	.p2align	4
	.type	demo_main,@function
demo_main:                              # @demo_main
# %bb.0:
	movups	.L__const.demo_main.a+16(%rip), %xmm0
	movaps	%xmm0, -24(%rsp)
	movups	.L__const.demo_main.a(%rip), %xmm0
	movaps	%xmm0, -40(%rsp)
	movups	.L__const.demo_main.b+16(%rip), %xmm0
	movaps	%xmm0, -56(%rsp)
	movups	.L__const.demo_main.b(%rip), %xmm0
	movaps	%xmm0, -72(%rsp)
	movups	.L__const.demo_main.c+16(%rip), %xmm0
	movaps	%xmm0, -88(%rsp)
	movups	.L__const.demo_main.c(%rip), %xmm0
	movaps	%xmm0, -104(%rsp)
	leaq	-72(%rsp), %rax
	movq	%rax, -24(%rsp)
	movq	$0, -88(%rsp)
	leaq	-104(%rsp), %rax
	movq	%rax, -56(%rsp)
	leaq	-40(%rsp), %rax
	.p2align	4
.LBB5_2:                                # =>This Inner Loop Header: Depth=1
	movq	%rax, %rcx
	movq	16(%rax), %rax
	testq	%rax, %rax
	je	.LBB5_3
# %bb.1:                                #   in Loop: Header=BB5_2 Depth=1
	cmpl	$0, 8(%rax)
	jne	.LBB5_2
# %bb.4:
	movq	16(%rax), %rdx
	movq	%rdx, 16(%rcx)
	movq	$0, 16(%rax)
	movl	24(%rax), %eax
	retq
.LBB5_3:
	movl	$-1, %eax
	retq
.Lfunc_end5:
	.size	demo_main, .Lfunc_end5-demo_main
                                        # -- End function
	.type	.L__const.demo_main.a,@object   # @__const.demo_main.a
	.section	.rodata.cst32,"aM",@progbits,32
	.p2align	3, 0x0
.L__const.demo_main.a:
	.quad	0
	.long	2                               # 0x2
	.zero	4
	.quad	0
	.long	1                               # 0x1
	.zero	4
	.size	.L__const.demo_main.a, 32

	.type	.L__const.demo_main.b,@object   # @__const.demo_main.b
	.p2align	3, 0x0
.L__const.demo_main.b:
	.quad	0
	.long	0                               # 0x0
	.zero	4
	.quad	0
	.long	2                               # 0x2
	.zero	4
	.size	.L__const.demo_main.b, 32

	.type	.L__const.demo_main.c,@object   # @__const.demo_main.c
	.p2align	3, 0x0
.L__const.demo_main.c:
	.quad	0
	.long	0                               # 0x0
	.zero	4
	.quad	0
	.long	3                               # 0x3
	.zero	4
	.size	.L__const.demo_main.c, 32

	.ident	"clang version 20.1.8"
	.section	".note.GNU-stack","",@progbits
	.addrsig
