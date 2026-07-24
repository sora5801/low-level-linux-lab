	.file	"disasm.c"
	.text
	.globl	x86_decode                      # -- Begin function x86_decode
	.p2align	4
	.type	x86_decode,@function
x86_decode:                             # @x86_decode
# %bb.0:
	pushq	%rbp
	movq	%rsp, %rbp
	pushq	%r15
	pushq	%r14
	pushq	%r13
	pushq	%r12
	pushq	%rbx
	subq	$152, %rsp
	cmpl	$15, %esi
	movl	$15, %r12d
	movl	%esi, -60(%rbp)                 # 4-byte Spill
	cmovbl	%esi, %r12d
	movq	%rdi, -104(%rbp)
	movl	%r12d, -96(%rbp)
	movq	$0, -92(%rbp)
	movl	$0, -84(%rbp)
	leaq	24(%rcx), %r11
	movq	%r11, -56(%rbp)
	movq	$64, -48(%rbp)
	movb	$0, 24(%rcx)
	movq	%rdx, -184(%rbp)                # 8-byte Spill
	movq	%rdx, (%rcx)
	movl	$0, 12(%rcx)
	movq	%rcx, %rax
	movq	$0, 16(%rcx)
	xorl	%r10d, %r10d
	movl	-92(%rbp), %esi
	xorl	%r14d, %r14d
	xorl	%r15d, %r15d
	xorl	%r8d, %r8d
	jmp	.LBB0_1
.LBB0_13:                               #   in Loop: Header=BB0_1 Depth=1
	movl	$1, %r10d
	.p2align	4
.LBB0_16:                               #   in Loop: Header=BB0_1 Depth=1
	incl	%esi
	movl	%esi, -92(%rbp)
	movb	$1, %r9b
	testb	%r9b, %r9b
	je	.LBB0_17
.LBB0_1:                                # =>This Inner Loop Header: Depth=1
	movl	$256, %r9d                      # imm = 0x100
	cmpl	%r12d, %esi
	jae	.LBB0_3
# %bb.2:                                #   in Loop: Header=BB0_1 Depth=1
	movl	%esi, %ecx
	movzbl	(%rdi,%rcx), %r9d
.LBB0_3:                                #   in Loop: Header=BB0_1 Depth=1
	cmpl	$239, %r9d
	jle	.LBB0_8
# %bb.4:                                #   in Loop: Header=BB0_1 Depth=1
	cmpl	$240, %r9d
	je	.LBB0_13
# %bb.5:                                #   in Loop: Header=BB0_1 Depth=1
	cmpl	$242, %r9d
	je	.LBB0_14
# %bb.6:                                #   in Loop: Header=BB0_1 Depth=1
	cmpl	$243, %r9d
	jne	.LBB0_10
# %bb.7:                                #   in Loop: Header=BB0_1 Depth=1
	movl	$1, %r15d
	jmp	.LBB0_16
	.p2align	4
.LBB0_8:                                #   in Loop: Header=BB0_1 Depth=1
	cmpl	$102, %r9d
	je	.LBB0_15
# %bb.9:                                #   in Loop: Header=BB0_1 Depth=1
	cmpl	$103, %r9d
	je	.LBB0_16
.LBB0_10:                               #   in Loop: Header=BB0_1 Depth=1
	movl	%r9d, %ecx
	andl	$-2, %ecx
	cmpl	$100, %ecx
	je	.LBB0_16
# %bb.11:                               #   in Loop: Header=BB0_1 Depth=1
	orl	$8, %r9d
	andl	$-17, %r9d
	cmpl	$46, %r9d
	je	.LBB0_16
# %bb.12:                               #   in Loop: Header=BB0_1 Depth=1
	xorl	%r9d, %r9d
	testb	%r9b, %r9b
	jne	.LBB0_1
	jmp	.LBB0_17
.LBB0_14:                               #   in Loop: Header=BB0_1 Depth=1
	movl	$1, %r14d
	jmp	.LBB0_16
.LBB0_15:                               #   in Loop: Header=BB0_1 Depth=1
	movl	$1, %r8d
	jmp	.LBB0_16
.LBB0_17:
	movq	%r11, -192(%rbp)                # 8-byte Spill
	movl	-92(%rbp), %r11d
	movl	$256, %ebx                      # imm = 0x100
	cmpl	%r12d, %r11d
	jae	.LBB0_19
# %bb.18:
	movzbl	(%rdi,%r11), %ebx
.LBB0_19:
	movl	%ebx, %ecx
	andl	$-16, %ecx
	movl	%ecx, -108(%rbp)                # 4-byte Spill
	cmpl	$64, %ecx
	jne	.LBB0_21
# %bb.20:
	movl	%ebx, %esi
	shrl	$2, %esi
	andl	$1, %esi
	movl	%ebx, %edx
	shrl	%edx
	andl	$1, %edx
	movl	%ebx, %ecx
	andl	$1, %ecx
	incl	%r11d
	movl	%r11d, -92(%rbp)
	testb	$8, %bl
	sete	%dil
	movl	$1, -68(%rbp)                   # 4-byte Folded Spill
	jmp	.LBB0_22
.LBB0_21:
	movb	$1, %dil
	movl	$0, -68(%rbp)                   # 4-byte Folded Spill
	xorl	%ecx, %ecx
	xorl	%edx, %edx
	xorl	%esi, %esi
.LBB0_22:
	xorl	%r9d, %r9d
	testl	%r8d, %r8d
	sete	%r9b
	leal	2(,%r9,2), %r9d
	testb	%dil, %dil
	movl	$8, %r11d
	cmovnel	%r9d, %r11d
	movl	%r11d, -64(%rbp)                # 4-byte Spill
	testl	%r10d, %r10d
	je	.LBB0_27
# %bb.23:
	movb	$108, %bl
	movl	$1, %r10d
	leaq	.L.str(%rip), %r11
	jmp	.LBB0_25
	.p2align	4
.LBB0_24:                               #   in Loop: Header=BB0_25 Depth=1
	movzbl	(%r10,%r11), %ebx
	incq	%r10
	cmpq	$6, %r10
	je	.LBB0_27
.LBB0_25:                               # =>This Inner Loop Header: Depth=1
	movl	-44(%rbp), %r9d
	leal	1(%r9), %r13d
	cmpl	-48(%rbp), %r13d
	jae	.LBB0_24
# %bb.26:                               #   in Loop: Header=BB0_25 Depth=1
	movq	-56(%rbp), %r12
	movl	%r13d, -44(%rbp)
	movb	%bl, (%r12,%r9)
	movq	-56(%rbp), %r9
	movl	-44(%rbp), %ebx
	movb	$0, (%r9,%rbx)
	jmp	.LBB0_24
.LBB0_27:
	movl	-96(%rbp), %r10d
	movl	-92(%rbp), %r11d
	cmpl	%r10d, %r11d
	jae	.LBB0_35
# %bb.28:
	movq	-104(%rbp), %r9
	leal	1(%r11), %ebx
	movl	%ebx, -92(%rbp)
	movzbl	(%r9,%r11), %ebx
	cmpl	$15, %ebx
	je	.LBB0_36
.LBB0_29:
	cmpl	$63, %ebx
	ja	.LBB0_38
# %bb.30:
	movl	%ebx, %r11d
	andl	$7, %r11d
	cmpl	$5, %r11d
	ja	.LBB0_38
# %bb.31:
	movl	%ebx, %edi
	andl	$-8, %edi
	leaq	ALU(%rip), %r8
	movq	(%rdi,%r8), %r14
	cmpl	$5, %r11d
	je	.LBB0_128
# %bb.32:
	cmpl	$4, %r11d
	movl	-60(%rbp), %r15d                # 4-byte Reload
	jne	.LBB0_139
# %bb.33:
	movl	-92(%rbp), %edx
	cmpl	%r10d, %edx
	movq	%rax, %r11
	jae	.LBB0_167
# %bb.34:
	movq	-104(%rbp), %rcx
	leal	1(%rdx), %esi
	movl	%esi, -92(%rbp)
	movsbq	(%rcx,%rdx), %rdx
	jmp	.LBB0_168
.LBB0_35:
	movl	$1, -88(%rbp)
	xorl	%ebx, %ebx
	cmpl	$15, %ebx
	jne	.LBB0_29
.LBB0_36:
	movl	-92(%rbp), %edi
	cmpl	%r10d, %edi
	movq	%rax, %r11
	jae	.LBB0_44
# %bb.37:
	movq	-104(%rbp), %r8
	leal	1(%rdi), %r9d
	movl	%r9d, -92(%rbp)
	movzbl	(%r8,%rdi), %r12d
	jmp	.LBB0_45
.LBB0_38:
	movl	%ebx, %r11d
	andl	$-8, %r11d
	cmpl	$80, %r11d
	je	.LBB0_61
# %bb.39:
	cmpl	$88, %r11d
	jne	.LBB0_77
# %bb.40:
	movb	$112, %dil
	xorl	%edx, %edx
	leaq	.L.str.51(%rip), %rsi
	movq	%rax, %r11
	movl	-60(%rbp), %r15d                # 4-byte Reload
	jmp	.LBB0_42
	.p2align	4
.LBB0_41:                               #   in Loop: Header=BB0_42 Depth=1
	movzbl	1(%rdx,%rsi), %edi
	incq	%rdx
	cmpq	$4, %rdx
	je	.LBB0_56
.LBB0_42:                               # =>This Inner Loop Header: Depth=1
	movl	-44(%rbp), %r8d
	leal	1(%r8), %r9d
	cmpl	-48(%rbp), %r9d
	jae	.LBB0_41
# %bb.43:                               #   in Loop: Header=BB0_42 Depth=1
	movq	-56(%rbp), %r10
	movl	%r9d, -44(%rbp)
	movb	%dil, (%r10,%r8)
	movq	-56(%rbp), %rdi
	movl	-44(%rbp), %r8d
	movb	$0, (%rdi,%r8)
	jmp	.LBB0_41
.LBB0_44:
	movl	$1, -88(%rbp)
	xorl	%r12d, %r12d
.LBB0_45:
	cmpl	$30, %r12d
	sete	%dil
	testl	%r15d, %r15d
	setne	%r8b
	andb	%dil, %r8b
	cmpb	$1, %r8b
	jne	.LBB0_48
# %bb.46:
	movl	-92(%rbp), %edx
	cmpl	%r10d, %edx
	jae	.LBB0_70
# %bb.47:
	movq	-104(%rbp), %rcx
	leal	1(%rdx), %esi
	movl	%esi, -92(%rbp)
	movzbl	(%rcx,%rdx), %edx
	jmp	.LBB0_71
.LBB0_48:
	cmpl	$30, %r12d
	jle	.LBB0_82
# %bb.49:
	cmpl	$31, %r12d
	je	.LBB0_116
# %bb.50:
	cmpl	$49, %r12d
	je	.LBB0_120
# %bb.51:
	cmpl	$162, %r12d
	jne	.LBB0_161
# %bb.52:
	movb	$99, %dil
	movl	$1, %edx
	leaq	.L.str.7(%rip), %rsi
	movl	-60(%rbp), %r15d                # 4-byte Reload
	jmp	.LBB0_54
	.p2align	4
.LBB0_53:                               #   in Loop: Header=BB0_54 Depth=1
	movzbl	(%rdx,%rsi), %edi
	incq	%rdx
	cmpq	$6, %rdx
	je	.LBB0_327
.LBB0_54:                               # =>This Inner Loop Header: Depth=1
	movl	-44(%rbp), %ecx
	leal	1(%rcx), %r8d
	cmpl	-48(%rbp), %r8d
	jae	.LBB0_53
# %bb.55:                               #   in Loop: Header=BB0_54 Depth=1
	movq	-56(%rbp), %r9
	movl	%r8d, -44(%rbp)
	movb	%dil, (%r9,%rcx)
	movq	-56(%rbp), %rcx
	movl	-44(%rbp), %edi
	movb	$0, (%rcx,%rdi)
	jmp	.LBB0_53
.LBB0_56:
	leal	(%rbx,%rcx,8), %ecx
	addl	$-88, %ecx
	leaq	R64(%rip), %rdx
	movq	(%rdx,%rcx,8), %rdx
	movzbl	(%rdx), %esi
	testb	%sil, %sil
	je	.LBB0_327
# %bb.57:
	incq	%rdx
	jmp	.LBB0_59
	.p2align	4
.LBB0_58:                               #   in Loop: Header=BB0_59 Depth=1
	movzbl	(%rdx), %esi
	incq	%rdx
	testb	%sil, %sil
	je	.LBB0_327
.LBB0_59:                               # =>This Inner Loop Header: Depth=1
	movl	-44(%rbp), %ecx
	leal	1(%rcx), %edi
	cmpl	-48(%rbp), %edi
	jae	.LBB0_58
# %bb.60:                               #   in Loop: Header=BB0_59 Depth=1
	movq	-56(%rbp), %r8
	movl	%edi, -44(%rbp)
	movb	%sil, (%r8,%rcx)
	movq	-56(%rbp), %rcx
	movl	-44(%rbp), %esi
	movb	$0, (%rcx,%rsi)
	jmp	.LBB0_58
.LBB0_61:
	movb	$112, %dil
	movl	$1, %edx
	leaq	.L.str.50(%rip), %rsi
	movq	%rax, %r11
	movl	-60(%rbp), %r15d                # 4-byte Reload
	jmp	.LBB0_63
	.p2align	4
.LBB0_62:                               #   in Loop: Header=BB0_63 Depth=1
	movzbl	(%rdx,%rsi), %edi
	incq	%rdx
	cmpq	$6, %rdx
	je	.LBB0_65
.LBB0_63:                               # =>This Inner Loop Header: Depth=1
	movl	-44(%rbp), %r8d
	leal	1(%r8), %r9d
	cmpl	-48(%rbp), %r9d
	jae	.LBB0_62
# %bb.64:                               #   in Loop: Header=BB0_63 Depth=1
	movq	-56(%rbp), %r10
	movl	%r9d, -44(%rbp)
	movb	%dil, (%r10,%r8)
	movq	-56(%rbp), %rdi
	movl	-44(%rbp), %r8d
	movb	$0, (%rdi,%r8)
	jmp	.LBB0_62
.LBB0_65:
	leal	(%rbx,%rcx,8), %ecx
	addl	$-80, %ecx
	leaq	R64(%rip), %rdx
	movq	(%rdx,%rcx,8), %rdx
	movzbl	(%rdx), %esi
	testb	%sil, %sil
	je	.LBB0_327
# %bb.66:
	incq	%rdx
	jmp	.LBB0_68
	.p2align	4
.LBB0_67:                               #   in Loop: Header=BB0_68 Depth=1
	movzbl	(%rdx), %esi
	incq	%rdx
	testb	%sil, %sil
	je	.LBB0_327
.LBB0_68:                               # =>This Inner Loop Header: Depth=1
	movl	-44(%rbp), %ecx
	leal	1(%rcx), %edi
	cmpl	-48(%rbp), %edi
	jae	.LBB0_67
# %bb.69:                               #   in Loop: Header=BB0_68 Depth=1
	movq	-56(%rbp), %r8
	movl	%edi, -44(%rbp)
	movb	%sil, (%r8,%rcx)
	movq	-56(%rbp), %rcx
	movl	-44(%rbp), %esi
	movb	$0, (%rcx,%rsi)
	jmp	.LBB0_67
.LBB0_70:
	movl	$1, -88(%rbp)
	xorl	%edx, %edx
.LBB0_71:
	movl	-60(%rbp), %r15d                # 4-byte Reload
	cmpb	$-6, %dl
	je	.LBB0_88
# %bb.72:
	movzbl	%dl, %ecx
	cmpl	$251, %ecx
	jne	.LBB0_92
# %bb.73:
	movb	$101, %dil
	movl	$1, %edx
	leaq	.L.str.2(%rip), %rsi
	jmp	.LBB0_75
	.p2align	4
.LBB0_74:                               #   in Loop: Header=BB0_75 Depth=1
	movzbl	(%rdx,%rsi), %edi
	incq	%rdx
	cmpq	$8, %rdx
	je	.LBB0_327
.LBB0_75:                               # =>This Inner Loop Header: Depth=1
	movl	-44(%rbp), %ecx
	leal	1(%rcx), %r8d
	cmpl	-48(%rbp), %r8d
	jae	.LBB0_74
# %bb.76:                               #   in Loop: Header=BB0_75 Depth=1
	movq	-56(%rbp), %r9
	movl	%r8d, -44(%rbp)
	movb	%dil, (%r9,%rcx)
	movq	-56(%rbp), %rcx
	movl	-44(%rbp), %edi
	movb	$0, (%rcx,%rdi)
	jmp	.LBB0_74
.LBB0_77:
	cmpl	$99, %ebx
	jne	.LBB0_96
# %bb.78:
	movl	-60(%rbp), %r14d                # 4-byte Reload
	movq	%rax, %rbx
	leaq	-104(%rbp), %rdi
	leaq	-176(%rbp), %r8
                                        # kill: def $ecx killed $ecx killed $rcx
	callq	parse_modrm
	movb	$109, %dl
	movl	$1, %eax
	leaq	.L.str.52(%rip), %rcx
	jmp	.LBB0_80
	.p2align	4
.LBB0_79:                               #   in Loop: Header=BB0_80 Depth=1
	movzbl	(%rax,%rcx), %edx
	incq	%rax
	cmpq	$8, %rax
	je	.LBB0_101
.LBB0_80:                               # =>This Inner Loop Header: Depth=1
	movl	-44(%rbp), %esi
	leal	1(%rsi), %edi
	cmpl	-48(%rbp), %edi
	jae	.LBB0_79
# %bb.81:                               #   in Loop: Header=BB0_80 Depth=1
	movq	-56(%rbp), %r8
	movl	%edi, -44(%rbp)
	movb	%dl, (%r8,%rsi)
	movq	-56(%rbp), %rdx
	movl	-44(%rbp), %esi
	movb	$0, (%rdx,%rsi)
	jmp	.LBB0_79
.LBB0_82:
	cmpl	$5, %r12d
	je	.LBB0_124
# %bb.83:
	cmpl	$11, %r12d
	jne	.LBB0_161
# %bb.84:
	movb	$117, %dil
	movl	$1, %edx
	leaq	.L.str.5(%rip), %rsi
	movl	-60(%rbp), %r15d                # 4-byte Reload
	jmp	.LBB0_86
	.p2align	4
.LBB0_85:                               #   in Loop: Header=BB0_86 Depth=1
	movzbl	(%rdx,%rsi), %edi
	incq	%rdx
	cmpq	$4, %rdx
	je	.LBB0_327
.LBB0_86:                               # =>This Inner Loop Header: Depth=1
	movl	-44(%rbp), %ecx
	leal	1(%rcx), %r8d
	cmpl	-48(%rbp), %r8d
	jae	.LBB0_85
# %bb.87:                               #   in Loop: Header=BB0_86 Depth=1
	movq	-56(%rbp), %r9
	movl	%r8d, -44(%rbp)
	movb	%dil, (%r9,%rcx)
	movq	-56(%rbp), %rcx
	movl	-44(%rbp), %edi
	movb	$0, (%rcx,%rdi)
	jmp	.LBB0_85
.LBB0_88:
	movb	$101, %dil
	movl	$1, %edx
	leaq	.L.str.1(%rip), %rsi
	jmp	.LBB0_90
	.p2align	4
.LBB0_89:                               #   in Loop: Header=BB0_90 Depth=1
	movzbl	(%rdx,%rsi), %edi
	incq	%rdx
	cmpq	$8, %rdx
	je	.LBB0_327
.LBB0_90:                               # =>This Inner Loop Header: Depth=1
	movl	-44(%rbp), %ecx
	leal	1(%rcx), %r8d
	cmpl	-48(%rbp), %r8d
	jae	.LBB0_89
# %bb.91:                               #   in Loop: Header=BB0_90 Depth=1
	movq	-56(%rbp), %r9
	movl	%r8d, -44(%rbp)
	movb	%dil, (%r9,%rcx)
	movq	-56(%rbp), %rcx
	movl	-44(%rbp), %edi
	movb	$0, (%rcx,%rdi)
	jmp	.LBB0_89
.LBB0_92:
	movb	$110, %dil
	movl	$1, %edx
	leaq	.L.str.3(%rip), %rsi
	jmp	.LBB0_94
	.p2align	4
.LBB0_93:                               #   in Loop: Header=BB0_94 Depth=1
	movzbl	(%rdx,%rsi), %edi
	incq	%rdx
	cmpq	$4, %rdx
	je	.LBB0_327
.LBB0_94:                               # =>This Inner Loop Header: Depth=1
	movl	-44(%rbp), %ecx
	leal	1(%rcx), %r8d
	cmpl	-48(%rbp), %r8d
	jae	.LBB0_93
# %bb.95:                               #   in Loop: Header=BB0_94 Depth=1
	movq	-56(%rbp), %r9
	movl	%r8d, -44(%rbp)
	movb	%dil, (%r9,%rcx)
	movq	-56(%rbp), %rcx
	movl	-44(%rbp), %edi
	movb	$0, (%rcx,%rdi)
	jmp	.LBB0_93
.LBB0_96:
	movq	%rax, -80(%rbp)                 # 8-byte Spill
	movl	%ebx, %r9d
	orl	$2, %r9d
	cmpl	$106, %r9d
	jne	.LBB0_187
# %bb.97:
	xorl	%ecx, %ecx
	cmpl	$104, %ebx
	sete	%cl
	leal	(%rcx,%rcx,2), %esi
	incl	%esi
	leaq	-104(%rbp), %rdi
	callq	rd_imm_sext
	movq	%rax, %rdx
	movl	-60(%rbp), %r15d                # 4-byte Reload
	movb	$112, %r8b
	movl	$1, %esi
	leaq	.L.str.50(%rip), %rdi
	movq	-80(%rbp), %r11                 # 8-byte Reload
	jmp	.LBB0_99
	.p2align	4
.LBB0_98:                               #   in Loop: Header=BB0_99 Depth=1
	movzbl	(%rsi,%rdi), %r8d
	incq	%rsi
	cmpq	$6, %rsi
	je	.LBB0_151
.LBB0_99:                               # =>This Inner Loop Header: Depth=1
	movl	-44(%rbp), %ecx
	leal	1(%rcx), %r9d
	cmpl	-48(%rbp), %r9d
	jae	.LBB0_98
# %bb.100:                              #   in Loop: Header=BB0_99 Depth=1
	movq	-56(%rbp), %r10
	movl	%r9d, -44(%rbp)
	movb	%r8b, (%r10,%rcx)
	movq	-56(%rbp), %rcx
	movl	-44(%rbp), %r8d
	movb	$0, (%rcx,%r8)
	jmp	.LBB0_98
.LBB0_101:
	movl	-172(%rbp), %eax
	andl	$15, %eax
	leaq	R64(%rip), %rcx
	movq	(%rcx,%rax,8), %rdx
	movzbl	(%rdx), %esi
	testb	%sil, %sil
	movq	%rbx, %r11
	movl	%r14d, %r15d
	je	.LBB0_106
# %bb.102:
	incq	%rdx
	jmp	.LBB0_104
	.p2align	4
.LBB0_103:                              #   in Loop: Header=BB0_104 Depth=1
	movzbl	(%rdx), %esi
	incq	%rdx
	testb	%sil, %sil
	je	.LBB0_106
.LBB0_104:                              # =>This Inner Loop Header: Depth=1
	movl	-44(%rbp), %ecx
	leal	1(%rcx), %edi
	cmpl	-48(%rbp), %edi
	jae	.LBB0_103
# %bb.105:                              #   in Loop: Header=BB0_104 Depth=1
	movq	-56(%rbp), %r8
	movl	%edi, -44(%rbp)
	movb	%sil, (%r8,%rcx)
	movq	-56(%rbp), %rcx
	movl	-44(%rbp), %esi
	movb	$0, (%rcx,%rsi)
	jmp	.LBB0_103
.LBB0_106:
	movb	$44, %dil
	xorl	%edx, %edx
	leaq	.L.str.10(%rip), %rsi
	jmp	.LBB0_108
	.p2align	4
.LBB0_107:                              #   in Loop: Header=BB0_108 Depth=1
	movzbl	1(%rdx,%rsi), %edi
	incq	%rdx
	cmpq	$2, %rdx
	je	.LBB0_110
.LBB0_108:                              # =>This Inner Loop Header: Depth=1
	movl	-44(%rbp), %ecx
	leal	1(%rcx), %r8d
	cmpl	-48(%rbp), %r8d
	jae	.LBB0_107
# %bb.109:                              #   in Loop: Header=BB0_108 Depth=1
	movq	-56(%rbp), %r9
	movl	%r8d, -44(%rbp)
	movb	%dil, (%r9,%rcx)
	movq	-56(%rbp), %rcx
	movl	-44(%rbp), %edi
	movb	$0, (%rcx,%rdi)
	jmp	.LBB0_107
.LBB0_110:
	cmpl	$0, -176(%rbp)
	je	.LBB0_166
# %bb.111:
	movl	-168(%rbp), %ecx
	andl	$15, %ecx
	leaq	R32(%rip), %rdx
	movq	(%rdx,%rcx,8), %rdx
	movzbl	(%rdx), %esi
	testb	%sil, %sil
	je	.LBB0_474
# %bb.112:
	incq	%rdx
	jmp	.LBB0_114
	.p2align	4
.LBB0_113:                              #   in Loop: Header=BB0_114 Depth=1
	movzbl	(%rdx), %esi
	incq	%rdx
	testb	%sil, %sil
	je	.LBB0_474
.LBB0_114:                              # =>This Inner Loop Header: Depth=1
	movl	-44(%rbp), %ecx
	leal	1(%rcx), %edi
	cmpl	-48(%rbp), %edi
	jae	.LBB0_113
# %bb.115:                              #   in Loop: Header=BB0_114 Depth=1
	movq	-56(%rbp), %r8
	movl	%edi, -44(%rbp)
	movb	%sil, (%r8,%rcx)
	movq	-56(%rbp), %rcx
	movl	-44(%rbp), %esi
	movb	$0, (%rcx,%rsi)
	jmp	.LBB0_113
.LBB0_116:
	leaq	-104(%rbp), %rdi
	leaq	-176(%rbp), %r8
	movq	%r11, %rbx
                                        # kill: def $ecx killed $ecx killed $rcx
	callq	parse_modrm
	movl	-60(%rbp), %r15d                # 4-byte Reload
	movq	%rbx, %r11
	movb	$110, %dil
	movl	$1, %edx
	leaq	.L.str.3(%rip), %rsi
	jmp	.LBB0_118
	.p2align	4
.LBB0_117:                              #   in Loop: Header=BB0_118 Depth=1
	movzbl	(%rdx,%rsi), %edi
	incq	%rdx
	cmpq	$4, %rdx
	je	.LBB0_327
.LBB0_118:                              # =>This Inner Loop Header: Depth=1
	movl	-44(%rbp), %ecx
	leal	1(%rcx), %r8d
	cmpl	-48(%rbp), %r8d
	jae	.LBB0_117
# %bb.119:                              #   in Loop: Header=BB0_118 Depth=1
	movq	-56(%rbp), %r9
	movl	%r8d, -44(%rbp)
	movb	%dil, (%r9,%rcx)
	movq	-56(%rbp), %rcx
	movl	-44(%rbp), %edi
	movb	$0, (%rcx,%rdi)
	jmp	.LBB0_117
.LBB0_120:
	movb	$114, %dil
	movl	$1, %edx
	leaq	.L.str.6(%rip), %rsi
	movl	-60(%rbp), %r15d                # 4-byte Reload
	jmp	.LBB0_122
	.p2align	4
.LBB0_121:                              #   in Loop: Header=BB0_122 Depth=1
	movzbl	(%rdx,%rsi), %edi
	incq	%rdx
	cmpq	$6, %rdx
	je	.LBB0_327
.LBB0_122:                              # =>This Inner Loop Header: Depth=1
	movl	-44(%rbp), %ecx
	leal	1(%rcx), %r8d
	cmpl	-48(%rbp), %r8d
	jae	.LBB0_121
# %bb.123:                              #   in Loop: Header=BB0_122 Depth=1
	movq	-56(%rbp), %r9
	movl	%r8d, -44(%rbp)
	movb	%dil, (%r9,%rcx)
	movq	-56(%rbp), %rcx
	movl	-44(%rbp), %edi
	movb	$0, (%rcx,%rdi)
	jmp	.LBB0_121
.LBB0_124:
	movb	$115, %dil
	movl	$1, %edx
	leaq	.L.str.4(%rip), %rsi
	movl	-60(%rbp), %r15d                # 4-byte Reload
	jmp	.LBB0_126
	.p2align	4
.LBB0_125:                              #   in Loop: Header=BB0_126 Depth=1
	movzbl	(%rdx,%rsi), %edi
	incq	%rdx
	cmpq	$8, %rdx
	je	.LBB0_327
.LBB0_126:                              # =>This Inner Loop Header: Depth=1
	movl	-44(%rbp), %ecx
	leal	1(%rcx), %r8d
	cmpl	-48(%rbp), %r8d
	jae	.LBB0_125
# %bb.127:                              #   in Loop: Header=BB0_126 Depth=1
	movq	-56(%rbp), %r9
	movl	%r8d, -44(%rbp)
	movb	%dil, (%r9,%rcx)
	movq	-56(%rbp), %rcx
	movl	-44(%rbp), %edi
	movb	$0, (%rcx,%rdi)
	jmp	.LBB0_125
.LBB0_128:
	movl	-60(%rbp), %ebx                 # 4-byte Reload
	movq	%rax, %r15
	xorl	%eax, %eax
	cmpl	$2, -64(%rbp)                   # 4-byte Folded Reload
	setne	%al
	leal	2(,%rax,2), %esi
	leaq	-104(%rbp), %rdi
	callq	rd_imm_sext
	movq	%rax, %rdx
	movzbl	(%r14), %eax
	testb	%al, %al
	je	.LBB0_133
# %bb.129:
	incq	%r14
	jmp	.LBB0_131
	.p2align	4
.LBB0_130:                              #   in Loop: Header=BB0_131 Depth=1
	movzbl	(%r14), %eax
	incq	%r14
	testb	%al, %al
	je	.LBB0_133
.LBB0_131:                              # =>This Inner Loop Header: Depth=1
	movl	-44(%rbp), %ecx
	leal	1(%rcx), %esi
	cmpl	-48(%rbp), %esi
	jae	.LBB0_130
# %bb.132:                              #   in Loop: Header=BB0_131 Depth=1
	movq	-56(%rbp), %rdi
	movl	%esi, -44(%rbp)
	movb	%al, (%rdi,%rcx)
	movq	-56(%rbp), %rax
	movl	-44(%rbp), %ecx
	movb	$0, (%rax,%rcx)
	jmp	.LBB0_130
.LBB0_133:
	movl	-44(%rbp), %eax
	leal	1(%rax), %ecx
	cmpl	-48(%rbp), %ecx
	jae	.LBB0_135
# %bb.134:
	movq	-56(%rbp), %rsi
	movl	%ecx, -44(%rbp)
	movb	$32, (%rsi,%rax)
	movq	-56(%rbp), %rax
	movl	-44(%rbp), %ecx
	movb	$0, (%rax,%rcx)
.LBB0_135:
	movl	-64(%rbp), %eax                 # 4-byte Reload
	cmpl	$2, %eax
	movq	%r15, %r11
	movl	%ebx, %r15d
	je	.LBB0_213
# %bb.136:
	cmpl	$8, %eax
	je	.LBB0_212
# %bb.137:
	cmpl	$4, %eax
	jne	.LBB0_214
# %bb.138:
	leaq	R32(%rip), %rsi
	jmp	.LBB0_215
.LBB0_139:
	movq	%rax, %r15
	testb	$5, %bl
	movl	$1, %eax
	movl	-64(%rbp), %edi                 # 4-byte Reload
	cmovel	%eax, %edi
	movl	%edi, -64(%rbp)                 # 4-byte Spill
	leaq	-104(%rbp), %rdi
	leaq	-176(%rbp), %r8
                                        # kill: def $ecx killed $ecx killed $rcx
	callq	parse_modrm
	movzbl	(%r14), %eax
	testb	%al, %al
	je	.LBB0_144
# %bb.140:
	incq	%r14
	jmp	.LBB0_142
	.p2align	4
.LBB0_141:                              #   in Loop: Header=BB0_142 Depth=1
	movzbl	(%r14), %eax
	incq	%r14
	testb	%al, %al
	je	.LBB0_144
.LBB0_142:                              # =>This Inner Loop Header: Depth=1
	movl	-44(%rbp), %ecx
	leal	1(%rcx), %edx
	cmpl	-48(%rbp), %edx
	jae	.LBB0_141
# %bb.143:                              #   in Loop: Header=BB0_142 Depth=1
	movq	-56(%rbp), %rsi
	movl	%edx, -44(%rbp)
	movb	%al, (%rsi,%rcx)
	movq	-56(%rbp), %rax
	movl	-44(%rbp), %ecx
	movb	$0, (%rax,%rcx)
	jmp	.LBB0_141
.LBB0_144:
	andl	$6, %ebx
	movl	-44(%rbp), %eax
	leal	1(%rax), %ecx
	cmpl	-48(%rbp), %ecx
	jae	.LBB0_146
