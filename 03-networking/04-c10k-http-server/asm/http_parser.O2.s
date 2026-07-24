	.file	"http_parser.c"
	.text
	.globl	hp_slice_ci_eq                  # -- Begin function hp_slice_ci_eq
	.p2align	4
	.type	hp_slice_ci_eq,@function
hp_slice_ci_eq:                         # @hp_slice_ci_eq
# %bb.0:
	testq	%rdx, %rdx
	je	.LBB0_5
# %bb.1:
	addq	%rsi, %rdi
	xorl	%eax, %eax
	xorl	%esi, %esi
	.p2align	4
.LBB0_3:                                # =>This Inner Loop Header: Depth=1
	movzbl	(%rdi,%rsi), %r9d
	leal	-65(%r9), %r8d
	leal	32(%r9), %r10d
	cmpb	$26, %r8b
	movzbl	%r10b, %r8d
	cmovael	%r9d, %r8d
	movzbl	(%rcx,%rsi), %r10d
	leal	-65(%r10), %r9d
	leal	32(%r10), %r11d
	cmpb	$26, %r9b
	movzbl	%r11b, %r9d
	cmovael	%r10d, %r9d
	testb	%r9b, %r9b
	je	.LBB0_6
# %bb.4:                                #   in Loop: Header=BB0_3 Depth=1
	cmpb	%r9b, %r8b
	jne	.LBB0_6
# %bb.2:                                #   in Loop: Header=BB0_3 Depth=1
	incq	%rsi
	cmpq	%rsi, %rdx
	jne	.LBB0_3
.LBB0_5:
	xorl	%eax, %eax
	cmpb	$0, (%rcx,%rdx)
	sete	%al
.LBB0_6:
                                        # kill: def $eax killed $eax killed $rax
	retq
.Lfunc_end0:
	.size	hp_slice_ci_eq, .Lfunc_end0-hp_slice_ci_eq
                                        # -- End function
	.globl	hp_execute                      # -- Begin function hp_execute
	.p2align	4
	.type	hp_execute,@function
hp_execute:                             # @hp_execute
# %bb.0:
	pushq	%rbp
	pushq	%r15
	pushq	%r14
	pushq	%r13
	pushq	%r12
	pushq	%rbx
	movq	8(%rdi), %rcx
	cmpq	%rdx, %rcx
	jae	.LBB1_2
# %bb.1:
	leaq	-1(%rsi), %rax
	movq	%rax, -16(%rsp)                 # 8-byte Spill
	leaq	88(%rdi), %r9
	leaq	1112(%rdi), %r12
	movl	2152(%rdi), %r10d
	leaq	hp_execute.V(%rip), %rbx
	jmp	.LBB1_7
.LBB1_2:
	movl	(%rdi), %edx
	xorl	%eax, %eax
	cmpl	$12, %edx
	sete	%cl
	cmpl	$13, %edx
	je	.LBB1_137
# %bb.3:
	movb	%cl, %al
	jmp	.LBB1_138
.LBB1_4:                                #   in Loop: Header=BB1_7 Depth=1
	leaq	.L.str.3(%rip), %rax
	cmpb	$0, (%r11,%rax)
	je	.LBB1_103
.LBB1_5:                                #   in Loop: Header=BB1_7 Depth=1
	movl	$6, (%rdi)
	movq	-8(%rsp), %r9                   # 8-byte Reload
	leaq	hp_execute.V(%rip), %rbx
	.p2align	4
.LBB1_6:                                #   in Loop: Header=BB1_7 Depth=1
	incq	%rcx
	cmpq	%rdx, %rcx
	je	.LBB1_132
.LBB1_7:                                # =>This Loop Header: Depth=1
                                        #     Child Loop BB1_35 Depth 2
                                        #     Child Loop BB1_58 Depth 2
                                        #     Child Loop BB1_64 Depth 2
                                        #     Child Loop BB1_105 Depth 2
                                        #     Child Loop BB1_96 Depth 2
                                        #     Child Loop BB1_111 Depth 2
	movzbl	(%rsi,%rcx), %r13d
	incl	%r10d
	movl	%r10d, 2152(%rdi)
	cmpl	$8192, %r10d                    # imm = 0x2000
	ja	.LBB1_136
