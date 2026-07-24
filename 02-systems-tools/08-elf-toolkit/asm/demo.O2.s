	.file	"demo.c"
	.text
	.globl	sym_by_addr                     # -- Begin function sym_by_addr
	.p2align	4
	.type	sym_by_addr,@function
sym_by_addr:                            # @sym_by_addr
# %bb.0:
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
	retq
.LBB0_1:
	movl	$-1, %eax
	retq
.Lfunc_end0:
	.size	sym_by_addr, .Lfunc_end0-sym_by_addr
                                        # -- End function
	.globl	modrm_bytes                     # -- Begin function modrm_bytes
	.p2align	4
	.type	modrm_bytes,@function
modrm_bytes:                            # @modrm_bytes
# %bb.0:
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
	retq
.LBB1_1:
	movl	$0, (%rdx)
	xorl	%eax, %eax
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
	retq
.LBB1_12:
	addl	$4, %eax
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
	testl	%esi, %esi
	je	.LBB2_12
# %bb.1:
	pushq	%rbp
	pushq	%r15
	pushq	%r14
	pushq	%r13
	pushq	%r12
	pushq	%rbx
	subq	$24, %rsp
	movl	%esi, %eax
	xorl	%ebx, %ebx
	movabsq	$198158383604367617, %rcx       # imm = 0x2C0000000010101
	xorl	%r15d, %r15d
	.p2align	4
.LBB2_2:                                # =>This Inner Loop Header: Depth=1
	movzbl	(%rdi,%rbx), %edx
	leal	-46(%rdx), %r9d
	cmpl	$57, %r9d
	ja	.LBB2_5
# %bb.3:                                #   in Loop: Header=BB2_2 Depth=1
	btq	%r9, %rcx
	jb	.LBB2_8
# %bb.4:                                #   in Loop: Header=BB2_2 Depth=1
	movl	$1, %r8d
	cmpq	$56, %r9
	je	.LBB2_9
	.p2align	4
.LBB2_5:                                #   in Loop: Header=BB2_2 Depth=1
	leal	-240(%rdx), %r8d
	cmpl	$3, %r8d
	ja	.LBB2_7
# %bb.6:                                #   in Loop: Header=BB2_2 Depth=1
	cmpl	$1, %r8d
	jne	.LBB2_8
.LBB2_7:                                #   in Loop: Header=BB2_2 Depth=1
	cmpl	$38, %edx
	jne	.LBB2_13
	.p2align	4
.LBB2_8:                                #   in Loop: Header=BB2_2 Depth=1
	movl	%r15d, %r8d
.LBB2_9:                                #   in Loop: Header=BB2_2 Depth=1
	incq	%rbx
	movl	%r8d, %r15d
	cmpq	%rbx, %rax
	jne	.LBB2_2
.LBB2_10:
	xorl	%eax, %eax
.LBB2_11:
	addq	$24, %rsp
	popq	%rbx
	popq	%r12
	popq	%r13
	popq	%r14
	popq	%r15
	popq	%rbp
	retq
.LBB2_12:
	xorl	%eax, %eax
	retq
.LBB2_13:
	movzbl	(%rdi,%rbx), %r9d
	movl	%r9d, %ecx
	andb	$-16, %cl
	xorl	%r14d, %r14d
	cmpb	$64, %cl
	sete	%r14b
	leal	(%rbx,%r14), %edx
	xorl	%eax, %eax
	cmpl	%esi, %edx
	jae	.LBB2_11
# %bb.14:
	xorl	%r11d, %r11d
	cmpb	$64, %cl
	sete	%r11b
	leaq	(%r11,%rbx), %r8
	incq	%r8
	addq	%rbx, %r11
	movl	%esi, %edx
	negl	%edx
	movzbl	(%rdi,%r11), %r12d
	cmpb	$15, %r12b
	jne	.LBB2_19
# %bb.15:
	cmpl	%esi, %r8d
	jae	.LBB2_11
# %bb.16:
	xorl	%r9d, %r9d
	cmpb	$64, %cl
	sete	%r10b
	movl	%r8d, %r8d
	cmpb	$-113, (%rdi,%r8)
	jg	.LBB2_24
# %bb.17:
	xorl	%eax, %eax
	cmpb	$64, %cl
	sete	%al
	leal	(%rax,%rbx), %ecx
	addl	$6, %ecx
.LBB2_18:
	xorl	%eax, %eax
	cmpl	%esi, %ecx
	cmovbel	%ecx, %eax
	jmp	.LBB2_11
.LBB2_19:
	movl	%r12d, %r10d
	andb	$-16, %r10b
	cmpb	$80, %r10b
	je	.LBB2_20
