	.file	"disasm.c"
	.text
	.globl	x86_decode                      # -- Begin function x86_decode
	.p2align	4
	.type	x86_decode,@function
x86_decode:                             # @x86_decode
# %bb.0:
	pushq	%rbp
	pushq	%r15
	pushq	%r14
	pushq	%r13
	pushq	%r12
	pushq	%rbx
	subq	$184, %rsp
	movq	%rcx, %r12
	movl	%esi, %r15d
	movq	%rdi, 72(%rsp)
	cmpl	$15, %esi
	movl	$15, %eax
	cmovbl	%esi, %eax
	movl	%eax, 80(%rsp)
	movq	$0, 84(%rsp)
	movl	$0, 92(%rsp)
	leaq	24(%rcx), %rcx
	movq	%rcx, 64(%rsp)                  # 8-byte Spill
	movq	%rcx, 8(%rsp)
	movq	$64, 16(%rsp)
	movb	$0, 24(%r12)
	movq	%rdx, 176(%rsp)                 # 8-byte Spill
	movq	%rdx, (%r12)
	movl	$0, 12(%r12)
	movq	$0, 16(%r12)
	movl	84(%rsp), %ecx
	xorl	%r8d, %r8d
	testl	%esi, %esi
	je	.LBB0_22
# %bb.1:
	xorl	%r11d, %r11d
	xorl	%r9d, %r9d
	xorl	%ebx, %ebx
	xorl	%r10d, %r10d
	jmp	.LBB0_4
.LBB0_2:                                #   in Loop: Header=BB0_4 Depth=1
	movl	$1, %r10d
	.p2align	4
.LBB0_3:                                #   in Loop: Header=BB0_4 Depth=1
	incq	%r8
	movl	%r8d, %ecx
	cmpl	%eax, %r8d
	jae	.LBB0_21
.LBB0_4:                                # =>This Inner Loop Header: Depth=1
	movzbl	(%rdi,%r8), %edx
	cmpl	$239, %edx
	jle	.LBB0_9
# %bb.5:                                #   in Loop: Header=BB0_4 Depth=1
	cmpl	$240, %edx
	je	.LBB0_2
# %bb.6:                                #   in Loop: Header=BB0_4 Depth=1
	cmpl	$242, %edx
	je	.LBB0_19
# %bb.7:                                #   in Loop: Header=BB0_4 Depth=1
	cmpl	$243, %edx
	jne	.LBB0_11
# %bb.8:                                #   in Loop: Header=BB0_4 Depth=1
	movl	$1, %r9d
	jmp	.LBB0_3
	.p2align	4
.LBB0_9:                                #   in Loop: Header=BB0_4 Depth=1
	cmpl	$102, %edx
	je	.LBB0_20
# %bb.10:                               #   in Loop: Header=BB0_4 Depth=1
	cmpl	$103, %edx
	je	.LBB0_3
.LBB0_11:                               #   in Loop: Header=BB0_4 Depth=1
	movl	%edx, %esi
	andb	$-2, %sil
	cmpb	$100, %sil
	je	.LBB0_3
# %bb.12:                               #   in Loop: Header=BB0_4 Depth=1
	orl	$8, %edx
	andl	$-17, %edx
	cmpl	$46, %edx
	je	.LBB0_3
	jmp	.LBB0_13
.LBB0_19:                               #   in Loop: Header=BB0_4 Depth=1
	movl	$1, %ebx
	jmp	.LBB0_3
.LBB0_20:                               #   in Loop: Header=BB0_4 Depth=1
	movl	$1, %r11d
	jmp	.LBB0_3
.LBB0_21:
	movl	%r8d, %ecx
	jmp	.LBB0_23
.LBB0_22:
	xorl	%r10d, %r10d
	xorl	%ebx, %ebx
	xorl	%r9d, %r9d
	xorl	%r11d, %r11d
.LBB0_23:
	movl	%ecx, 84(%rsp)
.LBB0_24:
	xorl	%r14d, %r14d
	testl	%r11d, %r11d
	sete	%r14b
	leal	2(,%r14,2), %ecx
	movl	%ecx, 44(%rsp)                  # 4-byte Spill
	leaq	R8H(%rip), %rbp
	xorl	%esi, %esi
	movb	$1, %r11b
	xorl	%edx, %edx
	xorl	%ecx, %ecx
	movb	$1, %r13b
	movl	%r13d, 104(%rsp)                # 4-byte Spill
	xorl	%r13d, %r13d
	testl	%r10d, %r10d
	je	.LBB0_15
.LBB0_25:
	movl	$1801678700, 24(%r12)           # imm = 0x6B636F6C
	movl	$5, 20(%rsp)
	movw	$32, 28(%r12)
	movl	$5, %r10d
	movq	%r10, 48(%rsp)                  # 8-byte Spill
	cmpl	%eax, %r8d
	jae	.LBB0_16
.LBB0_26:
	movl	%r13d, 28(%rsp)                 # 4-byte Spill
	movq	%r15, 32(%rsp)                  # 8-byte Spill
	movl	%r8d, %r15d
	leaq	1(%r15), %r10
	movl	%r10d, 84(%rsp)
	movzbl	(%rdi,%r15), %r13d
	cmpq	$15, %r13
	jne	.LBB0_34
# %bb.27:
	cmpl	%eax, %r10d
	jae	.LBB0_47
# %bb.28:
	addq	$2, %r15
	movl	%r15d, 84(%rsp)
	movzbl	(%rdi,%r10), %r13d
	testl	%r9d, %r9d
	setne	%r14b
	cmpl	$30, %r13d
	jne	.LBB0_67
# %bb.29:
	testl	%r9d, %r9d
	je	.LBB0_67
# %bb.30:
	cmpl	%eax, %r15d
	jae	.LBB0_83
# %bb.31:
	addl	$3, %r8d
	movl	%r8d, 84(%rsp)
	movzbl	(%rdi,%r15), %eax
	cmpl	$251, %eax
	je	.LBB0_217
# %bb.32:
	cmpl	$250, %eax
	movq	32(%rsp), %r15                  # 8-byte Reload
	movq	48(%rsp), %rcx                  # 8-byte Reload
	movq	64(%rsp), %rdx                  # 8-byte Reload
	jne	.LBB0_84
# %bb.33:
	movl	%ecx, %eax
	addl	$7, %ecx
	movl	%ecx, 20(%rsp)
	movabsq	$14696563693874789, %rcx        # imm = 0x34367262646E65
	movq	%rcx, (%rdx,%rax)
	jmp	.LBB0_85
.LBB0_34:
	cmpb	$63, %r13b
	ja	.LBB0_40
# %bb.35:
	movl	%r13d, %r15d
	andl	$7, %r15d
	cmpl	$5, %r15d
	ja	.LBB0_40
# %bb.36:
	movl	%r13d, %r9d
	andl	$-8, %r9d
	leaq	ALU(%rip), %r11
	movq	(%r9,%r11), %rbx
	cmpl	$5, %r15d
	je	.LBB0_144
# %bb.37:
	cmpl	$4, %r15d
	movl	44(%rsp), %ebp                  # 4-byte Reload
	jne	.LBB0_149
# %bb.38:
	cmpl	%eax, %r10d
	movq	32(%rsp), %r15                  # 8-byte Reload
	jae	.LBB0_210
# %bb.39:
	addl	$2, %r8d
	movl	%r8d, 84(%rsp)
	movsbq	(%rdi,%r10), %rax
	jmp	.LBB0_211
.LBB0_40:
	movl	%r13d, %r15d
	andb	$-8, %r15b
	cmpb	$88, %r15b
	je	.LBB0_62
# %bb.41:
	movq	%r12, 56(%rsp)                  # 8-byte Spill
	movl	%r13d, %r12d
	andl	$-8, %r12d
	cmpl	$80, %r12d
	jne	.LBB0_72
# %bb.42:
	movq	48(%rsp), %rdx                  # 8-byte Reload
	movl	%edx, %eax
	movq	64(%rsp), %rsi                  # 8-byte Reload
	movl	$1752397168, (%rsi,%rax)        # imm = 0x68737570
	addl	$5, %edx
	movl	%edx, 20(%rsp)
	movw	$32, 4(%rsi,%rax)
	leal	-80(%r13,%rcx,8), %eax
	leaq	R64(%rip), %rcx
	movq	(%rcx,%rax,8), %rax
	movzbl	(%rax), %ecx
	testb	%cl, %cl
	movq	56(%rsp), %r12                  # 8-byte Reload
	je	.LBB0_219
# %bb.43:
	incq	%rax
	movq	32(%rsp), %r15                  # 8-byte Reload
	jmp	.LBB0_45
	.p2align	4
.LBB0_44:                               #   in Loop: Header=BB0_45 Depth=1
	movzbl	(%rax), %ecx
	incq	%rax
	testb	%cl, %cl
	je	.LBB0_85
.LBB0_45:                               # =>This Inner Loop Header: Depth=1
	movl	20(%rsp), %edx
	leal	1(%rdx), %esi
	cmpl	16(%rsp), %esi
	jae	.LBB0_44
# %bb.46:                               #   in Loop: Header=BB0_45 Depth=1
	movq	8(%rsp), %rdi
	movl	%esi, 20(%rsp)
	movb	%cl, (%rdi,%rdx)
	movq	8(%rsp), %rcx
	movl	20(%rsp), %edx
	movb	$0, (%rcx,%rdx)
	jmp	.LBB0_44
.LBB0_47:
	movl	$1, 88(%rsp)
	testl	%r9d, %r9d
	setne	%r14b
	xorl	%r13d, %r13d
	xorl	%ebp, %ebp
	movq	32(%rsp), %r15                  # 8-byte Reload
.LBB0_48:
	cmpl	$239, %r13d
	je	.LBB0_60
# %bb.49:
	cmpl	$84, %r13d
	je	.LBB0_60
# %bb.50:
	cmpl	$42, %r13d
	je	.LBB0_60
# %bb.51:
	cmpl	$16, %ebp
	je	.LBB0_60
# %bb.52:
	cmpl	$40, %ebp
	je	.LBB0_60
# %bb.53:
	cmpl	$214, %r13d
	je	.LBB0_60
# %bb.54:
	movl	%ebp, %eax
	andl	$-18, %eax
	cmpl	$110, %eax
	je	.LBB0_60
# %bb.55:
	movl	%ebp, %eax
	andl	$-4, %eax
	cmpl	$92, %eax
	je	.LBB0_60
# %bb.56:
	leal	-87(%r13), %edi
	cmpl	$4, %edi
	jb	.LBB0_60
# %bb.57:
	cmpl	$81, %r13d
	je	.LBB0_60
# %bb.58:
	cmpl	$44, %eax
	je	.LBB0_60
# %bb.59:
	movq	48(%rsp), %rcx                  # 8-byte Reload
	movl	%ecx, %eax
	movq	64(%rsp), %rdx                  # 8-byte Reload
	movl	$1684103720, (%rdx,%rax)        # imm = 0x64616228
	addl	$5, %ecx
	movl	%ecx, 20(%rsp)
	movw	$41, 4(%rdx,%rax)
	jmp	.LBB0_85
.LBB0_60:
	leaq	72(%rsp), %rdi
	leaq	112(%rsp), %r8
                                        # kill: def $ecx killed $ecx killed $rcx
	callq	parse_modrm
	cmpl	$16, %ebp
	jne	.LBB0_81
# %bb.61:
	testl	%ebx, %ebx
	leaq	.L.str.17(%rip), %rax
	leaq	.L.str.16(%rip), %rcx
	cmoveq	%rax, %rcx
	leaq	.L.str.15(%rip), %rax
	testb	%r14b, %r14b
	cmoveq	%rcx, %rax
	movl	28(%rsp), %r8d                  # 4-byte Reload
	movq	48(%rsp), %rdx                  # 8-byte Reload
	jmp	.LBB0_186
.LBB0_62:
	movl	48(%rsp), %eax                  # 4-byte Reload
	leaq	4(%rax), %rdx
	movl	%edx, 20(%rsp)
	movq	64(%rsp), %rdx                  # 8-byte Reload
	movl	$544239472, (%rdx,%rax)         # imm = 0x20706F70
	movb	$0, 4(%rdx,%rax)
	leal	-88(%r13,%rcx,8), %eax
	leaq	R64(%rip), %rcx
	movq	(%rcx,%rax,8), %rax
	movzbl	(%rax), %ecx
	testb	%cl, %cl
	je	.LBB0_219
# %bb.63:
	incq	%rax
	movq	32(%rsp), %r15                  # 8-byte Reload
	jmp	.LBB0_65
	.p2align	4
.LBB0_64:                               #   in Loop: Header=BB0_65 Depth=1
	movzbl	(%rax), %ecx
	incq	%rax
	testb	%cl, %cl
	je	.LBB0_85
.LBB0_65:                               # =>This Inner Loop Header: Depth=1
	movl	20(%rsp), %edx
	leal	1(%rdx), %esi
	cmpl	16(%rsp), %esi
	jae	.LBB0_64
# %bb.66:                               #   in Loop: Header=BB0_65 Depth=1
	movq	8(%rsp), %rdi
	movl	%esi, 20(%rsp)
	movb	%cl, (%rdi,%rdx)
	movq	8(%rsp), %rcx
	movl	20(%rsp), %edx
	movb	$0, (%rcx,%rdx)
	jmp	.LBB0_64
.LBB0_67:
	cmpl	$30, %r13d
	jle	.LBB0_86
# %bb.68:
	cmpl	$31, %r13d
	je	.LBB0_207
# %bb.69:
	cmpl	$49, %r13d
	je	.LBB0_208
# %bb.70:
	cmpl	$162, %r13d
	jne	.LBB0_263
# %bb.71:
	movq	48(%rsp), %rcx                  # 8-byte Reload
	movl	%ecx, %eax
	movq	64(%rsp), %rdx                  # 8-byte Reload
	movl	$1769304163, (%rdx,%rax)        # imm = 0x69757063
	addl	$5, %ecx
	movl	%ecx, 20(%rsp)
	movw	$100, 4(%rdx,%rax)
	jmp	.LBB0_219
.LBB0_72:
	cmpl	$104, %r13d
	movq	56(%rsp), %r12                  # 8-byte Reload
	jle	.LBB0_89
# %bb.73:
	cmpl	$105, %r13d
	je	.LBB0_76
# %bb.74:
	cmpl	$106, %r13d
	je	.LBB0_91
# %bb.75:
	cmpl	$107, %r13d
	jne	.LBB0_268
.LBB0_76:
	leaq	72(%rsp), %r14
	leaq	112(%rsp), %r8
	movq	%r14, %rdi
                                        # kill: def $ecx killed $ecx killed $rcx
	callq	parse_modrm
	xorl	%eax, %eax
	movl	44(%rsp), %ebx                  # 4-byte Reload
	cmpl	$2, %ebx
	setne	%al
	cmpl	$105, %r13d
	leal	2(%rax,%rax), %eax
	movl	$1, %esi
	cmovel	%eax, %esi
	movq	%r14, %rdi
	callq	rd_imm_sext
	movq	%rax, %r13
	movq	48(%rsp), %r14                  # 8-byte Reload
	movl	%r14d, %eax
	movq	64(%rsp), %rcx                  # 8-byte Reload
	movl	$1819635049, (%rcx,%rax)        # imm = 0x6C756D69
	addl	$5, %r14d
	movl	%r14d, 20(%rsp)
	movw	$32, 4(%rcx,%rax)
	movl	116(%rsp), %edi
	movl	%ebx, %esi
	movl	28(%rsp), %edx                  # 4-byte Reload
	callq	reg_name
	movzbl	(%rax), %ecx
	testb	%cl, %cl
	je	.LBB0_303
# %bb.77:
	incq	%rax
	jmp	.LBB0_79
	.p2align	4
.LBB0_78:                               #   in Loop: Header=BB0_79 Depth=1
	movzbl	(%rax), %ecx
	incq	%rax
	testb	%cl, %cl
	je	.LBB0_206
.LBB0_79:                               # =>This Inner Loop Header: Depth=1
	movl	20(%rsp), %edx
	leal	1(%rdx), %esi
	cmpl	16(%rsp), %esi
	jae	.LBB0_78
# %bb.80:                               #   in Loop: Header=BB0_79 Depth=1
	movq	8(%rsp), %rdi
	movl	%esi, 20(%rsp)
	movb	%cl, (%rdi,%rdx)
	movq	8(%rsp), %rcx
	movl	20(%rsp), %edx
	movb	$0, (%rcx,%rdx)
	jmp	.LBB0_78
.LBB0_81:
	cmpl	$40, %ebp
	movq	48(%rsp), %rdx                  # 8-byte Reload
	jne	.LBB0_181
# %bb.82:
	leaq	.L.str.18(%rip), %rax
	jmp	.LBB0_185
.LBB0_83:
	movl	$1, 88(%rsp)
	movq	32(%rsp), %r15                  # 8-byte Reload
	movq	48(%rsp), %rcx                  # 8-byte Reload
	movq	64(%rsp), %rdx                  # 8-byte Reload
.LBB0_84:
	movl	%ecx, %eax
	addl	$3, %ecx
	movl	%ecx, 20(%rsp)
	movl	$7368558, (%rdx,%rax)           # imm = 0x706F6E
.LBB0_85:
	xorl	%r14d, %r14d
	xorl	%ecx, %ecx
	jmp	.LBB0_221
.LBB0_86:
	cmpl	$5, %r13d
	je	.LBB0_209
# %bb.87:
	cmpl	$11, %r13d
	jne	.LBB0_263
# %bb.88:
	movq	48(%rsp), %rcx                  # 8-byte Reload
	movl	%ecx, %eax
	addl	$3, %ecx
	movl	%ecx, 20(%rsp)
	movq	64(%rsp), %rcx                  # 8-byte Reload
	movl	$3302517, (%rcx,%rax)           # imm = 0x326475
	jmp	.LBB0_219
.LBB0_89:
	cmpl	$99, %r13d
	je	.LBB0_258
# %bb.90:
	cmpl	$104, %r13d
	jne	.LBB0_268
.LBB0_91:
	xorl	%eax, %eax
	cmpl	$104, %r13d
	sete	%al
	leal	(%rax,%rax,2), %esi
	incl	%esi
	leaq	72(%rsp), %rdi
	callq	rd_imm_sext
	movl	48(%rsp), %ecx                  # 4-byte Reload
	leaq	7(%rcx), %rdx
	movl	%edx, 20(%rsp)
	movabsq	$33829912954762608, %rsi        # imm = 0x78302068737570
	movq	64(%rsp), %rdi                  # 8-byte Reload
	movq	%rsi, (%rdi,%rcx)
	testq	%rax, %rax
	je	.LBB0_314
# %bb.92:
	xorl	%ecx, %ecx
	movq	%rax, %rdx
	movq	32(%rsp), %r15                  # 8-byte Reload
	.p2align	4
.LBB0_93:                               # =>This Inner Loop Header: Depth=1
	movl	%eax, %esi
	andl	$15, %esi
	leal	87(%rsi), %edi
	leal	48(%rsi), %r8d
	cmpl	$10, %esi
	movzbl	%r8b, %esi
	movzbl	%dil, %edi
	cmovbl	%esi, %edi
	movb	%dil, 112(%rsp,%rcx)
	incq	%rcx
	shrq	$4, %rdx
	cmpq	$15, %rax
	movq	%rdx, %rax
	ja	.LBB0_93
	jmp	.LBB0_95
	.p2align	4
.LBB0_94:                               #   in Loop: Header=BB0_95 Depth=1
	decq	%rcx
	je	.LBB0_85
.LBB0_95:                               # =>This Inner Loop Header: Depth=1
	movl	20(%rsp), %eax
	leal	1(%rax), %edx
	cmpl	16(%rsp), %edx
	jae	.LBB0_94
# %bb.96:                               #   in Loop: Header=BB0_95 Depth=1
	movzbl	111(%rsp,%rcx), %esi
	movq	8(%rsp), %rdi
	movl	%edx, 20(%rsp)
	movb	%sil, (%rdi,%rax)
	movq	8(%rsp), %rax
	movl	20(%rsp), %edx
	movb	$0, (%rax,%rdx)
	jmp	.LBB0_94
.LBB0_13:
	movl	%ecx, 84(%rsp)
	movzbl	(%rdi,%r8), %ecx
	movzbl	%cl, %ebp
	movl	%ebp, %ecx
	andl	$-16, %ecx
	cmpl	$64, %ecx
	jne	.LBB0_24
# %bb.14:
	movl	%ebp, %esi
	shrl	$2, %esi
	andl	$1, %esi
	movl	%ebp, %edx
	shrl	%edx
	andl	$1, %edx
	movl	%ebp, %ecx
	andl	$1, %ecx
	incl	%r8d
	movl	%r8d, 84(%rsp)
	xorl	%r14d, %r14d
	testl	%r11d, %r11d
	sete	%r14b
	testb	$8, %bpl
	leal	2(%r14,%r14), %r11d
	movl	$8, %ebp
	cmovel	%r11d, %ebp
	movl	%ebp, 44(%rsp)                  # 4-byte Spill
	sete	%r11b
	movl	$1, %r13d
	leaq	R8L(%rip), %rbp
	movl	$0, 104(%rsp)                   # 4-byte Folded Spill
	testl	%r10d, %r10d
	jne	.LBB0_25
.LBB0_15:
	movq	$0, 48(%rsp)                    # 8-byte Folded Spill
	cmpl	%eax, %r8d
	jb	.LBB0_26
.LBB0_16:
	movl	$1, 88(%rsp)
	xorl	%r13d, %r13d
	leaq	.L.str.190(%rip), %rbx
	movl	44(%rsp), %ebp                  # 4-byte Reload
.LBB0_17:
	testb	$5, %r13b
	movl	$1, %eax
	cmovel	%eax, %ebp
	leaq	72(%rsp), %rdi
	leaq	112(%rsp), %r8
                                        # kill: def $ecx killed $ecx killed $rcx
	callq	parse_modrm
	movzbl	(%rbx), %eax
	testb	%al, %al
	je	.LBB0_101
# %bb.18:
	incq	%rbx
	jmp	.LBB0_98
	.p2align	4
.LBB0_97:                               #   in Loop: Header=BB0_98 Depth=1
	movzbl	(%rbx), %eax
	incq	%rbx
	testb	%al, %al
	je	.LBB0_100
.LBB0_98:                               # =>This Inner Loop Header: Depth=1
	movl	20(%rsp), %ecx
	leal	1(%rcx), %edx
	cmpl	16(%rsp), %edx
	jae	.LBB0_97
# %bb.99:                               #   in Loop: Header=BB0_98 Depth=1
	movq	8(%rsp), %rsi
	movl	%edx, 20(%rsp)
	movb	%al, (%rsi,%rcx)
	movq	8(%rsp), %rax
	movl	20(%rsp), %ecx
	movb	$0, (%rax,%rcx)
	jmp	.LBB0_97
.LBB0_100:
	movl	16(%rsp), %eax
	movl	20(%rsp), %edx
	jmp	.LBB0_102
.LBB0_101:
	movl	$64, %eax
	movq	48(%rsp), %rdx                  # 8-byte Reload
.LBB0_102:
	andl	$6, %r13d
	leal	1(%rdx), %ecx
	cmpl	%eax, %ecx
	jae	.LBB0_104
# %bb.103:
	movq	8(%rsp), %rax
	movl	%ecx, 20(%rsp)
	movl	%edx, %ecx
	movb	$32, (%rax,%rcx)
	movq	8(%rsp), %rax
	movl	20(%rsp), %ecx
	movb	$0, (%rax,%rcx)
.LBB0_104:
	cmpl	$2, %r13d
	jne	.LBB0_109
# %bb.105:
	movl	116(%rsp), %eax
	cmpl	$2, %ebp
	je	.LBB0_115
# %bb.106:
	cmpl	$4, %ebp
	je	.LBB0_114
# %bb.107:
	cmpl	$8, %ebp
	jne	.LBB0_116
# %bb.108:
	andl	$15, %eax
	leaq	R64(%rip), %rcx
	jmp	.LBB0_123
.LBB0_109:
	cmpl	$0, 112(%rsp)
	je	.LBB0_156
# %bb.110:
	movl	120(%rsp), %eax
	cmpl	$2, %ebp
	je	.LBB0_119
# %bb.111:
	cmpl	$4, %ebp
	je	.LBB0_118
# %bb.112:
	cmpl	$8, %ebp
	jne	.LBB0_120
# %bb.113:
	andl	$15, %eax
	leaq	R64(%rip), %rcx
	jmp	.LBB0_151
.LBB0_156:
	leaq	8(%rsp), %rdi
	leaq	112(%rsp), %rsi
	callq	render_mem
	jmp	.LBB0_157
.LBB0_114:
	andl	$15, %eax
	leaq	R32(%rip), %rcx
	jmp	.LBB0_123
.LBB0_115:
	andl	$15, %eax
	leaq	R16(%rip), %rcx
	jmp	.LBB0_123
.LBB0_116:
	cmpb	$0, 104(%rsp)                   # 1-byte Folded Reload
	je	.LBB0_122
# %bb.117:
	andl	$7, %eax
	leaq	R8H(%rip), %rcx
	jmp	.LBB0_123
.LBB0_118:
	andl	$15, %eax
	leaq	R32(%rip), %rcx
	jmp	.LBB0_151
.LBB0_119:
	andl	$15, %eax
	leaq	R16(%rip), %rcx
	jmp	.LBB0_151
.LBB0_120:
	cmpb	$0, 104(%rsp)                   # 1-byte Folded Reload
	je	.LBB0_150
# %bb.121:
	andl	$7, %eax
	leaq	R8H(%rip), %rcx
	jmp	.LBB0_151
.LBB0_122:
	andl	$15, %eax
	leaq	R8L(%rip), %rcx
.LBB0_123:
	leaq	(%rcx,%rax,8), %rax
	movq	(%rax), %rax
	movzbl	(%rax), %ecx
	testb	%cl, %cl
	je	.LBB0_128
