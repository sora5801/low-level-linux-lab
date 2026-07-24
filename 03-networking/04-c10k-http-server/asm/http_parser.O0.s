	.file	"http_parser.c"
	.text
	.globl	hp_slice_ci_eq                  # -- Begin function hp_slice_ci_eq
	.p2align	4
	.type	hp_slice_ci_eq,@function
hp_slice_ci_eq:                         # @hp_slice_ci_eq
# %bb.0:
	pushq	%rbp
	movq	%rsp, %rbp
	subq	$64, %rsp
	movq	%rsi, -24(%rbp)
	movq	%rdx, -16(%rbp)
	movq	%rdi, -32(%rbp)
	movq	%rcx, -40(%rbp)
	movq	$0, -48(%rbp)
.LBB0_1:                                # =>This Inner Loop Header: Depth=1
	movq	-48(%rbp), %rax
	cmpq	-16(%rbp), %rax
	jae	.LBB0_8
# %bb.2:                                #   in Loop: Header=BB0_1 Depth=1
	movq	-32(%rbp), %rax
	movq	-24(%rbp), %rcx
	addq	-48(%rbp), %rcx
	movzbl	(%rax,%rcx), %edi
	callq	lc
	movb	%al, -49(%rbp)
	movq	-40(%rbp), %rax
	movq	-48(%rbp), %rcx
	movzbl	(%rax,%rcx), %edi
	callq	lc
	movb	%al, -50(%rbp)
	movzbl	-50(%rbp), %eax
	cmpl	$0, %eax
	jne	.LBB0_4
# %bb.3:
	movl	$0, -4(%rbp)
	jmp	.LBB0_9
.LBB0_4:                                #   in Loop: Header=BB0_1 Depth=1
	movzbl	-49(%rbp), %eax
	movzbl	-50(%rbp), %ecx
	cmpl	%ecx, %eax
	je	.LBB0_6
# %bb.5:
	movl	$0, -4(%rbp)
	jmp	.LBB0_9
.LBB0_6:                                #   in Loop: Header=BB0_1 Depth=1
	jmp	.LBB0_7
.LBB0_7:                                #   in Loop: Header=BB0_1 Depth=1
	movq	-48(%rbp), %rax
	addq	$1, %rax
	movq	%rax, -48(%rbp)
	jmp	.LBB0_1
.LBB0_8:
	movq	-40(%rbp), %rax
	movq	-16(%rbp), %rcx
	movsbl	(%rax,%rcx), %eax
	cmpl	$0, %eax
	sete	%al
	andb	$1, %al
	movzbl	%al, %eax
	movl	%eax, -4(%rbp)
.LBB0_9:
	movl	-4(%rbp), %eax
	addq	$64, %rsp
	popq	%rbp
	retq
.Lfunc_end0:
	.size	hp_slice_ci_eq, .Lfunc_end0-hp_slice_ci_eq
                                        # -- End function
	.p2align	4                               # -- Begin function lc
	.type	lc,@function
lc:                                     # @lc
# %bb.0:
	pushq	%rbp
	movq	%rsp, %rbp
	movb	%dil, %al
	movb	%al, -1(%rbp)
	movzbl	-1(%rbp), %eax
	cmpl	$65, %eax
	jl	.LBB1_3
# %bb.1:
	movzbl	-1(%rbp), %eax
	cmpl	$90, %eax
	jg	.LBB1_3
# %bb.2:
	movzbl	-1(%rbp), %eax
	addl	$32, %eax
                                        # kill: def $al killed $al killed $eax
	movzbl	%al, %eax
	movl	%eax, -8(%rbp)                  # 4-byte Spill
	jmp	.LBB1_4
.LBB1_3:
	movzbl	-1(%rbp), %eax
	movl	%eax, -8(%rbp)                  # 4-byte Spill
.LBB1_4:
	movl	-8(%rbp), %eax                  # 4-byte Reload
                                        # kill: def $al killed $al killed $eax
	popq	%rbp
	retq
.Lfunc_end1:
	.size	lc, .Lfunc_end1-lc
                                        # -- End function
	.globl	hp_execute                      # -- Begin function hp_execute
	.p2align	4
	.type	hp_execute,@function
hp_execute:                             # @hp_execute
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
.LBB2_1:                                # =>This Inner Loop Header: Depth=1
	movq	-40(%rbp), %rax
	cmpq	-32(%rbp), %rax
	jae	.LBB2_89
# %bb.2:                                #   in Loop: Header=BB2_1 Depth=1
	movq	-24(%rbp), %rax
	movq	-40(%rbp), %rcx
	movb	(%rax,%rcx), %al
	movb	%al, -41(%rbp)
	movq	-16(%rbp), %rcx
	movl	2152(%rcx), %eax
	addl	$1, %eax
	movl	%eax, 2152(%rcx)
	cmpl	$8192, %eax                     # imm = 0x2000
	jbe	.LBB2_4