# %bb.145:
	movq	-56(%rbp), %rdx
	movl	%ecx, -44(%rbp)
	movb	$32, (%rdx,%rax)
	movq	-56(%rbp), %rax
	movl	-44(%rbp), %ecx
	movb	$0, (%rax,%rcx)
.LBB0_146:
	cmpl	$2, %ebx
	movl	-64(%rbp), %ebx                 # 4-byte Reload
	jne	.LBB0_192
# %bb.147:
	movl	-172(%rbp), %eax
	cmpl	$2, %ebx
	je	.LBB0_293
# %bb.148:
	cmpl	$4, %ebx
	je	.LBB0_292
# %bb.149:
	cmpl	$8, %ebx
	jne	.LBB0_294
# %bb.150:
	andl	$15, %eax
	leaq	R64(%rip), %rcx
	jmp	.LBB0_370
.LBB0_151:
	movb	$48, %r8b
	xorl	%esi, %esi
	leaq	.L.str.117(%rip), %rdi
	jmp	.LBB0_153
	.p2align	4
.LBB0_152:                              #   in Loop: Header=BB0_153 Depth=1
	movzbl	1(%rsi,%rdi), %r8d
	incq	%rsi
	cmpq	$2, %rsi
	je	.LBB0_155
.LBB0_153:                              # =>This Inner Loop Header: Depth=1
	movl	-44(%rbp), %ecx
	leal	1(%rcx), %r9d
	cmpl	-48(%rbp), %r9d
	jae	.LBB0_152
# %bb.154:                              #   in Loop: Header=BB0_153 Depth=1
	movq	-56(%rbp), %r10
	movl	%r9d, -44(%rbp)
	movb	%r8b, (%r10,%rcx)
	movq	-56(%rbp), %rcx
	movl	-44(%rbp), %r8d
	movb	$0, (%rcx,%r8)
	jmp	.LBB0_152
.LBB0_155:
	testq	%rdx, %rdx
	je	.LBB0_325
# %bb.156:
	xorl	%esi, %esi
	movq	%rdx, %rdi
	.p2align	4
.LBB0_157:                              # =>This Inner Loop Header: Depth=1
	movl	%edx, %ecx
	andl	$15, %ecx
	leal	87(%rcx), %r8d
	leal	48(%rcx), %r9d
	cmpl	$10, %ecx
	movzbl	%r9b, %ecx
	movzbl	%r8b, %r8d
	cmovbl	%ecx, %r8d
	movb	%r8b, -176(%rbp,%rsi)
	incq	%rsi
	shrq	$4, %rdi
	cmpq	$15, %rdx
	movq	%rdi, %rdx
	ja	.LBB0_157
	jmp	.LBB0_159
	.p2align	4
.LBB0_158:                              #   in Loop: Header=BB0_159 Depth=1
	decq	%rsi
	je	.LBB0_327
.LBB0_159:                              # =>This Inner Loop Header: Depth=1
	movl	-44(%rbp), %edx
	leal	1(%rdx), %ecx
	cmpl	-48(%rbp), %ecx
	jae	.LBB0_158
# %bb.160:                              #   in Loop: Header=BB0_159 Depth=1
	movzbl	-177(%rbp,%rsi), %edi
	movq	-56(%rbp), %r8
	movl	%ecx, -44(%rbp)
	movb	%dil, (%r8,%rdx)
	movq	-56(%rbp), %rcx
	movl	-44(%rbp), %edx
	movb	$0, (%rcx,%rdx)
	jmp	.LBB0_158
.LBB0_161:
	movl	%r12d, %edi
	andb	$-16, %dil
	cmpb	$64, %dil
	je	.LBB0_246
# %bb.162:
	movl	%r12d, %edi
	andl	$-16, %edi
	cmpl	$144, %edi
	je	.LBB0_234
# %bb.163:
	cmpl	$128, %edi
	jne	.LBB0_269
# %bb.164:
	movl	-92(%rbp), %edx
	cmpl	%r10d, %edx
	jae	.LBB0_296
# %bb.165:
	movq	-104(%rbp), %rcx
	leal	1(%rdx), %esi
	movl	%esi, -92(%rbp)
	movzbl	(%rcx,%rdx), %esi
	jmp	.LBB0_297
.LBB0_166:
	leaq	-56(%rbp), %rdi
	leaq	-176(%rbp), %rsi
	callq	render_mem
	movl	%r14d, %r15d
	movq	%rbx, %r11
	jmp	.LBB0_474
.LBB0_167:
	movl	$1, -88(%rbp)
	xorl	%edx, %edx
.LBB0_168:
	movzbl	(%r14), %esi
	testb	%sil, %sil
	je	.LBB0_173
# %bb.169:
	incq	%r14
	jmp	.LBB0_171
	.p2align	4
.LBB0_170:                              #   in Loop: Header=BB0_171 Depth=1
	movzbl	(%r14), %esi
	incq	%r14
	testb	%sil, %sil
	je	.LBB0_173
.LBB0_171:                              # =>This Inner Loop Header: Depth=1
	movl	-44(%rbp), %ecx
	leal	1(%rcx), %edi
	cmpl	-48(%rbp), %edi
	jae	.LBB0_170
# %bb.172:                              #   in Loop: Header=BB0_171 Depth=1
	movq	-56(%rbp), %r8
	movl	%edi, -44(%rbp)
	movb	%sil, (%r8,%rcx)
	movq	-56(%rbp), %rcx
	movl	-44(%rbp), %esi
	movb	$0, (%rcx,%rsi)
	jmp	.LBB0_170
.LBB0_173:
	movb	$32, %r8b
	movl	$1, %esi
	leaq	.L.str.49(%rip), %rdi
	jmp	.LBB0_175
	.p2align	4
.LBB0_174:                              #   in Loop: Header=BB0_175 Depth=1
	movzbl	(%rsi,%rdi), %r8d
	incq	%rsi
	cmpq	$6, %rsi
	je	.LBB0_177
.LBB0_175:                              # =>This Inner Loop Header: Depth=1
	movl	-44(%rbp), %ecx
	leal	1(%rcx), %r9d
	cmpl	-48(%rbp), %r9d
	jae	.LBB0_174
# %bb.176:                              #   in Loop: Header=BB0_175 Depth=1
	movq	-56(%rbp), %r10
	movl	%r9d, -44(%rbp)
	movb	%r8b, (%r10,%rcx)
	movq	-56(%rbp), %rcx
	movl	-44(%rbp), %r8d
	movb	$0, (%rcx,%r8)
	jmp	.LBB0_174
.LBB0_177:
	movb	$48, %r8b
	xorl	%esi, %esi
	leaq	.L.str.117(%rip), %rdi
	jmp	.LBB0_179
	.p2align	4
.LBB0_178:                              #   in Loop: Header=BB0_179 Depth=1
	movzbl	1(%rsi,%rdi), %r8d
	incq	%rsi
	cmpq	$2, %rsi
	je	.LBB0_181
.LBB0_179:                              # =>This Inner Loop Header: Depth=1
	movl	-44(%rbp), %ecx
	leal	1(%rcx), %r9d
	cmpl	-48(%rbp), %r9d
	jae	.LBB0_178
# %bb.180:                              #   in Loop: Header=BB0_179 Depth=1
	movq	-56(%rbp), %r10
	movl	%r9d, -44(%rbp)
	movb	%r8b, (%r10,%rcx)
	movq	-56(%rbp), %rcx
	movl	-44(%rbp), %r8d
	movb	$0, (%rcx,%r8)
	jmp	.LBB0_178
.LBB0_181:
	testq	%rdx, %rdx
	je	.LBB0_325
# %bb.182:
	xorl	%esi, %esi
	movq	%rdx, %rdi
	.p2align	4
.LBB0_183:                              # =>This Inner Loop Header: Depth=1
	movl	%edx, %ecx
	andl	$15, %ecx
	leal	87(%rcx), %r8d
	leal	48(%rcx), %r9d
	cmpl	$10, %ecx
	movzbl	%r9b, %ecx
	movzbl	%r8b, %r8d
	cmovbl	%ecx, %r8d
	movb	%r8b, -176(%rbp,%rsi)
	incq	%rsi
	shrq	$4, %rdi
	cmpq	$15, %rdx
	movq	%rdi, %rdx
	ja	.LBB0_183
	jmp	.LBB0_185
	.p2align	4
.LBB0_184:                              #   in Loop: Header=BB0_185 Depth=1
	decq	%rsi
	je	.LBB0_327
.LBB0_185:                              # =>This Inner Loop Header: Depth=1
	movl	-44(%rbp), %edx
	leal	1(%rdx), %ecx
	cmpl	-48(%rbp), %ecx
	jae	.LBB0_184
# %bb.186:                              #   in Loop: Header=BB0_185 Depth=1
	movzbl	-177(%rbp,%rsi), %edi
	movq	-56(%rbp), %r8
	movl	%ecx, -44(%rbp)
	movb	%dil, (%r8,%rdx)
	movq	-56(%rbp), %rcx
	movl	-44(%rbp), %edx
	movb	$0, (%rcx,%rdx)
	jmp	.LBB0_184
.LBB0_187:
	cmpl	$107, %r9d
	jne	.LBB0_275
# %bb.188:
	movl	-60(%rbp), %r14d                # 4-byte Reload
	leaq	-104(%rbp), %r12
	leaq	-176(%rbp), %r8
	movq	%r12, %rdi
                                        # kill: def $ecx killed $ecx killed $rcx
	callq	parse_modrm
	xorl	%eax, %eax
	cmpl	$2, -64(%rbp)                   # 4-byte Folded Reload
	setne	%al
	cmpl	$105, %ebx
	leal	2(%rax,%rax), %eax
	movl	$1, %esi
	cmovel	%eax, %esi
	movq	%r12, %rdi
	callq	rd_imm_sext
	movq	%rax, %r12
	movb	$105, %dl
	movl	$1, %eax
	leaq	.L.str.13(%rip), %rcx
	jmp	.LBB0_190
	.p2align	4
.LBB0_189:                              #   in Loop: Header=BB0_190 Depth=1
	movzbl	(%rax,%rcx), %edx
	incq	%rax
	cmpq	$6, %rax
	je	.LBB0_197
.LBB0_190:                              # =>This Inner Loop Header: Depth=1
	movl	-44(%rbp), %esi
	leal	1(%rsi), %edi
	cmpl	-48(%rbp), %edi
	jae	.LBB0_189
# %bb.191:                              #   in Loop: Header=BB0_190 Depth=1
	movq	-56(%rbp), %r8
	movl	%edi, -44(%rbp)
	movb	%dl, (%r8,%rsi)
	movq	-56(%rbp), %rdx
	movl	-44(%rbp), %esi
	movb	$0, (%rdx,%rsi)
	jmp	.LBB0_189
.LBB0_192:
	cmpl	$0, -176(%rbp)
	je	.LBB0_447
# %bb.193:
	movl	-168(%rbp), %edx
	cmpl	$2, %ebx
	je	.LBB0_366
# %bb.194:
	cmpl	$4, %ebx
	movq	%r15, %r11
	movl	-60(%rbp), %r15d                # 4-byte Reload
	je	.LBB0_365
# %bb.195:
	cmpl	$8, %ebx
	jne	.LBB0_367
# %bb.196:
	andl	$15, %edx
	leaq	R64(%rip), %rcx
	jmp	.LBB0_441
.LBB0_197:
	movl	-172(%rbp), %edi
	movl	-64(%rbp), %esi                 # 4-byte Reload
	movl	-68(%rbp), %edx                 # 4-byte Reload
	callq	reg_name
	movzbl	(%rax), %ecx
	testb	%cl, %cl
	je	.LBB0_202
# %bb.198:
	incq	%rax
	jmp	.LBB0_200
	.p2align	4
.LBB0_199:                              #   in Loop: Header=BB0_200 Depth=1
	movzbl	(%rax), %ecx
	incq	%rax
	testb	%cl, %cl
	je	.LBB0_202
.LBB0_200:                              # =>This Inner Loop Header: Depth=1
	movl	-44(%rbp), %edx
	leal	1(%rdx), %esi
	cmpl	-48(%rbp), %esi
	jae	.LBB0_199
# %bb.201:                              #   in Loop: Header=BB0_200 Depth=1
	movq	-56(%rbp), %rdi
	movl	%esi, -44(%rbp)
	movb	%cl, (%rdi,%rdx)
	movq	-56(%rbp), %rcx
	movl	-44(%rbp), %edx
	movb	$0, (%rcx,%rdx)
	jmp	.LBB0_199
.LBB0_202:
	movb	$44, %cl
	xorl	%eax, %eax
	leaq	.L.str.10(%rip), %rbx
	jmp	.LBB0_204
	.p2align	4
.LBB0_203:                              #   in Loop: Header=BB0_204 Depth=1
	movzbl	1(%rax,%rbx), %ecx
	incq	%rax
	cmpq	$2, %rax
	je	.LBB0_206
.LBB0_204:                              # =>This Inner Loop Header: Depth=1
	movl	-44(%rbp), %edx
	leal	1(%rdx), %esi
	cmpl	-48(%rbp), %esi
	jae	.LBB0_203
# %bb.205:                              #   in Loop: Header=BB0_204 Depth=1
	movq	-56(%rbp), %rdi
	movl	%esi, -44(%rbp)
	movb	%cl, (%rdi,%rdx)
	movq	-56(%rbp), %rcx
	movl	-44(%rbp), %edx
	movb	$0, (%rcx,%rdx)
	jmp	.LBB0_203
.LBB0_206:
	leaq	-56(%rbp), %rdi
	leaq	-176(%rbp), %rsi
	xorl	%r15d, %r15d
	movl	-64(%rbp), %edx                 # 4-byte Reload
	movl	-68(%rbp), %ecx                 # 4-byte Reload
	xorl	%r8d, %r8d
	xorl	%r9d, %r9d
	callq	render_rm
	movb	$44, %al
	jmp	.LBB0_208
	.p2align	4
.LBB0_207:                              #   in Loop: Header=BB0_208 Depth=1
	movzbl	1(%r15,%rbx), %eax
	incq	%r15
	cmpq	$2, %r15
	je	.LBB0_210
.LBB0_208:                              # =>This Inner Loop Header: Depth=1
	movl	-44(%rbp), %ecx
	leal	1(%rcx), %edx
	cmpl	-48(%rbp), %edx
	jae	.LBB0_207
# %bb.209:                              #   in Loop: Header=BB0_208 Depth=1
	movq	-56(%rbp), %rsi
	movl	%edx, -44(%rbp)
	movb	%al, (%rsi,%rcx)
	movq	-56(%rbp), %rax
	movl	-44(%rbp), %ecx
	movb	$0, (%rax,%rcx)
	jmp	.LBB0_207
.LBB0_210:
	leaq	-56(%rbp), %rdi
	movq	%r12, %rsi
	callq	sb_0xhex
	cmpl	$0, -164(%rbp)
	setne	%sil
	je	.LBB0_278
# %bb.211:
	movq	-136(%rbp), %rbx
	jmp	.LBB0_279
.LBB0_212:
	leaq	R64(%rip), %rsi
	jmp	.LBB0_215
.LBB0_213:
	leaq	R16(%rip), %rsi
	jmp	.LBB0_215
.LBB0_214:
	cmpl	$64, -108(%rbp)                 # 4-byte Folded Reload
	leaq	R8L(%rip), %rcx
	leaq	R8H(%rip), %rsi
	cmoveq	%rcx, %rsi
.LBB0_215:
	movq	(%rsi), %rsi
	movzbl	(%rsi), %edi
	testb	%dil, %dil
	je	.LBB0_220
# %bb.216:
	incq	%rsi
	jmp	.LBB0_218
	.p2align	4
.LBB0_217:                              #   in Loop: Header=BB0_218 Depth=1
	movzbl	(%rsi), %edi
	incq	%rsi
	testb	%dil, %dil
	je	.LBB0_220
.LBB0_218:                              # =>This Inner Loop Header: Depth=1
	movl	-44(%rbp), %ecx
	leal	1(%rcx), %r8d
	cmpl	-48(%rbp), %r8d
	jae	.LBB0_217
# %bb.219:                              #   in Loop: Header=BB0_218 Depth=1
	movq	-56(%rbp), %r9
	movl	%r8d, -44(%rbp)
	movb	%dil, (%r9,%rcx)
	movq	-56(%rbp), %rcx
	movl	-44(%rbp), %edi
	movb	$0, (%rcx,%rdi)
	jmp	.LBB0_217
.LBB0_220:
	movb	$44, %r8b
	xorl	%esi, %esi
	leaq	.L.str.10(%rip), %rdi
	jmp	.LBB0_222
	.p2align	4
.LBB0_221:                              #   in Loop: Header=BB0_222 Depth=1
	movzbl	1(%rsi,%rdi), %r8d
	incq	%rsi
	cmpq	$2, %rsi
	je	.LBB0_224
.LBB0_222:                              # =>This Inner Loop Header: Depth=1
	movl	-44(%rbp), %ecx
	leal	1(%rcx), %r9d
	cmpl	-48(%rbp), %r9d
	jae	.LBB0_221
# %bb.223:                              #   in Loop: Header=BB0_222 Depth=1
	movq	-56(%rbp), %r10
	movl	%r9d, -44(%rbp)
	movb	%r8b, (%r10,%rcx)
	movq	-56(%rbp), %rcx
	movl	-44(%rbp), %r8d
	movb	$0, (%rcx,%r8)
	jmp	.LBB0_221
.LBB0_224:
	movb	$48, %r8b
	xorl	%esi, %esi
	leaq	.L.str.117(%rip), %rdi
	jmp	.LBB0_226
	.p2align	4
.LBB0_225:                              #   in Loop: Header=BB0_226 Depth=1
	movzbl	1(%rsi,%rdi), %r8d
	incq	%rsi
	cmpq	$2, %rsi
	je	.LBB0_228
.LBB0_226:                              # =>This Inner Loop Header: Depth=1
	movl	-44(%rbp), %ecx
	leal	1(%rcx), %r9d
	cmpl	-48(%rbp), %r9d
	jae	.LBB0_225
# %bb.227:                              #   in Loop: Header=BB0_226 Depth=1
	movq	-56(%rbp), %r10
	movl	%r9d, -44(%rbp)
	movb	%r8b, (%r10,%rcx)
	movq	-56(%rbp), %rcx
	movl	-44(%rbp), %r8d
	movb	$0, (%rcx,%r8)
	jmp	.LBB0_225
.LBB0_228:
	testq	%rdx, %rdx
	je	.LBB0_325
# %bb.229:
	xorl	%esi, %esi
	movq	%rdx, %rdi
	.p2align	4
.LBB0_230:                              # =>This Inner Loop Header: Depth=1
	movl	%edx, %ecx
	andl	$15, %ecx
	leal	87(%rcx), %r8d
	leal	48(%rcx), %r9d
	cmpl	$10, %ecx
	movzbl	%r9b, %ecx
	movzbl	%r8b, %r8d
	cmovbl	%ecx, %r8d
	movb	%r8b, -176(%rbp,%rsi)
	incq	%rsi
	shrq	$4, %rdi
	cmpq	$15, %rdx
	movq	%rdi, %rdx
	ja	.LBB0_230
	jmp	.LBB0_232
	.p2align	4
.LBB0_231:                              #   in Loop: Header=BB0_232 Depth=1
	decq	%rsi
	je	.LBB0_327
.LBB0_232:                              # =>This Inner Loop Header: Depth=1
	movl	-44(%rbp), %edx
	leal	1(%rdx), %ecx
	cmpl	-48(%rbp), %ecx
	jae	.LBB0_231
# %bb.233:                              #   in Loop: Header=BB0_232 Depth=1
	movzbl	-177(%rbp,%rsi), %edi
	movq	-56(%rbp), %r8
	movl	%ecx, -44(%rbp)
	movb	%dil, (%r8,%rdx)
	movq	-56(%rbp), %rcx
	movl	-44(%rbp), %edx
	movb	$0, (%rcx,%rdx)
	jmp	.LBB0_231
.LBB0_234:
	movl	-60(%rbp), %r14d                # 4-byte Reload
	movq	%r11, %r15
	leaq	-104(%rbp), %rdi
	leaq	-176(%rbp), %r8
                                        # kill: def $ecx killed $ecx killed $rcx
	callq	parse_modrm
	movb	$115, %dl
	movl	$1, %eax
	leaq	.L.str.8(%rip), %rcx
	jmp	.LBB0_236
	.p2align	4
.LBB0_235:                              #   in Loop: Header=BB0_236 Depth=1
	movzbl	(%rax,%rcx), %edx
	incq	%rax
	cmpq	$4, %rax
	je	.LBB0_238
.LBB0_236:                              # =>This Inner Loop Header: Depth=1
	movl	-44(%rbp), %esi
	leal	1(%rsi), %edi
	cmpl	-48(%rbp), %edi
	jae	.LBB0_235
# %bb.237:                              #   in Loop: Header=BB0_236 Depth=1
	movq	-56(%rbp), %r8
	movl	%edi, -44(%rbp)
	movb	%dl, (%r8,%rsi)
	movq	-56(%rbp), %rdx
	movl	-44(%rbp), %esi
	movb	$0, (%rdx,%rsi)
	jmp	.LBB0_235
.LBB0_238:
	andl	$15, %r12d
	leaq	CC(%rip), %rax
	movq	(%rax,%r12,8), %rax
	movzbl	(%rax), %ecx
	testb	%cl, %cl
	je	.LBB0_243
# %bb.239:
	incq	%rax
	jmp	.LBB0_241
	.p2align	4
.LBB0_240:                              #   in Loop: Header=BB0_241 Depth=1
	movzbl	(%rax), %ecx
	incq	%rax
	testb	%cl, %cl
	je	.LBB0_243
.LBB0_241:                              # =>This Inner Loop Header: Depth=1
	movl	-44(%rbp), %edx
	leal	1(%rdx), %esi
	cmpl	-48(%rbp), %esi
	jae	.LBB0_240
# %bb.242:                              #   in Loop: Header=BB0_241 Depth=1
	movq	-56(%rbp), %rdi
	movl	%esi, -44(%rbp)
	movb	%cl, (%rdi,%rdx)
	movq	-56(%rbp), %rcx
	movl	-44(%rbp), %edx
	movb	$0, (%rcx,%rdx)
	jmp	.LBB0_240
.LBB0_243:
	movl	-44(%rbp), %eax
	leal	1(%rax), %ecx
	cmpl	-48(%rbp), %ecx
	jae	.LBB0_245
# %bb.244:
	movq	-56(%rbp), %rdx
	movl	%ecx, -44(%rbp)
	movb	$32, (%rdx,%rax)
	movq	-56(%rbp), %rax
	movl	-44(%rbp), %ecx
	movb	$0, (%rax,%rcx)
.LBB0_245:
	leaq	-56(%rbp), %rdi
	leaq	-176(%rbp), %rsi
	xorl	%ebx, %ebx
	movl	$1, %edx
	movl	-68(%rbp), %ecx                 # 4-byte Reload
	movl	$1, %r8d
	jmp	.LBB0_267
.LBB0_246:
	movl	-60(%rbp), %r14d                # 4-byte Reload
	movq	%r11, %r15
	leaq	-104(%rbp), %rdi
	leaq	-176(%rbp), %r8
                                        # kill: def $ecx killed $ecx killed $rcx
	callq	parse_modrm
	movb	$99, %dl
	xorl	%eax, %eax
	leaq	.L.str.9(%rip), %rcx
	jmp	.LBB0_248
	.p2align	4
.LBB0_247:                              #   in Loop: Header=BB0_248 Depth=1
	movzbl	1(%rax,%rcx), %edx
	incq	%rax
	cmpq	$4, %rax
	je	.LBB0_250
.LBB0_248:                              # =>This Inner Loop Header: Depth=1
	movl	-44(%rbp), %esi
	leal	1(%rsi), %edi
	cmpl	-48(%rbp), %edi
	jae	.LBB0_247
# %bb.249:                              #   in Loop: Header=BB0_248 Depth=1
	movq	-56(%rbp), %r8
	movl	%edi, -44(%rbp)
	movb	%dl, (%r8,%rsi)
	movq	-56(%rbp), %rdx
	movl	-44(%rbp), %esi
	movb	$0, (%rdx,%rsi)
	jmp	.LBB0_247
.LBB0_250:
	andl	$15, %r12d
	leaq	CC(%rip), %rax
	movq	(%rax,%r12,8), %rax
	movzbl	(%rax), %ecx
	testb	%cl, %cl
	je	.LBB0_255
# %bb.251:
	incq	%rax
	jmp	.LBB0_253
	.p2align	4
.LBB0_252:                              #   in Loop: Header=BB0_253 Depth=1
	movzbl	(%rax), %ecx
	incq	%rax
	testb	%cl, %cl
	je	.LBB0_255
.LBB0_253:                              # =>This Inner Loop Header: Depth=1
	movl	-44(%rbp), %edx
	leal	1(%rdx), %esi
	cmpl	-48(%rbp), %esi
	jae	.LBB0_252
# %bb.254:                              #   in Loop: Header=BB0_253 Depth=1
	movq	-56(%rbp), %rdi
	movl	%esi, -44(%rbp)
	movb	%cl, (%rdi,%rdx)
	movq	-56(%rbp), %rcx
	movl	-44(%rbp), %edx
	movb	$0, (%rcx,%rdx)
	jmp	.LBB0_252
.LBB0_255:
	movl	-44(%rbp), %eax
	leal	1(%rax), %ecx
	cmpl	-48(%rbp), %ecx
	jae	.LBB0_257
# %bb.256:
	movq	-56(%rbp), %rdx
	movl	%ecx, -44(%rbp)
	movb	$32, (%rdx,%rax)
	movq	-56(%rbp), %rax
	movl	-44(%rbp), %ecx
	movb	$0, (%rax,%rcx)
.LBB0_257:
	movl	-172(%rbp), %edi
	movl	-64(%rbp), %esi                 # 4-byte Reload
	movl	-68(%rbp), %edx                 # 4-byte Reload
	callq	reg_name
	movzbl	(%rax), %ecx
	testb	%cl, %cl
	je	.LBB0_262
# %bb.258:
	incq	%rax
	jmp	.LBB0_260
	.p2align	4
.LBB0_259:                              #   in Loop: Header=BB0_260 Depth=1
	movzbl	(%rax), %ecx
	incq	%rax
	testb	%cl, %cl
	je	.LBB0_262
.LBB0_260:                              # =>This Inner Loop Header: Depth=1
	movl	-44(%rbp), %edx
	leal	1(%rdx), %esi
	cmpl	-48(%rbp), %esi
	jae	.LBB0_259
# %bb.261:                              #   in Loop: Header=BB0_260 Depth=1
	movq	-56(%rbp), %rdi
	movl	%esi, -44(%rbp)
	movb	%cl, (%rdi,%rdx)
	movq	-56(%rbp), %rcx
	movl	-44(%rbp), %edx
	movb	$0, (%rcx,%rdx)
	jmp	.LBB0_259
.LBB0_262:
	movb	$44, %dl
	xorl	%eax, %eax
	leaq	.L.str.10(%rip), %rcx
	jmp	.LBB0_264
	.p2align	4
.LBB0_263:                              #   in Loop: Header=BB0_264 Depth=1
	movzbl	1(%rax,%rcx), %edx
	incq	%rax
	cmpq	$2, %rax
	je	.LBB0_266
.LBB0_264:                              # =>This Inner Loop Header: Depth=1
	movl	-44(%rbp), %esi
	leal	1(%rsi), %edi
	cmpl	-48(%rbp), %edi
	jae	.LBB0_263
# %bb.265:                              #   in Loop: Header=BB0_264 Depth=1
	movq	-56(%rbp), %r8
	movl	%edi, -44(%rbp)
	movb	%dl, (%r8,%rsi)
	movq	-56(%rbp), %rdx
	movl	-44(%rbp), %esi
	movb	$0, (%rdx,%rsi)
	jmp	.LBB0_263
.LBB0_266:
	leaq	-56(%rbp), %rdi
	leaq	-176(%rbp), %rsi
	xorl	%ebx, %ebx
	movl	-64(%rbp), %edx                 # 4-byte Reload
	movl	-68(%rbp), %ecx                 # 4-byte Reload
	xorl	%r8d, %r8d
.LBB0_267:
	xorl	%r9d, %r9d
	callq	render_rm
	xorl	%esi, %esi
.LBB0_268:
	movq	%r15, %r11
	movl	%r14d, %r15d
	jmp	.LBB0_328
.LBB0_269:
	movl	%r12d, %ebx
	andl	$-2, %ebx
	movl	%ebx, %edi
	orl	$8, %edi
	cmpl	$190, %edi
	jne	.LBB0_360
# %bb.270:
	movl	-60(%rbp), %r14d                # 4-byte Reload
	movq	%r11, %r15
	leaq	-104(%rbp), %rdi
	leaq	-176(%rbp), %r8
                                        # kill: def $ecx killed $ecx killed $rcx
	callq	parse_modrm
	testb	$8, %r12b
	leaq	.L.str.12(%rip), %rcx
	leaq	.L.str.11(%rip), %rax
	cmoveq	%rcx, %rax
	movzbl	(%rax), %ecx
	testb	%cl, %cl
	je	.LBB0_280
# %bb.271:
	incq	%rax
	jmp	.LBB0_273
.LBB0_272:                              #   in Loop: Header=BB0_273 Depth=1
	movzbl	(%rax), %ecx
	incq	%rax
	testb	%cl, %cl
	je	.LBB0_280
.LBB0_273:                              # =>This Inner Loop Header: Depth=1
	movl	-44(%rbp), %edx
	leal	1(%rdx), %esi
	cmpl	-48(%rbp), %esi
	jae	.LBB0_272
# %bb.274:                              #   in Loop: Header=BB0_273 Depth=1
	movq	-56(%rbp), %rdi
	movl	%esi, -44(%rbp)
	movb	%cl, (%rdi,%rdx)
	movq	-56(%rbp), %rcx
	movl	-44(%rbp), %edx
	movb	$0, (%rcx,%rdx)
	jmp	.LBB0_272
.LBB0_447:
	leaq	-56(%rbp), %rdi
	leaq	-176(%rbp), %rsi
	callq	render_mem
	movq	%r15, %r11
	movl	-60(%rbp), %r15d                # 4-byte Reload
	jmp	.LBB0_448
.LBB0_275:
	movl	%ebx, %r9d
	andl	$-16, %r9d
	cmpl	$112, %r9d
	jne	.LBB0_354
# %bb.276:
	movl	-92(%rbp), %edx
	cmpl	%r10d, %edx
	movq	-80(%rbp), %rcx                 # 8-byte Reload
	jae	.LBB0_388
# %bb.277:
	movq	-104(%rbp), %rsi
	leal	1(%rdx), %edi
	movl	%edi, -92(%rbp)
	movsbq	(%rsi,%rdx), %rsi
	jmp	.LBB0_389
.LBB0_278:
	xorl	%ebx, %ebx
.LBB0_279:
	movq	-80(%rbp), %r11                 # 8-byte Reload
	movl	%r14d, %r15d
	jmp	.LBB0_328
.LBB0_280:
	andl	$1, %r12d
	movl	-172(%rbp), %edi
	movl	-64(%rbp), %esi                 # 4-byte Reload
	movl	-68(%rbp), %edx                 # 4-byte Reload
	callq	reg_name
	movzbl	(%rax), %ecx
	testb	%cl, %cl
	je	.LBB0_285
# %bb.281:
	incq	%rax
	jmp	.LBB0_283
.LBB0_282:                              #   in Loop: Header=BB0_283 Depth=1
	movzbl	(%rax), %ecx
	incq	%rax
	testb	%cl, %cl
	je	.LBB0_285
.LBB0_283:                              # =>This Inner Loop Header: Depth=1
	movl	-44(%rbp), %edx
	leal	1(%rdx), %esi
	cmpl	-48(%rbp), %esi
	jae	.LBB0_282
# %bb.284:                              #   in Loop: Header=BB0_283 Depth=1
	movq	-56(%rbp), %rdi
	movl	%esi, -44(%rbp)
	movb	%cl, (%rdi,%rdx)
	movq	-56(%rbp), %rcx
	movl	-44(%rbp), %edx
	movb	$0, (%rcx,%rdx)
	jmp	.LBB0_282
.LBB0_285:
	incl	%r12d
	movb	$44, %dl
	xorl	%eax, %eax
	leaq	.L.str.10(%rip), %rcx
	jmp	.LBB0_287
	.p2align	4
.LBB0_286:                              #   in Loop: Header=BB0_287 Depth=1
	movzbl	1(%rax,%rcx), %edx
	incq	%rax
	cmpq	$2, %rax
	je	.LBB0_289
.LBB0_287:                              # =>This Inner Loop Header: Depth=1
	movl	-44(%rbp), %esi
	leal	1(%rsi), %edi
	cmpl	-48(%rbp), %edi
	jae	.LBB0_286
# %bb.288:                              #   in Loop: Header=BB0_287 Depth=1
	movq	-56(%rbp), %r8
	movl	%edi, -44(%rbp)
	movb	%dl, (%r8,%rsi)
	movq	-56(%rbp), %rdx
	movl	-44(%rbp), %esi
	movb	$0, (%rdx,%rsi)
	jmp	.LBB0_286
.LBB0_289:
	xorl	%ebx, %ebx
	cmpl	$0, -176(%rbp)
	movl	$0, %r8d
	cmovel	%r12d, %r8d
	leaq	-56(%rbp), %rdi
	leaq	-176(%rbp), %rsi
	movl	%r12d, %edx
	movl	-68(%rbp), %ecx                 # 4-byte Reload
