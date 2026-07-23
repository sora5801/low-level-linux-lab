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
	movq	%rsp, %rbp
	pushq	%r15
	pushq	%r14
	pushq	%r13
	pushq	%r12
	pushq	%rbx
	movq	%rcx, %rax
	movl	16(%rbp), %ecx
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
	leal	1(%r15), %r12d
	jmp	.LBB0_4
	.p2align	4
.LBB0_7:                                #   in Loop: Header=BB0_4 Depth=2
	leaq	-1(%r14), %rcx
	andq	%rcx, %r14
	je	.LBB0_8
.LBB0_4:                                #   Parent Loop BB0_2 Depth=1
                                        # =>  This Inner Loop Header: Depth=2
	rep		bsfq	%r14, %rcx
	movl	%r15d, %r13d
	orl	%ecx, %r13d
	movslq	%r13d, %r13
	cmpb	$0, (%rdi,%r13)
	jne	.LBB0_7
# %bb.5:                                #   in Loop: Header=BB0_4 Depth=2
	movl	(%rsi,%r13,4), %r13d
	leal	(%r11,%r13,4), %r13d
	movslq	%r13d, %r13
	testq	%r10, (%rdx,%r13,8)
	je	.LBB0_7
# %bb.6:                                #   in Loop: Header=BB0_4 Depth=2
	addl	%r12d, %ecx
	movl	$1, %r13d
	shlq	%cl, %r13
	sarl	$6, %ecx
	movslq	%ecx, %rcx
	orq	%r13, (%r8,%rcx,8)
	jmp	.LBB0_7
.LBB0_9:
	popq	%rbx
	popq	%r12
	popq	%r13
	popq	%r14
	popq	%r15
	popq	%rbp
.LBB0_10:
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
	pushq	%r15
	pushq	%r14
	pushq	%r13
	pushq	%r12
	pushq	%rbx
	subq	$120, %rsp
	movq	%rcx, %r13
	movq	%rdx, -144(%rbp)                # 8-byte Spill
	movq	%rdi, -56(%rbp)                 # 8-byte Spill
	movq	8(%rsi), %rbx
	movq	16(%rsi), %rax
	movq	%rax, -80(%rbp)                 # 8-byte Spill
	movq	24(%rsi), %r15
	movq	%rsi, -96(%rbp)                 # 8-byte Spill
	movslq	(%rsi), %r14
	testq	%r14, %r14
	jle	.LBB1_2
# %bb.1:
	leaq	(,%r14,8), %rdx
	movq	%rbx, %rdi
	xorl	%esi, %esi
	callq	memset@PLT
.LBB1_2:
	movq	-96(%rbp), %rax                 # 8-byte Reload
	movq	32(%rax), %rdx
	movq	-56(%rbp), %rdi                 # 8-byte Reload
	movl	56(%rdi), %ecx
	movq	%rbx, %rsi
	callq	add_state
	testq	%r13, %r13
	je	.LBB1_3
# %bb.4:
	leaq	(,%r14,8), %rax
	movq	%rax, -112(%rbp)                # 8-byte Spill
	movl	%r14d, %esi
	xorl	%edi, %edi
	movq	%r14, -120(%rbp)                # 8-byte Spill
	movq	%r13, -136(%rbp)                # 8-byte Spill
	movq	%rsi, -104(%rbp)                # 8-byte Spill
	jmp	.LBB1_5
	.p2align	4
.LBB1_9:                                #   in Loop: Header=BB1_5 Depth=1
	movq	%r15, %rax
.LBB1_32:                               #   in Loop: Header=BB1_5 Depth=1
	incq	%rdi
	cmpq	%r13, %rdi
	sete	%cl
	orb	%dl, %cl
	movq	%rax, %r15
	cmpb	$1, %cl
	je	.LBB1_33
.LBB1_5:                                # =>This Loop Header: Depth=1
                                        #     Child Loop BB1_11 Depth 2
                                        #     Child Loop BB1_16 Depth 2
                                        #       Child Loop BB1_18 Depth 3
                                        #     Child Loop BB1_26 Depth 2
                                        #       Child Loop BB1_28 Depth 3
	testl	%r14d, %r14d
	jle	.LBB1_6