# %bb.3:
	movq	-16(%rbp), %rax
	movl	$13, (%rax)
	jmp	.LBB2_89
.LBB2_4:                                #   in Loop: Header=BB2_1 Depth=1
	movq	-16(%rbp), %rax
	movl	(%rax), %eax
	movl	%eax, -48(%rbp)                 # 4-byte Spill
	testl	%eax, %eax
	je	.LBB2_5
	jmp	.LBB2_96
.LBB2_96:                               #   in Loop: Header=BB2_1 Depth=1
	movl	-48(%rbp), %eax                 # 4-byte Reload
	subl	$1, %eax
	je	.LBB2_18
	jmp	.LBB2_97
.LBB2_97:                               #   in Loop: Header=BB2_1 Depth=1
	movl	-48(%rbp), %eax                 # 4-byte Reload
	subl	$2, %eax
	je	.LBB2_30
	jmp	.LBB2_98
.LBB2_98:                               #   in Loop: Header=BB2_1 Depth=1
	movl	-48(%rbp), %eax                 # 4-byte Reload
	subl	$3, %eax
	je	.LBB2_35
	jmp	.LBB2_99
.LBB2_99:                               #   in Loop: Header=BB2_1 Depth=1
	movl	-48(%rbp), %eax                 # 4-byte Reload
	subl	$4, %eax
	je	.LBB2_39
	jmp	.LBB2_100
.LBB2_100:                              #   in Loop: Header=BB2_1 Depth=1
	movl	-48(%rbp), %eax                 # 4-byte Reload
	subl	$5, %eax
	je	.LBB2_42
	jmp	.LBB2_101
.LBB2_101:                              #   in Loop: Header=BB2_1 Depth=1
	movl	-48(%rbp), %eax                 # 4-byte Reload
	subl	$6, %eax
	je	.LBB2_45
	jmp	.LBB2_102
.LBB2_102:                              #   in Loop: Header=BB2_1 Depth=1
	movl	-48(%rbp), %eax                 # 4-byte Reload
	subl	$7, %eax
	je	.LBB2_52
	jmp	.LBB2_103
.LBB2_103:                              #   in Loop: Header=BB2_1 Depth=1
	movl	-48(%rbp), %eax                 # 4-byte Reload
	subl	$8, %eax
	je	.LBB2_59
	jmp	.LBB2_104
.LBB2_104:                              #   in Loop: Header=BB2_1 Depth=1
	movl	-48(%rbp), %eax                 # 4-byte Reload
	subl	$9, %eax
	je	.LBB2_71
	jmp	.LBB2_105
.LBB2_105:                              #   in Loop: Header=BB2_1 Depth=1
	movl	-48(%rbp), %eax                 # 4-byte Reload
	subl	$10, %eax
	je	.LBB2_78
	jmp	.LBB2_106
.LBB2_106:
	movl	-48(%rbp), %eax                 # 4-byte Reload
	subl	$11, %eax
	je	.LBB2_83
	jmp	.LBB2_86
.LBB2_5:                                #   in Loop: Header=BB2_1 Depth=1
	movzbl	-41(%rbp), %eax
	cmpl	$32, %eax
	jne	.LBB2_9
# %bb.6:                                #   in Loop: Header=BB2_1 Depth=1
	movq	-16(%rbp), %rax
	cmpq	$0, 24(%rax)
	jne	.LBB2_8
# %bb.7:
	movq	-16(%rbp), %rax
	movl	$13, (%rax)
	jmp	.LBB2_90
.LBB2_8:                                #   in Loop: Header=BB2_1 Depth=1
	movq	-16(%rbp), %rax
	movl	$1, (%rax)
	movq	-40(%rbp), %rcx
	addq	$1, %rcx
	movq	-16(%rbp), %rax
	movq	%rcx, 32(%rax)
	jmp	.LBB2_17
.LBB2_9:                                #   in Loop: Header=BB2_1 Depth=1
	movzbl	-41(%rbp), %edi
	callq	is_tchar
	cmpl	$0, %eax
	je	.LBB2_15
# %bb.10:                               #   in Loop: Header=BB2_1 Depth=1
	movq	-16(%rbp), %rax
	cmpq	$0, 24(%rax)
	jne	.LBB2_12
# %bb.11:                               #   in Loop: Header=BB2_1 Depth=1
	movq	-40(%rbp), %rcx
	movq	-16(%rbp), %rax
	movq	%rcx, 16(%rax)
.LBB2_12:                               #   in Loop: Header=BB2_1 Depth=1
	movq	-16(%rbp), %rcx
	movq	24(%rcx), %rax
	addq	$1, %rax
	movq	%rax, 24(%rcx)
	cmpq	$16, %rax
	jbe	.LBB2_14
# %bb.13:
	movq	-16(%rbp), %rax
	movl	$13, (%rax)
	jmp	.LBB2_90
.LBB2_14:                               #   in Loop: Header=BB2_1 Depth=1
	jmp	.LBB2_16
