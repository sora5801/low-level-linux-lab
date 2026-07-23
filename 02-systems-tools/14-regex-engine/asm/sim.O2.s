	.file	"sim.c"
	.text
	.globl	nfa_step                        # -- Begin function nfa_step
	.p2align	4
	.type	nfa_step,@function
nfa_step:                               # @nfa_step
# %bb.0:
	testl	%r9d, %r9d
	jle	.LBB0_10
# %bb.1:
	pushq	%rbp
	pushq	%r15
	pushq	%r14
	pushq	%r12
	pushq	%rbx
	movq	%rcx, %rax
	movl	48(%rsp), %ecx
	movl	$1, %r10d
	shlq	%cl, %r10
	movl	%ecx, %r11d
	sarl	$6, %r11d
	movl	%r9d, %r9d
	xorl	%ebx, %ebx
	jmp	.LBB0_2
	.p2align	4
.LBB0_8:                                #   in Loop: Header=BB0_2 Depth=1
	incq	%rbx
	cmpq	%r9, %rbx
	je	.LBB0_9
.LBB0_2:                                # =>This Loop Header: Depth=1
                                        #     Child Loop BB0_4 Depth 2
	movq	(%rax,%rbx,8), %r14
	testq	%r14, %r14
	je	.LBB0_8
# %bb.3:                                #   in Loop: Header=BB0_2 Depth=1
	movl	%ebx, %r15d
	shll	$6, %r15d
	leal	1(%r15), %ebp
	jmp	.LBB0_4
	.p2align	4
.LBB0_7:                                #   in Loop: Header=BB0_4 Depth=2
	leaq	-1(%r14), %rcx
	andq	%rcx, %r14
	je	.LBB0_8
.LBB0_4:                                #   Parent Loop BB0_2 Depth=1
                                        # =>  This Inner Loop Header: Depth=2
	rep		bsfq	%r14, %rcx
	movl	%r15d, %r12d
	orl	%ecx, %r12d
	movslq	%r12d, %r12
	cmpb	$0, (%rdi,%r12)
	jne	.LBB0_7
# %bb.5:                                #   in Loop: Header=BB0_4 Depth=2
	movl	(%rsi,%r12,4), %r12d
	leal	(%r11,%r12,4), %r12d
	movslq	%r12d, %r12
	testq	%r10, (%rdx,%r12,8)
	je	.LBB0_7
# %bb.6:                                #   in Loop: Header=BB0_4 Depth=2
	addl	%ebp, %ecx
	movl	$1, %r12d
	shlq	%cl, %r12
	sarl	$6, %ecx
	movslq	%ecx, %rcx
	orq	%r12, (%r8,%rcx,8)
	jmp	.LBB0_7
.LBB0_9:
	popq	%rbx
	popq	%r12
	popq	%r14
	popq	%r15
	popq	%rbp
.LBB0_10:
	retq
.Lfunc_end0:
	.size	nfa_step, .Lfunc_end0-nfa_step
                                        # -- End function
	.section	.rodata.cst16,"aM",@progbits,16
	.p2align	4, 0x0                          # -- Begin function re_fullmatch
.LCPI1_0:
	.long	1                               # 0x1
	.long	1                               # 0x1
	.zero	4
	.zero	4
	.text
	.globl	re_fullmatch
	.p2align	4
	.type	re_fullmatch,@function
re_fullmatch:                           # @re_fullmatch
# %bb.0:
	pushq	%rbp
	pushq	%r15
	pushq	%r14
	pushq	%r13
	pushq	%r12
	pushq	%rbx
	subq	$104, %rsp
	movq	%rcx, 16(%rsp)                  # 8-byte Spill
	movq	%rdx, 72(%rsp)                  # 8-byte Spill
	movq	%rdi, %rbx
	movq	8(%rsi), %r14
	movq	16(%rsi), %r15
	movq	24(%rsi), %r12
	movq	%rsi, 8(%rsp)                   # 8-byte Spill
	movl	(%rsi), %r13d
	testl	%r13d, %r13d
	jle	.LBB1_2
