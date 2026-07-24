	.file	"demo.c"
	.text
	.globl	sym_by_addr                     # -- Begin function sym_by_addr
	.p2align	4
	.type	sym_by_addr,@function
sym_by_addr:                            # @sym_by_addr
# %bb.0:
	pushq	%rbp
	movq	%rsp, %rbp
	testl	%esi, %esi
	jle	.LBB0_1
# %bb.2:
	decl	%esi
	movl	$-1, %eax
	xorl	%ecx, %ecx
	jmp	.LBB0_3
	.p2align	4
.LBB0_5:                                #   in Loop: Header=BB0_3 Depth=1
	addl	%r8d, %ecx
	incl	%ecx
	movl	%r9d, %eax
	cmpl	%esi, %ecx
	jg	.LBB0_7
.LBB0_3:                                # =>This Inner Loop Header: Depth=1
	movl	%esi, %r9d
	subl	%ecx, %r9d
	movl	%r9d, %r8d
	shrl	$31, %r8d
	addl	%r9d, %r8d
	sarl	%r8d
	leal	(%r8,%rcx), %r9d
	movslq	%r9d, %r10
	shlq	$4, %r10
	cmpq	%rdx, (%rdi,%r10)
	jbe	.LBB0_5
# %bb.4:                                #   in Loop: Header=BB0_3 Depth=1
	leal	(%r8,%rcx), %esi
	decl	%esi
	cmpl	%esi, %ecx
	jle	.LBB0_3
.LBB0_7:
	popq	%rbp
	retq
.LBB0_1:
	movl	$-1, %eax
	popq	%rbp
	retq
.Lfunc_end0:
	.size	sym_by_addr, .Lfunc_end0-sym_by_addr
                                        # -- End function
	.globl	modrm_bytes                     # -- Begin function modrm_bytes
	.p2align	4
	.type	modrm_bytes,@function
modrm_bytes:                            # @modrm_bytes
# %bb.0:
	pushq	%rbp
	movq	%rsp, %rbp
	testl	%esi, %esi
	je	.LBB1_1
# %bb.2:
	movzbl	(%rdi), %r8d
	movl	%r8d, %ecx
	shrl	$6, %ecx
	movl	$1, %eax
	cmpl	$3, %ecx
	jne	.LBB1_3
.LBB1_13:
	popq	%rbp
	retq
.LBB1_1:
	movl	$0, (%rdx)
	xorl	%eax, %eax
	popq	%rbp
	retq
.LBB1_3:
	movl	%r8d, %r9d
	andl	$7, %r9d
	cmpl	$4, %r9d
	jne	.LBB1_8
# %bb.4:
	cmpl	$1, %esi
	jne	.LBB1_6
# %bb.5:
	movl	$0, (%rdx)
	popq	%rbp
	retq
.LBB1_8:
	xorl	%eax, %eax
	cmpl	$5, %r9d
	sete	%al
	cmpb	$64, %r8b
	leal	1(,%rax,4), %edx
	movl	$1, %eax
	cmovbl	%edx, %eax
.LBB1_9:
	cmpl	$2, %ecx
	je	.LBB1_12
# %bb.10:
	cmpl	$1, %ecx
	jne	.LBB1_13
# %bb.11:
	incl	%eax
	popq	%rbp
	retq
.LBB1_12:
	addl	$4, %eax
	popq	%rbp
	retq
.LBB1_6:
	movl	$2, %eax
	cmpb	$63, %r8b
	ja	.LBB1_9
# %bb.7:
	movzbl	1(%rdi), %eax
	andb	$7, %al
	xorl	%edx, %edx
	cmpb	$5, %al
	sete	%dl
	leal	2(,%rdx,4), %eax
	jmp	.LBB1_9
.Lfunc_end1:
	.size	modrm_bytes, .Lfunc_end1-modrm_bytes
                                        # -- End function
	.globl	x86_insn_len                    # -- Begin function x86_insn_len
	.p2align	4
	.type	x86_insn_len,@function
x86_insn_len:                           # @x86_insn_len
# %bb.0:
	pushq	%rbp
	movq	%rsp, %rbp
	pushq	%r15
	pushq	%r14
	pushq	%r13
	pushq	%r12
	pushq	%rbx
	pushq	%rax
	xorl	%eax, %eax
	movabsq	$198158383604367617, %rcx       # imm = 0x2C0000000010101
	xorl	%r15d, %r15d
	xorl	%ebx, %ebx
	jmp	.LBB2_1