# %bb.8:                                #   in Loop: Header=BB1_7 Depth=1
	movl	(%rdi), %r11d
	cmpl	$5, %r11d
	jg	.LBB1_16
# %bb.9:                                #   in Loop: Header=BB1_7 Depth=1
	cmpl	$2, %r11d
	jg	.LBB1_24
# %bb.10:                               #   in Loop: Header=BB1_7 Depth=1
	testl	%r11d, %r11d
	je	.LBB1_49
# %bb.11:                               #   in Loop: Header=BB1_7 Depth=1
	cmpl	$1, %r11d
	je	.LBB1_39
# %bb.12:                               #   in Loop: Header=BB1_7 Depth=1
	cmpl	$2, %r11d
	jne	.LBB1_136
# %bb.13:                               #   in Loop: Header=BB1_7 Depth=1
	movzbl	2156(%rdi), %eax
	cmpb	(%rax,%rbx), %r13b
	jne	.LBB1_136
# %bb.14:                               #   in Loop: Header=BB1_7 Depth=1
	incb	%al
	movb	%al, 2156(%rdi)
	cmpb	$7, %al
	jne	.LBB1_6
# %bb.15:                               #   in Loop: Header=BB1_7 Depth=1
	movl	$3, (%rdi)
	jmp	.LBB1_6
	.p2align	4
.LBB1_16:                               #   in Loop: Header=BB1_7 Depth=1
	cmpl	$8, %r11d
	jg	.LBB1_29
# %bb.17:                               #   in Loop: Header=BB1_7 Depth=1
	cmpl	$6, %r11d
	je	.LBB1_47
# %bb.18:                               #   in Loop: Header=BB1_7 Depth=1
	cmpl	$7, %r11d
	je	.LBB1_37
# %bb.19:                               #   in Loop: Header=BB1_7 Depth=1
	cmpl	$8, %r11d
	jne	.LBB1_136
# %bb.20:                               #   in Loop: Header=BB1_7 Depth=1
	cmpl	$12, %r13d
	jle	.LBB1_88
# %bb.21:                               #   in Loop: Header=BB1_7 Depth=1
	cmpl	$13, %r13d
	je	.LBB1_91
# %bb.22:                               #   in Loop: Header=BB1_7 Depth=1
	cmpl	$32, %r13d
	je	.LBB1_6
# %bb.23:                               #   in Loop: Header=BB1_7 Depth=1
	cmpl	$127, %r13d
	jne	.LBB1_90
	jmp	.LBB1_136
	.p2align	4
.LBB1_24:                               #   in Loop: Header=BB1_7 Depth=1
	cmpl	$3, %r11d
	je	.LBB1_52
# %bb.25:                               #   in Loop: Header=BB1_7 Depth=1
	cmpl	$4, %r11d
	je	.LBB1_42
# %bb.26:                               #   in Loop: Header=BB1_7 Depth=1
	cmpl	$5, %r11d
	jne	.LBB1_136
# %bb.27:                               #   in Loop: Header=BB1_7 Depth=1
	cmpl	$10, %r13d
	jne	.LBB1_136
# %bb.28:                               #   in Loop: Header=BB1_7 Depth=1
	movl	$6, (%rdi)
	jmp	.LBB1_6
.LBB1_29:                               #   in Loop: Header=BB1_7 Depth=1
	cmpl	$9, %r11d
	je	.LBB1_44
# %bb.30:                               #   in Loop: Header=BB1_7 Depth=1
	cmpl	$10, %r11d
	jne	.LBB1_133
# %bb.31:                               #   in Loop: Header=BB1_7 Depth=1
	cmpl	$10, %r13d
	jne	.LBB1_136
# %bb.32:                               #   in Loop: Header=BB1_7 Depth=1
	movq	72(%rdi), %r13
	movq	80(%rdi), %rbp
	testq	%rbp, %rbp
	je	.LBB1_54
