	.file	"sim.c"
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
	.globl	re_fullmatch                    # -- Begin function re_fullmatch
	.p2align	4
	.type	re_fullmatch,@function
re_fullmatch:                           # @re_fullmatch
# %bb.0:
	pushq	%rbp
	movq	%rsp, %rbp
	subq	$112, %rsp
	movq	%rdi, -16(%rbp)
	movq	%rsi, -24(%rbp)
	movq	%rdx, -32(%rbp)
	movq	%rcx, -40(%rbp)
	movq	-24(%rbp), %rax
	movq	8(%rax), %rax
	movq	%rax, -48(%rbp)
	movq	-24(%rbp), %rax
	movq	16(%rax), %rax
	movq	%rax, -56(%rbp)
	movq	-24(%rbp), %rax
	movq	24(%rax), %rax
	movq	%rax, -64(%rbp)
	movq	-24(%rbp), %rax
	movl	(%rax), %eax
	movl	%eax, -68(%rbp)
	movq	-48(%rbp), %rdi
	movl	-68(%rbp), %esi
	callq	bs_clear
	movq	-16(%rbp), %rdi
	movq	-48(%rbp), %rsi
	movq	-24(%rbp), %rax
	movq	32(%rax), %rdx
	movq	-16(%rbp), %rax
	movl	56(%rax), %ecx
	callq	add_state
	movq	$0, -80(%rbp)
.LBB1_1:                                # =>This Loop Header: Depth=1
                                        #     Child Loop BB1_3 Depth 2
	movq	-80(%rbp), %rax
	cmpq	-40(%rbp), %rax
	jae	.LBB1_10
# %bb.2:                                #   in Loop: Header=BB1_1 Depth=1
	movl	$0, -84(%rbp)
	movl	$0, -88(%rbp)
.LBB1_3:                                #   Parent Loop BB1_1 Depth=1
                                        # =>  This Inner Loop Header: Depth=2
	movl	-88(%rbp), %eax
	cmpl	-68(%rbp), %eax
	jge	.LBB1_6
# %bb.4:                                #   in Loop: Header=BB1_3 Depth=2
	movq	-48(%rbp), %rax
	movslq	-88(%rbp), %rcx
	cmpq	$0, (%rax,%rcx,8)
	setne	%al
	andb	$1, %al
	movzbl	%al, %eax
	orl	-84(%rbp), %eax
	movl	%eax, -84(%rbp)
# %bb.5:                                #   in Loop: Header=BB1_3 Depth=2
	movl	-88(%rbp), %eax
	addl	$1, %eax
	movl	%eax, -88(%rbp)
	jmp	.LBB1_3
.LBB1_6:                                #   in Loop: Header=BB1_1 Depth=1
	cmpl	$0, -84(%rbp)
	jne	.LBB1_8
# %bb.7:
	movl	$0, -4(%rbp)
	jmp	.LBB1_11
.LBB1_8:                                #   in Loop: Header=BB1_1 Depth=1
	movq	-56(%rbp), %rdi
	movl	-68(%rbp), %esi
	callq	bs_clear
	movq	-16(%rbp), %rax
	movq	8(%rax), %rdi
	movq	-16(%rbp), %rax
	movq	32(%rax), %rsi
	movq	-16(%rbp), %rax
	movq	48(%rax), %rdx
	movq	-48(%rbp), %rcx
	movq	-56(%rbp), %r8
	movl	-68(%rbp), %r9d
	movq	-32(%rbp), %rax
	movq	-80(%rbp), %r10
	movzbl	(%rax,%r10), %eax
	movl	%eax, (%rsp)
	callq	nfa_step
	movq	-16(%rbp), %rdi
	movq	-56(%rbp), %rsi
	movq	-64(%rbp), %rdx
	movq	-24(%rbp), %rax
	movq	32(%rax), %rcx
	movl	-68(%rbp), %r8d
	callq	closure_from_seed
	movq	-48(%rbp), %rax
	movq	%rax, -96(%rbp)
	movq	-64(%rbp), %rax
	movq	%rax, -48(%rbp)
	movq	-96(%rbp), %rax
	movq	%rax, -64(%rbp)