.LBB2_15:
	movq	-16(%rbp), %rax
	movl	$13, (%rax)
	jmp	.LBB2_90
.LBB2_16:                               #   in Loop: Header=BB2_1 Depth=1
	jmp	.LBB2_17
.LBB2_17:                               #   in Loop: Header=BB2_1 Depth=1
	jmp	.LBB2_87
.LBB2_18:                               #   in Loop: Header=BB2_1 Depth=1
	movzbl	-41(%rbp), %eax
	cmpl	$32, %eax
	jne	.LBB2_22
# %bb.19:                               #   in Loop: Header=BB2_1 Depth=1
	movq	-16(%rbp), %rax
	cmpq	$0, 40(%rax)
	jne	.LBB2_21
# %bb.20:
	movq	-16(%rbp), %rax
	movl	$13, (%rax)
	jmp	.LBB2_90
.LBB2_21:                               #   in Loop: Header=BB2_1 Depth=1
	movq	-16(%rbp), %rax
	movl	$2, (%rax)
	movq	-16(%rbp), %rax
	movb	$0, 2156(%rax)
	jmp	.LBB2_29
.LBB2_22:                               #   in Loop: Header=BB2_1 Depth=1
	movzbl	-41(%rbp), %eax
	cmpl	$32, %eax
	jle	.LBB2_24
# %bb.23:                               #   in Loop: Header=BB2_1 Depth=1
	movzbl	-41(%rbp), %eax
	cmpl	$127, %eax
	jne	.LBB2_25
.LBB2_24:
	movq	-16(%rbp), %rax
	movl	$13, (%rax)
	jmp	.LBB2_90
.LBB2_25:                               #   in Loop: Header=BB2_1 Depth=1
	movq	-16(%rbp), %rax
	cmpq	$0, 40(%rax)
	jne	.LBB2_27
# %bb.26:                               #   in Loop: Header=BB2_1 Depth=1
	movq	-40(%rbp), %rcx
	movq	-16(%rbp), %rax
	movq	%rcx, 32(%rax)
.LBB2_27:                               #   in Loop: Header=BB2_1 Depth=1
	movq	-16(%rbp), %rax
	movq	40(%rax), %rcx
	addq	$1, %rcx
	movq	%rcx, 40(%rax)
# %bb.28:                               #   in Loop: Header=BB2_1 Depth=1
	jmp	.LBB2_29
.LBB2_29:                               #   in Loop: Header=BB2_1 Depth=1
	jmp	.LBB2_87
.LBB2_30:                               #   in Loop: Header=BB2_1 Depth=1
	movzbl	-41(%rbp), %eax
	movq	-16(%rbp), %rcx
	movzbl	2156(%rcx), %ecx
	movl	%ecx, %edx
	leaq	hp_execute.V(%rip), %rcx
	movzbl	(%rcx,%rdx), %ecx
	cmpl	%ecx, %eax
	je	.LBB2_32
# %bb.31:
	movq	-16(%rbp), %rax
	movl	$13, (%rax)
	jmp	.LBB2_90
.LBB2_32:                               #   in Loop: Header=BB2_1 Depth=1
	movq	-16(%rbp), %rcx
	movb	2156(%rcx), %al
	addb	$1, %al
	movb	%al, 2156(%rcx)
	movzbl	%al, %eax
	cmpl	$7, %eax
	jne	.LBB2_34
# %bb.33:                               #   in Loop: Header=BB2_1 Depth=1
	movq	-16(%rbp), %rax
	movl	$3, (%rax)
.LBB2_34:                               #   in Loop: Header=BB2_1 Depth=1
	jmp	.LBB2_87
.LBB2_35:                               #   in Loop: Header=BB2_1 Depth=1
	movzbl	-41(%rbp), %eax
	cmpl	$48, %eax
	jl	.LBB2_37
# %bb.36:                               #   in Loop: Header=BB2_1 Depth=1
	movzbl	-41(%rbp), %eax
	cmpl	$57, %eax
	jle	.LBB2_38
.LBB2_37:
	movq	-16(%rbp), %rax
	movl	$13, (%rax)
	jmp	.LBB2_90
.LBB2_38:                               #   in Loop: Header=BB2_1 Depth=1
	movzbl	-41(%rbp), %ecx
	subl	$48, %ecx
	movq	-16(%rbp), %rax
	movl	%ecx, 48(%rax)
	movq	-16(%rbp), %rax
	movl	48(%rax), %edx
	xorl	%ecx, %ecx
	movl	$1, %eax
	cmpl	$1, %edx
	cmovgel	%eax, %ecx
	movq	-16(%rbp), %rax
	movl	%ecx, 2140(%rax)
	movq	-16(%rbp), %rax
	movl	$4, (%rax)
	jmp	.LBB2_87
.LBB2_39:                               #   in Loop: Header=BB2_1 Depth=1
	movzbl	-41(%rbp), %eax
	cmpl	$13, %eax
	je	.LBB2_41