# %bb.124:
	incq	%rax
	jmp	.LBB0_126
	.p2align	4
.LBB0_125:                              #   in Loop: Header=BB0_126 Depth=1
	movzbl	(%rax), %ecx
	incq	%rax
	testb	%cl, %cl
	je	.LBB0_128
.LBB0_126:                              # =>This Inner Loop Header: Depth=1
	movl	20(%rsp), %edx
	leal	1(%rdx), %esi
	cmpl	16(%rsp), %esi
	jae	.LBB0_125
# %bb.127:                              #   in Loop: Header=BB0_126 Depth=1
	movq	8(%rsp), %rdi
	movl	%esi, 20(%rsp)
	movb	%cl, (%rdi,%rdx)
	movq	8(%rsp), %rcx
	movl	20(%rsp), %edx
	movb	$0, (%rcx,%rdx)
	jmp	.LBB0_125
.LBB0_128:
	movl	16(%rsp), %edx
	movl	20(%rsp), %eax
	leal	1(%rax), %ecx
	cmpl	%edx, %ecx
	jae	.LBB0_129
# %bb.135:
	movq	8(%rsp), %rdx
	movl	%ecx, 20(%rsp)
	movb	$44, (%rdx,%rax)
	movq	8(%rsp), %rax
	movl	20(%rsp), %ecx
	movb	$0, (%rax,%rcx)
	movl	16(%rsp), %edx
	movl	20(%rsp), %eax
	leal	1(%rax), %ecx
	cmpl	%edx, %ecx
	jb	.LBB0_136
.LBB0_130:
	cmpl	$0, 112(%rsp)
	jne	.LBB0_131
	jmp	.LBB0_137
.LBB0_129:
	cmpl	%edx, %ecx
	jae	.LBB0_130
.LBB0_136:
	movq	8(%rsp), %rdx
	movl	%ecx, 20(%rsp)
	movl	%eax, %eax
	movb	$32, (%rdx,%rax)
	movq	8(%rsp), %rax
	movl	20(%rsp), %ecx
	movb	$0, (%rax,%rcx)
	cmpl	$0, 112(%rsp)
	je	.LBB0_137
.LBB0_131:
	movl	120(%rsp), %eax
	cmpl	$2, %ebp
	je	.LBB0_141
# %bb.132:
	cmpl	$4, %ebp
	je	.LBB0_140
# %bb.133:
	cmpl	$8, %ebp
	jne	.LBB0_142
# %bb.134:
	andl	$15, %eax
	leaq	R64(%rip), %rcx
	jmp	.LBB0_176
.LBB0_140:
	andl	$15, %eax
	leaq	R32(%rip), %rcx
	jmp	.LBB0_176
.LBB0_141:
	andl	$15, %eax
	leaq	R16(%rip), %rcx
	jmp	.LBB0_176
.LBB0_142:
	cmpb	$0, 104(%rsp)                   # 1-byte Folded Reload
	je	.LBB0_175
# %bb.143:
	andl	$7, %eax
	leaq	R8H(%rip), %rcx
	jmp	.LBB0_176
.LBB0_144:
	xorl	%eax, %eax
	movl	44(%rsp), %ebp                  # 4-byte Reload
	cmpl	$2, %ebp
	setne	%al
	leal	2(,%rax,2), %esi
	leaq	72(%rsp), %rdi
	callq	rd_imm_sext
	movzbl	(%rbx), %ecx
	testb	%cl, %cl
	je	.LBB0_296
# %bb.145:
	incq	%rbx
	movq	32(%rsp), %r15                  # 8-byte Reload
	jmp	.LBB0_147
	.p2align	4
.LBB0_146:                              #   in Loop: Header=BB0_147 Depth=1
	movzbl	(%rbx), %ecx
	incq	%rbx
	testb	%cl, %cl
	je	.LBB0_205
.LBB0_147:                              # =>This Inner Loop Header: Depth=1
	movl	20(%rsp), %edx
	leal	1(%rdx), %esi
	cmpl	16(%rsp), %esi
	jae	.LBB0_146
# %bb.148:                              #   in Loop: Header=BB0_147 Depth=1
	movq	8(%rsp), %rdi
	movl	%esi, 20(%rsp)
	movb	%cl, (%rdi,%rdx)
	movq	8(%rsp), %rcx
	movl	20(%rsp), %edx
	movb	$0, (%rcx,%rdx)
	jmp	.LBB0_146
.LBB0_149:
	movq	32(%rsp), %r15                  # 8-byte Reload
	jmp	.LBB0_17
.LBB0_150:
	andl	$15, %eax
	leaq	R8L(%rip), %rcx
.LBB0_151:
	leaq	(%rcx,%rax,8), %rax
	movq	(%rax), %rax
	movzbl	(%rax), %ecx
	testb	%cl, %cl
	je	.LBB0_157
# %bb.152:
	incq	%rax
	jmp	.LBB0_154
	.p2align	4
.LBB0_153:                              #   in Loop: Header=BB0_154 Depth=1
	movzbl	(%rax), %ecx
	incq	%rax
	testb	%cl, %cl
	je	.LBB0_157
.LBB0_154:                              # =>This Inner Loop Header: Depth=1
	movl	20(%rsp), %edx
	leal	1(%rdx), %esi
	cmpl	16(%rsp), %esi
	jae	.LBB0_153
# %bb.155:                              #   in Loop: Header=BB0_154 Depth=1
	movq	8(%rsp), %rdi
	movl	%esi, 20(%rsp)
	movb	%cl, (%rdi,%rdx)
	movq	8(%rsp), %rcx
	movl	20(%rsp), %edx
	movb	$0, (%rcx,%rdx)
	jmp	.LBB0_153
.LBB0_157:
	movl	16(%rsp), %edx
	movl	20(%rsp), %eax
	leal	1(%rax), %ecx
	cmpl	%edx, %ecx
	jae	.LBB0_159
# %bb.158:
	movq	8(%rsp), %rdx
	movl	%ecx, 20(%rsp)
	movb	$44, (%rdx,%rax)
	movq	8(%rsp), %rax
	movl	20(%rsp), %ecx
	movb	$0, (%rax,%rcx)
	movl	16(%rsp), %edx
	movl	20(%rsp), %eax
	leal	1(%rax), %ecx
.LBB0_159:
	cmpl	%edx, %ecx
	jae	.LBB0_161
# %bb.160:
	movq	8(%rsp), %rdx
	movl	%ecx, 20(%rsp)
	movl	%eax, %eax
	movb	$32, (%rdx,%rax)
	movq	8(%rsp), %rax
	movl	20(%rsp), %ecx
	movb	$0, (%rax,%rcx)
.LBB0_161:
	movl	116(%rsp), %eax
	cmpl	$2, %ebp
	je	.LBB0_166
# %bb.162:
	cmpl	$4, %ebp
	je	.LBB0_165
# %bb.163:
	cmpl	$8, %ebp
	jne	.LBB0_167
# %bb.164:
	andl	$15, %eax
	leaq	R64(%rip), %rcx
	jmp	.LBB0_170
.LBB0_165:
	andl	$15, %eax
	leaq	R32(%rip), %rcx
	jmp	.LBB0_170
.LBB0_166:
	andl	$15, %eax
	leaq	R16(%rip), %rcx
	jmp	.LBB0_170
.LBB0_167:
	cmpb	$0, 104(%rsp)                   # 1-byte Folded Reload
	je	.LBB0_169
# %bb.168:
	andl	$7, %eax
	leaq	R8H(%rip), %rcx
	jmp	.LBB0_170
.LBB0_169:
	andl	$15, %eax
	leaq	R8L(%rip), %rcx
.LBB0_170:
	leaq	(%rcx,%rax,8), %rax
	movq	(%rax), %rax
	movzbl	(%rax), %ecx
	testb	%cl, %cl
	je	.LBB0_138
# %bb.171:
	incq	%rax
	jmp	.LBB0_173
	.p2align	4
.LBB0_172:                              #   in Loop: Header=BB0_173 Depth=1
	movzbl	(%rax), %ecx
	incq	%rax
	testb	%cl, %cl
	je	.LBB0_138
.LBB0_173:                              # =>This Inner Loop Header: Depth=1
	movl	20(%rsp), %edx
	leal	1(%rdx), %esi
	cmpl	16(%rsp), %esi
	jae	.LBB0_172
# %bb.174:                              #   in Loop: Header=BB0_173 Depth=1
	movq	8(%rsp), %rdi
	movl	%esi, 20(%rsp)
	movb	%cl, (%rdi,%rdx)
	movq	8(%rsp), %rcx
	movl	20(%rsp), %edx
	movb	$0, (%rcx,%rdx)
	jmp	.LBB0_172
.LBB0_175:
	andl	$15, %eax
	leaq	R8L(%rip), %rcx
.LBB0_176:
	leaq	(%rcx,%rax,8), %rax
	movq	(%rax), %rax
	movzbl	(%rax), %ecx
	testb	%cl, %cl
	je	.LBB0_138
# %bb.177:
	incq	%rax
	jmp	.LBB0_179
	.p2align	4
.LBB0_178:                              #   in Loop: Header=BB0_179 Depth=1
	movzbl	(%rax), %ecx
	incq	%rax
	testb	%cl, %cl
	je	.LBB0_138
.LBB0_179:                              # =>This Inner Loop Header: Depth=1
	movl	20(%rsp), %edx
	leal	1(%rdx), %esi
	cmpl	16(%rsp), %esi
	jae	.LBB0_178
# %bb.180:                              #   in Loop: Header=BB0_179 Depth=1
	movq	8(%rsp), %rdi
	movl	%esi, 20(%rsp)
	movb	%cl, (%rdi,%rdx)
	movq	8(%rsp), %rcx
	movl	20(%rsp), %edx
	movb	$0, (%rcx,%rdx)
	jmp	.LBB0_178
.LBB0_181:
	cmpl	$42, %r13d
	jne	.LBB0_271
# %bb.182:
	testl	%ebx, %ebx
	leaq	.L.str.21(%rip), %rax
	leaq	.L.str.20(%rip), %rcx
	cmoveq	%rax, %rcx
	leaq	.L.str.19(%rip), %rax
.LBB0_183:
	testb	%r14b, %r14b
.LBB0_184:
	cmoveq	%rcx, %rax
.LBB0_185:
	movl	28(%rsp), %r8d                  # 4-byte Reload
.LBB0_186:
	movzbl	(%rax), %ecx
	testb	%cl, %cl
	je	.LBB0_192
# %bb.187:
	incq	%rax
	jmp	.LBB0_189
	.p2align	4
.LBB0_188:                              #   in Loop: Header=BB0_189 Depth=1
	movzbl	(%rax), %ecx
	incq	%rax
	testb	%cl, %cl
	je	.LBB0_191
.LBB0_189:                              # =>This Inner Loop Header: Depth=1
	movl	20(%rsp), %edx
	leal	1(%rdx), %esi
	cmpl	16(%rsp), %esi
	jae	.LBB0_188
# %bb.190:                              #   in Loop: Header=BB0_189 Depth=1
	movq	8(%rsp), %rdi
	movl	%esi, 20(%rsp)
	movb	%cl, (%rdi,%rdx)
	movq	8(%rsp), %rcx
	movl	20(%rsp), %edx
	movb	$0, (%rcx,%rdx)
	jmp	.LBB0_188
.LBB0_191:
	movl	16(%rsp), %eax
	movl	20(%rsp), %edx
	jmp	.LBB0_193
.LBB0_192:
	movl	$64, %eax
.LBB0_193:
	leal	1(%rdx), %ecx
	cmpl	%eax, %ecx
	jae	.LBB0_195
# %bb.194:
	movq	8(%rsp), %rax
	movl	%ecx, 20(%rsp)
	movl	%edx, %ecx
	movb	$32, (%rax,%rcx)
	movq	8(%rsp), %rax
	movl	20(%rsp), %ecx
	movb	$0, (%rax,%rcx)
.LBB0_195:
	movl	116(%rsp), %eax
	andl	$15, %eax
	leaq	XMM(%rip), %rcx
	movq	(%rcx,%rax,8), %rax
	movzbl	(%rax), %ecx
	testb	%cl, %cl
	je	.LBB0_200
# %bb.196:
	incq	%rax
	jmp	.LBB0_198
	.p2align	4
.LBB0_197:                              #   in Loop: Header=BB0_198 Depth=1
	movzbl	(%rax), %ecx
	incq	%rax
	testb	%cl, %cl
	je	.LBB0_200
.LBB0_198:                              # =>This Inner Loop Header: Depth=1
	movl	20(%rsp), %edx
	leal	1(%rdx), %esi
	cmpl	16(%rsp), %esi
	jae	.LBB0_197
# %bb.199:                              #   in Loop: Header=BB0_198 Depth=1
	movq	8(%rsp), %rdi
	movl	%esi, 20(%rsp)
	movb	%cl, (%rdi,%rdx)
	movq	8(%rsp), %rcx
	movl	20(%rsp), %edx
	movb	$0, (%rcx,%rdx)
	jmp	.LBB0_197
.LBB0_200:
	movl	16(%rsp), %edx
	movl	20(%rsp), %eax
	leal	1(%rax), %ecx
	cmpl	%edx, %ecx
	jae	.LBB0_202
# %bb.201:
	movq	8(%rsp), %rdx
	movl	%ecx, 20(%rsp)
	movb	$44, (%rdx,%rax)
	movq	8(%rsp), %rax
	movl	20(%rsp), %ecx
	movb	$0, (%rax,%rcx)
	movl	16(%rsp), %edx
	movl	20(%rsp), %eax
	leal	1(%rax), %ecx
.LBB0_202:
	cmpl	%edx, %ecx
	jae	.LBB0_204
# %bb.203:
	movq	8(%rsp), %rdx
	movl	%ecx, 20(%rsp)
	movl	%eax, %eax
	movb	$32, (%rdx,%rax)
	movq	8(%rsp), %rax
	movl	20(%rsp), %ecx
	movb	$0, (%rax,%rcx)
.LBB0_204:
	leaq	8(%rsp), %rdi
	leaq	112(%rsp), %rsi
	xorl	%r14d, %r14d
	movl	44(%rsp), %edx                  # 4-byte Reload
	movl	%r8d, %ecx
	xorl	%r8d, %r8d
	movl	$1, %r9d
.LBB0_531:
	callq	render_rm
	cmpl	$0, 124(%rsp)
	setne	%cl
	je	.LBB0_221
	jmp	.LBB0_532
.LBB0_205:
	movl	16(%rsp), %ecx
	movl	20(%rsp), %esi
	jmp	.LBB0_297
.LBB0_206:
	movl	16(%rsp), %eax
	movl	20(%rsp), %edx
	jmp	.LBB0_304
.LBB0_207:
	leaq	72(%rsp), %rdi
	leaq	112(%rsp), %r8
                                        # kill: def $ecx killed $ecx killed $rcx
	callq	parse_modrm
	movq	48(%rsp), %rcx                  # 8-byte Reload
	movl	%ecx, %eax
	addl	$3, %ecx
	movl	%ecx, 20(%rsp)
	movq	64(%rsp), %rcx                  # 8-byte Reload
	movl	$7368558, (%rcx,%rax)           # imm = 0x706F6E
	jmp	.LBB0_219
.LBB0_208:
	movq	48(%rsp), %rcx                  # 8-byte Reload
	movl	%ecx, %eax
	movq	64(%rsp), %rdx                  # 8-byte Reload
	movl	$1937007730, (%rdx,%rax)        # imm = 0x73746472
	addl	$5, %ecx
	movl	%ecx, 20(%rsp)
	movw	$99, 4(%rdx,%rax)
	jmp	.LBB0_219
.LBB0_209:
	movq	48(%rsp), %rcx                  # 8-byte Reload
	movl	%ecx, %eax
	addl	$7, %ecx
	movl	%ecx, 20(%rsp)
	movabsq	$30518463020890483, %rcx        # imm = 0x6C6C6163737973
	jmp	.LBB0_218
.LBB0_210:
	movl	$1, 88(%rsp)
	xorl	%eax, %eax
.LBB0_211:
	movq	48(%rsp), %rsi                  # 8-byte Reload
	movzbl	(%rbx), %ecx
	testb	%cl, %cl
	je	.LBB0_275
# %bb.212:
	incq	%rbx
	jmp	.LBB0_214
	.p2align	4
.LBB0_213:                              #   in Loop: Header=BB0_214 Depth=1
	movzbl	(%rbx), %ecx
	incq	%rbx
	testb	%cl, %cl
	je	.LBB0_216
.LBB0_214:                              # =>This Inner Loop Header: Depth=1
	movl	20(%rsp), %edx
	leal	1(%rdx), %esi
	cmpl	16(%rsp), %esi
	jae	.LBB0_213
# %bb.215:                              #   in Loop: Header=BB0_214 Depth=1
	movq	8(%rsp), %rdi
	movl	%esi, 20(%rsp)
	movb	%cl, (%rdi,%rdx)
	movq	8(%rsp), %rcx
	movl	20(%rsp), %edx
	movb	$0, (%rcx,%rdx)
	jmp	.LBB0_213
.LBB0_216:
	movl	16(%rsp), %ecx
	movl	20(%rsp), %esi
	jmp	.LBB0_276
.LBB0_217:
	movq	48(%rsp), %rcx                  # 8-byte Reload
	movl	%ecx, %eax
	addl	$7, %ecx
	movl	%ecx, 20(%rsp)
	movabsq	$14130315205570149, %rcx        # imm = 0x32337262646E65
.LBB0_218:
	movq	64(%rsp), %rdx                  # 8-byte Reload
	movq	%rcx, (%rdx,%rax)
.LBB0_219:
	xorl	%r14d, %r14d
	xorl	%ecx, %ecx
.LBB0_220:
	movq	32(%rsp), %r15                  # 8-byte Reload
	jmp	.LBB0_221
.LBB0_258:
	leaq	72(%rsp), %rdi
	leaq	112(%rsp), %r8
                                        # kill: def $ecx killed $ecx killed $rcx
	callq	parse_modrm
	movq	48(%rsp), %rdx                  # 8-byte Reload
	movl	%edx, %eax
	addl	$7, %edx
	movl	%edx, 20(%rsp)
	movabsq	$9117667750735725, %rcx         # imm = 0x20647873766F6D
	movq	64(%rsp), %rsi                  # 8-byte Reload
	movq	%rcx, (%rsi,%rax)
	movl	116(%rsp), %eax
	andl	$15, %eax
	leaq	R64(%rip), %rcx
	movq	(%rcx,%rax,8), %rax
	movzbl	(%rax), %ecx
	testb	%cl, %cl
	je	.LBB0_347
# %bb.259:
	incq	%rax
	movq	32(%rsp), %r15                  # 8-byte Reload
	jmp	.LBB0_261
	.p2align	4
.LBB0_260:                              #   in Loop: Header=BB0_261 Depth=1
	movzbl	(%rax), %ecx
	incq	%rax
	testb	%cl, %cl
	je	.LBB0_315
.LBB0_261:                              # =>This Inner Loop Header: Depth=1
	movl	20(%rsp), %edx
	leal	1(%rdx), %esi
	cmpl	16(%rsp), %esi
	jae	.LBB0_260
# %bb.262:                              #   in Loop: Header=BB0_261 Depth=1
	movq	8(%rsp), %rdi
	movl	%esi, 20(%rsp)
	movb	%cl, (%rdi,%rdx)
	movq	8(%rsp), %rcx
	movl	20(%rsp), %edx
	movb	$0, (%rcx,%rdx)
	jmp	.LBB0_260
.LBB0_263:
	movl	%r13d, %r9d
	andb	$-16, %r9b
	cmpb	$64, %r9b
	je	.LBB0_364
# %bb.264:
	movl	%r13d, %r9d
	andl	$-16, %r9d
	cmpl	$144, %r9d
	je	.LBB0_359
# %bb.265:
	cmpl	$128, %r9d
	jne	.LBB0_369
# %bb.266:
	cmpl	%eax, %r15d
	jae	.LBB0_415
# %bb.267:
	addl	$3, %r8d
	movl	%r8d, 84(%rsp)
	movzbl	(%rdi,%r15), %edx
	movl	%r8d, %r15d
	jmp	.LBB0_416
.LBB0_268:
	movl	%r13d, %r12d
	andl	$-16, %r12d
	cmpl	$112, %r12d
	jne	.LBB0_338
# %bb.269:
	cmpl	%eax, %r10d
	jae	.LBB0_375
# %bb.270:
	addl	$2, %r8d
	movl	%r8d, 84(%rsp)
	movsbq	(%rdi,%r10), %rsi
	jmp	.LBB0_376
.LBB0_271:
	cmpl	$46, %r13d
	jg	.LBB0_344
# %bb.272:
	cmpl	$44, %r13d
	je	.LBB0_412
# %bb.273:
	cmpl	$46, %r13d
	jne	.LBB0_410
# %bb.274:
	leaq	.L.str.24(%rip), %rax
	jmp	.LBB0_185
.LBB0_275:
	movl	$64, %ecx
.LBB0_276:
	leal	1(%rsi), %edx
	cmpl	%ecx, %edx
	jae	.LBB0_277
# %bb.284:
	movq	8(%rsp), %rcx
	movl	%edx, 20(%rsp)
	movl	%esi, %edx
	movb	$32, (%rcx,%rdx)
	movq	8(%rsp), %rcx
	movl	20(%rsp), %edx
	movb	$0, (%rcx,%rdx)
	movl	16(%rsp), %ecx
	movl	20(%rsp), %esi
	leal	1(%rsi), %edx
	cmpl	%ecx, %edx
	jb	.LBB0_285
.LBB0_278:
	cmpl	%ecx, %edx
	jae	.LBB0_279
.LBB0_286:
	movq	8(%rsp), %rcx
	movl	%edx, 20(%rsp)
	movl	%esi, %edx
	movb	$108, (%rcx,%rdx)
	movq	8(%rsp), %rcx
	movl	20(%rsp), %edx
	movb	$0, (%rcx,%rdx)
	movl	16(%rsp), %ecx
	movl	20(%rsp), %esi
	leal	1(%rsi), %edx
	cmpl	%ecx, %edx
	jb	.LBB0_287
.LBB0_280:
	cmpl	%ecx, %edx
	jae	.LBB0_281
.LBB0_288:
	movq	8(%rsp), %rcx
	movl	%edx, 20(%rsp)
	movl	%esi, %edx
	movb	$32, (%rcx,%rdx)
	movq	8(%rsp), %rcx
	movl	20(%rsp), %edx
	movb	$0, (%rcx,%rdx)
	movl	16(%rsp), %ecx
	movl	20(%rsp), %esi
	leal	1(%rsi), %edx
	cmpl	%ecx, %edx
	jb	.LBB0_289
.LBB0_282:
	cmpl	%ecx, %edx
	jae	.LBB0_283
.LBB0_290:
	movq	8(%rsp), %rcx
	movl	%edx, 20(%rsp)
	movl	%esi, %edx
	movb	$120, (%rcx,%rdx)
	movq	8(%rsp), %rcx
	movl	20(%rsp), %edx
	movb	$0, (%rcx,%rdx)
	testq	%rax, %rax
	jne	.LBB0_291
	jmp	.LBB0_462
.LBB0_277:
	cmpl	%ecx, %edx
	jae	.LBB0_278
.LBB0_285:
	movq	8(%rsp), %rcx
	movl	%edx, 20(%rsp)
	movl	%esi, %edx
	movb	$97, (%rcx,%rdx)
	movq	8(%rsp), %rcx
	movl	20(%rsp), %edx
	movb	$0, (%rcx,%rdx)
	movl	16(%rsp), %ecx
	movl	20(%rsp), %esi
	leal	1(%rsi), %edx
	cmpl	%ecx, %edx
	jb	.LBB0_286
.LBB0_279:
	cmpl	%ecx, %edx
	jae	.LBB0_280
.LBB0_287:
	movq	8(%rsp), %rcx
	movl	%edx, 20(%rsp)
	movl	%esi, %edx
	movb	$44, (%rcx,%rdx)
	movq	8(%rsp), %rcx
	movl	20(%rsp), %edx
	movb	$0, (%rcx,%rdx)
	movl	16(%rsp), %ecx
	movl	20(%rsp), %esi
	leal	1(%rsi), %edx
	cmpl	%ecx, %edx
	jb	.LBB0_288
.LBB0_281:
	cmpl	%ecx, %edx
	jae	.LBB0_282
.LBB0_289:
	movq	8(%rsp), %rcx
	movl	%edx, 20(%rsp)
	movl	%esi, %edx
	movb	$48, (%rcx,%rdx)
	movq	8(%rsp), %rcx
	movl	20(%rsp), %edx
	movb	$0, (%rcx,%rdx)
	movl	16(%rsp), %ecx
	movl	20(%rsp), %esi
	leal	1(%rsi), %edx
	cmpl	%ecx, %edx
	jb	.LBB0_290
.LBB0_283:
	testq	%rax, %rax
	je	.LBB0_462
.LBB0_291:
	xorl	%ecx, %ecx
	movq	%rax, %rdx
	.p2align	4
.LBB0_292:                              # =>This Inner Loop Header: Depth=1
	movl	%eax, %esi
	andl	$15, %esi
	leal	87(%rsi), %edi
	leal	48(%rsi), %r8d
	cmpl	$10, %esi
	movzbl	%r8b, %esi
	movzbl	%dil, %edi
	cmovbl	%esi, %edi
	movb	%dil, 112(%rsp,%rcx)
	incq	%rcx
	shrq	$4, %rdx
	cmpq	$15, %rax
	movq	%rdx, %rax
	ja	.LBB0_292
	jmp	.LBB0_294
	.p2align	4
.LBB0_293:                              #   in Loop: Header=BB0_294 Depth=1
	decq	%rcx
	je	.LBB0_85
.LBB0_294:                              # =>This Inner Loop Header: Depth=1
	movl	20(%rsp), %eax
	leal	1(%rax), %edx
	cmpl	16(%rsp), %edx
	jae	.LBB0_293