# %bb.9:                                #   in Loop: Header=BB1_1 Depth=1
	movq	-80(%rbp), %rax
	addq	$1, %rax
	movq	%rax, -80(%rbp)
	jmp	.LBB1_1
.LBB1_10:
	movq	-48(%rbp), %rdi
	movq	-16(%rbp), %rax
	movl	60(%rax), %esi
	callq	bs_test
	movl	%eax, -4(%rbp)
.LBB1_11:
	movl	-4(%rbp), %eax
	addq	$112, %rsp
	popq	%rbp
	retq
.Lfunc_end1:
	.size	re_fullmatch, .Lfunc_end1-re_fullmatch
                                        # -- End function
	.p2align	4                               # -- Begin function bs_clear
	.type	bs_clear,@function
bs_clear:                               # @bs_clear
# %bb.0:
	pushq	%rbp
	movq	%rsp, %rbp
	movq	%rdi, -8(%rbp)
	movl	%esi, -12(%rbp)
	movl	$0, -16(%rbp)
.LBB2_1:                                # =>This Inner Loop Header: Depth=1
	movl	-16(%rbp), %eax
	cmpl	-12(%rbp), %eax
	jge	.LBB2_4
# %bb.2:                                #   in Loop: Header=BB2_1 Depth=1
	movq	-8(%rbp), %rax
	movslq	-16(%rbp), %rcx
	movq	$0, (%rax,%rcx,8)
# %bb.3:                                #   in Loop: Header=BB2_1 Depth=1
	movl	-16(%rbp), %eax
	addl	$1, %eax
	movl	%eax, -16(%rbp)
	jmp	.LBB2_1
.LBB2_4:
	popq	%rbp
	retq
.Lfunc_end2:
	.size	bs_clear, .Lfunc_end2-bs_clear
                                        # -- End function
	.p2align	4                               # -- Begin function add_state
	.type	add_state,@function
add_state:                              # @add_state
# %bb.0:
	pushq	%rbp
	movq	%rsp, %rbp
	subq	$64, %rsp
	movq	%rdi, -8(%rbp)
	movq	%rsi, -16(%rbp)
	movq	%rdx, -24(%rbp)
	movl	%ecx, -28(%rbp)
	movq	-16(%rbp), %rdi
	movl	-28(%rbp), %esi
	callq	bs_test
	cmpl	$0, %eax
	je	.LBB3_2
# %bb.1:
	jmp	.LBB3_15
.LBB3_2:
	movl	$0, -32(%rbp)
	movq	-16(%rbp), %rdi
	movl	-28(%rbp), %esi
	callq	bs_set
	movl	-28(%rbp), %edx
	movq	-24(%rbp), %rax
	movl	-32(%rbp), %ecx
	movl	%ecx, %esi
	addl	$1, %esi
	movl	%esi, -32(%rbp)
	movslq	%ecx, %rcx
	movl	%edx, (%rax,%rcx,4)
.LBB3_3:                                # =>This Inner Loop Header: Depth=1
	cmpl	$0, -32(%rbp)
	je	.LBB3_15
# %bb.4:                                #   in Loop: Header=BB3_3 Depth=1
	movq	-24(%rbp), %rax
	movslq	-32(%rbp), %rcx
	movl	%ecx, %edx
	decl	%edx
	movl	%edx, -32(%rbp)
	movl	-4(%rax,%rcx,4), %eax
	movl	%eax, -36(%rbp)
	movq	-8(%rbp), %rax
	movq	8(%rax), %rax
	movslq	-36(%rbp), %rcx
	movzbl	(%rax,%rcx), %eax
	movl	%eax, -52(%rbp)                 # 4-byte Spill
	subl	$1, %eax
	je	.LBB3_8
	jmp	.LBB3_16
.LBB3_16:                               #   in Loop: Header=BB3_3 Depth=1
	movl	-52(%rbp), %eax                 # 4-byte Reload
	subl	$2, %eax
	jne	.LBB3_13
	jmp	.LBB3_5