# %bb.33:                               #   in Loop: Header=BB1_7 Depth=1
	movq	-16(%rsp), %rax                 # 8-byte Reload
	addq	%r13, %rax
	jmp	.LBB1_35
	.p2align	4
.LBB1_34:                               #   in Loop: Header=BB1_35 Depth=2
	decq	%rbp
	je	.LBB1_54
.LBB1_35:                               #   Parent Loop BB1_7 Depth=1
                                        # =>  This Inner Loop Header: Depth=2
	movzbl	(%rax,%rbp), %r8d
	cmpl	$32, %r8d
	je	.LBB1_34
# %bb.36:                               #   in Loop: Header=BB1_35 Depth=2
	cmpl	$9, %r8d
	je	.LBB1_34
	jmp	.LBB1_55
.LBB1_37:                               #   in Loop: Header=BB1_7 Depth=1
	cmpl	$58, %r13d
	jne	.LBB1_67
# %bb.38:                               #   in Loop: Header=BB1_7 Depth=1
	movl	$8, (%rdi)
	leaq	1(%rcx), %rax
	movq	%rax, 72(%rdi)
	movq	$0, 80(%rdi)
	jmp	.LBB1_6
.LBB1_39:                               #   in Loop: Header=BB1_7 Depth=1
	cmpl	$32, %r13d
	jne	.LBB1_72
# %bb.40:                               #   in Loop: Header=BB1_7 Depth=1
	cmpq	$0, 40(%rdi)
	je	.LBB1_136
# %bb.41:                               #   in Loop: Header=BB1_7 Depth=1
	movl	$2, (%rdi)
	movb	$0, 2156(%rdi)
	jmp	.LBB1_6
.LBB1_42:                               #   in Loop: Header=BB1_7 Depth=1
	cmpl	$13, %r13d
	jne	.LBB1_136
# %bb.43:                               #   in Loop: Header=BB1_7 Depth=1
	movl	$5, (%rdi)
	jmp	.LBB1_6
.LBB1_44:                               #   in Loop: Header=BB1_7 Depth=1
	cmpl	$13, %r13d
	je	.LBB1_92
# %bb.45:                               #   in Loop: Header=BB1_7 Depth=1
	cmpl	$10, %r13d
	je	.LBB1_136
# %bb.46:                               #   in Loop: Header=BB1_7 Depth=1
	incq	80(%rdi)
	jmp	.LBB1_6
.LBB1_47:                               #   in Loop: Header=BB1_7 Depth=1
	cmpl	$13, %r13d
	jne	.LBB1_77
# %bb.48:                               #   in Loop: Header=BB1_7 Depth=1
	movl	$11, (%rdi)
	jmp	.LBB1_6
.LBB1_49:                               #   in Loop: Header=BB1_7 Depth=1
	cmpl	$32, %r13d
	jne	.LBB1_82
# %bb.50:                               #   in Loop: Header=BB1_7 Depth=1
	cmpq	$0, 24(%rdi)
	je	.LBB1_136
# %bb.51:                               #   in Loop: Header=BB1_7 Depth=1
	movl	$1, (%rdi)
	leaq	1(%rcx), %rax
	movq	%rax, 32(%rdi)
	jmp	.LBB1_6
.LBB1_52:                               #   in Loop: Header=BB1_7 Depth=1
	leal	-58(%r13), %eax
	cmpb	$-10, %al
	jb	.LBB1_136
# %bb.53:                               #   in Loop: Header=BB1_7 Depth=1
	movl	%r13d, %eax
	addl	$-48, %eax
	movl	%eax, 48(%rdi)
	xorl	%eax, %eax
	cmpb	$49, %r13b
	setae	%al
	movl	%eax, 2140(%rdi)
	movl	$4, (%rdi)
	jmp	.LBB1_6
.LBB1_54:                               #   in Loop: Header=BB1_7 Depth=1
	xorl	%ebp, %ebp
