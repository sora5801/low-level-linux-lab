	.file	"http_parser.c"
	.text
	.globl	hp_slice_ci_eq                  # -- Begin function hp_slice_ci_eq
	.p2align	4
	.type	hp_slice_ci_eq,@function
hp_slice_ci_eq:                         # @hp_slice_ci_eq
# %bb.0:
	pushq	%rbp
	movq	%rsp, %rbp
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
	popq	%rbp
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
	movq	%rsp, %rbp
	pushq	%r15
	pushq	%r14
	pushq	%r13
	pushq	%r12
	pushq	%rbx
	movq	8(%rdi), %rax
	cmpq	%rdx, %rax
	jae	.LBB1_137
# %bb.1:
	leaq	88(%rdi), %r14
	leaq	1112(%rdi), %rbx
	movl	2152(%rdi), %r9d
	movabsq	$-2305843009213680003, %r12     # imm = 0xE00000000000367D
	leaq	hp_execute.V(%rip), %r8
	.p2align	4
.LBB1_2:                                # =>This Loop Header: Depth=1
                                        #     Child Loop BB1_51 Depth 2
                                        #     Child Loop BB1_85 Depth 2
                                        #     Child Loop BB1_91 Depth 2
                                        #     Child Loop BB1_99 Depth 2
                                        #     Child Loop BB1_108 Depth 2
                                        #       Child Loop BB1_109 Depth 3
                                        #     Child Loop BB1_116 Depth 2
                                        #       Child Loop BB1_117 Depth 3
	movzbl	(%rsi,%rax), %r10d
	incl	%r9d
	movl	%r9d, 2152(%rdi)
	cmpl	$8193, %r9d                     # imm = 0x2001
	jb	.LBB1_4
# %bb.3:                                #   in Loop: Header=BB1_2 Depth=1
	movl	$13, (%rdi)
	movl	$2, %r13d
	jmp	.LBB1_78
	.p2align	4
.LBB1_4:                                #   in Loop: Header=BB1_2 Depth=1
	movl	(%rdi), %r11d
	cmpl	$5, %r11d
	jg	.LBB1_12
# %bb.5:                                #   in Loop: Header=BB1_2 Depth=1
	cmpl	$2, %r11d
	jg	.LBB1_21
# %bb.6:                                #   in Loop: Header=BB1_2 Depth=1
	testl	%r11d, %r11d
	je	.LBB1_31
# %bb.7:                                #   in Loop: Header=BB1_2 Depth=1
	cmpl	$1, %r11d
	je	.LBB1_41
# %bb.8:                                #   in Loop: Header=BB1_2 Depth=1
	cmpl	$2, %r11d
	jne	.LBB1_76
# %bb.9:                                #   in Loop: Header=BB1_2 Depth=1
	movzbl	2156(%rdi), %ecx
	cmpb	(%rcx,%r8), %r10b
	jne	.LBB1_76
# %bb.10:                               #   in Loop: Header=BB1_2 Depth=1
	incb	%cl
	movb	%cl, 2156(%rdi)
	xorl	%r13d, %r13d
	cmpb	$7, %cl
	jne	.LBB1_78
# %bb.11:                               #   in Loop: Header=BB1_2 Depth=1
	movl	$3, (%rdi)
	jmp	.LBB1_78
	.p2align	4
.LBB1_12:                               #   in Loop: Header=BB1_2 Depth=1
	cmpl	$8, %r11d
	jg	.LBB1_26
# %bb.13:                               #   in Loop: Header=BB1_2 Depth=1
	cmpl	$6, %r11d
	je	.LBB1_34
# %bb.14:                               #   in Loop: Header=BB1_2 Depth=1
	cmpl	$7, %r11d
	je	.LBB1_44
# %bb.15:                               #   in Loop: Header=BB1_2 Depth=1
	cmpl	$8, %r11d
	jne	.LBB1_76
# %bb.16:                               #   in Loop: Header=BB1_2 Depth=1
	xorl	%r13d, %r13d
	cmpl	$12, %r10d
	jle	.LBB1_74
# %bb.17:                               #   in Loop: Header=BB1_2 Depth=1
	cmpl	$127, %r10d
	je	.LBB1_76
# %bb.18:                               #   in Loop: Header=BB1_2 Depth=1
	cmpl	$32, %r10d
	je	.LBB1_78