# %bb.21:
	movzbl	%r12b, %r10d
	leal	-144(%r10), %r13d
	cmpl	$60, %r13d
	ja	.LBB2_30
# %bb.22:
	movabsq	$1299288492496388865, %rbp      # imm = 0x1208000000000301
	btq	%r13, %rbp
	jae	.LBB2_30
.LBB2_20:
	movl	%r8d, %eax
	jmp	.LBB2_11
.LBB2_24:
	movb	%r10b, %r9b
	leaq	(%rbx,%rdx), %rsi
	addq	%r9, %rsi
	xorl	%r10d, %r10d
	cmpb	$64, %cl
	sete	%r11b
	cmpl	$-2, %esi
	je	.LBB2_11
# %bb.25:
	leaq	(%r9,%rbx), %rcx
	addq	$2, %rcx
	movl	%ecx, %r8d
	movzbl	(%rdi,%r8), %r9d
	movl	%r9d, %ecx
	shrl	$6, %ecx
	movl	$1, %esi
	cmpl	$3, %ecx
	je	.LBB2_52
# %bb.26:
	movl	%r9d, %esi
	andl	$7, %esi
	cmpl	$4, %esi
	jne	.LBB2_47
# %bb.27:
	movb	%r11b, %r10b
	addq	%r10, %rdx
	addq	%rbx, %rdx
	addq	$3, %rdx
	testl	%edx, %edx
	je	.LBB2_11
# %bb.28:
	movl	$2, %esi
	cmpb	$63, %r9b
	ja	.LBB2_48
# %bb.29:
	movzbl	1(%rdi,%r8), %eax
	andb	$7, %al
	xorl	%edx, %edx
	cmpb	$5, %al
	sete	%dl
	leal	2(,%rdx,4), %esi
	jmp	.LBB2_48
.LBB2_30:
	cmpl	$244, %r10d
	je	.LBB2_20
# %bb.31:
	cmpb	$63, %r12b
	ja	.LBB2_36
# %bb.32:
	movl	%r12d, %ebp
	andb	$7, %bpl
	cmpb	$5, %bpl
	ja	.LBB2_36
# %bb.33:
	je	.LBB2_57
# %bb.34:
	movzbl	%bpl, %eax
	cmpl	$4, %eax
	jne	.LBB2_58
# %bb.35:
	xorl	%eax, %eax
	cmpb	$64, %cl
	sete	%al
	leal	(%rax,%rbx), %ecx
	addl	$2, %ecx
	jmp	.LBB2_18
.LBB2_36:
	movl	%r12d, %ebp
	andb	$-4, %bpl
	cmpb	$-120, %bpl
	je	.LBB2_41
# %bb.37:
	leal	-99(%r10), %r13d
	movq	%r13, 16(%rsp)                  # 8-byte Spill
	cmpl	$42, %r13d
	ja	.LBB2_39
# %bb.38:
	movabsq	$4423816314881, %r13            # imm = 0x40600000001
	movb	%bpl, 15(%rsp)                  # 1-byte Spill
	movq	16(%rsp), %rbp                  # 8-byte Reload
	btq	%rbp, %r13
	movzbl	15(%rsp), %ebp                  # 1-byte Folded Reload
	jb	.LBB2_41
.LBB2_39:
	cmpl	$255, %r10d
	je	.LBB2_41
# %bb.40:
	cmpb	$-48, %bpl
	jne	.LBB2_62
.LBB2_41:
	addq	%rdx, %r11
	xorl	%r10d, %r10d
	cmpb	$64, %cl
	sete	%bpl
	cmpl	$-1, %r11d
	je	.LBB2_11
# %bb.42:
	movl	%r8d, %r8d
	movzbl	(%rdi,%r8), %r9d
	movl	%r9d, %ecx
	shrl	$6, %ecx
	movl	$1, %esi
	cmpl	$3, %ecx
	je	.LBB2_61
# %bb.43:
	movl	%r9d, %esi
	andl	$7, %esi
	cmpl	$4, %esi
	jne	.LBB2_53
# %bb.44:
	movb	%bpl, %r10b
	addq	%r10, %rdx
	addq	%rbx, %rdx
	addq	$2, %rdx
	testl	%edx, %edx
	je	.LBB2_11
# %bb.45:
	movl	$2, %esi
	cmpb	$63, %r9b
	ja	.LBB2_54
# %bb.46:
	movzbl	1(%rdi,%r8), %eax
	andb	$7, %al
	xorl	%edx, %edx
	cmpb	$5, %al
	sete	%dl
	leal	2(,%rdx,4), %esi
	jmp	.LBB2_54
.LBB2_47:
	xorl	%eax, %eax
	cmpl	$5, %esi
	sete	%al
	cmpb	$64, %r9b
	leal	1(,%rax,4), %eax
	movl	$1, %esi
	cmovbl	%eax, %esi