.LBB2_10:                               #   in Loop: Header=BB2_1 Depth=1
	xorl	%edx, %edx
	testb	%dl, %dl
	je	.LBB2_11
	.p2align	4
.LBB2_1:                                # =>This Inner Loop Header: Depth=1
	cmpl	%esi, %ebx
	jae	.LBB2_34
# %bb.2:                                #   in Loop: Header=BB2_1 Depth=1
	movl	%ebx, %edx
	movzbl	(%rdi,%rdx), %edx
	leal	-46(%rdx), %r9d
	cmpl	$57, %r9d
	ja	.LBB2_5
# %bb.3:                                #   in Loop: Header=BB2_1 Depth=1
	btq	%r9, %rcx
	jb	.LBB2_8
# %bb.4:                                #   in Loop: Header=BB2_1 Depth=1
	movl	$1, %r8d
	cmpq	$56, %r9
	je	.LBB2_9
	.p2align	4
.LBB2_5:                                #   in Loop: Header=BB2_1 Depth=1
	leal	-240(%rdx), %r8d
	cmpl	$3, %r8d
	ja	.LBB2_7
# %bb.6:                                #   in Loop: Header=BB2_1 Depth=1
	cmpl	$1, %r8d
	jne	.LBB2_8
.LBB2_7:                                #   in Loop: Header=BB2_1 Depth=1
	cmpl	$38, %edx
	jne	.LBB2_10
	.p2align	4
.LBB2_8:                                #   in Loop: Header=BB2_1 Depth=1
	movl	%r15d, %r8d
.LBB2_9:                                #   in Loop: Header=BB2_1 Depth=1
	incl	%ebx
	movb	$1, %dl
	movl	%r8d, %r15d
	testb	%dl, %dl
	jne	.LBB2_1
.LBB2_11:
	movb	$1, %cl
	cmpl	%esi, %ebx
	jae	.LBB2_14
# %bb.12:
	movl	%ebx, %eax
	movzbl	(%rdi,%rax), %eax
	movl	%eax, %edx
	andb	$-16, %dl
	cmpb	$64, %dl
	jne	.LBB2_14
# %bb.13:
	incl	%ebx
	testb	$8, %al
	sete	%cl
.LBB2_14:
	xorl	%eax, %eax
	cmpl	%esi, %ebx
	jae	.LBB2_34
# %bb.15:
	leal	1(%rbx), %r14d
	movl	%ebx, %edx
	movzbl	(%rdi,%rdx), %r12d
	cmpb	$15, %r12b
	jne	.LBB2_21
# %bb.16:
	cmpl	%r14d, %esi
	jbe	.LBB2_34
# %bb.17:
	movl	%r14d, %eax
	cmpb	$-113, (%rdi,%rax)
	jg	.LBB2_26
# %bb.18:
	addl	$6, %ebx
.LBB2_19:
	xorl	%eax, %eax
	cmpl	%esi, %ebx
.LBB2_20:
	cmovbel	%ebx, %eax
	jmp	.LBB2_34
.LBB2_21:
	movl	%r12d, %edx
	andb	$-16, %dl
	cmpb	$80, %dl
	je	.LBB2_22
# %bb.23:
	movzbl	%r12b, %edx
	leal	-144(%rdx), %r8d
	cmpl	$60, %r8d
	ja	.LBB2_28
# %bb.24:
	movabsq	$1299288492496388865, %r9       # imm = 0x1208000000000301
	btq	%r8, %r9
	jae	.LBB2_28
.LBB2_22:
	movl	%r14d, %eax
	jmp	.LBB2_34
.LBB2_26:
	addl	$2, %ebx
	xorl	%ecx, %ecx
	movb	$1, %r8b
	subl	%ebx, %esi
	jne	.LBB2_30
# %bb.27:
	xorl	%eax, %eax
	jmp	.LBB2_32
.LBB2_28:
	cmpl	$244, %edx
	je	.LBB2_22
# %bb.35:
	cmpb	$63, %r12b
	ja	.LBB2_44
# %bb.36:
	movl	%r12d, %r8d
	andb	$7, %r8b
	cmpb	$5, %r8b
	ja	.LBB2_44
# %bb.37:
	je	.LBB2_67
# %bb.38:
	movzbl	%r8b, %eax
	cmpl	$4, %eax
	jne	.LBB2_68
# %bb.39:
	addl	$2, %ebx
	jmp	.LBB2_19
.LBB2_30:
	movl	%ebx, %r9d
	movzbl	(%rdi,%r9), %r10d
	movl	%r10d, %edx
	shrl	$6, %edx
	movl	$1, %eax
	cmpl	$3, %edx
	jne	.LBB2_40
