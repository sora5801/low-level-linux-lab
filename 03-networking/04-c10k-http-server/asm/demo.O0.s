	.file	"demo.c"
	.text
	.globl	hp_parse                        # -- Begin function hp_parse
	.p2align	4
	.type	hp_parse,@function
hp_parse:                               # @hp_parse
# %bb.0:
	pushq	%rbp
	movq	%rsp, %rbp
	subq	$48, %rsp
	movq	%rdi, -16(%rbp)
	movq	%rsi, -24(%rbp)
	movq	%rdx, -32(%rbp)
	movq	-16(%rbp), %rax
	movq	8(%rax), %rax
	movq	%rax, -40(%rbp)
.LBB0_1:                                # =>This Inner Loop Header: Depth=1
	movq	-40(%rbp), %rax
	cmpq	-32(%rbp), %rax
	jae	.LBB0_87
# %bb.2:                                #   in Loop: Header=BB0_1 Depth=1
	movq	-24(%rbp), %rax
	movq	-40(%rbp), %rcx
	movb	(%rax,%rcx), %al
	movb	%al, -41(%rbp)
	movq	-16(%rbp), %rcx
	movl	56(%rcx), %eax
	addl	$1, %eax
	movl	%eax, 56(%rcx)
	cmpl	$8192, %eax                     # imm = 0x2000
	jbe	.LBB0_4
# %bb.3:
	movq	-16(%rbp), %rax
	movl	$13, (%rax)
	jmp	.LBB0_87
.LBB0_4:                                #   in Loop: Header=BB0_1 Depth=1
	movq	-16(%rbp), %rax
	movl	(%rax), %eax
	movl	%eax, -48(%rbp)                 # 4-byte Spill
	testl	%eax, %eax
	je	.LBB0_5
	jmp	.LBB0_94
.LBB0_94:                               #   in Loop: Header=BB0_1 Depth=1
	movl	-48(%rbp), %eax                 # 4-byte Reload
	subl	$1, %eax
	je	.LBB0_18
	jmp	.LBB0_95
.LBB0_95:                               #   in Loop: Header=BB0_1 Depth=1
	movl	-48(%rbp), %eax                 # 4-byte Reload
	subl	$2, %eax
	je	.LBB0_30
	jmp	.LBB0_96
.LBB0_96:                               #   in Loop: Header=BB0_1 Depth=1
	movl	-48(%rbp), %eax                 # 4-byte Reload
	subl	$3, %eax
	je	.LBB0_35
	jmp	.LBB0_97
.LBB0_97:                               #   in Loop: Header=BB0_1 Depth=1
	movl	-48(%rbp), %eax                 # 4-byte Reload
	subl	$4, %eax
	je	.LBB0_39
	jmp	.LBB0_98
.LBB0_98:                               #   in Loop: Header=BB0_1 Depth=1
	movl	-48(%rbp), %eax                 # 4-byte Reload
	subl	$5, %eax
	je	.LBB0_42
	jmp	.LBB0_99
.LBB0_99:                               #   in Loop: Header=BB0_1 Depth=1
	movl	-48(%rbp), %eax                 # 4-byte Reload
	subl	$6, %eax
	je	.LBB0_45
	jmp	.LBB0_100
.LBB0_100:                              #   in Loop: Header=BB0_1 Depth=1
	movl	-48(%rbp), %eax                 # 4-byte Reload
	subl	$7, %eax
	je	.LBB0_54
	jmp	.LBB0_101
.LBB0_101:                              #   in Loop: Header=BB0_1 Depth=1
	movl	-48(%rbp), %eax                 # 4-byte Reload
	subl	$8, %eax
	je	.LBB0_60
	jmp	.LBB0_102
.LBB0_102:                              #   in Loop: Header=BB0_1 Depth=1
	movl	-48(%rbp), %eax                 # 4-byte Reload
	subl	$9, %eax
	je	.LBB0_72
	jmp	.LBB0_103
.LBB0_103:                              #   in Loop: Header=BB0_1 Depth=1
	movl	-48(%rbp), %eax                 # 4-byte Reload
	subl	$10, %eax
	je	.LBB0_78
	jmp	.LBB0_104
.LBB0_104:
	movl	-48(%rbp), %eax                 # 4-byte Reload
	subl	$11, %eax
	je	.LBB0_81
	jmp	.LBB0_84
.LBB0_5:                                #   in Loop: Header=BB0_1 Depth=1
	movzbl	-41(%rbp), %eax
	cmpl	$32, %eax
	jne	.LBB0_9