# %bb.295:                              #   in Loop: Header=BB0_294 Depth=1
	movzbl	111(%rsp,%rcx), %esi
	movq	8(%rsp), %rdi
	movl	%edx, 20(%rsp)
	movb	%sil, (%rdi,%rax)
	movq	8(%rsp), %rax
	movl	20(%rsp), %edx
	movb	$0, (%rax,%rdx)
	jmp	.LBB0_293
.LBB0_296:
	movl	$64, %ecx
	movq	32(%rsp), %r15                  # 8-byte Reload
	movq	48(%rsp), %rsi                  # 8-byte Reload
.LBB0_297:
	leal	1(%rsi), %edx
	cmpl	%ecx, %edx
	jae	.LBB0_299
# %bb.298:
	movq	8(%rsp), %rcx
	movl	%edx, 20(%rsp)
	movl	%esi, %edx
	movb	$32, (%rcx,%rdx)
	movq	8(%rsp), %rcx
	movl	20(%rsp), %edx
	movb	$0, (%rcx,%rdx)
.LBB0_299:
	cmpl	$2, %ebp
	je	.LBB0_317
# %bb.300:
	cmpl	$8, %ebp
	je	.LBB0_316
# %bb.301:
	cmpl	$4, %ebp
	jne	.LBB0_318
# %bb.302:
	leaq	R32(%rip), %rcx
	jmp	.LBB0_319
.LBB0_303:
	movl	$64, %eax
	movq	%r14, %rdx
.LBB0_304:
	leal	1(%rdx), %ecx
	cmpl	%eax, %ecx
	jae	.LBB0_306
# %bb.305:
	movq	8(%rsp), %rax
	movl	%ecx, 20(%rsp)
	movl	%edx, %ecx
	movb	$44, (%rax,%rcx)
	movq	8(%rsp), %rax
	movl	20(%rsp), %ecx
	movb	$0, (%rax,%rcx)
	movl	16(%rsp), %eax
	movl	20(%rsp), %edx
	leal	1(%rdx), %ecx
.LBB0_306:
	movq	32(%rsp), %r15                  # 8-byte Reload
	cmpl	%eax, %ecx
	jae	.LBB0_308
# %bb.307:
	movq	8(%rsp), %rax
	movl	%ecx, 20(%rsp)
	movl	%edx, %ecx
	movb	$32, (%rax,%rcx)
	movq	8(%rsp), %rax
	movl	20(%rsp), %ecx
	movb	$0, (%rax,%rcx)
.LBB0_308:
	leaq	8(%rsp), %rdi
	leaq	112(%rsp), %rsi
	movl	%ebx, %edx
	movl	28(%rsp), %ecx                  # 4-byte Reload
	xorl	%r8d, %r8d
	xorl	%r9d, %r9d
	callq	render_rm
	movl	16(%rsp), %edx
	movl	20(%rsp), %eax
	leal	1(%rax), %ecx
	cmpl	%edx, %ecx
	jae	.LBB0_310
# %bb.309:
	movq	8(%rsp), %rdx
	movl	%ecx, 20(%rsp)
	movb	$44, (%rdx,%rax)
	movq	8(%rsp), %rax
	movl	20(%rsp), %ecx
	movb	$0, (%rax,%rcx)
	movl	16(%rsp), %edx
	movl	20(%rsp), %eax
	leal	1(%rax), %ecx
.LBB0_310:
	cmpl	%edx, %ecx
	jae	.LBB0_312
# %bb.311:
	movq	8(%rsp), %rdx
	movl	%ecx, 20(%rsp)
	movl	%eax, %eax
	movb	$32, (%rdx,%rax)
	movq	8(%rsp), %rax
	movl	20(%rsp), %ecx
	movb	$0, (%rax,%rcx)
.LBB0_312:
	leaq	8(%rsp), %rdi
	movq	%r13, %rsi
.LBB0_313:
	callq	sb_0xhex
	jmp	.LBB0_138
.LBB0_314:
	leaq	8(%rcx), %rax
	movl	%eax, 20(%rsp)
	movb	$48, (%rdi,%rdx)
	movb	$0, 8(%rdi,%rcx)
	movq	32(%rsp), %r15                  # 8-byte Reload
	jmp	.LBB0_85
.LBB0_315:
	movl	16(%rsp), %eax
	movl	20(%rsp), %edx
	jmp	.LBB0_348
.LBB0_316:
	leaq	R64(%rip), %rcx
	jmp	.LBB0_319
.LBB0_317:
	leaq	R16(%rip), %rcx
	jmp	.LBB0_319
.LBB0_318:
	leaq	R8H(%rip), %rdx
	leaq	R8L(%rip), %rcx
	cmpb	$0, 104(%rsp)                   # 1-byte Folded Reload
	cmovneq	%rdx, %rcx
.LBB0_319:
	movq	(%rcx), %rcx
	movzbl	(%rcx), %edx
	testb	%dl, %dl
	je	.LBB0_324
# %bb.320:
	incq	%rcx
	jmp	.LBB0_322
	.p2align	4
.LBB0_321:                              #   in Loop: Header=BB0_322 Depth=1
	movzbl	(%rcx), %edx
	incq	%rcx
	testb	%dl, %dl
	je	.LBB0_324
.LBB0_322:                              # =>This Inner Loop Header: Depth=1
	movl	20(%rsp), %esi
	leal	1(%rsi), %edi
	cmpl	16(%rsp), %edi
	jae	.LBB0_321
# %bb.323:                              #   in Loop: Header=BB0_322 Depth=1
	movq	8(%rsp), %r8
	movl	%edi, 20(%rsp)
	movb	%dl, (%r8,%rsi)
	movq	8(%rsp), %rdx
	movl	20(%rsp), %esi
	movb	$0, (%rdx,%rsi)
	jmp	.LBB0_321
.LBB0_324:
	movl	16(%rsp), %esi
	movl	20(%rsp), %ecx
	leal	1(%rcx), %edx
	cmpl	%esi, %edx
	jae	.LBB0_325
# %bb.329:
	movq	8(%rsp), %rsi
	movl	%edx, 20(%rsp)
	movb	$44, (%rsi,%rcx)
	movq	8(%rsp), %rcx
	movl	20(%rsp), %edx
	movb	$0, (%rcx,%rdx)
	movl	16(%rsp), %esi
	movl	20(%rsp), %ecx
	leal	1(%rcx), %edx
	cmpl	%esi, %edx
	jb	.LBB0_330
.LBB0_326:
	cmpl	%esi, %edx
	jae	.LBB0_327
.LBB0_331:
	movq	8(%rsp), %rsi
	movl	%edx, 20(%rsp)
	movl	%ecx, %ecx
	movb	$48, (%rsi,%rcx)
	movq	8(%rsp), %rcx
	movl	20(%rsp), %edx
	movb	$0, (%rcx,%rdx)
	movl	16(%rsp), %esi
	movl	20(%rsp), %ecx
	leal	1(%rcx), %edx
	cmpl	%esi, %edx
	jb	.LBB0_332
.LBB0_328:
	testq	%rax, %rax
	jne	.LBB0_333
	jmp	.LBB0_462
.LBB0_325:
	cmpl	%esi, %edx
	jae	.LBB0_326
.LBB0_330:
	movq	8(%rsp), %rsi
	movl	%edx, 20(%rsp)
	movl	%ecx, %ecx
	movb	$32, (%rsi,%rcx)
	movq	8(%rsp), %rcx
	movl	20(%rsp), %edx
	movb	$0, (%rcx,%rdx)
	movl	16(%rsp), %esi
	movl	20(%rsp), %ecx
	leal	1(%rcx), %edx
	cmpl	%esi, %edx
	jb	.LBB0_331
.LBB0_327:
	cmpl	%esi, %edx
	jae	.LBB0_328
.LBB0_332:
	movq	8(%rsp), %rsi
	movl	%edx, 20(%rsp)
	movl	%ecx, %ecx
	movb	$120, (%rsi,%rcx)
	movq	8(%rsp), %rcx
	movl	20(%rsp), %edx
	movb	$0, (%rcx,%rdx)
	testq	%rax, %rax
	je	.LBB0_462
.LBB0_333:
	xorl	%ecx, %ecx
	movq	%rax, %rdx
	.p2align	4
.LBB0_334:                              # =>This Inner Loop Header: Depth=1
	movl	%eax, %esi
	andl	$15, %esi
	leal	87(%rsi), %edi
	leal	48(%rsi), %r8d
	cmpl	$10, %esi
	movzbl	%r8b, %esi
	movzbl	%dil, %edi
	cmovbl	%esi, %edi
	movb	%dil, 112(%rsp,%rcx)
	incq	%rcx
	shrq	$4, %rdx
	cmpq	$15, %rax
	movq	%rdx, %rax
	ja	.LBB0_334
	jmp	.LBB0_336
	.p2align	4
.LBB0_335:                              #   in Loop: Header=BB0_336 Depth=1
	decq	%rcx
	je	.LBB0_85
.LBB0_336:                              # =>This Inner Loop Header: Depth=1
	movl	20(%rsp), %eax
	leal	1(%rax), %edx
	cmpl	16(%rsp), %edx
	jae	.LBB0_335
# %bb.337:                              #   in Loop: Header=BB0_336 Depth=1
	movzbl	111(%rsp,%rcx), %esi
	movq	8(%rsp), %rdi
	movl	%edx, 20(%rsp)
	movb	%sil, (%rdi,%rax)
	movq	8(%rsp), %rax
	movl	20(%rsp), %edx
	movb	$0, (%rax,%rdx)
	jmp	.LBB0_335
.LBB0_338:
	movl	%r13d, %r12d
	andl	$-2, %r12d
	movl	%r12d, 100(%rsp)                # 4-byte Spill
	cmpl	$128, %r12d
	setne	111(%rsp)                       # 1-byte Folded Spill
	cmpl	$131, %r13d
	setne	%r12b
	testb	%r12b, 111(%rsp)                # 1-byte Folded Reload
	jne	.LBB0_382
# %bb.339:
	cmpl	$128, %r13d
	movl	$1, %ebx
	movl	44(%rsp), %ebp                  # 4-byte Reload
	movl	%ebp, %r12d
	cmovel	%ebx, %r12d
	leaq	72(%rsp), %r14
	leaq	112(%rsp), %r8
	movq	%r14, %rdi
                                        # kill: def $ecx killed $ecx killed $rcx
	callq	parse_modrm
	xorl	%eax, %eax
	cmpl	$2, %ebp
	setne	%al
	cmpl	$129, %r13d
	leal	2(%rax,%rax), %esi
	cmovnel	%ebx, %esi
	movq	%r14, %rdi
	callq	rd_imm_sext
	movq	%rax, %rbp
	movslq	160(%rsp), %rax
	leaq	ALU(%rip), %rcx
	movq	(%rcx,%rax,8), %rax
	movzbl	(%rax), %r8d
	testb	%r8b, %r8b
	je	.LBB0_464
# %bb.340:
	incq	%rax
	movq	32(%rsp), %r15                  # 8-byte Reload
	movl	28(%rsp), %ecx                  # 4-byte Reload
	jmp	.LBB0_342
.LBB0_341:                              #   in Loop: Header=BB0_342 Depth=1
	movzbl	(%rax), %r8d
	incq	%rax
	testb	%r8b, %r8b
	je	.LBB0_409
.LBB0_342:                              # =>This Inner Loop Header: Depth=1
	movl	20(%rsp), %edx
	leal	1(%rdx), %esi
	cmpl	16(%rsp), %esi
	jae	.LBB0_341
# %bb.343:                              #   in Loop: Header=BB0_342 Depth=1
	movq	8(%rsp), %rdi
	movl	%esi, 20(%rsp)
	movb	%r8b, (%rdi,%rdx)
	movq	8(%rsp), %rsi
	movl	20(%rsp), %edx
	movb	$0, (%rsi,%rdx)
	jmp	.LBB0_341
.LBB0_344:
	cmpl	$47, %r13d
	je	.LBB0_414
# %bb.345:
	cmpl	$87, %r13d
	jne	.LBB0_410
# %bb.346:
	leaq	.L.str.26(%rip), %rax
	jmp	.LBB0_185
.LBB0_347:
	movl	$64, %eax
	movq	32(%rsp), %r15                  # 8-byte Reload
.LBB0_348:
	leal	1(%rdx), %ecx
	cmpl	%eax, %ecx
	jae	.LBB0_349
# %bb.356:
	movq	8(%rsp), %rax
	movl	%ecx, 20(%rsp)
	movl	%edx, %ecx
	movb	$44, (%rax,%rcx)
	movq	8(%rsp), %rax
	movl	20(%rsp), %ecx
	movb	$0, (%rax,%rcx)
	movl	16(%rsp), %eax
	movl	20(%rsp), %edx
	leal	1(%rdx), %ecx
	cmpl	%eax, %ecx
	jb	.LBB0_357
.LBB0_350:
	cmpl	$0, 112(%rsp)
	je	.LBB0_137
.LBB0_351:
	movl	120(%rsp), %eax
	andl	$15, %eax
	leaq	R32(%rip), %rcx
	movq	(%rcx,%rax,8), %rax
	movzbl	(%rax), %ecx
	testb	%cl, %cl
	je	.LBB0_138
# %bb.352:
	incq	%rax
	jmp	.LBB0_354
.LBB0_353:                              #   in Loop: Header=BB0_354 Depth=1
	movzbl	(%rax), %ecx
	incq	%rax
	testb	%cl, %cl
	je	.LBB0_138
.LBB0_354:                              # =>This Inner Loop Header: Depth=1
	movl	20(%rsp), %edx
	leal	1(%rdx), %esi
	cmpl	16(%rsp), %esi
	jae	.LBB0_353
# %bb.355:                              #   in Loop: Header=BB0_354 Depth=1
	movq	8(%rsp), %rdi
	movl	%esi, 20(%rsp)
	movb	%cl, (%rdi,%rdx)
	movq	8(%rsp), %rcx
	movl	20(%rsp), %edx
	movb	$0, (%rcx,%rdx)
	jmp	.LBB0_353
.LBB0_349:
	cmpl	%eax, %ecx
	jae	.LBB0_350
.LBB0_357:
	movq	8(%rsp), %rax
	movl	%ecx, 20(%rsp)
	movl	%edx, %ecx
	movb	$32, (%rax,%rcx)
	movq	8(%rsp), %rax
	movl	20(%rsp), %ecx
	movb	$0, (%rax,%rcx)
	cmpl	$0, 112(%rsp)
	jne	.LBB0_351
.LBB0_137:
	leaq	8(%rsp), %rdi
	leaq	112(%rsp), %rsi
	callq	render_mem
.LBB0_138:
	cmpl	$0, 124(%rsp)
	setne	%cl
	je	.LBB0_139
.LBB0_532:
	movq	152(%rsp), %r14
	jmp	.LBB0_221
.LBB0_139:
	xorl	%r14d, %r14d
.LBB0_221:
	cmpl	$0, 88(%rsp)
	je	.LBB0_223
# %bb.222:
	movl	$1684103720, 24(%r12)           # imm = 0x64616228
	movw	$41, 28(%r12)
	movl	$0, 12(%r12)
	jmp	.LBB0_257
.LBB0_223:
	movl	84(%rsp), %r15d
	testb	%cl, %cl
	je	.LBB0_257
# %bb.224:
	movl	20(%rsp), %eax
	leal	1(%rax), %ecx
	cmpl	16(%rsp), %ecx
	jae	.LBB0_226
# %bb.225:
	movq	8(%rsp), %rdx
	movl	%ecx, 20(%rsp)
	movb	$32, (%rdx,%rax)
	movq	8(%rsp), %rax
	movl	20(%rsp), %ecx
	movb	$0, (%rax,%rcx)
.LBB0_226:
	movl	20(%rsp), %eax
	leal	1(%rax), %ecx
	cmpl	16(%rsp), %ecx
	jae	.LBB0_228
# %bb.227:
	movq	8(%rsp), %rdx
	movl	%ecx, 20(%rsp)
	movb	$32, (%rdx,%rax)
	movq	8(%rsp), %rax
	movl	20(%rsp), %ecx
	movb	$0, (%rax,%rcx)
.LBB0_228:
	movl	20(%rsp), %eax
	leal	1(%rax), %ecx
	cmpl	16(%rsp), %ecx
	jae	.LBB0_230
# %bb.229:
	movq	8(%rsp), %rdx
	movl	%ecx, 20(%rsp)
	movb	$32, (%rdx,%rax)
	movq	8(%rsp), %rax
	movl	20(%rsp), %ecx
	movb	$0, (%rax,%rcx)
.LBB0_230:
	movl	20(%rsp), %eax
	leal	1(%rax), %ecx
	cmpl	16(%rsp), %ecx
	jae	.LBB0_232
# %bb.231:
	movq	8(%rsp), %rdx
	movl	%ecx, 20(%rsp)
	movb	$32, (%rdx,%rax)
	movq	8(%rsp), %rax
	movl	20(%rsp), %ecx
	movb	$0, (%rax,%rcx)
.LBB0_232:
	movl	20(%rsp), %eax
	leal	1(%rax), %ecx
	cmpl	16(%rsp), %ecx
	jae	.LBB0_234
# %bb.233:
	movq	8(%rsp), %rdx
	movl	%ecx, 20(%rsp)
	movb	$32, (%rdx,%rax)
	movq	8(%rsp), %rax
	movl	20(%rsp), %ecx
	movb	$0, (%rax,%rcx)
.LBB0_234:
	movl	20(%rsp), %eax
	leal	1(%rax), %ecx
	cmpl	16(%rsp), %ecx
	jae	.LBB0_236
# %bb.235:
	movq	8(%rsp), %rdx
	movl	%ecx, 20(%rsp)
	movb	$32, (%rdx,%rax)
	movq	8(%rsp), %rax
	movl	20(%rsp), %ecx
	movb	$0, (%rax,%rcx)
.LBB0_236:
	movl	20(%rsp), %eax
	leal	1(%rax), %ecx
	cmpl	16(%rsp), %ecx
	jae	.LBB0_238
# %bb.237:
	movq	8(%rsp), %rdx
	movl	%ecx, 20(%rsp)
	movb	$32, (%rdx,%rax)
	movq	8(%rsp), %rax
	movl	20(%rsp), %ecx
	movb	$0, (%rax,%rcx)
.LBB0_238:
	movl	20(%rsp), %eax
	leal	1(%rax), %ecx
	cmpl	16(%rsp), %ecx
	jae	.LBB0_240
# %bb.239:
	movq	8(%rsp), %rdx
	movl	%ecx, 20(%rsp)
	movb	$32, (%rdx,%rax)
	movq	8(%rsp), %rax
	movl	20(%rsp), %ecx
	movb	$0, (%rax,%rcx)
.LBB0_240:
	movl	20(%rsp), %eax
	leal	1(%rax), %ecx
	cmpl	16(%rsp), %ecx
	jae	.LBB0_242
# %bb.241:
	movq	8(%rsp), %rdx
	movl	%ecx, 20(%rsp)
	movb	$35, (%rdx,%rax)
	movq	8(%rsp), %rax
	movl	20(%rsp), %ecx
	movb	$0, (%rax,%rcx)
.LBB0_242:
	movl	20(%rsp), %eax
	leal	1(%rax), %ecx
	cmpl	16(%rsp), %ecx
	jae	.LBB0_244
# %bb.243:
	movq	8(%rsp), %rdx
	movl	%ecx, 20(%rsp)
	movb	$32, (%rdx,%rax)
	movq	8(%rsp), %rax
	movl	20(%rsp), %ecx
	movb	$0, (%rax,%rcx)
.LBB0_244:
	addq	176(%rsp), %r14                 # 8-byte Folded Reload
	movl	16(%rsp), %edx
	movl	20(%rsp), %eax
	leal	1(%rax), %ecx
	cmpl	%edx, %ecx
	jae	.LBB0_246
# %bb.245:
	movq	8(%rsp), %rdx
	movl	%ecx, 20(%rsp)
	movb	$48, (%rdx,%rax)
	movq	8(%rsp), %rax
	movl	20(%rsp), %ecx
	movb	$0, (%rax,%rcx)
	movl	16(%rsp), %edx
	movl	20(%rsp), %eax
	leal	1(%rax), %ecx
.LBB0_246:
	addq	%r15, %r14
	cmpl	%edx, %ecx
	jae	.LBB0_248
# %bb.247:
	movq	8(%rsp), %rdx
	movl	%ecx, 20(%rsp)
	movl	%eax, %eax
	movb	$120, (%rdx,%rax)
	movq	8(%rsp), %rax
	movl	20(%rsp), %ecx
	movb	$0, (%rax,%rcx)
.LBB0_248:
	testq	%r14, %r14
	je	.LBB0_254
# %bb.249:
	xorl	%eax, %eax
	movq	%r14, %rcx
	.p2align	4
.LBB0_250:                              # =>This Inner Loop Header: Depth=1
	movl	%r14d, %edx
	andl	$15, %edx
	leal	87(%rdx), %esi
	leal	48(%rdx), %edi
	cmpl	$10, %edx
	movzbl	%dil, %edx
	movzbl	%sil, %esi
	cmovbl	%edx, %esi
	movb	%sil, 112(%rsp,%rax)
	incq	%rax
	shrq	$4, %rcx
	cmpq	$15, %r14
	movq	%rcx, %r14
	ja	.LBB0_250
	jmp	.LBB0_252
	.p2align	4
.LBB0_251:                              #   in Loop: Header=BB0_252 Depth=1
	decq	%rax
	je	.LBB0_256
.LBB0_252:                              # =>This Inner Loop Header: Depth=1
	movl	20(%rsp), %ecx
	leal	1(%rcx), %edx
	cmpl	16(%rsp), %edx
	jae	.LBB0_251
# %bb.253:                              #   in Loop: Header=BB0_252 Depth=1
	movzbl	111(%rsp,%rax), %esi
	movq	8(%rsp), %rdi
	movl	%edx, 20(%rsp)
	movb	%sil, (%rdi,%rcx)
	movq	8(%rsp), %rcx
	movl	20(%rsp), %edx
	movb	$0, (%rcx,%rdx)
	jmp	.LBB0_251
.LBB0_254:
	movl	20(%rsp), %eax
	leal	1(%rax), %ecx
	cmpl	16(%rsp), %ecx
	jae	.LBB0_256
# %bb.255:
	movq	8(%rsp), %rdx
	movl	%ecx, 20(%rsp)
	movb	$48, (%rdx,%rax)
	movq	8(%rsp), %rax
	movl	20(%rsp), %ecx
	movb	$0, (%rax,%rcx)
.LBB0_256:
	movl	84(%rsp), %r15d
.LBB0_257:
	cmpl	$1, %r15d
	adcl	$0, %r15d
	movl	%r15d, 8(%r12)
	movl	%r15d, %eax
	addq	$184, %rsp
	popq	%rbx
	popq	%r12
	popq	%r13
	popq	%r14
	popq	%r15
	popq	%rbp
	retq
.LBB0_359:
	leaq	72(%rsp), %rdi
	leaq	112(%rsp), %r8
                                        # kill: def $ecx killed $ecx killed $rcx
	callq	parse_modrm
	movq	48(%rsp), %rsi                  # 8-byte Reload
	movl	%esi, %eax
	addl	$3, %esi
	movl	%esi, 20(%rsp)
	movq	64(%rsp), %rcx                  # 8-byte Reload
	movl	$7628147, (%rcx,%rax)           # imm = 0x746573
	andl	$15, %r13d
	leaq	CC(%rip), %rax
	movq	(%rax,%r13,8), %rax
	movzbl	(%rax), %r8d
	testb	%r8b, %r8b
	je	.LBB0_431
# %bb.360:
	incq	%rax
	movq	32(%rsp), %r15                  # 8-byte Reload
	movl	28(%rsp), %ecx                  # 4-byte Reload
	jmp	.LBB0_362
.LBB0_361:                              #   in Loop: Header=BB0_362 Depth=1
	movzbl	(%rax), %r8d
	incq	%rax
	testb	%r8b, %r8b
	je	.LBB0_407
.LBB0_362:                              # =>This Inner Loop Header: Depth=1
	movl	20(%rsp), %edx
	leal	1(%rdx), %esi
	cmpl	16(%rsp), %esi
	jae	.LBB0_361
# %bb.363:                              #   in Loop: Header=BB0_362 Depth=1
	movq	8(%rsp), %rdi
	movl	%esi, 20(%rsp)
	movb	%r8b, (%rdi,%rdx)
	movq	8(%rsp), %rsi
	movl	20(%rsp), %edx
	movb	$0, (%rsi,%rdx)
	jmp	.LBB0_361
.LBB0_364:
	leaq	72(%rsp), %rdi
	leaq	112(%rsp), %r8
                                        # kill: def $ecx killed $ecx killed $rcx
	callq	parse_modrm
	movl	48(%rsp), %eax                  # 4-byte Reload
	leaq	4(%rax), %rcx
	movl	%ecx, 20(%rsp)
	movq	64(%rsp), %rdx                  # 8-byte Reload
	movl	$1987013987, (%rdx,%rax)        # imm = 0x766F6D63
	movb	$0, 4(%rdx,%rax)
	andl	$15, %r13d
	leaq	CC(%rip), %rax
	movq	(%rax,%r13,8), %rax
	movzbl	(%rax), %edx
	testb	%dl, %dl
	je	.LBB0_435
# %bb.365:
	incq	%rax
	movq	32(%rsp), %r15                  # 8-byte Reload
	movl	44(%rsp), %esi                  # 4-byte Reload
	movl	28(%rsp), %ebx                  # 4-byte Reload
	jmp	.LBB0_367
.LBB0_366:                              #   in Loop: Header=BB0_367 Depth=1
	movzbl	(%rax), %edx
	incq	%rax
	testb	%dl, %dl
	je	.LBB0_408
.LBB0_367:                              # =>This Inner Loop Header: Depth=1
	movl	20(%rsp), %ecx
	leal	1(%rcx), %r8d
	cmpl	16(%rsp), %r8d
	jae	.LBB0_366