# %bb.40:
	movq	-16(%rbp), %rax
	movl	$13, (%rax)
	jmp	.LBB2_90
.LBB2_41:                               #   in Loop: Header=BB2_1 Depth=1
	movq	-16(%rbp), %rax
	movl	$5, (%rax)
	jmp	.LBB2_87
.LBB2_42:                               #   in Loop: Header=BB2_1 Depth=1
	movzbl	-41(%rbp), %eax
	cmpl	$10, %eax
	je	.LBB2_44
# %bb.43:
	movq	-16(%rbp), %rax
	movl	$13, (%rax)
	jmp	.LBB2_90
.LBB2_44:                               #   in Loop: Header=BB2_1 Depth=1
	movq	-16(%rbp), %rax
	movl	$6, (%rax)
	jmp	.LBB2_87
.LBB2_45:                               #   in Loop: Header=BB2_1 Depth=1
	movzbl	-41(%rbp), %eax
	cmpl	$13, %eax
	jne	.LBB2_47
# %bb.46:                               #   in Loop: Header=BB2_1 Depth=1
	movq	-16(%rbp), %rax
	movl	$11, (%rax)
	jmp	.LBB2_51
.LBB2_47:                               #   in Loop: Header=BB2_1 Depth=1
	movzbl	-41(%rbp), %edi
	callq	is_tchar
	cmpl	$0, %eax
	je	.LBB2_49
# %bb.48:                               #   in Loop: Header=BB2_1 Depth=1
	movq	-40(%rbp), %rcx
	movq	-16(%rbp), %rax
	movq	%rcx, 56(%rax)
	movq	-16(%rbp), %rax
	movq	$1, 64(%rax)
	movq	-16(%rbp), %rax
	movl	$7, (%rax)
	jmp	.LBB2_50
.LBB2_49:
	movq	-16(%rbp), %rax
	movl	$13, (%rax)
	jmp	.LBB2_90
.LBB2_50:                               #   in Loop: Header=BB2_1 Depth=1
	jmp	.LBB2_51
.LBB2_51:                               #   in Loop: Header=BB2_1 Depth=1
	jmp	.LBB2_87
.LBB2_52:                               #   in Loop: Header=BB2_1 Depth=1
	movzbl	-41(%rbp), %eax
	cmpl	$58, %eax
	jne	.LBB2_54
# %bb.53:                               #   in Loop: Header=BB2_1 Depth=1
	movq	-16(%rbp), %rax
	movl	$8, (%rax)
	movq	-40(%rbp), %rcx
	addq	$1, %rcx
	movq	-16(%rbp), %rax
	movq	%rcx, 72(%rax)
	movq	-16(%rbp), %rax
	movq	$0, 80(%rax)
	jmp	.LBB2_58
.LBB2_54:                               #   in Loop: Header=BB2_1 Depth=1
	movzbl	-41(%rbp), %edi
	callq	is_tchar
	cmpl	$0, %eax
	je	.LBB2_56
# %bb.55:                               #   in Loop: Header=BB2_1 Depth=1
	movq	-16(%rbp), %rax
	movq	64(%rax), %rcx
	addq	$1, %rcx
	movq	%rcx, 64(%rax)
	jmp	.LBB2_57
.LBB2_56:
	movq	-16(%rbp), %rax
	movl	$13, (%rax)
	jmp	.LBB2_90
.LBB2_57:                               #   in Loop: Header=BB2_1 Depth=1
	jmp	.LBB2_58
.LBB2_58:                               #   in Loop: Header=BB2_1 Depth=1
	jmp	.LBB2_87
.LBB2_59:                               #   in Loop: Header=BB2_1 Depth=1
	movzbl	-41(%rbp), %eax
	cmpl	$32, %eax
	je	.LBB2_61
# %bb.60:                               #   in Loop: Header=BB2_1 Depth=1
	movzbl	-41(%rbp), %eax
	cmpl	$9, %eax
	jne	.LBB2_62
.LBB2_61:                               #   in Loop: Header=BB2_1 Depth=1
	jmp	.LBB2_70
.LBB2_62:                               #   in Loop: Header=BB2_1 Depth=1
	movzbl	-41(%rbp), %eax
	cmpl	$13, %eax
	jne	.LBB2_64
# %bb.63:                               #   in Loop: Header=BB2_1 Depth=1
	movq	-40(%rbp), %rcx
	movq	-16(%rbp), %rax
	movq	%rcx, 72(%rax)
	movq	-16(%rbp), %rax
	movq	$0, 80(%rax)
	movq	-16(%rbp), %rax
	movl	$10, (%rax)
	jmp	.LBB2_69
.LBB2_64:                               #   in Loop: Header=BB2_1 Depth=1
	movzbl	-41(%rbp), %eax
	cmpl	$10, %eax
	je	.LBB2_66