# %bb.19:                               #   in Loop: Header=BB1_2 Depth=1
	cmpl	$13, %r10d
	jne	.LBB1_105
# %bb.20:                               #   in Loop: Header=BB1_2 Depth=1
	movq	%rax, 72(%rdi)
	movq	$0, 80(%rdi)
	movl	$10, (%rdi)
	jmp	.LBB1_78
.LBB1_21:                               #   in Loop: Header=BB1_2 Depth=1
	cmpl	$3, %r11d
	je	.LBB1_36
# %bb.22:                               #   in Loop: Header=BB1_2 Depth=1
	cmpl	$4, %r11d
	je	.LBB1_46
# %bb.23:                               #   in Loop: Header=BB1_2 Depth=1
	cmpl	$5, %r11d
	jne	.LBB1_76
# %bb.24:                               #   in Loop: Header=BB1_2 Depth=1
	cmpl	$10, %r10d
	jne	.LBB1_76
# %bb.25:                               #   in Loop: Header=BB1_2 Depth=1
	movl	$6, (%rdi)
	xorl	%r13d, %r13d
	jmp	.LBB1_78
.LBB1_26:                               #   in Loop: Header=BB1_2 Depth=1
	cmpl	$9, %r11d
	je	.LBB1_38
# %bb.27:                               #   in Loop: Header=BB1_2 Depth=1
	cmpl	$10, %r11d
	je	.LBB1_48
# %bb.28:                               #   in Loop: Header=BB1_2 Depth=1
	cmpl	$11, %r11d
	jne	.LBB1_76
# %bb.29:                               #   in Loop: Header=BB1_2 Depth=1
	cmpl	$10, %r10d
	jne	.LBB1_76
# %bb.30:                               #   in Loop: Header=BB1_2 Depth=1
	movl	$12, (%rdi)
	incq	%rax
	jmp	.LBB1_77
.LBB1_31:                               #   in Loop: Header=BB1_2 Depth=1
	cmpl	$32, %r10d
	jne	.LBB1_55
# %bb.32:                               #   in Loop: Header=BB1_2 Depth=1
	cmpq	$0, 24(%rdi)
	je	.LBB1_76
# %bb.33:                               #   in Loop: Header=BB1_2 Depth=1
	movl	$1, (%rdi)
	leaq	1(%rax), %rcx
	movq	%rcx, 32(%rdi)
	xorl	%r13d, %r13d
	jmp	.LBB1_78
.LBB1_34:                               #   in Loop: Header=BB1_2 Depth=1
	cmpl	$13, %r10d
	jne	.LBB1_62
# %bb.35:                               #   in Loop: Header=BB1_2 Depth=1
	movl	$11, (%rdi)
	xorl	%r13d, %r13d
	jmp	.LBB1_78
.LBB1_36:                               #   in Loop: Header=BB1_2 Depth=1
	leal	-58(%r10), %ecx
	cmpb	$-11, %cl
	jbe	.LBB1_76
# %bb.37:                               #   in Loop: Header=BB1_2 Depth=1
	movl	%r10d, %ecx
	addl	$-48, %ecx
	movl	%ecx, 48(%rdi)
	xorl	%ecx, %ecx
	cmpb	$49, %r10b
	setae	%cl
	movl	%ecx, 2140(%rdi)
	movl	$4, (%rdi)
	xorl	%r13d, %r13d
	jmp	.LBB1_78
.LBB1_38:                               #   in Loop: Header=BB1_2 Depth=1
	cmpl	$10, %r10d
	je	.LBB1_76
# %bb.39:                               #   in Loop: Header=BB1_2 Depth=1
	cmpl	$13, %r10d
	jne	.LBB1_80
# %bb.40:                               #   in Loop: Header=BB1_2 Depth=1
	movl	$10, (%rdi)
	xorl	%r13d, %r13d
	jmp	.LBB1_78
.LBB1_41:                               #   in Loop: Header=BB1_2 Depth=1
	cmpl	$32, %r10d
	jne	.LBB1_67
# %bb.42:                               #   in Loop: Header=BB1_2 Depth=1
	cmpq	$0, 40(%rdi)
	je	.LBB1_76
# %bb.43:                               #   in Loop: Header=BB1_2 Depth=1
	movl	$2, (%rdi)
	movb	$0, 2156(%rdi)
	xorl	%r13d, %r13d
	jmp	.LBB1_78