# %bb.6:                                #   in Loop: Header=BB0_1 Depth=1
	movq	-16(%rbp), %rax
	cmpq	$0, 24(%rax)
	jne	.LBB0_8
# %bb.7:
	movq	-16(%rbp), %rax
	movl	$13, (%rax)
	jmp	.LBB0_88
.LBB0_8:                                #   in Loop: Header=BB0_1 Depth=1
	movq	-40(%rbp), %rcx
	addq	$1, %rcx
	movq	-16(%rbp), %rax
	movq	%rcx, 32(%rax)
	movq	-16(%rbp), %rax
	movl	$1, (%rax)
	jmp	.LBB0_17
.LBB0_9:                                #   in Loop: Header=BB0_1 Depth=1
	movzbl	-41(%rbp), %edi
	callq	is_tchar
	cmpl	$0, %eax
	je	.LBB0_15
# %bb.10:                               #   in Loop: Header=BB0_1 Depth=1
	movq	-16(%rbp), %rax
	cmpq	$0, 24(%rax)
	jne	.LBB0_12
# %bb.11:                               #   in Loop: Header=BB0_1 Depth=1
	movq	-40(%rbp), %rcx
	movq	-16(%rbp), %rax
	movq	%rcx, 16(%rax)
.LBB0_12:                               #   in Loop: Header=BB0_1 Depth=1
	movq	-16(%rbp), %rcx
	movq	24(%rcx), %rax
	addq	$1, %rax
	movq	%rax, 24(%rcx)
	cmpq	$16, %rax
	jbe	.LBB0_14
# %bb.13:
	movq	-16(%rbp), %rax
	movl	$13, (%rax)
	jmp	.LBB0_88
.LBB0_14:                               #   in Loop: Header=BB0_1 Depth=1
	jmp	.LBB0_16
.LBB0_15:
	movq	-16(%rbp), %rax
	movl	$13, (%rax)
	jmp	.LBB0_88
.LBB0_16:                               #   in Loop: Header=BB0_1 Depth=1
	jmp	.LBB0_17
.LBB0_17:                               #   in Loop: Header=BB0_1 Depth=1
	jmp	.LBB0_85
.LBB0_18:                               #   in Loop: Header=BB0_1 Depth=1
	movzbl	-41(%rbp), %eax
	cmpl	$32, %eax
	jne	.LBB0_22
# %bb.19:                               #   in Loop: Header=BB0_1 Depth=1
	movq	-16(%rbp), %rax
	cmpq	$0, 40(%rax)
	jne	.LBB0_21
# %bb.20:
	movq	-16(%rbp), %rax
	movl	$13, (%rax)
	jmp	.LBB0_88
.LBB0_21:                               #   in Loop: Header=BB0_1 Depth=1
	movq	-16(%rbp), %rax
	movb	$0, 60(%rax)
	movq	-16(%rbp), %rax
	movl	$2, (%rax)
	jmp	.LBB0_29
.LBB0_22:                               #   in Loop: Header=BB0_1 Depth=1
	movzbl	-41(%rbp), %eax
	cmpl	$32, %eax
	jle	.LBB0_24
# %bb.23:                               #   in Loop: Header=BB0_1 Depth=1
	movzbl	-41(%rbp), %eax
	cmpl	$127, %eax
	jne	.LBB0_25
.LBB0_24:
	movq	-16(%rbp), %rax
	movl	$13, (%rax)
	jmp	.LBB0_88
.LBB0_25:                               #   in Loop: Header=BB0_1 Depth=1
	movq	-16(%rbp), %rax
	cmpq	$0, 40(%rax)
	jne	.LBB0_27
# %bb.26:                               #   in Loop: Header=BB0_1 Depth=1
	movq	-40(%rbp), %rcx
	movq	-16(%rbp), %rax
	movq	%rcx, 32(%rax)
.LBB0_27:                               #   in Loop: Header=BB0_1 Depth=1
	movq	-16(%rbp), %rax
	movq	40(%rax), %rcx
	addq	$1, %rcx
	movq	%rcx, 40(%rax)
# %bb.28:                               #   in Loop: Header=BB0_1 Depth=1
	jmp	.LBB0_29
.LBB0_29:                               #   in Loop: Header=BB0_1 Depth=1
	jmp	.LBB0_85
.LBB0_30:                               #   in Loop: Header=BB0_1 Depth=1
	movzbl	-41(%rbp), %eax
	movq	-16(%rbp), %rcx
	movzbl	60(%rcx), %ecx
	movl	%ecx, %edx
	leaq	hp_parse.V(%rip), %rcx
	movzbl	(%rcx,%rdx), %ecx
	cmpl	%ecx, %eax
	je	.LBB0_32