.LBB2_31:
	xorl	%r8d, %r8d
.LBB2_32:
	addl	%ebx, %eax
.LBB2_33:
	testb	%r8b, %r8b
	cmovnel	%ecx, %eax
.LBB2_34:
	addq	$8, %rsp
	popq	%rbx
	popq	%r12
	popq	%r13
	popq	%r14
	popq	%r15
	popq	%rbp
	retq
.LBB2_40:
	movl	%r10d, %r11d
	andl	$7, %r11d
	cmpl	$4, %r11d
	jne	.LBB2_51
# %bb.41:
	cmpl	$1, %esi
	je	.LBB2_32
# %bb.42:
	movl	$2, %eax
	cmpb	$63, %r10b
	ja	.LBB2_52
# %bb.43:
	movzbl	1(%rdi,%r9), %eax
	andb	$7, %al
	xorl	%esi, %esi
	cmpb	$5, %al
	sete	%sil
	leal	2(,%rsi,4), %eax
	jmp	.LBB2_52
.LBB2_44:
	movl	%r12d, %r8d
	andb	$-4, %r8b
	cmpb	$-120, %r8b
	je	.LBB2_49
# %bb.45:
	leal	-99(%rdx), %r9d
	cmpl	$42, %r9d
	ja	.LBB2_47
# %bb.46:
	movabsq	$4423816314881, %r10            # imm = 0x40600000001
	btq	%r9, %r10
	jb	.LBB2_49
.LBB2_47:
	cmpl	$255, %edx
	je	.LBB2_49
# %bb.48:
	cmpb	$-48, %r8b
	jne	.LBB2_71
.LBB2_49:
	xorl	%ecx, %ecx
	movb	$1, %r8b
	subl	%r14d, %esi
	jne	.LBB2_55
# %bb.50:
	xorl	%eax, %eax
	addl	%r14d, %eax
	jmp	.LBB2_33
.LBB2_51:
	xorl	%eax, %eax
	cmpl	$5, %r11d
	sete	%al
	cmpb	$64, %r10b
	leal	1(,%rax,4), %esi
	movl	$1, %eax
	cmovbl	%esi, %eax
.LBB2_52:
	cmpl	$2, %edx
	je	.LBB2_58
# %bb.53:
	cmpl	$1, %edx
	jne	.LBB2_31
# %bb.54:
	incl	%eax
	jmp	.LBB2_31
.LBB2_55:
	movl	%r14d, %r9d
	movzbl	(%rdi,%r9), %r10d
	movl	%r10d, %edx
	shrl	$6, %edx
	movl	$1, %eax
	cmpl	$3, %edx
	jne	.LBB2_59
.LBB2_56:
	xorl	%r8d, %r8d
.LBB2_57:
	addl	%r14d, %eax
	jmp	.LBB2_33
.LBB2_58:
	addl	$4, %eax
	jmp	.LBB2_31
.LBB2_59:
	movl	%r10d, %r11d
	andl	$7, %r11d
	cmpl	$4, %r11d
	jne	.LBB2_63
# %bb.60:
	cmpl	$1, %esi
	je	.LBB2_57
# %bb.61:
	movl	$2, %eax
	cmpb	$63, %r10b
	ja	.LBB2_64
# %bb.62:
	movzbl	1(%rdi,%r9), %eax
	andb	$7, %al
	xorl	%esi, %esi
	cmpb	$5, %al
	sete	%sil
	leal	2(,%rsi,4), %eax
	jmp	.LBB2_64
.LBB2_63:
	xorl	%eax, %eax
	cmpl	$5, %r11d
	sete	%al
	cmpb	$64, %r10b
	leal	1(,%rax,4), %esi
	movl	$1, %eax
	cmovbl	%esi, %eax
.LBB2_64:
	cmpl	$2, %edx
	je	.LBB2_70
# %bb.65:
	cmpl	$1, %edx
	jne	.LBB2_56
# %bb.66:
	incl	%eax
	jmp	.LBB2_56
.LBB2_67:
	xorl	%eax, %eax
	testl	%r15d, %r15d
	sete	%al
	leal	(%r14,%rax,2), %ecx
	addl	$2, %ecx
	xorl	%eax, %eax
	cmpl	%esi, %ecx
	cmovbel	%ecx, %eax
	jmp	.LBB2_34
.LBB2_68:
	movl	$1, -44(%rbp)
	movl	%r14d, %eax
	addq	%rax, %rdi
	subl	%r14d, %esi
	leaq	-44(%rbp), %rdx
	callq	modrm_bytes
	movl	-44(%rbp), %ecx
	addl	%r14d, %eax