# %bb.1:
	leaq	(,%r13,8), %rdx
	movq	%r14, %rdi
	xorl	%esi, %esi
	callq	memset@PLT
.LBB1_2:
	movq	8(%rsp), %rax                   # 8-byte Reload
	movq	32(%rax), %rdx
	movl	56(%rbx), %ecx
	movq	%rbx, %rdi
	movq	%r14, %rsi
	callq	add_state
	cmpq	$0, 16(%rsp)                    # 8-byte Folded Reload
	je	.LBB1_26
# %bb.3:
	leaq	(,%r13,8), %rax
	movq	%rax, 24(%rsp)                  # 8-byte Spill
	movl	%r13d, %eax
	andl	$2147483644, %eax               # imm = 0x7FFFFFFC
	movq	%rax, 56(%rsp)                  # 8-byte Spill
	movl	%r13d, %esi
	shrl	$2, %esi
	andl	$536870911, %esi                # imm = 0x1FFFFFFF
	shlq	$5, %rsi
	pxor	%xmm5, %xmm5
	movdqa	.LCPI1_0(%rip), %xmm6           # xmm6 = [1,1,u,u]
	xorl	%ebp, %ebp
	movq	%rbx, 32(%rsp)                  # 8-byte Spill
	movq	%r15, 96(%rsp)                  # 8-byte Spill
	movq	%r13, 88(%rsp)                  # 8-byte Spill
	movq	%rsi, 64(%rsp)                  # 8-byte Spill
	jmp	.LBB1_5
	.p2align	4
.LBB1_4:                                #   in Loop: Header=BB1_5 Depth=1
	movq	80(%rsp), %rbp                  # 8-byte Reload
	incq	%rbp
	movq	(%rsp), %rax                    # 8-byte Reload
	movq	%rax, %r14
	cmpq	16(%rsp), %rbp                  # 8-byte Folded Reload
	movq	32(%rsp), %rbx                  # 8-byte Reload
	movq	40(%rsp), %r12                  # 8-byte Reload
	movq	64(%rsp), %rsi                  # 8-byte Reload
	pxor	%xmm5, %xmm5
	movdqa	.LCPI1_0(%rip), %xmm6           # xmm6 = [1,1,u,u]
	je	.LBB1_27
.LBB1_5:                                # =>This Loop Header: Depth=1
                                        #     Child Loop BB1_9 Depth 2
                                        #     Child Loop BB1_11 Depth 2
                                        #     Child Loop BB1_15 Depth 2
                                        #       Child Loop BB1_18 Depth 3
                                        #     Child Loop BB1_23 Depth 2
                                        #       Child Loop BB1_25 Depth 3
	testl	%r13d, %r13d
	jle	.LBB1_29
# %bb.6:                                #   in Loop: Header=BB1_5 Depth=1
	cmpl	$4, %r13d
	jae	.LBB1_8
# %bb.7:                                #   in Loop: Header=BB1_5 Depth=1
	xorl	%ecx, %ecx
	xorl	%eax, %eax
	jmp	.LBB1_11
	.p2align	4
.LBB1_8:                                #   in Loop: Header=BB1_5 Depth=1
	pxor	%xmm0, %xmm0
	xorl	%eax, %eax
	pxor	%xmm1, %xmm1
	.p2align	4
.LBB1_9:                                #   Parent Loop BB1_5 Depth=1
                                        # =>  This Inner Loop Header: Depth=2
	movdqu	(%r14,%rax), %xmm2
	movdqu	16(%r14,%rax), %xmm3
	pcmpeqd	%xmm5, %xmm2
	pshufd	$189, %xmm2, %xmm4              # xmm4 = xmm2[1,3,3,2]
	pshufd	$232, %xmm2, %xmm2              # xmm2 = xmm2[0,2,2,3]
	pand	%xmm4, %xmm2
	pandn	%xmm6, %xmm2
	por	%xmm2, %xmm0
	pcmpeqd	%xmm5, %xmm3
	pshufd	$189, %xmm3, %xmm2              # xmm2 = xmm3[1,3,3,2]
	pshufd	$232, %xmm3, %xmm3              # xmm3 = xmm3[0,2,2,3]
	pand	%xmm2, %xmm3
	pandn	%xmm6, %xmm3
	por	%xmm3, %xmm1
	addq	$32, %rax
	cmpq	%rax, %rsi
	jne	.LBB1_9