# %bb.31:
	movq	-16(%rbp), %rax
	movl	$13, (%rax)
	jmp	.LBB0_88
.LBB0_32:                               #   in Loop: Header=BB0_1 Depth=1
	movq	-16(%rbp), %rcx
	movb	60(%rcx), %al
	addb	$1, %al
	movb	%al, 60(%rcx)
	movzbl	%al, %eax
	cmpl	$7, %eax
	jne	.LBB0_34
# %bb.33:                               #   in Loop: Header=BB0_1 Depth=1
	movq	-16(%rbp), %rax
	movl	$3, (%rax)
.LBB0_34:                               #   in Loop: Header=BB0_1 Depth=1
	jmp	.LBB0_85
.LBB0_35:                               #   in Loop: Header=BB0_1 Depth=1
	movzbl	-41(%rbp), %eax
	cmpl	$48, %eax
	jl	.LBB0_37
# %bb.36:                               #   in Loop: Header=BB0_1 Depth=1
	movzbl	-41(%rbp), %eax
	cmpl	$57, %eax
	jle	.LBB0_38
.LBB0_37:
	movq	-16(%rbp), %rax
	movl	$13, (%rax)
	jmp	.LBB0_88
.LBB0_38:                               #   in Loop: Header=BB0_1 Depth=1
	movzbl	-41(%rbp), %ecx
	subl	$48, %ecx
	movq	-16(%rbp), %rax
	movl	%ecx, 48(%rax)
	movq	-16(%rbp), %rax
	movl	$4, (%rax)
	jmp	.LBB0_85
.LBB0_39:                               #   in Loop: Header=BB0_1 Depth=1
	movzbl	-41(%rbp), %eax
	cmpl	$13, %eax
	je	.LBB0_41
# %bb.40:
	movq	-16(%rbp), %rax
	movl	$13, (%rax)
	jmp	.LBB0_88
.LBB0_41:                               #   in Loop: Header=BB0_1 Depth=1
	movq	-16(%rbp), %rax
	movl	$5, (%rax)
	jmp	.LBB0_85
.LBB0_42:                               #   in Loop: Header=BB0_1 Depth=1
	movzbl	-41(%rbp), %eax
	cmpl	$10, %eax
	je	.LBB0_44
# %bb.43:
	movq	-16(%rbp), %rax
	movl	$13, (%rax)
	jmp	.LBB0_88
.LBB0_44:                               #   in Loop: Header=BB0_1 Depth=1
	movq	-16(%rbp), %rax
	movl	$6, (%rax)
	jmp	.LBB0_85
.LBB0_45:                               #   in Loop: Header=BB0_1 Depth=1
	movzbl	-41(%rbp), %eax
	cmpl	$13, %eax
	jne	.LBB0_47
# %bb.46:                               #   in Loop: Header=BB0_1 Depth=1
	movq	-16(%rbp), %rax
	movl	$11, (%rax)
	jmp	.LBB0_53
.LBB0_47:                               #   in Loop: Header=BB0_1 Depth=1
	movzbl	-41(%rbp), %edi
	callq	is_tchar
	cmpl	$0, %eax
	je	.LBB0_51
# %bb.48:                               #   in Loop: Header=BB0_1 Depth=1
	movq	-16(%rbp), %rax
	cmpl	$64, 52(%rax)
	jb	.LBB0_50
# %bb.49:
	movq	-16(%rbp), %rax
	movl	$13, (%rax)
	jmp	.LBB0_88
.LBB0_50:                               #   in Loop: Header=BB0_1 Depth=1
	movq	-16(%rbp), %rax
	movl	$7, (%rax)
	jmp	.LBB0_52
.LBB0_51:
	movq	-16(%rbp), %rax
	movl	$13, (%rax)
	jmp	.LBB0_88
.LBB0_52:                               #   in Loop: Header=BB0_1 Depth=1
	jmp	.LBB0_53
.LBB0_53:                               #   in Loop: Header=BB0_1 Depth=1
	jmp	.LBB0_85
.LBB0_54:                               #   in Loop: Header=BB0_1 Depth=1
	movzbl	-41(%rbp), %eax
	cmpl	$58, %eax
	jne	.LBB0_56
# %bb.55:                               #   in Loop: Header=BB0_1 Depth=1
	movq	-16(%rbp), %rax
	movl	$8, (%rax)
	jmp	.LBB0_59
.LBB0_56:                               #   in Loop: Header=BB0_1 Depth=1
	movzbl	-41(%rbp), %edi
	callq	is_tchar
	cmpl	$0, %eax
	jne	.LBB0_58