.LBB1_44:                               #   in Loop: Header=BB1_2 Depth=1
	cmpl	$58, %r10d
	jne	.LBB1_69
# %bb.45:                               #   in Loop: Header=BB1_2 Depth=1
	movl	$8, (%rdi)
	leaq	1(%rax), %rcx
	movq	%rcx, 72(%rdi)
	movq	$0, 80(%rdi)
	xorl	%r13d, %r13d
	jmp	.LBB1_78
.LBB1_46:                               #   in Loop: Header=BB1_2 Depth=1
	cmpl	$13, %r10d
	jne	.LBB1_76
# %bb.47:                               #   in Loop: Header=BB1_2 Depth=1
	movl	$5, (%rdi)
	xorl	%r13d, %r13d
	jmp	.LBB1_78
.LBB1_48:                               #   in Loop: Header=BB1_2 Depth=1
	cmpl	$10, %r10d
	jne	.LBB1_76
# %bb.49:                               #   in Loop: Header=BB1_2 Depth=1
	movq	%rbx, -56(%rbp)                 # 8-byte Spill
	movq	56(%rdi), %rcx
	movq	%rcx, -48(%rbp)                 # 8-byte Spill
	movq	64(%rdi), %r11
	movq	72(%rdi), %rbx
	movq	80(%rdi), %r13
	leaq	(%rsi,%rbx), %r10
	jmp	.LBB1_51
	.p2align	4
.LBB1_50:                               #   in Loop: Header=BB1_51 Depth=2
	decq	%r13
	movb	$1, %cl
	testb	%cl, %cl
	je	.LBB1_82
.LBB1_51:                               #   Parent Loop BB1_2 Depth=1
                                        # =>  This Inner Loop Header: Depth=2
	testq	%r13, %r13
	je	.LBB1_81
# %bb.52:                               #   in Loop: Header=BB1_51 Depth=2
	movzbl	-1(%r10,%r13), %ecx
	cmpl	$32, %ecx
	je	.LBB1_50
# %bb.53:                               #   in Loop: Header=BB1_51 Depth=2
	cmpl	$9, %ecx
	je	.LBB1_50
# %bb.54:                               #   in Loop: Header=BB1_51 Depth=2
	xorl	%ecx, %ecx
	testb	%cl, %cl
	jne	.LBB1_51
	jmp	.LBB1_82
.LBB1_55:                               #   in Loop: Header=BB1_2 Depth=1
	leal	-48(%r10), %ecx
	cmpb	$10, %cl
	jb	.LBB1_59
# %bb.56:                               #   in Loop: Header=BB1_2 Depth=1
	movl	%r10d, %ecx
	andb	$-33, %cl
	addb	$-65, %cl
	cmpb	$26, %cl
	jb	.LBB1_59
# %bb.57:                               #   in Loop: Header=BB1_2 Depth=1
	movl	%r10d, %ecx
	addl	$-33, %ecx
	cmpl	$63, %ecx
	ja	.LBB1_122
# %bb.58:                               #   in Loop: Header=BB1_2 Depth=1
	btq	%rcx, %r12
	jae	.LBB1_122
.LBB1_59:                               #   in Loop: Header=BB1_2 Depth=1
	movq	24(%rdi), %rcx
	testq	%rcx, %rcx
	jne	.LBB1_61
# %bb.60:                               #   in Loop: Header=BB1_2 Depth=1
	movq	%rax, 16(%rdi)
.LBB1_61:                               #   in Loop: Header=BB1_2 Depth=1
	incq	%rcx
	movq	%rcx, 24(%rdi)
	xorl	%r13d, %r13d
	cmpq	$17, %rcx
	jae	.LBB1_76
	jmp	.LBB1_78
.LBB1_62:                               #   in Loop: Header=BB1_2 Depth=1
	leal	-48(%r10), %ecx
	cmpb	$10, %cl
	jb	.LBB1_66
# %bb.63:                               #   in Loop: Header=BB1_2 Depth=1
	movl	%r10d, %ecx
	andb	$-33, %cl
	addb	$-65, %cl
	cmpb	$26, %cl
	jb	.LBB1_66
# %bb.64:                               #   in Loop: Header=BB1_2 Depth=1
	leal	-33(%r10), %ecx
	cmpl	$63, %ecx
	ja	.LBB1_124