.LBB0_290:
	xorl	%r9d, %r9d
	callq	render_rm
	cmpl	$0, -164(%rbp)
	setne	%sil
	je	.LBB0_268
# %bb.291:
	movq	-136(%rbp), %rbx
	jmp	.LBB0_268
.LBB0_292:
	andl	$15, %eax
	leaq	R32(%rip), %rcx
	jmp	.LBB0_370
.LBB0_293:
	andl	$15, %eax
	leaq	R16(%rip), %rcx
	jmp	.LBB0_370
.LBB0_294:
	cmpl	$64, -108(%rbp)                 # 4-byte Folded Reload
	jne	.LBB0_369
# %bb.295:
	andl	$15, %eax
	leaq	R8L(%rip), %rcx
	jmp	.LBB0_370
.LBB0_296:
	movl	$1, -88(%rbp)
	xorl	%esi, %esi
.LBB0_297:
	movl	-60(%rbp), %r15d                # 4-byte Reload
	movl	-92(%rbp), %edx
	cmpl	%r10d, %edx
	jae	.LBB0_299
# %bb.298:
	movq	-104(%rbp), %rcx
	leal	1(%rdx), %edi
	movl	%edi, -92(%rbp)
	movzbl	(%rcx,%rdx), %edx
	shll	$8, %edx
	jmp	.LBB0_300
.LBB0_299:
	movl	$1, -88(%rbp)
	xorl	%edx, %edx
.LBB0_300:
	movl	-92(%rbp), %edi
	cmpl	%r10d, %edi
	jae	.LBB0_302
# %bb.301:
	movq	-104(%rbp), %rcx
	leal	1(%rdi), %r8d
	movl	%r8d, -92(%rbp)
	movzbl	(%rcx,%rdi), %edi
	shll	$16, %edi
	jmp	.LBB0_303
.LBB0_302:
	movl	$1, -88(%rbp)
	xorl	%edi, %edi
.LBB0_303:
	movl	-92(%rbp), %r8d
	cmpl	%r10d, %r8d
	jae	.LBB0_305
# %bb.304:
	movq	-104(%rbp), %rcx
	leal	1(%r8), %r9d
	movl	%r9d, -92(%rbp)
	movzbl	(%rcx,%r8), %ecx
	shll	$24, %ecx
	jmp	.LBB0_306
.LBB0_305:
	movl	$1, -88(%rbp)
	xorl	%ecx, %ecx
.LBB0_306:
	orl	%ecx, %edi
	orl	%esi, %edx
	movl	-44(%rbp), %ecx
	leal	1(%rcx), %esi
	cmpl	-48(%rbp), %esi
	jae	.LBB0_308
# %bb.307:
	movq	-56(%rbp), %r8
	movl	%esi, -44(%rbp)
	movb	$106, (%r8,%rcx)
	movq	-56(%rbp), %rcx
	movl	-44(%rbp), %esi
	movb	$0, (%rcx,%rsi)
.LBB0_308:
	orl	%edi, %edx
	andl	$15, %r12d
	leaq	CC(%rip), %rcx
	movq	(%rcx,%r12,8), %rsi
	movzbl	(%rsi), %edi
	testb	%dil, %dil
	je	.LBB0_313
# %bb.309:
	incq	%rsi
	jmp	.LBB0_311
	.p2align	4
.LBB0_310:                              #   in Loop: Header=BB0_311 Depth=1
	movzbl	(%rsi), %edi
	incq	%rsi
	testb	%dil, %dil
	je	.LBB0_313
.LBB0_311:                              # =>This Inner Loop Header: Depth=1
	movl	-44(%rbp), %ecx
	leal	1(%rcx), %r8d
	cmpl	-48(%rbp), %r8d
	jae	.LBB0_310
# %bb.312:                              #   in Loop: Header=BB0_311 Depth=1
	movq	-56(%rbp), %r9
	movl	%r8d, -44(%rbp)
	movb	%dil, (%r9,%rcx)
	movq	-56(%rbp), %rcx
	movl	-44(%rbp), %edi
	movb	$0, (%rcx,%rdi)
	jmp	.LBB0_310
.LBB0_313:
	movslq	%edx, %rdx
	movl	-44(%rbp), %ecx
	leal	1(%rcx), %esi
	cmpl	-48(%rbp), %esi
	jae	.LBB0_315
# %bb.314:
	movq	-56(%rbp), %rdi
	movl	%esi, -44(%rbp)
	movb	$32, (%rdi,%rcx)
	movq	-56(%rbp), %rcx
	movl	-44(%rbp), %esi
	movb	$0, (%rcx,%rsi)
.LBB0_315:
	movl	$1, 12(%r11)
	movl	-92(%rbp), %ecx
	addq	-184(%rbp), %rdx                # 8-byte Folded Reload
	addq	%rcx, %rdx
	movq	%rdx, 16(%r11)
	movb	$48, %r8b
	xorl	%esi, %esi
	leaq	.L.str.117(%rip), %rdi
	jmp	.LBB0_317
	.p2align	4
.LBB0_316:                              #   in Loop: Header=BB0_317 Depth=1
	movzbl	1(%rsi,%rdi), %r8d
	incq	%rsi
	cmpq	$2, %rsi
	je	.LBB0_319
.LBB0_317:                              # =>This Inner Loop Header: Depth=1
	movl	-44(%rbp), %ecx
	leal	1(%rcx), %r9d
	cmpl	-48(%rbp), %r9d
	jae	.LBB0_316
# %bb.318:                              #   in Loop: Header=BB0_317 Depth=1
	movq	-56(%rbp), %r10
	movl	%r9d, -44(%rbp)
	movb	%r8b, (%r10,%rcx)
	movq	-56(%rbp), %rcx
	movl	-44(%rbp), %r8d
	movb	$0, (%rcx,%r8)
	jmp	.LBB0_316
.LBB0_319:
	testq	%rdx, %rdx
	je	.LBB0_325
# %bb.320:
	xorl	%esi, %esi
	movq	%rdx, %rdi
	.p2align	4
.LBB0_321:                              # =>This Inner Loop Header: Depth=1
	movl	%edx, %ecx
	andl	$15, %ecx
	leal	87(%rcx), %r8d
	leal	48(%rcx), %r9d
	cmpl	$10, %ecx
	movzbl	%r9b, %ecx
	movzbl	%r8b, %r8d
	cmovbl	%ecx, %r8d
	movb	%r8b, -176(%rbp,%rsi)
	incq	%rsi
	shrq	$4, %rdi
	cmpq	$15, %rdx
	movq	%rdi, %rdx
	ja	.LBB0_321
	jmp	.LBB0_323
	.p2align	4
.LBB0_322:                              #   in Loop: Header=BB0_323 Depth=1
	decq	%rsi
	je	.LBB0_327
.LBB0_323:                              # =>This Inner Loop Header: Depth=1
	movl	-44(%rbp), %edx
	leal	1(%rdx), %ecx
	cmpl	-48(%rbp), %ecx
	jae	.LBB0_322
# %bb.324:                              #   in Loop: Header=BB0_323 Depth=1
	movzbl	-177(%rbp,%rsi), %edi
	movq	-56(%rbp), %r8
	movl	%ecx, -44(%rbp)
	movb	%dil, (%r8,%rdx)
	movq	-56(%rbp), %rcx
	movl	-44(%rbp), %edx
	movb	$0, (%rcx,%rdx)
	jmp	.LBB0_322
.LBB0_325:
	movl	-44(%rbp), %ecx
	leal	1(%rcx), %edx
	cmpl	-48(%rbp), %edx
	jae	.LBB0_327
# %bb.326:
	movq	-56(%rbp), %rsi
	movl	%edx, -44(%rbp)
	movb	$48, (%rsi,%rcx)
	movq	-56(%rbp), %rcx
	movl	-44(%rbp), %edx
	movb	$0, (%rcx,%rdx)
.LBB0_327:
	xorl	%ebx, %ebx
	xorl	%esi, %esi
.LBB0_328:
	cmpl	$0, -88(%rbp)
	je	.LBB0_335
# %bb.329:
	movq	-192(%rbp), %r10                # 8-byte Reload
	movb	$0, (%r10)
	movb	$40, %dil
	xorl	%r8d, %r8d
	movl	$1, %edx
	leaq	.L.str.48(%rip), %rsi
	jmp	.LBB0_332
	.p2align	4
.LBB0_330:                              #   in Loop: Header=BB0_332 Depth=1
	movl	%r8d, %ecx
	movb	%dil, (%r10,%rcx)
	movl	%r9d, %ecx
	movb	$0, (%r10,%rcx)
.LBB0_331:                              #   in Loop: Header=BB0_332 Depth=1
	movzbl	(%rdx,%rsi), %edi
	incq	%rdx
	movl	%r9d, %r8d
	cmpq	$6, %rdx
	je	.LBB0_334
.LBB0_332:                              # =>This Inner Loop Header: Depth=1
	leal	1(%r8), %r9d
	cmpl	$63, %r9d
	jbe	.LBB0_330
# %bb.333:                              #   in Loop: Header=BB0_332 Depth=1
	movl	%r8d, %r9d
	jmp	.LBB0_331
.LBB0_334:
	movl	$0, 12(%r11)
	jmp	.LBB0_353
.LBB0_335:
	testb	%sil, %sil
	je	.LBB0_352
# %bb.336:
	movl	-92(%rbp), %eax
	addq	-184(%rbp), %rbx                # 8-byte Folded Reload
	movb	$32, %dil
	movl	$1, %edx
	leaq	.L.str.100(%rip), %rsi
	jmp	.LBB0_338
	.p2align	4
.LBB0_337:                              #   in Loop: Header=BB0_338 Depth=1
	movzbl	(%rdx,%rsi), %edi
	incq	%rdx
	cmpq	$11, %rdx
	je	.LBB0_340
.LBB0_338:                              # =>This Inner Loop Header: Depth=1
	movl	-44(%rbp), %ecx
	leal	1(%rcx), %r8d
	cmpl	-48(%rbp), %r8d
	jae	.LBB0_337
# %bb.339:                              #   in Loop: Header=BB0_338 Depth=1
	movq	-56(%rbp), %r9
	movl	%r8d, -44(%rbp)
	movb	%dil, (%r9,%rcx)
	movq	-56(%rbp), %rcx
	movl	-44(%rbp), %edi
	movb	$0, (%rcx,%rdi)
	jmp	.LBB0_337
.LBB0_340:
	movb	$48, %dil
	xorl	%edx, %edx
	leaq	.L.str.117(%rip), %rsi
	jmp	.LBB0_342
	.p2align	4
.LBB0_341:                              #   in Loop: Header=BB0_342 Depth=1
	movzbl	1(%rdx,%rsi), %edi
	incq	%rdx
	cmpq	$2, %rdx
	je	.LBB0_344
.LBB0_342:                              # =>This Inner Loop Header: Depth=1
	movl	-44(%rbp), %ecx
	leal	1(%rcx), %r8d
	cmpl	-48(%rbp), %r8d
	jae	.LBB0_341
# %bb.343:                              #   in Loop: Header=BB0_342 Depth=1
	movq	-56(%rbp), %r9
	movl	%r8d, -44(%rbp)
	movb	%dil, (%r9,%rcx)
	movq	-56(%rbp), %rcx
	movl	-44(%rbp), %edi
	movb	$0, (%rcx,%rdi)
	jmp	.LBB0_341
.LBB0_344:
	addq	%rax, %rbx
	je	.LBB0_350
# %bb.345:
	xorl	%eax, %eax
	movq	%rbx, %rdx
	.p2align	4
.LBB0_346:                              # =>This Inner Loop Header: Depth=1
	movl	%ebx, %ecx
	andl	$15, %ecx
	leal	87(%rcx), %esi
	leal	48(%rcx), %edi
	cmpl	$10, %ecx
	movzbl	%dil, %ecx
	movzbl	%sil, %esi
	cmovbl	%ecx, %esi
	movb	%sil, -176(%rbp,%rax)
	incq	%rax
	shrq	$4, %rdx
	cmpq	$15, %rbx
	movq	%rdx, %rbx
	ja	.LBB0_346
	jmp	.LBB0_348
	.p2align	4
.LBB0_347:                              #   in Loop: Header=BB0_348 Depth=1
	decq	%rax
	je	.LBB0_352
.LBB0_348:                              # =>This Inner Loop Header: Depth=1
	movl	-44(%rbp), %edx
	leal	1(%rdx), %ecx
	cmpl	-48(%rbp), %ecx
	jae	.LBB0_347
# %bb.349:                              #   in Loop: Header=BB0_348 Depth=1
	movzbl	-177(%rbp,%rax), %esi
	movq	-56(%rbp), %rdi
	movl	%ecx, -44(%rbp)
	movb	%sil, (%rdi,%rdx)
	movq	-56(%rbp), %rcx
	movl	-44(%rbp), %edx
	movb	$0, (%rcx,%rdx)
	jmp	.LBB0_347
.LBB0_350:
	movl	-44(%rbp), %eax
	leal	1(%rax), %ecx
	cmpl	-48(%rbp), %ecx
	jae	.LBB0_352
# %bb.351:
	movq	-56(%rbp), %rdx
	movl	%ecx, -44(%rbp)
	movb	$48, (%rdx,%rax)
	movq	-56(%rbp), %rax
	movl	-44(%rbp), %ecx
	movb	$0, (%rax,%rcx)
.LBB0_352:
	movl	-92(%rbp), %r15d
.LBB0_353:
	cmpl	$1, %r15d
	adcl	$0, %r15d
	movl	%r15d, 8(%r11)
	movl	%r15d, %eax
	addq	$152, %rsp
	popq	%rbx
	popq	%r12
	popq	%r13
	popq	%r14
	popq	%r15
	popq	%rbp
	retq
.LBB0_354:
	movl	%ebx, %r12d
	andl	$-2, %r12d
	cmpl	$128, %r12d
	setne	%r9b
	cmpl	$131, %ebx
	setne	%r10b
	testb	%r9b, %r10b
	jne	.LBB0_399
# %bb.355:
	cmpl	$128, %ebx
	movl	$1, %r14d
	movl	-64(%rbp), %r13d                # 4-byte Reload
	movl	%r13d, %r12d
	cmovel	%r14d, %r12d
	leaq	-104(%rbp), %r15
	leaq	-176(%rbp), %r8
	movq	%r15, %rdi
                                        # kill: def $ecx killed $ecx killed $rcx
	callq	parse_modrm
	xorl	%eax, %eax
	cmpl	$2, %r13d
	setne	%al
	cmpl	$129, %ebx
	leal	2(%rax,%rax), %esi
	cmovnel	%r14d, %esi
	movq	%r15, %rdi
	callq	rd_imm_sext
	movq	%rax, %r14
	movslq	-128(%rbp), %rax
	leaq	ALU(%rip), %rcx
	movq	(%rcx,%rax,8), %rax
	movzbl	(%rax), %ecx
	testb	%cl, %cl
	je	.LBB0_405
# %bb.356:
	incq	%rax
	jmp	.LBB0_358
.LBB0_357:                              #   in Loop: Header=BB0_358 Depth=1
	movzbl	(%rax), %ecx
	incq	%rax
	testb	%cl, %cl
	je	.LBB0_405
.LBB0_358:                              # =>This Inner Loop Header: Depth=1
	movl	-44(%rbp), %edx
	leal	1(%rdx), %esi
	cmpl	-48(%rbp), %esi
	jae	.LBB0_357
# %bb.359:                              #   in Loop: Header=BB0_358 Depth=1
	movq	-56(%rbp), %rdi
	movl	%esi, -44(%rbp)
	movb	%cl, (%rdi,%rdx)
	movq	-56(%rbp), %rcx
	movl	-44(%rbp), %edx
	movb	$0, (%rcx,%rdx)
	jmp	.LBB0_357
.LBB0_360:
	cmpl	$175, %r12d
	jne	.LBB0_415
# %bb.361:
	movl	-60(%rbp), %r14d                # 4-byte Reload
	movq	%r11, %r15
	leaq	-104(%rbp), %rdi
	leaq	-176(%rbp), %r8
                                        # kill: def $ecx killed $ecx killed $rcx
	callq	parse_modrm
	movb	$105, %dl
	movl	$1, %eax
	leaq	.L.str.13(%rip), %rcx
	jmp	.LBB0_363
.LBB0_362:                              #   in Loop: Header=BB0_363 Depth=1
	movzbl	(%rax,%rcx), %edx
	incq	%rax
	cmpq	$6, %rax
	je	.LBB0_430
.LBB0_363:                              # =>This Inner Loop Header: Depth=1
	movl	-44(%rbp), %esi
	leal	1(%rsi), %edi
	cmpl	-48(%rbp), %edi
	jae	.LBB0_362
# %bb.364:                              #   in Loop: Header=BB0_363 Depth=1
	movq	-56(%rbp), %r8
	movl	%edi, -44(%rbp)
	movb	%dl, (%r8,%rsi)
	movq	-56(%rbp), %rdx
	movl	-44(%rbp), %esi
	movb	$0, (%rdx,%rsi)
	jmp	.LBB0_362
.LBB0_365:
	andl	$15, %edx
	leaq	R32(%rip), %rcx
	jmp	.LBB0_441
.LBB0_366:
	andl	$15, %edx
	leaq	R16(%rip), %rax
	leaq	(%rax,%rdx,8), %rdx
	movq	%r15, %r11
	movl	-60(%rbp), %r15d                # 4-byte Reload
	jmp	.LBB0_442
.LBB0_367:
	cmpl	$64, -108(%rbp)                 # 4-byte Folded Reload
	jne	.LBB0_440
# %bb.368:
	andl	$15, %edx
	leaq	R8L(%rip), %rcx
	jmp	.LBB0_441
.LBB0_369:
	andl	$7, %eax
	leaq	R8H(%rip), %rcx
.LBB0_370:
	leaq	(%rcx,%rax,8), %rax
	movq	(%rax), %rax
	movzbl	(%rax), %ecx
	testb	%cl, %cl
	je	.LBB0_375
# %bb.371:
	incq	%rax
	jmp	.LBB0_373
	.p2align	4
.LBB0_372:                              #   in Loop: Header=BB0_373 Depth=1
	movzbl	(%rax), %ecx
	incq	%rax
	testb	%cl, %cl
	je	.LBB0_375
.LBB0_373:                              # =>This Inner Loop Header: Depth=1
	movl	-44(%rbp), %edx
	leal	1(%rdx), %esi
	cmpl	-48(%rbp), %esi
	jae	.LBB0_372
# %bb.374:                              #   in Loop: Header=BB0_373 Depth=1
	movq	-56(%rbp), %rdi
	movl	%esi, -44(%rbp)
	movb	%cl, (%rdi,%rdx)
	movq	-56(%rbp), %rcx
	movl	-44(%rbp), %edx
	movb	$0, (%rcx,%rdx)
	jmp	.LBB0_372
.LBB0_375:
	movb	$44, %dl
	xorl	%eax, %eax
	leaq	.L.str.10(%rip), %rcx
	jmp	.LBB0_377
	.p2align	4
.LBB0_376:                              #   in Loop: Header=BB0_377 Depth=1
	movzbl	1(%rax,%rcx), %edx
	incq	%rax
	cmpq	$2, %rax
	je	.LBB0_379
.LBB0_377:                              # =>This Inner Loop Header: Depth=1
	movl	-44(%rbp), %esi
	leal	1(%rsi), %edi
	cmpl	-48(%rbp), %edi
	jae	.LBB0_376
# %bb.378:                              #   in Loop: Header=BB0_377 Depth=1
	movq	-56(%rbp), %r8
	movl	%edi, -44(%rbp)
	movb	%dl, (%r8,%rsi)
	movq	-56(%rbp), %rdx
	movl	-44(%rbp), %esi
	movb	$0, (%rdx,%rsi)
	jmp	.LBB0_376
.LBB0_379:
	cmpl	$0, -176(%rbp)
	je	.LBB0_473
# %bb.380:
	movl	-168(%rbp), %edx
	cmpl	$2, %ebx
	je	.LBB0_385
# %bb.381:
	cmpl	$4, %ebx
	movq	%r15, %r11
	movl	-60(%rbp), %r15d                # 4-byte Reload
	je	.LBB0_384
# %bb.382:
	cmpl	$8, %ebx
	jne	.LBB0_386
# %bb.383:
	andl	$15, %edx
	leaq	R64(%rip), %rcx
	jmp	.LBB0_467
.LBB0_473:
	leaq	-56(%rbp), %rdi
	leaq	-176(%rbp), %rsi
	callq	render_mem
	movq	%r15, %r11
	movl	-60(%rbp), %r15d                # 4-byte Reload
	jmp	.LBB0_474
.LBB0_384:
	andl	$15, %edx
	leaq	R32(%rip), %rcx
	jmp	.LBB0_467
.LBB0_385:
	andl	$15, %edx
	leaq	R16(%rip), %rax
	leaq	(%rax,%rdx,8), %rdx
	movq	%r15, %r11
	movl	-60(%rbp), %r15d                # 4-byte Reload
	jmp	.LBB0_468
.LBB0_386:
	cmpl	$64, -108(%rbp)                 # 4-byte Folded Reload
	jne	.LBB0_466
# %bb.387:
	andl	$15, %edx
	leaq	R8L(%rip), %rcx
	jmp	.LBB0_467
.LBB0_388:
	movl	$1, -88(%rbp)
	xorl	%esi, %esi
.LBB0_389:
	movl	-60(%rbp), %r14d                # 4-byte Reload
	movl	-44(%rbp), %edx
	leal	1(%rdx), %edi
	cmpl	-48(%rbp), %edi
	jae	.LBB0_391
# %bb.390:
	movq	-56(%rbp), %r8
	movl	%edi, -44(%rbp)
	movb	$106, (%r8,%rdx)
	movq	-56(%rbp), %rdx
	movl	-44(%rbp), %edi
	movb	$0, (%rdx,%rdi)
.LBB0_391:
	andl	$15, %ebx
	leaq	CC(%rip), %rdx
	movq	(%rdx,%rbx,8), %rdx
	movzbl	(%rdx), %edi
	testb	%dil, %dil
	je	.LBB0_396
# %bb.392:
	incq	%rdx
	jmp	.LBB0_394
.LBB0_393:                              #   in Loop: Header=BB0_394 Depth=1
	movzbl	(%rdx), %edi
	incq	%rdx
	testb	%dil, %dil
	je	.LBB0_396
.LBB0_394:                              # =>This Inner Loop Header: Depth=1
	movl	-44(%rbp), %r8d
	leal	1(%r8), %r9d
	cmpl	-48(%rbp), %r9d
	jae	.LBB0_393
# %bb.395:                              #   in Loop: Header=BB0_394 Depth=1
	movq	-56(%rbp), %r10
	movl	%r9d, -44(%rbp)
	movb	%dil, (%r10,%r8)
	movq	-56(%rbp), %rdi
	movl	-44(%rbp), %r8d
	movb	$0, (%rdi,%r8)
	jmp	.LBB0_393
.LBB0_396:
	movl	-44(%rbp), %eax
	leal	1(%rax), %edx
	cmpl	-48(%rbp), %edx
	jae	.LBB0_398
# %bb.397:
	movq	-56(%rbp), %rdi
	movl	%edx, -44(%rbp)
	movb	$32, (%rdi,%rax)
	movq	-56(%rbp), %rax
	movl	-44(%rbp), %edx
	movb	$0, (%rax,%rdx)
.LBB0_398:
	movl	$1, 12(%rcx)
	movl	-92(%rbp), %eax
	addq	-184(%rbp), %rsi                # 8-byte Folded Reload
	addq	%rax, %rsi
	movq	%rsi, 16(%rcx)
	leaq	-56(%rbp), %rdi
	callq	sb_0xhex
	movq	-80(%rbp), %r11                 # 8-byte Reload
	xorl	%ebx, %ebx
	xorl	%esi, %esi
	movl	%r14d, %r15d
	jmp	.LBB0_328
.LBB0_399:
	cmpl	$134, %r12d
	je	.LBB0_488
# %bb.400:
	cmpl	$132, %r12d
	jne	.LBB0_501
# %bb.401:
	movl	-60(%rbp), %r14d                # 4-byte Reload
	leaq	-104(%rbp), %rdi
	leaq	-176(%rbp), %r8
                                        # kill: def $ecx killed $ecx killed $rcx
	callq	parse_modrm
	movb	$116, %dl
	movl	$1, %eax
	leaq	.L.str.53(%rip), %rcx
	jmp	.LBB0_403
.LBB0_402:                              #   in Loop: Header=BB0_403 Depth=1
	movzbl	(%rax,%rcx), %edx
	incq	%rax
	cmpq	$6, %rax
	je	.LBB0_479
.LBB0_403:                              # =>This Inner Loop Header: Depth=1
	movl	-44(%rbp), %esi
	leal	1(%rsi), %edi
	cmpl	-48(%rbp), %edi
	jae	.LBB0_402
# %bb.404:                              #   in Loop: Header=BB0_403 Depth=1
	movq	-56(%rbp), %r8
	movl	%edi, -44(%rbp)
	movb	%dl, (%r8,%rsi)
	movq	-56(%rbp), %rdx
	movl	-44(%rbp), %esi
	movb	$0, (%rdx,%rsi)
	jmp	.LBB0_402
.LBB0_405:
	movl	-44(%rbp), %eax
	leal	1(%rax), %ecx
	cmpl	-48(%rbp), %ecx
	jae	.LBB0_407
# %bb.406:
	movq	-56(%rbp), %rdx
	movl	%ecx, -44(%rbp)
	movb	$32, (%rdx,%rax)
	movq	-56(%rbp), %rax
	movl	-44(%rbp), %ecx
	movb	$0, (%rax,%rcx)
.LBB0_407:
	xorl	%ebx, %ebx
	cmpl	$0, -176(%rbp)
	movl	$0, %r8d
	cmovel	%r12d, %r8d
	leaq	-56(%rbp), %rdi
	leaq	-176(%rbp), %rsi
	movl	%r12d, %edx
	movl	-68(%rbp), %ecx                 # 4-byte Reload
	xorl	%r9d, %r9d
	callq	render_rm
	movb	$44, %cl
	leaq	.L.str.10(%rip), %rax
	jmp	.LBB0_409
.LBB0_408:                              #   in Loop: Header=BB0_409 Depth=1
	movzbl	1(%rbx,%rax), %ecx
	incq	%rbx
	cmpq	$2, %rbx
	je	.LBB0_411
.LBB0_409:                              # =>This Inner Loop Header: Depth=1
	movl	-44(%rbp), %edx
	leal	1(%rdx), %esi
	cmpl	-48(%rbp), %esi
	jae	.LBB0_408
# %bb.410:                              #   in Loop: Header=BB0_409 Depth=1
	movq	-56(%rbp), %rdi
	movl	%esi, -44(%rbp)
	movb	%cl, (%rdi,%rdx)
	movq	-56(%rbp), %rcx
	movl	-44(%rbp), %edx
	movb	$0, (%rcx,%rdx)
	jmp	.LBB0_408
.LBB0_415:
	cmpl	$239, %r12d
	je	.LBB0_476
# %bb.416:
	cmpl	$84, %r12d
	je	.LBB0_476
# %bb.417:
	cmpl	$42, %r12d
	je	.LBB0_476
# %bb.418:
	cmpl	$16, %ebx
	je	.LBB0_476
# %bb.419:
	cmpl	$40, %ebx
	je	.LBB0_476
# %bb.420:
	cmpl	$214, %r12d
	je	.LBB0_476
# %bb.421:
	movl	%r12d, %edi
	andl	$-18, %edi
	cmpl	$110, %edi
	je	.LBB0_476
# %bb.422:
	movl	%r12d, %edi
	andl	$-4, %edi
	cmpl	$92, %edi
	je	.LBB0_476
# %bb.423:
	leal	-87(%r12), %r8d
	cmpl	$4, %r8d
	jb	.LBB0_476
# %bb.424:
	cmpl	$81, %r12d
	je	.LBB0_476
# %bb.425:
	cmpl	$44, %edi
	je	.LBB0_476
# %bb.426:
	movb	$40, %r8b
	movl	$1, %edx
	leaq	.L.str.48(%rip), %rdi
	movl	-60(%rbp), %r15d                # 4-byte Reload
	jmp	.LBB0_428
.LBB0_427:                              #   in Loop: Header=BB0_428 Depth=1
	movzbl	(%rdx,%rdi), %r8d
	incq	%rdx
	xorl	%ebx, %ebx
	movl	$0, %esi
	cmpq	$6, %rdx
	je	.LBB0_328
.LBB0_428:                              # =>This Inner Loop Header: Depth=1
	movl	-44(%rbp), %ecx
	leal	1(%rcx), %esi
	cmpl	-48(%rbp), %esi
	jae	.LBB0_427
# %bb.429:                              #   in Loop: Header=BB0_428 Depth=1
	movq	-56(%rbp), %r9
	movl	%esi, -44(%rbp)
	movb	%r8b, (%r9,%rcx)
	movq	-56(%rbp), %rcx
	movl	-44(%rbp), %esi
	movb	$0, (%rcx,%rsi)
	jmp	.LBB0_427
.LBB0_430:
	movl	-172(%rbp), %edi
	movl	-64(%rbp), %esi                 # 4-byte Reload
	movl	-68(%rbp), %edx                 # 4-byte Reload
	callq	reg_name
	movzbl	(%rax), %ecx
	testb	%cl, %cl
	je	.LBB0_435
# %bb.431:
	incq	%rax
	jmp	.LBB0_433
.LBB0_432:                              #   in Loop: Header=BB0_433 Depth=1
	movzbl	(%rax), %ecx
	incq	%rax
	testb	%cl, %cl
	je	.LBB0_435
.LBB0_433:                              # =>This Inner Loop Header: Depth=1
	movl	-44(%rbp), %edx
	leal	1(%rdx), %esi
	cmpl	-48(%rbp), %esi
	jae	.LBB0_432
# %bb.434:                              #   in Loop: Header=BB0_433 Depth=1
	movq	-56(%rbp), %rdi
	movl	%esi, -44(%rbp)
	movb	%cl, (%rdi,%rdx)
	movq	-56(%rbp), %rcx
	movl	-44(%rbp), %edx
	movb	$0, (%rcx,%rdx)
	jmp	.LBB0_432
.LBB0_435:
	movb	$44, %dl
	xorl	%eax, %eax
	leaq	.L.str.10(%rip), %rcx
	jmp	.LBB0_437
.LBB0_436:                              #   in Loop: Header=BB0_437 Depth=1
	movzbl	1(%rax,%rcx), %edx
	incq	%rax
	cmpq	$2, %rax
	je	.LBB0_439
.LBB0_437:                              # =>This Inner Loop Header: Depth=1
	movl	-44(%rbp), %esi
	leal	1(%rsi), %edi
	cmpl	-48(%rbp), %edi
	jae	.LBB0_436
# %bb.438:                              #   in Loop: Header=BB0_437 Depth=1
	movq	-56(%rbp), %r8
	movl	%edi, -44(%rbp)
	movb	%dl, (%r8,%rsi)
	movq	-56(%rbp), %rdx
	movl	-44(%rbp), %esi
	movb	$0, (%rdx,%rsi)
	jmp	.LBB0_436
.LBB0_439:
	leaq	-56(%rbp), %rdi
	leaq	-176(%rbp), %rsi
	xorl	%ebx, %ebx
	movl	-64(%rbp), %edx                 # 4-byte Reload
	movl	-68(%rbp), %ecx                 # 4-byte Reload
	xorl	%r8d, %r8d
	jmp	.LBB0_290
.LBB0_440:
	andl	$7, %edx
	leaq	R8H(%rip), %rcx
.LBB0_441:
	leaq	(%rcx,%rdx,8), %rdx
.LBB0_442:
	movq	(%rdx), %rdx
	movzbl	(%rdx), %esi
	testb	%sil, %sil
	je	.LBB0_448
# %bb.443:
	incq	%rdx
	jmp	.LBB0_445
	.p2align	4
.LBB0_444:                              #   in Loop: Header=BB0_445 Depth=1
	movzbl	(%rdx), %esi
	incq	%rdx
	testb	%sil, %sil
	je	.LBB0_448
.LBB0_445:                              # =>This Inner Loop Header: Depth=1
	movl	-44(%rbp), %ecx
	leal	1(%rcx), %edi
	cmpl	-48(%rbp), %edi
	jae	.LBB0_444
# %bb.446:                              #   in Loop: Header=BB0_445 Depth=1
	movq	-56(%rbp), %r8
	movl	%edi, -44(%rbp)
	movb	%sil, (%r8,%rcx)
	movq	-56(%rbp), %rcx
	movl	-44(%rbp), %esi
	movb	$0, (%rcx,%rsi)
	jmp	.LBB0_444
.LBB0_448:
	movb	$44, %dil
	xorl	%edx, %edx
	leaq	.L.str.10(%rip), %rsi
	jmp	.LBB0_450
	.p2align	4
.LBB0_449:                              #   in Loop: Header=BB0_450 Depth=1
	movzbl	1(%rdx,%rsi), %edi
	incq	%rdx
	cmpq	$2, %rdx
	je	.LBB0_452