.LBB1_55:                               #   in Loop: Header=BB1_7 Depth=1
	movl	2136(%rdi), %eax
	cmpq	$63, %rax
	ja	.LBB1_136
# %bb.56:                               #   in Loop: Header=BB1_7 Depth=1
	movq	%rdx, -24(%rsp)                 # 8-byte Spill
	movq	56(%rdi), %rbx
	movq	64(%rdi), %r11
	movq	%rax, %r8
	shlq	$4, %r8
	movq	%rbx, (%r9,%r8)
	movq	%r9, -8(%rsp)                   # 8-byte Spill
	movq	%r11, 8(%r9,%r8)
	movq	%r13, (%r12,%r8)
	movq	%r12, %rdx
	movq	%rbp, 8(%r12,%r8)
	incl	%eax
	movl	%eax, 2136(%rdi)
	testq	%r11, %r11
	je	.LBB1_61
# %bb.57:                               #   in Loop: Header=BB1_7 Depth=1
	leaq	(%rsi,%rbx), %r12
	xorl	%eax, %eax
	.p2align	4
.LBB1_58:                               #   Parent Loop BB1_7 Depth=1
                                        # =>  This Inner Loop Header: Depth=2
	movzbl	(%r12,%rax), %r9d
	leal	-65(%r9), %r8d
	leal	32(%r9), %r15d
	cmpb	$26, %r8b
	movzbl	%r15b, %r8d
	cmovael	%r9d, %r8d
	leaq	.L.str(%rip), %r9
	movzbl	(%rax,%r9), %r15d
	leal	-65(%r15), %r9d
	leal	32(%r15), %r14d
	cmpb	$26, %r9b
	movzbl	%r14b, %r9d
	cmovael	%r15d, %r9d
	testb	%r9b, %r9b
	je	.LBB1_62
# %bb.59:                               #   in Loop: Header=BB1_58 Depth=2
	cmpb	%r9b, %r8b
	jne	.LBB1_62
# %bb.60:                               #   in Loop: Header=BB1_58 Depth=2
	incq	%rax
	cmpq	%rax, %r11
	jne	.LBB1_58
.LBB1_61:                               #   in Loop: Header=BB1_7 Depth=1
	leaq	.L.str(%rip), %rax
	cmpb	$0, (%r11,%rax)
	je	.LBB1_93
.LBB1_62:                               #   in Loop: Header=BB1_7 Depth=1
	testq	%r11, %r11
	movq	%rdx, %r12
	movq	-24(%rsp), %rdx                 # 8-byte Reload
	je	.LBB1_4
# %bb.63:                               #   in Loop: Header=BB1_7 Depth=1
	addq	%rsi, %rbx
	xorl	%eax, %eax
	.p2align	4
.LBB1_64:                               #   Parent Loop BB1_7 Depth=1
                                        # =>  This Inner Loop Header: Depth=2
	movzbl	(%rbx,%rax), %r9d
	leal	-65(%r9), %r8d
	leal	32(%r9), %r14d
	cmpb	$26, %r8b
	movzbl	%r14b, %r8d
	cmovael	%r9d, %r8d
	leaq	.L.str.3(%rip), %r9
	movzbl	(%rax,%r9), %r14d
	leal	-65(%r14), %r9d
	leal	32(%r14), %r15d
	cmpb	$26, %r9b
	movzbl	%r15b, %r9d
	cmovael	%r14d, %r9d
	testb	%r9b, %r9b
	je	.LBB1_5
# %bb.65:                               #   in Loop: Header=BB1_64 Depth=2
	cmpb	%r9b, %r8b
	jne	.LBB1_5
# %bb.66:                               #   in Loop: Header=BB1_64 Depth=2
	incq	%rax
	cmpq	%rax, %r11
	jne	.LBB1_64
	jmp	.LBB1_4
.LBB1_67:                               #   in Loop: Header=BB1_7 Depth=1
	leal	-48(%r13), %eax
	cmpb	$10, %al
	jb	.LBB1_71