# %bb.10:                               #   in Loop: Header=BB1_5 Depth=1
	xorl	%eax, %eax
	xorl	%ecx, %ecx
	.p2align	4
.LBB1_11:                               #   Parent Loop BB1_5 Depth=1
                                        # =>  This Inner Loop Header: Depth=2
	xorl	%edx, %edx
	cmpq	$0, (%rbx,%rax,8)
	setne	%dl
	orl	%edx, %ecx
	incq	%rax
	cmpq	%rax, %rsi
	jne	.LBB1_11
# %bb.7:                                #   in Loop: Header=BB1_5 Depth=1
	testl	%ecx, %ecx
	sete	%dl
	testb	%dl, %dl
	jne	.LBB1_9
	jmp	.LBB1_12
	.p2align	4
.LBB1_6:                                #   in Loop: Header=BB1_5 Depth=1
	movb	$1, %dl
	testb	%dl, %dl
	jne	.LBB1_9
.LBB1_12:                               #   in Loop: Header=BB1_5 Depth=1
	movl	%edx, -44(%rbp)                 # 4-byte Spill
	movq	%rdi, -72(%rbp)                 # 8-byte Spill
	movq	%r15, -64(%rbp)                 # 8-byte Spill
	testl	%r14d, %r14d
	jle	.LBB1_14
# %bb.13:                               #   in Loop: Header=BB1_5 Depth=1
	movq	-80(%rbp), %rdi                 # 8-byte Reload
	xorl	%esi, %esi
	movq	-112(%rbp), %rdx                # 8-byte Reload
	callq	memset@PLT
.LBB1_14:                               #   in Loop: Header=BB1_5 Depth=1
	testl	%r14d, %r14d
	jle	.LBB1_23
# %bb.15:                               #   in Loop: Header=BB1_5 Depth=1
	movq	-56(%rbp), %rcx                 # 8-byte Reload
	movq	8(%rcx), %rax
	movq	32(%rcx), %rdx
	movq	48(%rcx), %rsi
	movq	-144(%rbp), %rcx                # 8-byte Reload
	movq	-72(%rbp), %rdi                 # 8-byte Reload
	movzbl	(%rcx,%rdi), %ecx
	movl	%ecx, %edi
	shrl	$6, %edi
	movl	$1, %r8d
                                        # kill: def $cl killed $cl killed $ecx
	shlq	%cl, %r8
	xorl	%r9d, %r9d
	jmp	.LBB1_16
	.p2align	4
.LBB1_22:                               #   in Loop: Header=BB1_16 Depth=2
	incq	%r9
	movq	-120(%rbp), %r14                # 8-byte Reload
	cmpq	%r14, %r9
	je	.LBB1_23
.LBB1_16:                               #   Parent Loop BB1_5 Depth=1
                                        # =>  This Loop Header: Depth=2
                                        #       Child Loop BB1_18 Depth 3
	movq	(%rbx,%r9,8), %r10
	testq	%r10, %r10
	je	.LBB1_22
# %bb.17:                               #   in Loop: Header=BB1_16 Depth=2
	movl	%r9d, %r11d
	shll	$6, %r11d
	leal	1(%r11), %r15d
	jmp	.LBB1_18
	.p2align	4
.LBB1_21:                               #   in Loop: Header=BB1_18 Depth=3
	leaq	-1(%r10), %rcx
	andq	%rcx, %r10
	je	.LBB1_22
.LBB1_18:                               #   Parent Loop BB1_5 Depth=1
                                        #     Parent Loop BB1_16 Depth=2
                                        # =>    This Inner Loop Header: Depth=3
	rep		bsfq	%r10, %rcx
	movl	%r11d, %r14d
	orl	%ecx, %r14d
	movslq	%r14d, %r14
	cmpb	$0, (%rax,%r14)
	jne	.LBB1_21
# %bb.19:                               #   in Loop: Header=BB1_18 Depth=3
	movl	(%rdx,%r14,4), %r14d
	leal	(%rdi,%r14,4), %r14d
	movslq	%r14d, %r14
	testq	%r8, (%rsi,%r14,8)
	je	.LBB1_21