.LBB0_450:                              # =>This Inner Loop Header: Depth=1
	movl	-44(%rbp), %ecx
	leal	1(%rcx), %r8d
	cmpl	-48(%rbp), %r8d
	jae	.LBB0_449
# %bb.451:                              #   in Loop: Header=BB0_450 Depth=1
	movq	-56(%rbp), %r9
	movl	%r8d, -44(%rbp)
	movb	%dil, (%r9,%rcx)
	movq	-56(%rbp), %rcx
	movl	-44(%rbp), %edi
	movb	$0, (%rcx,%rdi)
	jmp	.LBB0_449
.LBB0_452:
	movl	-172(%rbp), %edx
	cmpl	$2, %ebx
	je	.LBB0_457
# %bb.453:
	cmpl	$4, %ebx
	je	.LBB0_456
# %bb.454:
	cmpl	$8, %ebx
	jne	.LBB0_458
# %bb.455:
	andl	$15, %edx
	leaq	R64(%rip), %rcx
	jmp	.LBB0_461
.LBB0_456:
	andl	$15, %edx
	leaq	R32(%rip), %rcx
	jmp	.LBB0_461
.LBB0_457:
	andl	$15, %edx
	leaq	R16(%rip), %rcx
	jmp	.LBB0_461
.LBB0_458:
	cmpl	$64, -108(%rbp)                 # 4-byte Folded Reload
	jne	.LBB0_460
# %bb.459:
	andl	$15, %edx
	leaq	R8L(%rip), %rcx
	jmp	.LBB0_461
.LBB0_460:
	andl	$7, %edx
	leaq	R8H(%rip), %rcx
.LBB0_461:
	leaq	(%rcx,%rdx,8), %rdx
	movq	(%rdx), %rdx
	movzbl	(%rdx), %esi
	testb	%sil, %sil
	je	.LBB0_474
# %bb.462:
	incq	%rdx
	jmp	.LBB0_464
	.p2align	4
.LBB0_463:                              #   in Loop: Header=BB0_464 Depth=1
	movzbl	(%rdx), %esi
	incq	%rdx
	testb	%sil, %sil
	je	.LBB0_474
.LBB0_464:                              # =>This Inner Loop Header: Depth=1
	movl	-44(%rbp), %ecx
	leal	1(%rcx), %edi
	cmpl	-48(%rbp), %edi
	jae	.LBB0_463
# %bb.465:                              #   in Loop: Header=BB0_464 Depth=1
	movq	-56(%rbp), %r8
	movl	%edi, -44(%rbp)
	movb	%sil, (%r8,%rcx)
	movq	-56(%rbp), %rcx
	movl	-44(%rbp), %esi
	movb	$0, (%rcx,%rsi)
	jmp	.LBB0_463
.LBB0_466:
	andl	$7, %edx
	leaq	R8H(%rip), %rcx
.LBB0_467:
	leaq	(%rcx,%rdx,8), %rdx
.LBB0_468:
	movq	(%rdx), %rdx
	movzbl	(%rdx), %esi
	testb	%sil, %sil
	je	.LBB0_474
# %bb.469:
	incq	%rdx
	jmp	.LBB0_471
	.p2align	4
.LBB0_470:                              #   in Loop: Header=BB0_471 Depth=1
	movzbl	(%rdx), %esi
	incq	%rdx
	testb	%sil, %sil
	je	.LBB0_474
.LBB0_471:                              # =>This Inner Loop Header: Depth=1
	movl	-44(%rbp), %ecx
	leal	1(%rcx), %edi
	cmpl	-48(%rbp), %edi
	jae	.LBB0_470
# %bb.472:                              #   in Loop: Header=BB0_471 Depth=1
	movq	-56(%rbp), %r8
	movl	%edi, -44(%rbp)
	movb	%sil, (%r8,%rcx)
	movq	-56(%rbp), %rcx
	movl	-44(%rbp), %esi
	movb	$0, (%rcx,%rsi)
	jmp	.LBB0_470
.LBB0_476:
	movq	%r11, -80(%rbp)                 # 8-byte Spill
	leaq	-104(%rbp), %rdi
	leaq	-176(%rbp), %r8
                                        # kill: def $ecx killed $ecx killed $rcx
	callq	parse_modrm
	cmpl	$16, %ebx
	jne	.LBB0_506
# %bb.477:
	testl	%r14d, %r14d
	leaq	.L.str.17(%rip), %rax
	leaq	.L.str.16(%rip), %rcx
	cmoveq	%rax, %rcx
	testl	%r15d, %r15d
	leaq	.L.str.15(%rip), %rax
.LBB0_478:
	cmoveq	%rcx, %rax
	jmp	.LBB0_508
.LBB0_479:
	cmpl	$132, %ebx
	movl	$1, %eax
	movl	-64(%rbp), %edx                 # 4-byte Reload
	cmovel	%eax, %edx
	leaq	-56(%rbp), %rdi
	leaq	-176(%rbp), %rsi
	xorl	%ebx, %ebx
	movl	%edx, %r15d
	movl	-68(%rbp), %ecx                 # 4-byte Reload
	xorl	%r8d, %r8d
	xorl	%r9d, %r9d
	callq	render_rm
	movb	$44, %cl
	leaq	.L.str.10(%rip), %rax
	jmp	.LBB0_481
.LBB0_480:                              #   in Loop: Header=BB0_481 Depth=1
	movzbl	1(%rbx,%rax), %ecx
	incq	%rbx
	cmpq	$2, %rbx
	je	.LBB0_483
.LBB0_481:                              # =>This Inner Loop Header: Depth=1
	movl	-44(%rbp), %edx
	leal	1(%rdx), %esi
	cmpl	-48(%rbp), %esi
	jae	.LBB0_480
# %bb.482:                              #   in Loop: Header=BB0_481 Depth=1
	movq	-56(%rbp), %rdi
	movl	%esi, -44(%rbp)
	movb	%cl, (%rdi,%rdx)
	movq	-56(%rbp), %rcx
	movl	-44(%rbp), %edx
	movb	$0, (%rcx,%rdx)
	jmp	.LBB0_480
.LBB0_483:
	movl	-172(%rbp), %edi
	movl	%r15d, %esi
	movl	-68(%rbp), %edx                 # 4-byte Reload
	callq	reg_name
	movzbl	(%rax), %esi
	testb	%sil, %sil
	movq	-80(%rbp), %r11                 # 8-byte Reload
	movl	%r14d, %r15d
	je	.LBB0_474
# %bb.484:
	movq	%rax, %rdx
	incq	%rdx
	jmp	.LBB0_486
.LBB0_485:                              #   in Loop: Header=BB0_486 Depth=1
	movzbl	(%rdx), %esi
	incq	%rdx
	testb	%sil, %sil
	je	.LBB0_474
.LBB0_486:                              # =>This Inner Loop Header: Depth=1
	movl	-44(%rbp), %ecx
	leal	1(%rcx), %edi
	cmpl	-48(%rbp), %edi
	jae	.LBB0_485
# %bb.487:                              #   in Loop: Header=BB0_486 Depth=1
	movq	-56(%rbp), %r8
	movl	%edi, -44(%rbp)
	movb	%sil, (%r8,%rcx)
	movq	-56(%rbp), %rcx
	movl	-44(%rbp), %esi
	movb	$0, (%rcx,%rsi)
	jmp	.LBB0_485
.LBB0_488:
	movl	-60(%rbp), %r14d                # 4-byte Reload
	leaq	-104(%rbp), %rdi
	leaq	-176(%rbp), %r8
                                        # kill: def $ecx killed $ecx killed $rcx
	callq	parse_modrm
	movb	$120, %dl
	movl	$1, %eax
	leaq	.L.str.54(%rip), %rcx
	jmp	.LBB0_490
.LBB0_489:                              #   in Loop: Header=BB0_490 Depth=1
	movzbl	(%rax,%rcx), %edx
	incq	%rax
	cmpq	$6, %rax
	je	.LBB0_492
.LBB0_490:                              # =>This Inner Loop Header: Depth=1
	movl	-44(%rbp), %esi
	leal	1(%rsi), %edi
	cmpl	-48(%rbp), %edi
	jae	.LBB0_489
# %bb.491:                              #   in Loop: Header=BB0_490 Depth=1
	movq	-56(%rbp), %r8
	movl	%edi, -44(%rbp)
	movb	%dl, (%r8,%rsi)
	movq	-56(%rbp), %rdx
	movl	-44(%rbp), %esi
	movb	$0, (%rdx,%rsi)
	jmp	.LBB0_489
.LBB0_492:
	cmpl	$134, %ebx
	movl	$1, %eax
	movl	-64(%rbp), %edx                 # 4-byte Reload
	cmovel	%eax, %edx
	leaq	-56(%rbp), %rdi
	leaq	-176(%rbp), %rsi
	xorl	%ebx, %ebx
	movl	%edx, %r15d
	movl	-68(%rbp), %ecx                 # 4-byte Reload
	xorl	%r8d, %r8d
	xorl	%r9d, %r9d
	callq	render_rm
	movb	$44, %cl
	leaq	.L.str.10(%rip), %rax
	jmp	.LBB0_494
.LBB0_493:                              #   in Loop: Header=BB0_494 Depth=1
	movzbl	1(%rbx,%rax), %ecx
	incq	%rbx
	cmpq	$2, %rbx
	je	.LBB0_496
.LBB0_494:                              # =>This Inner Loop Header: Depth=1
	movl	-44(%rbp), %edx
	leal	1(%rdx), %esi
	cmpl	-48(%rbp), %esi
	jae	.LBB0_493
# %bb.495:                              #   in Loop: Header=BB0_494 Depth=1
	movq	-56(%rbp), %rdi
	movl	%esi, -44(%rbp)
	movb	%cl, (%rdi,%rdx)
	movq	-56(%rbp), %rcx
	movl	-44(%rbp), %edx
	movb	$0, (%rcx,%rdx)
	jmp	.LBB0_493
.LBB0_496:
	movl	-172(%rbp), %edi
	movl	%r15d, %esi
	movl	-68(%rbp), %edx                 # 4-byte Reload
	callq	reg_name
	movzbl	(%rax), %esi
	testb	%sil, %sil
	movq	-80(%rbp), %r11                 # 8-byte Reload
	movl	%r14d, %r15d
	je	.LBB0_327
# %bb.497:
	movq	%rax, %rdx
	incq	%rdx
	jmp	.LBB0_499
.LBB0_498:                              #   in Loop: Header=BB0_499 Depth=1
	movzbl	(%rdx), %esi
	incq	%rdx
	testb	%sil, %sil
	je	.LBB0_327
.LBB0_499:                              # =>This Inner Loop Header: Depth=1
	movl	-44(%rbp), %ecx
	leal	1(%rcx), %edi
	cmpl	-48(%rbp), %edi
	jae	.LBB0_498
# %bb.500:                              #   in Loop: Header=BB0_499 Depth=1
	movq	-56(%rbp), %r8
	movl	%edi, -44(%rbp)
	movb	%sil, (%r8,%rcx)
	movq	-56(%rbp), %rcx
	movl	-44(%rbp), %esi
	movb	$0, (%rcx,%rsi)
	jmp	.LBB0_498
.LBB0_501:
	movl	%ebx, %r9d
	andl	$-4, %r9d
	cmpl	$136, %r9d
	jne	.LBB0_528
# %bb.502:
	movl	-60(%rbp), %r14d                # 4-byte Reload
	testb	$1, %bl
	movl	$1, %eax
	movl	-64(%rbp), %edi                 # 4-byte Reload
	cmovel	%eax, %edi
	movl	%edi, -64(%rbp)                 # 4-byte Spill
	leaq	-104(%rbp), %rdi
	leaq	-176(%rbp), %r8
                                        # kill: def $ecx killed $ecx killed $rcx
	callq	parse_modrm
	movb	$109, %dl
	xorl	%eax, %eax
	leaq	.L.str.55(%rip), %rcx
	jmp	.LBB0_504
.LBB0_503:                              #   in Loop: Header=BB0_504 Depth=1
	movzbl	1(%rax,%rcx), %edx
	incq	%rax
	cmpq	$4, %rax
	je	.LBB0_535
.LBB0_504:                              # =>This Inner Loop Header: Depth=1
	movl	-44(%rbp), %esi
	leal	1(%rsi), %edi
	cmpl	-48(%rbp), %edi
	jae	.LBB0_503
# %bb.505:                              #   in Loop: Header=BB0_504 Depth=1
	movq	-56(%rbp), %r8
	movl	%edi, -44(%rbp)
	movb	%dl, (%r8,%rsi)
	movq	-56(%rbp), %rdx
	movl	-44(%rbp), %esi
	movb	$0, (%rdx,%rsi)
	jmp	.LBB0_503
.LBB0_506:
	cmpl	$40, %ebx
	jne	.LBB0_540
# %bb.507:
	leaq	.L.str.18(%rip), %rax
.LBB0_508:
	movzbl	(%rax), %ecx
	testb	%cl, %cl
	movl	-64(%rbp), %edx                 # 4-byte Reload
	je	.LBB0_513
# %bb.509:
	incq	%rax
	jmp	.LBB0_511
.LBB0_510:                              #   in Loop: Header=BB0_511 Depth=1
	movzbl	(%rax), %ecx
	incq	%rax
	testb	%cl, %cl
	je	.LBB0_513
.LBB0_511:                              # =>This Inner Loop Header: Depth=1
	movl	-44(%rbp), %r8d
	leal	1(%r8), %esi
	cmpl	-48(%rbp), %esi
	jae	.LBB0_510
# %bb.512:                              #   in Loop: Header=BB0_511 Depth=1
	movq	-56(%rbp), %rdi
	movl	%esi, -44(%rbp)
	movb	%cl, (%rdi,%r8)
	movq	-56(%rbp), %rcx
	movl	-44(%rbp), %esi
	movb	$0, (%rcx,%rsi)
	jmp	.LBB0_510
.LBB0_513:
	movl	-44(%rbp), %eax
	leal	1(%rax), %ecx
	cmpl	-48(%rbp), %ecx
	jae	.LBB0_515
# %bb.514:
	movq	-56(%rbp), %rsi
	movl	%ecx, -44(%rbp)
	movb	$32, (%rsi,%rax)
	movq	-56(%rbp), %rax
	movl	-44(%rbp), %ecx
	movb	$0, (%rax,%rcx)
.LBB0_515:
	movl	-172(%rbp), %eax
	andl	$15, %eax
	leaq	XMM(%rip), %rcx
	movq	(%rcx,%rax,8), %rax
	movzbl	(%rax), %ecx
	testb	%cl, %cl
	je	.LBB0_520
# %bb.516:
	incq	%rax
	jmp	.LBB0_518
.LBB0_517:                              #   in Loop: Header=BB0_518 Depth=1
	movzbl	(%rax), %ecx
	incq	%rax
	testb	%cl, %cl
	je	.LBB0_520
.LBB0_518:                              # =>This Inner Loop Header: Depth=1
	movl	-44(%rbp), %r8d
	leal	1(%r8), %esi
	cmpl	-48(%rbp), %esi
	jae	.LBB0_517
# %bb.519:                              #   in Loop: Header=BB0_518 Depth=1
	movq	-56(%rbp), %rdi
	movl	%esi, -44(%rbp)
	movb	%cl, (%rdi,%r8)
	movq	-56(%rbp), %rcx
	movl	-44(%rbp), %esi
	movb	$0, (%rcx,%rsi)
	jmp	.LBB0_517
.LBB0_520:
	movb	$44, %r9b
	xorl	%eax, %eax
	leaq	.L.str.10(%rip), %rcx
	jmp	.LBB0_522
.LBB0_521:                              #   in Loop: Header=BB0_522 Depth=1
	movzbl	1(%rax,%rcx), %r9d
	incq	%rax
	cmpq	$2, %rax
	je	.LBB0_524
.LBB0_522:                              # =>This Inner Loop Header: Depth=1
	movl	-44(%rbp), %esi
	leal	1(%rsi), %edi
	cmpl	-48(%rbp), %edi
	jae	.LBB0_521
# %bb.523:                              #   in Loop: Header=BB0_522 Depth=1
	movq	-56(%rbp), %r8
	movl	%edi, -44(%rbp)
	movb	%r9b, (%r8,%rsi)
	movq	-56(%rbp), %rdi
	movl	-44(%rbp), %esi
	movb	$0, (%rdi,%rsi)
	jmp	.LBB0_521
.LBB0_524:
	leaq	-56(%rbp), %rdi
	leaq	-176(%rbp), %rsi
	xorl	%ebx, %ebx
	movl	-68(%rbp), %ecx                 # 4-byte Reload
	xorl	%r8d, %r8d
	movl	$1, %r9d
.LBB0_525:
	callq	render_rm
	cmpl	$0, -164(%rbp)
	setne	%sil
	je	.LBB0_527
	jmp	.LBB0_526
.LBB0_528:
	cmpl	$144, %ebx
	je	.LBB0_567
# %bb.529:
	cmpl	$143, %ebx
	je	.LBB0_562
# %bb.530:
	cmpl	$141, %ebx
	jne	.LBB0_572
# %bb.531:
	leaq	-104(%rbp), %rdi
	leaq	-176(%rbp), %r8
                                        # kill: def $ecx killed $ecx killed $rcx
	callq	parse_modrm
	movb	$108, %dl
	xorl	%eax, %eax
	leaq	.L.str.56(%rip), %rcx
	jmp	.LBB0_533
.LBB0_532:                              #   in Loop: Header=BB0_533 Depth=1
	movzbl	1(%rax,%rcx), %edx
	incq	%rax
	cmpq	$4, %rax
	je	.LBB0_577
.LBB0_533:                              # =>This Inner Loop Header: Depth=1
	movl	-44(%rbp), %esi
	leal	1(%rsi), %edi
	cmpl	-48(%rbp), %edi
	jae	.LBB0_532
# %bb.534:                              #   in Loop: Header=BB0_533 Depth=1
	movq	-56(%rbp), %r8
	movl	%edi, -44(%rbp)
	movb	%dl, (%r8,%rsi)
	movq	-56(%rbp), %rdx
	movl	-44(%rbp), %esi
	movb	$0, (%rdx,%rsi)
	jmp	.LBB0_532
.LBB0_535:
	testb	$2, %bl
	jne	.LBB0_542
# %bb.536:
	leaq	-56(%rbp), %rdi
	leaq	-176(%rbp), %rsi
	xorl	%ebx, %ebx
	movl	-64(%rbp), %edx                 # 4-byte Reload
	movl	-68(%rbp), %ecx                 # 4-byte Reload
	xorl	%r8d, %r8d
	xorl	%r9d, %r9d
	callq	render_rm
	movb	$44, %cl
	leaq	.L.str.10(%rip), %rax
	jmp	.LBB0_538
.LBB0_537:                              #   in Loop: Header=BB0_538 Depth=1
	movzbl	1(%rbx,%rax), %ecx
	incq	%rbx
	cmpq	$2, %rbx
	je	.LBB0_547
.LBB0_538:                              # =>This Inner Loop Header: Depth=1
	movl	-44(%rbp), %edx
	leal	1(%rdx), %esi
	cmpl	-48(%rbp), %esi
	jae	.LBB0_537
# %bb.539:                              #   in Loop: Header=BB0_538 Depth=1
	movq	-56(%rbp), %rdi
	movl	%esi, -44(%rbp)
	movb	%cl, (%rdi,%rdx)
	movq	-56(%rbp), %rcx
	movl	-44(%rbp), %edx
	movb	$0, (%rcx,%rdx)
	jmp	.LBB0_537
.LBB0_540:
	cmpl	$42, %r12d
	jne	.LBB0_558
# %bb.541:
	testl	%r14d, %r14d
	leaq	.L.str.21(%rip), %rax
	leaq	.L.str.20(%rip), %rcx
	cmoveq	%rax, %rcx
	testl	%r15d, %r15d
	leaq	.L.str.19(%rip), %rax
	jmp	.LBB0_478
.LBB0_542:
	movl	-172(%rbp), %edi
	movl	-64(%rbp), %esi                 # 4-byte Reload
	movl	-68(%rbp), %edx                 # 4-byte Reload
	callq	reg_name
	movzbl	(%rax), %ecx
	testb	%cl, %cl
	je	.LBB0_552
# %bb.543:
	incq	%rax
	jmp	.LBB0_545
.LBB0_544:                              #   in Loop: Header=BB0_545 Depth=1
	movzbl	(%rax), %ecx
	incq	%rax
	testb	%cl, %cl
	je	.LBB0_552
.LBB0_545:                              # =>This Inner Loop Header: Depth=1
	movl	-44(%rbp), %edx
	leal	1(%rdx), %esi
	cmpl	-48(%rbp), %esi
	jae	.LBB0_544
# %bb.546:                              #   in Loop: Header=BB0_545 Depth=1
	movq	-56(%rbp), %rdi
	movl	%esi, -44(%rbp)
	movb	%cl, (%rdi,%rdx)
	movq	-56(%rbp), %rcx
	movl	-44(%rbp), %edx
	movb	$0, (%rcx,%rdx)
	jmp	.LBB0_544
.LBB0_547:
	movl	-172(%rbp), %edi
	movl	-64(%rbp), %esi                 # 4-byte Reload
	movl	-68(%rbp), %edx                 # 4-byte Reload
	callq	reg_name
	movzbl	(%rax), %esi
	testb	%sil, %sil
	movq	-80(%rbp), %r11                 # 8-byte Reload
	movl	%r14d, %r15d
	je	.LBB0_474
# %bb.548:
	movq	%rax, %rdx
	incq	%rdx
	jmp	.LBB0_550
.LBB0_549:                              #   in Loop: Header=BB0_550 Depth=1
	movzbl	(%rdx), %esi
	incq	%rdx
	testb	%sil, %sil
	je	.LBB0_474
.LBB0_550:                              # =>This Inner Loop Header: Depth=1
	movl	-44(%rbp), %ecx
	leal	1(%rcx), %edi
	cmpl	-48(%rbp), %edi
	jae	.LBB0_549
# %bb.551:                              #   in Loop: Header=BB0_550 Depth=1
	movq	-56(%rbp), %r8
	movl	%edi, -44(%rbp)
	movb	%sil, (%r8,%rcx)
	movq	-56(%rbp), %rcx
	movl	-44(%rbp), %esi
	movb	$0, (%rcx,%rsi)
	jmp	.LBB0_549
.LBB0_552:
	movb	$44, %dl
	xorl	%eax, %eax
	leaq	.L.str.10(%rip), %rcx
	jmp	.LBB0_554
.LBB0_553:                              #   in Loop: Header=BB0_554 Depth=1
	movzbl	1(%rax,%rcx), %edx
	incq	%rax
	cmpq	$2, %rax
	je	.LBB0_556
.LBB0_554:                              # =>This Inner Loop Header: Depth=1
	movl	-44(%rbp), %esi
	leal	1(%rsi), %edi
	cmpl	-48(%rbp), %edi
	jae	.LBB0_553
# %bb.555:                              #   in Loop: Header=BB0_554 Depth=1
	movq	-56(%rbp), %r8
	movl	%edi, -44(%rbp)
	movb	%dl, (%r8,%rsi)
	movq	-56(%rbp), %rdx
	movl	-44(%rbp), %esi
	movb	$0, (%rdx,%rsi)
	jmp	.LBB0_553
.LBB0_556:
	leaq	-56(%rbp), %rdi
	leaq	-176(%rbp), %rsi
	movl	-64(%rbp), %edx                 # 4-byte Reload
	movl	-68(%rbp), %ecx                 # 4-byte Reload
	xorl	%r8d, %r8d
	xorl	%r9d, %r9d
	callq	render_rm
	movq	-80(%rbp), %r11                 # 8-byte Reload
	movl	%r14d, %r15d
.LBB0_474:
	cmpl	$0, -164(%rbp)
	setne	%sil
	je	.LBB0_557
# %bb.475:
	movq	-136(%rbp), %rbx
	jmp	.LBB0_328
.LBB0_557:
	xorl	%ebx, %ebx
	jmp	.LBB0_328
.LBB0_558:
	cmpl	$46, %r12d
	jg	.LBB0_587
# %bb.559:
	cmpl	$44, %r12d
	je	.LBB0_603
# %bb.560:
	cmpl	$46, %r12d
	jne	.LBB0_601
# %bb.561:
	leaq	.L.str.24(%rip), %rax
	jmp	.LBB0_508
.LBB0_562:
	movl	-60(%rbp), %r14d                # 4-byte Reload
	leaq	-104(%rbp), %rdi
	leaq	-176(%rbp), %r8
                                        # kill: def $ecx killed $ecx killed $rcx
	callq	parse_modrm
	movb	$112, %dl
	xorl	%eax, %eax
	leaq	.L.str.51(%rip), %rcx
	jmp	.LBB0_564
.LBB0_563:                              #   in Loop: Header=BB0_564 Depth=1
	movzbl	1(%rax,%rcx), %edx
	incq	%rax
	cmpq	$4, %rax
	je	.LBB0_566
.LBB0_564:                              # =>This Inner Loop Header: Depth=1
	movl	-44(%rbp), %esi
	leal	1(%rsi), %edi
	cmpl	-48(%rbp), %edi
	jae	.LBB0_563
# %bb.565:                              #   in Loop: Header=BB0_564 Depth=1
	movq	-56(%rbp), %r8
	movl	%edi, -44(%rbp)
	movb	%dl, (%r8,%rsi)
	movq	-56(%rbp), %rdx
	movl	-44(%rbp), %esi
	movb	$0, (%rdx,%rsi)
	jmp	.LBB0_563
.LBB0_566:
	xorl	%r8d, %r8d
	cmpl	$0, -176(%rbp)
	sete	%r8b
	shll	$3, %r8d
	leaq	-56(%rbp), %rdi
	leaq	-176(%rbp), %rsi
	xorl	%ebx, %ebx
	movl	$8, %edx
	movl	-68(%rbp), %ecx                 # 4-byte Reload
	xorl	%r9d, %r9d
	callq	render_rm
	xorl	%esi, %esi
	jmp	.LBB0_279
.LBB0_567:
	testl	%r15d, %r15d
	leaq	.L.str.3(%rip), %rcx
	leaq	.L.str.57(%rip), %rdx
	cmoveq	%rcx, %rdx
	movzbl	(%rdx), %edi
	testb	%dil, %dil
	je	.LBB0_755
# %bb.568:
	incq	%rdx
	movq	-80(%rbp), %r11                 # 8-byte Reload
	movl	-60(%rbp), %r15d                # 4-byte Reload
	jmp	.LBB0_570
.LBB0_569:                              #   in Loop: Header=BB0_570 Depth=1
	movzbl	(%rdx), %edi
	incq	%rdx
	xorl	%ebx, %ebx
	movl	$0, %esi
	testb	%dil, %dil
	je	.LBB0_328
.LBB0_570:                              # =>This Inner Loop Header: Depth=1
	movl	-44(%rbp), %ecx
	leal	1(%rcx), %esi
	cmpl	-48(%rbp), %esi
	jae	.LBB0_569
# %bb.571:                              #   in Loop: Header=BB0_570 Depth=1
	movq	-56(%rbp), %r8
	movl	%esi, -44(%rbp)
	movb	%dil, (%r8,%rcx)
	movq	-56(%rbp), %rcx
	movl	-44(%rbp), %esi
	movb	$0, (%rcx,%rsi)
	jmp	.LBB0_569
.LBB0_572:
	leal	-145(%rbx), %eax
	cmpl	$6, %eax
	ja	.LBB0_590
# %bb.573:
	movb	$120, %dl
	movl	$1, %eax
	leaq	.L.str.54(%rip), %rsi
	jmp	.LBB0_575
.LBB0_574:                              #   in Loop: Header=BB0_575 Depth=1
	movzbl	(%rax,%rsi), %edx
	incq	%rax
	cmpq	$6, %rax
	je	.LBB0_597
.LBB0_575:                              # =>This Inner Loop Header: Depth=1
	movl	-44(%rbp), %edi
	leal	1(%rdi), %r8d
	cmpl	-48(%rbp), %r8d
	jae	.LBB0_574
# %bb.576:                              #   in Loop: Header=BB0_575 Depth=1
	movq	-56(%rbp), %r9
	movl	%r8d, -44(%rbp)
	movb	%dl, (%r9,%rdi)
	movq	-56(%rbp), %rdx
	movl	-44(%rbp), %edi
	movb	$0, (%rdx,%rdi)
	jmp	.LBB0_574
.LBB0_577:
	movl	-172(%rbp), %edi
	movl	-64(%rbp), %esi                 # 4-byte Reload
	movl	-68(%rbp), %edx                 # 4-byte Reload
	callq	reg_name
	movzbl	(%rax), %ecx
	testb	%cl, %cl
	je	.LBB0_582
# %bb.578:
	incq	%rax
	jmp	.LBB0_580
.LBB0_579:                              #   in Loop: Header=BB0_580 Depth=1
	movzbl	(%rax), %ecx
	incq	%rax
	testb	%cl, %cl
	je	.LBB0_582
.LBB0_580:                              # =>This Inner Loop Header: Depth=1
	movl	-44(%rbp), %edx
	leal	1(%rdx), %esi
	cmpl	-48(%rbp), %esi
	jae	.LBB0_579
# %bb.581:                              #   in Loop: Header=BB0_580 Depth=1
	movq	-56(%rbp), %rdi
	movl	%esi, -44(%rbp)
	movb	%cl, (%rdi,%rdx)
	movq	-56(%rbp), %rcx
	movl	-44(%rbp), %edx
	movb	$0, (%rcx,%rdx)
	jmp	.LBB0_579
.LBB0_582:
	movb	$44, %dl
	xorl	%eax, %eax
	leaq	.L.str.10(%rip), %rcx
	jmp	.LBB0_584
.LBB0_583:                              #   in Loop: Header=BB0_584 Depth=1
	movzbl	1(%rax,%rcx), %edx
	incq	%rax
	cmpq	$2, %rax
	je	.LBB0_586
.LBB0_584:                              # =>This Inner Loop Header: Depth=1
	movl	-44(%rbp), %esi
	leal	1(%rsi), %edi
	cmpl	-48(%rbp), %edi
	jae	.LBB0_583
# %bb.585:                              #   in Loop: Header=BB0_584 Depth=1
	movq	-56(%rbp), %r8
	movl	%edi, -44(%rbp)
	movb	%dl, (%r8,%rsi)
	movq	-56(%rbp), %rdx
	movl	-44(%rbp), %esi
	movb	$0, (%rdx,%rsi)
	jmp	.LBB0_583
.LBB0_586:
	leaq	-56(%rbp), %rdi
	leaq	-176(%rbp), %rsi
	callq	render_mem
	jmp	.LBB0_413
.LBB0_587:
	cmpl	$47, %r12d
	je	.LBB0_604
# %bb.588:
	cmpl	$87, %r12d
	jne	.LBB0_601
# %bb.589:
	leaq	.L.str.26(%rip), %rax
	jmp	.LBB0_508
.LBB0_590:
	cmpl	$153, %ebx
	je	.LBB0_605
# %bb.591:
	cmpl	$152, %ebx
	jne	.LBB0_610
# %bb.592:
	testl	%r8d, %r8d
	leaq	.L.str.60(%rip), %rax
	leaq	.L.str.59(%rip), %rcx
	cmoveq	%rax, %rcx
	leaq	.L.str.58(%rip), %rdx
	testb	%dil, %dil
	cmovneq	%rcx, %rdx
	movzbl	(%rdx), %edi
	testb	%dil, %dil
	je	.LBB0_755
# %bb.593:
	incq	%rdx
	movq	-80(%rbp), %r11                 # 8-byte Reload
	movl	-60(%rbp), %r15d                # 4-byte Reload
	jmp	.LBB0_595
.LBB0_594:                              #   in Loop: Header=BB0_595 Depth=1
	movzbl	(%rdx), %edi
	incq	%rdx
	xorl	%ebx, %ebx
	movl	$0, %esi
	testb	%dil, %dil
	je	.LBB0_328
.LBB0_595:                              # =>This Inner Loop Header: Depth=1
	movl	-44(%rbp), %ecx
	leal	1(%rcx), %esi
	cmpl	-48(%rbp), %esi
	jae	.LBB0_594
# %bb.596:                              #   in Loop: Header=BB0_595 Depth=1
	movq	-56(%rbp), %r8
	movl	%esi, -44(%rbp)
	movb	%dil, (%r8,%rcx)
	movq	-56(%rbp), %rcx
	movl	-44(%rbp), %esi
	movb	$0, (%rcx,%rsi)
	jmp	.LBB0_594
.LBB0_597:
	cmpl	$2, -64(%rbp)                   # 4-byte Folded Reload
	je	.LBB0_622
# %bb.598:
	cmpl	$8, -64(%rbp)                   # 4-byte Folded Reload
	je	.LBB0_621
# %bb.599:
	cmpl	$4, -64(%rbp)                   # 4-byte Folded Reload
	jne	.LBB0_623
# %bb.600:
	leaq	R32(%rip), %rax
	jmp	.LBB0_624
.LBB0_601:
	cmpl	$84, %r12d
	jne	.LBB0_616
# %bb.602:
	leaq	.L.str.27(%rip), %rax
	jmp	.LBB0_508
.LBB0_603:
	testl	%r15d, %r15d
	leaq	.L.str.22(%rip), %rcx
	leaq	.L.str.23(%rip), %rax
	cmovneq	%rcx, %rax
	jmp	.LBB0_508