# %bb.10:                               #   in Loop: Header=BB1_5 Depth=1
	por	%xmm0, %xmm1
	pshufd	$85, %xmm1, %xmm0               # xmm0 = xmm1[1,1,1,1]
	por	%xmm1, %xmm0
	movd	%xmm0, %eax
	movq	56(%rsp), %rdx                  # 8-byte Reload
	movq	%rdx, %rcx
	cmpl	%r13d, %edx
	je	.LBB1_12
	.p2align	4
.LBB1_11:                               #   Parent Loop BB1_5 Depth=1
                                        # =>  This Inner Loop Header: Depth=2
	xorl	%edx, %edx
	cmpq	$0, (%r14,%rcx,8)
	setne	%dl
	orl	%edx, %eax
	incq	%rcx
	cmpq	%rcx, %r13
	jne	.LBB1_11
.LBB1_12:                               #   in Loop: Header=BB1_5 Depth=1
	testl	%eax, %eax
	je	.LBB1_29
# %bb.13:                               #   in Loop: Header=BB1_5 Depth=1
	movq	%r12, (%rsp)                    # 8-byte Spill
	movq	%r14, 40(%rsp)                  # 8-byte Spill
	movq	%r15, %rdi
	xorl	%esi, %esi
	movq	24(%rsp), %rdx                  # 8-byte Reload
	callq	memset@PLT
	movq	8(%rbx), %rax
	movq	32(%rbx), %rdx
	movq	48(%rbx), %rsi
	movq	72(%rsp), %rcx                  # 8-byte Reload
	movq	%rbp, 80(%rsp)                  # 8-byte Spill
	movzbl	(%rcx,%rbp), %ecx
	movl	%ecx, %edi
	shrl	$6, %edi
	movl	$1, %r8d
                                        # kill: def $cl killed $cl killed $ecx
	shlq	%cl, %r8
	xorl	%r9d, %r9d
	jmp	.LBB1_15
	.p2align	4
.LBB1_14:                               #   in Loop: Header=BB1_15 Depth=2
	incq	%r9
	cmpq	%r13, %r9
	je	.LBB1_21
.LBB1_15:                               #   Parent Loop BB1_5 Depth=1
                                        # =>  This Loop Header: Depth=2
                                        #       Child Loop BB1_18 Depth 3
	movq	40(%rsp), %rcx                  # 8-byte Reload
	movq	(%rcx,%r9,8), %r10
	testq	%r10, %r10
	je	.LBB1_14
# %bb.16:                               #   in Loop: Header=BB1_15 Depth=2
	movl	%r9d, %r11d
	shll	$6, %r11d
	leal	1(%r11), %r14d
	jmp	.LBB1_18
	.p2align	4
.LBB1_17:                               #   in Loop: Header=BB1_18 Depth=3
	leaq	-1(%r10), %rcx
	andq	%rcx, %r10
	je	.LBB1_14
.LBB1_18:                               #   Parent Loop BB1_5 Depth=1
                                        #     Parent Loop BB1_15 Depth=2
                                        # =>    This Inner Loop Header: Depth=3
	rep		bsfq	%r10, %rcx
	movl	%r11d, %ebp
	orl	%ecx, %ebp
	movslq	%ebp, %r12
	cmpb	$0, (%rax,%r12)
	jne	.LBB1_17
# %bb.19:                               #   in Loop: Header=BB1_18 Depth=3
	movl	(%rdx,%r12,4), %r12d
	leal	(%rdi,%r12,4), %ebp
	movslq	%ebp, %r12
	testq	%r8, (%rsi,%r12,8)
	je	.LBB1_17