# %bb.65:                               #   in Loop: Header=BB2_1 Depth=1
	movzbl	-41(%rbp), %eax
	cmpl	$127, %eax
	jne	.LBB2_67
.LBB2_66:
	movq	-16(%rbp), %rax
	movl	$13, (%rax)
	jmp	.LBB2_90
.LBB2_67:                               #   in Loop: Header=BB2_1 Depth=1
	movq	-40(%rbp), %rcx
	movq	-16(%rbp), %rax
	movq	%rcx, 72(%rax)
	movq	-16(%rbp), %rax
	movq	$1, 80(%rax)
	movq	-16(%rbp), %rax
	movl	$9, (%rax)
# %bb.68:                               #   in Loop: Header=BB2_1 Depth=1
	jmp	.LBB2_69
.LBB2_69:                               #   in Loop: Header=BB2_1 Depth=1
	jmp	.LBB2_70
.LBB2_70:                               #   in Loop: Header=BB2_1 Depth=1
	jmp	.LBB2_87
.LBB2_71:                               #   in Loop: Header=BB2_1 Depth=1
	movzbl	-41(%rbp), %eax
	cmpl	$13, %eax
	jne	.LBB2_73
# %bb.72:                               #   in Loop: Header=BB2_1 Depth=1
	movq	-16(%rbp), %rax
	movl	$10, (%rax)
	jmp	.LBB2_77
.LBB2_73:                               #   in Loop: Header=BB2_1 Depth=1
	movzbl	-41(%rbp), %eax
	cmpl	$10, %eax
	jne	.LBB2_75
# %bb.74:
	movq	-16(%rbp), %rax
	movl	$13, (%rax)
	jmp	.LBB2_90
.LBB2_75:                               #   in Loop: Header=BB2_1 Depth=1
	movq	-16(%rbp), %rax
	movq	80(%rax), %rcx
	addq	$1, %rcx
	movq	%rcx, 80(%rax)
# %bb.76:                               #   in Loop: Header=BB2_1 Depth=1
	jmp	.LBB2_77
.LBB2_77:                               #   in Loop: Header=BB2_1 Depth=1
	jmp	.LBB2_87
.LBB2_78:                               #   in Loop: Header=BB2_1 Depth=1
	movzbl	-41(%rbp), %eax
	cmpl	$10, %eax
	je	.LBB2_80
# %bb.79:
	movq	-16(%rbp), %rax
	movl	$13, (%rax)
	jmp	.LBB2_90
.LBB2_80:                               #   in Loop: Header=BB2_1 Depth=1
	movq	-16(%rbp), %rdi
	movq	-24(%rbp), %rsi
	callq	finish_header
	cmpl	$0, %eax
	je	.LBB2_82
# %bb.81:
	movq	-16(%rbp), %rax
	movl	$13, (%rax)
	jmp	.LBB2_90
.LBB2_82:                               #   in Loop: Header=BB2_1 Depth=1
	movq	-16(%rbp), %rax
	movl	$6, (%rax)
	jmp	.LBB2_87
.LBB2_83:
	movzbl	-41(%rbp), %eax
	cmpl	$10, %eax
	je	.LBB2_85
# %bb.84:
	movq	-16(%rbp), %rax
	movl	$13, (%rax)
	jmp	.LBB2_90
.LBB2_85:
	movq	-16(%rbp), %rax
	movl	$12, (%rax)
	movq	-40(%rbp), %rax
	addq	$1, %rax
	movq	%rax, -40(%rbp)
	jmp	.LBB2_90
.LBB2_86:
	movq	-16(%rbp), %rax
	movl	$13, (%rax)
	jmp	.LBB2_90
.LBB2_87:                               #   in Loop: Header=BB2_1 Depth=1
	jmp	.LBB2_88
.LBB2_88:                               #   in Loop: Header=BB2_1 Depth=1
	movq	-40(%rbp), %rax
	addq	$1, %rax
	movq	%rax, -40(%rbp)
	jmp	.LBB2_1
.LBB2_89:
	jmp	.LBB2_90
.LBB2_90:
	movq	-40(%rbp), %rcx
	movq	-16(%rbp), %rax
	movq	%rcx, 8(%rax)
	movq	-16(%rbp), %rax
	cmpl	$13, (%rax)
	jne	.LBB2_92
# %bb.91:
	movl	$-1, -4(%rbp)
	jmp	.LBB2_95
.LBB2_92:
	movq	-16(%rbp), %rax
	cmpl	$12, (%rax)
	jne	.LBB2_94
# %bb.93:
	movl	$1, -4(%rbp)
	jmp	.LBB2_95
.LBB2_94:
	movl	$0, -4(%rbp)
.LBB2_95:
	movl	-4(%rbp), %eax
	addq	$48, %rsp
	popq	%rbp
	retq
.Lfunc_end2:
	.size	hp_execute, .Lfunc_end2-hp_execute
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
	jl	.LBB3_2
# %bb.1:
	movzbl	-5(%rbp), %eax
	cmpl	$122, %eax
	jle	.LBB3_6