# %bb.65:                               #   in Loop: Header=BB1_2 Depth=1
	btq	%rcx, %r12
	jae	.LBB1_124
.LBB1_66:                               #   in Loop: Header=BB1_2 Depth=1
	movq	%rax, 56(%rdi)
	movq	$1, 64(%rdi)
	movl	$7, (%rdi)
	xorl	%r13d, %r13d
	jmp	.LBB1_78
.LBB1_67:                               #   in Loop: Header=BB1_2 Depth=1
	movq	%r8, %r11
	cmpb	$33, %r10b
	setae	%cl
	cmpl	$127, %r10d
	setne	%r8b
	testb	%r8b, %cl
	jne	.LBB1_101
# %bb.68:                               #   in Loop: Header=BB1_2 Depth=1
	movl	$13, (%rdi)
	movl	$6, %r13d
	movq	%r11, %r8
	jmp	.LBB1_78
.LBB1_69:                               #   in Loop: Header=BB1_2 Depth=1
	leal	-48(%r10), %ecx
	cmpb	$10, %cl
	jb	.LBB1_73
# %bb.70:                               #   in Loop: Header=BB1_2 Depth=1
	movl	%r10d, %ecx
	andb	$-33, %cl
	addb	$-65, %cl
	cmpb	$26, %cl
	jb	.LBB1_73
# %bb.71:                               #   in Loop: Header=BB1_2 Depth=1
	leal	-33(%r10), %ecx
	cmpl	$63, %ecx
	ja	.LBB1_126
# %bb.72:                               #   in Loop: Header=BB1_2 Depth=1
	btq	%rcx, %r12
	jae	.LBB1_126
.LBB1_73:                               #   in Loop: Header=BB1_2 Depth=1
	incq	64(%rdi)
	xorl	%r13d, %r13d
	jmp	.LBB1_78
.LBB1_74:                               #   in Loop: Header=BB1_2 Depth=1
	cmpl	$9, %r10d
	je	.LBB1_78
# %bb.75:                               #   in Loop: Header=BB1_2 Depth=1
	cmpl	$10, %r10d
	jne	.LBB1_105
	.p2align	4
.LBB1_76:                               #   in Loop: Header=BB1_2 Depth=1
	movl	$13, (%rdi)
.LBB1_77:                               #   in Loop: Header=BB1_2 Depth=1
	movl	$6, %r13d
.LBB1_78:                               #   in Loop: Header=BB1_2 Depth=1
	testl	%r13d, %r13d
	jne	.LBB1_134
# %bb.79:                               #   in Loop: Header=BB1_2 Depth=1
	incq	%rax
	cmpq	%rdx, %rax
	jb	.LBB1_2
	jmp	.LBB1_137
.LBB1_80:                               #   in Loop: Header=BB1_2 Depth=1
	incq	80(%rdi)
	xorl	%r13d, %r13d
	jmp	.LBB1_78
.LBB1_81:                               #   in Loop: Header=BB1_2 Depth=1
	xorl	%r13d, %r13d
.LBB1_82:                               #   in Loop: Header=BB1_2 Depth=1
	movl	2136(%rdi), %ecx
	cmpq	$63, %rcx
	ja	.LBB1_104
# %bb.83:                               #   in Loop: Header=BB1_2 Depth=1
	movq	%rcx, %r8
	shlq	$4, %r8
	movq	-48(%rbp), %r15                 # 8-byte Reload
	movq	%r15, (%r14,%r8)
	movq	%r14, -64(%rbp)                 # 8-byte Spill
	movq	%r11, 8(%r14,%r8)
	movq	-56(%rbp), %r14                 # 8-byte Reload
	movq	%rbx, (%r14,%r8)
	movq	%r13, 8(%r14,%r8)
	incl	%ecx
	movl	%ecx, 2136(%rdi)
	testq	%r11, %r11
	je	.LBB1_88
# %bb.84:                               #   in Loop: Header=BB1_2 Depth=1
	movq	-48(%rbp), %rcx                 # 8-byte Reload
	leaq	(%rsi,%rcx), %rbx
	xorl	%ecx, %ecx
	.p2align	4