.LBB2_48:
	cmpl	$2, %ecx
	je	.LBB2_51
# %bb.49:
	cmpl	$1, %ecx
	jne	.LBB2_52
# %bb.50:
	incl	%esi
	jmp	.LBB2_52
.LBB2_51:
	addl	$4, %esi
.LBB2_52:
	addl	%r14d, %esi
	leal	(%rbx,%rsi), %eax
	addl	$2, %eax
	jmp	.LBB2_11
.LBB2_53:
	xorl	%eax, %eax
	cmpl	$5, %esi
	sete	%al
	cmpb	$64, %r9b
	leal	1(,%rax,4), %eax
	movl	$1, %esi
	cmovbl	%eax, %esi
.LBB2_54:
	cmpl	$2, %ecx
	je	.LBB2_60
# %bb.55:
	cmpl	$1, %ecx
	jne	.LBB2_61
# %bb.56:
	incl	%esi
	jmp	.LBB2_61
.LBB2_57:
	xorl	%eax, %eax
	testl	%r15d, %r15d
	sete	%al
	leal	(%r14,%rax,2), %eax
	leal	(%rbx,%rax), %ecx
	addl	$3, %ecx
	jmp	.LBB2_18
.LBB2_58:
	xorl	%eax, %eax
	cmpb	$64, %cl
	sete	%al
	movl	$1, 8(%rsp)
	movl	%r8d, %ecx
	addq	%rcx, %rdi
	notl	%eax
	addl	%esi, %eax
	subl	%ebx, %eax
	leaq	8(%rsp), %rdx
	movl	%eax, %esi
	callq	modrm_bytes
                                        # kill: def $eax killed $eax def $rax
	movl	8(%rsp), %ecx
	addl	%r14d, %eax
	testl	%ecx, %ecx
	leal	1(%rbx,%rax), %eax
.LBB2_59:
	cmovel	%ecx, %eax
	jmp	.LBB2_11
.LBB2_60:
	addl	$4, %esi
.LBB2_61:
	addl	%r14d, %esi
	leal	(%rbx,%rsi), %eax
	incl	%eax
	jmp	.LBB2_11
.LBB2_62:
	movq	%rdi, %r13
	movl	%esi, %ebp
	cmpl	$191, %r10d
	jg	.LBB2_70
# %bb.63:
	leal	-105(%r10), %esi
	cmpl	$38, %esi
	ja	.LBB2_73
# %bb.64:
	movl	$75497476, %edi                 # imm = 0x4800004
	btq	%rsi, %rdi
	jb	.LBB2_76
# %bb.65:
	movabsq	$276488519680, %rdi             # imm = 0x4060000000
	btq	%rsi, %rdi
	movq	%r13, %rdi
	jb	.LBB2_41
# %bb.66:
	movl	$16777217, %edx                 # imm = 0x1000001
	btq	%rsi, %rdx
	jae	.LBB2_73
.LBB2_67:
	xorl	%esi, %esi
	cmpb	$64, %cl
	sete	%sil
	movl	$1, 8(%rsp)
	movl	%r8d, %eax
	movq	%r13, %rdi
	addq	%rax, %rdi
	notl	%esi
	addl	%ebp, %esi
	subl	%ebx, %esi
	leaq	8(%rsp), %rdx
	callq	modrm_bytes
                                        # kill: def $eax killed $eax def $rax
	cmpl	$0, 8(%rsp)
	je	.LBB2_10
# %bb.68:
	xorl	%ecx, %ecx
	testl	%r15d, %r15d
	sete	%cl
	leal	(%rax,%rcx,2), %eax
	addl	%r14d, %eax
	leal	(%rbx,%rax), %ecx
	addl	$3, %ecx
.LBB2_69:
	xorl	%eax, %eax
	cmpl	%ebp, %ecx
	cmovbel	%ecx, %eax
	jmp	.LBB2_11
.LBB2_70:
	leal	-192(%r10), %edx
	cmpl	$2, %edx
	jb	.LBB2_76
# %bb.71:
	cmpl	$198, %r10d
	je	.LBB2_76
# %bb.72:
	cmpl	$199, %r10d
	je	.LBB2_67
.LBB2_73:
	movl	%r12d, %edx
	andb	$-2, %dl
	cmpb	$-10, %dl
	jne	.LBB2_78