.LBB3_5:                                #   in Loop: Header=BB3_3 Depth=1
	movq	-8(%rbp), %rax
	movq	16(%rax), %rax
	movslq	-36(%rbp), %rcx
	movl	(%rax,%rcx,4), %eax
	movl	%eax, -40(%rbp)
	movq	-16(%rbp), %rdi
	movl	-40(%rbp), %esi
	callq	bs_test
	cmpl	$0, %eax
	jne	.LBB3_7
# %bb.6:                                #   in Loop: Header=BB3_3 Depth=1
	movq	-16(%rbp), %rdi
	movl	-40(%rbp), %esi
	callq	bs_set
	movl	-40(%rbp), %edx
	movq	-24(%rbp), %rax
	movl	-32(%rbp), %ecx
	movl	%ecx, %esi
	addl	$1, %esi
	movl	%esi, -32(%rbp)
	movslq	%ecx, %rcx
	movl	%edx, (%rax,%rcx,4)
.LBB3_7:                                #   in Loop: Header=BB3_3 Depth=1
	jmp	.LBB3_14
.LBB3_8:                                #   in Loop: Header=BB3_3 Depth=1
	movq	-8(%rbp), %rax
	movq	16(%rax), %rax
	movslq	-36(%rbp), %rcx
	movl	(%rax,%rcx,4), %eax
	movl	%eax, -44(%rbp)
	movq	-8(%rbp), %rax
	movq	24(%rax), %rax
	movslq	-36(%rbp), %rcx
	movl	(%rax,%rcx,4), %eax
	movl	%eax, -48(%rbp)
	movq	-16(%rbp), %rdi
	movl	-44(%rbp), %esi
	callq	bs_test
	cmpl	$0, %eax
	jne	.LBB3_10
# %bb.9:                                #   in Loop: Header=BB3_3 Depth=1
	movq	-16(%rbp), %rdi
	movl	-44(%rbp), %esi
	callq	bs_set
	movl	-44(%rbp), %edx
	movq	-24(%rbp), %rax
	movl	-32(%rbp), %ecx
	movl	%ecx, %esi
	addl	$1, %esi
	movl	%esi, -32(%rbp)
	movslq	%ecx, %rcx
	movl	%edx, (%rax,%rcx,4)
.LBB3_10:                               #   in Loop: Header=BB3_3 Depth=1
	movq	-16(%rbp), %rdi
	movl	-48(%rbp), %esi
	callq	bs_test
	cmpl	$0, %eax
	jne	.LBB3_12
# %bb.11:                               #   in Loop: Header=BB3_3 Depth=1
	movq	-16(%rbp), %rdi
	movl	-48(%rbp), %esi
	callq	bs_set
	movl	-48(%rbp), %edx
	movq	-24(%rbp), %rax
	movl	-32(%rbp), %ecx
	movl	%ecx, %esi
	addl	$1, %esi
	movl	%esi, -32(%rbp)
	movslq	%ecx, %rcx
	movl	%edx, (%rax,%rcx,4)
.LBB3_12:                               #   in Loop: Header=BB3_3 Depth=1
	jmp	.LBB3_14
.LBB3_13:                               #   in Loop: Header=BB3_3 Depth=1
	jmp	.LBB3_14
.LBB3_14:                               #   in Loop: Header=BB3_3 Depth=1
	jmp	.LBB3_3
.LBB3_15:
	addq	$64, %rsp
	popq	%rbp
	retq
.Lfunc_end3:
	.size	add_state, .Lfunc_end3-add_state
                                        # -- End function
	.p2align	4                               # -- Begin function closure_from_seed
	.type	closure_from_seed,@function
closure_from_seed:                      # @closure_from_seed
# %bb.0:
	pushq	%rbp
	movq	%rsp, %rbp
	subq	$64, %rsp
	movq	%rdi, -8(%rbp)
	movq	%rsi, -16(%rbp)
	movq	%rdx, -24(%rbp)
	movq	%rcx, -32(%rbp)
	movl	%r8d, -36(%rbp)
	movq	-24(%rbp), %rdi
	movl	-36(%rbp), %esi
	callq	bs_clear
	movl	$0, -40(%rbp)