.LBB1_85:                               #   Parent Loop BB1_2 Depth=1
                                        # =>  This Inner Loop Header: Depth=2
	movzbl	(%rbx,%rcx), %r14d
	leal	-65(%r14), %r8d
	leal	32(%r14), %r15d
	cmpb	$26, %r8b
	movzbl	%r15b, %r8d
	cmovael	%r14d, %r8d
	leaq	.L.str(%rip), %r14
	movzbl	(%rcx,%r14), %r15d
	leal	-65(%r15), %r14d
	leal	32(%r15), %r12d
	cmpb	$26, %r14b
	movzbl	%r12b, %r14d
	cmovael	%r15d, %r14d
	testb	%r14b, %r14b
	je	.LBB1_89
# %bb.86:                               #   in Loop: Header=BB1_85 Depth=2
	cmpb	%r14b, %r8b
	jne	.LBB1_89
# %bb.87:                               #   in Loop: Header=BB1_85 Depth=2
	incq	%rcx
	cmpq	%rcx, %r11
	jne	.LBB1_85
.LBB1_88:                               #   in Loop: Header=BB1_2 Depth=1
	leaq	.L.str(%rip), %rcx
	cmpb	$0, (%r11,%rcx)
	je	.LBB1_106
.LBB1_89:                               #   in Loop: Header=BB1_2 Depth=1
	testq	%r11, %r11
	movabsq	$-2305843009213680003, %r12     # imm = 0xE00000000000367D
	je	.LBB1_94
# %bb.90:                               #   in Loop: Header=BB1_2 Depth=1
	addq	%rsi, -48(%rbp)                 # 8-byte Folded Spill
	xorl	%ecx, %ecx
	.p2align	4
.LBB1_91:                               #   Parent Loop BB1_2 Depth=1
                                        # =>  This Inner Loop Header: Depth=2
	movq	-48(%rbp), %r8                  # 8-byte Reload
	movzbl	(%r8,%rcx), %ebx
	leal	-65(%rbx), %r8d
	leal	32(%rbx), %r14d
	cmpb	$26, %r8b
	movzbl	%r14b, %r8d
	cmovael	%ebx, %r8d
	leaq	.L.str.3(%rip), %rbx
	movzbl	(%rcx,%rbx), %r14d
	leal	-65(%r14), %ebx
	leal	32(%r14), %r15d
	cmpb	$26, %bl
	movzbl	%r15b, %ebx
	cmovael	%r14d, %ebx
	testb	%bl, %bl
	je	.LBB1_132
# %bb.92:                               #   in Loop: Header=BB1_91 Depth=2
	cmpb	%bl, %r8b
	jne	.LBB1_132
# %bb.93:                               #   in Loop: Header=BB1_91 Depth=2
	incq	%rcx
	cmpq	%rcx, %r11
	jne	.LBB1_91
.LBB1_94:                               #   in Loop: Header=BB1_2 Depth=1
	leaq	.L.str.3(%rip), %rcx
	cmpb	$0, (%r11,%rcx)
	jne	.LBB1_132
# %bb.95:                               #   in Loop: Header=BB1_2 Depth=1
	xorl	%r11d, %r11d
	testq	%r13, %r13
	setne	%r11b
	je	.LBB1_128
# %bb.96:                               #   in Loop: Header=BB1_2 Depth=1
	decq	%r13
	xorl	%ecx, %ecx
	xorl	%r12d, %r12d
	jmp	.LBB1_99
.LBB1_97:                               #   in Loop: Header=BB1_99 Depth=2
	leaq	(%r12,%r12,4), %r14
	andl	$15, %r8d
	leaq	(%r8,%r14,2), %r12
.LBB1_98:                               #   in Loop: Header=BB1_99 Depth=2
	cmpq	%rcx, %r13
	leaq	1(%rcx), %rcx
	sete	%r8b
	orb	%r8b, %bl
	cmpb	$1, %bl
	je	.LBB1_129
.LBB1_99:                               #   Parent Loop BB1_2 Depth=1
                                        # =>  This Inner Loop Header: Depth=2
	movzbl	(%r10,%rcx), %r8d
	leal	-58(%r8), %ebx
	cmpb	$-10, %bl
	setb	%r14b
	movabsq	$922337203685477579, %rbx       # imm = 0xCCCCCCCCCCCCCCB
	cmpq	%rbx, %r12
	setg	%bl
	orb	%r14b, %bl
	je	.LBB1_97