.LBB3_2:
	movzbl	-5(%rbp), %eax
	cmpl	$65, %eax
	jl	.LBB3_4
# %bb.3:
	movzbl	-5(%rbp), %eax
	cmpl	$90, %eax
	jle	.LBB3_6
.LBB3_4:
	movzbl	-5(%rbp), %eax
	cmpl	$48, %eax
	jl	.LBB3_7
# %bb.5:
	movzbl	-5(%rbp), %eax
	cmpl	$57, %eax
	jg	.LBB3_7
.LBB3_6:
	movl	$1, -4(%rbp)
	jmp	.LBB3_10
.LBB3_7:
	movzbl	-5(%rbp), %eax
	movl	%eax, -12(%rbp)                 # 4-byte Spill
	subl	$33, %eax
	je	.LBB3_8
	jmp	.LBB3_11
.LBB3_11:
	movl	-12(%rbp), %eax                 # 4-byte Reload
	addl	$-35, %eax
	subl	$5, %eax
	jb	.LBB3_8
	jmp	.LBB3_12
.LBB3_12:
	movl	-12(%rbp), %eax                 # 4-byte Reload
	addl	$-42, %eax
	subl	$2, %eax
	jb	.LBB3_8
	jmp	.LBB3_13
.LBB3_13:
	movl	-12(%rbp), %eax                 # 4-byte Reload
	addl	$-45, %eax
	subl	$2, %eax
	jb	.LBB3_8
	jmp	.LBB3_14
.LBB3_14:
	movl	-12(%rbp), %eax                 # 4-byte Reload
	addl	$-94, %eax
	subl	$3, %eax
	jb	.LBB3_8
	jmp	.LBB3_15
.LBB3_15:
	movl	-12(%rbp), %eax                 # 4-byte Reload
	subl	$124, %eax
	je	.LBB3_8
	jmp	.LBB3_16
.LBB3_16:
	movl	-12(%rbp), %eax                 # 4-byte Reload
	subl	$126, %eax
	jne	.LBB3_9
	jmp	.LBB3_8
.LBB3_8:
	movl	$1, -4(%rbp)
	jmp	.LBB3_10
.LBB3_9:
	movl	$0, -4(%rbp)
.LBB3_10:
	movl	-4(%rbp), %eax
	popq	%rbp
	retq
.Lfunc_end3:
	.size	is_tchar, .Lfunc_end3-is_tchar
                                        # -- End function
	.p2align	4                               # -- Begin function finish_header
	.type	finish_header,@function
finish_header:                          # @finish_header
# %bb.0:
	pushq	%rbp
	movq	%rsp, %rbp
	subq	$112, %rsp
	movq	%rdi, -16(%rbp)
	movq	%rsi, -24(%rbp)
	movq	-16(%rbp), %rax
	movq	56(%rax), %rcx
	movq	%rcx, -40(%rbp)
	movq	64(%rax), %rax
	movq	%rax, -32(%rbp)
	movq	-16(%rbp), %rax
	movq	72(%rax), %rcx
	movq	%rcx, -56(%rbp)
	movq	80(%rax), %rax
	movq	%rax, -48(%rbp)
.LBB4_1:                                # =>This Inner Loop Header: Depth=1
	cmpq	$0, -48(%rbp)
	jbe	.LBB4_7
# %bb.2:                                #   in Loop: Header=BB4_1 Depth=1
	movq	-24(%rbp), %rax
	movq	-56(%rbp), %rcx
	addq	-48(%rbp), %rcx
	subq	$1, %rcx
	movb	(%rax,%rcx), %al
	movb	%al, -57(%rbp)
	movzbl	-57(%rbp), %eax
	cmpl	$32, %eax
	je	.LBB4_4
# %bb.3:                                #   in Loop: Header=BB4_1 Depth=1
	movzbl	-57(%rbp), %eax
	cmpl	$9, %eax
	jne	.LBB4_5
.LBB4_4:                                #   in Loop: Header=BB4_1 Depth=1
	movq	-48(%rbp), %rax
	addq	$-1, %rax
	movq	%rax, -48(%rbp)
	jmp	.LBB4_6
.LBB4_5:
	jmp	.LBB4_7
.LBB4_6:                                #   in Loop: Header=BB4_1 Depth=1
	jmp	.LBB4_1
.LBB4_7:
	movq	-16(%rbp), %rax
	cmpl	$64, 2136(%rax)
	jb	.LBB4_9
# %bb.8:
	movl	$-1, -4(%rbp)
	jmp	.LBB4_32