# %bb.20:                               #   in Loop: Header=BB1_18 Depth=3
	addl	%r14d, %ecx
	movl	$1, %r12d
	shlq	%cl, %r12
	sarl	$6, %ecx
	movslq	%ecx, %rcx
	orq	%r12, (%r15,%rcx,8)
	jmp	.LBB1_17
	.p2align	4
.LBB1_21:                               #   in Loop: Header=BB1_5 Depth=1
	movq	8(%rsp), %rax                   # 8-byte Reload
	movq	32(%rax), %r14
	movq	(%rsp), %rdi                    # 8-byte Reload
	xorl	%esi, %esi
	movq	24(%rsp), %rdx                  # 8-byte Reload
	callq	memset@PLT
	xorl	%eax, %eax
	jmp	.LBB1_23
	.p2align	4
.LBB1_22:                               #   in Loop: Header=BB1_23 Depth=2
	movq	48(%rsp), %rax                  # 8-byte Reload
	incq	%rax
	movq	88(%rsp), %r13                  # 8-byte Reload
	cmpq	%r13, %rax
	movq	96(%rsp), %r15                  # 8-byte Reload
	je	.LBB1_4
.LBB1_23:                               #   Parent Loop BB1_5 Depth=1
                                        # =>  This Loop Header: Depth=2
                                        #       Child Loop BB1_25 Depth 3
	movq	%rax, 48(%rsp)                  # 8-byte Spill
	movq	(%r15,%rax,8), %rbp
	testq	%rbp, %rbp
	movq	32(%rsp), %r15                  # 8-byte Reload
	movq	(%rsp), %r12                    # 8-byte Reload
	je	.LBB1_22
# %bb.24:                               #   in Loop: Header=BB1_23 Depth=2
	movq	48(%rsp), %rax                  # 8-byte Reload
	movl	%eax, %r13d
	shll	$6, %r13d
	.p2align	4
.LBB1_25:                               #   Parent Loop BB1_5 Depth=1
                                        #     Parent Loop BB1_23 Depth=2
                                        # =>    This Inner Loop Header: Depth=3
	rep		bsfq	%rbp, %rcx
	leaq	-1(%rbp), %rbx
	orl	%r13d, %ecx
	movq	%r15, %rdi
	movq	%r12, %rsi
	movq	%r14, %rdx
                                        # kill: def $ecx killed $ecx killed $rcx
	callq	add_state
	andq	%rbx, %rbp
	jne	.LBB1_25
	jmp	.LBB1_22
.LBB1_29:
	xorl	%eax, %eax
	jmp	.LBB1_30
.LBB1_26:
	movq	%r14, %rax
.LBB1_27:
	movl	60(%rbx), %edx
	movl	%edx, %ecx
	sarl	$6, %ecx
	movslq	%ecx, %rcx
	movq	(%rax,%rcx,8), %rcx
	xorl	%eax, %eax
	btq	%rdx, %rcx
	setb	%al
.LBB1_30:
                                        # kill: def $eax killed $eax killed $rax
	addq	$104, %rsp
	popq	%rbx
	popq	%r12
	popq	%r13
	popq	%r14
	popq	%r15
	popq	%rbp
	retq
.Lfunc_end1:
	.size	re_fullmatch, .Lfunc_end1-re_fullmatch
                                        # -- End function
	.p2align	4                               # -- Begin function add_state
	.type	add_state,@function
add_state:                              # @add_state
# %bb.0:
                                        # kill: def $ecx killed $ecx def $rcx
	movl	%ecx, %eax
	sarl	$6, %eax
	cltq
	movq	(%rsi,%rax,8), %r8
	movl	$1, %r9d
	shlq	%cl, %r9
	btq	%rcx, %r8
	jae	.LBB2_1
# %bb.12:
	retq