# %bb.20:                               #   in Loop: Header=BB1_18 Depth=3
	addl	%r15d, %ecx
	movl	$1, %r14d
	shlq	%cl, %r14
	sarl	$6, %ecx
	movslq	%ecx, %rcx
	movq	-80(%rbp), %r12                 # 8-byte Reload
	orq	%r14, (%r12,%rcx,8)
	jmp	.LBB1_21
	.p2align	4
.LBB1_23:                               #   in Loop: Header=BB1_5 Depth=1
	testl	%r14d, %r14d
	jle	.LBB1_24
# %bb.25:                               #   in Loop: Header=BB1_5 Depth=1
	movq	%rbx, -152(%rbp)                # 8-byte Spill
	movq	-96(%rbp), %rax                 # 8-byte Reload
	movq	32(%rax), %rax
	movq	%rax, -160(%rbp)                # 8-byte Spill
	movq	-64(%rbp), %rdi                 # 8-byte Reload
	xorl	%esi, %esi
	movq	-112(%rbp), %rdx                # 8-byte Reload
	callq	memset@PLT
	xorl	%ecx, %ecx
	jmp	.LBB1_26
	.p2align	4
.LBB1_29:                               #   in Loop: Header=BB1_26 Depth=2
	movq	-128(%rbp), %rcx                # 8-byte Reload
	incq	%rcx
	movq	-120(%rbp), %r14                # 8-byte Reload
	cmpq	%r14, %rcx
	je	.LBB1_30
.LBB1_26:                               #   Parent Loop BB1_5 Depth=1
                                        # =>  This Loop Header: Depth=2
                                        #       Child Loop BB1_28 Depth 3
	movq	-80(%rbp), %rax                 # 8-byte Reload
	movq	%rcx, -128(%rbp)                # 8-byte Spill
	movq	(%rax,%rcx,8), %r12
	testq	%r12, %r12
	movq	-56(%rbp), %r15                 # 8-byte Reload
	movq	-64(%rbp), %r13                 # 8-byte Reload
	movq	-160(%rbp), %r14                # 8-byte Reload
	je	.LBB1_29
# %bb.27:                               #   in Loop: Header=BB1_26 Depth=2
	movq	-128(%rbp), %rax                # 8-byte Reload
                                        # kill: def $eax killed $eax killed $rax
	shll	$6, %eax
	movl	%eax, -84(%rbp)                 # 4-byte Spill
	.p2align	4
.LBB1_28:                               #   Parent Loop BB1_5 Depth=1
                                        #     Parent Loop BB1_26 Depth=2
                                        # =>    This Inner Loop Header: Depth=3
	rep		bsfq	%r12, %rcx
	leaq	-1(%r12), %rbx
	orl	-84(%rbp), %ecx                 # 4-byte Folded Reload
	movq	%r15, %rdi
	movq	%r13, %rsi
	movq	%r14, %rdx
                                        # kill: def $ecx killed $ecx killed $rcx
	callq	add_state
	andq	%rbx, %r12
	jne	.LBB1_28
	jmp	.LBB1_29
	.p2align	4
.LBB1_30:                               #   in Loop: Header=BB1_5 Depth=1
	movq	-152(%rbp), %rax                # 8-byte Reload
	movq	-64(%rbp), %rbx                 # 8-byte Reload
	movq	-136(%rbp), %r13                # 8-byte Reload
	jmp	.LBB1_31
.LBB1_24:                               #   in Loop: Header=BB1_5 Depth=1
	movq	%rbx, %rax
	movq	-64(%rbp), %rbx                 # 8-byte Reload
.LBB1_31:                               #   in Loop: Header=BB1_5 Depth=1
	movq	-104(%rbp), %rsi                # 8-byte Reload
	movq	-72(%rbp), %rdi                 # 8-byte Reload
	movl	-44(%rbp), %edx                 # 4-byte Reload
	jmp	.LBB1_32
.LBB1_3:
	xorl	%edx, %edx
.LBB1_33:
	testb	%dl, %dl
	movl	$0, %eax
	jne	.LBB1_35
# %bb.34:
	movq	-56(%rbp), %rax                 # 8-byte Reload
	movl	60(%rax), %edx
	movl	%edx, %ecx
	sarl	$6, %ecx
	movslq	%ecx, %rcx
	movq	(%rbx,%rcx,8), %rcx
	xorl	%eax, %eax
	btq	%rdx, %rcx
	setb	%al