.LBB0_604:
	leaq	.L.str.25(%rip), %rax
	jmp	.LBB0_508
.LBB0_605:
	testl	%r8d, %r8d
	leaq	.L.str.63(%rip), %rax
	leaq	.L.str.62(%rip), %rcx
	cmoveq	%rax, %rcx
	leaq	.L.str.61(%rip), %rdx
	testb	%dil, %dil
	cmovneq	%rcx, %rdx
	movzbl	(%rdx), %edi
	testb	%dil, %dil
	je	.LBB0_755
# %bb.606:
	incq	%rdx
	movq	-80(%rbp), %r11                 # 8-byte Reload
	movl	-60(%rbp), %r15d                # 4-byte Reload
	jmp	.LBB0_608
.LBB0_607:                              #   in Loop: Header=BB0_608 Depth=1
	movzbl	(%rdx), %edi
	incq	%rdx
	xorl	%ebx, %ebx
	movl	$0, %esi
	testb	%dil, %dil
	je	.LBB0_328
.LBB0_608:                              # =>This Inner Loop Header: Depth=1
	movl	-44(%rbp), %ecx
	leal	1(%rcx), %esi
	cmpl	-48(%rbp), %esi
	jae	.LBB0_607
# %bb.609:                              #   in Loop: Header=BB0_608 Depth=1
	movq	-56(%rbp), %r8
	movl	%esi, -44(%rbp)
	movb	%dil, (%r8,%rcx)
	movq	-56(%rbp), %rcx
	movl	-44(%rbp), %esi
	movb	$0, (%rcx,%rsi)
	jmp	.LBB0_607
.LBB0_610:
	leal	-170(%rbx), %eax
	cmpl	$6, %eax
	jae	.LBB0_638
.LBB0_611:
	testl	%r15d, %r15d
	je	.LBB0_649
# %bb.612:
	movb	$114, %dl
	xorl	%eax, %eax
	leaq	.L.str.64(%rip), %rcx
	jmp	.LBB0_614
.LBB0_613:                              #   in Loop: Header=BB0_614 Depth=1
	movzbl	1(%rax,%rcx), %edx
	incq	%rax
	cmpq	$4, %rax
	je	.LBB0_654
.LBB0_614:                              # =>This Inner Loop Header: Depth=1
	movl	-44(%rbp), %esi
	leal	1(%rsi), %r8d
	cmpl	-48(%rbp), %r8d
	jae	.LBB0_613
# %bb.615:                              #   in Loop: Header=BB0_614 Depth=1
	movq	-56(%rbp), %r9
	movl	%r8d, -44(%rbp)
	movb	%dl, (%r9,%rsi)
	movq	-56(%rbp), %rdx
	movl	-44(%rbp), %esi
	movb	$0, (%rdx,%rsi)
	jmp	.LBB0_613
.LBB0_616:
	cmpl	$91, %r12d
	jle	.LBB0_645
# %bb.617:
	cmpl	$109, %r12d
	jg	.LBB0_658
# %bb.618:
	cmpl	$92, %r12d
	je	.LBB0_680
# %bb.619:
	cmpl	$94, %r12d
	jne	.LBB0_683
# %bb.620:
	testl	%r14d, %r14d
	leaq	.L.str.39(%rip), %rax
	leaq	.L.str.38(%rip), %rcx
	cmoveq	%rax, %rcx
	testl	%r15d, %r15d
	leaq	.L.str.37(%rip), %rax
	jmp	.LBB0_478
.LBB0_621:
	leaq	R64(%rip), %rax
	jmp	.LBB0_624
.LBB0_622:
	leaq	R16(%rip), %rax
	jmp	.LBB0_624
.LBB0_623:
	cmpl	$64, -108(%rbp)                 # 4-byte Folded Reload
	leaq	R8L(%rip), %rdx
	leaq	R8H(%rip), %rax
	cmoveq	%rdx, %rax
.LBB0_624:
	movq	(%rax), %rax
	movzbl	(%rax), %edx
	testb	%dl, %dl
	je	.LBB0_629
# %bb.625:
	incq	%rax
	jmp	.LBB0_627
.LBB0_626:                              #   in Loop: Header=BB0_627 Depth=1
	movzbl	(%rax), %edx
	incq	%rax
	testb	%dl, %dl
	je	.LBB0_629
.LBB0_627:                              # =>This Inner Loop Header: Depth=1
	movl	-44(%rbp), %esi
	leal	1(%rsi), %edi
	cmpl	-48(%rbp), %edi
	jae	.LBB0_626
# %bb.628:                              #   in Loop: Header=BB0_627 Depth=1
	movq	-56(%rbp), %r8
	movl	%edi, -44(%rbp)
	movb	%dl, (%r8,%rsi)
	movq	-56(%rbp), %rdx
	movl	-44(%rbp), %esi
	movb	$0, (%rdx,%rsi)
	jmp	.LBB0_626
.LBB0_629:
	movb	$44, %dl
	xorl	%eax, %eax
	leaq	.L.str.10(%rip), %rsi
	jmp	.LBB0_631
.LBB0_630:                              #   in Loop: Header=BB0_631 Depth=1
	movzbl	1(%rax,%rsi), %edx
	incq	%rax
	cmpq	$2, %rax
	je	.LBB0_633
.LBB0_631:                              # =>This Inner Loop Header: Depth=1
	movl	-44(%rbp), %edi
	leal	1(%rdi), %r8d
	cmpl	-48(%rbp), %r8d
	jae	.LBB0_630
# %bb.632:                              #   in Loop: Header=BB0_631 Depth=1
	movq	-56(%rbp), %r9
	movl	%r8d, -44(%rbp)
	movb	%dl, (%r9,%rdi)
	movq	-56(%rbp), %rdx
	movl	-44(%rbp), %edi
	movb	$0, (%rdx,%rdi)
	jmp	.LBB0_630
.LBB0_633:
	leal	(%rbx,%rcx,8), %edi
	addl	$-144, %edi
	movl	-64(%rbp), %esi                 # 4-byte Reload
	movl	-68(%rbp), %edx                 # 4-byte Reload
	callq	reg_name
	movzbl	(%rax), %edi
	testb	%dil, %dil
	je	.LBB0_755
# %bb.634:
	movq	%rax, %rdx
	incq	%rdx
	movq	-80(%rbp), %r11                 # 8-byte Reload
	movl	-60(%rbp), %r15d                # 4-byte Reload
	jmp	.LBB0_636
.LBB0_635:                              #   in Loop: Header=BB0_636 Depth=1
	movzbl	(%rdx), %edi
	incq	%rdx
	xorl	%ebx, %ebx
	movl	$0, %esi
	testb	%dil, %dil
	je	.LBB0_328
.LBB0_636:                              # =>This Inner Loop Header: Depth=1
	movl	-44(%rbp), %ecx
	leal	1(%rcx), %esi
	cmpl	-48(%rbp), %esi
	jae	.LBB0_635
# %bb.637:                              #   in Loop: Header=BB0_636 Depth=1
	movq	-56(%rbp), %r8
	movl	%esi, -44(%rbp)
	movb	%dil, (%r8,%rcx)
	movq	-56(%rbp), %rcx
	movl	-44(%rbp), %esi
	movb	$0, (%rcx,%rsi)
	jmp	.LBB0_635
.LBB0_638:
	cmpl	$164, %r12d
	je	.LBB0_611
# %bb.639:
	cmpl	$168, %r12d
	je	.LBB0_671
# %bb.640:
	cmpl	$166, %r12d
	je	.LBB0_611
# %bb.641:
	cmpl	$184, %r11d
	je	.LBB0_686
# %bb.642:
	cmpl	$176, %r11d
	jne	.LBB0_691
# %bb.643:
	movl	-92(%rbp), %eax
	cmpl	-96(%rbp), %eax
	jae	.LBB0_715
# %bb.644:
	movq	-104(%rbp), %rdx
	leal	1(%rax), %esi
	movl	%esi, -92(%rbp)
	movzbl	(%rdx,%rax), %esi
	jmp	.LBB0_716
.LBB0_645:
	cmpl	$88, %r12d
	je	.LBB0_679
# %bb.646:
	cmpl	$89, %r12d
	je	.LBB0_681
# %bb.647:
	cmpl	$90, %r12d
	jne	.LBB0_683
# %bb.648:
	testl	%r14d, %r14d
	leaq	.L.str.42(%rip), %rax
	leaq	.L.str.41(%rip), %rcx
	cmoveq	%rax, %rcx
	testl	%r15d, %r15d
	leaq	.L.str.40(%rip), %rax
	jmp	.LBB0_478
.LBB0_649:
	testl	%r14d, %r14d
	je	.LBB0_654
# %bb.650:
	movb	$114, %dl
	movl	$1, %eax
	leaq	.L.str.65(%rip), %rcx
	jmp	.LBB0_652
.LBB0_651:                              #   in Loop: Header=BB0_652 Depth=1
	movzbl	(%rax,%rcx), %edx
	incq	%rax
	cmpq	$7, %rax
	je	.LBB0_654
.LBB0_652:                              # =>This Inner Loop Header: Depth=1
	movl	-44(%rbp), %esi
	leal	1(%rsi), %r8d
	cmpl	-48(%rbp), %r8d
	jae	.LBB0_651
# %bb.653:                              #   in Loop: Header=BB0_652 Depth=1
	movq	-56(%rbp), %r9
	movl	%r8d, -44(%rbp)
	movb	%dl, (%r9,%rsi)
	movq	-56(%rbp), %rdx
	movl	-44(%rbp), %esi
	movb	$0, (%rdx,%rsi)
	jmp	.LBB0_651
.LBB0_654:
	cmpl	$166, %r12d
	je	.LBB0_662
# %bb.655:
	cmpl	$170, %r12d
	je	.LBB0_661
# %bb.656:
	cmpl	$172, %r12d
	jne	.LBB0_663
# %bb.657:
	leaq	.L.str.69(%rip), %rax
	jmp	.LBB0_664
.LBB0_658:
	cmpl	$110, %r12d
	je	.LBB0_682
# %bb.659:
	cmpl	$126, %r12d
	jne	.LBB0_683
# %bb.660:
	testl	%r15d, %r15d
	leaq	.L.str.44(%rip), %rcx
	leaq	.L.str.43(%rip), %rax
	cmovneq	%rcx, %rax
	jmp	.LBB0_508
.LBB0_661:
	leaq	.L.str.68(%rip), %rax
	jmp	.LBB0_664
.LBB0_662:
	leaq	.L.str.67(%rip), %rax
	jmp	.LBB0_664
.LBB0_663:
	cmpl	$174, %r12d
	leaq	.L.str.70(%rip), %rcx
	leaq	.L.str.66(%rip), %rax
	cmoveq	%rcx, %rax
.LBB0_664:
	movzbl	(%rax), %ecx
	testb	%cl, %cl
	je	.LBB0_669
# %bb.665:
	incq	%rax
	jmp	.LBB0_667
.LBB0_666:                              #   in Loop: Header=BB0_667 Depth=1
	movzbl	(%rax), %ecx
	incq	%rax
	testb	%cl, %cl
	je	.LBB0_669
.LBB0_667:                              # =>This Inner Loop Header: Depth=1
	movl	-44(%rbp), %edx
	leal	1(%rdx), %esi
	cmpl	-48(%rbp), %esi
	jae	.LBB0_666
# %bb.668:                              #   in Loop: Header=BB0_667 Depth=1
	movq	-56(%rbp), %r8
	movl	%esi, -44(%rbp)
	movb	%cl, (%r8,%rdx)
	movq	-56(%rbp), %rcx
	movl	-44(%rbp), %edx
	movb	$0, (%rcx,%rdx)
	jmp	.LBB0_666
.LBB0_669:
	movl	-44(%rbp), %eax
	leal	1(%rax), %ecx
	cmpl	-48(%rbp), %ecx
	jae	.LBB0_755
# %bb.670:
	testb	%dil, %dil
	movl	$100, %edx
	movl	$113, %esi
	cmovnel	%edx, %esi
	testb	$1, %bl
	movl	$98, %edx
	cmovnel	%esi, %edx
	movq	-56(%rbp), %rsi
	movl	%ecx, -44(%rbp)
	movb	%dl, (%rsi,%rax)
	movq	-56(%rbp), %rax
	movl	-44(%rbp), %ecx
	movb	$0, (%rax,%rcx)
	jmp	.LBB0_755
.LBB0_671:
	xorl	%eax, %eax
	cmpl	$2, -64(%rbp)                   # 4-byte Folded Reload
	setne	%al
	cmpl	$168, %ebx
	leal	2(%rax,%rax), %eax
	movl	$1, %esi
	cmovnel	%eax, %esi
	leaq	-104(%rbp), %rdi
	callq	rd_imm_sext
	movb	$116, %sil
	movl	$1, %ecx
	leaq	.L.str.53(%rip), %rdx
	jmp	.LBB0_673
.LBB0_672:                              #   in Loop: Header=BB0_673 Depth=1
	movzbl	(%rcx,%rdx), %esi
	incq	%rcx
	cmpq	$6, %rcx
	je	.LBB0_675
.LBB0_673:                              # =>This Inner Loop Header: Depth=1
	movl	-44(%rbp), %edi
	leal	1(%rdi), %r8d
	cmpl	-48(%rbp), %r8d
	jae	.LBB0_672
# %bb.674:                              #   in Loop: Header=BB0_673 Depth=1
	movq	-56(%rbp), %r9
	movl	%r8d, -44(%rbp)
	movb	%sil, (%r9,%rdi)
	movq	-56(%rbp), %rsi
	movl	-44(%rbp), %edi
	movb	$0, (%rsi,%rdi)
	jmp	.LBB0_672
.LBB0_675:
	cmpl	$168, %ebx
	movl	$1, %ecx
	movl	-64(%rbp), %edx                 # 4-byte Reload
	cmovel	%ecx, %edx
	cmpl	$2, %edx
	je	.LBB0_699
# %bb.676:
	movl	%edx, %ecx
	cmpl	$8, %edx
	je	.LBB0_698
# %bb.677:
	cmpl	$4, %ecx
	jne	.LBB0_700
# %bb.678:
	leaq	R32(%rip), %rcx
	jmp	.LBB0_701
.LBB0_679:
	testl	%r14d, %r14d
	leaq	.L.str.30(%rip), %rax
	leaq	.L.str.29(%rip), %rcx
	cmoveq	%rax, %rcx
	testl	%r15d, %r15d
	leaq	.L.str.28(%rip), %rax
	jmp	.LBB0_478
.LBB0_680:
	testl	%r14d, %r14d
	leaq	.L.str.36(%rip), %rax
	leaq	.L.str.35(%rip), %rcx
	cmoveq	%rax, %rcx
	testl	%r15d, %r15d
	leaq	.L.str.34(%rip), %rax
	jmp	.LBB0_478
.LBB0_681:
	testl	%r14d, %r14d
	leaq	.L.str.33(%rip), %rax
	leaq	.L.str.32(%rip), %rcx
	cmoveq	%rax, %rcx
	testl	%r15d, %r15d
	leaq	.L.str.31(%rip), %rax
	jmp	.LBB0_478
.LBB0_682:
	leaq	.L.str.43(%rip), %rax
	jmp	.LBB0_508
.LBB0_683:
	movl	%r12d, %eax
	andl	$-17, %eax
	cmpl	$111, %eax
	jne	.LBB0_685
# %bb.684:
	testl	%r15d, %r15d
	leaq	.L.str.45(%rip), %rcx
	leaq	.L.str.46(%rip), %rax
	cmovneq	%rcx, %rax
	jmp	.LBB0_508
.LBB0_685:
	cmpl	$239, %r12d
	leaq	.L.str.47(%rip), %rcx
	leaq	.L.str.14(%rip), %rax
	jmp	.LBB0_478
.LBB0_686:
	leal	(%rbx,%rcx,8), %r14d
	addl	$-184, %r14d
	testb	%dil, %dil
	je	.LBB0_729
# %bb.687:
	xorl	%eax, %eax
	cmpl	$2, -64(%rbp)                   # 4-byte Folded Reload
	setne	%al
	leal	2(,%rax,2), %esi
	leaq	-104(%rbp), %rdi
	callq	rd_imm_sext
	movq	%rax, %rbx
	movb	$109, %dl
	xorl	%eax, %eax
	leaq	.L.str.55(%rip), %rcx
	jmp	.LBB0_689
.LBB0_688:                              #   in Loop: Header=BB0_689 Depth=1
	movzbl	1(%rax,%rcx), %edx
	incq	%rax
	cmpq	$4, %rax
	je	.LBB0_733
.LBB0_689:                              # =>This Inner Loop Header: Depth=1
	movl	-44(%rbp), %esi
	leal	1(%rsi), %edi
	cmpl	-48(%rbp), %edi
	jae	.LBB0_688
# %bb.690:                              #   in Loop: Header=BB0_689 Depth=1
	movq	-56(%rbp), %r8
	movl	%edi, -44(%rbp)
	movb	%dl, (%r8,%rsi)
	movq	-56(%rbp), %rdx
	movl	-44(%rbp), %esi
	movb	$0, (%rdx,%rsi)
	jmp	.LBB0_688
.LBB0_691:
	leal	-192(%r12), %eax
	cmpl	$18, %eax
	ja	.LBB0_761
# %bb.692:
	movl	$327681, %edi                   # imm = 0x50001
	btl	%eax, %edi
	jae	.LBB0_761
# %bb.693:
	testb	$1, %bl
	movl	$1, %eax
	movl	-64(%rbp), %edi                 # 4-byte Reload
	cmovel	%eax, %edi
	movl	%edi, -64(%rbp)                 # 4-byte Spill
	leaq	-104(%rbp), %rdi
	leaq	-176(%rbp), %r8
                                        # kill: def $ecx killed $ecx killed $rcx
	callq	parse_modrm
	movslq	-128(%rbp), %rax
	leaq	SHIFT(%rip), %rcx
	movq	(%rcx,%rax,8), %rax
	movzbl	(%rax), %ecx
	testb	%cl, %cl
	je	.LBB0_710
# %bb.694:
	incq	%rax
	jmp	.LBB0_696
.LBB0_695:                              #   in Loop: Header=BB0_696 Depth=1
	movzbl	(%rax), %ecx
	incq	%rax
	testb	%cl, %cl
	je	.LBB0_710
.LBB0_696:                              # =>This Inner Loop Header: Depth=1
	movl	-44(%rbp), %edx
	leal	1(%rdx), %esi
	cmpl	-48(%rbp), %esi
	jae	.LBB0_695
# %bb.697:                              #   in Loop: Header=BB0_696 Depth=1
	movq	-56(%rbp), %rdi
	movl	%esi, -44(%rbp)
	movb	%cl, (%rdi,%rdx)
	movq	-56(%rbp), %rcx
	movl	-44(%rbp), %edx
	movb	$0, (%rcx,%rdx)
	jmp	.LBB0_695
.LBB0_698:
	leaq	R64(%rip), %rcx
	jmp	.LBB0_701
.LBB0_699:
	leaq	R16(%rip), %rcx
	jmp	.LBB0_701
.LBB0_700:
	cmpl	$64, -108(%rbp)                 # 4-byte Folded Reload
	leaq	R8L(%rip), %rdx
	leaq	R8H(%rip), %rcx
	cmoveq	%rdx, %rcx
.LBB0_701:
	movq	(%rcx), %rcx
	movzbl	(%rcx), %edx
	testb	%dl, %dl
	je	.LBB0_706
# %bb.702:
	incq	%rcx
	jmp	.LBB0_704
.LBB0_703:                              #   in Loop: Header=BB0_704 Depth=1
	movzbl	(%rcx), %edx
	incq	%rcx
	testb	%dl, %dl
	je	.LBB0_706
.LBB0_704:                              # =>This Inner Loop Header: Depth=1
	movl	-44(%rbp), %esi
	leal	1(%rsi), %edi
	cmpl	-48(%rbp), %edi
	jae	.LBB0_703
# %bb.705:                              #   in Loop: Header=BB0_704 Depth=1
	movq	-56(%rbp), %r8
	movl	%edi, -44(%rbp)
	movb	%dl, (%r8,%rsi)
	movq	-56(%rbp), %rdx
	movl	-44(%rbp), %esi
	movb	$0, (%rdx,%rsi)
	jmp	.LBB0_703
.LBB0_706:
	movb	$44, %sil
	xorl	%ecx, %ecx
	leaq	.L.str.10(%rip), %rdx
	jmp	.LBB0_708
.LBB0_707:                              #   in Loop: Header=BB0_708 Depth=1
	movzbl	1(%rcx,%rdx), %esi
	incq	%rcx
	cmpq	$2, %rcx
	je	.LBB0_751
.LBB0_708:                              # =>This Inner Loop Header: Depth=1
	movl	-44(%rbp), %edi
	leal	1(%rdi), %r8d
	cmpl	-48(%rbp), %r8d
	jae	.LBB0_707
# %bb.709:                              #   in Loop: Header=BB0_708 Depth=1
	movq	-56(%rbp), %r9
	movl	%r8d, -44(%rbp)
	movb	%sil, (%r9,%rdi)
	movq	-56(%rbp), %rsi
	movl	-44(%rbp), %edi
	movb	$0, (%rsi,%rdi)
	jmp	.LBB0_707
.LBB0_710:
	movl	-44(%rbp), %eax
	leal	1(%rax), %ecx
	cmpl	-48(%rbp), %ecx
	jae	.LBB0_712
# %bb.711:
	movq	-56(%rbp), %rdx
	movl	%ecx, -44(%rbp)
	movb	$32, (%rdx,%rax)
	movq	-56(%rbp), %rax
	movl	-44(%rbp), %ecx
	movb	$0, (%rax,%rcx)
.LBB0_712:
	xorl	%r8d, %r8d
	cmpl	$0, -176(%rbp)
	movl	-64(%rbp), %edx                 # 4-byte Reload
	cmovel	%edx, %r8d
	leaq	-56(%rbp), %rdi
	leaq	-176(%rbp), %rsi
	movl	-68(%rbp), %ecx                 # 4-byte Reload
	xorl	%r9d, %r9d
	callq	render_rm
	cmpl	$192, %r12d
	jne	.LBB0_756
# %bb.713:
	movl	-92(%rbp), %eax
	cmpl	-96(%rbp), %eax
	jae	.LBB0_769
# %bb.714:
	movq	-104(%rbp), %rcx
	leal	1(%rax), %edx
	movl	%edx, -92(%rbp)
	movzbl	(%rcx,%rax), %esi
	jmp	.LBB0_770
.LBB0_715:
	movl	$1, -88(%rbp)
	xorl	%esi, %esi
.LBB0_716:
	movb	$109, %dl
	xorl	%eax, %eax
	leaq	.L.str.55(%rip), %rdi
	jmp	.LBB0_718
.LBB0_717:                              #   in Loop: Header=BB0_718 Depth=1
	movzbl	1(%rax,%rdi), %edx
	incq	%rax
	cmpq	$4, %rax
	je	.LBB0_720
.LBB0_718:                              # =>This Inner Loop Header: Depth=1
	movl	-44(%rbp), %r8d
	leal	1(%r8), %r9d
	cmpl	-48(%rbp), %r9d
	jae	.LBB0_717
# %bb.719:                              #   in Loop: Header=BB0_718 Depth=1
	movq	-56(%rbp), %r10
	movl	%r9d, -44(%rbp)
	movb	%dl, (%r10,%r8)
	movq	-56(%rbp), %rdx
	movl	-44(%rbp), %r8d
	movb	$0, (%rdx,%r8)
	jmp	.LBB0_717
.LBB0_720:
	movl	%ebx, %eax
	andl	$15, %eax
	shll	$6, %ecx
	shll	$3, %eax
	orl	%ecx, %eax
	leaq	R8L(%rip), %rcx
	addq	%rax, %rcx
	andl	$7, %ebx
	cmpl	$64, -108(%rbp)                 # 4-byte Folded Reload
	leaq	R8H(%rip), %rax
	leaq	(%rax,%rbx,8), %rax
	cmoveq	%rcx, %rax
	movq	(%rax), %rax
	movzbl	(%rax), %ecx
	testb	%cl, %cl
	je	.LBB0_725
# %bb.721:
	incq	%rax
	jmp	.LBB0_723
.LBB0_722:                              #   in Loop: Header=BB0_723 Depth=1
	movzbl	(%rax), %ecx
	incq	%rax
	testb	%cl, %cl
	je	.LBB0_725
.LBB0_723:                              # =>This Inner Loop Header: Depth=1
	movl	-44(%rbp), %edx
	leal	1(%rdx), %edi
	cmpl	-48(%rbp), %edi
	jae	.LBB0_722
# %bb.724:                              #   in Loop: Header=BB0_723 Depth=1
	movq	-56(%rbp), %r8
	movl	%edi, -44(%rbp)
	movb	%cl, (%r8,%rdx)
	movq	-56(%rbp), %rcx
	movl	-44(%rbp), %edx
	movb	$0, (%rcx,%rdx)
	jmp	.LBB0_722
.LBB0_725:
	movb	$44, %dl
	xorl	%eax, %eax
	leaq	.L.str.10(%rip), %rcx
	jmp	.LBB0_727
.LBB0_726:                              #   in Loop: Header=BB0_727 Depth=1
	movzbl	1(%rax,%rcx), %edx
	incq	%rax
	cmpq	$2, %rax
	je	.LBB0_753
.LBB0_727:                              # =>This Inner Loop Header: Depth=1
	movl	-44(%rbp), %edi
	leal	1(%rdi), %r8d
	cmpl	-48(%rbp), %r8d
	jae	.LBB0_726
# %bb.728:                              #   in Loop: Header=BB0_727 Depth=1
	movq	-56(%rbp), %r9
	movl	%r8d, -44(%rbp)
	movb	%dl, (%r9,%rdi)
	movq	-56(%rbp), %rdx
	movl	-44(%rbp), %edi
	movb	$0, (%rdx,%rdi)
	jmp	.LBB0_726
.LBB0_729:
	leaq	-104(%rbp), %rdi
	callq	rd64
	movb	$109, %sil
	movl	$1, %ecx
	leaq	.L.str.71(%rip), %rdx
	jmp	.LBB0_731
.LBB0_730:                              #   in Loop: Header=BB0_731 Depth=1
	movzbl	(%rcx,%rdx), %esi
	incq	%rcx
	cmpq	$8, %rcx
	je	.LBB0_738
.LBB0_731:                              # =>This Inner Loop Header: Depth=1
	movl	-44(%rbp), %edi
	leal	1(%rdi), %r8d
	cmpl	-48(%rbp), %r8d
	jae	.LBB0_730
# %bb.732:                              #   in Loop: Header=BB0_731 Depth=1
	movq	-56(%rbp), %r9
	movl	%r8d, -44(%rbp)
	movb	%sil, (%r9,%rdi)
	movq	-56(%rbp), %rsi
	movl	-44(%rbp), %edi
	movb	$0, (%rsi,%rdi)
	jmp	.LBB0_730
.LBB0_733:
	movl	%r14d, %edi
	movl	-64(%rbp), %esi                 # 4-byte Reload
	movl	-68(%rbp), %edx                 # 4-byte Reload
	callq	reg_name
	movzbl	(%rax), %ecx
	testb	%cl, %cl
	je	.LBB0_743
# %bb.734:
	incq	%rax
	jmp	.LBB0_736
.LBB0_735:                              #   in Loop: Header=BB0_736 Depth=1
	movzbl	(%rax), %ecx
	incq	%rax
	testb	%cl, %cl
	je	.LBB0_743
.LBB0_736:                              # =>This Inner Loop Header: Depth=1
	movl	-44(%rbp), %edx
	leal	1(%rdx), %esi
	cmpl	-48(%rbp), %esi
	jae	.LBB0_735
# %bb.737:                              #   in Loop: Header=BB0_736 Depth=1
	movq	-56(%rbp), %rdi
	movl	%esi, -44(%rbp)
	movb	%cl, (%rdi,%rdx)
	movq	-56(%rbp), %rcx
	movl	-44(%rbp), %edx
	movb	$0, (%rcx,%rdx)
	jmp	.LBB0_735
.LBB0_738:
	movl	%r14d, %ecx
	leaq	R64(%rip), %rdx
	movq	(%rdx,%rcx,8), %rcx
	movzbl	(%rcx), %edx
	testb	%dl, %dl
	je	.LBB0_747
# %bb.739:
	incq	%rcx
	jmp	.LBB0_741
.LBB0_740:                              #   in Loop: Header=BB0_741 Depth=1
	movzbl	(%rcx), %edx
	incq	%rcx
	testb	%dl, %dl
	je	.LBB0_747
.LBB0_741:                              # =>This Inner Loop Header: Depth=1
	movl	-44(%rbp), %esi
	leal	1(%rsi), %edi
	cmpl	-48(%rbp), %edi
	jae	.LBB0_740
# %bb.742:                              #   in Loop: Header=BB0_741 Depth=1
	movq	-56(%rbp), %r8
	movl	%edi, -44(%rbp)
	movb	%dl, (%r8,%rsi)
	movq	-56(%rbp), %rdx
	movl	-44(%rbp), %esi
	movb	$0, (%rdx,%rsi)
	jmp	.LBB0_740
.LBB0_743:
	movb	$44, %dl
	xorl	%eax, %eax
	leaq	.L.str.10(%rip), %rcx
	jmp	.LBB0_745
.LBB0_744:                              #   in Loop: Header=BB0_745 Depth=1
	movzbl	1(%rax,%rcx), %edx
	incq	%rax
	cmpq	$2, %rax
	je	.LBB0_752
.LBB0_745:                              # =>This Inner Loop Header: Depth=1
	movl	-44(%rbp), %esi
	leal	1(%rsi), %edi
	cmpl	-48(%rbp), %edi
	jae	.LBB0_744
# %bb.746:                              #   in Loop: Header=BB0_745 Depth=1
	movq	-56(%rbp), %r8
	movl	%edi, -44(%rbp)
	movb	%dl, (%r8,%rsi)
	movq	-56(%rbp), %rdx
	movl	-44(%rbp), %esi
	movb	$0, (%rdx,%rsi)
	jmp	.LBB0_744
.LBB0_747:
	movb	$44, %sil
	xorl	%ecx, %ecx
	leaq	.L.str.10(%rip), %rdx
	jmp	.LBB0_749
.LBB0_748:                              #   in Loop: Header=BB0_749 Depth=1
	movzbl	1(%rcx,%rdx), %esi
	incq	%rcx
	cmpq	$2, %rcx
	je	.LBB0_751
.LBB0_749:                              # =>This Inner Loop Header: Depth=1
	movl	-44(%rbp), %edi
	leal	1(%rdi), %r8d
	cmpl	-48(%rbp), %r8d
	jae	.LBB0_748
# %bb.750:                              #   in Loop: Header=BB0_749 Depth=1
	movq	-56(%rbp), %r9
	movl	%r8d, -44(%rbp)
	movb	%sil, (%r9,%rdi)
	movq	-56(%rbp), %rsi
	movl	-44(%rbp), %edi
	movb	$0, (%rsi,%rdi)
	jmp	.LBB0_748
.LBB0_751:
	leaq	-56(%rbp), %rdi
	movq	%rax, %rsi
	jmp	.LBB0_754
.LBB0_752:
	movl	%ebx, %esi
.LBB0_753:
	leaq	-56(%rbp), %rdi
.LBB0_754:
	callq	sb_0xhex
.LBB0_755:
	xorl	%ebx, %ebx
	xorl	%esi, %esi
	jmp	.LBB0_527
.LBB0_756:
	andl	$-46, %ebx
	movb	$44, %dl
	cmpl	$208, %ebx
	jne	.LBB0_775
# %bb.757:
	movl	$1, %eax
	leaq	.L.str.72(%rip), %rcx
	jmp	.LBB0_759
.LBB0_758:                              #   in Loop: Header=BB0_759 Depth=1
	movzbl	(%rax,%rcx), %edx
	incq	%rax
	cmpq	$4, %rax
	je	.LBB0_413
.LBB0_759:                              # =>This Inner Loop Header: Depth=1
	movl	-44(%rbp), %esi
	leal	1(%rsi), %edi
	cmpl	-48(%rbp), %edi
	jae	.LBB0_758
# %bb.760:                              #   in Loop: Header=BB0_759 Depth=1
	movq	-56(%rbp), %r8
	movl	%edi, -44(%rbp)
	movb	%dl, (%r8,%rsi)
	movq	-56(%rbp), %rdx
	movl	-44(%rbp), %esi
	movb	$0, (%rdx,%rsi)
	jmp	.LBB0_758
.LBB0_761:
	cmpl	$200, %ebx
	jle	.LBB0_779
# %bb.762:
	cmpl	$201, %ebx
	je	.LBB0_785
# %bb.763:
	cmpl	$205, %ebx
	je	.LBB0_789
# %bb.764:
	cmpl	$204, %ebx
	jne	.LBB0_793
# %bb.765:
	movb	$105, %r8b
	xorl	%edx, %edx
	leaq	.L.str.77(%rip), %rdi
	movq	-80(%rbp), %r11                 # 8-byte Reload
	movl	-60(%rbp), %r15d                # 4-byte Reload
	jmp	.LBB0_767
.LBB0_766:                              #   in Loop: Header=BB0_767 Depth=1
	movzbl	1(%rdx,%rdi), %r8d
	incq	%rdx
	xorl	%ebx, %ebx
	movl	$0, %esi
	cmpq	$4, %rdx
	je	.LBB0_328