# %bb.68:                               #   in Loop: Header=BB1_7 Depth=1
	movl	%r13d, %eax
	andb	$-33, %al
	addb	$-65, %al
	cmpb	$26, %al
	jb	.LBB1_71
# %bb.69:                               #   in Loop: Header=BB1_7 Depth=1
	leal	-33(%r13), %eax
	cmpl	$63, %eax
	ja	.LBB1_123
# %bb.70:                               #   in Loop: Header=BB1_7 Depth=1
	movabsq	$-2305843009213680003, %r8      # imm = 0xE00000000000367D
	btq	%rax, %r8
	jae	.LBB1_123
.LBB1_71:                               #   in Loop: Header=BB1_7 Depth=1
	incq	64(%rdi)
	jmp	.LBB1_6
.LBB1_72:                               #   in Loop: Header=BB1_7 Depth=1
	cmpb	$33, %r13b
	jb	.LBB1_136
# %bb.73:                               #   in Loop: Header=BB1_7 Depth=1
	cmpb	$127, %r13b
	je	.LBB1_136
# %bb.74:                               #   in Loop: Header=BB1_7 Depth=1
	movq	40(%rdi), %rax
	testq	%rax, %rax
	jne	.LBB1_76
# %bb.75:                               #   in Loop: Header=BB1_7 Depth=1
	movq	%rcx, 32(%rdi)
.LBB1_76:                               #   in Loop: Header=BB1_7 Depth=1
	incq	%rax
	movq	%rax, 40(%rdi)
	jmp	.LBB1_6
.LBB1_77:                               #   in Loop: Header=BB1_7 Depth=1
	leal	-48(%r13), %eax
	cmpb	$10, %al
	jb	.LBB1_81
# %bb.78:                               #   in Loop: Header=BB1_7 Depth=1
	movl	%r13d, %eax
	andb	$-33, %al
	addb	$-65, %al
	cmpb	$26, %al
	jb	.LBB1_81
# %bb.79:                               #   in Loop: Header=BB1_7 Depth=1
	leal	-33(%r13), %eax
	cmpl	$63, %eax
	ja	.LBB1_125
# %bb.80:                               #   in Loop: Header=BB1_7 Depth=1
	movabsq	$-2305843009213680003, %r8      # imm = 0xE00000000000367D
	btq	%rax, %r8
	jae	.LBB1_125
.LBB1_81:                               #   in Loop: Header=BB1_7 Depth=1
	movq	%rcx, 56(%rdi)
	movq	$1, 64(%rdi)
	movl	$7, (%rdi)
	jmp	.LBB1_6
.LBB1_82:                               #   in Loop: Header=BB1_7 Depth=1
	leal	-48(%r13), %eax
	cmpb	$10, %al
	jb	.LBB1_86
# %bb.83:                               #   in Loop: Header=BB1_7 Depth=1
	movl	%r13d, %eax
	andb	$-33, %al
	addb	$-65, %al
	cmpb	$26, %al
	jb	.LBB1_86
# %bb.84:                               #   in Loop: Header=BB1_7 Depth=1
	movl	%r13d, %eax
	addl	$-33, %eax
	cmpl	$63, %eax
	ja	.LBB1_127
# %bb.85:                               #   in Loop: Header=BB1_7 Depth=1
	movabsq	$-2305843009213680003, %r8      # imm = 0xE00000000000367D
	btq	%rax, %r8
	jae	.LBB1_127
.LBB1_86:                               #   in Loop: Header=BB1_7 Depth=1
	movq	24(%rdi), %rax
	testq	%rax, %rax
	je	.LBB1_102
# %bb.87:                               #   in Loop: Header=BB1_7 Depth=1
	incq	%rax
	movq	%rax, 24(%rdi)
	cmpq	$16, %rax
	jbe	.LBB1_6
	jmp	.LBB1_136
.LBB1_88:                               #   in Loop: Header=BB1_7 Depth=1
	cmpl	$9, %r13d
	je	.LBB1_6
# %bb.89:                               #   in Loop: Header=BB1_7 Depth=1
	cmpl	$10, %r13d
	je	.LBB1_136
