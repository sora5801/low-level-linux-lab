	.file	"demo.c"
	.text
	.globl	nfa_step                        # -- Begin function nfa_step
	.p2align	4
	.type	nfa_step,@function
nfa_step:                               # @nfa_step
# %bb.0:
	pushq	%rbp
	movq	%rsp, %rbp
	movl	16(%rbp), %eax
	movq	%rdi, -8(%rbp)
	movq	%rsi, -16(%rbp)
	movq	%rdx, -24(%rbp)
	movq	%rcx, -32(%rbp)
	movq	%r8, -40(%rbp)
	movl	%r9d, -44(%rbp)
	movl	$0, -48(%rbp)
.LBB0_1:                                # =>This Loop Header: Depth=1
                                        #     Child Loop BB0_3 Depth 2
	movl	-48(%rbp), %eax
	cmpl	-44(%rbp), %eax
	jge	.LBB0_11
# %bb.2:                                #   in Loop: Header=BB0_1 Depth=1
	movq	-32(%rbp), %rax
	movslq	-48(%rbp), %rcx
	movq	(%rax,%rcx,8), %rax
	movq	%rax, -56(%rbp)
.LBB0_3:                                #   Parent Loop BB0_1 Depth=1
                                        # =>  This Inner Loop Header: Depth=2
	cmpq	$0, -56(%rbp)
	je	.LBB0_9
# %bb.4:                                #   in Loop: Header=BB0_3 Depth=2
	movq	-56(%rbp), %rcx
                                        # implicit-def: $rax
	rep		bsfq	%rcx, %rax
                                        # kill: def $eax killed $eax killed $rax
	movl	%eax, -60(%rbp)
	movq	-56(%rbp), %rax
	subq	$1, %rax
	andq	-56(%rbp), %rax
	movq	%rax, -56(%rbp)
	movl	-48(%rbp), %eax
	shll	$6, %eax
	addl	-60(%rbp), %eax
	movl	%eax, -64(%rbp)
	movq	-8(%rbp), %rax
	movslq	-64(%rbp), %rcx
	movzbl	(%rax,%rcx), %eax
	cmpl	$0, %eax
	je	.LBB0_6
# %bb.5:                                #   in Loop: Header=BB0_3 Depth=2
	jmp	.LBB0_3
.LBB0_6:                                #   in Loop: Header=BB0_3 Depth=2
	movq	-16(%rbp), %rax
	movslq	-64(%rbp), %rcx
	movl	(%rax,%rcx,4), %eax
	movl	%eax, -68(%rbp)
	movq	-24(%rbp), %rax
	movl	-68(%rbp), %ecx
	shll	$2, %ecx
	movl	16(%rbp), %edx
	sarl	$6, %edx
	addl	%edx, %ecx
	movslq	%ecx, %rcx
	movq	(%rax,%rcx,8), %rax
	movq	%rax, -80(%rbp)
	movq	-80(%rbp), %rax
	movl	16(%rbp), %ecx
	andl	$63, %ecx
	movl	%ecx, %ecx
                                        # kill: def $rcx killed $ecx
                                        # kill: def $cl killed $rcx
	shrq	%cl, %rax
	andq	$1, %rax
	cmpq	$0, %rax
	je	.LBB0_8
# %bb.7:                                #   in Loop: Header=BB0_3 Depth=2
	movl	-64(%rbp), %eax
	addl	$1, %eax
	movl	%eax, -84(%rbp)
	movl	-84(%rbp), %eax
	andl	$63, %eax
	movl	%eax, %eax
	movl	%eax, %ecx
	movl	$1, %edx
                                        # kill: def $cl killed $rcx
	shlq	%cl, %rdx
	movq	-40(%rbp), %rax
	movl	-84(%rbp), %ecx
	sarl	$6, %ecx
	movslq	%ecx, %rcx
	orq	(%rax,%rcx,8), %rdx
	movq	%rdx, (%rax,%rcx,8)
.LBB0_8:                                #   in Loop: Header=BB0_3 Depth=2
	jmp	.LBB0_3
.LBB0_9:                                #   in Loop: Header=BB0_1 Depth=1
	jmp	.LBB0_10