# %bb.368:                              #   in Loop: Header=BB0_367 Depth=1
	movq	8(%rsp), %rdi
	movl	%r8d, 20(%rsp)
	movb	%dl, (%rdi,%rcx)
	movq	8(%rsp), %rcx
	movl	20(%rsp), %edx
	movb	$0, (%rcx,%rdx)
	jmp	.LBB0_366
.LBB0_369:
	movl	%r13d, %ebp
	andl	$-2, %ebp
	movl	%ebp, %eax
	orl	$8, %eax
	cmpl	$190, %eax
	jne	.LBB0_472
# %bb.370:
	leaq	72(%rsp), %rdi
	leaq	112(%rsp), %r8
                                        # kill: def $ecx killed $ecx killed $rcx
	callq	parse_modrm
	testb	$8, %r13b
	leaq	.L.str.12(%rip), %rcx
	leaq	.L.str.11(%rip), %rax
	cmoveq	%rcx, %rax
	movzbl	(%rax), %ecx
	testb	%cl, %cl
	movq	32(%rsp), %r15                  # 8-byte Reload
	movl	44(%rsp), %esi                  # 4-byte Reload
	je	.LBB0_397
# %bb.371:
	incq	%rax
	jmp	.LBB0_373
.LBB0_372:                              #   in Loop: Header=BB0_373 Depth=1
	movzbl	(%rax), %ecx
	incq	%rax
	testb	%cl, %cl
	je	.LBB0_397
.LBB0_373:                              # =>This Inner Loop Header: Depth=1
	movl	20(%rsp), %edx
	leal	1(%rdx), %r8d
	cmpl	16(%rsp), %r8d
	jae	.LBB0_372
# %bb.374:                              #   in Loop: Header=BB0_373 Depth=1
	movq	8(%rsp), %rdi
	movl	%r8d, 20(%rsp)
	movb	%cl, (%rdi,%rdx)
	movq	8(%rsp), %rcx
	movl	20(%rsp), %edx
	movb	$0, (%rcx,%rdx)
	jmp	.LBB0_372
.LBB0_375:
	movl	$1, 88(%rsp)
	xorl	%esi, %esi
.LBB0_376:
	movq	56(%rsp), %r12                  # 8-byte Reload
	movq	32(%rsp), %r15                  # 8-byte Reload
	movq	48(%rsp), %rdx                  # 8-byte Reload
	movl	%edx, %eax
	incl	%edx
	movl	%edx, 20(%rsp)
	movq	64(%rsp), %rcx                  # 8-byte Reload
	movw	$106, (%rcx,%rax)
	andl	$15, %r13d
	leaq	CC(%rip), %rax
	movq	(%rax,%r13,8), %rax
	movzbl	(%rax), %ecx
	testb	%cl, %cl
	je	.LBB0_393
# %bb.377:
	incq	%rax
	jmp	.LBB0_379
.LBB0_378:                              #   in Loop: Header=BB0_379 Depth=1
	movzbl	(%rax), %ecx
	incq	%rax
	testb	%cl, %cl
	je	.LBB0_381
.LBB0_379:                              # =>This Inner Loop Header: Depth=1
	movl	20(%rsp), %edx
	leal	1(%rdx), %edi
	cmpl	16(%rsp), %edi
	jae	.LBB0_378
# %bb.380:                              #   in Loop: Header=BB0_379 Depth=1
	movq	8(%rsp), %r8
	movl	%edi, 20(%rsp)
	movb	%cl, (%r8,%rdx)
	movq	8(%rsp), %rcx
	movl	20(%rsp), %edx
	movb	$0, (%rcx,%rdx)
	jmp	.LBB0_378
.LBB0_381:
	movl	16(%rsp), %eax
	movl	20(%rsp), %edx
	jmp	.LBB0_394
.LBB0_382:
	movl	100(%rsp), %r12d                # 4-byte Reload
	cmpl	$134, %r12d
	je	.LBB0_478
# %bb.383:
	cmpl	$132, %r12d
	jne	.LBB0_487
# %bb.384:
	leaq	72(%rsp), %rdi
	leaq	112(%rsp), %r14
                                        # kill: def $ecx killed $ecx killed $rcx
	movq	%r14, %r8
	callq	parse_modrm
	movq	48(%rsp), %rcx                  # 8-byte Reload
	leal	5(%rcx), %eax
	cmpl	$132, %r13d
	movl	%ecx, %ecx
	movq	64(%rsp), %rdx                  # 8-byte Reload
	movl	$1953719668, (%rdx,%rcx)        # imm = 0x74736574
	movl	%eax, 20(%rsp)
	movw	$32, 4(%rdx,%rcx)
	movl	$1, %eax
	movl	44(%rsp), %ebx                  # 4-byte Reload
	cmovel	%eax, %ebx
	leaq	8(%rsp), %rdi
	movq	%r14, %rsi
	movl	%ebx, %edx
	movl	28(%rsp), %ebp                  # 4-byte Reload
	movl	%ebp, %ecx
	xorl	%r8d, %r8d
	xorl	%r9d, %r9d
	callq	render_rm
	movl	16(%rsp), %edx
	movl	20(%rsp), %eax
	leal	1(%rax), %ecx
	cmpl	%edx, %ecx
	movq	56(%rsp), %r12                  # 8-byte Reload
	jae	.LBB0_386
# %bb.385:
	movq	8(%rsp), %rdx
	movl	%ecx, 20(%rsp)
	movb	$44, (%rdx,%rax)
	movq	8(%rsp), %rax
	movl	20(%rsp), %ecx
	movb	$0, (%rax,%rcx)
	movl	16(%rsp), %edx
	movl	20(%rsp), %eax
	leal	1(%rax), %ecx
.LBB0_386:
	movq	32(%rsp), %r15                  # 8-byte Reload
	cmpl	%edx, %ecx
	jae	.LBB0_388
# %bb.387:
	movq	8(%rsp), %rdx
	movl	%ecx, 20(%rsp)
	movl	%eax, %eax
	movb	$32, (%rdx,%rax)
	movq	8(%rsp), %rax
	movl	20(%rsp), %ecx
	movb	$0, (%rax,%rcx)
.LBB0_388:
	movl	116(%rsp), %edi
	movl	%ebx, %esi
	movl	%ebp, %edx
	callq	reg_name
	movzbl	(%rax), %ecx
	testb	%cl, %cl
	je	.LBB0_138
# %bb.389:
	incq	%rax
	jmp	.LBB0_391
.LBB0_390:                              #   in Loop: Header=BB0_391 Depth=1
	movzbl	(%rax), %ecx
	incq	%rax
	testb	%cl, %cl
	je	.LBB0_138
.LBB0_391:                              # =>This Inner Loop Header: Depth=1
	movl	20(%rsp), %edx
	leal	1(%rdx), %esi
	cmpl	16(%rsp), %esi
	jae	.LBB0_390
# %bb.392:                              #   in Loop: Header=BB0_391 Depth=1
	movq	8(%rsp), %rdi
	movl	%esi, 20(%rsp)
	movb	%cl, (%rdi,%rdx)
	movq	8(%rsp), %rcx
	movl	20(%rsp), %edx
	movb	$0, (%rcx,%rdx)
	jmp	.LBB0_390
.LBB0_393:
	movl	$64, %eax
.LBB0_394:
	leal	1(%rdx), %ecx
	cmpl	%eax, %ecx
	jae	.LBB0_396
# %bb.395:
	movq	8(%rsp), %rax
	movl	%ecx, 20(%rsp)
	movl	%edx, %ecx
	movb	$32, (%rax,%rcx)
	movq	8(%rsp), %rax
	movl	20(%rsp), %ecx
	movb	$0, (%rax,%rcx)
.LBB0_396:
	movl	$1, 12(%r12)
	movl	84(%rsp), %eax
	addq	176(%rsp), %rsi                 # 8-byte Folded Reload
	addq	%rax, %rsi
	movq	%rsi, 16(%r12)
	leaq	8(%rsp), %rdi
	callq	sb_0xhex
	jmp	.LBB0_85
.LBB0_397:
	movl	116(%rsp), %edi
	movl	28(%rsp), %edx                  # 4-byte Reload
	callq	reg_name
	movzbl	(%rax), %ecx
	testb	%cl, %cl
	je	.LBB0_402
# %bb.398:
	incq	%rax
	jmp	.LBB0_400
.LBB0_399:                              #   in Loop: Header=BB0_400 Depth=1
	movzbl	(%rax), %ecx
	incq	%rax
	testb	%cl, %cl
	je	.LBB0_402
.LBB0_400:                              # =>This Inner Loop Header: Depth=1
	movl	20(%rsp), %edx
	leal	1(%rdx), %esi
	cmpl	16(%rsp), %esi
	jae	.LBB0_399
# %bb.401:                              #   in Loop: Header=BB0_400 Depth=1
	movq	8(%rsp), %rdi
	movl	%esi, 20(%rsp)
	movb	%cl, (%rdi,%rdx)
	movq	8(%rsp), %rcx
	movl	20(%rsp), %edx
	movb	$0, (%rcx,%rdx)
	jmp	.LBB0_399
.LBB0_402:
	andl	$1, %r13d
	movl	16(%rsp), %edx
	movl	20(%rsp), %eax
	leal	1(%rax), %ecx
	cmpl	%edx, %ecx
	jae	.LBB0_404
# %bb.403:
	movq	8(%rsp), %rdx
	movl	%ecx, 20(%rsp)
	movb	$44, (%rdx,%rax)
	movq	8(%rsp), %rax
	movl	20(%rsp), %ecx
	movb	$0, (%rax,%rcx)
	movl	16(%rsp), %edx
	movl	20(%rsp), %eax
	leal	1(%rax), %ecx
.LBB0_404:
	incl	%r13d
	cmpl	%edx, %ecx
	jae	.LBB0_406
# %bb.405:
	movq	8(%rsp), %rdx
	movl	%ecx, 20(%rsp)
	movl	%eax, %eax
	movb	$32, (%rdx,%rax)
	movq	8(%rsp), %rax
	movl	20(%rsp), %ecx
	movb	$0, (%rax,%rcx)
.LBB0_406:
	xorl	%r14d, %r14d
	cmpl	$0, 112(%rsp)
	movl	$0, %r8d
	cmovel	%r13d, %r8d
	leaq	8(%rsp), %rdi
	leaq	112(%rsp), %rsi
	movl	%r13d, %edx
	movl	28(%rsp), %ecx                  # 4-byte Reload
.LBB0_530:
	xorl	%r9d, %r9d
	jmp	.LBB0_531
.LBB0_407:
	movl	16(%rsp), %eax
	movl	20(%rsp), %esi
	jmp	.LBB0_432
.LBB0_408:
	movl	16(%rsp), %eax
	movl	20(%rsp), %ecx
	jmp	.LBB0_436
.LBB0_409:
	movl	16(%rsp), %eax
	movl	20(%rsp), %esi
	jmp	.LBB0_465
.LBB0_410:
	cmpl	$84, %r13d
	jne	.LBB0_498
# %bb.411:
	leaq	.L.str.27(%rip), %rax
	jmp	.LBB0_185
.LBB0_412:
	leaq	.L.str.22(%rip), %rcx
	leaq	.L.str.23(%rip), %rax
.LBB0_413:
	testb	%r14b, %r14b
	cmovneq	%rcx, %rax
	jmp	.LBB0_185
.LBB0_414:
	leaq	.L.str.25(%rip), %rax
	jmp	.LBB0_185
.LBB0_415:
	movl	$1, 88(%rsp)
	xorl	%edx, %edx
.LBB0_416:
	cmpl	%eax, %r15d
	jae	.LBB0_418
# %bb.417:
	movl	%r15d, %ecx
	incl	%r15d
	movl	%r15d, 84(%rsp)
	movzbl	(%rdi,%rcx), %ecx
	shll	$8, %ecx
	jmp	.LBB0_419
.LBB0_418:
	movl	$1, 88(%rsp)
	xorl	%ecx, %ecx
.LBB0_419:
	cmpl	%eax, %r15d
	jae	.LBB0_421
# %bb.420:
	movl	%r15d, %esi
	incl	%r15d
	movl	%r15d, 84(%rsp)
	movzbl	(%rdi,%rsi), %esi
	shll	$16, %esi
	jmp	.LBB0_422
.LBB0_421:
	movl	$1, 88(%rsp)
	xorl	%esi, %esi
.LBB0_422:
	movq	64(%rsp), %r8                   # 8-byte Reload
	cmpl	%eax, %r15d
	jae	.LBB0_424
# %bb.423:
	leal	1(%r15), %eax
	movl	%eax, 84(%rsp)
	movl	%r15d, %eax
	movzbl	(%rdi,%rax), %eax
	shll	$24, %eax
	jmp	.LBB0_425
.LBB0_424:
	movl	$1, 88(%rsp)
	xorl	%eax, %eax
.LBB0_425:
	movq	32(%rsp), %r15                  # 8-byte Reload
	orl	%eax, %esi
	orl	%edx, %ecx
	orl	%esi, %ecx
	movq	48(%rsp), %rdi                  # 8-byte Reload
	movl	%edi, %eax
	incl	%edi
	movl	%edi, 20(%rsp)
	movw	$106, (%r8,%rax)
	andl	$15, %r13d
	leaq	CC(%rip), %rax
	movq	(%rax,%r13,8), %rax
	movzbl	(%rax), %edx
	testb	%dl, %dl
	je	.LBB0_449
# %bb.426:
	incq	%rax
	jmp	.LBB0_428
.LBB0_427:                              #   in Loop: Header=BB0_428 Depth=1
	movzbl	(%rax), %edx
	incq	%rax
	testb	%dl, %dl
	je	.LBB0_430
.LBB0_428:                              # =>This Inner Loop Header: Depth=1
	movl	20(%rsp), %esi
	leal	1(%rsi), %edi
	cmpl	16(%rsp), %edi
	jae	.LBB0_427
# %bb.429:                              #   in Loop: Header=BB0_428 Depth=1
	movq	8(%rsp), %r8
	movl	%edi, 20(%rsp)
	movb	%dl, (%r8,%rsi)
	movq	8(%rsp), %rdx
	movl	20(%rsp), %esi
	movb	$0, (%rdx,%rsi)
	jmp	.LBB0_427
.LBB0_430:
	movl	16(%rsp), %edx
	movl	20(%rsp), %edi
	jmp	.LBB0_450
.LBB0_431:
	movl	$64, %eax
	movq	32(%rsp), %r15                  # 8-byte Reload
	movl	28(%rsp), %ecx                  # 4-byte Reload
.LBB0_432:
	leal	1(%rsi), %edx
	cmpl	%eax, %edx
	jae	.LBB0_434
# %bb.433:
	movq	8(%rsp), %rax
	movl	%edx, 20(%rsp)
	movl	%esi, %edx
	movb	$32, (%rax,%rdx)
	movq	8(%rsp), %rax
	movl	20(%rsp), %edx
	movb	$0, (%rax,%rdx)
.LBB0_434:
	leaq	8(%rsp), %rdi
	leaq	112(%rsp), %rsi
	xorl	%r14d, %r14d
	movl	$1, %edx
	movl	$1, %r8d
	jmp	.LBB0_448
.LBB0_435:
	movl	$64, %eax
	movq	32(%rsp), %r15                  # 8-byte Reload
	movl	44(%rsp), %esi                  # 4-byte Reload
	movl	28(%rsp), %ebx                  # 4-byte Reload
.LBB0_436:
	leal	1(%rcx), %edx
	cmpl	%eax, %edx
	jae	.LBB0_438
# %bb.437:
	movq	8(%rsp), %rax
	movl	%edx, 20(%rsp)
	movl	%ecx, %ecx
	movb	$32, (%rax,%rcx)
	movq	8(%rsp), %rax
	movl	20(%rsp), %ecx
	movb	$0, (%rax,%rcx)
.LBB0_438:
	movl	116(%rsp), %edi
	movl	%ebx, %edx
	callq	reg_name
	movzbl	(%rax), %ecx
	testb	%cl, %cl
	je	.LBB0_443
# %bb.439:
	incq	%rax
	jmp	.LBB0_441
.LBB0_440:                              #   in Loop: Header=BB0_441 Depth=1
	movzbl	(%rax), %ecx
	incq	%rax
	testb	%cl, %cl
	je	.LBB0_443
.LBB0_441:                              # =>This Inner Loop Header: Depth=1
	movl	20(%rsp), %edx
	leal	1(%rdx), %esi
	cmpl	16(%rsp), %esi
	jae	.LBB0_440
# %bb.442:                              #   in Loop: Header=BB0_441 Depth=1
	movq	8(%rsp), %rdi
	movl	%esi, 20(%rsp)
	movb	%cl, (%rdi,%rdx)
	movq	8(%rsp), %rcx
	movl	20(%rsp), %edx
	movb	$0, (%rcx,%rdx)
	jmp	.LBB0_440
.LBB0_443:
	movl	16(%rsp), %edx
	movl	20(%rsp), %eax
	leal	1(%rax), %ecx
	cmpl	%edx, %ecx
	jae	.LBB0_445
# %bb.444:
	movq	8(%rsp), %rdx
	movl	%ecx, 20(%rsp)
	movb	$44, (%rdx,%rax)
	movq	8(%rsp), %rax
	movl	20(%rsp), %ecx
	movb	$0, (%rax,%rcx)
	movl	16(%rsp), %edx
	movl	20(%rsp), %eax
	leal	1(%rax), %ecx
.LBB0_445:
	cmpl	%edx, %ecx
	jae	.LBB0_447
# %bb.446:
	movq	8(%rsp), %rdx
	movl	%ecx, 20(%rsp)
	movl	%eax, %eax
	movb	$32, (%rdx,%rax)
	movq	8(%rsp), %rax
	movl	20(%rsp), %ecx
	movb	$0, (%rax,%rcx)
.LBB0_447:
	leaq	8(%rsp), %rdi
	leaq	112(%rsp), %rsi
	xorl	%r14d, %r14d
	movl	44(%rsp), %edx                  # 4-byte Reload
	movl	%ebx, %ecx
	xorl	%r8d, %r8d
.LBB0_448:
	xorl	%r9d, %r9d
	callq	render_rm
	xorl	%ecx, %ecx
	jmp	.LBB0_221
.LBB0_449:
	movl	$64, %edx
.LBB0_450:
	movslq	%ecx, %rax
	leal	1(%rdi), %ecx
	cmpl	%edx, %ecx
	jae	.LBB0_452
# %bb.451:
	movq	8(%rsp), %rdx
	movl	%ecx, 20(%rsp)
	movl	%edi, %ecx
	movb	$32, (%rdx,%rcx)
	movq	8(%rsp), %rcx
	movl	20(%rsp), %edx
	movb	$0, (%rcx,%rdx)
	movl	16(%rsp), %edx
	movl	20(%rsp), %edi
	leal	1(%rdi), %ecx
.LBB0_452:
	movl	$1, 12(%r12)
	movl	84(%rsp), %esi
	addq	176(%rsp), %rax                 # 8-byte Folded Reload
	addq	%rsi, %rax
	movq	%rax, 16(%r12)
	cmpl	%edx, %ecx
	jae	.LBB0_453
# %bb.460:
	movq	8(%rsp), %rdx
	movl	%ecx, 20(%rsp)
	movl	%edi, %ecx
	movb	$48, (%rdx,%rcx)
	movq	8(%rsp), %rcx
	movl	20(%rsp), %edx
	movb	$0, (%rcx,%rdx)
	movl	16(%rsp), %edx
	movl	20(%rsp), %edi
	leal	1(%rdi), %ecx
	cmpl	%edx, %ecx
	jb	.LBB0_461
.LBB0_454:
	testq	%rax, %rax
	je	.LBB0_462
.LBB0_455:
	xorl	%ecx, %ecx
	movq	%rax, %rdx
.LBB0_456:                              # =>This Inner Loop Header: Depth=1
	movl	%eax, %esi
	andl	$15, %esi
	leal	87(%rsi), %edi
	leal	48(%rsi), %r8d
	cmpl	$10, %esi
	movzbl	%r8b, %esi
	movzbl	%dil, %edi
	cmovbl	%esi, %edi
	movb	%dil, 112(%rsp,%rcx)
	incq	%rcx
	shrq	$4, %rdx
	cmpq	$15, %rax
	movq	%rdx, %rax
	ja	.LBB0_456
	jmp	.LBB0_458
.LBB0_457:                              #   in Loop: Header=BB0_458 Depth=1
	decq	%rcx
	je	.LBB0_85
.LBB0_458:                              # =>This Inner Loop Header: Depth=1
	movl	20(%rsp), %eax
	leal	1(%rax), %edx
	cmpl	16(%rsp), %edx
	jae	.LBB0_457
# %bb.459:                              #   in Loop: Header=BB0_458 Depth=1
	movzbl	111(%rsp,%rcx), %esi
	movq	8(%rsp), %rdi
	movl	%edx, 20(%rsp)
	movb	%sil, (%rdi,%rax)
	movq	8(%rsp), %rax
	movl	20(%rsp), %edx
	movb	$0, (%rax,%rdx)
	jmp	.LBB0_457
.LBB0_453:
	cmpl	%edx, %ecx
	jae	.LBB0_454
.LBB0_461:
	movq	8(%rsp), %rdx
	movl	%ecx, 20(%rsp)
	movl	%edi, %ecx
	movb	$120, (%rdx,%rcx)
	movq	8(%rsp), %rcx
	movl	20(%rsp), %edx
	movb	$0, (%rcx,%rdx)
	testq	%rax, %rax
	jne	.LBB0_455
.LBB0_462:
	movl	20(%rsp), %eax
	leal	1(%rax), %ecx
	cmpl	16(%rsp), %ecx
	jae	.LBB0_85
# %bb.463:
	movq	8(%rsp), %rdx
	movl	%ecx, 20(%rsp)
	movb	$48, (%rdx,%rax)
	movq	8(%rsp), %rax
	movl	20(%rsp), %ecx
	movb	$0, (%rax,%rcx)
	jmp	.LBB0_85
.LBB0_464:
	movl	$64, %eax
	movq	32(%rsp), %r15                  # 8-byte Reload
	movl	28(%rsp), %ecx                  # 4-byte Reload
	movq	48(%rsp), %rsi                  # 8-byte Reload
.LBB0_465:
	leal	1(%rsi), %edx
	cmpl	%eax, %edx
	jae	.LBB0_467
# %bb.466:
	movq	8(%rsp), %rax
	movl	%edx, 20(%rsp)
	movl	%esi, %edx
	movb	$32, (%rax,%rdx)
	movq	8(%rsp), %rax
	movl	20(%rsp), %edx
	movb	$0, (%rax,%rdx)
.LBB0_467:
	xorl	%r8d, %r8d
	cmpl	$0, 112(%rsp)
	cmovel	%r12d, %r8d
	leaq	8(%rsp), %rdi
	leaq	112(%rsp), %rsi
	movl	%r12d, %edx
	xorl	%r9d, %r9d
	callq	render_rm
	movl	16(%rsp), %edx
	movl	20(%rsp), %eax
	leal	1(%rax), %ecx
	cmpl	%edx, %ecx
	jae	.LBB0_469
# %bb.468:
	movq	8(%rsp), %rdx
	movl	%ecx, 20(%rsp)
	movb	$44, (%rdx,%rax)
	movq	8(%rsp), %rax
	movl	20(%rsp), %ecx
	movb	$0, (%rax,%rcx)
	movl	16(%rsp), %edx
	movl	20(%rsp), %eax
	leal	1(%rax), %ecx
.LBB0_469:
	movq	56(%rsp), %r12                  # 8-byte Reload
	cmpl	%edx, %ecx
	jae	.LBB0_471
# %bb.470:
	movq	8(%rsp), %rdx
	movl	%ecx, 20(%rsp)
	movl	%eax, %eax
	movb	$32, (%rdx,%rax)
	movq	8(%rsp), %rax
	movl	20(%rsp), %ecx
	movb	$0, (%rax,%rcx)
.LBB0_471:
	leaq	8(%rsp), %rdi
	movq	%rbp, %rsi
	jmp	.LBB0_313
.LBB0_472:
	cmpl	$175, %r13d
	movq	32(%rsp), %r15                  # 8-byte Reload
	jne	.LBB0_48
# %bb.473:
	leaq	72(%rsp), %rdi
	leaq	112(%rsp), %r8
                                        # kill: def $ecx killed $ecx killed $rcx
	callq	parse_modrm
	movq	48(%rsp), %rcx                  # 8-byte Reload
	movl	%ecx, %eax
	movq	64(%rsp), %rdx                  # 8-byte Reload
	movl	$1819635049, (%rdx,%rax)        # imm = 0x6C756D69
	addl	$5, %ecx
	movq	%rcx, %r14
	movl	%ecx, 20(%rsp)
	movw	$32, 4(%rdx,%rax)
	movl	116(%rsp), %edi
	movl	44(%rsp), %esi                  # 4-byte Reload
	movl	28(%rsp), %ebx                  # 4-byte Reload
	movl	%ebx, %edx
	callq	reg_name
	movzbl	(%rax), %ecx
	testb	%cl, %cl
	je	.LBB0_524
# %bb.474:
	incq	%rax
	jmp	.LBB0_476
.LBB0_475:                              #   in Loop: Header=BB0_476 Depth=1
	movzbl	(%rax), %ecx
	incq	%rax
	testb	%cl, %cl
	je	.LBB0_515
.LBB0_476:                              # =>This Inner Loop Header: Depth=1
	movl	20(%rsp), %edx
	leal	1(%rdx), %esi
	cmpl	16(%rsp), %esi
	jae	.LBB0_475
# %bb.477:                              #   in Loop: Header=BB0_476 Depth=1
	movq	8(%rsp), %rdi
	movl	%esi, 20(%rsp)
	movb	%cl, (%rdi,%rdx)
	movq	8(%rsp), %rcx
	movl	20(%rsp), %edx
	movb	$0, (%rcx,%rdx)
	jmp	.LBB0_475