.LBB1_90:                               #   in Loop: Header=BB1_7 Depth=1
	movq	%rcx, 72(%rdi)
	movq	$1, 80(%rdi)
	movl	$9, (%rdi)
	jmp	.LBB1_6
.LBB1_91:                               #   in Loop: Header=BB1_7 Depth=1
	movq	%rcx, 72(%rdi)
	movq	$0, 80(%rdi)
.LBB1_92:                               #   in Loop: Header=BB1_7 Depth=1
	movl	$10, (%rdi)
	jmp	.LBB1_6
.LBB1_93:                               #   in Loop: Header=BB1_7 Depth=1
	cmpq	$5, %rbp
	movq	%rdx, %r12
	movq	-24(%rsp), %rdx                 # 8-byte Reload
	jb	.LBB1_5
# %bb.94:                               #   in Loop: Header=BB1_7 Depth=1
	addq	%rsi, %r13
	movl	$5, %r11d
	jmp	.LBB1_96
	.p2align	4
.LBB1_95:                               #   in Loop: Header=BB1_96 Depth=2
	incq	%r11
	cmpq	%rbp, %r11
	ja	.LBB1_108
.LBB1_96:                               #   Parent Loop BB1_7 Depth=1
                                        # =>  This Inner Loop Header: Depth=2
	movzbl	-5(%r13,%r11), %eax
	leal	-65(%rax), %r8d
	leal	32(%rax), %r9d
	cmpb	$26, %r8b
	movzbl	%r9b, %r8d
	cmovael	%eax, %r8d
	cmpb	$99, %r8b
	jne	.LBB1_95
# %bb.97:                               #   in Loop: Header=BB1_96 Depth=2
	movzbl	-4(%r13,%r11), %eax
	leal	-65(%rax), %r8d
	leal	32(%rax), %r9d
	cmpb	$26, %r8b
	movzbl	%r9b, %r8d
	cmovael	%eax, %r8d
	cmpb	$108, %r8b
	jne	.LBB1_95
# %bb.98:                               #   in Loop: Header=BB1_96 Depth=2
	movzbl	-3(%r13,%r11), %eax
	leal	-65(%rax), %r8d
	leal	32(%rax), %r9d
	cmpb	$26, %r8b
	movzbl	%r9b, %r8d
	cmovael	%eax, %r8d
	cmpb	$111, %r8b
	jne	.LBB1_95
# %bb.99:                               #   in Loop: Header=BB1_96 Depth=2
	movzbl	-2(%r13,%r11), %eax
	leal	-65(%rax), %r8d
	leal	32(%rax), %r9d
	cmpb	$26, %r8b
	movzbl	%r9b, %r8d
	cmovael	%eax, %r8d
	cmpb	$115, %r8b
	jne	.LBB1_95
# %bb.100:                              #   in Loop: Header=BB1_96 Depth=2
	movzbl	-1(%r13,%r11), %eax
	leal	-65(%rax), %r8d
	leal	32(%rax), %r9d
	cmpb	$26, %r8b
	movzbl	%r9b, %r8d
	cmovael	%eax, %r8d
	cmpb	$101, %r8b
	jne	.LBB1_95
# %bb.101:                              #   in Loop: Header=BB1_7 Depth=1
	movl	$0, 2140(%rdi)
	jmp	.LBB1_5
.LBB1_102:                              #   in Loop: Header=BB1_7 Depth=1
	movq	%rcx, 16(%rdi)
	movq	$1, 24(%rdi)
	jmp	.LBB1_6
.LBB1_103:                              #   in Loop: Header=BB1_7 Depth=1
	testq	%rbp, %rbp
	je	.LBB1_130
# %bb.104:                              #   in Loop: Header=BB1_7 Depth=1
	addq	%rsi, %r13
	xorl	%r11d, %r11d
	xorl	%eax, %eax
	movabsq	$922337203685477579, %rbx       # imm = 0xCCCCCCCCCCCCCCB
	.p2align	4