# %bb.57:
	movq	-16(%rbp), %rax
	movl	$13, (%rax)
	jmp	.LBB0_88
.LBB0_58:                               #   in Loop: Header=BB0_1 Depth=1
	jmp	.LBB0_59
.LBB0_59:                               #   in Loop: Header=BB0_1 Depth=1
	jmp	.LBB0_85
.LBB0_60:                               #   in Loop: Header=BB0_1 Depth=1
	movzbl	-41(%rbp), %eax
	cmpl	$32, %eax
	je	.LBB0_62
# %bb.61:                               #   in Loop: Header=BB0_1 Depth=1
	movzbl	-41(%rbp), %eax
	cmpl	$9, %eax
	jne	.LBB0_63
.LBB0_62:                               #   in Loop: Header=BB0_1 Depth=1
	jmp	.LBB0_71
.LBB0_63:                               #   in Loop: Header=BB0_1 Depth=1
	movzbl	-41(%rbp), %eax
	cmpl	$13, %eax
	jne	.LBB0_65
# %bb.64:                               #   in Loop: Header=BB0_1 Depth=1
	movq	-16(%rbp), %rax
	movl	52(%rax), %ecx
	addl	$1, %ecx
	movl	%ecx, 52(%rax)
	movq	-16(%rbp), %rax
	movl	$10, (%rax)
	jmp	.LBB0_70
.LBB0_65:                               #   in Loop: Header=BB0_1 Depth=1
	movzbl	-41(%rbp), %eax
	cmpl	$10, %eax
	je	.LBB0_67
# %bb.66:                               #   in Loop: Header=BB0_1 Depth=1
	movzbl	-41(%rbp), %eax
	cmpl	$127, %eax
	jne	.LBB0_68
.LBB0_67:
	movq	-16(%rbp), %rax
	movl	$13, (%rax)
	jmp	.LBB0_88
.LBB0_68:                               #   in Loop: Header=BB0_1 Depth=1
	movq	-16(%rbp), %rax
	movl	$9, (%rax)
# %bb.69:                               #   in Loop: Header=BB0_1 Depth=1
	jmp	.LBB0_70
.LBB0_70:                               #   in Loop: Header=BB0_1 Depth=1
	jmp	.LBB0_71
.LBB0_71:                               #   in Loop: Header=BB0_1 Depth=1
	jmp	.LBB0_85
.LBB0_72:                               #   in Loop: Header=BB0_1 Depth=1
	movzbl	-41(%rbp), %eax
	cmpl	$13, %eax
	jne	.LBB0_74
# %bb.73:                               #   in Loop: Header=BB0_1 Depth=1
	movq	-16(%rbp), %rax
	movl	52(%rax), %ecx
	addl	$1, %ecx
	movl	%ecx, 52(%rax)
	movq	-16(%rbp), %rax
	movl	$10, (%rax)
	jmp	.LBB0_77
.LBB0_74:                               #   in Loop: Header=BB0_1 Depth=1
	movzbl	-41(%rbp), %eax
	cmpl	$10, %eax
	jne	.LBB0_76
# %bb.75:
	movq	-16(%rbp), %rax
	movl	$13, (%rax)
	jmp	.LBB0_88
.LBB0_76:                               #   in Loop: Header=BB0_1 Depth=1
	jmp	.LBB0_77
.LBB0_77:                               #   in Loop: Header=BB0_1 Depth=1
	jmp	.LBB0_85
.LBB0_78:                               #   in Loop: Header=BB0_1 Depth=1
	movzbl	-41(%rbp), %eax
	cmpl	$10, %eax
	je	.LBB0_80
# %bb.79:
	movq	-16(%rbp), %rax
	movl	$13, (%rax)
	jmp	.LBB0_88
.LBB0_80:                               #   in Loop: Header=BB0_1 Depth=1
	movq	-16(%rbp), %rax
	movl	$6, (%rax)
	jmp	.LBB0_85
.LBB0_81:
	movzbl	-41(%rbp), %eax
	cmpl	$10, %eax
	je	.LBB0_83
# %bb.82:
	movq	-16(%rbp), %rax
	movl	$13, (%rax)
	jmp	.LBB0_88
.LBB0_83:
	movq	-16(%rbp), %rax
	movl	$12, (%rax)
	movq	-40(%rbp), %rax
	addq	$1, %rax
	movq	%rax, -40(%rbp)
	jmp	.LBB0_88
.LBB0_84:
	movq	-16(%rbp), %rax
	movl	$13, (%rax)
	jmp	.LBB0_88