.LBB0_767:                              # =>This Inner Loop Header: Depth=1
	movl	-44(%rbp), %eax
	leal	1(%rax), %ecx
	cmpl	-48(%rbp), %ecx
	jae	.LBB0_766
# %bb.768:                              #   in Loop: Header=BB0_767 Depth=1
	movq	-56(%rbp), %rsi
	movl	%ecx, -44(%rbp)
	movb	%r8b, (%rsi,%rax)
	movq	-56(%rbp), %rax
	movl	-44(%rbp), %ecx
	movb	$0, (%rax,%rcx)
	jmp	.LBB0_766
.LBB0_769:
	movl	$1, -88(%rbp)
	xorl	%esi, %esi
.LBB0_770:
	movb	$44, %dl
	xorl	%eax, %eax
	leaq	.L.str.10(%rip), %rcx
	jmp	.LBB0_772
.LBB0_771:                              #   in Loop: Header=BB0_772 Depth=1
	movzbl	1(%rax,%rcx), %edx
	incq	%rax
	cmpq	$2, %rax
	je	.LBB0_774
.LBB0_772:                              # =>This Inner Loop Header: Depth=1
	movl	-44(%rbp), %edi
	leal	1(%rdi), %r8d
	cmpl	-48(%rbp), %r8d
	jae	.LBB0_771
# %bb.773:                              #   in Loop: Header=BB0_772 Depth=1
	movq	-56(%rbp), %r9
	movl	%r8d, -44(%rbp)
	movb	%dl, (%r9,%rdi)
	movq	-56(%rbp), %rdx
	movl	-44(%rbp), %edi
	movb	$0, (%rdx,%rdi)
	jmp	.LBB0_771
.LBB0_774:
	leaq	-56(%rbp), %rdi
	jmp	.LBB0_412
.LBB0_775:
	xorl	%eax, %eax
	leaq	.L.str.73(%rip), %rcx
	jmp	.LBB0_777
.LBB0_776:                              #   in Loop: Header=BB0_777 Depth=1
	movzbl	1(%rax,%rcx), %edx
	incq	%rax
	cmpq	$4, %rax
	je	.LBB0_413
.LBB0_777:                              # =>This Inner Loop Header: Depth=1
	movl	-44(%rbp), %esi
	leal	1(%rsi), %edi
	cmpl	-48(%rbp), %edi
	jae	.LBB0_776
# %bb.778:                              #   in Loop: Header=BB0_777 Depth=1
	movq	-56(%rbp), %r8
	movl	%edi, -44(%rbp)
	movb	%dl, (%r8,%rsi)
	movq	-56(%rbp), %rdx
	movl	-44(%rbp), %esi
	movb	$0, (%rdx,%rsi)
	jmp	.LBB0_776
.LBB0_779:
	cmpl	$194, %ebx
	je	.LBB0_791
# %bb.780:
	cmpl	$195, %ebx
	jne	.LBB0_793
# %bb.781:
	movb	$114, %r8b
	movl	$1, %edx
	leaq	.L.str.75(%rip), %rdi
	movq	-80(%rbp), %r11                 # 8-byte Reload
	movl	-60(%rbp), %r15d                # 4-byte Reload
	jmp	.LBB0_783
.LBB0_782:                              #   in Loop: Header=BB0_783 Depth=1
	movzbl	(%rdx,%rdi), %r8d
	incq	%rdx
	xorl	%ebx, %ebx
	movl	$0, %esi
	cmpq	$4, %rdx
	je	.LBB0_328
.LBB0_783:                              # =>This Inner Loop Header: Depth=1
	movl	-44(%rbp), %eax
	leal	1(%rax), %ecx
	cmpl	-48(%rbp), %ecx
	jae	.LBB0_782
# %bb.784:                              #   in Loop: Header=BB0_783 Depth=1
	movq	-56(%rbp), %rsi
	movl	%ecx, -44(%rbp)
	movb	%r8b, (%rsi,%rax)
	movq	-56(%rbp), %rax
	movl	-44(%rbp), %ecx
	movb	$0, (%rax,%rcx)
	jmp	.LBB0_782
.LBB0_785:
	movb	$108, %r8b
	movl	$1, %edx
	leaq	.L.str.76(%rip), %rdi
	movq	-80(%rbp), %r11                 # 8-byte Reload
	movl	-60(%rbp), %r15d                # 4-byte Reload
	jmp	.LBB0_787
.LBB0_786:                              #   in Loop: Header=BB0_787 Depth=1
	movzbl	(%rdx,%rdi), %r8d
	incq	%rdx
	xorl	%ebx, %ebx
	movl	$0, %esi
	cmpq	$6, %rdx
	je	.LBB0_328
.LBB0_787:                              # =>This Inner Loop Header: Depth=1
	movl	-44(%rbp), %eax
	leal	1(%rax), %ecx
	cmpl	-48(%rbp), %ecx
	jae	.LBB0_786
# %bb.788:                              #   in Loop: Header=BB0_787 Depth=1
	movq	-56(%rbp), %rsi
	movl	%ecx, -44(%rbp)
	movb	%r8b, (%rsi,%rax)
	movq	-56(%rbp), %rax
	movl	-44(%rbp), %ecx
	movb	$0, (%rax,%rcx)
	jmp	.LBB0_786
.LBB0_789:
	movl	-92(%rbp), %eax
	cmpl	-96(%rbp), %eax
	jae	.LBB0_799
# %bb.790:
	movq	-104(%rbp), %rcx
	leal	1(%rax), %edx
	movl	%edx, -92(%rbp)
	movzbl	(%rcx,%rax), %esi
	jmp	.LBB0_800
.LBB0_791:
	movl	-96(%rbp), %ecx
	movl	-92(%rbp), %eax
	cmpl	%ecx, %eax
	jae	.LBB0_804
# %bb.792:
	movq	-104(%rbp), %rdx
	leal	1(%rax), %esi
	movl	%esi, -92(%rbp)
	movzbl	(%rdx,%rax), %eax
	jmp	.LBB0_805
.LBB0_793:
	cmpl	$232, %r12d
	je	.LBB0_813
# %bb.794:
	cmpl	$198, %r12d
	jne	.LBB0_818
# %bb.795:
	leaq	-104(%rbp), %r14
	leaq	-176(%rbp), %r8
	movq	%r14, %rdi
                                        # kill: def $ecx killed $ecx killed $rcx
	callq	parse_modrm
	xorl	%eax, %eax
	cmpl	$2, -64(%rbp)                   # 4-byte Folded Reload
	setne	%al
	cmpl	$198, %ebx
	leal	2(%rax,%rax), %eax
	movl	$1, %esi
	cmovnel	%eax, %esi
	movq	%r14, %rdi
	callq	rd_imm_sext
	movq	%rax, %r14
	movb	$109, %dl
	xorl	%eax, %eax
	leaq	.L.str.55(%rip), %rcx
	jmp	.LBB0_797
.LBB0_796:                              #   in Loop: Header=BB0_797 Depth=1
	movzbl	1(%rax,%rcx), %edx
	incq	%rax
	cmpq	$4, %rax
	je	.LBB0_825
.LBB0_797:                              # =>This Inner Loop Header: Depth=1
	movl	-44(%rbp), %esi
	leal	1(%rsi), %edi
	cmpl	-48(%rbp), %edi
	jae	.LBB0_796
# %bb.798:                              #   in Loop: Header=BB0_797 Depth=1
	movq	-56(%rbp), %r8
	movl	%edi, -44(%rbp)
	movb	%dl, (%r8,%rsi)
	movq	-56(%rbp), %rdx
	movl	-44(%rbp), %esi
	movb	$0, (%rdx,%rsi)
	jmp	.LBB0_796
.LBB0_799:
	movl	$1, -88(%rbp)
	xorl	%esi, %esi
.LBB0_800:
	movb	$105, %dl
	xorl	%eax, %eax
	leaq	.L.str.78(%rip), %rcx
	jmp	.LBB0_802
.LBB0_801:                              #   in Loop: Header=BB0_802 Depth=1
	movzbl	1(%rax,%rcx), %edx
	incq	%rax
	cmpq	$4, %rax
	je	.LBB0_753
.LBB0_802:                              # =>This Inner Loop Header: Depth=1
	movl	-44(%rbp), %edi
	leal	1(%rdi), %r8d
	cmpl	-48(%rbp), %r8d
	jae	.LBB0_801
# %bb.803:                              #   in Loop: Header=BB0_802 Depth=1
	movq	-56(%rbp), %r9
	movl	%r8d, -44(%rbp)
	movb	%dl, (%r9,%rdi)
	movq	-56(%rbp), %rdx
	movl	-44(%rbp), %edi
	movb	$0, (%rdx,%rdi)
	jmp	.LBB0_801
.LBB0_804:
	movl	$1, -88(%rbp)
	xorl	%eax, %eax
.LBB0_805:
	movl	-92(%rbp), %edx
	cmpl	%ecx, %edx
	jae	.LBB0_807
# %bb.806:
	movq	-104(%rbp), %rcx
	leal	1(%rdx), %esi
	movl	%esi, -92(%rbp)
	movzbl	(%rcx,%rdx), %esi
	shll	$8, %esi
	jmp	.LBB0_808
.LBB0_807:
	movl	$1, -88(%rbp)
	xorl	%esi, %esi
.LBB0_808:
	movb	$114, %dil
	xorl	%ecx, %ecx
	leaq	.L.str.74(%rip), %rdx
	jmp	.LBB0_810
.LBB0_809:                              #   in Loop: Header=BB0_810 Depth=1
	movzbl	1(%rcx,%rdx), %edi
	incq	%rcx
	cmpq	$4, %rcx
	je	.LBB0_812
.LBB0_810:                              # =>This Inner Loop Header: Depth=1
	movl	-44(%rbp), %r8d
	leal	1(%r8), %r9d
	cmpl	-48(%rbp), %r9d
	jae	.LBB0_809
# %bb.811:                              #   in Loop: Header=BB0_810 Depth=1
	movq	-56(%rbp), %r10
	movl	%r9d, -44(%rbp)
	movb	%dil, (%r10,%r8)
	movq	-56(%rbp), %rdi
	movl	-44(%rbp), %r8d
	movb	$0, (%rdi,%r8)
	jmp	.LBB0_809
.LBB0_812:
	orq	%rax, %rsi
	jmp	.LBB0_753
.LBB0_813:
	leaq	-104(%rbp), %rdi
	movl	$4, %esi
	callq	rd_imm_sext
	cmpl	$232, %ebx
	leaq	.L.str.79(%rip), %rdx
	leaq	.L.str.80(%rip), %rcx
	cmoveq	%rdx, %rcx
	movzbl	(%rcx), %edx
	testb	%dl, %dl
	je	.LBB0_829
# %bb.814:
	incq	%rcx
	jmp	.LBB0_816
.LBB0_815:                              #   in Loop: Header=BB0_816 Depth=1
	movzbl	(%rcx), %edx
	incq	%rcx
	testb	%dl, %dl
	je	.LBB0_829
.LBB0_816:                              # =>This Inner Loop Header: Depth=1
	movl	-44(%rbp), %esi
	leal	1(%rsi), %edi
	cmpl	-48(%rbp), %edi
	jae	.LBB0_815
# %bb.817:                              #   in Loop: Header=BB0_816 Depth=1
	movq	-56(%rbp), %r8
	movl	%edi, -44(%rbp)
	movb	%dl, (%r8,%rsi)
	movq	-56(%rbp), %rdx
	movl	-44(%rbp), %esi
	movb	$0, (%rdx,%rsi)
	jmp	.LBB0_815
.LBB0_818:
	cmpl	$235, %ebx
	je	.LBB0_830
# %bb.819:
	cmpl	$244, %ebx
	je	.LBB0_832
# %bb.820:
	cmpl	$245, %ebx
	jne	.LBB0_836
# %bb.821:
	movb	$99, %r8b
	movl	$1, %edx
	leaq	.L.str.82(%rip), %rdi
	movq	-80(%rbp), %r11                 # 8-byte Reload
	movl	-60(%rbp), %r15d                # 4-byte Reload
	jmp	.LBB0_823
.LBB0_822:                              #   in Loop: Header=BB0_823 Depth=1
	movzbl	(%rdx,%rdi), %r8d
	incq	%rdx
	xorl	%ebx, %ebx
	movl	$0, %esi
	cmpq	$4, %rdx
	je	.LBB0_328
.LBB0_823:                              # =>This Inner Loop Header: Depth=1
	movl	-44(%rbp), %eax
	leal	1(%rax), %ecx
	cmpl	-48(%rbp), %ecx
	jae	.LBB0_822
# %bb.824:                              #   in Loop: Header=BB0_823 Depth=1
	movq	-56(%rbp), %rsi
	movl	%ecx, -44(%rbp)
	movb	%r8b, (%rsi,%rax)
	movq	-56(%rbp), %rax
	movl	-44(%rbp), %ecx
	movb	$0, (%rax,%rcx)
	jmp	.LBB0_822
.LBB0_825:
	cmpl	$198, %ebx
	movl	$1, %eax
	movl	-64(%rbp), %edx                 # 4-byte Reload
	cmovel	%eax, %edx
	xorl	%ebx, %ebx
	cmpl	$0, -176(%rbp)
	movl	$0, %r8d
	cmovel	%edx, %r8d
	leaq	-56(%rbp), %rdi
	leaq	-176(%rbp), %rsi
	movl	-68(%rbp), %ecx                 # 4-byte Reload
	xorl	%r9d, %r9d
	callq	render_rm
	movb	$44, %cl
	leaq	.L.str.10(%rip), %rax
	jmp	.LBB0_827
.LBB0_826:                              #   in Loop: Header=BB0_827 Depth=1
	movzbl	1(%rbx,%rax), %ecx
	incq	%rbx
	cmpq	$2, %rbx
	je	.LBB0_411
.LBB0_827:                              # =>This Inner Loop Header: Depth=1
	movl	-44(%rbp), %edx
	leal	1(%rdx), %esi
	cmpl	-48(%rbp), %esi
	jae	.LBB0_826
# %bb.828:                              #   in Loop: Header=BB0_827 Depth=1
	movq	-56(%rbp), %rdi
	movl	%esi, -44(%rbp)
	movb	%cl, (%rdi,%rdx)
	movq	-56(%rbp), %rcx
	movl	-44(%rbp), %edx
	movb	$0, (%rcx,%rdx)
	jmp	.LBB0_826
.LBB0_411:
	leaq	-56(%rbp), %rdi
	movq	%r14, %rsi
.LBB0_412:
	callq	sb_0xhex
.LBB0_413:
	cmpl	$0, -164(%rbp)
	setne	%sil
	je	.LBB0_414
.LBB0_526:
	movq	-136(%rbp), %rbx
	jmp	.LBB0_527
.LBB0_414:
	xorl	%ebx, %ebx
.LBB0_527:
	movq	-80(%rbp), %r11                 # 8-byte Reload
	movl	-60(%rbp), %r15d                # 4-byte Reload
	jmp	.LBB0_328
.LBB0_829:
	movq	-80(%rbp), %rbx                 # 8-byte Reload
	movl	$1, 12(%rbx)
	movl	-92(%rbp), %ecx
	addq	-184(%rbp), %rax                # 8-byte Folded Reload
	addq	%rcx, %rax
	movq	%rax, 16(%rbx)
	leaq	-56(%rbp), %rdi
	movq	%rax, %rsi
	jmp	.LBB0_848
.LBB0_830:
	movl	-92(%rbp), %eax
	cmpl	-96(%rbp), %eax
	jae	.LBB0_842
# %bb.831:
	movq	-104(%rbp), %rcx
	leal	1(%rax), %edx
	movl	%edx, -92(%rbp)
	movsbq	(%rcx,%rax), %rsi
	jmp	.LBB0_843
.LBB0_832:
	movb	$104, %r8b
	movl	$1, %edx
	leaq	.L.str.81(%rip), %rdi
	movq	-80(%rbp), %r11                 # 8-byte Reload
	movl	-60(%rbp), %r15d                # 4-byte Reload
	jmp	.LBB0_834
.LBB0_833:                              #   in Loop: Header=BB0_834 Depth=1
	movzbl	(%rdx,%rdi), %r8d
	incq	%rdx
	xorl	%ebx, %ebx
	movl	$0, %esi
	cmpq	$4, %rdx
	je	.LBB0_328
.LBB0_834:                              # =>This Inner Loop Header: Depth=1
	movl	-44(%rbp), %eax
	leal	1(%rax), %ecx
	cmpl	-48(%rbp), %ecx
	jae	.LBB0_833
# %bb.835:                              #   in Loop: Header=BB0_834 Depth=1
	movq	-56(%rbp), %rsi
	movl	%ecx, -44(%rbp)
	movb	%r8b, (%rsi,%rax)
	movq	-56(%rbp), %rax
	movl	-44(%rbp), %ecx
	movb	$0, (%rax,%rcx)
	jmp	.LBB0_833
.LBB0_836:
	cmpl	$246, %r12d
	jne	.LBB0_849
# %bb.837:
	cmpl	$246, %ebx
	movl	$1, %r14d
	cmovnel	-64(%rbp), %r14d                # 4-byte Folded Reload
	leaq	-104(%rbp), %rdi
	leaq	-176(%rbp), %r8
                                        # kill: def $ecx killed $ecx killed $rcx
	callq	parse_modrm
	movslq	-128(%rbp), %rax
	leaq	x86_decode.G3(%rip), %rcx
	movq	(%rcx,%rax,8), %rax
	movzbl	(%rax), %ecx
	testb	%cl, %cl
	je	.LBB0_859
# %bb.838:
	incq	%rax
	jmp	.LBB0_840
.LBB0_839:                              #   in Loop: Header=BB0_840 Depth=1
	movzbl	(%rax), %ecx
	incq	%rax
	testb	%cl, %cl
	je	.LBB0_859
.LBB0_840:                              # =>This Inner Loop Header: Depth=1
	movl	-44(%rbp), %edx
	leal	1(%rdx), %esi
	cmpl	-48(%rbp), %esi
	jae	.LBB0_839
# %bb.841:                              #   in Loop: Header=BB0_840 Depth=1
	movq	-56(%rbp), %rdi
	movl	%esi, -44(%rbp)
	movb	%cl, (%rdi,%rdx)
	movq	-56(%rbp), %rcx
	movl	-44(%rbp), %edx
	movb	$0, (%rcx,%rdx)
	jmp	.LBB0_839
.LBB0_842:
	movl	$1, -88(%rbp)
	xorl	%esi, %esi
.LBB0_843:
	movb	$106, %dl
	xorl	%eax, %eax
	leaq	.L.str.80(%rip), %rcx
	jmp	.LBB0_845
.LBB0_844:                              #   in Loop: Header=BB0_845 Depth=1
	movzbl	1(%rax,%rcx), %edx
	incq	%rax
	cmpq	$4, %rax
	je	.LBB0_847
.LBB0_845:                              # =>This Inner Loop Header: Depth=1
	movl	-44(%rbp), %edi
	leal	1(%rdi), %r8d
	cmpl	-48(%rbp), %r8d
	jae	.LBB0_844
# %bb.846:                              #   in Loop: Header=BB0_845 Depth=1
	movq	-56(%rbp), %r9
	movl	%r8d, -44(%rbp)
	movb	%dl, (%r9,%rdi)
	movq	-56(%rbp), %rdx
	movl	-44(%rbp), %edi
	movb	$0, (%rdx,%rdi)
	jmp	.LBB0_844
.LBB0_847:
	movq	-80(%rbp), %rbx                 # 8-byte Reload
	movl	$1, 12(%rbx)
	movl	-92(%rbp), %eax
	addq	-184(%rbp), %rsi                # 8-byte Folded Reload
	addq	%rax, %rsi
	movq	%rsi, 16(%rbx)
	leaq	-56(%rbp), %rdi
.LBB0_848:
	callq	sb_0xhex
	movq	%rbx, %r11
	xorl	%ebx, %ebx
	xorl	%esi, %esi
	movl	-60(%rbp), %r15d                # 4-byte Reload
	jmp	.LBB0_328
.LBB0_849:
	cmpl	$254, %ebx
	je	.LBB0_867
# %bb.850:
	cmpl	$255, %ebx
	jne	.LBB0_872
# %bb.851:
	leaq	-104(%rbp), %rdi
	leaq	-176(%rbp), %r8
                                        # kill: def $ecx killed $ecx killed $rcx
	callq	parse_modrm
	movslq	-128(%rbp), %rax
	cmpq	$6, %rax
	ja	.LBB0_854
# %bb.852:
	movl	$84, %ecx
	btl	%eax, %ecx
	jae	.LBB0_854
# %bb.853:
	movl	$8, -64(%rbp)                   # 4-byte Folded Spill
.LBB0_854:
	leaq	x86_decode.G5(%rip), %rcx
	movq	(%rcx,%rax,8), %rax
	movzbl	(%rax), %ecx
	testb	%cl, %cl
	je	.LBB0_876
# %bb.855:
	incq	%rax
	jmp	.LBB0_857
.LBB0_856:                              #   in Loop: Header=BB0_857 Depth=1
	movzbl	(%rax), %ecx
	incq	%rax
	testb	%cl, %cl
	je	.LBB0_876
.LBB0_857:                              # =>This Inner Loop Header: Depth=1
	movl	-44(%rbp), %edx
	leal	1(%rdx), %esi
	cmpl	-48(%rbp), %esi
	jae	.LBB0_856
# %bb.858:                              #   in Loop: Header=BB0_857 Depth=1
	movq	-56(%rbp), %rdi
	movl	%esi, -44(%rbp)
	movb	%cl, (%rdi,%rdx)
	movq	-56(%rbp), %rcx
	movl	-44(%rbp), %edx
	movb	$0, (%rcx,%rdx)
	jmp	.LBB0_856
.LBB0_859:
	movl	-44(%rbp), %eax
	leal	1(%rax), %ecx
	cmpl	-48(%rbp), %ecx
	jae	.LBB0_861
# %bb.860:
	movq	-56(%rbp), %rdx
	movl	%ecx, -44(%rbp)
	movb	$32, (%rdx,%rax)
	movq	-56(%rbp), %rax
	movl	-44(%rbp), %ecx
	movb	$0, (%rax,%rcx)
.LBB0_861:
	xorl	%r8d, %r8d
	cmpl	$0, -176(%rbp)
	cmovel	%r14d, %r8d
	leaq	-56(%rbp), %rdi
	leaq	-176(%rbp), %rsi
	movl	%r14d, %edx
	movl	-68(%rbp), %ecx                 # 4-byte Reload
	xorl	%r9d, %r9d
	callq	render_rm
	cmpl	$1, -128(%rbp)
	jg	.LBB0_413
# %bb.862:
	xorl	%eax, %eax
	cmpl	$2, -64(%rbp)                   # 4-byte Folded Reload
	setne	%al
	cmpl	$246, %ebx
	leal	2(%rax,%rax), %eax
	movl	$1, %esi
	cmovnel	%eax, %esi
	leaq	-104(%rbp), %rdi
	callq	rd_imm_sext
	movb	$44, %sil
	xorl	%ecx, %ecx
	leaq	.L.str.10(%rip), %rdx
	jmp	.LBB0_864
.LBB0_863:                              #   in Loop: Header=BB0_864 Depth=1
	movzbl	1(%rcx,%rdx), %esi
	incq	%rcx
	cmpq	$2, %rcx
	je	.LBB0_866
.LBB0_864:                              # =>This Inner Loop Header: Depth=1
	movl	-44(%rbp), %edi
	leal	1(%rdi), %r8d
	cmpl	-48(%rbp), %r8d
	jae	.LBB0_863
# %bb.865:                              #   in Loop: Header=BB0_864 Depth=1
	movq	-56(%rbp), %r9
	movl	%r8d, -44(%rbp)
	movb	%sil, (%r9,%rdi)
	movq	-56(%rbp), %rsi
	movl	-44(%rbp), %edi
	movb	$0, (%rsi,%rdi)
	jmp	.LBB0_863
.LBB0_866:
	leaq	-56(%rbp), %rdi
	movq	%rax, %rsi
	jmp	.LBB0_412
.LBB0_867:
	leaq	-104(%rbp), %rdi
	leaq	-176(%rbp), %r8
                                        # kill: def $ecx killed $ecx killed $rcx
	callq	parse_modrm
	movl	-128(%rbp), %eax
	cmpl	$1, %eax
	leaq	.L.str.91(%rip), %rcx
	leaq	.L.str.92(%rip), %rdx
	cmoveq	%rcx, %rdx
	testl	%eax, %eax
	leaq	.L.str.90(%rip), %rax
	cmovneq	%rdx, %rax
	movzbl	(%rax), %ecx
	testb	%cl, %cl
	je	.LBB0_879
# %bb.868:
	incq	%rax
	jmp	.LBB0_870
.LBB0_869:                              #   in Loop: Header=BB0_870 Depth=1
	movzbl	(%rax), %ecx
	incq	%rax
	testb	%cl, %cl
	je	.LBB0_879
.LBB0_870:                              # =>This Inner Loop Header: Depth=1
	movl	-44(%rbp), %edx
	leal	1(%rdx), %esi
	cmpl	-48(%rbp), %esi
	jae	.LBB0_869
# %bb.871:                              #   in Loop: Header=BB0_870 Depth=1
	movq	-56(%rbp), %rdi
	movl	%esi, -44(%rbp)
	movb	%cl, (%rdi,%rdx)
	movq	-56(%rbp), %rcx
	movl	-44(%rbp), %edx
	movb	$0, (%rcx,%rdx)
	jmp	.LBB0_869
.LBB0_872:
	movb	$40, %r8b
	movl	$1, %edx
	leaq	.L.str.48(%rip), %rdi
	movq	-80(%rbp), %r11                 # 8-byte Reload
	movl	-60(%rbp), %r15d                # 4-byte Reload
	jmp	.LBB0_874
.LBB0_873:                              #   in Loop: Header=BB0_874 Depth=1
	movzbl	(%rdx,%rdi), %r8d
	incq	%rdx
	xorl	%ebx, %ebx
	movl	$0, %esi
	cmpq	$6, %rdx
	je	.LBB0_328
.LBB0_874:                              # =>This Inner Loop Header: Depth=1
	movl	-44(%rbp), %eax
	leal	1(%rax), %ecx
	cmpl	-48(%rbp), %ecx
	jae	.LBB0_873
# %bb.875:                              #   in Loop: Header=BB0_874 Depth=1
	movq	-56(%rbp), %rsi
	movl	%ecx, -44(%rbp)
	movb	%r8b, (%rsi,%rax)
	movq	-56(%rbp), %rax
	movl	-44(%rbp), %ecx
	movb	$0, (%rax,%rcx)
	jmp	.LBB0_873
.LBB0_876:
	movl	-44(%rbp), %eax
	leal	1(%rax), %ecx
	cmpl	-48(%rbp), %ecx
	jae	.LBB0_878
# %bb.877:
	movq	-56(%rbp), %rdx
	movl	%ecx, -44(%rbp)
	movb	$32, (%rdx,%rax)
	movq	-56(%rbp), %rax
	movl	-44(%rbp), %ecx
	movb	$0, (%rax,%rcx)
.LBB0_878:
	xorl	%ebx, %ebx
	cmpl	$0, -176(%rbp)
	movl	$0, %r8d
	movl	-64(%rbp), %edx                 # 4-byte Reload
	cmovel	%edx, %r8d
	leaq	-56(%rbp), %rdi
	leaq	-176(%rbp), %rsi
	movl	-68(%rbp), %ecx                 # 4-byte Reload
	xorl	%r9d, %r9d
	jmp	.LBB0_525
.LBB0_879:
	xorl	%r8d, %r8d
	cmpl	$0, -176(%rbp)
	sete	%r8b
	leaq	-56(%rbp), %rdi
	leaq	-176(%rbp), %rsi
	xorl	%ebx, %ebx
	movl	$1, %edx
	movl	-68(%rbp), %ecx                 # 4-byte Reload
	xorl	%r9d, %r9d
	callq	render_rm
	xorl	%esi, %esi
	jmp	.LBB0_527
.Lfunc_end0:
	.size	x86_decode, .Lfunc_end0-x86_decode
                                        # -- End function
	.p2align	4                               # -- Begin function parse_modrm
	.type	parse_modrm,@function
parse_modrm:                            # @parse_modrm
# %bb.0:
	movl	8(%rdi), %eax
	movl	12(%rdi), %r9d
	cmpl	%eax, %r9d
	jae	.LBB1_1
# %bb.2:
	movq	(%rdi), %r10
	leal	1(%r9), %r11d
	movl	%r11d, 12(%rdi)
	movzbl	(%r10,%r9), %r10d
	jmp	.LBB1_3
.LBB1_1:
	movl	$1, 16(%rdi)
	xorl	%r10d, %r10d
.LBB1_3:
	pushq	%rbp
	movq	%rsp, %rbp
	pushq	%r14
	pushq	%rbx
	movl	%r10d, %r9d
	shrl	$6, %r9d
	movl	%r10d, %ebx
	shrl	$3, %ebx
	andl	$7, %ebx
	movl	%r10d, %r11d
	andl	$7, %r11d
	movl	$0, (%r8)
	movq	$0, 12(%r8)
	movl	$0, 24(%r8)
	movl	$1, 32(%r8)
	movq	$0, 40(%r8)
	movl	%ebx, 48(%r8)
	xorl	%r14d, %r14d
	testl	%esi, %esi
	setne	%r14b
	leal	(%rbx,%r14,8), %esi
	movl	%esi, 4(%r8)
	cmpl	$3, %r9d
	jne	.LBB1_5
# %bb.4:
	movl	$1, (%r8)
	xorl	%eax, %eax
	testl	%ecx, %ecx
	setne	%al
	leal	(%r11,%rax,8), %eax
	movl	%eax, 8(%r8)
	jmp	.LBB1_23
.LBB1_5:
	cmpl	$4, %r11d
	jne	.LBB1_15
# %bb.6:
	movl	12(%rdi), %esi
	cmpl	%eax, %esi
	jae	.LBB1_7
# %bb.8:
	movq	(%rdi), %rax
	leal	1(%rsi), %r11d
	movl	%r11d, 12(%rdi)
	movzbl	(%rax,%rsi), %eax
	jmp	.LBB1_9
.LBB1_15:
	andl	$-57, %r10d
	cmpl	$5, %r10d
	jne	.LBB1_14
# %bb.16:
	movl	$1, 12(%r8)
	movl	$4, %esi
	jmp	.LBB1_17
.LBB1_7:
	movl	$1, 16(%rdi)
	xorl	%eax, %eax
.LBB1_9:
	movl	%eax, %esi
	shrl	$3, %esi
	andl	$7, %esi
	movl	%eax, %r11d
	andl	$7, %r11d
	testl	%edx, %edx
	jne	.LBB1_11
# %bb.10:
	cmpl	$4, %esi
	je	.LBB1_12
.LBB1_11:
	xorl	%ebx, %ebx
	testl	%edx, %edx
	setne	%bl
	shrl	$6, %eax
	movl	$1, 24(%r8)
	leal	(%rsi,%rbx,8), %edx
	movl	%edx, 28(%r8)
	movl	$1, %esi
	movl	%ecx, %edx
	movl	%eax, %ecx
	shll	%cl, %esi
	movl	%edx, %ecx
	movl	%esi, 32(%r8)
.LBB1_12:
	cmpl	$5, %r11d
	jne	.LBB1_14
# %bb.13:
	movl	$4, %esi
	cmpl	$64, %r10d
	jb	.LBB1_17
.LBB1_14:
	movl	$1, 16(%r8)
	xorl	%eax, %eax
	testl	%ecx, %ecx
	setne	%al
	leal	(%r11,%rax,8), %eax
	movl	%eax, 20(%r8)
	xorl	%esi, %esi
.LBB1_17:
	cmpl	$1, %r9d
	je	.LBB1_18
# %bb.19:
	cmpl	$2, %r9d
	jne	.LBB1_21
# %bb.20:
	movl	$4, %esi
.LBB1_21:
	testl	%esi, %esi
	je	.LBB1_23
.LBB1_22:
	movq	%r8, %rbx
	callq	rd_imm_sext
	movq	%rax, 40(%rbx)
.LBB1_23:
	popq	%rbx
	popq	%r14
	popq	%rbp
	retq
.LBB1_18:
	movl	%r9d, %esi
	testl	%esi, %esi
	jne	.LBB1_22
	jmp	.LBB1_23
.Lfunc_end1:
	.size	parse_modrm, .Lfunc_end1-parse_modrm
                                        # -- End function
	.p2align	4                               # -- Begin function rd_imm_sext
	.type	rd_imm_sext,@function
rd_imm_sext:                            # @rd_imm_sext
# %bb.0:
	pushq	%rbp
	movq	%rsp, %rbp
	cmpl	$4, %esi
	je	.LBB2_12
# %bb.1:
	cmpl	$2, %esi
	je	.LBB2_5
# %bb.2:
	cmpl	$1, %esi
	jne	.LBB2_25
# %bb.3:
	movl	12(%rdi), %eax
	cmpl	8(%rdi), %eax
	jae	.LBB2_4
# %bb.26:
	movq	(%rdi), %rcx
	leal	1(%rax), %edx
	movl	%edx, 12(%rdi)
	movsbq	(%rcx,%rax), %rax
	popq	%rbp
	retq