.LBB1_35:
	addq	$120, %rsp
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
	btq	%rcx, %r8
	jae	.LBB2_1
# %bb.12:
	retq
.LBB2_1:
	pushq	%rbp
	movq	%rsp, %rbp
	pushq	%r15
	pushq	%r14
	pushq	%rbx
	movl	%ecx, %r9d
	andl	$63, %r9d
	btsq	%r9, %r8
	movq	%r8, (%rsi,%rax,8)
	movl	%ecx, (%rdx)
	movq	8(%rdi), %rax
	movl	$1, %r8d
	jmp	.LBB2_2
	.p2align	4
.LBB2_10:                               #   in Loop: Header=BB2_2 Depth=1
	movl	%ecx, %r8d
	testl	%ecx, %ecx
	je	.LBB2_11
.LBB2_2:                                # =>This Inner Loop Header: Depth=1
	leal	-1(%r8), %ecx
	movslq	%r8d, %r9
	movslq	-4(%rdx,%r9,4), %r10
	decq	%r9
	movzbl	(%rax,%r10), %r11d
	cmpl	$1, %r11d
	je	.LBB2_6
# %bb.3:                                #   in Loop: Header=BB2_2 Depth=1
	cmpl	$2, %r11d
	jne	.LBB2_10
# %bb.4:                                #   in Loop: Header=BB2_2 Depth=1
	movq	16(%rdi), %r11
	movl	(%r11,%r10,4), %r10d
	movl	%r10d, %r11d
	sarl	$6, %r11d
	movslq	%r11d, %r11
	movq	(%rsi,%r11,8), %rbx
	movl	%r10d, %r14d
	andl	$63, %r14d
	btq	%r14, %rbx
	jb	.LBB2_10
# %bb.5:                                #   in Loop: Header=BB2_2 Depth=1
	btsq	%r14, %rbx
	movq	%rbx, (%rsi,%r11,8)
	movl	%r10d, (%rdx,%r9,4)
	movl	%r8d, %ecx
	jmp	.LBB2_10
	.p2align	4
.LBB2_6:                                #   in Loop: Header=BB2_2 Depth=1
	movq	16(%rdi), %r11
	movq	24(%rdi), %rbx
	movl	(%r11,%r10,4), %r11d
	movl	(%rbx,%r10,4), %r10d
	movl	%r11d, %ebx
	sarl	$6, %ebx
	movslq	%ebx, %rbx
	movq	(%rsi,%rbx,8), %r14
	movl	%r11d, %r15d
	andl	$63, %r15d
	btq	%r15, %r14
	jb	.LBB2_8
# %bb.7:                                #   in Loop: Header=BB2_2 Depth=1
	btsq	%r15, %r14
	movq	%r14, (%rsi,%rbx,8)
	movl	%r11d, (%rdx,%r9,4)
	movl	%r8d, %ecx
.LBB2_8:                                #   in Loop: Header=BB2_2 Depth=1
	movl	%r10d, %r8d
	sarl	$6, %r8d
	movslq	%r8d, %r8
	movq	(%rsi,%r8,8), %r9
	movl	%r10d, %r11d
	andl	$63, %r11d
	btq	%r11, %r9
	jb	.LBB2_10
# %bb.9:                                #   in Loop: Header=BB2_2 Depth=1
	btsq	%r11, %r9
	movq	%r9, (%rsi,%r8,8)
	movslq	%ecx, %r8
	incl	%ecx
	movl	%r10d, (%rdx,%r8,4)
	jmp	.LBB2_10
.LBB2_11:
	popq	%rbx
	popq	%r14
	popq	%r15
	popq	%rbp
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
	movq	%rsp, %rbp
	pushq	%r15
	pushq	%r14
	pushq	%r13
	pushq	%r12
	pushq	%rbx
	subq	$104, %rsp
	movq	%rcx, -120(%rbp)                # 8-byte Spill
	movq	%rdx, -112(%rbp)                # 8-byte Spill
	movq	%rdi, %r14
	movq	8(%rsi), %r13
	movq	16(%rsi), %r12
	movq	24(%rsi), %rbx
	movq	%rsi, -88(%rbp)                 # 8-byte Spill
	movslq	(%rsi), %r15
	testq	%r15, %r15
	jle	.LBB3_2