.LBB2_69:
	testl	%ecx, %ecx
	cmovel	%ecx, %eax
	jmp	.LBB2_34
.LBB2_70:
	addl	$4, %eax
	jmp	.LBB2_56
.LBB2_71:
	cmpl	$191, %edx
	jg	.LBB2_78
# %bb.72:
	leal	-105(%rdx), %r8d
	cmpl	$38, %r8d
	ja	.LBB2_81
# %bb.73:
	movl	$75497476, %r9d                 # imm = 0x4800004
	btq	%r8, %r9
	jb	.LBB2_84
# %bb.74:
	movabsq	$276488519680, %r9              # imm = 0x4060000000
	btq	%r8, %r9
	jb	.LBB2_49
# %bb.75:
	movl	$16777217, %r9d                 # imm = 0x1000001
	btq	%r8, %r9
	jae	.LBB2_81
.LBB2_76:
	movl	$1, -44(%rbp)
	movl	%r14d, %eax
	addq	%rax, %rdi
	movl	%esi, %ebx
	subl	%r14d, %esi
	leaq	-44(%rbp), %rdx
	callq	modrm_bytes
                                        # kill: def $eax killed $eax def $rax
	cmpl	$0, -44(%rbp)
	je	.LBB2_85
# %bb.77:
	xorl	%ecx, %ecx
	testl	%r15d, %r15d
	sete	%cl
	leal	(%r14,%rcx,2), %ecx
	addl	%eax, %ecx
	addl	$2, %ecx
	xorl	%eax, %eax
	cmpl	%ebx, %ecx
	cmovbel	%ecx, %eax
	jmp	.LBB2_34
.LBB2_78:
	leal	-192(%rdx), %r8d
	cmpl	$2, %r8d
	jb	.LBB2_84
# %bb.79:
	cmpl	$198, %edx
	je	.LBB2_84
# %bb.80:
	cmpl	$199, %edx
	je	.LBB2_76
.LBB2_81:
	movl	%esi, %r13d
	movl	%r12d, %esi
	andb	$-2, %sil
	cmpb	$-10, %sil
	jne	.LBB2_86
# %bb.82:
	movl	$1, -44(%rbp)
	movl	%r14d, %eax
	addq	%rax, %rdi
	movl	%r13d, %esi
	subl	%r14d, %esi
	leaq	-44(%rbp), %rdx
	movq	%rdi, %rbx
	callq	modrm_bytes
	cmpl	$0, -44(%rbp)
	je	.LBB2_85
# %bb.83:
	movl	%eax, %ecx
	xorl	%eax, %eax
	testl	%r15d, %r15d
	sete	%al
	xorl	%edx, %edx
	testb	$48, (%rbx)
	sete	%dl
	leal	2(%rax,%rax), %eax
	cmovnel	%edx, %eax
	cmpb	$-10, %r12b
	cmovel	%edx, %eax
	addl	%r14d, %ecx
	addl	%eax, %ecx
	xorl	%eax, %eax
	cmpl	%r13d, %ecx
	cmovbel	%ecx, %eax
	jmp	.LBB2_34
.LBB2_84:
	movl	$1, -44(%rbp)
	movl	%r14d, %eax
	addq	%rax, %rdi
	movl	%esi, %eax
	subl	%r14d, %eax
	leaq	-44(%rbp), %rdx
	movl	%esi, %r14d
	movl	%eax, %esi
	callq	modrm_bytes
                                        # kill: def $eax killed $eax def $rax
	leal	(%rbx,%rax), %ecx
	addl	$2, %ecx
	xorl	%eax, %eax
	cmpl	%r14d, %ecx
	cmovbel	%ecx, %eax
	movl	-44(%rbp), %ecx
	jmp	.LBB2_69
.LBB2_85:
	xorl	%eax, %eax
	jmp	.LBB2_34
.LBB2_86:
	cmpb	$112, %r12b
	jge	.LBB2_88
# %bb.87:
	cmpb	$-21, %r12b
	jne	.LBB2_91
.LBB2_88:
	addl	$2, %ebx
.LBB2_89:
	xorl	%eax, %eax
	cmpl	%r13d, %ebx
	jmp	.LBB2_20
.LBB2_91:
	cmpb	$-24, %sil
	jne	.LBB2_93
.LBB2_92:
	addl	$5, %ebx
	jmp	.LBB2_89