.LBB0_478:
	leaq	72(%rsp), %rdi
	leaq	112(%rsp), %r14
                                        # kill: def $ecx killed $ecx killed $rcx
	movq	%r14, %r8
	callq	parse_modrm
	movq	48(%rsp), %rcx                  # 8-byte Reload
	leal	5(%rcx), %eax
	cmpl	$134, %r13d
	movl	%ecx, %ecx
	movq	64(%rsp), %rdx                  # 8-byte Reload
	movl	$1734894456, (%rdx,%rcx)        # imm = 0x67686378
	movl	%eax, 20(%rsp)
	movw	$32, 4(%rdx,%rcx)
	movl	$1, %eax
	movl	44(%rsp), %ebx                  # 4-byte Reload
	cmovel	%eax, %ebx
	leaq	8(%rsp), %rdi
	movq	%r14, %rsi
	movl	%ebx, %edx
	movl	28(%rsp), %ebp                  # 4-byte Reload
	movl	%ebp, %ecx
	xorl	%r8d, %r8d
	xorl	%r9d, %r9d
	callq	render_rm
	movl	16(%rsp), %edx
	movl	20(%rsp), %eax
	leal	1(%rax), %ecx
	cmpl	%edx, %ecx
	movq	56(%rsp), %r12                  # 8-byte Reload
	jae	.LBB0_480
# %bb.479:
	movq	8(%rsp), %rdx
	movl	%ecx, 20(%rsp)
	movb	$44, (%rdx,%rax)
	movq	8(%rsp), %rax
	movl	20(%rsp), %ecx
	movb	$0, (%rax,%rcx)
	movl	16(%rsp), %edx
	movl	20(%rsp), %eax
	leal	1(%rax), %ecx
.LBB0_480:
	movq	32(%rsp), %r15                  # 8-byte Reload
	cmpl	%edx, %ecx
	jae	.LBB0_482
# %bb.481:
	movq	8(%rsp), %rdx
	movl	%ecx, 20(%rsp)
	movl	%eax, %eax
	movb	$32, (%rdx,%rax)
	movq	8(%rsp), %rax
	movl	20(%rsp), %ecx
	movb	$0, (%rax,%rcx)
.LBB0_482:
	movl	116(%rsp), %edi
	movl	%ebx, %esi
	movl	%ebp, %edx
	callq	reg_name
	movzbl	(%rax), %ecx
	testb	%cl, %cl
	je	.LBB0_85
# %bb.483:
	incq	%rax
	jmp	.LBB0_485
.LBB0_484:                              #   in Loop: Header=BB0_485 Depth=1
	movzbl	(%rax), %ecx
	incq	%rax
	testb	%cl, %cl
	je	.LBB0_85
.LBB0_485:                              # =>This Inner Loop Header: Depth=1
	movl	20(%rsp), %edx
	leal	1(%rdx), %esi
	cmpl	16(%rsp), %esi
	jae	.LBB0_484
# %bb.486:                              #   in Loop: Header=BB0_485 Depth=1
	movq	8(%rsp), %rdi
	movl	%esi, 20(%rsp)
	movb	%cl, (%rdi,%rdx)
	movq	8(%rsp), %rcx
	movl	20(%rsp), %edx
	movb	$0, (%rcx,%rdx)
	jmp	.LBB0_484
.LBB0_487:
	movl	%r13d, %r12d
	andl	$-4, %r12d
	cmpl	$136, %r12d
	jne	.LBB0_503
# %bb.488:
	testb	$1, %r13b
	movl	$1, %eax
	movl	44(%rsp), %ebp                  # 4-byte Reload
	cmovel	%eax, %ebp
	leaq	72(%rsp), %rdi
	leaq	112(%rsp), %r8
                                        # kill: def $ecx killed $ecx killed $rcx
	callq	parse_modrm
	movl	48(%rsp), %eax                  # 4-byte Reload
	leaq	4(%rax), %rbx
	movl	%ebx, 20(%rsp)
	movq	64(%rsp), %rcx                  # 8-byte Reload
	movl	$544632685, (%rcx,%rax)         # imm = 0x20766F6D
	movb	$0, 4(%rcx,%rax)
	testb	$2, %r13b
	jne	.LBB0_519
# %bb.489:
	leaq	8(%rsp), %rdi
	leaq	112(%rsp), %rsi
	movl	%ebp, %edx
	movl	28(%rsp), %ebx                  # 4-byte Reload
	movl	%ebx, %ecx
	xorl	%r8d, %r8d
	xorl	%r9d, %r9d
	callq	render_rm
	movl	16(%rsp), %edx
	movl	20(%rsp), %eax
	leal	1(%rax), %ecx
	cmpl	%edx, %ecx
	movq	32(%rsp), %r15                  # 8-byte Reload
	jae	.LBB0_491
# %bb.490:
	movq	8(%rsp), %rdx
	movl	%ecx, 20(%rsp)
	movb	$44, (%rdx,%rax)
	movq	8(%rsp), %rax
	movl	20(%rsp), %ecx
	movb	$0, (%rax,%rcx)
	movl	16(%rsp), %edx
	movl	20(%rsp), %eax
	leal	1(%rax), %ecx
.LBB0_491:
	movq	56(%rsp), %r12                  # 8-byte Reload
	cmpl	%edx, %ecx
	jae	.LBB0_493
# %bb.492:
	movq	8(%rsp), %rdx
	movl	%ecx, 20(%rsp)
	movl	%eax, %eax
	movb	$32, (%rdx,%rax)
	movq	8(%rsp), %rax
	movl	20(%rsp), %ecx
	movb	$0, (%rax,%rcx)
.LBB0_493:
	movl	116(%rsp), %edi
	movl	%ebp, %esi
	movl	%ebx, %edx
	callq	reg_name
	movzbl	(%rax), %ecx
	testb	%cl, %cl
	je	.LBB0_138
# %bb.494:
	incq	%rax
	jmp	.LBB0_496
.LBB0_495:                              #   in Loop: Header=BB0_496 Depth=1
	movzbl	(%rax), %ecx
	incq	%rax
	testb	%cl, %cl
	je	.LBB0_138
.LBB0_496:                              # =>This Inner Loop Header: Depth=1
	movl	20(%rsp), %edx
	leal	1(%rdx), %esi
	cmpl	16(%rsp), %esi
	jae	.LBB0_495
# %bb.497:                              #   in Loop: Header=BB0_496 Depth=1
	movq	8(%rsp), %rdi
	movl	%esi, 20(%rsp)
	movb	%cl, (%rdi,%rdx)
	movq	8(%rsp), %rcx
	movl	20(%rsp), %edx
	movb	$0, (%rcx,%rdx)
	jmp	.LBB0_495
.LBB0_498:
	movzbl	%r13b, %eax
	cmpl	$91, %eax
	jle	.LBB0_511
# %bb.499:
	cmpl	$109, %eax
	jg	.LBB0_516
# %bb.500:
	cmpl	$92, %eax
	je	.LBB0_549
# %bb.501:
	cmpl	$94, %eax
	jne	.LBB0_547
# %bb.502:
	testl	%ebx, %ebx
	leaq	.L.str.39(%rip), %rax
	leaq	.L.str.38(%rip), %rcx
	cmoveq	%rax, %rcx
	leaq	.L.str.37(%rip), %rax
	jmp	.LBB0_183
.LBB0_503:
	cmpl	$144, %r13d
	je	.LBB0_536
# %bb.504:
	cmpl	$143, %r13d
	je	.LBB0_534
# %bb.505:
	cmpl	$141, %r13d
	jne	.LBB0_541
# %bb.506:
	leaq	72(%rsp), %rdi
	leaq	112(%rsp), %r8
                                        # kill: def $ecx killed $ecx killed $rcx
	callq	parse_modrm
	movl	48(%rsp), %eax                  # 4-byte Reload
	leaq	4(%rax), %rbx
	movl	%ebx, 20(%rsp)
	movq	64(%rsp), %rcx                  # 8-byte Reload
	movl	$543253868, (%rcx,%rax)         # imm = 0x2061656C
	movb	$0, 4(%rcx,%rax)
	movl	116(%rsp), %edi
	movl	44(%rsp), %esi                  # 4-byte Reload
	movl	28(%rsp), %edx                  # 4-byte Reload
	callq	reg_name
	movzbl	(%rax), %ecx
	testb	%cl, %cl
	je	.LBB0_567
# %bb.507:
	incq	%rax
	jmp	.LBB0_509
.LBB0_508:                              #   in Loop: Header=BB0_509 Depth=1
	movzbl	(%rax), %ecx
	incq	%rax
	testb	%cl, %cl
	je	.LBB0_558
.LBB0_509:                              # =>This Inner Loop Header: Depth=1
	movl	20(%rsp), %edx
	leal	1(%rdx), %esi
	cmpl	16(%rsp), %esi
	jae	.LBB0_508
# %bb.510:                              #   in Loop: Header=BB0_509 Depth=1
	movq	8(%rsp), %rdi
	movl	%esi, 20(%rsp)
	movb	%cl, (%rdi,%rdx)
	movq	8(%rsp), %rcx
	movl	20(%rsp), %edx
	movb	$0, (%rcx,%rdx)
	jmp	.LBB0_508
.LBB0_511:
	cmpl	$88, %eax
	je	.LBB0_546
# %bb.512:
	cmpl	$89, %eax
	je	.LBB0_551
# %bb.513:
	cmpl	$90, %eax
	jne	.LBB0_547
# %bb.514:
	testl	%ebx, %ebx
	leaq	.L.str.42(%rip), %rax
	leaq	.L.str.41(%rip), %rcx
	cmoveq	%rax, %rcx
	leaq	.L.str.40(%rip), %rax
	jmp	.LBB0_183
.LBB0_515:
	movl	16(%rsp), %eax
	movl	20(%rsp), %edx
	jmp	.LBB0_525
.LBB0_516:
	cmpl	$110, %eax
	je	.LBB0_550
# %bb.517:
	cmpl	$126, %eax
	jne	.LBB0_547
# %bb.518:
	leaq	.L.str.44(%rip), %rcx
	leaq	.L.str.43(%rip), %rax
	jmp	.LBB0_413
.LBB0_519:
	movl	116(%rsp), %edi
	movl	%ebp, %esi
	movl	28(%rsp), %r14d                 # 4-byte Reload
	movl	%r14d, %edx
	callq	reg_name
	movzbl	(%rax), %ecx
	testb	%cl, %cl
	movq	32(%rsp), %r15                  # 8-byte Reload
	je	.LBB0_552
# %bb.520:
	incq	%rax
	movq	56(%rsp), %r12                  # 8-byte Reload
	jmp	.LBB0_522
.LBB0_521:                              #   in Loop: Header=BB0_522 Depth=1
	movzbl	(%rax), %ecx
	incq	%rax
	testb	%cl, %cl
	je	.LBB0_533
.LBB0_522:                              # =>This Inner Loop Header: Depth=1
	movl	20(%rsp), %edx
	leal	1(%rdx), %esi
	cmpl	16(%rsp), %esi
	jae	.LBB0_521
# %bb.523:                              #   in Loop: Header=BB0_522 Depth=1
	movq	8(%rsp), %rdi
	movl	%esi, 20(%rsp)
	movb	%cl, (%rdi,%rdx)
	movq	8(%rsp), %rcx
	movl	20(%rsp), %edx
	movb	$0, (%rcx,%rdx)
	jmp	.LBB0_521
.LBB0_524:
	movl	$64, %eax
	movq	%r14, %rdx
.LBB0_525:
	leal	1(%rdx), %ecx
	cmpl	%eax, %ecx
	jae	.LBB0_527
# %bb.526:
	movq	8(%rsp), %rax
	movl	%ecx, 20(%rsp)
	movl	%edx, %ecx
	movb	$44, (%rax,%rcx)
	movq	8(%rsp), %rax
	movl	20(%rsp), %ecx
	movb	$0, (%rax,%rcx)
	movl	16(%rsp), %eax
	movl	20(%rsp), %edx
	leal	1(%rdx), %ecx
.LBB0_527:
	cmpl	%eax, %ecx
	jae	.LBB0_529
# %bb.528:
	movq	8(%rsp), %rax
	movl	%ecx, 20(%rsp)
	movl	%edx, %ecx
	movb	$32, (%rax,%rcx)
	movq	8(%rsp), %rax
	movl	20(%rsp), %ecx
	movb	$0, (%rax,%rcx)
.LBB0_529:
	leaq	8(%rsp), %rdi
	leaq	112(%rsp), %rsi
	xorl	%r14d, %r14d
	movl	44(%rsp), %edx                  # 4-byte Reload
	movl	%ebx, %ecx
	xorl	%r8d, %r8d
	jmp	.LBB0_530
.LBB0_533:
	movl	16(%rsp), %eax
	movl	20(%rsp), %ebx
	jmp	.LBB0_553
.LBB0_534:
	leaq	72(%rsp), %rdi
	leaq	112(%rsp), %r13
                                        # kill: def $ecx killed $ecx killed $rcx
	movq	%r13, %r8
	callq	parse_modrm
	movl	48(%rsp), %eax                  # 4-byte Reload
	leaq	4(%rax), %rcx
	movl	%ecx, 20(%rsp)
	movq	64(%rsp), %rcx                  # 8-byte Reload
	movl	$544239472, (%rcx,%rax)         # imm = 0x20706F70
	movb	$0, 4(%rcx,%rax)
	xorl	%r8d, %r8d
	cmpl	$0, 112(%rsp)
	sete	%r8b
	shll	$3, %r8d
	leaq	8(%rsp), %rdi
	xorl	%r14d, %r14d
	movq	%r13, %rsi
	movl	$8, %edx
.LBB0_535:
	movl	28(%rsp), %ecx                  # 4-byte Reload
	xorl	%r9d, %r9d
	callq	render_rm
	jmp	.LBB0_705
.LBB0_536:
	testl	%r9d, %r9d
	leaq	.L.str.3(%rip), %rcx
	leaq	.L.str.57(%rip), %rax
	cmoveq	%rcx, %rax
	movzbl	(%rax), %edx
	testb	%dl, %dl
	je	.LBB0_704
# %bb.537:
	incq	%rax
	movq	56(%rsp), %r12                  # 8-byte Reload
	movq	32(%rsp), %r15                  # 8-byte Reload
	jmp	.LBB0_539
.LBB0_538:                              #   in Loop: Header=BB0_539 Depth=1
	movzbl	(%rax), %edx
	incq	%rax
	xorl	%r14d, %r14d
	movl	$0, %ecx
	testb	%dl, %dl
	je	.LBB0_221
.LBB0_539:                              # =>This Inner Loop Header: Depth=1
	movl	20(%rsp), %ecx
	leal	1(%rcx), %esi
	cmpl	16(%rsp), %esi
	jae	.LBB0_538
# %bb.540:                              #   in Loop: Header=BB0_539 Depth=1
	movq	8(%rsp), %rdi
	movl	%esi, 20(%rsp)
	movb	%dl, (%rdi,%rcx)
	movq	8(%rsp), %rcx
	movl	20(%rsp), %edx
	movb	$0, (%rcx,%rdx)
	jmp	.LBB0_538
.LBB0_541:
	leal	111(%r13), %r12d
	cmpb	$6, %r12b
	ja	.LBB0_559
# %bb.542:
	movq	48(%rsp), %rdx                  # 8-byte Reload
	movl	%edx, %eax
	movq	64(%rsp), %rdi                  # 8-byte Reload
	movl	$1734894456, (%rdi,%rax)        # imm = 0x67686378
	addl	$5, %edx
	movq	%rdx, %rsi
	movl	%edx, 20(%rsp)
	movw	$32, 4(%rdi,%rax)
	cmpl	$2, 44(%rsp)                    # 4-byte Folded Reload
	je	.LBB0_584
# %bb.543:
	cmpl	$8, 44(%rsp)                    # 4-byte Folded Reload
	je	.LBB0_583
# %bb.544:
	cmpl	$4, 44(%rsp)                    # 4-byte Folded Reload
	jne	.LBB0_585
# %bb.545:
	leaq	R32(%rip), %rax
	jmp	.LBB0_586
.LBB0_546:
	testl	%ebx, %ebx
	leaq	.L.str.30(%rip), %rax
	leaq	.L.str.29(%rip), %rcx
	cmoveq	%rax, %rcx
	leaq	.L.str.28(%rip), %rax
	jmp	.LBB0_183
.LBB0_547:
	movl	%r13d, %eax
	andl	$-17, %eax
	cmpl	$111, %eax
	jne	.LBB0_566
# %bb.548:
	leaq	.L.str.45(%rip), %rcx
	leaq	.L.str.46(%rip), %rax
	jmp	.LBB0_413
.LBB0_549:
	testl	%ebx, %ebx
	leaq	.L.str.36(%rip), %rax
	leaq	.L.str.35(%rip), %rcx
	cmoveq	%rax, %rcx
	leaq	.L.str.34(%rip), %rax
	jmp	.LBB0_183
.LBB0_550:
	leaq	.L.str.43(%rip), %rax
	jmp	.LBB0_185
.LBB0_551:
	testl	%ebx, %ebx
	leaq	.L.str.33(%rip), %rax
	leaq	.L.str.32(%rip), %rcx
	cmoveq	%rax, %rcx
	leaq	.L.str.31(%rip), %rax
	jmp	.LBB0_183
.LBB0_552:
	movl	$64, %eax
	movq	56(%rsp), %r12                  # 8-byte Reload
.LBB0_553:
	leal	1(%rbx), %ecx
	cmpl	%eax, %ecx
	jae	.LBB0_555
# %bb.554:
	movq	8(%rsp), %rax
	movl	%ecx, 20(%rsp)
	movl	%ebx, %ecx
	movb	$44, (%rax,%rcx)
	movq	8(%rsp), %rax
	movl	20(%rsp), %ecx
	movb	$0, (%rax,%rcx)
	movl	16(%rsp), %eax
	movl	20(%rsp), %ebx
	leal	1(%rbx), %ecx
.LBB0_555:
	cmpl	%eax, %ecx
	jae	.LBB0_557
# %bb.556:
	movq	8(%rsp), %rax
	movl	%ecx, 20(%rsp)
	movl	%ebx, %ecx
	movb	$32, (%rax,%rcx)
	movq	8(%rsp), %rax
	movl	20(%rsp), %ecx
	movb	$0, (%rax,%rcx)
.LBB0_557:
	leaq	8(%rsp), %rdi
	leaq	112(%rsp), %rsi
	movl	%ebp, %edx
	movl	%r14d, %ecx
	xorl	%r8d, %r8d
	xorl	%r9d, %r9d
	callq	render_rm
	jmp	.LBB0_138
.LBB0_558:
	movl	16(%rsp), %eax
	movl	20(%rsp), %ebx
	jmp	.LBB0_568
.LBB0_559:
	cmpq	$153, %r13
	je	.LBB0_575
# %bb.560:
	cmpl	$152, %r13d
	jne	.LBB0_580
# %bb.561:
	leaq	.L.str.60(%rip), %rax
	leaq	.L.str.59(%rip), %rcx
	testb	%r14b, %r14b
	cmovneq	%rax, %rcx
	leaq	.L.str.58(%rip), %rax
	testb	%r11b, %r11b
	cmovneq	%rcx, %rax
	movzbl	(%rax), %edx
	testb	%dl, %dl
	je	.LBB0_704
# %bb.562:
	incq	%rax
	movq	56(%rsp), %r12                  # 8-byte Reload
	movq	32(%rsp), %r15                  # 8-byte Reload
	jmp	.LBB0_564
.LBB0_563:                              #   in Loop: Header=BB0_564 Depth=1
	movzbl	(%rax), %edx
	incq	%rax
	xorl	%r14d, %r14d
	movl	$0, %ecx
	testb	%dl, %dl
	je	.LBB0_221
.LBB0_564:                              # =>This Inner Loop Header: Depth=1
	movl	20(%rsp), %ecx
	leal	1(%rcx), %esi
	cmpl	16(%rsp), %esi
	jae	.LBB0_563
# %bb.565:                              #   in Loop: Header=BB0_564 Depth=1
	movq	8(%rsp), %rdi
	movl	%esi, 20(%rsp)
	movb	%dl, (%rdi,%rcx)
	movq	8(%rsp), %rcx
	movl	20(%rsp), %edx
	movb	$0, (%rcx,%rdx)
	jmp	.LBB0_563
.LBB0_566:
	cmpl	$239, %r13d
	leaq	.L.str.47(%rip), %rcx
	leaq	.L.str.14(%rip), %rax
	jmp	.LBB0_184
.LBB0_567:
	movl	$64, %eax
.LBB0_568:
	leal	1(%rbx), %ecx
	cmpl	%eax, %ecx
	jae	.LBB0_570
# %bb.569:
	movq	8(%rsp), %rax
	movl	%ecx, 20(%rsp)
	movl	%ebx, %ecx
	movb	$44, (%rax,%rcx)
	movq	8(%rsp), %rax
	movl	20(%rsp), %ecx
	movb	$0, (%rax,%rcx)
	movl	16(%rsp), %eax
	movl	20(%rsp), %ebx
	leal	1(%rbx), %ecx
.LBB0_570:
	cmpl	%eax, %ecx
	jae	.LBB0_572
# %bb.571:
	movq	8(%rsp), %rax
	movl	%ecx, 20(%rsp)
	movl	%ebx, %ecx
	movb	$32, (%rax,%rcx)
	movq	8(%rsp), %rax
	movl	20(%rsp), %ecx
	movb	$0, (%rax,%rcx)
.LBB0_572:
	leaq	8(%rsp), %rdi
	leaq	112(%rsp), %rsi
	callq	render_mem
.LBB0_573:
	cmpl	$0, 124(%rsp)
	setne	%cl
	je	.LBB0_789
.LBB0_574:
	movq	152(%rsp), %r14
	jmp	.LBB0_706
.LBB0_789:
	xorl	%r14d, %r14d
	jmp	.LBB0_706
.LBB0_575:
	leaq	.L.str.63(%rip), %rax
	leaq	.L.str.62(%rip), %rcx
	testb	%r14b, %r14b
	cmovneq	%rax, %rcx
	leaq	.L.str.61(%rip), %rax
	testb	%r11b, %r11b
	cmovneq	%rcx, %rax
	movzbl	(%rax), %edx
	testb	%dl, %dl
	je	.LBB0_704
# %bb.576:
	incq	%rax
	movq	56(%rsp), %r12                  # 8-byte Reload
	movq	32(%rsp), %r15                  # 8-byte Reload
	jmp	.LBB0_578
.LBB0_577:                              #   in Loop: Header=BB0_578 Depth=1
	movzbl	(%rax), %edx
	incq	%rax
	xorl	%r14d, %r14d
	movl	$0, %ecx
	testb	%dl, %dl
	je	.LBB0_221
.LBB0_578:                              # =>This Inner Loop Header: Depth=1
	movl	20(%rsp), %ecx
	leal	1(%rcx), %esi
	cmpl	16(%rsp), %esi
	jae	.LBB0_577
# %bb.579:                              #   in Loop: Header=BB0_578 Depth=1
	movq	8(%rsp), %rdi
	movl	%esi, 20(%rsp)
	movb	%dl, (%rdi,%rcx)
	movq	8(%rsp), %rcx
	movl	20(%rsp), %edx
	movb	$0, (%rcx,%rdx)
	jmp	.LBB0_577
.LBB0_580:
	leal	86(%r13), %r12d
	movzbl	100(%rsp), %r14d                # 1-byte Folded Reload
	cmpb	$6, %r12b
	jae	.LBB0_602
.LBB0_581:
	testl	%r9d, %r9d
	je	.LBB0_609
# %bb.582:
	movq	48(%rsp), %rcx                  # 8-byte Reload
	leal	1(%rcx), %edx
	movl	%ecx, %eax
	movq	64(%rsp), %rsi                  # 8-byte Reload
	movb	$114, (%rsi,%rax)
	movl	%ecx, %eax
	orl	$2, %eax
	movl	$4, %ecx
	movb	$112, %sil
	movl	$3, %edi
	movb	$101, %r8b
	jmp	.LBB0_611
.LBB0_583:
	leaq	R64(%rip), %rax
	jmp	.LBB0_586
.LBB0_584:
	leaq	R16(%rip), %rax
	jmp	.LBB0_586
.LBB0_585:
	leaq	R8H(%rip), %rdx
	leaq	R8L(%rip), %rax
	cmpb	$0, 104(%rsp)                   # 1-byte Folded Reload
	cmovneq	%rdx, %rax
.LBB0_586:
	movq	(%rax), %rax
	movzbl	(%rax), %edx
	testb	%dl, %dl
	je	.LBB0_592
# %bb.587:
	incq	%rax
	jmp	.LBB0_589
.LBB0_588:                              #   in Loop: Header=BB0_589 Depth=1
	movzbl	(%rax), %edx
	incq	%rax
	testb	%dl, %dl
	je	.LBB0_591
.LBB0_589:                              # =>This Inner Loop Header: Depth=1
	movl	20(%rsp), %esi
	leal	1(%rsi), %edi
	cmpl	16(%rsp), %edi
	jae	.LBB0_588
# %bb.590:                              #   in Loop: Header=BB0_589 Depth=1
	movq	8(%rsp), %r8
	movl	%edi, 20(%rsp)
	movb	%dl, (%r8,%rsi)
	movq	8(%rsp), %rdx
	movl	20(%rsp), %esi
	movb	$0, (%rdx,%rsi)
	jmp	.LBB0_588
.LBB0_591:
	movl	16(%rsp), %eax
	movl	20(%rsp), %esi
	jmp	.LBB0_593
.LBB0_592:
	movl	$64, %eax
.LBB0_593:
	leal	1(%rsi), %edx
	cmpl	%eax, %edx
	jae	.LBB0_595