# %bb.1:
	leaq	(,%r15,8), %rdx
	movq	%r13, %rdi
	xorl	%esi, %esi
	callq	memset@PLT
.LBB3_2:
	leaq	(,%r15,8), %rax
	movq	%rax, -80(%rbp)                 # 8-byte Spill
	movq	$0, -48(%rbp)                   # 8-byte Folded Spill
	movq	%r15, -64(%rbp)                 # 8-byte Spill
	movq	%r14, -96(%rbp)                 # 8-byte Spill
	movq	%r12, -136(%rbp)                # 8-byte Spill
	jmp	.LBB3_3
	.p2align	4
.LBB3_22:                               #   in Loop: Header=BB3_3 Depth=1
	incq	-48(%rbp)                       # 8-byte Folded Spill
	movq	-128(%rbp), %rbx                # 8-byte Reload
	movq	-56(%rbp), %r13                 # 8-byte Reload
	movq	-96(%rbp), %r14                 # 8-byte Reload
	movq	-64(%rbp), %r15                 # 8-byte Reload
.LBB3_3:                                # =>This Loop Header: Depth=1
                                        #     Child Loop BB3_9 Depth 2
                                        #       Child Loop BB3_11 Depth 3
                                        #     Child Loop BB3_18 Depth 2
                                        #       Child Loop BB3_20 Depth 3
	movq	-88(%rbp), %rax                 # 8-byte Reload
	movq	32(%rax), %rdx
	movl	56(%r14), %ecx
	movq	%r14, %rdi
	movq	%r13, %rsi
	callq	add_state
	movl	60(%r14), %ecx
	movl	%ecx, %eax
	movl	$1, %edx
                                        # kill: def $cl killed $cl killed $ecx
	shlq	%cl, %rdx
	sarl	$6, %eax
	cltq
	andq	(%r13,%rax,8), %rdx
	jne	.LBB3_23
# %bb.4:                                #   in Loop: Header=BB3_3 Depth=1
	movq	-48(%rbp), %rax                 # 8-byte Reload
	cmpq	-120(%rbp), %rax                # 8-byte Folded Reload
	je	.LBB3_23
# %bb.5:                                #   in Loop: Header=BB3_3 Depth=1
	movq	%rbx, -56(%rbp)                 # 8-byte Spill
	testl	%r15d, %r15d
	jle	.LBB3_7
# %bb.6:                                #   in Loop: Header=BB3_3 Depth=1
	movq	%r12, %rdi
	xorl	%esi, %esi
	movq	-80(%rbp), %rdx                 # 8-byte Reload
	callq	memset@PLT
.LBB3_7:                                #   in Loop: Header=BB3_3 Depth=1
	testl	%r15d, %r15d
	jle	.LBB3_16
# %bb.8:                                #   in Loop: Header=BB3_3 Depth=1
	movq	8(%r14), %rax
	movq	32(%r14), %rdx
	movq	48(%r14), %rsi
	movq	-112(%rbp), %rcx                # 8-byte Reload
	movq	-48(%rbp), %rdi                 # 8-byte Reload
	movzbl	(%rcx,%rdi), %ecx
	movl	%ecx, %edi
	shrl	$6, %edi
	movl	$1, %r8d
                                        # kill: def $cl killed $cl killed $ecx
	shlq	%cl, %r8
	xorl	%r9d, %r9d
	jmp	.LBB3_9
	.p2align	4
.LBB3_15:                               #   in Loop: Header=BB3_9 Depth=2
	incq	%r9
	movq	-64(%rbp), %r15                 # 8-byte Reload
	cmpq	%r15, %r9
	je	.LBB3_16
.LBB3_9:                                #   Parent Loop BB3_3 Depth=1
                                        # =>  This Loop Header: Depth=2
                                        #       Child Loop BB3_11 Depth 3
	movq	(%r13,%r9,8), %r10
	testq	%r10, %r10
	je	.LBB3_15