.LBB4_1:                                # =>This Loop Header: Depth=1
                                        #     Child Loop BB4_3 Depth 2
	movl	-40(%rbp), %eax
	cmpl	-36(%rbp), %eax
	jge	.LBB4_7
# %bb.2:                                #   in Loop: Header=BB4_1 Depth=1
	movq	-16(%rbp), %rax
	movslq	-40(%rbp), %rcx
	movq	(%rax,%rcx,8), %rax
	movq	%rax, -48(%rbp)
.LBB4_3:                                #   Parent Loop BB4_1 Depth=1
                                        # =>  This Inner Loop Header: Depth=2
	cmpq	$0, -48(%rbp)
	je	.LBB4_5
# %bb.4:                                #   in Loop: Header=BB4_3 Depth=2
	movq	-48(%rbp), %rcx
                                        # implicit-def: $rax
	rep		bsfq	%rcx, %rax
                                        # kill: def $eax killed $eax killed $rax
	movl	%eax, -52(%rbp)
	movq	-48(%rbp), %rax
	subq	$1, %rax
	andq	-48(%rbp), %rax
	movq	%rax, -48(%rbp)
	movq	-8(%rbp), %rdi
	movq	-24(%rbp), %rsi
	movq	-32(%rbp), %rdx
	movl	-40(%rbp), %ecx
	shll	$6, %ecx
	addl	-52(%rbp), %ecx
	callq	add_state
	jmp	.LBB4_3
.LBB4_5:                                #   in Loop: Header=BB4_1 Depth=1
	jmp	.LBB4_6
.LBB4_6:                                #   in Loop: Header=BB4_1 Depth=1
	movl	-40(%rbp), %eax
	addl	$1, %eax
	movl	%eax, -40(%rbp)
	jmp	.LBB4_1
.LBB4_7:
	addq	$64, %rsp
	popq	%rbp
	retq
.Lfunc_end4:
	.size	closure_from_seed, .Lfunc_end4-closure_from_seed
                                        # -- End function
	.p2align	4                               # -- Begin function bs_test
	.type	bs_test,@function
bs_test:                                # @bs_test
# %bb.0:
	pushq	%rbp
	movq	%rsp, %rbp
	movq	%rdi, -8(%rbp)
	movl	%esi, -12(%rbp)
	movq	-8(%rbp), %rax
	movl	-12(%rbp), %ecx
	sarl	$6, %ecx
	movslq	%ecx, %rcx
	movq	(%rax,%rcx,8), %rax
	movl	-12(%rbp), %ecx
	andl	$63, %ecx
	movl	%ecx, %ecx
                                        # kill: def $rcx killed $ecx
                                        # kill: def $cl killed $rcx
	shrq	%cl, %rax
	andq	$1, %rax
                                        # kill: def $eax killed $eax killed $rax
	popq	%rbp
	retq
.Lfunc_end5:
	.size	bs_test, .Lfunc_end5-bs_test
                                        # -- End function
	.globl	re_contains                     # -- Begin function re_contains
	.p2align	4
	.type	re_contains,@function
re_contains:                            # @re_contains
# %bb.0:
	pushq	%rbp
	movq	%rsp, %rbp
	subq	$96, %rsp
	movq	%rdi, -16(%rbp)
	movq	%rsi, -24(%rbp)
	movq	%rdx, -32(%rbp)
	movq	%rcx, -40(%rbp)
	movq	-24(%rbp), %rax
	movq	8(%rax), %rax
	movq	%rax, -48(%rbp)
	movq	-24(%rbp), %rax
	movq	16(%rax), %rax
	movq	%rax, -56(%rbp)
	movq	-24(%rbp), %rax
	movq	24(%rax), %rax
	movq	%rax, -64(%rbp)
	movq	-24(%rbp), %rax
	movl	(%rax), %eax
	movl	%eax, -68(%rbp)
	movq	-48(%rbp), %rdi
	movl	-68(%rbp), %esi
	callq	bs_clear
	movq	$0, -80(%rbp)