.LBB1_105:                              #   Parent Loop BB1_7 Depth=1
                                        # =>  This Inner Loop Header: Depth=2
	movzbl	(%r13,%r11), %r8d
	leal	-58(%r8), %r9d
	cmpb	$-10, %r9b
	jb	.LBB1_130
# %bb.106:                              #   in Loop: Header=BB1_105 Depth=2
	cmpq	%rbx, %rax
	jg	.LBB1_130
# %bb.107:                              #   in Loop: Header=BB1_105 Depth=2
	leaq	(%rax,%rax,4), %rax
	andl	$15, %r8d
	leaq	(%r8,%rax,2), %rax
	incq	%r11
	cmpq	%r11, %rbp
	jne	.LBB1_105
	jmp	.LBB1_131
.LBB1_130:                              #   in Loop: Header=BB1_7 Depth=1
	movq	$-1, %rax
.LBB1_131:                              #   in Loop: Header=BB1_7 Depth=1
	movq	%rax, 2144(%rdi)
	jmp	.LBB1_5
.LBB1_108:                              #   in Loop: Header=BB1_7 Depth=1
	cmpq	$10, %rbp
	jb	.LBB1_5
# %bb.109:                              #   in Loop: Header=BB1_7 Depth=1
	movl	$10, %r11d
	jmp	.LBB1_111
	.p2align	4
.LBB1_110:                              #   in Loop: Header=BB1_111 Depth=2
	incq	%r11
	cmpq	%rbp, %r11
	ja	.LBB1_5
.LBB1_111:                              #   Parent Loop BB1_7 Depth=1
                                        # =>  This Inner Loop Header: Depth=2
	movzbl	-10(%r13,%r11), %eax
	leal	-65(%rax), %r8d
	leal	32(%rax), %r9d
	cmpb	$26, %r8b
	movzbl	%r9b, %r8d
	cmovael	%eax, %r8d
	cmpb	$107, %r8b
	jne	.LBB1_110
# %bb.112:                              #   in Loop: Header=BB1_111 Depth=2
	movzbl	-9(%r13,%r11), %eax
	leal	-65(%rax), %r8d
	leal	32(%rax), %r9d
	cmpb	$26, %r8b
	movzbl	%r9b, %r8d
	cmovael	%eax, %r8d
	cmpb	$101, %r8b
	jne	.LBB1_110
# %bb.113:                              #   in Loop: Header=BB1_111 Depth=2
	movzbl	-8(%r13,%r11), %eax
	leal	-65(%rax), %r8d
	leal	32(%rax), %r9d
	cmpb	$26, %r8b
	movzbl	%r9b, %r8d
	cmovael	%eax, %r8d
	cmpb	$101, %r8b
	jne	.LBB1_110
# %bb.114:                              #   in Loop: Header=BB1_111 Depth=2
	movzbl	-7(%r13,%r11), %eax
	leal	-65(%rax), %r8d
	leal	32(%rax), %r9d
	cmpb	$26, %r8b
	movzbl	%r9b, %r8d
	cmovael	%eax, %r8d
	cmpb	$112, %r8b
	jne	.LBB1_110
# %bb.115:                              #   in Loop: Header=BB1_111 Depth=2
	movzbl	-6(%r13,%r11), %eax
	leal	-65(%rax), %r8d
	leal	32(%rax), %r9d
	cmpb	$26, %r8b
	movzbl	%r9b, %r8d
	cmovael	%eax, %r8d
	cmpb	$45, %r8b
	jne	.LBB1_110
# %bb.116:                              #   in Loop: Header=BB1_111 Depth=2
	movzbl	-5(%r13,%r11), %eax
	leal	-65(%rax), %r8d
	leal	32(%rax), %r9d
	cmpb	$26, %r8b
	movzbl	%r9b, %r8d
	cmovael	%eax, %r8d
	cmpb	$97, %r8b
	jne	.LBB1_110