# %bb.594:
	movq	8(%rsp), %rax
	movl	%edx, 20(%rsp)
	movl	%esi, %edx
	movb	$44, (%rax,%rdx)
	movq	8(%rsp), %rax
	movl	20(%rsp), %edx
	movb	$0, (%rax,%rdx)
	movl	16(%rsp), %eax
	movl	20(%rsp), %edx
	movq	%rdx, %rsi
	incl	%edx
.LBB0_595:
	cmpl	%eax, %edx
	jae	.LBB0_597
# %bb.596:
	movq	8(%rsp), %rax
	movl	%edx, 20(%rsp)
	movl	%esi, %edx
	movb	$32, (%rax,%rdx)
	movq	8(%rsp), %rax
	movl	20(%rsp), %edx
	movb	$0, (%rax,%rdx)
.LBB0_597:
	leal	-144(%r13,%rcx,8), %edi
	movl	44(%rsp), %esi                  # 4-byte Reload
	movl	28(%rsp), %edx                  # 4-byte Reload
	callq	reg_name
	movzbl	(%rax), %edx
	testb	%dl, %dl
	je	.LBB0_704
# %bb.598:
	incq	%rax
	movq	56(%rsp), %r12                  # 8-byte Reload
	movq	32(%rsp), %r15                  # 8-byte Reload
	jmp	.LBB0_600
.LBB0_599:                              #   in Loop: Header=BB0_600 Depth=1
	movzbl	(%rax), %edx
	incq	%rax
	xorl	%r14d, %r14d
	movl	$0, %ecx
	testb	%dl, %dl
	je	.LBB0_221
.LBB0_600:                              # =>This Inner Loop Header: Depth=1
	movl	20(%rsp), %ecx
	leal	1(%rcx), %esi
	cmpl	16(%rsp), %esi
	jae	.LBB0_599
# %bb.601:                              #   in Loop: Header=BB0_600 Depth=1
	movq	8(%rsp), %rdi
	movl	%esi, 20(%rsp)
	movb	%dl, (%rdi,%rcx)
	movq	8(%rsp), %rcx
	movl	20(%rsp), %edx
	movb	$0, (%rcx,%rdx)
	jmp	.LBB0_599
.LBB0_602:
	cmpb	$-92, 100(%rsp)                 # 1-byte Folded Reload
	je	.LBB0_581
# %bb.603:
	cmpl	$168, %r14d
	je	.LBB0_628
# %bb.604:
	cmpl	$166, %r14d
	je	.LBB0_581
# %bb.605:
	cmpb	$-72, %r15b
	je	.LBB0_632
# %bb.606:
	movzbl	%r15b, %r9d
	cmpl	$176, %r9d
	jne	.LBB0_638
# %bb.607:
	cmpl	%eax, %r10d
	jae	.LBB0_660
# %bb.608:
	addl	$2, %r8d
	movl	%r8d, 84(%rsp)
	movzbl	(%rdi,%r10), %esi
	jmp	.LBB0_661
.LBB0_609:
	testl	%ebx, %ebx
	je	.LBB0_612
# %bb.610:
	movq	48(%rsp), %rcx                  # 8-byte Reload
	movl	%ecx, %eax
	movq	64(%rsp), %rsi                  # 8-byte Reload
	movw	$25970, (%rsi,%rax)             # imm = 0x6572
	leal	3(%rcx), %edx
	movb	$112, 2(%rsi,%rax)
	leal	4(%rcx), %eax
	movl	$6, %ecx
	movb	$122, %sil
	movl	$5, %edi
	movb	$110, %r8b
.LBB0_611:
	movl	%edx, %edx
	movq	64(%rsp), %r9                   # 8-byte Reload
	movb	%r8b, (%r9,%rdx)
	movl	48(%rsp), %edx                  # 4-byte Reload
	movl	%edi, %edi
	addq	%rdx, %rdi
	movl	%eax, %eax
	movb	%sil, (%r9,%rax)
	movl	%ecx, %eax
	addq	%rdx, %rax
	movl	%eax, 20(%rsp)
	movb	$32, (%r9,%rdi)
	movq	%rax, 48(%rsp)                  # 8-byte Spill
	movb	$0, (%r9,%rax)
.LBB0_612:
	cmpb	$-90, 100(%rsp)                 # 1-byte Folded Reload
	je	.LBB0_617
# %bb.613:
	cmpl	$170, %r14d
	je	.LBB0_616
# %bb.614:
	cmpl	$172, %r14d
	jne	.LBB0_618
# %bb.615:
	leaq	.L.str.69(%rip), %rax
	jmp	.LBB0_619
.LBB0_616:
	leaq	.L.str.68(%rip), %rax
	jmp	.LBB0_619
.LBB0_617:
	leaq	.L.str.67(%rip), %rax
	jmp	.LBB0_619
.LBB0_618:
	cmpl	$174, 100(%rsp)                 # 4-byte Folded Reload
	leaq	.L.str.70(%rip), %rcx
	leaq	.L.str.66(%rip), %rax
	cmoveq	%rcx, %rax
.LBB0_619:
	movzbl	(%rax), %ecx
	testb	%cl, %cl
	je	.LBB0_625
# %bb.620:
	incq	%rax
	jmp	.LBB0_622
.LBB0_621:                              #   in Loop: Header=BB0_622 Depth=1
	movzbl	(%rax), %ecx
	incq	%rax
	testb	%cl, %cl
	je	.LBB0_624
.LBB0_622:                              # =>This Inner Loop Header: Depth=1
	movl	20(%rsp), %edx
	leal	1(%rdx), %esi
	cmpl	16(%rsp), %esi
	jae	.LBB0_621
# %bb.623:                              #   in Loop: Header=BB0_622 Depth=1
	movq	8(%rsp), %rdi
	movl	%esi, 20(%rsp)
	movb	%cl, (%rdi,%rdx)
	movq	8(%rsp), %rcx
	movl	20(%rsp), %edx
	movb	$0, (%rcx,%rdx)
	jmp	.LBB0_621
.LBB0_624:
	movl	16(%rsp), %ecx
	movl	20(%rsp), %eax
	movq	%rax, 48(%rsp)                  # 8-byte Spill
	jmp	.LBB0_626
.LBB0_625:
	movl	$64, %ecx
.LBB0_626:
	movq	48(%rsp), %rax                  # 8-byte Reload
	incl	%eax
	cmpl	%ecx, %eax
	jae	.LBB0_704
# %bb.627:
	cmpl	$8, 44(%rsp)                    # 4-byte Folded Reload
	movl	$113, %ecx
	movl	$100, %edx
	cmovel	%ecx, %edx
	testb	$1, %r13b
	movl	$98, %ecx
	cmovnel	%edx, %ecx
	movq	8(%rsp), %rdx
	movl	%eax, 20(%rsp)
	movl	48(%rsp), %eax                  # 4-byte Reload
	movb	%cl, (%rdx,%rax)
	movq	8(%rsp), %rax
	movl	20(%rsp), %ecx
	movb	$0, (%rax,%rcx)
	jmp	.LBB0_704
.LBB0_628:
	xorl	%eax, %eax
	movl	44(%rsp), %r14d                 # 4-byte Reload
	cmpl	$2, %r14d
	setne	%al
	cmpl	$168, %r13d
	leal	2(%rax,%rax), %esi
	movl	$1, %ebx
	cmovel	%ebx, %esi
	leaq	72(%rsp), %rdi
	callq	rd_imm_sext
	movq	48(%rsp), %rdx                  # 8-byte Reload
	leal	5(%rdx), %ecx
	cmpq	$168, %r13
	movl	%edx, %edx
	movq	64(%rsp), %rsi                  # 8-byte Reload
	movl	$1953719668, (%rsi,%rdx)        # imm = 0x74736574
	movl	%ecx, 20(%rsp)
	movw	$32, 4(%rsi,%rdx)
	cmovel	%ebx, %r14d
	cmpl	$2, %r14d
	je	.LBB0_645
# %bb.629:
	movl	%r14d, %edx
	cmpl	$8, %r14d
	je	.LBB0_646
# %bb.630:
	cmpl	$4, %edx
	jne	.LBB0_647
# %bb.631:
	leaq	R32(%rip), %rbp
	jmp	.LBB0_647
.LBB0_632:
	leal	-184(%r13,%rcx,8), %r13d
	testb	%r11b, %r11b
	je	.LBB0_667
# %bb.633:
	xorl	%eax, %eax
	movl	44(%rsp), %ebp                  # 4-byte Reload
	cmpl	$2, %ebp
	setne	%al
	leal	2(,%rax,2), %esi
	leaq	72(%rsp), %rdi
	callq	rd_imm_sext
	movq	%rax, %r14
	movl	48(%rsp), %eax                  # 4-byte Reload
	leaq	4(%rax), %rbx
	movl	%ebx, 20(%rsp)
	movq	64(%rsp), %rcx                  # 8-byte Reload
	movl	$544632685, (%rcx,%rax)         # imm = 0x20766F6D
	movb	$0, 4(%rcx,%rax)
	movl	%r13d, %edi
	movl	%ebp, %esi
	movl	28(%rsp), %edx                  # 4-byte Reload
	callq	reg_name
	movzbl	(%rax), %ecx
	testb	%cl, %cl
	je	.LBB0_696
# %bb.634:
	incq	%rax
	jmp	.LBB0_636
.LBB0_635:                              #   in Loop: Header=BB0_636 Depth=1
	movzbl	(%rax), %ecx
	incq	%rax
	testb	%cl, %cl
	je	.LBB0_678
.LBB0_636:                              # =>This Inner Loop Header: Depth=1
	movl	20(%rsp), %edx
	leal	1(%rdx), %esi
	cmpl	16(%rsp), %esi
	jae	.LBB0_635
# %bb.637:                              #   in Loop: Header=BB0_636 Depth=1
	movq	8(%rsp), %rdi
	movl	%esi, 20(%rsp)
	movb	%cl, (%rdi,%rdx)
	movq	8(%rsp), %rcx
	movl	20(%rsp), %edx
	movb	$0, (%rcx,%rdx)
	jmp	.LBB0_635
.LBB0_638:
	leal	-192(%r14), %r9d
	cmpl	$18, %r9d
	ja	.LBB0_690
# %bb.639:
	movl	$327681, %r11d                  # imm = 0x50001
	btl	%r9d, %r11d
	jae	.LBB0_690
# %bb.640:
	testb	$1, %r13b
	movl	$1, %eax
	movl	44(%rsp), %edi                  # 4-byte Reload
	cmovel	%eax, %edi
	movl	%edi, 44(%rsp)                  # 4-byte Spill
	leaq	72(%rsp), %rdi
	leaq	112(%rsp), %r8
                                        # kill: def $ecx killed $ecx killed $rcx
	callq	parse_modrm
	movslq	160(%rsp), %rax
	leaq	SHIFT(%rip), %rcx
	movq	(%rcx,%rax,8), %rax
	movzbl	(%rax), %ecx
	testb	%cl, %cl
	je	.LBB0_680
# %bb.641:
	incq	%rax
	jmp	.LBB0_643
.LBB0_642:                              #   in Loop: Header=BB0_643 Depth=1
	movzbl	(%rax), %ecx
	incq	%rax
	testb	%cl, %cl
	je	.LBB0_672
.LBB0_643:                              # =>This Inner Loop Header: Depth=1
	movl	20(%rsp), %edx
	leal	1(%rdx), %esi
	cmpl	16(%rsp), %esi
	jae	.LBB0_642
# %bb.644:                              #   in Loop: Header=BB0_643 Depth=1
	movq	8(%rsp), %rdi
	movl	%esi, 20(%rsp)
	movb	%cl, (%rdi,%rdx)
	movq	8(%rsp), %rcx
	movl	20(%rsp), %edx
	movb	$0, (%rcx,%rdx)
	jmp	.LBB0_642
.LBB0_645:
	leaq	R16(%rip), %rbp
	jmp	.LBB0_647
.LBB0_646:
	leaq	R64(%rip), %rbp
.LBB0_647:
	movq	(%rbp), %rdx
	movzbl	(%rdx), %esi
	testb	%sil, %sil
	je	.LBB0_653
# %bb.648:
	incq	%rdx
	jmp	.LBB0_650
.LBB0_649:                              #   in Loop: Header=BB0_650 Depth=1
	movzbl	(%rdx), %esi
	incq	%rdx
	testb	%sil, %sil
	je	.LBB0_652
.LBB0_650:                              # =>This Inner Loop Header: Depth=1
	movl	20(%rsp), %ecx
	leal	1(%rcx), %edi
	cmpl	16(%rsp), %edi
	jae	.LBB0_649
# %bb.651:                              #   in Loop: Header=BB0_650 Depth=1
	movq	8(%rsp), %r8
	movl	%edi, 20(%rsp)
	movb	%sil, (%r8,%rcx)
	movq	8(%rsp), %rcx
	movl	20(%rsp), %esi
	movb	$0, (%rcx,%rsi)
	jmp	.LBB0_649
.LBB0_652:
	movl	16(%rsp), %edx
	movl	20(%rsp), %ecx
	jmp	.LBB0_654
.LBB0_653:
	movl	$64, %edx
.LBB0_654:
	leal	1(%rcx), %esi
	cmpl	%edx, %esi
	jae	.LBB0_656
# %bb.655:
	movq	8(%rsp), %rdx
	movl	%esi, 20(%rsp)
	movl	%ecx, %ecx
	movb	$44, (%rdx,%rcx)
	movq	8(%rsp), %rcx
	movl	20(%rsp), %edx
	movb	$0, (%rcx,%rdx)
	movl	16(%rsp), %edx
	movl	20(%rsp), %ecx
	leal	1(%rcx), %esi
.LBB0_656:
	cmpl	%edx, %esi
	jae	.LBB0_659
# %bb.657:
	movq	8(%rsp), %rdx
	movl	%esi, 20(%rsp)
	movl	%ecx, %ecx
	movb	$32, (%rdx,%rcx)
.LBB0_658:
	movq	8(%rsp), %rcx
	movl	20(%rsp), %edx
	movb	$0, (%rcx,%rdx)
.LBB0_659:
	leaq	8(%rsp), %rdi
	movq	%rax, %rsi
	jmp	.LBB0_703
.LBB0_660:
	movl	$1, 88(%rsp)
	xorl	%esi, %esi
.LBB0_661:
	movl	48(%rsp), %edx                  # 4-byte Reload
	movl	%r13d, %eax
	andl	$15, %eax
	shll	$6, %ecx
	shll	$3, %eax
	orl	%ecx, %eax
	leaq	R8L(%rip), %rcx
	addq	%rax, %rcx
	andl	$7, %r13d
	leaq	R8H(%rip), %rax
	leaq	(%rax,%r13,8), %rdi
	leaq	4(%rdx), %rax
	movl	%eax, 20(%rsp)
	movq	64(%rsp), %r8                   # 8-byte Reload
	movl	$544632685, (%r8,%rdx)          # imm = 0x20766F6D
	cmpb	$0, 104(%rsp)                   # 1-byte Folded Reload
	cmoveq	%rcx, %rdi
	movb	$0, 4(%r8,%rdx)
	movq	(%rdi), %rcx
	movzbl	(%rcx), %edx
	testb	%dl, %dl
	je	.LBB0_673
# %bb.662:
	incq	%rcx
	jmp	.LBB0_664
.LBB0_663:                              #   in Loop: Header=BB0_664 Depth=1
	movzbl	(%rcx), %edx
	incq	%rcx
	testb	%dl, %dl
	je	.LBB0_666
.LBB0_664:                              # =>This Inner Loop Header: Depth=1
	movl	20(%rsp), %eax
	leal	1(%rax), %edi
	cmpl	16(%rsp), %edi
	jae	.LBB0_663
# %bb.665:                              #   in Loop: Header=BB0_664 Depth=1
	movq	8(%rsp), %r8
	movl	%edi, 20(%rsp)
	movb	%dl, (%r8,%rax)
	movq	8(%rsp), %rax
	movl	20(%rsp), %edx
	movb	$0, (%rax,%rdx)
	jmp	.LBB0_663
.LBB0_666:
	movl	16(%rsp), %ecx
	movl	20(%rsp), %eax
	jmp	.LBB0_674
.LBB0_667:
	leaq	72(%rsp), %rdi
	callq	rd64
	movq	48(%rsp), %rdx                  # 8-byte Reload
	movl	%edx, %ecx
	addl	$7, %edx
	movq	%rdx, %rsi
	movl	%edx, 20(%rsp)
	movabsq	$9134065633881965, %rdx         # imm = 0x20736261766F6D
	movq	64(%rsp), %rdi                  # 8-byte Reload
	movq	%rdx, (%rdi,%rcx)
	movl	%r13d, %ecx
	leaq	R64(%rip), %rdx
	movq	(%rdx,%rcx,8), %rcx
	movzbl	(%rcx), %edx
	testb	%dl, %dl
	je	.LBB0_707
# %bb.668:
	incq	%rcx
	jmp	.LBB0_670
.LBB0_669:                              #   in Loop: Header=BB0_670 Depth=1
	movzbl	(%rcx), %edx
	incq	%rcx
	testb	%dl, %dl
	je	.LBB0_679
.LBB0_670:                              # =>This Inner Loop Header: Depth=1
	movl	20(%rsp), %esi
	leal	1(%rsi), %edi
	cmpl	16(%rsp), %edi
	jae	.LBB0_669
# %bb.671:                              #   in Loop: Header=BB0_670 Depth=1
	movq	8(%rsp), %r8
	movl	%edi, 20(%rsp)
	movb	%dl, (%r8,%rsi)
	movq	8(%rsp), %rdx
	movl	20(%rsp), %esi
	movb	$0, (%rdx,%rsi)
	jmp	.LBB0_669
.LBB0_672:
	movl	16(%rsp), %eax
	movl	20(%rsp), %ecx
	movq	%rcx, 48(%rsp)                  # 8-byte Spill
	jmp	.LBB0_681
.LBB0_673:
	movl	$64, %ecx
.LBB0_674:
	leal	1(%rax), %edx
	cmpl	%ecx, %edx
	jae	.LBB0_676
# %bb.675:
	movq	8(%rsp), %rcx
	movl	%edx, 20(%rsp)
	movl	%eax, %eax
	movb	$44, (%rcx,%rax)
	movq	8(%rsp), %rax
	movl	20(%rsp), %ecx
	movb	$0, (%rax,%rcx)
	movl	16(%rsp), %ecx
	movl	20(%rsp), %eax
	leal	1(%rax), %edx
.LBB0_676:
	cmpl	%ecx, %edx
	jae	.LBB0_702
# %bb.677:
	movq	8(%rsp), %rcx
	movl	%edx, 20(%rsp)
	movl	%eax, %eax
	movb	$32, (%rcx,%rax)
	movq	8(%rsp), %rax
	movl	20(%rsp), %ecx
	movb	$0, (%rax,%rcx)
	jmp	.LBB0_702
.LBB0_678:
	movl	16(%rsp), %eax
	movl	20(%rsp), %ebx
	jmp	.LBB0_697
.LBB0_679:
	movl	16(%rsp), %ecx
	movl	20(%rsp), %esi
	jmp	.LBB0_708
.LBB0_680:
	movl	$64, %eax
.LBB0_681:
	movq	48(%rsp), %rcx                  # 8-byte Reload
	incl	%ecx
	cmpl	%eax, %ecx
	jae	.LBB0_683
# %bb.682:
	movq	8(%rsp), %rax
	movl	%ecx, 20(%rsp)
	movl	48(%rsp), %ecx                  # 4-byte Reload
	movb	$32, (%rax,%rcx)
	movq	8(%rsp), %rax
	movl	20(%rsp), %ecx
	movb	$0, (%rax,%rcx)
.LBB0_683:
	xorl	%r8d, %r8d
	cmpl	$0, 112(%rsp)
	movl	44(%rsp), %edx                  # 4-byte Reload
	cmovel	%edx, %r8d
	leaq	8(%rsp), %rdi
	leaq	112(%rsp), %rsi
	movl	28(%rsp), %ecx                  # 4-byte Reload
	xorl	%r9d, %r9d
	callq	render_rm
	cmpl	$192, 100(%rsp)                 # 4-byte Folded Reload
	jne	.LBB0_686
# %bb.684:
	movl	84(%rsp), %eax
	cmpl	80(%rsp), %eax
	jae	.LBB0_712
# %bb.685:
	movq	72(%rsp), %rcx
	leal	1(%rax), %edx
	movl	%edx, 84(%rsp)
	movzbl	(%rcx,%rax), %esi
	jmp	.LBB0_713
.LBB0_686:
	andl	$-46, %r13d
	movl	16(%rsp), %edx
	movl	20(%rsp), %eax
	leal	1(%rax), %ecx
	cmpl	$208, %r13d
	jne	.LBB0_718
# %bb.687:
	cmpl	%edx, %ecx
	jae	.LBB0_688
# %bb.725:
	movq	8(%rsp), %rdx
	movl	%ecx, 20(%rsp)
	movb	$44, (%rdx,%rax)
	movq	8(%rsp), %rax
	movl	20(%rsp), %ecx
	movb	$0, (%rax,%rcx)
	movl	16(%rsp), %edx
	movl	20(%rsp), %eax
	leal	1(%rax), %ecx
	cmpl	%edx, %ecx
	jb	.LBB0_726
.LBB0_689:
	cmpl	%edx, %ecx
	jae	.LBB0_573
	jmp	.LBB0_727
.LBB0_690:
	cmpl	$200, %r13d
	jle	.LBB0_722
# %bb.691:
	cmpl	$201, %r13d
	je	.LBB0_733
# %bb.692:
	cmpl	$204, %r13d
	je	.LBB0_734
# %bb.693:
	cmpl	$205, %r13d
	jne	.LBB0_737
# %bb.694:
	cmpl	%eax, %r10d
	jae	.LBB0_744
# %bb.695:
	addl	$2, %r8d
	movl	%r8d, 84(%rsp)
	movzbl	(%rdi,%r10), %esi
	jmp	.LBB0_745
.LBB0_696:
	movl	$64, %eax
.LBB0_697:
	leal	1(%rbx), %ecx
	cmpl	%eax, %ecx
	jae	.LBB0_699
# %bb.698:
	movq	8(%rsp), %rax
	movl	%ecx, 20(%rsp)
	movl	%ebx, %ecx
	movb	$44, (%rax,%rcx)
	movq	8(%rsp), %rax
	movl	20(%rsp), %ecx
	movb	$0, (%rax,%rcx)
	movl	16(%rsp), %eax
	movl	20(%rsp), %ebx
	leal	1(%rbx), %ecx
.LBB0_699:
	cmpl	%eax, %ecx
	jae	.LBB0_701
# %bb.700:
	movq	8(%rsp), %rax
	movl	%ecx, 20(%rsp)
	movl	%ebx, %ecx
	movb	$32, (%rax,%rcx)
	movq	8(%rsp), %rax
	movl	20(%rsp), %ecx
	movb	$0, (%rax,%rcx)
.LBB0_701:
	movl	%r14d, %esi
.LBB0_702:
	leaq	8(%rsp), %rdi
.LBB0_703:
	callq	sb_0xhex
.LBB0_704:
	xorl	%r14d, %r14d
.LBB0_705:
	xorl	%ecx, %ecx
.LBB0_706:
	movq	56(%rsp), %r12                  # 8-byte Reload
	jmp	.LBB0_220
.LBB0_707:
	movl	$64, %ecx
.LBB0_708:
	leal	1(%rsi), %edx
	cmpl	%ecx, %edx
	jae	.LBB0_710
# %bb.709:
	movq	8(%rsp), %rcx
	movl	%edx, 20(%rsp)
	movl	%esi, %edx
	movb	$44, (%rcx,%rdx)
	movq	8(%rsp), %rcx
	movl	20(%rsp), %edx
	movb	$0, (%rcx,%rdx)
	movl	16(%rsp), %ecx
	movl	20(%rsp), %edx
	movq	%rdx, %rsi
	incl	%edx
.LBB0_710:
	cmpl	%ecx, %edx
	jae	.LBB0_659
# %bb.711:
	movq	8(%rsp), %rcx
	movl	%edx, 20(%rsp)
	movl	%esi, %edx
	movb	$32, (%rcx,%rdx)
	jmp	.LBB0_658
.LBB0_712:
	movl	$1, 88(%rsp)
	xorl	%esi, %esi
.LBB0_713:
	movl	16(%rsp), %edx
	movl	20(%rsp), %eax
	leal	1(%rax), %ecx
	cmpl	%edx, %ecx
	jae	.LBB0_715
# %bb.714:
	movq	8(%rsp), %rdx
	movl	%ecx, 20(%rsp)
	movb	$44, (%rdx,%rax)
	movq	8(%rsp), %rax
	movl	20(%rsp), %ecx
	movb	$0, (%rax,%rcx)
	movl	16(%rsp), %edx
	movl	20(%rsp), %eax
	leal	1(%rax), %ecx
.LBB0_715:
	cmpl	%edx, %ecx
	jae	.LBB0_717
# %bb.716:
	movq	8(%rsp), %rdx
	movl	%ecx, 20(%rsp)
	movl	%eax, %eax
	movb	$32, (%rdx,%rax)
	movq	8(%rsp), %rax
	movl	20(%rsp), %ecx
	movb	$0, (%rax,%rcx)
.LBB0_717:
	leaq	8(%rsp), %rdi
	callq	sb_0xhex
	jmp	.LBB0_573
.LBB0_718:
	cmpl	%edx, %ecx
	jae	.LBB0_719
# %bb.728:
	movq	8(%rsp), %rdx
	movl	%ecx, 20(%rsp)
	movb	$44, (%rdx,%rax)
	movq	8(%rsp), %rax
	movl	20(%rsp), %ecx
	movb	$0, (%rax,%rcx)
	movl	16(%rsp), %edx
	movl	20(%rsp), %eax
	leal	1(%rax), %ecx
	cmpl	%edx, %ecx
	jb	.LBB0_729
.LBB0_720:
	cmpl	%edx, %ecx
	jae	.LBB0_721