.LBB2_93:
	movl	%r12d, %esi
	andb	$-8, %sil
	cmpb	$-72, %sil
	je	.LBB2_97
# %bb.94:
	movzbl	%sil, %ecx
	cmpl	$176, %ecx
	je	.LBB2_88
# %bb.95:
	cmpb	$106, %r12b
	je	.LBB2_88
# %bb.96:
	cmpl	$104, %edx
	je	.LBB2_92
	jmp	.LBB2_34
.LBB2_97:
	xorl	%eax, %eax
	testl	%r15d, %r15d
	sete	%al
	leal	2(,%rax,2), %eax
	testb	%cl, %cl
	movl	$8, %ecx
	cmovnel	%eax, %ecx
	addl	%ecx, %r14d
	xorl	%eax, %eax
	cmpl	%r13d, %r14d
	cmovbel	%r14d, %eax
	jmp	.LBB2_34
.Lfunc_end2:
	.size	x86_insn_len, .Lfunc_end2-x86_insn_len
                                        # -- End function
	.globl	demo_run                        # -- Begin function demo_run
	.p2align	4
	.type	demo_run,@function
demo_run:                               # @demo_run
# %bb.0:
	pushq	%rbp
	movq	%rsp, %rbp
	pushq	%r15
	pushq	%r14
	pushq	%r12
	pushq	%rbx
	movl	$3, %edx
	movl	$-1, %ecx
	xorl	%esi, %esi
	leaq	demo_run.tab(%rip), %rax
	jmp	.LBB3_1
	.p2align	4
.LBB3_3:                                #   in Loop: Header=BB3_1 Depth=1
	addl	%edi, %esi
	incl	%esi
	movl	%r8d, %ecx
	cmpl	%edx, %esi
	jg	.LBB3_5
.LBB3_1:                                # =>This Inner Loop Header: Depth=1
	movl	%edx, %r8d
	subl	%esi, %r8d
	movl	%r8d, %edi
	shrl	$31, %edi
	addl	%r8d, %edi
	sarl	%edi
	leal	(%rdi,%rsi), %r8d
	movslq	%r8d, %r9
	shlq	$4, %r9
	cmpq	$4241, (%r9,%rax)               # imm = 0x1091
	jb	.LBB3_3
# %bb.2:                                #   in Loop: Header=BB3_1 Depth=1
	leal	(%rdi,%rsi), %edx
	decl	%edx
	cmpl	%edx, %esi
	jle	.LBB3_1
.LBB3_5:
	xorl	%r14d, %r14d
	movl	$0, %r15d
	testl	%ecx, %ecx
	js	.LBB3_7
# %bb.6:
	movl	%ecx, %ecx
	shlq	$4, %rcx
	movl	$4240, %r15d                    # imm = 0x1090
	subl	(%rcx,%rax), %r15d
.LBB3_7:
	leaq	demo_run.code(%rip), %r12
	xorl	%ebx, %ebx
	.p2align	4
.LBB3_8:                                # =>This Inner Loop Header: Depth=1
	cmpl	$8, %r14d
	ja	.LBB3_10
# %bb.9:                                #   in Loop: Header=BB3_8 Depth=1
	movl	%r14d, %edi
	addq	%r12, %rdi
	movl	$9, %esi
	subl	%r14d, %esi
	callq	x86_insn_len
	addl	%eax, %ebx
	addl	%eax, %r14d
	testl	%eax, %eax
	jne	.LBB3_8
.LBB3_10:
	addl	%r15d, %ebx
	movl	%ebx, %eax
	popq	%rbx
	popq	%r12
	popq	%r14
	popq	%r15
	popq	%rbp
	retq
.Lfunc_end3:
	.size	demo_run, .Lfunc_end3-demo_run
                                        # -- End function
	.type	demo_run.tab,@object            # @demo_run.tab
	.section	.rodata,"a",@progbits
	.p2align	4, 0x0
demo_run.tab:
	.quad	4096                            # 0x1000
	.quad	64                              # 0x40
	.quad	4160                            # 0x1040
	.quad	64                              # 0x40
	.quad	4224                            # 0x1080
	.quad	64                              # 0x40
	.quad	4352                            # 0x1100
	.quad	32                              # 0x20
	.size	demo_run.tab, 64

	.type	demo_run.code,@object           # @demo_run.code
demo_run.code:
	.ascii	"UH\211\345H\203\354\020\303"
	.size	demo_run.code, 9

	.ident	"clang version 20.1.8"
	.section	".note.GNU-stack","",@progbits
	.addrsig
	.addrsig_sym demo_run.code