# %bb.10:                               #   in Loop: Header=BB3_9 Depth=2
	movl	%r9d, %r11d
	shll	$6, %r11d
	leal	1(%r11), %r15d
	jmp	.LBB3_11
	.p2align	4
.LBB3_14:                               #   in Loop: Header=BB3_11 Depth=3
	leaq	-1(%r10), %rcx
	andq	%rcx, %r10
	je	.LBB3_15
.LBB3_11:                               #   Parent Loop BB3_3 Depth=1
                                        #     Parent Loop BB3_9 Depth=2
                                        # =>    This Inner Loop Header: Depth=3
	rep		bsfq	%r10, %rcx
	movl	%r11d, %ebx
	orl	%ecx, %ebx
	movslq	%ebx, %rbx
	cmpb	$0, (%rax,%rbx)
	jne	.LBB3_14
# %bb.12:                               #   in Loop: Header=BB3_11 Depth=3
	movl	(%rdx,%rbx,4), %ebx
	leal	(%rdi,%rbx,4), %ebx
	movslq	%ebx, %rbx
	testq	%r8, (%rsi,%rbx,8)
	je	.LBB3_14
# %bb.13:                               #   in Loop: Header=BB3_11 Depth=3
	addl	%r15d, %ecx
	movl	$1, %ebx
	shlq	%cl, %rbx
	sarl	$6, %ecx
	movslq	%ecx, %rcx
	orq	%rbx, (%r12,%rcx,8)
	jmp	.LBB3_14
	.p2align	4
.LBB3_16:                               #   in Loop: Header=BB3_3 Depth=1
	movq	%r13, -128(%rbp)                # 8-byte Spill
	testl	%r15d, %r15d
	jle	.LBB3_22
# %bb.17:                               #   in Loop: Header=BB3_3 Depth=1
	movq	-88(%rbp), %rax                 # 8-byte Reload
	movq	32(%rax), %rax
	movq	%rax, -144(%rbp)                # 8-byte Spill
	movq	-56(%rbp), %rdi                 # 8-byte Reload
	xorl	%esi, %esi
	movq	-80(%rbp), %rdx                 # 8-byte Reload
	callq	memset@PLT
	xorl	%eax, %eax
	jmp	.LBB3_18
	.p2align	4
.LBB3_21:                               #   in Loop: Header=BB3_18 Depth=2
	movq	-104(%rbp), %rax                # 8-byte Reload
	incq	%rax
	cmpq	-64(%rbp), %rax                 # 8-byte Folded Reload
	movq	-136(%rbp), %r12                # 8-byte Reload
	je	.LBB3_22
.LBB3_18:                               #   Parent Loop BB3_3 Depth=1
                                        # =>  This Loop Header: Depth=2
                                        #       Child Loop BB3_20 Depth 3
	movq	%rax, -104(%rbp)                # 8-byte Spill
	movq	(%r12,%rax,8), %r13
	testq	%r13, %r13
	movq	-96(%rbp), %r15                 # 8-byte Reload
	movq	-56(%rbp), %rbx                 # 8-byte Reload
	movq	-144(%rbp), %r12                # 8-byte Reload
	je	.LBB3_21
# %bb.19:                               #   in Loop: Header=BB3_18 Depth=2
	movq	-104(%rbp), %rax                # 8-byte Reload
                                        # kill: def $eax killed $eax killed $rax
	shll	$6, %eax
	movl	%eax, -68(%rbp)                 # 4-byte Spill
	.p2align	4
.LBB3_20:                               #   Parent Loop BB3_3 Depth=1
                                        #     Parent Loop BB3_18 Depth=2
                                        # =>    This Inner Loop Header: Depth=3
	rep		bsfq	%r13, %rcx
	leaq	-1(%r13), %r14
	orl	-68(%rbp), %ecx                 # 4-byte Folded Reload
	movq	%r15, %rdi
	movq	%rbx, %rsi
	movq	%r12, %rdx
                                        # kill: def $ecx killed $ecx killed $rcx
	callq	add_state
	andq	%r14, %r13
	jne	.LBB3_20
	jmp	.LBB3_21
.LBB3_23:
	xorl	%eax, %eax
	testq	%rdx, %rdx
	setne	%al
	addq	$104, %rsp
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