.LBB0_730:
	movq	8(%rsp), %rdx
	movl	%ecx, 20(%rsp)
	movl	%eax, %eax
	movb	$99, (%rdx,%rax)
	movq	8(%rsp), %rax
	movl	20(%rsp), %ecx
	movb	$0, (%rax,%rcx)
	movl	16(%rsp), %edx
	movl	20(%rsp), %eax
	leal	1(%rax), %ecx
	cmpl	%edx, %ecx
	jae	.LBB0_573
	jmp	.LBB0_731
.LBB0_688:
	cmpl	%edx, %ecx
	jae	.LBB0_689
.LBB0_726:
	movq	8(%rsp), %rdx
	movl	%ecx, 20(%rsp)
	movl	%eax, %eax
	movb	$32, (%rdx,%rax)
	movq	8(%rsp), %rax
	movl	20(%rsp), %ecx
	movb	$0, (%rax,%rcx)
	movl	16(%rsp), %edx
	movl	20(%rsp), %eax
	leal	1(%rax), %ecx
	cmpl	%edx, %ecx
	jae	.LBB0_573
.LBB0_727:
	movq	8(%rsp), %rdx
	movl	%ecx, 20(%rsp)
	movl	%eax, %eax
	movb	$49, (%rdx,%rax)
	jmp	.LBB0_732
.LBB0_719:
	cmpl	%edx, %ecx
	jae	.LBB0_720
.LBB0_729:
	movq	8(%rsp), %rdx
	movl	%ecx, 20(%rsp)
	movl	%eax, %eax
	movb	$32, (%rdx,%rax)
	movq	8(%rsp), %rax
	movl	20(%rsp), %ecx
	movb	$0, (%rax,%rcx)
	movl	16(%rsp), %edx
	movl	20(%rsp), %eax
	leal	1(%rax), %ecx
	cmpl	%edx, %ecx
	jb	.LBB0_730
.LBB0_721:
	cmpl	%edx, %ecx
	jae	.LBB0_573
.LBB0_731:
	movq	8(%rsp), %rdx
	movl	%ecx, 20(%rsp)
	movl	%eax, %eax
	movb	$108, (%rdx,%rax)
.LBB0_732:
	movq	8(%rsp), %rax
	movl	20(%rsp), %ecx
	movb	$0, (%rax,%rcx)
	jmp	.LBB0_573
.LBB0_722:
	cmpl	$194, %r13d
	je	.LBB0_735
# %bb.723:
	cmpl	$195, %r13d
	jne	.LBB0_737
# %bb.724:
	movq	48(%rsp), %rcx                  # 8-byte Reload
	movl	%ecx, %eax
	addl	$3, %ecx
	movl	%ecx, 20(%rsp)
	movq	64(%rsp), %rcx                  # 8-byte Reload
	movl	$7628146, (%rcx,%rax)           # imm = 0x746572
	jmp	.LBB0_704
.LBB0_733:
	movq	48(%rsp), %rcx                  # 8-byte Reload
	movl	%ecx, %eax
	movq	64(%rsp), %rdx                  # 8-byte Reload
	movl	$1986094444, (%rdx,%rax)        # imm = 0x7661656C
	addl	$5, %ecx
	movl	%ecx, 20(%rsp)
	movw	$101, 4(%rdx,%rax)
	jmp	.LBB0_704
.LBB0_734:
	movl	48(%rsp), %eax                  # 4-byte Reload
	leaq	4(%rax), %rcx
	movl	%ecx, 20(%rsp)
	movq	64(%rsp), %rcx                  # 8-byte Reload
	movl	$863268457, (%rcx,%rax)         # imm = 0x33746E69
	movb	$0, 4(%rcx,%rax)
	jmp	.LBB0_704
.LBB0_735:
	cmpl	%eax, %r10d
	jae	.LBB0_746
# %bb.736:
	addl	$2, %r8d
	movl	%r8d, 84(%rsp)
	movzbl	(%rdi,%r10), %ecx
	movl	%r8d, %r10d
	jmp	.LBB0_747
.LBB0_737:
	cmpb	$-24, 100(%rsp)                 # 1-byte Folded Reload
	je	.LBB0_751
# %bb.738:
	cmpl	$198, %r14d
	jne	.LBB0_756
# %bb.739:
	leaq	72(%rsp), %r14
	leaq	112(%rsp), %r15
	movq	%r14, %rdi
                                        # kill: def $ecx killed $ecx killed $rcx
	movq	%r15, %r8
	callq	parse_modrm
	xorl	%eax, %eax
	movl	44(%rsp), %ebp                  # 4-byte Reload
	cmpl	$2, %ebp
	setne	%al
	cmpl	$198, %r13d
	leal	2(%rax,%rax), %esi
	movl	$1, %ebx
	cmovel	%ebx, %esi
	movq	%r14, %rdi
	callq	rd_imm_sext
	movq	%rax, %r14
	movl	48(%rsp), %eax                  # 4-byte Reload
	leaq	4(%rax), %rcx
	cmpq	$198, %r13
	movl	%ecx, 20(%rsp)
	movq	64(%rsp), %rcx                  # 8-byte Reload
	movl	$544632685, (%rcx,%rax)         # imm = 0x20766F6D
	movb	$0, 4(%rcx,%rax)
	cmovel	%ebx, %ebp
	xorl	%r8d, %r8d
	cmpl	$0, 112(%rsp)
	cmovel	%ebp, %r8d
	leaq	8(%rsp), %rdi
	movq	%r15, %rsi
	movl	%ebp, %edx
	movl	28(%rsp), %ecx                  # 4-byte Reload
	xorl	%r9d, %r9d
	callq	render_rm
	movl	16(%rsp), %edx
	movl	20(%rsp), %eax
	leal	1(%rax), %ecx
	cmpl	%edx, %ecx
	jae	.LBB0_741
# %bb.740:
	movq	8(%rsp), %rdx
	movl	%ecx, 20(%rsp)
	movb	$44, (%rdx,%rax)
	movq	8(%rsp), %rax
	movl	20(%rsp), %ecx
	movb	$0, (%rax,%rcx)
	movl	16(%rsp), %edx
	movl	20(%rsp), %eax
	leal	1(%rax), %ecx
.LBB0_741:
	cmpl	%edx, %ecx
	jae	.LBB0_743
# %bb.742:
	movq	8(%rsp), %rdx
	movl	%ecx, 20(%rsp)
	movl	%eax, %eax
	movb	$32, (%rdx,%rax)
	movq	8(%rsp), %rax
	movl	20(%rsp), %ecx
	movb	$0, (%rax,%rcx)
.LBB0_743:
	leaq	8(%rsp), %rdi
	movq	%r14, %rsi
	callq	sb_0xhex
	jmp	.LBB0_573
.LBB0_744:
	movl	$1, 88(%rsp)
	xorl	%esi, %esi
.LBB0_745:
	movl	48(%rsp), %eax                  # 4-byte Reload
	leaq	4(%rax), %rcx
	movl	%ecx, 20(%rsp)
	movq	64(%rsp), %rcx                  # 8-byte Reload
	movl	$544501353, (%rcx,%rax)         # imm = 0x20746E69
	movb	$0, 4(%rcx,%rax)
	jmp	.LBB0_702
.LBB0_746:
	movl	$1, 88(%rsp)
	xorl	%ecx, %ecx
.LBB0_747:
	cmpl	%eax, %r10d
	jae	.LBB0_749
# %bb.748:
	leal	1(%r10), %eax
	movl	%eax, 84(%rsp)
	movl	%r10d, %eax
	movzbl	(%rdi,%rax), %esi
	shll	$8, %esi
	jmp	.LBB0_750
.LBB0_749:
	movl	$1, 88(%rsp)
	xorl	%esi, %esi
.LBB0_750:
	orq	%rcx, %rsi
	movl	48(%rsp), %eax                  # 4-byte Reload
	leaq	4(%rax), %rcx
	movl	%ecx, 20(%rsp)
	movq	64(%rsp), %rcx                  # 8-byte Reload
	movl	$544499058, (%rcx,%rax)         # imm = 0x20746572
	movb	$0, 4(%rcx,%rax)
	jmp	.LBB0_702
.LBB0_751:
	leaq	72(%rsp), %rdi
	movl	$4, %esi
	callq	rd_imm_sext
	cmpl	$232, %r13d
	leaq	.L.str.79(%rip), %rdx
	leaq	.L.str.80(%rip), %rcx
	cmoveq	%rdx, %rcx
	movzbl	(%rcx), %edx
	testb	%dl, %dl
	je	.LBB0_761
# %bb.752:
	incq	%rcx
	jmp	.LBB0_754
.LBB0_753:                              #   in Loop: Header=BB0_754 Depth=1
	movzbl	(%rcx), %edx
	incq	%rcx
	testb	%dl, %dl
	je	.LBB0_761
.LBB0_754:                              # =>This Inner Loop Header: Depth=1
	movl	20(%rsp), %esi
	leal	1(%rsi), %edi
	cmpl	16(%rsp), %edi
	jae	.LBB0_753
# %bb.755:                              #   in Loop: Header=BB0_754 Depth=1
	movq	8(%rsp), %r8
	movl	%edi, 20(%rsp)
	movb	%dl, (%r8,%rsi)
	movq	8(%rsp), %rdx
	movl	20(%rsp), %esi
	movb	$0, (%rdx,%rsi)
	jmp	.LBB0_753
.LBB0_756:
	cmpl	$245, %r13d
	je	.LBB0_762
# %bb.757:
	cmpl	$244, %r13d
	je	.LBB0_763
# %bb.758:
	cmpl	$235, %r13d
	jne	.LBB0_764
# %bb.759:
	cmpl	%eax, %r10d
	jae	.LBB0_770
# %bb.760:
	addl	$2, %r8d
	movl	%r8d, 84(%rsp)
	movsbq	(%rdi,%r10), %rsi
	movl	%r8d, %r10d
	jmp	.LBB0_771
.LBB0_761:
	movq	56(%rsp), %r12                  # 8-byte Reload
	movl	$1, 12(%r12)
	movl	84(%rsp), %ecx
	addq	176(%rsp), %rax                 # 8-byte Folded Reload
	addq	%rcx, %rax
	movq	%rax, 16(%r12)
	leaq	8(%rsp), %rdi
	movq	%rax, %rsi
	callq	sb_0xhex
	jmp	.LBB0_219
.LBB0_762:
	movq	48(%rsp), %rcx                  # 8-byte Reload
	movl	%ecx, %eax
	addl	$3, %ecx
	movl	%ecx, 20(%rsp)
	movq	64(%rsp), %rcx                  # 8-byte Reload
	movl	$6516067, (%rcx,%rax)           # imm = 0x636D63
	jmp	.LBB0_704
.LBB0_763:
	movq	48(%rsp), %rcx                  # 8-byte Reload
	movl	%ecx, %eax
	addl	$3, %ecx
	movl	%ecx, 20(%rsp)
	movq	64(%rsp), %rcx                  # 8-byte Reload
	movl	$7629928, (%rcx,%rax)           # imm = 0x746C68
	jmp	.LBB0_704
.LBB0_764:
	cmpl	$246, 100(%rsp)                 # 4-byte Folded Reload
	jne	.LBB0_772
# %bb.765:
	cmpl	$246, %r13d
	movl	$1, %r12d
	cmovnel	44(%rsp), %r12d                 # 4-byte Folded Reload
	leaq	72(%rsp), %rdi
	leaq	112(%rsp), %r8
                                        # kill: def $ecx killed $ecx killed $rcx
	callq	parse_modrm
	movslq	160(%rsp), %rbx
	leaq	x86_decode.G3(%rip), %rax
	movq	(%rax,%rbx,8), %rax
	movzbl	(%rax), %ecx
	testb	%cl, %cl
	je	.LBB0_780
# %bb.766:
	incq	%rax
	jmp	.LBB0_768
.LBB0_767:                              #   in Loop: Header=BB0_768 Depth=1
	movzbl	(%rax), %ecx
	incq	%rax
	testb	%cl, %cl
	je	.LBB0_779
.LBB0_768:                              # =>This Inner Loop Header: Depth=1
	movl	20(%rsp), %edx
	leal	1(%rdx), %esi
	cmpl	16(%rsp), %esi
	jae	.LBB0_767
# %bb.769:                              #   in Loop: Header=BB0_768 Depth=1
	movq	8(%rsp), %rdi
	movl	%esi, 20(%rsp)
	movb	%cl, (%rdi,%rdx)
	movq	8(%rsp), %rcx
	movl	20(%rsp), %edx
	movb	$0, (%rcx,%rdx)
	jmp	.LBB0_767
.LBB0_770:
	movl	$1, 88(%rsp)
	xorl	%esi, %esi
.LBB0_771:
	movq	48(%rsp), %rcx                  # 8-byte Reload
	movl	%ecx, %eax
	addl	$4, %ecx
	movl	%ecx, 20(%rsp)
	movq	56(%rsp), %r12                  # 8-byte Reload
	movl	$544238954, 24(%r12,%rax)       # imm = 0x20706D6A
	movb	$0, 28(%r12,%rax)
	movl	$1, 12(%r12)
	movl	%r10d, %eax
	addq	176(%rsp), %rsi                 # 8-byte Folded Reload
	addq	%rax, %rsi
	movq	%rsi, 16(%r12)
	leaq	8(%rsp), %rdi
	callq	sb_0xhex
	jmp	.LBB0_219
.LBB0_772:
	cmpq	$255, %r13
	je	.LBB0_790
# %bb.773:
	cmpl	$254, %r13d
	jne	.LBB0_798
# %bb.774:
	leaq	72(%rsp), %rdi
	leaq	112(%rsp), %r8
                                        # kill: def $ecx killed $ecx killed $rcx
	callq	parse_modrm
	movl	160(%rsp), %eax
	cmpl	$1, %eax
	leaq	.L.str.91(%rip), %rcx
	leaq	.L.str.92(%rip), %rdx
	cmoveq	%rcx, %rdx
	testl	%eax, %eax
	leaq	.L.str.90(%rip), %rax
	cmovneq	%rdx, %rax
	movzbl	(%rax), %ecx
	testb	%cl, %cl
	je	.LBB0_799
# %bb.775:
	incq	%rax
	jmp	.LBB0_777
.LBB0_776:                              #   in Loop: Header=BB0_777 Depth=1
	movzbl	(%rax), %ecx
	incq	%rax
	testb	%cl, %cl
	je	.LBB0_799
.LBB0_777:                              # =>This Inner Loop Header: Depth=1
	movl	20(%rsp), %edx
	leal	1(%rdx), %esi
	cmpl	16(%rsp), %esi
	jae	.LBB0_776
# %bb.778:                              #   in Loop: Header=BB0_777 Depth=1
	movq	8(%rsp), %rdi
	movl	%esi, 20(%rsp)
	movb	%cl, (%rdi,%rdx)
	movq	8(%rsp), %rcx
	movl	20(%rsp), %edx
	movb	$0, (%rcx,%rdx)
	jmp	.LBB0_776
.LBB0_779:
	movl	16(%rsp), %eax
	movl	20(%rsp), %ecx
	movq	%rcx, 48(%rsp)                  # 8-byte Spill
	jmp	.LBB0_781
.LBB0_780:
	movl	$64, %eax
.LBB0_781:
	movq	48(%rsp), %rcx                  # 8-byte Reload
	incl	%ecx
	cmpl	%eax, %ecx
	jae	.LBB0_783
# %bb.782:
	movq	8(%rsp), %rax
	movl	%ecx, 20(%rsp)
	movl	48(%rsp), %ecx                  # 4-byte Reload
	movb	$32, (%rax,%rcx)
	movq	8(%rsp), %rax
	movl	20(%rsp), %ecx
	movb	$0, (%rax,%rcx)
.LBB0_783:
	xorl	%r8d, %r8d
	cmpl	$0, 112(%rsp)
	cmovel	%r12d, %r8d
	leaq	8(%rsp), %rdi
	leaq	112(%rsp), %rsi
	movl	%r12d, %edx
	movl	28(%rsp), %ecx                  # 4-byte Reload
	xorl	%r9d, %r9d
	callq	render_rm
	cmpl	$1, %ebx
	jg	.LBB0_573
# %bb.784:
	xorl	%eax, %eax
	cmpl	$2, 44(%rsp)                    # 4-byte Folded Reload
	setne	%al
	cmpl	$246, %r13d
	leal	2(%rax,%rax), %eax
	movl	$1, %esi
	cmovnel	%eax, %esi
	leaq	72(%rsp), %rdi
	callq	rd_imm_sext
	movl	16(%rsp), %esi
	movl	20(%rsp), %ecx
	leal	1(%rcx), %edx
	cmpl	%esi, %edx
	jae	.LBB0_786
# %bb.785:
	movq	8(%rsp), %rsi
	movl	%edx, 20(%rsp)
	movb	$44, (%rsi,%rcx)
	movq	8(%rsp), %rcx
	movl	20(%rsp), %edx
	movb	$0, (%rcx,%rdx)
	movl	16(%rsp), %esi
	movl	20(%rsp), %ecx
	leal	1(%rcx), %edx
.LBB0_786:
	cmpl	%esi, %edx
	jae	.LBB0_788
# %bb.787:
	movq	8(%rsp), %rsi
	movl	%edx, 20(%rsp)
	movl	%ecx, %ecx
	movb	$32, (%rsi,%rcx)
	movq	8(%rsp), %rcx
	movl	20(%rsp), %edx
	movb	$0, (%rcx,%rdx)
.LBB0_788:
	leaq	8(%rsp), %rdi
	movq	%rax, %rsi
	callq	sb_0xhex
	jmp	.LBB0_573
.LBB0_790:
	leaq	72(%rsp), %rdi
	leaq	112(%rsp), %r8
                                        # kill: def $ecx killed $ecx killed $rcx
	callq	parse_modrm
	movslq	160(%rsp), %rax
	cmpq	$6, %rax
	ja	.LBB0_793
# %bb.791:
	movl	$84, %ecx
	btl	%eax, %ecx
	jae	.LBB0_793
# %bb.792:
	movl	$8, 44(%rsp)                    # 4-byte Folded Spill
.LBB0_793:
	leaq	x86_decode.G5(%rip), %rcx
	movq	(%rcx,%rax,8), %rax
	movzbl	(%rax), %ecx
	testb	%cl, %cl
	je	.LBB0_801
# %bb.794:
	incq	%rax
	jmp	.LBB0_796
.LBB0_795:                              #   in Loop: Header=BB0_796 Depth=1
	movzbl	(%rax), %ecx
	incq	%rax
	testb	%cl, %cl
	je	.LBB0_800
.LBB0_796:                              # =>This Inner Loop Header: Depth=1
	movl	20(%rsp), %edx
	leal	1(%rdx), %esi
	cmpl	16(%rsp), %esi
	jae	.LBB0_795
# %bb.797:                              #   in Loop: Header=BB0_796 Depth=1
	movq	8(%rsp), %rdi
	movl	%esi, 20(%rsp)
	movb	%cl, (%rdi,%rdx)
	movq	8(%rsp), %rcx
	movl	20(%rsp), %edx
	movb	$0, (%rcx,%rdx)
	jmp	.LBB0_795
.LBB0_798:
	movq	48(%rsp), %rcx                  # 8-byte Reload
	movl	%ecx, %eax
	movq	64(%rsp), %rdx                  # 8-byte Reload
	movl	$1684103720, (%rdx,%rax)        # imm = 0x64616228
	addl	$5, %ecx
	movl	%ecx, 20(%rsp)
	movw	$41, 4(%rdx,%rax)
	jmp	.LBB0_704
.LBB0_799:
	xorl	%r8d, %r8d
	cmpl	$0, 112(%rsp)
	sete	%r8b
	leaq	8(%rsp), %rdi
	leaq	112(%rsp), %rsi
	xorl	%r14d, %r14d
	movl	$1, %edx
	jmp	.LBB0_535
.LBB0_800:
	movl	16(%rsp), %eax
	movl	20(%rsp), %ecx
	movq	%rcx, 48(%rsp)                  # 8-byte Spill
	jmp	.LBB0_802
.LBB0_801:
	movl	$64, %eax
.LBB0_802:
	movq	48(%rsp), %rcx                  # 8-byte Reload
	incl	%ecx
	cmpl	%eax, %ecx
	jae	.LBB0_804
# %bb.803:
	movq	8(%rsp), %rax
	movl	%ecx, 20(%rsp)
	movl	48(%rsp), %ecx                  # 4-byte Reload
	movb	$32, (%rax,%rcx)
	movq	8(%rsp), %rax
	movl	20(%rsp), %ecx
	movb	$0, (%rax,%rcx)
.LBB0_804:
	xorl	%r14d, %r14d
	cmpl	$0, 112(%rsp)
	movl	$0, %r8d
	movl	44(%rsp), %edx                  # 4-byte Reload
	cmovel	%edx, %r8d
	leaq	8(%rsp), %rdi
	leaq	112(%rsp), %rsi
	movl	28(%rsp), %ecx                  # 4-byte Reload
	xorl	%r9d, %r9d
	callq	render_rm
	cmpl	$0, 124(%rsp)
	setne	%cl
	jne	.LBB0_574
	jmp	.LBB0_706
.Lfunc_end0:
	.size	x86_decode, .Lfunc_end0-x86_decode
                                        # -- End function
	.p2align	4                               # -- Begin function parse_modrm
	.type	parse_modrm,@function
parse_modrm:                            # @parse_modrm
# %bb.0:
	pushq	%r15
	pushq	%r14
	pushq	%rbx
	movq	%r8, %rbx
	movl	8(%rdi), %r10d
	movl	12(%rdi), %eax
	cmpl	%r10d, %eax
	jae	.LBB1_1
# %bb.2:
	movq	(%rdi), %r8
	movl	%eax, %r11d
	incl	%r11d
	movl	%r11d, 12(%rdi)
	movzbl	(%r8,%rax), %r9d
	movl	%r11d, %eax
	jmp	.LBB1_3
.LBB1_1:
	movl	$1, 16(%rdi)
	xorl	%r9d, %r9d
.LBB1_3:
	movl	%r9d, %r8d
	shrl	$6, %r8d
	movl	%r9d, %r14d
	shrl	$3, %r14d
	andl	$7, %r14d
	movl	%r9d, %r11d
	andl	$7, %r11d
	movl	$0, (%rbx)
	movq	$0, 12(%rbx)
	movl	$0, 24(%rbx)
	movl	$1, 32(%rbx)
	movq	$0, 40(%rbx)
	movl	%r14d, 48(%rbx)
	xorl	%r15d, %r15d
	testl	%esi, %esi
	setne	%r15b
	leal	(%r14,%r15,8), %esi
	movl	%esi, 4(%rbx)
	cmpl	$3, %r8d
	jne	.LBB1_5
# %bb.4:
	movl	$1, (%rbx)
	xorl	%eax, %eax
	testl	%ecx, %ecx
	setne	%al
	leal	(%r11,%rax,8), %eax
	movl	%eax, 8(%rbx)
	jmp	.LBB1_25
.LBB1_5:
	cmpl	$4, %r11d
	jne	.LBB1_15
# %bb.6:
	cmpl	%r10d, %eax
	jae	.LBB1_7
# %bb.8:
	movq	(%rdi), %rsi
	leal	1(%rax), %r10d
	movl	%r10d, 12(%rdi)
	movl	%eax, %eax
	movzbl	(%rsi,%rax), %eax
	movl	%eax, %esi
	shrl	$3, %esi
	andl	$7, %esi
	movl	%eax, %r10d
	andl	$7, %r10d
	testl	%edx, %edx
	setne	%dl
	jne	.LBB1_10
# %bb.9:
	cmpl	$4, %esi
	je	.LBB1_11
	jmp	.LBB1_10
.LBB1_15:
	andl	$-57, %r9d
	cmpl	$5, %r9d
	jne	.LBB1_17
# %bb.16:
	movl	$1, 12(%rbx)
	jmp	.LBB1_13
.LBB1_7:
	movl	$1, 16(%rdi)
	testl	%edx, %edx
	setne	%dl
	xorl	%r10d, %r10d
	xorl	%esi, %esi
	xorl	%eax, %eax
.LBB1_10:
	shrl	$6, %eax
	movl	$1, 24(%rbx)
	movzbl	%dl, %edx
	leal	(%rsi,%rdx,8), %edx
	movl	%edx, 28(%rbx)
	movl	$1, %esi
	movl	%ecx, %edx
	movl	%eax, %ecx
	shll	%cl, %esi
	movl	%edx, %ecx
	movl	%esi, 32(%rbx)
.LBB1_11:
	xorl	%esi, %esi
	cmpl	$5, %r10d
	jne	.LBB1_14
# %bb.12:
	cmpl	$64, %r9d
	jae	.LBB1_14
.LBB1_13:
	movl	$4, %esi
	xorl	%eax, %eax
	cmpl	$1, %r8d
	jne	.LBB1_21
.LBB1_20:
	movl	%r8d, %esi
	jmp	.LBB1_24
.LBB1_14:
	movl	$1, 16(%rbx)
	xorl	%eax, %eax
	testl	%ecx, %ecx
	setne	%al
	leal	(%r10,%rax,8), %eax
	movl	%eax, 20(%rbx)
	jmp	.LBB1_18
.LBB1_17:
	movl	$1, 16(%rbx)
	xorl	%eax, %eax
	testl	%ecx, %ecx
	setne	%al
	leal	(%r11,%rax,8), %eax
	movl	%eax, 20(%rbx)
	xorl	%esi, %esi
.LBB1_18:
	movb	$1, %al
	cmpl	$1, %r8d
	je	.LBB1_20
.LBB1_21:
	cmpl	$2, %r8d
	jne	.LBB1_23
# %bb.22:
	movl	$4, %esi
	jmp	.LBB1_24
.LBB1_23:
	testb	%al, %al
	jne	.LBB1_25
.LBB1_24:
	callq	rd_imm_sext
	movq	%rax, 40(%rbx)
.LBB1_25:
	popq	%rbx
	popq	%r14
	popq	%r15
	retq