# %bb.100:                              #   in Loop: Header=BB1_99 Depth=2
	xorl	%r11d, %r11d
	jmp	.LBB1_98
.LBB1_101:                              #   in Loop: Header=BB1_2 Depth=1
	movq	40(%rdi), %rcx
	testq	%rcx, %rcx
	movq	%r11, %r8
	jne	.LBB1_103
# %bb.102:                              #   in Loop: Header=BB1_2 Depth=1
	movq	%rax, 32(%rdi)
.LBB1_103:                              #   in Loop: Header=BB1_2 Depth=1
	incq	%rcx
	movq	%rcx, 40(%rdi)
	xorl	%r13d, %r13d
	jmp	.LBB1_78
.LBB1_104:                              #   in Loop: Header=BB1_2 Depth=1
	movl	$13, (%rdi)
	movl	$6, %r13d
	movq	-56(%rbp), %rbx                 # 8-byte Reload
	jmp	.LBB1_78
.LBB1_105:                              #   in Loop: Header=BB1_2 Depth=1
	movq	%rax, 72(%rdi)
	movq	$1, 80(%rdi)
	movl	$9, (%rdi)
	jmp	.LBB1_78
.LBB1_106:                              #   in Loop: Header=BB1_2 Depth=1
	cmpq	$5, %r13
	jb	.LBB1_114
# %bb.107:                              #   in Loop: Header=BB1_2 Depth=1
	movq	%r10, %r11
	xorl	%r12d, %r12d
.LBB1_108:                              #   Parent Loop BB1_2 Depth=1
                                        # =>  This Loop Header: Depth=2
                                        #       Child Loop BB1_109 Depth 3
	xorl	%ecx, %ecx
	.p2align	4
.LBB1_109:                              #   Parent Loop BB1_2 Depth=1
                                        #     Parent Loop BB1_108 Depth=2
                                        # =>    This Inner Loop Header: Depth=3
	movzbl	(%r11,%rcx), %r8d
	leal	-65(%r8), %ebx
	leal	32(%r8), %r14d
	cmpb	$26, %bl
	movzbl	%r14b, %ebx
	cmovael	%r8d, %ebx
	leaq	.L.str.1(%rip), %r8
	movzbl	(%rcx,%r8), %r8d
	leal	-65(%r8), %r14d
	leal	32(%r8), %r15d
	cmpb	$26, %r14b
	movzbl	%r15b, %r14d
	cmovael	%r8d, %r14d
	cmpb	%r14b, %bl
	jne	.LBB1_112
# %bb.110:                              #   in Loop: Header=BB1_109 Depth=3
	incq	%rcx
	cmpq	$5, %rcx
	jne	.LBB1_109
# %bb.111:                              #   in Loop: Header=BB1_108 Depth=2
	movl	$5, %ecx
.LBB1_112:                              #   in Loop: Header=BB1_108 Depth=2
	cmpq	$5, %rcx
	je	.LBB1_130
# %bb.113:                              #   in Loop: Header=BB1_108 Depth=2
	leaq	1(%r12), %rcx
	addq	$6, %r12
	incq	%r11
	cmpq	%r13, %r12
	movq	%rcx, %r12
	jbe	.LBB1_108
.LBB1_114:                              #   in Loop: Header=BB1_2 Depth=1
	cmpq	$10, %r13
	movabsq	$-2305843009213680003, %r12     # imm = 0xE00000000000367D
	jb	.LBB1_132
# %bb.115:                              #   in Loop: Header=BB1_2 Depth=1
	xorl	%r11d, %r11d
.LBB1_116:                              #   Parent Loop BB1_2 Depth=1
                                        # =>  This Loop Header: Depth=2
                                        #       Child Loop BB1_117 Depth 3
	xorl	%ecx, %ecx
	.p2align	4
.LBB1_117:                              #   Parent Loop BB1_2 Depth=1
                                        #     Parent Loop BB1_116 Depth=2
                                        # =>    This Inner Loop Header: Depth=3
	movzbl	(%r10,%rcx), %r8d
	leal	-65(%r8), %ebx
	leal	32(%r8), %r14d
	cmpb	$26, %bl
	movzbl	%r14b, %ebx
	cmovael	%r8d, %ebx
	leaq	.L.str.2(%rip), %r8
	movzbl	(%rcx,%r8), %r8d
	leal	-65(%r8), %r14d
	leal	32(%r8), %r15d
	cmpb	$26, %r14b
	movzbl	%r15b, %r14d
	cmovael	%r8d, %r14d
	cmpb	%r14b, %bl
	jne	.LBB1_120