.LBB4_9:
	movq	-16(%rbp), %rax
	addq	$88, %rax
	movq	-16(%rbp), %rcx
	movl	2136(%rcx), %ecx
                                        # kill: def $rcx killed $ecx
	shlq	$4, %rcx
	addq	%rcx, %rax
	movq	-40(%rbp), %rcx
	movq	%rcx, (%rax)
	movq	-32(%rbp), %rcx
	movq	%rcx, 8(%rax)
	movq	-16(%rbp), %rax
	addq	$1112, %rax                     # imm = 0x458
	movq	-16(%rbp), %rcx
	movl	2136(%rcx), %ecx
                                        # kill: def $rcx killed $ecx
	shlq	$4, %rcx
	addq	%rcx, %rax
	movq	-56(%rbp), %rcx
	movq	%rcx, (%rax)
	movq	-48(%rbp), %rcx
	movq	%rcx, 8(%rax)
	movq	-16(%rbp), %rax
	movl	2136(%rax), %ecx
	addl	$1, %ecx
	movl	%ecx, 2136(%rax)
	movq	-24(%rbp), %rdi
	movq	-40(%rbp), %rsi
	movq	-32(%rbp), %rdx
	leaq	.L.str(%rip), %rcx
	callq	hp_slice_ci_eq
	cmpl	$0, %eax
	je	.LBB4_16
# %bb.10:
	movq	-24(%rbp), %rdi
	movq	-56(%rbp), %rsi
	movq	-48(%rbp), %rdx
	leaq	.L.str.1(%rip), %rcx
	callq	slice_ci_contains
	cmpl	$0, %eax
	je	.LBB4_12
# %bb.11:
	movq	-16(%rbp), %rax
	movl	$0, 2140(%rax)
	jmp	.LBB4_15
.LBB4_12:
	movq	-24(%rbp), %rdi
	movq	-56(%rbp), %rsi
	movq	-48(%rbp), %rdx
	leaq	.L.str.2(%rip), %rcx
	callq	slice_ci_contains
	cmpl	$0, %eax
	je	.LBB4_14
# %bb.13:
	movq	-16(%rbp), %rax
	movl	$1, 2140(%rax)
.LBB4_14:
	jmp	.LBB4_15
.LBB4_15:
	jmp	.LBB4_31
.LBB4_16:
	movq	-24(%rbp), %rdi
	movq	-40(%rbp), %rsi
	movq	-32(%rbp), %rdx
	leaq	.L.str.3(%rip), %rcx
	callq	hp_slice_ci_eq
	cmpl	$0, %eax
	je	.LBB4_30
# %bb.17:
	movq	$0, -72(%rbp)
	cmpq	$0, -48(%rbp)
	seta	%al
	andb	$1, %al
	movzbl	%al, %eax
	movl	%eax, -76(%rbp)
	movq	$0, -88(%rbp)
.LBB4_18:                               # =>This Inner Loop Header: Depth=1
	movq	-88(%rbp), %rax
	cmpq	-48(%rbp), %rax
	jae	.LBB4_26
# %bb.19:                               #   in Loop: Header=BB4_18 Depth=1
	movq	-24(%rbp), %rax
	movq	-56(%rbp), %rcx
	addq	-88(%rbp), %rcx
	movb	(%rax,%rcx), %al
	movb	%al, -89(%rbp)
	movzbl	-89(%rbp), %eax
	cmpl	$48, %eax
	jl	.LBB4_21
# %bb.20:                               #   in Loop: Header=BB4_18 Depth=1
	movzbl	-89(%rbp), %eax
	cmpl	$57, %eax
	jle	.LBB4_22
.LBB4_21:
	movl	$0, -76(%rbp)
	jmp	.LBB4_26
.LBB4_22:                               #   in Loop: Header=BB4_18 Depth=1
	movabsq	$922337203685477579, %rax       # imm = 0xCCCCCCCCCCCCCCB
	cmpq	%rax, -72(%rbp)
	jle	.LBB4_24
# %bb.23:
	movl	$0, -76(%rbp)
	jmp	.LBB4_26
.LBB4_24:                               #   in Loop: Header=BB4_18 Depth=1
	imulq	$10, -72(%rbp), %rax
	movzbl	-89(%rbp), %ecx
	subl	$48, %ecx
	movslq	%ecx, %rcx
	addq	%rcx, %rax
	movq	%rax, -72(%rbp)
# %bb.25:                               #   in Loop: Header=BB4_18 Depth=1
	movq	-88(%rbp), %rax
	addq	$1, %rax
	movq	%rax, -88(%rbp)
	jmp	.LBB4_18
.LBB4_26:
	cmpl	$0, -76(%rbp)
	je	.LBB4_28
# %bb.27:
	movq	-72(%rbp), %rax
	movq	%rax, -104(%rbp)                # 8-byte Spill
	jmp	.LBB4_29
.LBB4_28:
	movq	$-1, %rax
	movq	%rax, -104(%rbp)                # 8-byte Spill
	jmp	.LBB4_29
.LBB4_29:
	movq	-104(%rbp), %rcx                # 8-byte Reload
	movq	-16(%rbp), %rax
	movq	%rcx, 2144(%rax)
.LBB4_30:
	jmp	.LBB4_31
.LBB4_31:
	movl	$0, -4(%rbp)