.LBB2_1:
	pushq	%r15
	pushq	%r14
	pushq	%rbx
	orq	%r9, %r8
	movq	%r8, (%rsi,%rax,8)
	movl	%ecx, (%rdx)
	movq	8(%rdi), %r8
	movl	$1, %r10d
	jmp	.LBB2_2
	.p2align	4
.LBB2_10:                               #   in Loop: Header=BB2_2 Depth=1
	movl	%r9d, %r10d
	testl	%r9d, %r9d
	je	.LBB2_11
.LBB2_2:                                # =>This Inner Loop Header: Depth=1
	leal	-1(%r10), %r9d
	movslq	%r10d, %r11
	movslq	-4(%rdx,%r11,4), %rax
	decq	%r11
	movzbl	(%r8,%rax), %ecx
	cmpl	$1, %ecx
	je	.LBB2_6
# %bb.3:                                #   in Loop: Header=BB2_2 Depth=1
	cmpl	$2, %ecx
	jne	.LBB2_10
# %bb.4:                                #   in Loop: Header=BB2_2 Depth=1
	movq	16(%rdi), %rcx
	movl	(%rcx,%rax,4), %ecx
	movl	%ecx, %eax
	sarl	$6, %eax
	cltq
	movq	(%rsi,%rax,8), %rbx
	movl	$1, %r14d
	shlq	%cl, %r14
	btq	%rcx, %rbx
	jb	.LBB2_10
# %bb.5:                                #   in Loop: Header=BB2_2 Depth=1
	orq	%rbx, %r14
	movq	%r14, (%rsi,%rax,8)
	movl	%ecx, (%rdx,%r11,4)
	movl	%r10d, %r9d
	jmp	.LBB2_10
	.p2align	4
.LBB2_6:                                #   in Loop: Header=BB2_2 Depth=1
	movq	16(%rdi), %rcx
	movq	24(%rdi), %rbx
	movl	(%rcx,%rax,4), %ecx
	movl	(%rbx,%rax,4), %eax
	movl	%ecx, %ebx
	sarl	$6, %ebx
	movslq	%ebx, %rbx
	movq	(%rsi,%rbx,8), %r14
	movl	$1, %r15d
	shlq	%cl, %r15
	btq	%rcx, %r14
	jb	.LBB2_8
# %bb.7:                                #   in Loop: Header=BB2_2 Depth=1
	orq	%r15, %r14
	movq	%r14, (%rsi,%rbx,8)
	movl	%ecx, (%rdx,%r11,4)
	movl	%r10d, %r9d
.LBB2_8:                                #   in Loop: Header=BB2_2 Depth=1
	movl	%eax, %ecx
	sarl	$6, %ecx
	movslq	%ecx, %r10
	movq	(%rsi,%r10,8), %r11
	movl	$1, %ebx
	movl	%eax, %ecx
	shlq	%cl, %rbx
	btq	%rax, %r11
	jb	.LBB2_10
# %bb.9:                                #   in Loop: Header=BB2_2 Depth=1
	orq	%rbx, %r11
	movq	%r11, (%rsi,%r10,8)
	movslq	%r9d, %rcx
	incl	%r9d
	movl	%eax, (%rdx,%rcx,4)
	jmp	.LBB2_10
.LBB2_11:
	popq	%rbx
	popq	%r14
	popq	%r15
	retq
.Lfunc_end2:
	.size	add_state, .Lfunc_end2-add_state
                                        # -- End function
	.globl	re_contains                     # -- Begin function re_contains
	.p2align	4
	.type	re_contains,@function
re_contains:                            # @re_contains
# %bb.0:
	pushq	%rbp
	pushq	%r15
	pushq	%r14
	pushq	%r13
	pushq	%r12
	pushq	%rbx
	subq	$88, %rsp
	movq	%rcx, 64(%rsp)                  # 8-byte Spill
	movq	%rdx, 56(%rsp)                  # 8-byte Spill
	movq	%rdi, %r14
	movq	8(%rsi), %r12
	movq	16(%rsi), %r15
	movq	24(%rsi), %rbx
	movq	%rsi, 16(%rsp)                  # 8-byte Spill
	movslq	(%rsi), %r13
	leaq	(,%r13,8), %rbp
	testq	%r13, %r13
	jle	.LBB3_2