.LBB2_5:
	movl	8(%rdi), %ecx
	movl	12(%rdi), %eax
	cmpl	%ecx, %eax
	jae	.LBB2_6
# %bb.7:
	movq	(%rdi), %rdx
	leal	1(%rax), %esi
	movl	%esi, 12(%rdi)
	movzbl	(%rdx,%rax), %eax
	jmp	.LBB2_8
.LBB2_12:
	movl	8(%rdi), %ecx
	movl	12(%rdi), %eax
	cmpl	%ecx, %eax
	jae	.LBB2_13
# %bb.14:
	movq	(%rdi), %rdx
	leal	1(%rax), %esi
	movl	%esi, 12(%rdi)
	movzbl	(%rdx,%rax), %eax
	jmp	.LBB2_15
.LBB2_25:
	popq	%rbp
	jmp	rd64                            # TAILCALL
.LBB2_6:
	movl	$1, 16(%rdi)
	xorl	%eax, %eax
.LBB2_8:
	movl	12(%rdi), %edx
	cmpl	%ecx, %edx
	jae	.LBB2_9
# %bb.10:
	movq	(%rdi), %rcx
	leal	1(%rdx), %esi
	movl	%esi, 12(%rdi)
	movzbl	(%rcx,%rdx), %ecx
	shll	$8, %ecx
	jmp	.LBB2_11
.LBB2_9:
	movl	$1, 16(%rdi)
	xorl	%ecx, %ecx
.LBB2_11:
	orl	%eax, %ecx
	movswq	%cx, %rax
	popq	%rbp
	retq
.LBB2_13:
	movl	$1, 16(%rdi)
	xorl	%eax, %eax
.LBB2_15:
	movl	12(%rdi), %edx
	cmpl	%ecx, %edx
	jae	.LBB2_16
# %bb.17:
	movq	(%rdi), %rsi
	leal	1(%rdx), %r8d
	movl	%r8d, 12(%rdi)
	movzbl	(%rsi,%rdx), %edx
	shll	$8, %edx
	jmp	.LBB2_18
.LBB2_16:
	movl	$1, 16(%rdi)
	xorl	%edx, %edx
.LBB2_18:
	movl	12(%rdi), %esi
	cmpl	%ecx, %esi
	jae	.LBB2_19
# %bb.20:
	movq	(%rdi), %r8
	leal	1(%rsi), %r9d
	movl	%r9d, 12(%rdi)
	movzbl	(%r8,%rsi), %esi
	shll	$16, %esi
	jmp	.LBB2_21
.LBB2_19:
	movl	$1, 16(%rdi)
	xorl	%esi, %esi
.LBB2_21:
	movl	12(%rdi), %r8d
	cmpl	%ecx, %r8d
	jae	.LBB2_22
# %bb.23:
	movq	(%rdi), %rcx
	leal	1(%r8), %r9d
	movl	%r9d, 12(%rdi)
	movzbl	(%rcx,%r8), %ecx
	shll	$24, %ecx
	jmp	.LBB2_24
.LBB2_22:
	movl	$1, 16(%rdi)
	xorl	%ecx, %ecx
.LBB2_24:
	orl	%ecx, %esi
	orl	%eax, %edx
	orl	%esi, %edx
	movslq	%edx, %rax
	popq	%rbp
	retq
.LBB2_4:
	movl	$1, 16(%rdi)
	xorl	%eax, %eax
	popq	%rbp
	retq
.Lfunc_end2:
	.size	rd_imm_sext, .Lfunc_end2-rd_imm_sext
                                        # -- End function
	.p2align	4                               # -- Begin function sb_0xhex
	.type	sb_0xhex,@function
sb_0xhex:                               # @sb_0xhex
# %bb.0:
	pushq	%rbp
	movq	%rsp, %rbp
	movb	$48, %dl
	xorl	%eax, %eax
	leaq	.L.str.117(%rip), %rcx
	jmp	.LBB3_1
	.p2align	4
.LBB3_3:                                #   in Loop: Header=BB3_1 Depth=1
	movzbl	1(%rax,%rcx), %edx
	incq	%rax
	cmpq	$2, %rax
	je	.LBB3_4
.LBB3_1:                                # =>This Inner Loop Header: Depth=1
	movl	12(%rdi), %r8d
	leal	1(%r8), %r9d
	cmpl	8(%rdi), %r9d
	jae	.LBB3_3
# %bb.2:                                #   in Loop: Header=BB3_1 Depth=1
	movq	(%rdi), %r10
	movl	%r9d, 12(%rdi)
	movb	%dl, (%r10,%r8)
	movq	(%rdi), %rdx
	movl	12(%rdi), %r8d
	movb	$0, (%rdx,%r8)
	jmp	.LBB3_3
.LBB3_4:
	testq	%rsi, %rsi
	je	.LBB3_10
# %bb.5:
	xorl	%eax, %eax
	movq	%rsi, %rcx
	.p2align	4
.LBB3_6:                                # =>This Inner Loop Header: Depth=1
	movl	%esi, %edx
	andl	$15, %edx
	leal	87(%rdx), %r8d
	leal	48(%rdx), %r9d
	cmpl	$10, %edx
	movzbl	%r9b, %edx
	movzbl	%r8b, %r8d
	cmovbl	%edx, %r8d
	movb	%r8b, -16(%rbp,%rax)
	incq	%rax
	shrq	$4, %rcx
	cmpq	$15, %rsi
	movq	%rcx, %rsi
	ja	.LBB3_6
	jmp	.LBB3_7
	.p2align	4
.LBB3_9:                                #   in Loop: Header=BB3_7 Depth=1
	decq	%rax
	je	.LBB3_12
.LBB3_7:                                # =>This Inner Loop Header: Depth=1
	movl	12(%rdi), %ecx
	leal	1(%rcx), %edx
	cmpl	8(%rdi), %edx
	jae	.LBB3_9
# %bb.8:                                #   in Loop: Header=BB3_7 Depth=1
	movzbl	-17(%rbp,%rax), %esi
	movq	(%rdi), %r8
	movl	%edx, 12(%rdi)
	movb	%sil, (%r8,%rcx)
	movq	(%rdi), %rcx
	movl	12(%rdi), %edx
	movb	$0, (%rcx,%rdx)
	jmp	.LBB3_9
.LBB3_10:
	movl	12(%rdi), %eax
	leal	1(%rax), %ecx
	cmpl	8(%rdi), %ecx
	jae	.LBB3_12
# %bb.11:
	movq	(%rdi), %rdx
	movl	%ecx, 12(%rdi)
	movb	$48, (%rdx,%rax)
	movq	(%rdi), %rax
	movl	12(%rdi), %ecx
	movb	$0, (%rax,%rcx)
.LBB3_12:
	popq	%rbp
	retq
.Lfunc_end3:
	.size	sb_0xhex, .Lfunc_end3-sb_0xhex
                                        # -- End function
	.p2align	4                               # -- Begin function render_rm
	.type	render_rm,@function
render_rm:                              # @render_rm
# %bb.0:
	pushq	%rbp
	movq	%rsp, %rbp
	cmpl	$0, (%rsi)
	je	.LBB4_22
# %bb.1:
	movl	8(%rsi), %eax
	testl	%r9d, %r9d
	je	.LBB4_8
# %bb.2:
	andl	$15, %eax
	leaq	XMM(%rip), %rcx
	movq	(%rcx,%rax,8), %rax
	movzbl	(%rax), %ecx
	testb	%cl, %cl
	je	.LBB4_7
# %bb.3:
	incq	%rax
	jmp	.LBB4_4
	.p2align	4
.LBB4_6:                                #   in Loop: Header=BB4_4 Depth=1
	movzbl	(%rax), %ecx
	incq	%rax
	testb	%cl, %cl
	je	.LBB4_7
.LBB4_4:                                # =>This Inner Loop Header: Depth=1
	movl	12(%rdi), %edx
	leal	1(%rdx), %esi
	cmpl	8(%rdi), %esi
	jae	.LBB4_6
# %bb.5:                                #   in Loop: Header=BB4_4 Depth=1
	movq	(%rdi), %r8
	movl	%esi, 12(%rdi)
	movb	%cl, (%r8,%rdx)
	movq	(%rdi), %rcx
	movl	12(%rdi), %edx
	movb	$0, (%rcx,%rdx)
	jmp	.LBB4_6
.LBB4_22:
	cmpl	$1, %r8d
	jg	.LBB4_26
# %bb.23:
	testl	%r8d, %r8d
	je	.LBB4_36
# %bb.24:
	cmpl	$1, %r8d
	jne	.LBB4_30
# %bb.25:
	leaq	.L.str.118(%rip), %rax
	jmp	.LBB4_31
.LBB4_8:
	cmpl	$2, %edx
	je	.LBB4_13
# %bb.9:
	cmpl	$4, %edx
	je	.LBB4_12
# %bb.10:
	cmpl	$8, %edx
	jne	.LBB4_14
# %bb.11:
	andl	$15, %eax
	leaq	R64(%rip), %rcx
	jmp	.LBB4_17
.LBB4_26:
	cmpl	$4, %r8d
	je	.LBB4_29
# %bb.27:
	cmpl	$2, %r8d
	jne	.LBB4_30
# %bb.28:
	leaq	.L.str.119(%rip), %rax
	jmp	.LBB4_31
.LBB4_30:
	leaq	.L.str.121(%rip), %rax
	jmp	.LBB4_31
.LBB4_29:
	leaq	.L.str.120(%rip), %rax
.LBB4_31:
	movzbl	(%rax), %ecx
	testb	%cl, %cl
	je	.LBB4_36
# %bb.32:
	incq	%rax
	jmp	.LBB4_33
	.p2align	4
.LBB4_35:                               #   in Loop: Header=BB4_33 Depth=1
	movzbl	(%rax), %ecx
	incq	%rax
	testb	%cl, %cl
	je	.LBB4_36
.LBB4_33:                               # =>This Inner Loop Header: Depth=1
	movl	12(%rdi), %edx
	leal	1(%rdx), %r8d
	cmpl	8(%rdi), %r8d
	jae	.LBB4_35
# %bb.34:                               #   in Loop: Header=BB4_33 Depth=1
	movq	(%rdi), %r9
	movl	%r8d, 12(%rdi)
	movb	%cl, (%r9,%rdx)
	movq	(%rdi), %rcx
	movl	12(%rdi), %edx
	movb	$0, (%rcx,%rdx)
	jmp	.LBB4_35
.LBB4_36:
	popq	%rbp
	jmp	render_mem                      # TAILCALL
.LBB4_12:
	andl	$15, %eax
	leaq	R32(%rip), %rcx
	jmp	.LBB4_17
.LBB4_13:
	andl	$15, %eax
	leaq	R16(%rip), %rcx
	jmp	.LBB4_17
.LBB4_14:
	testl	%ecx, %ecx
	je	.LBB4_16
# %bb.15:
	andl	$15, %eax
	leaq	R8L(%rip), %rcx
	jmp	.LBB4_17
.LBB4_16:
	andl	$7, %eax
	leaq	R8H(%rip), %rcx
.LBB4_17:
	leaq	(%rcx,%rax,8), %rax
	movq	(%rax), %rax
	movzbl	(%rax), %ecx
	testb	%cl, %cl
	je	.LBB4_7
# %bb.18:
	incq	%rax
	jmp	.LBB4_19
	.p2align	4
.LBB4_21:                               #   in Loop: Header=BB4_19 Depth=1
	movzbl	(%rax), %ecx
	incq	%rax
	testb	%cl, %cl
	je	.LBB4_7
.LBB4_19:                               # =>This Inner Loop Header: Depth=1
	movl	12(%rdi), %edx
	leal	1(%rdx), %esi
	cmpl	8(%rdi), %esi
	jae	.LBB4_21
# %bb.20:                               #   in Loop: Header=BB4_19 Depth=1
	movq	(%rdi), %r8
	movl	%esi, 12(%rdi)
	movb	%cl, (%r8,%rdx)
	movq	(%rdi), %rcx
	movl	12(%rdi), %edx
	movb	$0, (%rcx,%rdx)
	jmp	.LBB4_21
.LBB4_7:
	popq	%rbp
	retq
.Lfunc_end4:
	.size	render_rm, .Lfunc_end4-render_rm
                                        # -- End function
	.p2align	4                               # -- Begin function reg_name
	.type	reg_name,@function
reg_name:                               # @reg_name
# %bb.0:
	pushq	%rbp
	movq	%rsp, %rbp
                                        # kill: def $edi killed $edi def $rdi
	cmpl	$2, %esi
	je	.LBB5_5
# %bb.1:
	cmpl	$4, %esi
	je	.LBB5_4
# %bb.2:
	cmpl	$8, %esi
	jne	.LBB5_6
# %bb.3:
	andl	$15, %edi
	leaq	R64(%rip), %rax
	jmp	.LBB5_9
.LBB5_4:
	andl	$15, %edi
	leaq	R32(%rip), %rax
	jmp	.LBB5_9
.LBB5_5:
	andl	$15, %edi
	leaq	R16(%rip), %rax
	jmp	.LBB5_9
.LBB5_6:
	testl	%edx, %edx
	je	.LBB5_8
# %bb.7:
	andl	$15, %edi
	leaq	R8L(%rip), %rax
	jmp	.LBB5_9
.LBB5_8:
	andl	$7, %edi
	leaq	R8H(%rip), %rax
.LBB5_9:
	leaq	(%rax,%rdi,8), %rax
	movq	(%rax), %rax
	popq	%rbp
	retq
.Lfunc_end5:
	.size	reg_name, .Lfunc_end5-reg_name
                                        # -- End function
	.p2align	4                               # -- Begin function render_mem
	.type	render_mem,@function
render_mem:                             # @render_mem
# %bb.0:
	movl	12(%rdi), %eax
	leal	1(%rax), %ecx
	cmpl	8(%rdi), %ecx
	jae	.LBB6_2
# %bb.1:
	movq	(%rdi), %rdx
	movl	%ecx, 12(%rdi)
	movb	$91, (%rdx,%rax)
	movq	(%rdi), %rax
	movl	12(%rdi), %ecx
	movb	$0, (%rax,%rcx)
.LBB6_2:
	pushq	%rbp
	movq	%rsp, %rbp
	pushq	%rbx
	subq	$24, %rsp
	cmpl	$0, 12(%rsi)
	je	.LBB6_3
# %bb.4:
	movb	$114, %dl
	movl	$1, %eax
	leaq	.L.str.214(%rip), %rcx
	jmp	.LBB6_5
	.p2align	4
.LBB6_7:                                #   in Loop: Header=BB6_5 Depth=1
	movzbl	(%rax,%rcx), %edx
	incq	%rax
	cmpq	$4, %rax
	je	.LBB6_8
.LBB6_5:                                # =>This Inner Loop Header: Depth=1
	movl	12(%rdi), %r8d
	leal	1(%r8), %r9d
	cmpl	8(%rdi), %r9d
	jae	.LBB6_7
# %bb.6:                                #   in Loop: Header=BB6_5 Depth=1
	movq	(%rdi), %r10
	movl	%r9d, 12(%rdi)
	movb	%dl, (%r10,%r8)
	movq	(%rdi), %rdx
	movl	12(%rdi), %r8d
	movb	$0, (%rdx,%r8)
	jmp	.LBB6_7
.LBB6_8:
	xorl	%ecx, %ecx
	cmpl	$0, 16(%rsi)
	jne	.LBB6_10
	jmp	.LBB6_15
.LBB6_3:
	movl	$1, %ecx
	cmpl	$0, 16(%rsi)
	je	.LBB6_15
.LBB6_10:
	movl	20(%rsi), %eax
	andl	$15, %eax
	leaq	R64(%rip), %rcx
	movq	(%rcx,%rax,8), %rax
	movzbl	(%rax), %edx
	xorl	%ecx, %ecx
	testb	%dl, %dl
	je	.LBB6_15
# %bb.11:
	incq	%rax
	jmp	.LBB6_12
	.p2align	4
.LBB6_14:                               #   in Loop: Header=BB6_12 Depth=1
	movzbl	(%rax), %edx
	incq	%rax
	testb	%dl, %dl
	je	.LBB6_15
.LBB6_12:                               # =>This Inner Loop Header: Depth=1
	movl	12(%rdi), %r8d
	leal	1(%r8), %r9d
	cmpl	8(%rdi), %r9d
	jae	.LBB6_14
# %bb.13:                               #   in Loop: Header=BB6_12 Depth=1
	movq	(%rdi), %r10
	movl	%r9d, 12(%rdi)
	movb	%dl, (%r10,%r8)
	movq	(%rdi), %rdx
	movl	12(%rdi), %r8d
	movb	$0, (%rdx,%r8)
	jmp	.LBB6_14
.LBB6_15:
	cmpl	$0, 24(%rsi)
	je	.LBB6_28
# %bb.16:
	testl	%ecx, %ecx
	jne	.LBB6_19
# %bb.17:
	movl	12(%rdi), %eax
	leal	1(%rax), %ecx
	cmpl	8(%rdi), %ecx
	jae	.LBB6_19
# %bb.18:
	movq	(%rdi), %rdx
	movl	%ecx, 12(%rdi)
	movb	$43, (%rdx,%rax)
	movq	(%rdi), %rax
	movl	12(%rdi), %ecx
	movb	$0, (%rax,%rcx)
.LBB6_19:
	movl	28(%rsi), %eax
	andl	$15, %eax
	leaq	R64(%rip), %rcx
	movq	(%rcx,%rax,8), %rax
	movzbl	(%rax), %ecx
	testb	%cl, %cl
	je	.LBB6_24
# %bb.20:
	incq	%rax
	jmp	.LBB6_21
	.p2align	4
.LBB6_23:                               #   in Loop: Header=BB6_21 Depth=1
	movzbl	(%rax), %ecx
	incq	%rax
	testb	%cl, %cl
	je	.LBB6_24
.LBB6_21:                               # =>This Inner Loop Header: Depth=1
	movl	12(%rdi), %edx
	leal	1(%rdx), %r8d
	cmpl	8(%rdi), %r8d
	jae	.LBB6_23
# %bb.22:                               #   in Loop: Header=BB6_21 Depth=1
	movq	(%rdi), %r9
	movl	%r8d, 12(%rdi)
	movb	%cl, (%r9,%rdx)
	movq	(%rdi), %rcx
	movl	12(%rdi), %edx
	movb	$0, (%rcx,%rdx)
	jmp	.LBB6_23
.LBB6_24:
	movl	12(%rdi), %eax
	leal	1(%rax), %ecx
	cmpl	8(%rdi), %ecx
	jae	.LBB6_26
# %bb.25:
	movq	(%rdi), %rdx
	movl	%ecx, 12(%rdi)
	movb	$42, (%rdx,%rax)
	movq	(%rdi), %rax
	movl	12(%rdi), %ecx
	movb	$0, (%rax,%rcx)
.LBB6_26:
	movl	12(%rdi), %eax
	leal	1(%rax), %edx
	xorl	%ecx, %ecx
	cmpl	8(%rdi), %edx
	jae	.LBB6_28
# %bb.27:
	movzbl	32(%rsi), %r8d
	addb	$48, %r8b
	movq	(%rdi), %r9
	movl	%edx, 12(%rdi)
	movb	%r8b, (%r9,%rax)
	movq	(%rdi), %rax
	movl	12(%rdi), %edx
	movb	$0, (%rax,%rdx)
.LBB6_28:
	movq	40(%rsi), %rax
	testq	%rax, %rax
	sete	%dl
	testl	%ecx, %ecx
	sete	%r8b
	testb	%dl, %r8b
	jne	.LBB6_54
# %bb.29:
	testl	%ecx, %ecx
	je	.LBB6_31
# %bb.30:
	movq	%rdi, %rbx
	movq	%rax, %rsi
	callq	sb_0xhex
	movq	%rbx, %rdi
.LBB6_54:
	movl	12(%rdi), %eax
	leal	1(%rax), %ecx
	cmpl	8(%rdi), %ecx
	jae	.LBB6_56
# %bb.55:
	movq	(%rdi), %rdx
	movl	%ecx, 12(%rdi)
	movb	$93, (%rdx,%rax)
	movq	(%rdi), %rax
	movl	12(%rdi), %ecx
	movb	$0, (%rax,%rcx)
.LBB6_56:
	addq	$24, %rsp
	popq	%rbx
	popq	%rbp
	retq
.LBB6_31:
	testq	%rax, %rax
	js	.LBB6_42
# %bb.32:
	movb	$43, %dl
	movl	$1, %eax
	leaq	.L.str.216(%rip), %rcx
	jmp	.LBB6_33
	.p2align	4
.LBB6_35:                               #   in Loop: Header=BB6_33 Depth=1
	movzbl	(%rax,%rcx), %edx
	incq	%rax
	cmpq	$4, %rax
	je	.LBB6_36
.LBB6_33:                               # =>This Inner Loop Header: Depth=1
	movl	12(%rdi), %r8d
	leal	1(%r8), %r9d
	cmpl	8(%rdi), %r9d
	jae	.LBB6_35
# %bb.34:                               #   in Loop: Header=BB6_33 Depth=1
	movq	(%rdi), %r10
	movl	%r9d, 12(%rdi)
	movb	%dl, (%r10,%r8)
	movq	(%rdi), %rdx
	movl	12(%rdi), %r8d
	movb	$0, (%rdx,%r8)
	jmp	.LBB6_35
.LBB6_36:
	movq	40(%rsi), %rcx
	testq	%rcx, %rcx
	je	.LBB6_52
# %bb.37:
	xorl	%eax, %eax
	movq	%rcx, %rdx
	.p2align	4
.LBB6_38:                               # =>This Inner Loop Header: Depth=1
	movl	%ecx, %esi
	andl	$15, %esi
	leal	87(%rsi), %r8d
	leal	48(%rsi), %r9d
	cmpl	$10, %esi
	movzbl	%r9b, %esi
	movzbl	%r8b, %r8d
	cmovbl	%esi, %r8d
	movb	%r8b, -32(%rbp,%rax)
	incq	%rax
	shrq	$4, %rdx
	cmpq	$15, %rcx
	movq	%rdx, %rcx
	ja	.LBB6_38
	jmp	.LBB6_39
	.p2align	4
.LBB6_41:                               #   in Loop: Header=BB6_39 Depth=1
	decq	%rax
	je	.LBB6_54
.LBB6_39:                               # =>This Inner Loop Header: Depth=1
	movl	12(%rdi), %ecx
	leal	1(%rcx), %edx
	cmpl	8(%rdi), %edx
	jae	.LBB6_41
# %bb.40:                               #   in Loop: Header=BB6_39 Depth=1
	movzbl	-33(%rbp,%rax), %esi
	movq	(%rdi), %r8
	movl	%edx, 12(%rdi)
	movb	%sil, (%r8,%rcx)
	movq	(%rdi), %rcx
	movl	12(%rdi), %edx
	movb	$0, (%rcx,%rdx)
	jmp	.LBB6_41
.LBB6_42:
	movb	$45, %dl
	movl	$1, %eax
	leaq	.L.str.215(%rip), %rcx
	jmp	.LBB6_43
	.p2align	4
.LBB6_45:                               #   in Loop: Header=BB6_43 Depth=1
	movzbl	(%rax,%rcx), %edx
	incq	%rax
	cmpq	$4, %rax
	je	.LBB6_46
.LBB6_43:                               # =>This Inner Loop Header: Depth=1
	movl	12(%rdi), %r8d
	leal	1(%r8), %r9d
	cmpl	8(%rdi), %r9d
	jae	.LBB6_45
# %bb.44:                               #   in Loop: Header=BB6_43 Depth=1
	movq	(%rdi), %r10
	movl	%r9d, 12(%rdi)
	movb	%dl, (%r10,%r8)
	movq	(%rdi), %rdx
	movl	12(%rdi), %r8d
	movb	$0, (%rdx,%r8)
	jmp	.LBB6_45
.LBB6_46:
	movq	40(%rsi), %rcx
	testq	%rcx, %rcx
	je	.LBB6_52
# %bb.47:
	negq	%rcx
	xorl	%eax, %eax
	movq	%rcx, %rdx
	.p2align	4
.LBB6_48:                               # =>This Inner Loop Header: Depth=1
	movl	%ecx, %esi
	andl	$15, %esi
	leal	87(%rsi), %r8d
	leal	48(%rsi), %r9d
	cmpl	$10, %esi
	movzbl	%r9b, %esi
	movzbl	%r8b, %r8d
	cmovbl	%esi, %r8d
	movb	%r8b, -32(%rbp,%rax)
	incq	%rax
	shrq	$4, %rdx
	cmpq	$15, %rcx
	movq	%rdx, %rcx
	ja	.LBB6_48
	jmp	.LBB6_49
	.p2align	4
.LBB6_51:                               #   in Loop: Header=BB6_49 Depth=1
	decq	%rax
	je	.LBB6_54
.LBB6_49:                               # =>This Inner Loop Header: Depth=1
	movl	12(%rdi), %ecx
	leal	1(%rcx), %edx
	cmpl	8(%rdi), %edx
	jae	.LBB6_51
# %bb.50:                               #   in Loop: Header=BB6_49 Depth=1
	movzbl	-33(%rbp,%rax), %esi
	movq	(%rdi), %r8
	movl	%edx, 12(%rdi)
	movb	%sil, (%r8,%rcx)
	movq	(%rdi), %rcx
	movl	12(%rdi), %edx
	movb	$0, (%rcx,%rdx)
	jmp	.LBB6_51
.LBB6_52:
	movl	12(%rdi), %eax
	leal	1(%rax), %ecx
	cmpl	8(%rdi), %ecx
	jae	.LBB6_54
# %bb.53:
	movq	(%rdi), %rdx
	movl	%ecx, 12(%rdi)
	movb	$48, (%rdx,%rax)
	movq	(%rdi), %rax
	movl	12(%rdi), %ecx
	movb	$0, (%rax,%rcx)
	jmp	.LBB6_54
.Lfunc_end6:
	.size	render_mem, .Lfunc_end6-render_mem
                                        # -- End function
	.p2align	4                               # -- Begin function rd64
	.type	rd64,@function
rd64:                                   # @rd64
# %bb.0:
	movl	8(%rdi), %eax
	movl	12(%rdi), %ecx
	cmpl	%eax, %ecx
	jae	.LBB7_1
# %bb.2:
	movq	(%rdi), %rdx
	leal	1(%rcx), %esi
	movl	%esi, 12(%rdi)
	movzbl	(%rdx,%rcx), %ecx
	jmp	.LBB7_3
.LBB7_1:
	movl	$1, 16(%rdi)
	xorl	%ecx, %ecx
.LBB7_3:
	movl	12(%rdi), %edx
	cmpl	%eax, %edx
	jae	.LBB7_4
# %bb.5:
	movq	(%rdi), %rsi
	leal	1(%rdx), %r8d
	movl	%r8d, 12(%rdi)
	movzbl	(%rsi,%rdx), %edx
	shll	$8, %edx
	jmp	.LBB7_6
.LBB7_4:
	movl	$1, 16(%rdi)
	xorl	%edx, %edx
.LBB7_6:
	movl	12(%rdi), %esi
	cmpl	%eax, %esi
	jae	.LBB7_7
# %bb.8:
	movq	(%rdi), %r8
	leal	1(%rsi), %r9d
	movl	%r9d, 12(%rdi)
	movzbl	(%r8,%rsi), %esi
	shll	$16, %esi
	jmp	.LBB7_9
.LBB7_7:
	movl	$1, 16(%rdi)
	xorl	%esi, %esi
.LBB7_9:
	movl	12(%rdi), %r8d
	cmpl	%eax, %r8d
	jae	.LBB7_10
# %bb.11:
	movq	(%rdi), %r9
	leal	1(%r8), %r10d
	movl	%r10d, 12(%rdi)
	movzbl	(%r9,%r8), %r8d
	shll	$24, %r8d
	jmp	.LBB7_12
.LBB7_10:
	movl	$1, 16(%rdi)
	xorl	%r8d, %r8d
.LBB7_12:
	movl	12(%rdi), %r9d
	cmpl	%eax, %r9d
	jae	.LBB7_13
# %bb.14:
	movq	(%rdi), %r10
	leal	1(%r9), %r11d
	movl	%r11d, 12(%rdi)
	movzbl	(%r10,%r9), %r9d
	jmp	.LBB7_15
.LBB7_13:
	movl	$1, 16(%rdi)
	xorl	%r9d, %r9d
.LBB7_15:
	pushq	%rbp
	movq	%rsp, %rbp
	pushq	%r14
	pushq	%rbx
	movl	12(%rdi), %r10d
	cmpl	%eax, %r10d
	jae	.LBB7_16
# %bb.17:
	movq	(%rdi), %r11
	leal	1(%r10), %ebx
	movl	%ebx, 12(%rdi)
	movzbl	(%r11,%r10), %r10d
	shll	$8, %r10d
	jmp	.LBB7_18
.LBB7_16:
	movl	$1, 16(%rdi)
	xorl	%r10d, %r10d
.LBB7_18:
	movl	12(%rdi), %r11d
	cmpl	%eax, %r11d
	jae	.LBB7_19
# %bb.20:
	movq	(%rdi), %rbx
	leal	1(%r11), %r14d
	movl	%r14d, 12(%rdi)
	movzbl	(%rbx,%r11), %r11d
	shll	$16, %r11d
	jmp	.LBB7_21
.LBB7_19:
	movl	$1, 16(%rdi)
	xorl	%r11d, %r11d
.LBB7_21:
	movl	12(%rdi), %ebx
	cmpl	%eax, %ebx
	jae	.LBB7_22
# %bb.23:
	movq	(%rdi), %rax
	leal	1(%rbx), %r14d
	movl	%r14d, 12(%rdi)
	movzbl	(%rax,%rbx), %eax
	shll	$24, %eax
	jmp	.LBB7_24
.LBB7_22:
	movl	$1, 16(%rdi)
	xorl	%eax, %eax
.LBB7_24:
	orq	%rcx, %rdx
	orq	%r8, %rsi
	orq	%rdx, %rsi
	orl	%r9d, %r10d
	orl	%r11d, %eax
	orl	%r10d, %eax
	shlq	$32, %rax
	orq	%rsi, %rax
	popq	%rbx
	popq	%r14
	popq	%rbp
	retq
.Lfunc_end7:
	.size	rd64, .Lfunc_end7-rd64
                                        # -- End function
	.type	.L.str,@object                  # @.str
	.section	.rodata.str1.1,"aMS",@progbits,1
.L.str:
	.asciz	"lock "
	.size	.L.str, 6

	.type	.L.str.1,@object                # @.str.1
.L.str.1:
	.asciz	"endbr64"
	.size	.L.str.1, 8

	.type	.L.str.2,@object                # @.str.2
.L.str.2:
	.asciz	"endbr32"
	.size	.L.str.2, 8

	.type	.L.str.3,@object                # @.str.3
.L.str.3:
	.asciz	"nop"
	.size	.L.str.3, 4

	.type	.L.str.4,@object                # @.str.4
.L.str.4:
	.asciz	"syscall"
	.size	.L.str.4, 8

	.type	.L.str.5,@object                # @.str.5
.L.str.5:
	.asciz	"ud2"
	.size	.L.str.5, 4

	.type	.L.str.6,@object                # @.str.6
.L.str.6:
	.asciz	"rdtsc"
	.size	.L.str.6, 6

	.type	.L.str.7,@object                # @.str.7
.L.str.7:
	.asciz	"cpuid"
	.size	.L.str.7, 6

	.type	CC,@object                      # @CC
	.section	.data.rel.ro,"aw",@progbits
	.p2align	4, 0x0
CC:
	.quad	.L.str.101
	.quad	.L.str.102
	.quad	.L.str.103
	.quad	.L.str.104
	.quad	.L.str.105
	.quad	.L.str.106
	.quad	.L.str.107
	.quad	.L.str.108
	.quad	.L.str.109
	.quad	.L.str.110
	.quad	.L.str.111
	.quad	.L.str.112
	.quad	.L.str.113
	.quad	.L.str.114
	.quad	.L.str.115
	.quad	.L.str.116
	.size	CC, 128

	.type	.L.str.8,@object                # @.str.8
	.section	.rodata.str1.1,"aMS",@progbits,1
.L.str.8:
	.asciz	"set"
	.size	.L.str.8, 4

	.type	.L.str.9,@object                # @.str.9
.L.str.9:
	.asciz	"cmov"
	.size	.L.str.9, 5

	.type	.L.str.10,@object               # @.str.10
.L.str.10:
	.asciz	", "
	.size	.L.str.10, 3

	.type	.L.str.11,@object               # @.str.11
.L.str.11:
	.asciz	"movsx "
	.size	.L.str.11, 7

	.type	.L.str.12,@object               # @.str.12
.L.str.12:
	.asciz	"movzx "
	.size	.L.str.12, 7

	.type	.L.str.13,@object               # @.str.13
.L.str.13:
	.asciz	"imul "
	.size	.L.str.13, 6

	.type	.L.str.14,@object               # @.str.14
.L.str.14:
	.asciz	"(sse)"
	.size	.L.str.14, 6

	.type	.L.str.15,@object               # @.str.15
.L.str.15:
	.asciz	"movss"
	.size	.L.str.15, 6

	.type	.L.str.16,@object               # @.str.16