.LBB0_85:                               #   in Loop: Header=BB0_1 Depth=1
	jmp	.LBB0_86
.LBB0_86:                               #   in Loop: Header=BB0_1 Depth=1
	movq	-40(%rbp), %rax
	addq	$1, %rax
	movq	%rax, -40(%rbp)
	jmp	.LBB0_1
.LBB0_87:
	jmp	.LBB0_88
.LBB0_88:
	movq	-40(%rbp), %rcx
	movq	-16(%rbp), %rax
	movq	%rcx, 8(%rax)
	movq	-16(%rbp), %rax
	cmpl	$13, (%rax)
	jne	.LBB0_90
# %bb.89:
	movl	$-1, -4(%rbp)
	jmp	.LBB0_93
.LBB0_90:
	movq	-16(%rbp), %rax
	cmpl	$12, (%rax)
	jne	.LBB0_92
# %bb.91:
	movl	$1, -4(%rbp)
	jmp	.LBB0_93
.LBB0_92:
	movl	$0, -4(%rbp)
.LBB0_93:
	movl	-4(%rbp), %eax
	addq	$48, %rsp
	popq	%rbp
	retq
.Lfunc_end0:
	.size	hp_parse, .Lfunc_end0-hp_parse
                                        # -- End function
	.p2align	4                               # -- Begin function is_tchar
	.type	is_tchar,@function
is_tchar:                               # @is_tchar
# %bb.0:
	pushq	%rbp
	movq	%rsp, %rbp
	movb	%dil, %al
	movb	%al, -5(%rbp)
	movzbl	-5(%rbp), %eax
	cmpl	$97, %eax
	jl	.LBB1_2
# %bb.1:
	movzbl	-5(%rbp), %eax
	cmpl	$122, %eax
	jle	.LBB1_6
.LBB1_2:
	movzbl	-5(%rbp), %eax
	cmpl	$65, %eax
	jl	.LBB1_4
# %bb.3:
	movzbl	-5(%rbp), %eax
	cmpl	$90, %eax
	jle	.LBB1_6
.LBB1_4:
	movzbl	-5(%rbp), %eax
	cmpl	$48, %eax
	jl	.LBB1_7
# %bb.5:
	movzbl	-5(%rbp), %eax
	cmpl	$57, %eax
	jg	.LBB1_7
.LBB1_6:
	movl	$1, -4(%rbp)
	jmp	.LBB1_10
.LBB1_7:
	movzbl	-5(%rbp), %eax
	movl	%eax, -12(%rbp)                 # 4-byte Spill
	subl	$33, %eax
	je	.LBB1_8
	jmp	.LBB1_11
.LBB1_11:
	movl	-12(%rbp), %eax                 # 4-byte Reload
	addl	$-35, %eax
	subl	$5, %eax
	jb	.LBB1_8
	jmp	.LBB1_12
.LBB1_12:
	movl	-12(%rbp), %eax                 # 4-byte Reload
	addl	$-42, %eax
	subl	$2, %eax
	jb	.LBB1_8
	jmp	.LBB1_13
.LBB1_13:
	movl	-12(%rbp), %eax                 # 4-byte Reload
	addl	$-45, %eax
	subl	$2, %eax
	jb	.LBB1_8
	jmp	.LBB1_14
.LBB1_14:
	movl	-12(%rbp), %eax                 # 4-byte Reload
	addl	$-94, %eax
	subl	$3, %eax
	jb	.LBB1_8
	jmp	.LBB1_15
.LBB1_15:
	movl	-12(%rbp), %eax                 # 4-byte Reload
	subl	$124, %eax
	je	.LBB1_8
	jmp	.LBB1_16
.LBB1_16:
	movl	-12(%rbp), %eax                 # 4-byte Reload
	subl	$126, %eax
	jne	.LBB1_9
	jmp	.LBB1_8
.LBB1_8:
	movl	$1, -4(%rbp)
	jmp	.LBB1_10
.LBB1_9:
	movl	$0, -4(%rbp)
.LBB1_10:
	movl	-4(%rbp), %eax
	popq	%rbp
	retq
.Lfunc_end1:
	.size	is_tchar, .Lfunc_end1-is_tchar
                                        # -- End function
	.type	hp_parse.V,@object              # @hp_parse.V
	.section	.rodata,"a",@progbits
hp_parse.V:
	.asciz	"HTTP/1."
	.size	hp_parse.V, 8

	.ident	"clang version 20.1.8"
	.section	".note.GNU-stack","",@progbits
	.addrsig
	.addrsig_sym is_tchar
	.addrsig_sym hp_parse.V