# %bb.1:
	movq	%r12, %rdi
	xorl	%esi, %esi
	movq	%rbp, %rdx
	callq	memset@PLT
.LBB3_2:
	movq	%rbp, 48(%rsp)                  # 8-byte Spill
	xorl	%ebp, %ebp
	movq	%r14, (%rsp)                    # 8-byte Spill
	movq	%r15, 80(%rsp)                  # 8-byte Spill
	movq	%r13, 72(%rsp)                  # 8-byte Spill
	jmp	.LBB3_3
	.p2align	4
.LBB3_19:                               #   in Loop: Header=BB3_3 Depth=1
	movq	24(%rsp), %rbp                  # 8-byte Reload
	incq	%rbp
	movq	32(%rsp), %rbx                  # 8-byte Reload
	movq	8(%rsp), %r12                   # 8-byte Reload
	movq	(%rsp), %r14                    # 8-byte Reload
.LBB3_3:                                # =>This Loop Header: Depth=1
                                        #     Child Loop BB3_7 Depth 2
                                        #       Child Loop BB3_9 Depth 3
                                        #     Child Loop BB3_15 Depth 2
                                        #       Child Loop BB3_17 Depth 3
	movq	16(%rsp), %rax                  # 8-byte Reload
	movq	32(%rax), %rdx
	movl	56(%r14), %ecx
	movq	%r14, %rdi
	movq	%r12, %rsi
	callq	add_state
	movl	60(%r14), %ecx
	movl	%ecx, %eax
	movl	$1, %edx
                                        # kill: def $cl killed $cl killed $ecx
	shlq	%cl, %rdx
	sarl	$6, %eax
	cltq
	andq	(%r12,%rax,8), %rdx
	jne	.LBB3_20
# %bb.4:                                #   in Loop: Header=BB3_3 Depth=1
	cmpq	64(%rsp), %rbp                  # 8-byte Folded Reload
	je	.LBB3_20
# %bb.5:                                #   in Loop: Header=BB3_3 Depth=1
	movq	%rbx, 8(%rsp)                   # 8-byte Spill
	movq	%rbp, 24(%rsp)                  # 8-byte Spill
	movq	%r12, 32(%rsp)                  # 8-byte Spill
	testl	%r13d, %r13d
	jle	.LBB3_19
# %bb.6:                                #   in Loop: Header=BB3_3 Depth=1
	movq	%r15, %rdi
	xorl	%esi, %esi
	movq	48(%rsp), %rbp                  # 8-byte Reload
	movq	%rbp, %rdx
	callq	memset@PLT
	movq	(%rsp), %rcx                    # 8-byte Reload
	movq	8(%rcx), %rax
	movq	32(%rcx), %rdx
	movq	48(%rcx), %rsi
	movq	56(%rsp), %rcx                  # 8-byte Reload
	movq	24(%rsp), %rdi                  # 8-byte Reload
	movzbl	(%rcx,%rdi), %ecx
	movl	%ecx, %edi
	shrl	$6, %edi
	movl	$1, %r8d
                                        # kill: def $cl killed $cl killed $ecx
	shlq	%cl, %r8
	xorl	%r9d, %r9d
	movq	32(%rsp), %r12                  # 8-byte Reload
	jmp	.LBB3_7
	.p2align	4
.LBB3_13:                               #   in Loop: Header=BB3_7 Depth=2
	incq	%r9
	cmpq	%r13, %r9
	je	.LBB3_14
.LBB3_7:                                #   Parent Loop BB3_3 Depth=1
                                        # =>  This Loop Header: Depth=2
                                        #       Child Loop BB3_9 Depth 3
	movq	(%r12,%r9,8), %r10
	testq	%r10, %r10
	je	.LBB3_13