.LBB4_32:
	movl	-4(%rbp), %eax
	addq	$112, %rsp
	popq	%rbp
	retq
.Lfunc_end4:
	.size	finish_header, .Lfunc_end4-finish_header
                                        # -- End function
	.p2align	4                               # -- Begin function slice_ci_contains
	.type	slice_ci_contains,@function
slice_ci_contains:                      # @slice_ci_contains
# %bb.0:
	pushq	%rbp
	movq	%rsp, %rbp
	subq	$80, %rsp
	movq	%rsi, -24(%rbp)
	movq	%rdx, -16(%rbp)
	movq	%rdi, -32(%rbp)
	movq	%rcx, -40(%rbp)
	movq	$0, -48(%rbp)
.LBB5_1:                                # =>This Inner Loop Header: Depth=1
	movq	-40(%rbp), %rax
	movq	-48(%rbp), %rcx
	cmpb	$0, (%rax,%rcx)
	je	.LBB5_3
# %bb.2:                                #   in Loop: Header=BB5_1 Depth=1
	movq	-48(%rbp), %rax
	addq	$1, %rax
	movq	%rax, -48(%rbp)
	jmp	.LBB5_1
.LBB5_3:
	cmpq	$0, -48(%rbp)
	je	.LBB5_5
# %bb.4:
	movq	-16(%rbp), %rax
	cmpq	-48(%rbp), %rax
	jae	.LBB5_6
.LBB5_5:
	movl	$0, -4(%rbp)
	jmp	.LBB5_19
.LBB5_6:
	movq	$0, -56(%rbp)
.LBB5_7:                                # =>This Loop Header: Depth=1
                                        #     Child Loop BB5_9 Depth 2
	movq	-56(%rbp), %rax
	addq	-48(%rbp), %rax
	cmpq	-16(%rbp), %rax
	ja	.LBB5_18
# %bb.8:                                #   in Loop: Header=BB5_7 Depth=1
	movq	$0, -64(%rbp)
.LBB5_9:                                #   Parent Loop BB5_7 Depth=1
                                        # =>  This Inner Loop Header: Depth=2
	movq	-64(%rbp), %rax
	cmpq	-48(%rbp), %rax
	jae	.LBB5_14
# %bb.10:                               #   in Loop: Header=BB5_9 Depth=2
	movq	-32(%rbp), %rax
	movq	-24(%rbp), %rcx
	addq	-56(%rbp), %rcx
	addq	-64(%rbp), %rcx
	movzbl	(%rax,%rcx), %edi
	callq	lc
	movzbl	%al, %eax
	movl	%eax, -68(%rbp)                 # 4-byte Spill
	movq	-40(%rbp), %rax
	movq	-64(%rbp), %rcx
	movzbl	(%rax,%rcx), %edi
	callq	lc
	movb	%al, %cl
	movl	-68(%rbp), %eax                 # 4-byte Reload
	movzbl	%cl, %ecx
	cmpl	%ecx, %eax
	je	.LBB5_12
# %bb.11:                               #   in Loop: Header=BB5_7 Depth=1
	jmp	.LBB5_14
.LBB5_12:                               #   in Loop: Header=BB5_9 Depth=2
	jmp	.LBB5_13
.LBB5_13:                               #   in Loop: Header=BB5_9 Depth=2
	movq	-64(%rbp), %rax
	addq	$1, %rax
	movq	%rax, -64(%rbp)
	jmp	.LBB5_9
.LBB5_14:                               #   in Loop: Header=BB5_7 Depth=1
	movq	-64(%rbp), %rax
	cmpq	-48(%rbp), %rax
	jne	.LBB5_16
# %bb.15:
	movl	$1, -4(%rbp)
	jmp	.LBB5_19
.LBB5_16:                               #   in Loop: Header=BB5_7 Depth=1
	jmp	.LBB5_17
.LBB5_17:                               #   in Loop: Header=BB5_7 Depth=1
	movq	-56(%rbp), %rax
	addq	$1, %rax
	movq	%rax, -56(%rbp)
	jmp	.LBB5_7
.LBB5_18:
	movl	$0, -4(%rbp)
.LBB5_19:
	movl	-4(%rbp), %eax
	addq	$80, %rsp
	popq	%rbp
	retq
.Lfunc_end5:
	.size	slice_ci_contains, .Lfunc_end5-slice_ci_contains
                                        # -- End function
	.type	hp_execute.V,@object            # @hp_execute.V
	.section	.rodata,"a",@progbits
hp_execute.V:
	.asciz	"HTTP/1."
	.size	hp_execute.V, 8

	.type	.L.str,@object                  # @.str
	.section	.rodata.str1.1,"aMS",@progbits,1
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
	.addrsig_sym hp_slice_ci_eq
	.addrsig_sym lc
	.addrsig_sym is_tchar
	.addrsig_sym finish_header
	.addrsig_sym slice_ci_contains
	.addrsig_sym hp_execute.V
