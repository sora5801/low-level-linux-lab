	.file	"demo.c"
	.text
	.globl	align_up                        # -- Begin function align_up
	.p2align	4
	.type	align_up,@function
align_up:                               # @align_up
# %bb.0:
	pushq	%rbp
	movq	%rsp, %rbp
	movq	%rdi, -8(%rbp)
	movq	%rsi, -16(%rbp)
	movq	-8(%rbp), %rax
	movq	-16(%rbp), %rcx
	subq	$1, %rcx
	addq	%rcx, %rax
	movq	-16(%rbp), %rcx
	subq	$1, %rcx
	xorq	$-1, %rcx
	andq	%rcx, %rax
	popq	%rbp
	retq
.Lfunc_end0:
	.size	align_up, .Lfunc_end0-align_up
                                        # -- End function
	.globl	runq_push                       # -- Begin function runq_push
	.p2align	4
	.type	runq_push,@function
runq_push:                              # @runq_push
# %bb.0:
	pushq	%rbp
	movq	%rsp, %rbp
	movq	%rdi, -8(%rbp)
	movq	%rsi, -16(%rbp)
	movq	-16(%rbp), %rax
	movq	$0, 16(%rax)
	movq	-8(%rbp), %rax
	cmpq	$0, 8(%rax)
	je	.LBB1_2
# %bb.1:
	movq	-16(%rbp), %rcx
	movq	-8(%rbp), %rax
	movq	8(%rax), %rax
	movq	%rcx, 16(%rax)
	jmp	.LBB1_3
.LBB1_2:
	movq	-16(%rbp), %rcx
	movq	-8(%rbp), %rax
	movq	%rcx, (%rax)
.LBB1_3:
	movq	-16(%rbp), %rcx
	movq	-8(%rbp), %rax
	movq	%rcx, 8(%rax)
	popq	%rbp
	retq
.Lfunc_end1:
	.size	runq_push, .Lfunc_end1-runq_push
                                        # -- End function
	.globl	runq_pop                        # -- Begin function runq_pop
	.p2align	4
	.type	runq_pop,@function
runq_pop:                               # @runq_pop
# %bb.0:
	pushq	%rbp
	movq	%rsp, %rbp
	movq	%rdi, -16(%rbp)
	movq	-16(%rbp), %rax
	movq	(%rax), %rax
	movq	%rax, -24(%rbp)
	cmpq	$0, -24(%rbp)
	jne	.LBB2_2
# %bb.1:
	movq	$0, -8(%rbp)
	jmp	.LBB2_5
.LBB2_2:
	movq	-24(%rbp), %rax
	movq	16(%rax), %rcx
	movq	-16(%rbp), %rax
	movq	%rcx, (%rax)
	movq	-16(%rbp), %rax
	cmpq	$0, (%rax)
	jne	.LBB2_4
# %bb.3:
	movq	-16(%rbp), %rax
	movq	$0, 8(%rax)
.LBB2_4:
	movq	-24(%rbp), %rax
	movq	$0, 16(%rax)
	movq	-24(%rbp), %rax
	movq	%rax, -8(%rbp)
.LBB2_5:
	movq	-8(%rbp), %rax
	popq	%rbp
	retq
.Lfunc_end2:
	.size	runq_pop, .Lfunc_end2-runq_pop
                                        # -- End function
	.globl	pick_next                       # -- Begin function pick_next
	.p2align	4
	.type	pick_next,@function
pick_next:                              # @pick_next
# %bb.0:
	pushq	%rbp
	movq	%rsp, %rbp
	movq	%rdi, -16(%rbp)
	movq	$0, -24(%rbp)
	movq	-16(%rbp), %rax
	movq	(%rax), %rax
	movq	%rax, -32(%rbp)
.LBB3_1:                                # =>This Inner Loop Header: Depth=1
	cmpq	$0, -32(%rbp)
	je	.LBB3_10
# %bb.2:                                #   in Loop: Header=BB3_1 Depth=1
	movq	-32(%rbp), %rax
	cmpl	$0, 8(%rax)
	jne	.LBB3_9
# %bb.3:
	cmpq	$0, -24(%rbp)
	je	.LBB3_5
# %bb.4:
	movq	-32(%rbp), %rax
	movq	16(%rax), %rcx
	movq	-24(%rbp), %rax
	movq	%rcx, 16(%rax)
	jmp	.LBB3_6
.LBB3_5:
	movq	-32(%rbp), %rax
	movq	16(%rax), %rcx
	movq	-16(%rbp), %rax
	movq	%rcx, (%rax)
.LBB3_6:
	movq	-16(%rbp), %rax
	movq	8(%rax), %rax
	cmpq	-32(%rbp), %rax
	jne	.LBB3_8
# %bb.7:
	movq	-24(%rbp), %rcx
	movq	-16(%rbp), %rax
	movq	%rcx, 8(%rax)