.LBB0_10:                               #   in Loop: Header=BB0_1 Depth=1
	movl	-48(%rbp), %eax
	addl	$1, %eax
	movl	%eax, -48(%rbp)
	jmp	.LBB0_1
.LBB0_11:
	popq	%rbp
	retq
.Lfunc_end0:
	.size	nfa_step, .Lfunc_end0-nfa_step
                                        # -- End function
	.globl	demo_run                        # -- Begin function demo_run
	.p2align	4
	.type	demo_run,@function
demo_run:                               # @demo_run
# %bb.0:
	pushq	%rbp
	movq	%rsp, %rbp
	subq	$48, %rsp
	movw	.L__const.demo_run.op(%rip), %ax
	movw	%ax, -3(%rbp)
	movb	.L__const.demo_run.op+2(%rip), %al
	movb	%al, -1(%rbp)
	movq	.L__const.demo_run.cls(%rip), %rax
	movq	%rax, -16(%rbp)
	movl	.L__const.demo_run.cls+8(%rip), %eax
	movl	%eax, -8(%rbp)
	movl	$0, -20(%rbp)
.LBB1_1:                                # =>This Inner Loop Header: Depth=1
	cmpl	$8, -20(%rbp)
	jge	.LBB1_4
# %bb.2:                                #   in Loop: Header=BB1_1 Depth=1
	movslq	-20(%rbp), %rcx
	leaq	g_sets(%rip), %rax
	movq	$0, (%rax,%rcx,8)
# %bb.3:                                #   in Loop: Header=BB1_1 Depth=1
	movl	-20(%rbp), %eax
	addl	$1, %eax
	movl	%eax, -20(%rbp)
	jmp	.LBB1_1
.LBB1_4:
	movabsq	$8589934592, %rax               # imm = 0x200000000
	orq	g_sets+8(%rip), %rax
	movq	%rax, g_sets+8(%rip)
	movabsq	$17179869184, %rax              # imm = 0x400000000
	orq	g_sets+40(%rip), %rax
	movq	%rax, g_sets+40(%rip)
	movabsq	$34359738368, %rax              # imm = 0x800000000
	orq	g_sets+40(%rip), %rax
	movq	%rax, g_sets+40(%rip)
	movq	.L__const.demo_run.cur(%rip), %rax
	movq	%rax, -32(%rbp)
	leaq	-40(%rbp), %rdi
	xorl	%esi, %esi
	movl	$8, %edx
	callq	memset@PLT
	leaq	-3(%rbp), %rdi
	leaq	-16(%rbp), %rsi
	leaq	-32(%rbp), %rcx
	leaq	-40(%rbp), %r8
	leaq	g_sets(%rip), %rdx
	movl	$1, %r9d
	movl	$97, (%rsp)
	callq	nfa_step
	movq	-40(%rbp), %rax
                                        # kill: def $eax killed $eax killed $rax
	addq	$48, %rsp
	popq	%rbp
	retq
.Lfunc_end1:
	.size	demo_run, .Lfunc_end1-demo_run
                                        # -- End function
	.type	.L__const.demo_run.op,@object   # @__const.demo_run.op
	.section	.rodata,"a",@progbits
.L__const.demo_run.op:
	.ascii	"\000\000\003"
	.size	.L__const.demo_run.op, 3

	.type	.L__const.demo_run.cls,@object  # @__const.demo_run.cls
	.p2align	2, 0x0
.L__const.demo_run.cls:
	.long	0                               # 0x0
	.long	1                               # 0x1
	.long	4294967295                      # 0xffffffff
	.size	.L__const.demo_run.cls, 12

	.type	g_sets,@object                  # @g_sets
	.local	g_sets
	.comm	g_sets,64,16
	.type	.L__const.demo_run.cur,@object  # @__const.demo_run.cur
	.section	.rodata.cst8,"aM",@progbits,8
	.p2align	3, 0x0
.L__const.demo_run.cur:
	.quad	1                               # 0x1
	.size	.L__const.demo_run.cur, 8

	.ident	"clang version 20.1.8"
	.section	".note.GNU-stack","",@progbits
	.addrsig
	.addrsig_sym nfa_step
	.addrsig_sym g_sets