# %bb.118:                              #   in Loop: Header=BB1_117 Depth=3
	incq	%rcx
	cmpq	$10, %rcx
	jne	.LBB1_117
# %bb.119:                              #   in Loop: Header=BB1_116 Depth=2
	movl	$10, %ecx
.LBB1_120:                              #   in Loop: Header=BB1_116 Depth=2
	cmpq	$10, %rcx
	je	.LBB1_133
# %bb.121:                              #   in Loop: Header=BB1_116 Depth=2
	leaq	1(%r11), %rcx
	addq	$11, %r11
	incq	%r10
	cmpq	%r13, %r11
	movq	%rcx, %r11
	jbe	.LBB1_116
	jmp	.LBB1_132
.LBB1_122:                              #   in Loop: Header=BB1_2 Depth=1
	cmpl	$124, %r10d
	je	.LBB1_59
# %bb.123:                              #   in Loop: Header=BB1_2 Depth=1
	cmpl	$126, %r10d
	je	.LBB1_59
	jmp	.LBB1_76
.LBB1_124:                              #   in Loop: Header=BB1_2 Depth=1
	cmpl	$124, %r10d
	je	.LBB1_66
# %bb.125:                              #   in Loop: Header=BB1_2 Depth=1
	cmpl	$126, %r10d
	je	.LBB1_66
	jmp	.LBB1_76
.LBB1_126:                              #   in Loop: Header=BB1_2 Depth=1
	cmpl	$124, %r10d
	je	.LBB1_73
# %bb.127:                              #   in Loop: Header=BB1_2 Depth=1
	cmpl	$126, %r10d
	je	.LBB1_73
	jmp	.LBB1_76
.LBB1_128:                              #   in Loop: Header=BB1_2 Depth=1
	xorl	%r12d, %r12d
.LBB1_129:                              #   in Loop: Header=BB1_2 Depth=1
	cmpl	$1, %r11d
	movl	$0, %ecx
	sbbq	%rcx, %rcx
	orq	%r12, %rcx
	movq	%rcx, 2144(%rdi)
	jmp	.LBB1_131
.LBB1_130:                              #   in Loop: Header=BB1_2 Depth=1
	movl	$0, 2140(%rdi)
.LBB1_131:                              #   in Loop: Header=BB1_2 Depth=1
	movabsq	$-2305843009213680003, %r12     # imm = 0xE00000000000367D
.LBB1_132:                              #   in Loop: Header=BB1_2 Depth=1
	movl	$6, (%rdi)
	xorl	%r13d, %r13d
	movq	-64(%rbp), %r14                 # 8-byte Reload
	movq	-56(%rbp), %rbx                 # 8-byte Reload
	leaq	hp_execute.V(%rip), %r8
	jmp	.LBB1_78
.LBB1_133:                              #   in Loop: Header=BB1_2 Depth=1
	movl	$1, 2140(%rdi)
	jmp	.LBB1_132
.LBB1_134:
	cmpl	$2, %r13d
	je	.LBB1_137
# %bb.135:
	cmpl	$4, %r13d
	jne	.LBB1_137
# %bb.136:
                                        # implicit-def: $eax
	jmp	.LBB1_138
.LBB1_137:
	movq	%rax, 8(%rdi)
	movl	(%rdi), %eax
	xorl	%ecx, %ecx
	cmpl	$12, %eax
	sete	%cl
	cmpl	$13, %eax
	movl	$-1, %eax
	cmovnel	%ecx, %eax
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

	.type	.L.str.1,@object                # @.str.1
.L.str.1:
	.asciz	"close"
	.size	.L.str.1, 6

	.type	.L.str.2,@object                # @.str.2
.L.str.2:
	.asciz	"keep-alive"
	.size	.L.str.2, 11

	.type	.L.str.3,@object                # @.str.3
.L.str.3:
	.asciz	"content-length"
	.size	.L.str.3, 15

	.ident	"clang version 20.1.8"
	.section	".note.GNU-stack","",@progbits
	.addrsig