.Lfunc_end1:
	.size	parse_modrm, .Lfunc_end1-parse_modrm
                                        # -- End function
	.p2align	4                               # -- Begin function rd_imm_sext
	.type	rd_imm_sext,@function
rd_imm_sext:                            # @rd_imm_sext
# %bb.0:
	cmpl	$4, %esi
	je	.LBB2_12
# %bb.1:
	cmpl	$2, %esi
	je	.LBB2_5
# %bb.2:
	cmpl	$1, %esi
	jne	rd64                            # TAILCALL
# %bb.3:
	movl	12(%rdi), %eax
	cmpl	8(%rdi), %eax
	jae	.LBB2_4
# %bb.25:
	movq	(%rdi), %rcx
	leal	1(%rax), %edx
	movl	%edx, 12(%rdi)
	movsbq	(%rcx,%rax), %rax
	retq
.LBB2_5:
	movl	8(%rdi), %edx
	movl	12(%rdi), %ecx
	cmpl	%edx, %ecx
	jae	.LBB2_6
# %bb.7:
	movq	(%rdi), %rax
	movl	%ecx, %esi
	incl	%esi
	movl	%esi, 12(%rdi)
	movzbl	(%rax,%rcx), %eax
	movl	%esi, %ecx
	cmpl	%edx, %ecx
	jb	.LBB2_10
.LBB2_9:
	movl	$1, 16(%rdi)
	xorl	%ecx, %ecx
	jmp	.LBB2_11
.LBB2_12:
	movl	8(%rdi), %ecx
	movl	12(%rdi), %edx
	cmpl	%ecx, %edx
	jae	.LBB2_13
# %bb.14:
	movq	(%rdi), %rax
	movl	%edx, %esi
	incl	%esi
	movl	%esi, 12(%rdi)
	movzbl	(%rax,%rdx), %eax
	movl	%esi, %edx
	cmpl	%ecx, %edx
	jb	.LBB2_17
.LBB2_16:
	movl	$1, 16(%rdi)
	xorl	%esi, %esi
	cmpl	%ecx, %edx
	jb	.LBB2_20
.LBB2_19:
	movl	$1, 16(%rdi)
	xorl	%r8d, %r8d
	cmpl	%ecx, %edx
	jb	.LBB2_23
.LBB2_22:
	movl	$1, 16(%rdi)
	xorl	%ecx, %ecx
	jmp	.LBB2_24
.LBB2_4:
	movl	$1, 16(%rdi)
	xorl	%eax, %eax
	retq
.LBB2_6:
	movl	$1, 16(%rdi)
	xorl	%eax, %eax
	cmpl	%edx, %ecx
	jae	.LBB2_9
.LBB2_10:
	movq	(%rdi), %rdx
	leal	1(%rcx), %esi
	movl	%esi, 12(%rdi)
	movl	%ecx, %ecx
	movzbl	(%rdx,%rcx), %ecx
	shll	$8, %ecx
.LBB2_11:
	orl	%eax, %ecx
	movswq	%cx, %rax
	retq
.LBB2_13:
	movl	$1, 16(%rdi)
	xorl	%eax, %eax
	cmpl	%ecx, %edx
	jae	.LBB2_16
.LBB2_17:
	movq	(%rdi), %rsi
	movl	%edx, %r8d
	incl	%edx
	movl	%edx, 12(%rdi)
	movzbl	(%rsi,%r8), %esi
	shll	$8, %esi
                                        # kill: def $edx killed $edx def $rdx
	cmpl	%ecx, %edx
	jae	.LBB2_19
.LBB2_20:
	movq	(%rdi), %r8
	movl	%edx, %r9d
	incl	%edx
	movl	%edx, 12(%rdi)
	movzbl	(%r8,%r9), %r8d
	shll	$16, %r8d
                                        # kill: def $edx killed $edx def $rdx
	cmpl	%ecx, %edx
	jae	.LBB2_22
.LBB2_23:
	movq	(%rdi), %rcx
	leal	1(%rdx), %r9d
	movl	%r9d, 12(%rdi)
	movl	%edx, %edx
	movzbl	(%rcx,%rdx), %ecx
	shll	$24, %ecx
.LBB2_24:
	orl	%ecx, %r8d
	orl	%eax, %esi
	orl	%r8d, %esi
	movslq	%esi, %rax
	retq
.Lfunc_end2:
	.size	rd_imm_sext, .Lfunc_end2-rd_imm_sext
                                        # -- End function
	.p2align	4                               # -- Begin function sb_0xhex
	.type	sb_0xhex,@function
sb_0xhex:                               # @sb_0xhex
# %bb.0:
	movl	8(%rdi), %edx
	movl	12(%rdi), %eax
	leal	1(%rax), %ecx
	cmpl	%edx, %ecx
	jb	.LBB3_1
# %bb.2:
	cmpl	%edx, %ecx
	jb	.LBB3_3
.LBB3_4:
	testq	%rsi, %rsi
	je	.LBB3_10
.LBB3_5:
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
	movb	%r8b, -24(%rsp,%rax)
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
	movzbl	-25(%rsp,%rax), %esi
	movq	(%rdi), %r8
	movl	%edx, 12(%rdi)
	movb	%sil, (%r8,%rcx)
	movq	(%rdi), %rcx
	movl	12(%rdi), %edx
	movb	$0, (%rcx,%rdx)
	jmp	.LBB3_9
.LBB3_1:
	movq	(%rdi), %rdx
	movl	%ecx, 12(%rdi)
	movb	$48, (%rdx,%rax)
	movq	(%rdi), %rax
	movl	12(%rdi), %ecx
	movb	$0, (%rax,%rcx)
	movl	8(%rdi), %edx
	movl	12(%rdi), %eax
	leal	1(%rax), %ecx
	cmpl	%edx, %ecx
	jae	.LBB3_4
.LBB3_3:
	movq	(%rdi), %rdx
	movl	%ecx, 12(%rdi)
	movl	%eax, %eax
	movb	$120, (%rdx,%rax)
	movq	(%rdi), %rax
	movl	12(%rdi), %ecx
	movb	$0, (%rax,%rcx)
	testq	%rsi, %rsi
	jne	.LBB3_5
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
	retq
.Lfunc_end3:
	.size	sb_0xhex, .Lfunc_end3-sb_0xhex
                                        # -- End function
	.p2align	4                               # -- Begin function render_rm
	.type	render_rm,@function
render_rm:                              # @render_rm
# %bb.0:
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
	je	render_mem                      # TAILCALL
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
	je	render_mem                      # TAILCALL
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
	retq
.Lfunc_end4:
	.size	render_rm, .Lfunc_end4-render_rm
                                        # -- End function
	.p2align	4                               # -- Begin function reg_name
	.type	reg_name,@function
reg_name:                               # @reg_name
# %bb.0:
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
	cmpl	$0, 12(%rsi)
	je	.LBB6_3
# %bb.4:
	movl	8(%rdi), %r8d
	movl	12(%rdi), %eax
	leal	1(%rax), %edx
	cmpl	%r8d, %edx
	jae	.LBB6_6
# %bb.5:
	movq	(%rdi), %rcx
	movl	%edx, 12(%rdi)
	movb	$114, (%rcx,%rax)
	movq	(%rdi), %rax
	movl	12(%rdi), %ecx
	movb	$0, (%rax,%rcx)
	movl	8(%rdi), %r8d
	movl	12(%rdi), %eax
	leal	1(%rax), %edx
.LBB6_6:
	cmpl	%r8d, %edx
	jae	.LBB6_8
# %bb.7:
	movq	(%rdi), %rcx
	movl	%edx, 12(%rdi)
	movl	%eax, %eax
	movb	$105, (%rcx,%rax)
	movq	(%rdi), %rax
	movl	12(%rdi), %ecx
	movb	$0, (%rax,%rcx)
	movl	8(%rdi), %r8d
	movl	12(%rdi), %eax
	leal	1(%rax), %edx
.LBB6_8:
	xorl	%ecx, %ecx
	cmpl	%r8d, %edx
	jae	.LBB6_10
# %bb.9:
	movq	(%rdi), %r8
	movl	%edx, 12(%rdi)
	movl	%eax, %eax
	movb	$112, (%r8,%rax)
	movq	(%rdi), %rax
	movl	12(%rdi), %edx
	movb	$0, (%rax,%rdx)
	jmp	.LBB6_10
.LBB6_3:
	movl	$1, %ecx
.LBB6_10:
	pushq	%rbx
	subq	$16, %rsp
	cmpl	$0, 16(%rsi)
	je	.LBB6_16
# %bb.11:
	movl	20(%rsi), %eax
	andl	$15, %eax
	leaq	R64(%rip), %rcx
	movq	(%rcx,%rax,8), %rax
	movzbl	(%rax), %edx
	xorl	%ecx, %ecx
	testb	%dl, %dl
	je	.LBB6_16
# %bb.12:
	incq	%rax
	jmp	.LBB6_13
	.p2align	4
.LBB6_15:                               #   in Loop: Header=BB6_13 Depth=1
	movzbl	(%rax), %edx
	incq	%rax
	testb	%dl, %dl
	je	.LBB6_16
.LBB6_13:                               # =>This Inner Loop Header: Depth=1
	movl	12(%rdi), %r8d
	leal	1(%r8), %r9d
	cmpl	8(%rdi), %r9d
	jae	.LBB6_15
# %bb.14:                               #   in Loop: Header=BB6_13 Depth=1
	movq	(%rdi), %r10
	movl	%r9d, 12(%rdi)
	movb	%dl, (%r10,%r8)
	movq	(%rdi), %rdx
	movl	12(%rdi), %r8d
	movb	$0, (%rdx,%r8)
	jmp	.LBB6_15
.LBB6_16:
	cmpl	$0, 24(%rsi)
	je	.LBB6_29
# %bb.17:
	testl	%ecx, %ecx
	jne	.LBB6_20
# %bb.18:
	movl	12(%rdi), %eax
	leal	1(%rax), %ecx
	cmpl	8(%rdi), %ecx
	jae	.LBB6_20
# %bb.19:
	movq	(%rdi), %rdx
	movl	%ecx, 12(%rdi)
	movb	$43, (%rdx,%rax)
	movq	(%rdi), %rax
	movl	12(%rdi), %ecx
	movb	$0, (%rax,%rcx)
.LBB6_20:
	movl	28(%rsi), %eax
	andl	$15, %eax
	leaq	R64(%rip), %rcx
	movq	(%rcx,%rax,8), %rax
	movzbl	(%rax), %ecx
	testb	%cl, %cl
	je	.LBB6_25
# %bb.21:
	incq	%rax
	jmp	.LBB6_22
	.p2align	4
.LBB6_24:                               #   in Loop: Header=BB6_22 Depth=1
	movzbl	(%rax), %ecx
	incq	%rax
	testb	%cl, %cl
	je	.LBB6_25
.LBB6_22:                               # =>This Inner Loop Header: Depth=1
	movl	12(%rdi), %edx
	leal	1(%rdx), %r8d
	cmpl	8(%rdi), %r8d
	jae	.LBB6_24
# %bb.23:                               #   in Loop: Header=BB6_22 Depth=1
	movq	(%rdi), %r9
	movl	%r8d, 12(%rdi)
	movb	%cl, (%r9,%rdx)
	movq	(%rdi), %rcx
	movl	12(%rdi), %edx
	movb	$0, (%rcx,%rdx)
	jmp	.LBB6_24
.LBB6_25:
	movl	8(%rdi), %r8d
	movl	12(%rdi), %eax
	leal	1(%rax), %edx
	cmpl	%r8d, %edx
	jae	.LBB6_27
# %bb.26:
	movq	(%rdi), %rcx
	movl	%edx, 12(%rdi)
	movb	$42, (%rcx,%rax)
	movq	(%rdi), %rax
	movl	12(%rdi), %ecx
	movb	$0, (%rax,%rcx)
	movl	8(%rdi), %r8d
	movl	12(%rdi), %eax
	leal	1(%rax), %edx
.LBB6_27:
	xorl	%ecx, %ecx
	cmpl	%r8d, %edx
	jae	.LBB6_29
# %bb.28:
	movzbl	32(%rsi), %r8d
	addb	$48, %r8b
	movq	(%rdi), %r9
	movl	%edx, 12(%rdi)
	movl	%eax, %eax
	movb	%r8b, (%r9,%rax)
	movq	(%rdi), %rax
	movl	12(%rdi), %edx
	movb	$0, (%rax,%rdx)
.LBB6_29:
	movq	40(%rsi), %rax
	testq	%rax, %rax
	sete	%dl
	testl	%ecx, %ecx
	sete	%r8b
	testb	%dl, %r8b
	jne	.LBB6_59
# %bb.30:
	testl	%ecx, %ecx
	je	.LBB6_32
# %bb.31:
	movq	%rdi, %rbx
	movq	%rax, %rsi
	callq	sb_0xhex
	movq	%rbx, %rdi
.LBB6_59:
	movl	12(%rdi), %eax
	leal	1(%rax), %ecx
	cmpl	8(%rdi), %ecx
	jae	.LBB6_61
# %bb.60:
	movq	(%rdi), %rdx
	movl	%ecx, 12(%rdi)
	movb	$93, (%rdx,%rax)
	movq	(%rdi), %rax
	movl	12(%rdi), %ecx
	movb	$0, (%rax,%rcx)
.LBB6_61:
	addq	$16, %rsp
	popq	%rbx
	retq
.LBB6_32:
	movl	8(%rdi), %r8d
	movl	12(%rdi), %ecx
	leal	1(%rcx), %edx
	testq	%rax, %rax
	js	.LBB6_45
# %bb.33:
	cmpl	%r8d, %edx
	jb	.LBB6_34
# %bb.35:
	cmpl	%r8d, %edx
	jb	.LBB6_36
.LBB6_37:
	cmpl	%r8d, %edx
	jae	.LBB6_39
.LBB6_38:
	movq	(%rdi), %rax
	movl	%edx, 12(%rdi)
	movl	%ecx, %ecx
	movb	$120, (%rax,%rcx)
	movq	(%rdi), %rax
	movl	12(%rdi), %ecx
	movb	$0, (%rax,%rcx)
.LBB6_39:
	movq	40(%rsi), %rcx
	testq	%rcx, %rcx
	je	.LBB6_57
# %bb.40:
	xorl	%eax, %eax
	movq	%rcx, %rdx
	.p2align	4
.LBB6_41:                               # =>This Inner Loop Header: Depth=1
	movl	%ecx, %esi
	andl	$15, %esi
	leal	87(%rsi), %r8d
	leal	48(%rsi), %r9d
	cmpl	$10, %esi
	movzbl	%r9b, %esi
	movzbl	%r8b, %r8d
	cmovbl	%esi, %r8d
	movb	%r8b, (%rsp,%rax)
	incq	%rax
	shrq	$4, %rdx
	cmpq	$15, %rcx
	movq	%rdx, %rcx
	ja	.LBB6_41
	jmp	.LBB6_42
	.p2align	4
.LBB6_44:                               #   in Loop: Header=BB6_42 Depth=1
	decq	%rax
	je	.LBB6_59
.LBB6_42:                               # =>This Inner Loop Header: Depth=1
	movl	12(%rdi), %ecx
	leal	1(%rcx), %edx
	cmpl	8(%rdi), %edx
	jae	.LBB6_44
# %bb.43:                               #   in Loop: Header=BB6_42 Depth=1
	movzbl	-1(%rsp,%rax), %esi
	movq	(%rdi), %r8
	movl	%edx, 12(%rdi)
	movb	%sil, (%r8,%rcx)
	movq	(%rdi), %rcx
	movl	12(%rdi), %edx
	movb	$0, (%rcx,%rdx)
	jmp	.LBB6_44
.LBB6_45:
	cmpl	%r8d, %edx
	jb	.LBB6_46
# %bb.47:
	cmpl	%r8d, %edx
	jb	.LBB6_48
.LBB6_49:
	cmpl	%r8d, %edx
	jae	.LBB6_51
.LBB6_50:
	movq	(%rdi), %rax
	movl	%edx, 12(%rdi)
	movl	%ecx, %ecx
	movb	$120, (%rax,%rcx)
	movq	(%rdi), %rax
	movl	12(%rdi), %ecx
	movb	$0, (%rax,%rcx)
.LBB6_51:
	movq	40(%rsi), %rcx
	testq	%rcx, %rcx
	je	.LBB6_57
# %bb.52:
	negq	%rcx
	xorl	%eax, %eax
	movq	%rcx, %rdx
	.p2align	4
.LBB6_53:                               # =>This Inner Loop Header: Depth=1
	movl	%ecx, %esi
	andl	$15, %esi
	leal	87(%rsi), %r8d
	leal	48(%rsi), %r9d
	cmpl	$10, %esi
	movzbl	%r9b, %esi
	movzbl	%r8b, %r8d
	cmovbl	%esi, %r8d
	movb	%r8b, (%rsp,%rax)
	incq	%rax
	shrq	$4, %rdx
	cmpq	$15, %rcx
	movq	%rdx, %rcx
	ja	.LBB6_53
	jmp	.LBB6_54
	.p2align	4
.LBB6_56:                               #   in Loop: Header=BB6_54 Depth=1
	decq	%rax
	je	.LBB6_59
.LBB6_54:                               # =>This Inner Loop Header: Depth=1
	movl	12(%rdi), %ecx
	leal	1(%rcx), %edx
	cmpl	8(%rdi), %edx
	jae	.LBB6_56
# %bb.55:                               #   in Loop: Header=BB6_54 Depth=1
	movzbl	-1(%rsp,%rax), %esi
	movq	(%rdi), %r8
	movl	%edx, 12(%rdi)
	movb	%sil, (%r8,%rcx)
	movq	(%rdi), %rcx
	movl	12(%rdi), %edx
	movb	$0, (%rcx,%rdx)
	jmp	.LBB6_56
.LBB6_57:
	movl	12(%rdi), %eax
	leal	1(%rax), %ecx
	cmpl	8(%rdi), %ecx
	jae	.LBB6_59
# %bb.58:
	movq	(%rdi), %rdx
	movl	%ecx, 12(%rdi)
	movb	$48, (%rdx,%rax)
	movq	(%rdi), %rax
	movl	12(%rdi), %ecx
	movb	$0, (%rax,%rcx)
	jmp	.LBB6_59
.LBB6_34:
	movq	(%rdi), %rax
	movl	%edx, 12(%rdi)
	movb	$43, (%rax,%rcx)
	movq	(%rdi), %rax
	movl	12(%rdi), %ecx
	movb	$0, (%rax,%rcx)
	movl	8(%rdi), %r8d
	movl	12(%rdi), %ecx
	leal	1(%rcx), %edx
	cmpl	%r8d, %edx
	jae	.LBB6_37
.LBB6_36:
	movq	(%rdi), %rax
	movl	%edx, 12(%rdi)
	movl	%ecx, %ecx
	movb	$48, (%rax,%rcx)
	movq	(%rdi), %rax
	movl	12(%rdi), %ecx
	movb	$0, (%rax,%rcx)
	movl	8(%rdi), %r8d
	movl	12(%rdi), %ecx
	leal	1(%rcx), %edx
	cmpl	%r8d, %edx
	jb	.LBB6_38
	jmp	.LBB6_39
.LBB6_46:
	movq	(%rdi), %rax
	movl	%edx, 12(%rdi)
	movb	$45, (%rax,%rcx)
	movq	(%rdi), %rax
	movl	12(%rdi), %ecx
	movb	$0, (%rax,%rcx)
	movl	8(%rdi), %r8d
	movl	12(%rdi), %ecx
	leal	1(%rcx), %edx
	cmpl	%r8d, %edx
	jae	.LBB6_49
.LBB6_48:
	movq	(%rdi), %rax
	movl	%edx, 12(%rdi)
	movl	%ecx, %ecx
	movb	$48, (%rax,%rcx)
	movq	(%rdi), %rax
	movl	12(%rdi), %ecx
	movb	$0, (%rax,%rcx)
	movl	8(%rdi), %r8d
	movl	12(%rdi), %ecx
	leal	1(%rcx), %edx
	cmpl	%r8d, %edx
	jb	.LBB6_50
	jmp	.LBB6_51
.Lfunc_end6:
	.size	render_mem, .Lfunc_end6-render_mem
                                        # -- End function
	.p2align	4                               # -- Begin function rd64
	.type	rd64,@function
rd64:                                   # @rd64
# %bb.0:
	movl	8(%rdi), %eax
	movl	12(%rdi), %r8d
	cmpl	%eax, %r8d
	jae	.LBB7_1
# %bb.2:
	movq	(%rdi), %rcx
	movl	%r8d, %edx
	incl	%edx
	movl	%edx, 12(%rdi)
	movzbl	(%rcx,%r8), %ecx
	movl	%edx, %r8d
	cmpl	%eax, %r8d
	jb	.LBB7_5
.LBB7_4:
	movl	$1, 16(%rdi)
	xorl	%edx, %edx
	cmpl	%eax, %r8d
	jb	.LBB7_8
.LBB7_7:
	movl	$1, 16(%rdi)
	xorl	%esi, %esi
	cmpl	%eax, %r8d
	jb	.LBB7_11
.LBB7_10:
	movl	$1, 16(%rdi)
	xorl	%r9d, %r9d
	cmpl	%eax, %r8d
	jb	.LBB7_14
.LBB7_13:
	movl	$1, 16(%rdi)
	xorl	%r10d, %r10d
	jmp	.LBB7_15
.LBB7_1:
	movl	$1, 16(%rdi)
	xorl	%ecx, %ecx
	cmpl	%eax, %r8d
	jae	.LBB7_4
.LBB7_5:
	movq	(%rdi), %rdx
	movl	%r8d, %esi
	incl	%r8d
	movl	%r8d, 12(%rdi)
	movzbl	(%rdx,%rsi), %edx
	shll	$8, %edx
                                        # kill: def $r8d killed $r8d def $r8
	cmpl	%eax, %r8d
	jae	.LBB7_7
.LBB7_8:
	movq	(%rdi), %rsi
	movl	%r8d, %r9d
	incl	%r8d
	movl	%r8d, 12(%rdi)
	movzbl	(%rsi,%r9), %esi
	shll	$16, %esi
                                        # kill: def $r8d killed $r8d def $r8
	cmpl	%eax, %r8d
	jae	.LBB7_10
.LBB7_11:
	movq	(%rdi), %r9
	movl	%r8d, %r10d
	incl	%r8d
	movl	%r8d, 12(%rdi)
	movzbl	(%r9,%r10), %r9d
	shll	$24, %r9d
                                        # kill: def $r8d killed $r8d def $r8
	cmpl	%eax, %r8d
	jae	.LBB7_13
.LBB7_14:
	movq	(%rdi), %r10
	movl	%r8d, %r11d
	incl	%r8d
	movl	%r8d, 12(%rdi)
	movzbl	(%r10,%r11), %r10d
                                        # kill: def $r8d killed $r8d def $r8
.LBB7_15:
	pushq	%rbp
	pushq	%r14
	pushq	%rbx
	cmpl	%eax, %r8d
	jae	.LBB7_16
# %bb.17:
	movq	(%rdi), %r11
	movl	%r8d, %ebx
	incl	%r8d
	movl	%r8d, 12(%rdi)
	movzbl	(%r11,%rbx), %r11d
	shll	$8, %r11d
                                        # kill: def $r8d killed $r8d def $r8
	cmpl	%eax, %r8d
	jb	.LBB7_20
.LBB7_19:
	movl	$1, 16(%rdi)
	xorl	%ebx, %ebx
	cmpl	%eax, %r8d
	jb	.LBB7_23
.LBB7_22:
	movl	$1, 16(%rdi)
	xorl	%eax, %eax
	jmp	.LBB7_24
.LBB7_16:
	movl	$1, 16(%rdi)
	xorl	%r11d, %r11d
	cmpl	%eax, %r8d
	jae	.LBB7_19
.LBB7_20:
	movq	(%rdi), %rbx
	movl	%r8d, %r14d
	incl	%r8d
	movl	%r8d, 12(%rdi)
	movzbl	(%rbx,%r14), %ebx
	shll	$16, %ebx
                                        # kill: def $r8d killed $r8d def $r8
	cmpl	%eax, %r8d
	jae	.LBB7_22
.LBB7_23:
	movq	(%rdi), %rax
	leal	1(%r8), %ebp
	movl	%ebp, 12(%rdi)
	movl	%r8d, %edi
	movzbl	(%rax,%rdi), %eax
	shll	$24, %eax
.LBB7_24:
	orq	%rcx, %rdx
	orq	%r9, %rsi
	orq	%rdx, %rsi
	orl	%r10d, %r11d
	orl	%ebx, %eax
	orl	%r11d, %eax
	shlq	$32, %rax
	orq	%rsi, %rax
	popq	%rbx
	popq	%r14
	popq	%rbp
	retq
.Lfunc_end7:
	.size	rd64, .Lfunc_end7-rd64
                                        # -- End function
	.type	.L.str.3,@object                # @.str.3
	.section	.rodata.str1.1,"aMS",@progbits,1
.L.str.3:
	.asciz	"nop"
	.size	.L.str.3, 4

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

	.type	.L.str.11,@object               # @.str.11
	.section	.rodata.str1.1,"aMS",@progbits,1
.L.str.11:
	.asciz	"movsx "
	.size	.L.str.11, 7

	.type	.L.str.12,@object               # @.str.12
.L.str.12:
	.asciz	"movzx "
	.size	.L.str.12, 7

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

	.type	R64,@object                     # @R64
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

	.type	.L.str.57,@object               # @.str.57
	.section	.rodata.str1.1,"aMS",@progbits,1
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

	.type	.L.str.79,@object               # @.str.79
	.section	.rodata.str1.1,"aMS",@progbits,1
.L.str.79:
	.asciz	"call "
	.size	.L.str.79, 6

	.type	.L.str.80,@object               # @.str.80
.L.str.80:
	.asciz	"jmp "
	.size	.L.str.80, 5

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