# %bb.117:                              #   in Loop: Header=BB1_111 Depth=2
	movzbl	-4(%r13,%r11), %eax
	leal	-65(%rax), %r8d
	leal	32(%rax), %r9d
	cmpb	$26, %r8b
	movzbl	%r9b, %r8d
	cmovael	%eax, %r8d
	cmpb	$108, %r8b
	jne	.LBB1_110
# %bb.118:                              #   in Loop: Header=BB1_111 Depth=2
	movzbl	-3(%r13,%r11), %eax
	leal	-65(%rax), %r8d
	leal	32(%rax), %r9d
	cmpb	$26, %r8b
	movzbl	%r9b, %r8d
	cmovael	%eax, %r8d
	cmpb	$105, %r8b
	jne	.LBB1_110
# %bb.119:                              #   in Loop: Header=BB1_111 Depth=2
	movzbl	-2(%r13,%r11), %eax
	leal	-65(%rax), %r8d
	leal	32(%rax), %r9d
	cmpb	$26, %r8b
	movzbl	%r9b, %r8d
	cmovael	%eax, %r8d
	cmpb	$118, %r8b
	jne	.LBB1_110
# %bb.120:                              #   in Loop: Header=BB1_111 Depth=2
	movzbl	-1(%r13,%r11), %eax
	leal	-65(%rax), %r8d
	leal	32(%rax), %r9d
	cmpb	$26, %r8b
	movzbl	%r9b, %r8d
	cmovael	%eax, %r8d
	cmpb	$101, %r8b
	jne	.LBB1_110
# %bb.121:                              #   in Loop: Header=BB1_7 Depth=1
	movl	$1, 2140(%rdi)
	jmp	.LBB1_5
.LBB1_123:                              #   in Loop: Header=BB1_7 Depth=1
	cmpl	$126, %r13d
	je	.LBB1_71
# %bb.124:                              #   in Loop: Header=BB1_7 Depth=1
	cmpl	$124, %r13d
	je	.LBB1_71
	jmp	.LBB1_136
.LBB1_125:                              #   in Loop: Header=BB1_7 Depth=1
	cmpl	$126, %r13d
	je	.LBB1_81
# %bb.126:                              #   in Loop: Header=BB1_7 Depth=1
	cmpl	$124, %r13d
	je	.LBB1_81
	jmp	.LBB1_136
.LBB1_127:                              #   in Loop: Header=BB1_7 Depth=1
	cmpl	$126, %r13d
	je	.LBB1_86
# %bb.128:                              #   in Loop: Header=BB1_7 Depth=1
	cmpl	$124, %r13d
	je	.LBB1_86
	jmp	.LBB1_136
.LBB1_132:
	xorl	%eax, %eax
	movq	%rdx, 8(%rdi)
	jmp	.LBB1_138
.LBB1_133:
	cmpl	$11, %r11d
	jne	.LBB1_136
# %bb.134:
	cmpl	$10, %r13d
	jne	.LBB1_136
# %bb.135:
	movl	$12, (%rdi)
	incq	%rcx
	movl	$1, %eax
	movq	%rcx, 8(%rdi)
	jmp	.LBB1_138
.LBB1_136:
	movl	$13, (%rdi)
	movq	%rcx, 8(%rdi)
.LBB1_137:
	movl	$-1, %eax
.LBB1_138:
	popq	%rbx
	popq	%r12
	popq	%r13
	popq	%r14
	popq	%r15
	popq	%rbp
	retq
.Lfunc_end1:
	.size	hp_execute, .Lfunc_end1-hp_execute
                                        # -- End function
	.type	hp_execute.V,@object            # @hp_execute.V
	.section	.rodata.str1.1,"aMS",@progbits,1
hp_execute.V:
	.asciz	"HTTP/1."
	.size	hp_execute.V, 8

	.type	.L.str,@object                  # @.str
.L.str:
	.asciz	"connection"
	.size	.L.str, 11

	.type	.L.str.3,@object                # @.str.3
.L.str.3:
	.asciz	"content-length"
	.size	.L.str.3, 15

	.ident	"clang version 20.1.8"
	.section	".note.GNU-stack","",@progbits
	.addrsig