.LBB3_8:
	movq	-32(%rbp), %rax
	movq	$0, 16(%rax)
	movq	-32(%rbp), %rax
	movq	%rax, -8(%rbp)
	jmp	.LBB3_11
.LBB3_9:                                #   in Loop: Header=BB3_1 Depth=1
	movq	-32(%rbp), %rax
	movq	%rax, -24(%rbp)
	movq	-32(%rbp), %rax
	movq	16(%rax), %rax
	movq	%rax, -32(%rbp)
	jmp	.LBB3_1
.LBB3_10:
	movq	$0, -8(%rbp)
.LBB3_11:
	movq	-8(%rbp), %rax
	popq	%rbp
	retq
.Lfunc_end3:
	.size	pick_next, .Lfunc_end3-pick_next
                                        # -- End function
	.globl	frame_init                      # -- Begin function frame_init
	.p2align	4
	.type	frame_init,@function
frame_init:                             # @frame_init
# %bb.0:
	pushq	%rbp
	movq	%rsp, %rbp
	movq	%rdi, -8(%rbp)
	movq	%rsi, -16(%rbp)
	movq	%rdx, -24(%rbp)
	movq	%rcx, -32(%rbp)
	movq	-8(%rbp), %rax
	subq	$56, %rax
	movq	%rax, -40(%rbp)
	movq	-40(%rbp), %rax
	movq	$0, (%rax)
	movq	-40(%rbp), %rax
	movq	$0, 8(%rax)
	movq	-24(%rbp), %rcx
	movq	-40(%rbp), %rax
	movq	%rcx, 16(%rax)
	movq	-16(%rbp), %rcx
	movq	-40(%rbp), %rax
	movq	%rcx, 24(%rax)
	movq	-40(%rbp), %rax
	movq	$0, 32(%rax)
	movq	-40(%rbp), %rax
	movq	$0, 40(%rax)
	movq	-32(%rbp), %rcx
	movq	-40(%rbp), %rax
	movq	%rcx, 48(%rax)
	movq	-40(%rbp), %rax
	popq	%rbp
	retq
.Lfunc_end4:
	.size	frame_init, .Lfunc_end4-frame_init
                                        # -- End function
	.globl	demo_main                       # -- Begin function demo_main
	.p2align	4
	.type	demo_main,@function
demo_main:                              # @demo_main
# %bb.0:
	pushq	%rbp
	movq	%rsp, %rbp
	subq	$128, %rsp
	movq	.L__const.demo_main.a(%rip), %rax
	movq	%rax, -32(%rbp)
	movq	.L__const.demo_main.a+8(%rip), %rax
	movq	%rax, -24(%rbp)
	movq	.L__const.demo_main.a+16(%rip), %rax
	movq	%rax, -16(%rbp)
	movq	.L__const.demo_main.a+24(%rip), %rax
	movq	%rax, -8(%rbp)
	movq	.L__const.demo_main.b(%rip), %rax
	movq	%rax, -64(%rbp)
	movq	.L__const.demo_main.b+8(%rip), %rax
	movq	%rax, -56(%rbp)
	movq	.L__const.demo_main.b+16(%rip), %rax
	movq	%rax, -48(%rbp)
	movq	.L__const.demo_main.b+24(%rip), %rax
	movq	%rax, -40(%rbp)
	movq	.L__const.demo_main.c(%rip), %rax
	movq	%rax, -96(%rbp)
	movq	.L__const.demo_main.c+8(%rip), %rax
	movq	%rax, -88(%rbp)
	movq	.L__const.demo_main.c+16(%rip), %rax
	movq	%rax, -80(%rbp)
	movq	.L__const.demo_main.c+24(%rip), %rax
	movq	%rax, -72(%rbp)
	leaq	-112(%rbp), %rdi
	xorl	%esi, %esi
	movl	$16, %edx
	callq	memset@PLT
	leaq	-112(%rbp), %rdi
	leaq	-32(%rbp), %rsi
	callq	runq_push
	leaq	-112(%rbp), %rdi
	leaq	-64(%rbp), %rsi
	callq	runq_push
	leaq	-112(%rbp), %rdi
	leaq	-96(%rbp), %rsi
	callq	runq_push
	leaq	-112(%rbp), %rdi
	callq	pick_next
	movq	%rax, -120(%rbp)
	cmpq	$0, -120(%rbp)
	je	.LBB5_2
# %bb.1:
	movq	-120(%rbp), %rax
	movl	24(%rax), %eax
	movl	%eax, -124(%rbp)                # 4-byte Spill
	jmp	.LBB5_3
.LBB5_2:
	movl	$4294967295, %eax               # imm = 0xFFFFFFFF
	movl	%eax, -124(%rbp)                # 4-byte Spill
	jmp	.LBB5_3
.LBB5_3:
	movl	-124(%rbp), %eax                # 4-byte Reload
	addq	$128, %rsp
	popq	%rbp
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
	.addrsig_sym runq_push
	.addrsig_sym pick_next