# %bb.8:                                #   in Loop: Header=BB3_7 Depth=2
	movl	%r9d, %r11d
	shll	$6, %r11d
	leal	1(%r11), %r14d
	jmp	.LBB3_9
	.p2align	4
.LBB3_12:                               #   in Loop: Header=BB3_9 Depth=3
	leaq	-1(%r10), %rcx
	andq	%rcx, %r10
	je	.LBB3_13
.LBB3_9:                                #   Parent Loop BB3_3 Depth=1
                                        #     Parent Loop BB3_7 Depth=2
                                        # =>    This Inner Loop Header: Depth=3
	rep		bsfq	%r10, %rcx
	movl	%r11d, %ebx
	orl	%ecx, %ebx
	movslq	%ebx, %rbx
	cmpb	$0, (%rax,%rbx)
	jne	.LBB3_12
# %bb.10:                               #   in Loop: Header=BB3_9 Depth=3
	movl	(%rdx,%rbx,4), %ebx
	leal	(%rdi,%rbx,4), %ebx
	movslq	%ebx, %rbx
	testq	%r8, (%rsi,%rbx,8)
	je	.LBB3_12
# %bb.11:                               #   in Loop: Header=BB3_9 Depth=3
	addl	%r14d, %ecx
	movl	$1, %ebx
	shlq	%cl, %rbx
	sarl	$6, %ecx
	movslq	%ecx, %rcx
	orq	%rbx, (%r15,%rcx,8)
	jmp	.LBB3_12
	.p2align	4
.LBB3_14:                               #   in Loop: Header=BB3_3 Depth=1
	movq	16(%rsp), %rax                  # 8-byte Reload
	movq	32(%rax), %r14
	movq	8(%rsp), %rdi                   # 8-byte Reload
	xorl	%esi, %esi
	movq	%rbp, %rdx
	callq	memset@PLT
	xorl	%eax, %eax
	jmp	.LBB3_15
	.p2align	4
.LBB3_18:                               #   in Loop: Header=BB3_15 Depth=2
	movq	40(%rsp), %rax                  # 8-byte Reload
	incq	%rax
	movq	72(%rsp), %r13                  # 8-byte Reload
	cmpq	%r13, %rax
	movq	80(%rsp), %r15                  # 8-byte Reload
	je	.LBB3_19
.LBB3_15:                               #   Parent Loop BB3_3 Depth=1
                                        # =>  This Loop Header: Depth=2
                                        #       Child Loop BB3_17 Depth 3
	movq	%rax, 40(%rsp)                  # 8-byte Spill
	movq	(%r15,%rax,8), %r15
	testq	%r15, %r15
	movq	(%rsp), %rbp                    # 8-byte Reload
	movq	8(%rsp), %rbx                   # 8-byte Reload
	je	.LBB3_18
# %bb.16:                               #   in Loop: Header=BB3_15 Depth=2
	movq	40(%rsp), %rax                  # 8-byte Reload
	movl	%eax, %r13d
	shll	$6, %r13d
	.p2align	4
.LBB3_17:                               #   Parent Loop BB3_3 Depth=1
                                        #     Parent Loop BB3_15 Depth=2
                                        # =>    This Inner Loop Header: Depth=3
	rep		bsfq	%r15, %rcx
	leaq	-1(%r15), %r12
	orl	%r13d, %ecx
	movq	%rbp, %rdi
	movq	%rbx, %rsi
	movq	%r14, %rdx
                                        # kill: def $ecx killed $ecx killed $rcx
	callq	add_state
	andq	%r12, %r15
	jne	.LBB3_17
	jmp	.LBB3_18
.LBB3_20:
	xorl	%eax, %eax
	testq	%rdx, %rdx
	setne	%al
	addq	$88, %rsp
	popq	%rbx
	popq	%r12
	popq	%r13
	popq	%r14
	popq	%r15
	popq	%rbp
	retq
.Lfunc_end3:
	.size	re_contains, .Lfunc_end3-re_contains
                                        # -- End function
	.ident	"clang version 20.1.8"
	.section	".note.GNU-stack","",@progbits
	.addrsig