.LBB6_1:                                # =>This Inner Loop Header: Depth=1
	movq	-16(%rbp), %rdi
	movq	-48(%rbp), %rsi
	movq	-24(%rbp), %rax
	movq	32(%rax), %rdx
	movq	-16(%rbp), %rax
	movl	56(%rax), %ecx
	callq	add_state
	movq	-48(%rbp), %rdi
	movq	-16(%rbp), %rax
	movl	60(%rax), %esi
	callq	bs_test
	cmpl	$0, %eax
	je	.LBB6_3
# %bb.2:
	movl	$1, -4(%rbp)
	jmp	.LBB6_8
.LBB6_3:                                #   in Loop: Header=BB6_1 Depth=1
	movq	-80(%rbp), %rax
	cmpq	-40(%rbp), %rax
	jne	.LBB6_5
# %bb.4:
	jmp	.LBB6_7
.LBB6_5:                                #   in Loop: Header=BB6_1 Depth=1
	movq	-56(%rbp), %rdi
	movl	-68(%rbp), %esi
	callq	bs_clear
	movq	-16(%rbp), %rax
	movq	8(%rax), %rdi
	movq	-16(%rbp), %rax
	movq	32(%rax), %rsi
	movq	-16(%rbp), %rax
	movq	48(%rax), %rdx
	movq	-48(%rbp), %rcx
	movq	-56(%rbp), %r8
	movl	-68(%rbp), %r9d
	movq	-32(%rbp), %rax
	movq	-80(%rbp), %r10
	movzbl	(%rax,%r10), %eax
	movl	%eax, (%rsp)
	callq	nfa_step
	movq	-16(%rbp), %rdi
	movq	-56(%rbp), %rsi
	movq	-64(%rbp), %rdx
	movq	-24(%rbp), %rax
	movq	32(%rax), %rcx
	movl	-68(%rbp), %r8d
	callq	closure_from_seed
	movq	-48(%rbp), %rax
	movq	%rax, -88(%rbp)
	movq	-64(%rbp), %rax
	movq	%rax, -48(%rbp)
	movq	-88(%rbp), %rax
	movq	%rax, -64(%rbp)
# %bb.6:                                #   in Loop: Header=BB6_1 Depth=1
	movq	-80(%rbp), %rax
	addq	$1, %rax
	movq	%rax, -80(%rbp)
	jmp	.LBB6_1
.LBB6_7:
	movl	$0, -4(%rbp)
.LBB6_8:
	movl	-4(%rbp), %eax
	addq	$96, %rsp
	popq	%rbp
	retq
.Lfunc_end6:
	.size	re_contains, .Lfunc_end6-re_contains
                                        # -- End function
	.p2align	4                               # -- Begin function bs_set
	.type	bs_set,@function
bs_set:                                 # @bs_set
# %bb.0:
	pushq	%rbp
	movq	%rsp, %rbp
	movq	%rdi, -8(%rbp)
	movl	%esi, -12(%rbp)
	movl	-12(%rbp), %eax
	andl	$63, %eax
	movl	%eax, %eax
	movl	%eax, %ecx
	movl	$1, %edx
                                        # kill: def $cl killed $rcx
	shlq	%cl, %rdx
	movq	-8(%rbp), %rax
	movl	-12(%rbp), %ecx
	sarl	$6, %ecx
	movslq	%ecx, %rcx
	orq	(%rax,%rcx,8), %rdx
	movq	%rdx, (%rax,%rcx,8)
	popq	%rbp
	retq
.Lfunc_end7:
	.size	bs_set, .Lfunc_end7-bs_set
                                        # -- End function
	.ident	"clang version 20.1.8"
	.section	".note.GNU-stack","",@progbits
	.addrsig
	.addrsig_sym nfa_step
	.addrsig_sym bs_clear
	.addrsig_sym add_state
	.addrsig_sym closure_from_seed
	.addrsig_sym bs_test
	.addrsig_sym bs_set