# %bb.74:
	xorl	%esi, %esi
	cmpb	$64, %cl
	sete	%sil
	movl	$1, 8(%rsp)
	movl	%r8d, %eax
	movq	%r13, %rdi
	addq	%rax, %rdi
	notl	%esi
	addl	%ebp, %esi
	subl	%ebx, %esi
	leaq	8(%rsp), %rdx
	movq	%rdi, %r13
	callq	modrm_bytes
                                        # kill: def $eax killed $eax def $rax
	cmpl	$0, 8(%rsp)
	je	.LBB2_10
# %bb.75:
	xorl	%ecx, %ecx
	testl	%r15d, %r15d
	sete	%cl
	xorl	%edx, %edx
	testb	$48, (%r13)
	sete	%dl
	leal	2(%rcx,%rcx), %ecx
	cmovnel	%edx, %ecx
	cmpb	$-10, %r12b
	cmovel	%edx, %ecx
	addl	%r14d, %eax
	addl	%ecx, %eax
	leal	(%rbx,%rax), %ecx
	incl	%ecx
	jmp	.LBB2_69
.LBB2_76:
	xorl	%esi, %esi
	cmpb	$64, %cl
	sete	%sil
	movl	$1, 8(%rsp)
	movl	%r8d, %eax
	movq	%r13, %rdi
	addq	%rax, %rdi
	notl	%esi
	addl	%ebp, %esi
	subl	%ebx, %esi
	leaq	8(%rsp), %rdx
	callq	modrm_bytes
                                        # kill: def $eax killed $eax def $rax
	addl	%r14d, %eax
	leal	(%rbx,%rax), %ecx
	addl	$2, %ecx
	xorl	%eax, %eax
	cmpl	%ebp, %ecx
	cmovbel	%ecx, %eax
	movl	8(%rsp), %ecx
	testl	%ecx, %ecx
	jmp	.LBB2_59
.LBB2_78:
	cmpb	$112, %r12b
	jge	.LBB2_80
# %bb.79:
	cmpb	$-21, %r12b
	jne	.LBB2_82
.LBB2_80:
	xorl	%eax, %eax
	cmpb	$64, %cl
	sete	%al
	leal	(%rax,%rbx), %ecx
	addl	$2, %ecx
	jmp	.LBB2_69
.LBB2_82:
	cmpb	$-24, %dl
	jne	.LBB2_84
.LBB2_83:
	xorl	%eax, %eax
	cmpb	$64, %cl
	sete	%al
	leal	(%rax,%rbx), %ecx
	addl	$5, %ecx
	jmp	.LBB2_69
.LBB2_84:
	movl	%r12d, %edx
	andb	$-8, %dl
	cmpb	$-72, %dl
	je	.LBB2_88
# %bb.85:
	movzbl	%dl, %edx
	cmpl	$176, %edx
	je	.LBB2_80
# %bb.86:
	cmpb	$106, %r12b
	je	.LBB2_80
# %bb.87:
	cmpl	$104, %r10d
	je	.LBB2_83
	jmp	.LBB2_11
.LBB2_88:
	andb	$-8, %r9b
	xorl	%eax, %eax
	testl	%r15d, %r15d
	sete	%al
	cmpb	$72, %r9b
	leal	2(%rax,%rax), %eax
	movl	$8, %ecx
	cmovnel	%eax, %ecx
	orl	%r14d, %ecx
	addl	%ebx, %ecx
	incl	%ecx
	jmp	.LBB2_69
.Lfunc_end2:
	.size	x86_insn_len, .Lfunc_end2-x86_insn_len
                                        # -- End function
	.globl	demo_run                        # -- Begin function demo_run
	.p2align	4
	.type	demo_run,@function
demo_run:                               # @demo_run
# %bb.0:
	pushq	%rbp
	pushq	%r15
	pushq	%r14
	pushq	%rbx
	pushq	%rax
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
	xorl	%ebp, %ebp
	movl	$0, %r14d
	testl	%ecx, %ecx
	js	.LBB3_7
# %bb.6:
	movl	%ecx, %ecx
	shlq	$4, %rcx
	movl	$4240, %r14d                    # imm = 0x1090
	subl	(%rcx,%rax), %r14d
.LBB3_7:
	leaq	demo_run.code(%rip), %r15
	xorl	%ebx, %ebx
	.p2align	4
.LBB3_8:                                # =>This Inner Loop Header: Depth=1
	cmpl	$8, %ebp
	ja	.LBB3_10
# %bb.9:                                #   in Loop: Header=BB3_8 Depth=1
	movl	%ebp, %edi
	addq	%r15, %rdi
	movl	$9, %esi
	subl	%ebp, %esi
	callq	x86_insn_len
	addl	%eax, %ebx
	addl	%eax, %ebp
	testl	%eax, %eax
	jne	.LBB3_8
.LBB3_10:
	addl	%r14d, %ebx
	movl	%ebx, %eax
	addq	$8, %rsp
	popq	%rbx
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