.L.str.16:
	.asciz	"movsd"
	.size	.L.str.16, 6

	.type	.L.str.17,@object               # @.str.17
.L.str.17:
	.asciz	"movups"
	.size	.L.str.17, 7

	.type	.L.str.18,@object               # @.str.18
.L.str.18:
	.asciz	"movaps"
	.size	.L.str.18, 7

	.type	.L.str.19,@object               # @.str.19
.L.str.19:
	.asciz	"cvtsi2ss"
	.size	.L.str.19, 9

	.type	.L.str.20,@object               # @.str.20
.L.str.20:
	.asciz	"cvtsi2sd"
	.size	.L.str.20, 9

	.type	.L.str.21,@object               # @.str.21
.L.str.21:
	.asciz	"cvtpi2ps"
	.size	.L.str.21, 9

	.type	.L.str.22,@object               # @.str.22
.L.str.22:
	.asciz	"cvttss2si"
	.size	.L.str.22, 10

	.type	.L.str.23,@object               # @.str.23
.L.str.23:
	.asciz	"cvttsd2si"
	.size	.L.str.23, 10

	.type	.L.str.24,@object               # @.str.24
.L.str.24:
	.asciz	"ucomiss"
	.size	.L.str.24, 8

	.type	.L.str.25,@object               # @.str.25
.L.str.25:
	.asciz	"comiss"
	.size	.L.str.25, 7

	.type	.L.str.26,@object               # @.str.26
.L.str.26:
	.asciz	"xorps"
	.size	.L.str.26, 6

	.type	.L.str.27,@object               # @.str.27
.L.str.27:
	.asciz	"andps"
	.size	.L.str.27, 6

	.type	.L.str.28,@object               # @.str.28
.L.str.28:
	.asciz	"addss"
	.size	.L.str.28, 6

	.type	.L.str.29,@object               # @.str.29
.L.str.29:
	.asciz	"addsd"
	.size	.L.str.29, 6

	.type	.L.str.30,@object               # @.str.30
.L.str.30:
	.asciz	"addps"
	.size	.L.str.30, 6

	.type	.L.str.31,@object               # @.str.31
.L.str.31:
	.asciz	"mulss"
	.size	.L.str.31, 6

	.type	.L.str.32,@object               # @.str.32
.L.str.32:
	.asciz	"mulsd"
	.size	.L.str.32, 6

	.type	.L.str.33,@object               # @.str.33
.L.str.33:
	.asciz	"mulps"
	.size	.L.str.33, 6

	.type	.L.str.34,@object               # @.str.34
.L.str.34:
	.asciz	"subss"
	.size	.L.str.34, 6

	.type	.L.str.35,@object               # @.str.35
.L.str.35:
	.asciz	"subsd"
	.size	.L.str.35, 6

	.type	.L.str.36,@object               # @.str.36
.L.str.36:
	.asciz	"subps"
	.size	.L.str.36, 6

	.type	.L.str.37,@object               # @.str.37
.L.str.37:
	.asciz	"divss"
	.size	.L.str.37, 6

	.type	.L.str.38,@object               # @.str.38
.L.str.38:
	.asciz	"divsd"
	.size	.L.str.38, 6

	.type	.L.str.39,@object               # @.str.39
.L.str.39:
	.asciz	"divps"
	.size	.L.str.39, 6

	.type	.L.str.40,@object               # @.str.40
.L.str.40:
	.asciz	"cvtss2sd"
	.size	.L.str.40, 9

	.type	.L.str.41,@object               # @.str.41
.L.str.41:
	.asciz	"cvtsd2ss"
	.size	.L.str.41, 9

	.type	.L.str.42,@object               # @.str.42
.L.str.42:
	.asciz	"cvtps2pd"
	.size	.L.str.42, 9

	.type	.L.str.43,@object               # @.str.43
.L.str.43:
	.asciz	"movd"
	.size	.L.str.43, 5

	.type	.L.str.44,@object               # @.str.44
.L.str.44:
	.asciz	"movq"
	.size	.L.str.44, 5

	.type	.L.str.45,@object               # @.str.45
.L.str.45:
	.asciz	"movdqu"
	.size	.L.str.45, 7

	.type	.L.str.46,@object               # @.str.46
.L.str.46:
	.asciz	"movdqa"
	.size	.L.str.46, 7

	.type	.L.str.47,@object               # @.str.47
.L.str.47:
	.asciz	"pxor"
	.size	.L.str.47, 5

	.type	XMM,@object                     # @XMM
	.section	.data.rel.ro,"aw",@progbits
	.p2align	4, 0x0
XMM:
	.quad	.L.str.174
	.quad	.L.str.175
	.quad	.L.str.176
	.quad	.L.str.177
	.quad	.L.str.178
	.quad	.L.str.179
	.quad	.L.str.180
	.quad	.L.str.181
	.quad	.L.str.182
	.quad	.L.str.183
	.quad	.L.str.184
	.quad	.L.str.185
	.quad	.L.str.186
	.quad	.L.str.187
	.quad	.L.str.188
	.quad	.L.str.189
	.size	XMM, 128

	.type	.L.str.48,@object               # @.str.48
	.section	.rodata.str1.1,"aMS",@progbits,1
.L.str.48:
	.asciz	"(bad)"
	.size	.L.str.48, 6

	.type	ALU,@object                     # @ALU
	.section	.data.rel.ro,"aw",@progbits
	.p2align	4, 0x0
ALU:
	.quad	.L.str.190
	.quad	.L.str.191
	.quad	.L.str.192
	.quad	.L.str.193
	.quad	.L.str.194
	.quad	.L.str.195
	.quad	.L.str.196
	.quad	.L.str.197
	.size	ALU, 64

	.type	.L.str.49,@object               # @.str.49
	.section	.rodata.str1.1,"aMS",@progbits,1
.L.str.49:
	.asciz	" al, "
	.size	.L.str.49, 6

	.type	.L.str.50,@object               # @.str.50
.L.str.50:
	.asciz	"push "
	.size	.L.str.50, 6

	.type	R64,@object                     # @R64
	.section	.data.rel.ro,"aw",@progbits
	.p2align	4, 0x0
R64:
	.quad	.L.str.198
	.quad	.L.str.199
	.quad	.L.str.200
	.quad	.L.str.201
	.quad	.L.str.202
	.quad	.L.str.203
	.quad	.L.str.204
	.quad	.L.str.205
	.quad	.L.str.206
	.quad	.L.str.207
	.quad	.L.str.208
	.quad	.L.str.209
	.quad	.L.str.210
	.quad	.L.str.211
	.quad	.L.str.212
	.quad	.L.str.213
	.size	R64, 128

	.type	.L.str.51,@object               # @.str.51
	.section	.rodata.str1.1,"aMS",@progbits,1
.L.str.51:
	.asciz	"pop "
	.size	.L.str.51, 5

	.type	.L.str.52,@object               # @.str.52
.L.str.52:
	.asciz	"movsxd "
	.size	.L.str.52, 8

	.type	.L.str.53,@object               # @.str.53
.L.str.53:
	.asciz	"test "
	.size	.L.str.53, 6

	.type	.L.str.54,@object               # @.str.54
.L.str.54:
	.asciz	"xchg "
	.size	.L.str.54, 6

	.type	.L.str.55,@object               # @.str.55
.L.str.55:
	.asciz	"mov "
	.size	.L.str.55, 5

	.type	.L.str.56,@object               # @.str.56
.L.str.56:
	.asciz	"lea "
	.size	.L.str.56, 5

	.type	.L.str.57,@object               # @.str.57
.L.str.57:
	.asciz	"pause"
	.size	.L.str.57, 6

	.type	.L.str.58,@object               # @.str.58
.L.str.58:
	.asciz	"cdqe"
	.size	.L.str.58, 5

	.type	.L.str.59,@object               # @.str.59
.L.str.59:
	.asciz	"cbw"
	.size	.L.str.59, 4

	.type	.L.str.60,@object               # @.str.60
.L.str.60:
	.asciz	"cwde"
	.size	.L.str.60, 5

	.type	.L.str.61,@object               # @.str.61
.L.str.61:
	.asciz	"cqo"
	.size	.L.str.61, 4

	.type	.L.str.62,@object               # @.str.62
.L.str.62:
	.asciz	"cwd"
	.size	.L.str.62, 4

	.type	.L.str.63,@object               # @.str.63
.L.str.63:
	.asciz	"cdq"
	.size	.L.str.63, 4

	.type	.L.str.64,@object               # @.str.64
.L.str.64:
	.asciz	"rep "
	.size	.L.str.64, 5

	.type	.L.str.65,@object               # @.str.65
.L.str.65:
	.asciz	"repnz "
	.size	.L.str.65, 7

	.type	.L.str.66,@object               # @.str.66
.L.str.66:
	.asciz	"movs"
	.size	.L.str.66, 5

	.type	.L.str.67,@object               # @.str.67
.L.str.67:
	.asciz	"cmps"
	.size	.L.str.67, 5

	.type	.L.str.68,@object               # @.str.68
.L.str.68:
	.asciz	"stos"
	.size	.L.str.68, 5

	.type	.L.str.69,@object               # @.str.69
.L.str.69:
	.asciz	"lods"
	.size	.L.str.69, 5

	.type	.L.str.70,@object               # @.str.70
.L.str.70:
	.asciz	"scas"
	.size	.L.str.70, 5

	.type	.L.str.71,@object               # @.str.71
.L.str.71:
	.asciz	"movabs "
	.size	.L.str.71, 8

	.type	SHIFT,@object                   # @SHIFT
	.section	.data.rel.ro,"aw",@progbits
	.p2align	4, 0x0
SHIFT:
	.quad	.L.str.217
	.quad	.L.str.218
	.quad	.L.str.219
	.quad	.L.str.220
	.quad	.L.str.221
	.quad	.L.str.222
	.quad	.L.str.223
	.quad	.L.str.224
	.size	SHIFT, 64

	.type	.L.str.72,@object               # @.str.72
	.section	.rodata.str1.1,"aMS",@progbits,1
.L.str.72:
	.asciz	", 1"
	.size	.L.str.72, 4

	.type	.L.str.73,@object               # @.str.73
.L.str.73:
	.asciz	", cl"
	.size	.L.str.73, 5

	.type	.L.str.74,@object               # @.str.74
.L.str.74:
	.asciz	"ret "
	.size	.L.str.74, 5

	.type	.L.str.75,@object               # @.str.75
.L.str.75:
	.asciz	"ret"
	.size	.L.str.75, 4

	.type	.L.str.76,@object               # @.str.76
.L.str.76:
	.asciz	"leave"
	.size	.L.str.76, 6

	.type	.L.str.77,@object               # @.str.77
.L.str.77:
	.asciz	"int3"
	.size	.L.str.77, 5

	.type	.L.str.78,@object               # @.str.78
.L.str.78:
	.asciz	"int "
	.size	.L.str.78, 5

	.type	.L.str.79,@object               # @.str.79
.L.str.79:
	.asciz	"call "
	.size	.L.str.79, 6

	.type	.L.str.80,@object               # @.str.80
.L.str.80:
	.asciz	"jmp "
	.size	.L.str.80, 5

	.type	.L.str.81,@object               # @.str.81
.L.str.81:
	.asciz	"hlt"
	.size	.L.str.81, 4

	.type	.L.str.82,@object               # @.str.82
.L.str.82:
	.asciz	"cmc"
	.size	.L.str.82, 4

	.type	x86_decode.G3,@object           # @x86_decode.G3
	.section	.data.rel.ro,"aw",@progbits
	.p2align	4, 0x0
x86_decode.G3:
	.quad	.L.str.83
	.quad	.L.str.83
	.quad	.L.str.84
	.quad	.L.str.85
	.quad	.L.str.86
	.quad	.L.str.87
	.quad	.L.str.88
	.quad	.L.str.89
	.size	x86_decode.G3, 64

	.type	.L.str.83,@object               # @.str.83
	.section	.rodata.str1.1,"aMS",@progbits,1
.L.str.83:
	.asciz	"test"
	.size	.L.str.83, 5

	.type	.L.str.84,@object               # @.str.84
.L.str.84:
	.asciz	"not"
	.size	.L.str.84, 4

	.type	.L.str.85,@object               # @.str.85
.L.str.85:
	.asciz	"neg"
	.size	.L.str.85, 4

	.type	.L.str.86,@object               # @.str.86
.L.str.86:
	.asciz	"mul"
	.size	.L.str.86, 4

	.type	.L.str.87,@object               # @.str.87
.L.str.87:
	.asciz	"imul"
	.size	.L.str.87, 5

	.type	.L.str.88,@object               # @.str.88
.L.str.88:
	.asciz	"div"
	.size	.L.str.88, 4

	.type	.L.str.89,@object               # @.str.89
.L.str.89:
	.asciz	"idiv"
	.size	.L.str.89, 5

	.type	.L.str.90,@object               # @.str.90
.L.str.90:
	.asciz	"inc "
	.size	.L.str.90, 5

	.type	.L.str.91,@object               # @.str.91
.L.str.91:
	.asciz	"dec "
	.size	.L.str.91, 5

	.type	.L.str.92,@object               # @.str.92
.L.str.92:
	.asciz	"(bad) "
	.size	.L.str.92, 7

	.type	x86_decode.G5,@object           # @x86_decode.G5
	.section	.data.rel.ro,"aw",@progbits
	.p2align	4, 0x0
x86_decode.G5:
	.quad	.L.str.93
	.quad	.L.str.94
	.quad	.L.str.95
	.quad	.L.str.96
	.quad	.L.str.97
	.quad	.L.str.98
	.quad	.L.str.99
	.quad	.L.str.48
	.size	x86_decode.G5, 64

	.type	.L.str.93,@object               # @.str.93
	.section	.rodata.str1.1,"aMS",@progbits,1
.L.str.93:
	.asciz	"inc"
	.size	.L.str.93, 4

	.type	.L.str.94,@object               # @.str.94
.L.str.94:
	.asciz	"dec"
	.size	.L.str.94, 4

	.type	.L.str.95,@object               # @.str.95
.L.str.95:
	.asciz	"call"
	.size	.L.str.95, 5

	.type	.L.str.96,@object               # @.str.96
.L.str.96:
	.asciz	"callf"
	.size	.L.str.96, 6

	.type	.L.str.97,@object               # @.str.97
.L.str.97:
	.asciz	"jmp"
	.size	.L.str.97, 4

	.type	.L.str.98,@object               # @.str.98
.L.str.98:
	.asciz	"jmpf"
	.size	.L.str.98, 5

	.type	.L.str.99,@object               # @.str.99
.L.str.99:
	.asciz	"push"
	.size	.L.str.99, 5

	.type	.L.str.100,@object              # @.str.100
.L.str.100:
	.asciz	"        # "
	.size	.L.str.100, 11

	.type	.L.str.101,@object              # @.str.101
.L.str.101:
	.asciz	"o"
	.size	.L.str.101, 2

	.type	.L.str.102,@object              # @.str.102
.L.str.102:
	.asciz	"no"
	.size	.L.str.102, 3

	.type	.L.str.103,@object              # @.str.103
.L.str.103:
	.asciz	"b"
	.size	.L.str.103, 2

	.type	.L.str.104,@object              # @.str.104
.L.str.104:
	.asciz	"ae"
	.size	.L.str.104, 3

	.type	.L.str.105,@object              # @.str.105
.L.str.105:
	.asciz	"e"
	.size	.L.str.105, 2

	.type	.L.str.106,@object              # @.str.106
.L.str.106:
	.asciz	"ne"
	.size	.L.str.106, 3

	.type	.L.str.107,@object              # @.str.107
.L.str.107:
	.asciz	"be"
	.size	.L.str.107, 3

	.type	.L.str.108,@object              # @.str.108
.L.str.108:
	.asciz	"a"
	.size	.L.str.108, 2

	.type	.L.str.109,@object              # @.str.109
.L.str.109:
	.asciz	"s"
	.size	.L.str.109, 2

	.type	.L.str.110,@object              # @.str.110
.L.str.110:
	.asciz	"ns"
	.size	.L.str.110, 3

	.type	.L.str.111,@object              # @.str.111
.L.str.111:
	.asciz	"p"
	.size	.L.str.111, 2

	.type	.L.str.112,@object              # @.str.112
.L.str.112:
	.asciz	"np"
	.size	.L.str.112, 3

	.type	.L.str.113,@object              # @.str.113
.L.str.113:
	.asciz	"l"
	.size	.L.str.113, 2

	.type	.L.str.114,@object              # @.str.114
.L.str.114:
	.asciz	"ge"
	.size	.L.str.114, 3

	.type	.L.str.115,@object              # @.str.115
.L.str.115:
	.asciz	"le"
	.size	.L.str.115, 3

	.type	.L.str.116,@object              # @.str.116
.L.str.116:
	.asciz	"g"
	.size	.L.str.116, 2

	.type	.L.str.117,@object              # @.str.117
.L.str.117:
	.asciz	"0x"
	.size	.L.str.117, 3

	.type	.L.str.118,@object              # @.str.118
.L.str.118:
	.asciz	"BYTE PTR "
	.size	.L.str.118, 10

	.type	.L.str.119,@object              # @.str.119
.L.str.119:
	.asciz	"WORD PTR "
	.size	.L.str.119, 10

	.type	.L.str.120,@object              # @.str.120
.L.str.120:
	.asciz	"DWORD PTR "
	.size	.L.str.120, 11

	.type	.L.str.121,@object              # @.str.121
.L.str.121:
	.asciz	"QWORD PTR "
	.size	.L.str.121, 11

	.type	R32,@object                     # @R32
	.section	.data.rel.ro,"aw",@progbits
	.p2align	4, 0x0
R32:
	.quad	.L.str.122
	.quad	.L.str.123
	.quad	.L.str.124
	.quad	.L.str.125
	.quad	.L.str.126
	.quad	.L.str.127
	.quad	.L.str.128
	.quad	.L.str.129
	.quad	.L.str.130
	.quad	.L.str.131
	.quad	.L.str.132
	.quad	.L.str.133
	.quad	.L.str.134
	.quad	.L.str.135
	.quad	.L.str.136
	.quad	.L.str.137
	.size	R32, 128

	.type	R16,@object                     # @R16
	.p2align	4, 0x0
R16:
	.quad	.L.str.138
	.quad	.L.str.139
	.quad	.L.str.140
	.quad	.L.str.141
	.quad	.L.str.142
	.quad	.L.str.143
	.quad	.L.str.144
	.quad	.L.str.145
	.quad	.L.str.146
	.quad	.L.str.147
	.quad	.L.str.148
	.quad	.L.str.149
	.quad	.L.str.150
	.quad	.L.str.151
	.quad	.L.str.152
	.quad	.L.str.153
	.size	R16, 128

	.type	R8L,@object                     # @R8L
	.p2align	4, 0x0
R8L:
	.quad	.L.str.154
	.quad	.L.str.155
	.quad	.L.str.156
	.quad	.L.str.157
	.quad	.L.str.158
	.quad	.L.str.159
	.quad	.L.str.160
	.quad	.L.str.161
	.quad	.L.str.162
	.quad	.L.str.163
	.quad	.L.str.164
	.quad	.L.str.165
	.quad	.L.str.166
	.quad	.L.str.167
	.quad	.L.str.168
	.quad	.L.str.169
	.size	R8L, 128

	.type	R8H,@object                     # @R8H
	.p2align	4, 0x0
R8H:
	.quad	.L.str.154
	.quad	.L.str.155
	.quad	.L.str.156
	.quad	.L.str.157
	.quad	.L.str.170
	.quad	.L.str.171
	.quad	.L.str.172
	.quad	.L.str.173
	.size	R8H, 64

	.type	.L.str.122,@object              # @.str.122
	.section	.rodata.str1.1,"aMS",@progbits,1
.L.str.122:
	.asciz	"eax"
	.size	.L.str.122, 4

	.type	.L.str.123,@object              # @.str.123
.L.str.123:
	.asciz	"ecx"
	.size	.L.str.123, 4

	.type	.L.str.124,@object              # @.str.124
.L.str.124:
	.asciz	"edx"
	.size	.L.str.124, 4

	.type	.L.str.125,@object              # @.str.125
.L.str.125:
	.asciz	"ebx"
	.size	.L.str.125, 4

	.type	.L.str.126,@object              # @.str.126
.L.str.126:
	.asciz	"esp"
	.size	.L.str.126, 4

	.type	.L.str.127,@object              # @.str.127
.L.str.127:
	.asciz	"ebp"
	.size	.L.str.127, 4

	.type	.L.str.128,@object              # @.str.128
.L.str.128:
	.asciz	"esi"
	.size	.L.str.128, 4

	.type	.L.str.129,@object              # @.str.129
.L.str.129:
	.asciz	"edi"
	.size	.L.str.129, 4

	.type	.L.str.130,@object              # @.str.130
.L.str.130:
	.asciz	"r8d"
	.size	.L.str.130, 4

	.type	.L.str.131,@object              # @.str.131
.L.str.131:
	.asciz	"r9d"
	.size	.L.str.131, 4

	.type	.L.str.132,@object              # @.str.132
.L.str.132:
	.asciz	"r10d"
	.size	.L.str.132, 5

	.type	.L.str.133,@object              # @.str.133
.L.str.133:
	.asciz	"r11d"
	.size	.L.str.133, 5

	.type	.L.str.134,@object              # @.str.134
.L.str.134:
	.asciz	"r12d"
	.size	.L.str.134, 5

	.type	.L.str.135,@object              # @.str.135
.L.str.135:
	.asciz	"r13d"
	.size	.L.str.135, 5

	.type	.L.str.136,@object              # @.str.136
.L.str.136:
	.asciz	"r14d"
	.size	.L.str.136, 5

	.type	.L.str.137,@object              # @.str.137
.L.str.137:
	.asciz	"r15d"
	.size	.L.str.137, 5

	.type	.L.str.138,@object              # @.str.138
.L.str.138:
	.asciz	"ax"
	.size	.L.str.138, 3

	.type	.L.str.139,@object              # @.str.139
.L.str.139:
	.asciz	"cx"
	.size	.L.str.139, 3

	.type	.L.str.140,@object              # @.str.140
.L.str.140:
	.asciz	"dx"
	.size	.L.str.140, 3

	.type	.L.str.141,@object              # @.str.141
.L.str.141:
	.asciz	"bx"
	.size	.L.str.141, 3

	.type	.L.str.142,@object              # @.str.142
.L.str.142:
	.asciz	"sp"
	.size	.L.str.142, 3

	.type	.L.str.143,@object              # @.str.143
.L.str.143:
	.asciz	"bp"
	.size	.L.str.143, 3

	.type	.L.str.144,@object              # @.str.144
.L.str.144:
	.asciz	"si"
	.size	.L.str.144, 3

	.type	.L.str.145,@object              # @.str.145
.L.str.145:
	.asciz	"di"
	.size	.L.str.145, 3

	.type	.L.str.146,@object              # @.str.146
.L.str.146:
	.asciz	"r8w"
	.size	.L.str.146, 4

	.type	.L.str.147,@object              # @.str.147
.L.str.147:
	.asciz	"r9w"
	.size	.L.str.147, 4

	.type	.L.str.148,@object              # @.str.148
.L.str.148:
	.asciz	"r10w"
	.size	.L.str.148, 5

	.type	.L.str.149,@object              # @.str.149
.L.str.149:
	.asciz	"r11w"
	.size	.L.str.149, 5

	.type	.L.str.150,@object              # @.str.150
.L.str.150:
	.asciz	"r12w"
	.size	.L.str.150, 5

	.type	.L.str.151,@object              # @.str.151
.L.str.151:
	.asciz	"r13w"
	.size	.L.str.151, 5

	.type	.L.str.152,@object              # @.str.152
.L.str.152:
	.asciz	"r14w"
	.size	.L.str.152, 5

	.type	.L.str.153,@object              # @.str.153
.L.str.153:
	.asciz	"r15w"
	.size	.L.str.153, 5

	.type	.L.str.154,@object              # @.str.154
.L.str.154:
	.asciz	"al"
	.size	.L.str.154, 3

	.type	.L.str.155,@object              # @.str.155
.L.str.155:
	.asciz	"cl"
	.size	.L.str.155, 3

	.type	.L.str.156,@object              # @.str.156
.L.str.156:
	.asciz	"dl"
	.size	.L.str.156, 3

	.type	.L.str.157,@object              # @.str.157
.L.str.157:
	.asciz	"bl"
	.size	.L.str.157, 3

	.type	.L.str.158,@object              # @.str.158
.L.str.158:
	.asciz	"spl"
	.size	.L.str.158, 4

	.type	.L.str.159,@object              # @.str.159
.L.str.159:
	.asciz	"bpl"
	.size	.L.str.159, 4

	.type	.L.str.160,@object              # @.str.160
.L.str.160:
	.asciz	"sil"
	.size	.L.str.160, 4

	.type	.L.str.161,@object              # @.str.161
.L.str.161:
	.asciz	"dil"
	.size	.L.str.161, 4

	.type	.L.str.162,@object              # @.str.162
.L.str.162:
	.asciz	"r8b"
	.size	.L.str.162, 4

	.type	.L.str.163,@object              # @.str.163
.L.str.163:
	.asciz	"r9b"
	.size	.L.str.163, 4

	.type	.L.str.164,@object              # @.str.164
.L.str.164:
	.asciz	"r10b"
	.size	.L.str.164, 5

	.type	.L.str.165,@object              # @.str.165
.L.str.165:
	.asciz	"r11b"
	.size	.L.str.165, 5

	.type	.L.str.166,@object              # @.str.166
.L.str.166:
	.asciz	"r12b"
	.size	.L.str.166, 5

	.type	.L.str.167,@object              # @.str.167
.L.str.167:
	.asciz	"r13b"
	.size	.L.str.167, 5

	.type	.L.str.168,@object              # @.str.168
.L.str.168:
	.asciz	"r14b"
	.size	.L.str.168, 5

	.type	.L.str.169,@object              # @.str.169
.L.str.169:
	.asciz	"r15b"
	.size	.L.str.169, 5

	.type	.L.str.170,@object              # @.str.170
.L.str.170:
	.asciz	"ah"
	.size	.L.str.170, 3

	.type	.L.str.171,@object              # @.str.171
.L.str.171:
	.asciz	"ch"
	.size	.L.str.171, 3

	.type	.L.str.172,@object              # @.str.172
.L.str.172:
	.asciz	"dh"
	.size	.L.str.172, 3

	.type	.L.str.173,@object              # @.str.173
.L.str.173:
	.asciz	"bh"
	.size	.L.str.173, 3

	.type	.L.str.174,@object              # @.str.174
.L.str.174:
	.asciz	"xmm0"
	.size	.L.str.174, 5

	.type	.L.str.175,@object              # @.str.175
.L.str.175:
	.asciz	"xmm1"
	.size	.L.str.175, 5

	.type	.L.str.176,@object              # @.str.176
.L.str.176:
	.asciz	"xmm2"
	.size	.L.str.176, 5

	.type	.L.str.177,@object              # @.str.177
.L.str.177:
	.asciz	"xmm3"
	.size	.L.str.177, 5

	.type	.L.str.178,@object              # @.str.178
.L.str.178:
	.asciz	"xmm4"
	.size	.L.str.178, 5

	.type	.L.str.179,@object              # @.str.179
.L.str.179:
	.asciz	"xmm5"
	.size	.L.str.179, 5

	.type	.L.str.180,@object              # @.str.180
.L.str.180:
	.asciz	"xmm6"
	.size	.L.str.180, 5

	.type	.L.str.181,@object              # @.str.181
.L.str.181:
	.asciz	"xmm7"
	.size	.L.str.181, 5

	.type	.L.str.182,@object              # @.str.182
.L.str.182:
	.asciz	"xmm8"
	.size	.L.str.182, 5

	.type	.L.str.183,@object              # @.str.183
.L.str.183:
	.asciz	"xmm9"
	.size	.L.str.183, 5

	.type	.L.str.184,@object              # @.str.184
.L.str.184:
	.asciz	"xmm10"
	.size	.L.str.184, 6

	.type	.L.str.185,@object              # @.str.185
.L.str.185:
	.asciz	"xmm11"
	.size	.L.str.185, 6

	.type	.L.str.186,@object              # @.str.186
.L.str.186:
	.asciz	"xmm12"
	.size	.L.str.186, 6

	.type	.L.str.187,@object              # @.str.187
.L.str.187:
	.asciz	"xmm13"
	.size	.L.str.187, 6

	.type	.L.str.188,@object              # @.str.188
.L.str.188:
	.asciz	"xmm14"
	.size	.L.str.188, 6

	.type	.L.str.189,@object              # @.str.189
.L.str.189:
	.asciz	"xmm15"
	.size	.L.str.189, 6

	.type	.L.str.190,@object              # @.str.190
.L.str.190:
	.asciz	"add"
	.size	.L.str.190, 4

	.type	.L.str.191,@object              # @.str.191
.L.str.191:
	.asciz	"or"
	.size	.L.str.191, 3

	.type	.L.str.192,@object              # @.str.192
.L.str.192:
	.asciz	"adc"
	.size	.L.str.192, 4

	.type	.L.str.193,@object              # @.str.193
.L.str.193:
	.asciz	"sbb"
	.size	.L.str.193, 4

	.type	.L.str.194,@object              # @.str.194
.L.str.194:
	.asciz	"and"
	.size	.L.str.194, 4

	.type	.L.str.195,@object              # @.str.195
.L.str.195:
	.asciz	"sub"
	.size	.L.str.195, 4

	.type	.L.str.196,@object              # @.str.196
.L.str.196:
	.asciz	"xor"
	.size	.L.str.196, 4

	.type	.L.str.197,@object              # @.str.197
.L.str.197:
	.asciz	"cmp"
	.size	.L.str.197, 4

	.type	.L.str.198,@object              # @.str.198
.L.str.198:
	.asciz	"rax"
	.size	.L.str.198, 4

	.type	.L.str.199,@object              # @.str.199
.L.str.199:
	.asciz	"rcx"
	.size	.L.str.199, 4

	.type	.L.str.200,@object              # @.str.200
.L.str.200:
	.asciz	"rdx"
	.size	.L.str.200, 4

	.type	.L.str.201,@object              # @.str.201
.L.str.201:
	.asciz	"rbx"
	.size	.L.str.201, 4

	.type	.L.str.202,@object              # @.str.202
.L.str.202:
	.asciz	"rsp"
	.size	.L.str.202, 4

	.type	.L.str.203,@object              # @.str.203
.L.str.203:
	.asciz	"rbp"
	.size	.L.str.203, 4

	.type	.L.str.204,@object              # @.str.204
.L.str.204:
	.asciz	"rsi"
	.size	.L.str.204, 4

	.type	.L.str.205,@object              # @.str.205
.L.str.205:
	.asciz	"rdi"
	.size	.L.str.205, 4

	.type	.L.str.206,@object              # @.str.206
.L.str.206:
	.asciz	"r8"
	.size	.L.str.206, 3

	.type	.L.str.207,@object              # @.str.207
.L.str.207:
	.asciz	"r9"
	.size	.L.str.207, 3

	.type	.L.str.208,@object              # @.str.208
.L.str.208:
	.asciz	"r10"
	.size	.L.str.208, 4

	.type	.L.str.209,@object              # @.str.209
.L.str.209:
	.asciz	"r11"
	.size	.L.str.209, 4

	.type	.L.str.210,@object              # @.str.210
.L.str.210:
	.asciz	"r12"
	.size	.L.str.210, 4

	.type	.L.str.211,@object              # @.str.211
.L.str.211:
	.asciz	"r13"
	.size	.L.str.211, 4

	.type	.L.str.212,@object              # @.str.212
.L.str.212:
	.asciz	"r14"
	.size	.L.str.212, 4

	.type	.L.str.213,@object              # @.str.213
.L.str.213:
	.asciz	"r15"
	.size	.L.str.213, 4

	.type	.L.str.214,@object              # @.str.214
.L.str.214:
	.asciz	"rip"
	.size	.L.str.214, 4

	.type	.L.str.215,@object              # @.str.215
.L.str.215:
	.asciz	"-0x"
	.size	.L.str.215, 4

	.type	.L.str.216,@object              # @.str.216
.L.str.216:
	.asciz	"+0x"
	.size	.L.str.216, 4

	.type	.L.str.217,@object              # @.str.217
.L.str.217:
	.asciz	"rol"
	.size	.L.str.217, 4

	.type	.L.str.218,@object              # @.str.218
.L.str.218:
	.asciz	"ror"
	.size	.L.str.218, 4

	.type	.L.str.219,@object              # @.str.219
.L.str.219:
	.asciz	"rcl"
	.size	.L.str.219, 4

	.type	.L.str.220,@object              # @.str.220
.L.str.220:
	.asciz	"rcr"
	.size	.L.str.220, 4

	.type	.L.str.221,@object              # @.str.221
.L.str.221:
	.asciz	"shl"
	.size	.L.str.221, 4

	.type	.L.str.222,@object              # @.str.222
.L.str.222:
	.asciz	"shr"
	.size	.L.str.222, 4

	.type	.L.str.223,@object              # @.str.223
.L.str.223:
	.asciz	"sal"
	.size	.L.str.223, 4

	.type	.L.str.224,@object              # @.str.224
.L.str.224:
	.asciz	"sar"
	.size	.L.str.224, 4

	.ident	"clang version 20.1.8"
	.section	".note.GNU-stack","",@progbits
	.addrsig
