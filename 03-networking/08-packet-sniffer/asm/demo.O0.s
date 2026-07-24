	.file	"demo.c"
	.text
	.globl	bpf_run                         # -- Begin function bpf_run
	.p2align	4
	.type	bpf_run,@function
bpf_run:                                # @bpf_run
# %bb.0:
	pushq	%rbp
	movq	%rsp, %rbp
	subq	$192, %rsp
	movq	%rdi, -16(%rbp)
	movl	%esi, -20(%rbp)
	movq	%rdx, -32(%rbp)
	movl	%ecx, -36(%rbp)
	movl	$0, -40(%rbp)
	movl	$0, -44(%rbp)
	leaq	-112(%rbp), %rdi
	xorl	%esi, %esi
	movl	$64, %edx
	callq	memset@PLT
	movl	$0, -116(%rbp)
	movl	$0, -120(%rbp)
.LBB0_1:                                # =>This Inner Loop Header: Depth=1
	movl	-116(%rbp), %eax
	cmpl	-20(%rbp), %eax
	jae	.LBB0_79
# %bb.2:                                #   in Loop: Header=BB0_1 Depth=1
	movq	-16(%rbp), %rax
	movl	-116(%rbp), %ecx
                                        # kill: def $rcx killed $ecx
	leaq	(%rax,%rcx,8), %rax
	movq	%rax, -128(%rbp)
	movq	-128(%rbp), %rax
	movw	(%rax), %ax
	movw	%ax, -130(%rbp)
	movq	-128(%rbp), %rax
	movl	4(%rax), %eax
	movl	%eax, -136(%rbp)
	movzwl	-130(%rbp), %eax
	andl	$7, %eax
	movl	%eax, -152(%rbp)                # 4-byte Spill
	je	.LBB0_3
	jmp	.LBB0_81
.LBB0_81:                               #   in Loop: Header=BB0_1 Depth=1
	movl	-152(%rbp), %eax                # 4-byte Reload
	subl	$1, %eax
	je	.LBB0_27
	jmp	.LBB0_82
.LBB0_82:                               #   in Loop: Header=BB0_1 Depth=1
	movl	-152(%rbp), %eax                # 4-byte Reload
	subl	$2, %eax
	je	.LBB0_36
	jmp	.LBB0_83
.LBB0_83:                               #   in Loop: Header=BB0_1 Depth=1
	movl	-152(%rbp), %eax                # 4-byte Reload
	subl	$3, %eax
	je	.LBB0_37
	jmp	.LBB0_84
.LBB0_84:                               #   in Loop: Header=BB0_1 Depth=1
	movl	-152(%rbp), %eax                # 4-byte Reload
	subl	$4, %eax
	je	.LBB0_38
	jmp	.LBB0_85
.LBB0_85:                               #   in Loop: Header=BB0_1 Depth=1
	movl	-152(%rbp), %eax                # 4-byte Reload
	subl	$5, %eax
	je	.LBB0_54
	jmp	.LBB0_86
.LBB0_86:                               #   in Loop: Header=BB0_1 Depth=1
	movl	-152(%rbp), %eax                # 4-byte Reload
	subl	$6, %eax
	je	.LBB0_69
	jmp	.LBB0_87
.LBB0_87:                               #   in Loop: Header=BB0_1 Depth=1
	movl	-152(%rbp), %eax                # 4-byte Reload
	subl	$7, %eax
	je	.LBB0_73
	jmp	.LBB0_77
.LBB0_3:                                #   in Loop: Header=BB0_1 Depth=1
	movzwl	-130(%rbp), %eax
	andl	$224, %eax
	movl	%eax, -156(%rbp)                # 4-byte Spill
	je	.LBB0_4
	jmp	.LBB0_101
.LBB0_101:                              #   in Loop: Header=BB0_1 Depth=1
	movl	-156(%rbp), %eax                # 4-byte Reload
	subl	$32, %eax
	je	.LBB0_7
	jmp	.LBB0_102
.LBB0_102:                              #   in Loop: Header=BB0_1 Depth=1
	movl	-156(%rbp), %eax                # 4-byte Reload
	subl	$64, %eax
	je	.LBB0_16
	jmp	.LBB0_103
.LBB0_103:                              #   in Loop: Header=BB0_1 Depth=1
	movl	-156(%rbp), %eax                # 4-byte Reload
	subl	$96, %eax
	je	.LBB0_6
	jmp	.LBB0_104
.LBB0_104:                              #   in Loop: Header=BB0_1 Depth=1
	movl	-156(%rbp), %eax                # 4-byte Reload
	subl	$128, %eax
	je	.LBB0_5
	jmp	.LBB0_25
.LBB0_4:                                #   in Loop: Header=BB0_1 Depth=1
	movl	-136(%rbp), %eax
	movl	%eax, -40(%rbp)
	jmp	.LBB0_26
.LBB0_5:                                #   in Loop: Header=BB0_1 Depth=1
	movl	-36(%rbp), %eax
	movl	%eax, -40(%rbp)
	jmp	.LBB0_26
.LBB0_6:                                #   in Loop: Header=BB0_1 Depth=1
	movl	-136(%rbp), %eax
	andl	$15, %eax
	movl	%eax, %eax
                                        # kill: def $rax killed $eax
	movl	-112(%rbp,%rax,4), %eax
	movl	%eax, -40(%rbp)
	jmp	.LBB0_26
.LBB0_7:                                #   in Loop: Header=BB0_1 Depth=1
	movzwl	-130(%rbp), %eax
	andl	$24, %eax
	cmpl	$0, %eax
	jne	.LBB0_9
# %bb.8:                                #   in Loop: Header=BB0_1 Depth=1
	movq	-32(%rbp), %rdi
	movl	-36(%rbp), %esi
	movl	-136(%rbp), %edx
	leaq	-120(%rbp), %rcx
	callq	load_word
	movl	%eax, -40(%rbp)
	jmp	.LBB0_13
.LBB0_9:                                #   in Loop: Header=BB0_1 Depth=1
	movzwl	-130(%rbp), %eax
	andl	$24, %eax
	cmpl	$8, %eax
	jne	.LBB0_11
# %bb.10:                               #   in Loop: Header=BB0_1 Depth=1
	movq	-32(%rbp), %rdi
	movl	-36(%rbp), %esi
	movl	-136(%rbp), %edx
	leaq	-120(%rbp), %rcx
	callq	load_half
	movl	%eax, -40(%rbp)
	jmp	.LBB0_12
.LBB0_11:                               #   in Loop: Header=BB0_1 Depth=1
	movq	-32(%rbp), %rdi
	movl	-36(%rbp), %esi
	movl	-136(%rbp), %edx
	leaq	-120(%rbp), %rcx
	callq	load_byte
	movl	%eax, -40(%rbp)
.LBB0_12:                               #   in Loop: Header=BB0_1 Depth=1
	jmp	.LBB0_13
.LBB0_13:                               #   in Loop: Header=BB0_1 Depth=1
	cmpl	$0, -120(%rbp)
	je	.LBB0_15
# %bb.14:
	movl	$0, -4(%rbp)
	jmp	.LBB0_80
.LBB0_15:                               #   in Loop: Header=BB0_1 Depth=1
	jmp	.LBB0_26
.LBB0_16:                               #   in Loop: Header=BB0_1 Depth=1
	movzwl	-130(%rbp), %eax
	andl	$24, %eax
	cmpl	$0, %eax
	jne	.LBB0_18
# %bb.17:                               #   in Loop: Header=BB0_1 Depth=1
	movq	-32(%rbp), %rdi
	movl	-36(%rbp), %esi
	movl	-44(%rbp), %edx
	addl	-136(%rbp), %edx
	leaq	-120(%rbp), %rcx
	callq	load_word
	movl	%eax, -40(%rbp)
	jmp	.LBB0_22
.LBB0_18:                               #   in Loop: Header=BB0_1 Depth=1
	movzwl	-130(%rbp), %eax
	andl	$24, %eax
	cmpl	$8, %eax
	jne	.LBB0_20
# %bb.19:                               #   in Loop: Header=BB0_1 Depth=1
	movq	-32(%rbp), %rdi
	movl	-36(%rbp), %esi
	movl	-44(%rbp), %edx
	addl	-136(%rbp), %edx
	leaq	-120(%rbp), %rcx
	callq	load_half
	movl	%eax, -40(%rbp)
	jmp	.LBB0_21
.LBB0_20:                               #   in Loop: Header=BB0_1 Depth=1
	movq	-32(%rbp), %rdi
	movl	-36(%rbp), %esi
	movl	-44(%rbp), %edx
	addl	-136(%rbp), %edx
	leaq	-120(%rbp), %rcx
	callq	load_byte
	movl	%eax, -40(%rbp)
.LBB0_21:                               #   in Loop: Header=BB0_1 Depth=1
	jmp	.LBB0_22
.LBB0_22:                               #   in Loop: Header=BB0_1 Depth=1
	cmpl	$0, -120(%rbp)
	je	.LBB0_24
# %bb.23:
	movl	$0, -4(%rbp)
	jmp	.LBB0_80
.LBB0_24:                               #   in Loop: Header=BB0_1 Depth=1
	jmp	.LBB0_26
.LBB0_25:
	movl	$0, -4(%rbp)
	jmp	.LBB0_80
.LBB0_26:                               #   in Loop: Header=BB0_1 Depth=1
	movl	-116(%rbp), %eax
	addl	$1, %eax
	movl	%eax, -116(%rbp)
	jmp	.LBB0_78
.LBB0_27:                               #   in Loop: Header=BB0_1 Depth=1
	movzwl	-130(%rbp), %eax
	andl	$224, %eax
	movl	%eax, -160(%rbp)                # 4-byte Spill
	je	.LBB0_28
	jmp	.LBB0_98
.LBB0_98:                               #   in Loop: Header=BB0_1 Depth=1
	movl	-160(%rbp), %eax                # 4-byte Reload
	subl	$96, %eax
	je	.LBB0_30
	jmp	.LBB0_99
.LBB0_99:                               #   in Loop: Header=BB0_1 Depth=1
	movl	-160(%rbp), %eax                # 4-byte Reload
	subl	$128, %eax
	je	.LBB0_29
	jmp	.LBB0_100
.LBB0_100:                              #   in Loop: Header=BB0_1 Depth=1
	movl	-160(%rbp), %eax                # 4-byte Reload
	subl	$160, %eax
	je	.LBB0_31
	jmp	.LBB0_34
.LBB0_28:                               #   in Loop: Header=BB0_1 Depth=1
	movl	-136(%rbp), %eax
	movl	%eax, -44(%rbp)
	jmp	.LBB0_35
.LBB0_29:                               #   in Loop: Header=BB0_1 Depth=1
	movl	-36(%rbp), %eax
	movl	%eax, -44(%rbp)
	jmp	.LBB0_35
.LBB0_30:                               #   in Loop: Header=BB0_1 Depth=1
	movl	-136(%rbp), %eax
	andl	$15, %eax
	movl	%eax, %eax
                                        # kill: def $rax killed $eax
	movl	-112(%rbp,%rax,4), %eax
	movl	%eax, -44(%rbp)
	jmp	.LBB0_35
.LBB0_31:                               #   in Loop: Header=BB0_1 Depth=1
	movq	-32(%rbp), %rdi
	movl	-36(%rbp), %esi
	movl	-136(%rbp), %edx
	leaq	-120(%rbp), %rcx
	callq	load_byte
	andl	$15, %eax
	shll	$2, %eax
	movl	%eax, -44(%rbp)
	cmpl	$0, -120(%rbp)
	je	.LBB0_33
# %bb.32:
	movl	$0, -4(%rbp)
	jmp	.LBB0_80
.LBB0_33:                               #   in Loop: Header=BB0_1 Depth=1
	jmp	.LBB0_35
.LBB0_34:
	movl	$0, -4(%rbp)
	jmp	.LBB0_80
.LBB0_35:                               #   in Loop: Header=BB0_1 Depth=1
	movl	-116(%rbp), %eax
	addl	$1, %eax
	movl	%eax, -116(%rbp)
	jmp	.LBB0_78
.LBB0_36:                               #   in Loop: Header=BB0_1 Depth=1
	movl	-40(%rbp), %ecx
	movl	-136(%rbp), %eax
	andl	$15, %eax
	movl	%eax, %eax
                                        # kill: def $rax killed $eax
	movl	%ecx, -112(%rbp,%rax,4)
	movl	-116(%rbp), %eax
	addl	$1, %eax
	movl	%eax, -116(%rbp)
	jmp	.LBB0_78
.LBB0_37:                               #   in Loop: Header=BB0_1 Depth=1
	movl	-44(%rbp), %ecx
	movl	-136(%rbp), %eax
	andl	$15, %eax
	movl	%eax, %eax
                                        # kill: def $rax killed $eax
	movl	%ecx, -112(%rbp,%rax,4)
	movl	-116(%rbp), %eax
	addl	$1, %eax
	movl	%eax, -116(%rbp)
	jmp	.LBB0_78
.LBB0_38:                               #   in Loop: Header=BB0_1 Depth=1
	movzwl	-130(%rbp), %eax
	andl	$8, %eax
	cmpl	$8, %eax
	jne	.LBB0_40
# %bb.39:                               #   in Loop: Header=BB0_1 Depth=1
	movl	-44(%rbp), %eax
	movl	%eax, -164(%rbp)                # 4-byte Spill
	jmp	.LBB0_41
.LBB0_40:                               #   in Loop: Header=BB0_1 Depth=1
	movl	-136(%rbp), %eax
	movl	%eax, -164(%rbp)                # 4-byte Spill
.LBB0_41:                               #   in Loop: Header=BB0_1 Depth=1
	movl	-164(%rbp), %eax                # 4-byte Reload
	movl	%eax, -140(%rbp)
	movzwl	-130(%rbp), %eax
	andl	$240, %eax
	movl	%eax, -168(%rbp)                # 4-byte Spill
	je	.LBB0_42
	jmp	.LBB0_91
.LBB0_91:                               #   in Loop: Header=BB0_1 Depth=1
	movl	-168(%rbp), %eax                # 4-byte Reload
	subl	$16, %eax
	je	.LBB0_43
	jmp	.LBB0_92
.LBB0_92:                               #   in Loop: Header=BB0_1 Depth=1
	movl	-168(%rbp), %eax                # 4-byte Reload
	subl	$32, %eax
	je	.LBB0_44
	jmp	.LBB0_93
.LBB0_93:                               #   in Loop: Header=BB0_1 Depth=1
	movl	-168(%rbp), %eax                # 4-byte Reload
	subl	$48, %eax
	je	.LBB0_45
	jmp	.LBB0_94
.LBB0_94:                               #   in Loop: Header=BB0_1 Depth=1
	movl	-168(%rbp), %eax                # 4-byte Reload
	subl	$64, %eax
	je	.LBB0_48
	jmp	.LBB0_95
.LBB0_95:                               #   in Loop: Header=BB0_1 Depth=1
	movl	-168(%rbp), %eax                # 4-byte Reload
	subl	$80, %eax
	je	.LBB0_49
	jmp	.LBB0_96
.LBB0_96:                               #   in Loop: Header=BB0_1 Depth=1
	movl	-168(%rbp), %eax                # 4-byte Reload
	subl	$96, %eax
	je	.LBB0_50
	jmp	.LBB0_97
.LBB0_97:                               #   in Loop: Header=BB0_1 Depth=1
	movl	-168(%rbp), %eax                # 4-byte Reload
	subl	$112, %eax
	je	.LBB0_51
	jmp	.LBB0_52
.LBB0_42:                               #   in Loop: Header=BB0_1 Depth=1
	movl	-140(%rbp), %eax
	addl	-40(%rbp), %eax
	movl	%eax, -40(%rbp)
	jmp	.LBB0_53
.LBB0_43:                               #   in Loop: Header=BB0_1 Depth=1
	movl	-140(%rbp), %ecx
	movl	-40(%rbp), %eax
	subl	%ecx, %eax
	movl	%eax, -40(%rbp)
	jmp	.LBB0_53
.LBB0_44:                               #   in Loop: Header=BB0_1 Depth=1
	movl	-140(%rbp), %eax
	imull	-40(%rbp), %eax
	movl	%eax, -40(%rbp)
	jmp	.LBB0_53
.LBB0_45:                               #   in Loop: Header=BB0_1 Depth=1
	cmpl	$0, -140(%rbp)
	jne	.LBB0_47
# %bb.46:
	movl	$0, -4(%rbp)
	jmp	.LBB0_80
.LBB0_47:                               #   in Loop: Header=BB0_1 Depth=1
	movl	-140(%rbp), %ecx
	movl	-40(%rbp), %eax
	xorl	%edx, %edx
	divl	%ecx
	movl	%eax, -40(%rbp)
	jmp	.LBB0_53
.LBB0_48:                               #   in Loop: Header=BB0_1 Depth=1
	movl	-140(%rbp), %eax
	orl	-40(%rbp), %eax
	movl	%eax, -40(%rbp)
	jmp	.LBB0_53
.LBB0_49:                               #   in Loop: Header=BB0_1 Depth=1
	movl	-140(%rbp), %eax
	andl	-40(%rbp), %eax
	movl	%eax, -40(%rbp)
	jmp	.LBB0_53
.LBB0_50:                               #   in Loop: Header=BB0_1 Depth=1
	movl	-140(%rbp), %ecx
	andl	$31, %ecx
	movl	-40(%rbp), %eax
                                        # kill: def $cl killed $ecx
	shll	%cl, %eax
	movl	%eax, -40(%rbp)
	jmp	.LBB0_53
.LBB0_51:                               #   in Loop: Header=BB0_1 Depth=1
	movl	-140(%rbp), %ecx
	andl	$31, %ecx
	movl	-40(%rbp), %eax
                                        # kill: def $cl killed $ecx
	shrl	%cl, %eax
	movl	%eax, -40(%rbp)
	jmp	.LBB0_53
.LBB0_52:
	movl	$0, -4(%rbp)
	jmp	.LBB0_80
.LBB0_53:                               #   in Loop: Header=BB0_1 Depth=1
	movl	-116(%rbp), %eax
	addl	$1, %eax
	movl	%eax, -116(%rbp)
	jmp	.LBB0_78
.LBB0_54:                               #   in Loop: Header=BB0_1 Depth=1
	movzwl	-130(%rbp), %eax
	andl	$240, %eax
	cmpl	$0, %eax
	jne	.LBB0_56
# %bb.55:                               #   in Loop: Header=BB0_1 Depth=1
	movl	-136(%rbp), %eax
	addl	$1, %eax
	addl	-116(%rbp), %eax
	movl	%eax, -116(%rbp)
	jmp	.LBB0_78
.LBB0_56:                               #   in Loop: Header=BB0_1 Depth=1
	movzwl	-130(%rbp), %eax
	andl	$8, %eax
	cmpl	$8, %eax
	jne	.LBB0_58
# %bb.57:                               #   in Loop: Header=BB0_1 Depth=1
	movl	-44(%rbp), %eax
	movl	%eax, -172(%rbp)                # 4-byte Spill
	jmp	.LBB0_59
.LBB0_58:                               #   in Loop: Header=BB0_1 Depth=1
	movl	-136(%rbp), %eax
	movl	%eax, -172(%rbp)                # 4-byte Spill
.LBB0_59:                               #   in Loop: Header=BB0_1 Depth=1
	movl	-172(%rbp), %eax                # 4-byte Reload
	movl	%eax, -144(%rbp)
	movzwl	-130(%rbp), %eax
	andl	$240, %eax
	movl	%eax, -176(%rbp)                # 4-byte Spill
	subl	$16, %eax
	je	.LBB0_60
	jmp	.LBB0_88
.LBB0_88:                               #   in Loop: Header=BB0_1 Depth=1
	movl	-176(%rbp), %eax                # 4-byte Reload
	subl	$32, %eax
	je	.LBB0_61
	jmp	.LBB0_89
.LBB0_89:                               #   in Loop: Header=BB0_1 Depth=1
	movl	-176(%rbp), %eax                # 4-byte Reload
	subl	$48, %eax
	je	.LBB0_62
	jmp	.LBB0_90
.LBB0_90:                               #   in Loop: Header=BB0_1 Depth=1
	movl	-176(%rbp), %eax                # 4-byte Reload
	subl	$64, %eax
	je	.LBB0_63
	jmp	.LBB0_64
.LBB0_60:                               #   in Loop: Header=BB0_1 Depth=1
	movl	-40(%rbp), %eax
	cmpl	-144(%rbp), %eax
	sete	%al
	andb	$1, %al
	movzbl	%al, %eax
	movl	%eax, -148(%rbp)
	jmp	.LBB0_65
.LBB0_61:                               #   in Loop: Header=BB0_1 Depth=1
	movl	-40(%rbp), %eax
	cmpl	-144(%rbp), %eax
	seta	%al
	andb	$1, %al
	movzbl	%al, %eax
	movl	%eax, -148(%rbp)
	jmp	.LBB0_65
.LBB0_62:                               #   in Loop: Header=BB0_1 Depth=1
	movl	-40(%rbp), %eax
	cmpl	-144(%rbp), %eax
	setae	%al
	andb	$1, %al
	movzbl	%al, %eax
	movl	%eax, -148(%rbp)
	jmp	.LBB0_65
.LBB0_63:                               #   in Loop: Header=BB0_1 Depth=1
	movl	-40(%rbp), %eax
	andl	-144(%rbp), %eax
	cmpl	$0, %eax
	setne	%al
	andb	$1, %al
	movzbl	%al, %eax
	movl	%eax, -148(%rbp)
	jmp	.LBB0_65
.LBB0_64:
	movl	$0, -4(%rbp)
	jmp	.LBB0_80
.LBB0_65:                               #   in Loop: Header=BB0_1 Depth=1
	cmpl	$0, -148(%rbp)
	je	.LBB0_67
# %bb.66:                               #   in Loop: Header=BB0_1 Depth=1
	movq	-128(%rbp), %rax
	movzbl	2(%rax), %eax
	movl	%eax, -180(%rbp)                # 4-byte Spill
	jmp	.LBB0_68
.LBB0_67:                               #   in Loop: Header=BB0_1 Depth=1
	movq	-128(%rbp), %rax
	movzbl	3(%rax), %eax
	movl	%eax, -180(%rbp)                # 4-byte Spill
.LBB0_68:                               #   in Loop: Header=BB0_1 Depth=1
	movl	-180(%rbp), %eax                # 4-byte Reload
	addl	$1, %eax
	addl	-116(%rbp), %eax
	movl	%eax, -116(%rbp)
	jmp	.LBB0_78
.LBB0_69:
	movzwl	-130(%rbp), %eax
	andl	$24, %eax
	cmpl	$16, %eax
	jne	.LBB0_71
# %bb.70:
	movl	-40(%rbp), %eax
	movl	%eax, -184(%rbp)                # 4-byte Spill
	jmp	.LBB0_72
.LBB0_71:
	movl	-136(%rbp), %eax
	movl	%eax, -184(%rbp)                # 4-byte Spill
.LBB0_72:
	movl	-184(%rbp), %eax                # 4-byte Reload
	movl	%eax, -4(%rbp)
	jmp	.LBB0_80
.LBB0_73:                               #   in Loop: Header=BB0_1 Depth=1
	movzwl	-130(%rbp), %eax
	andl	$128, %eax
	cmpl	$0, %eax
	je	.LBB0_75
# %bb.74:                               #   in Loop: Header=BB0_1 Depth=1
	movl	-44(%rbp), %eax
	movl	%eax, -40(%rbp)
	jmp	.LBB0_76
.LBB0_75:                               #   in Loop: Header=BB0_1 Depth=1
	movl	-40(%rbp), %eax
	movl	%eax, -44(%rbp)
.LBB0_76:                               #   in Loop: Header=BB0_1 Depth=1
	movl	-116(%rbp), %eax
	addl	$1, %eax
	movl	%eax, -116(%rbp)
	jmp	.LBB0_78
.LBB0_77:
	movl	$0, -4(%rbp)
	jmp	.LBB0_80
.LBB0_78:                               #   in Loop: Header=BB0_1 Depth=1
	jmp	.LBB0_1
.LBB0_79:
	movl	$0, -4(%rbp)
.LBB0_80:
	movl	-4(%rbp), %eax
	addq	$192, %rsp
	popq	%rbp
	retq
.Lfunc_end0:
	.size	bpf_run, .Lfunc_end0-bpf_run
                                        # -- End function
	.p2align	4                               # -- Begin function load_word
	.type	load_word,@function
load_word:                              # @load_word
# %bb.0:
	pushq	%rbp
	movq	%rsp, %rbp
	movq	%rdi, -16(%rbp)
	movl	%esi, -20(%rbp)
	movl	%edx, -24(%rbp)
	movq	%rcx, -32(%rbp)
	movl	-24(%rbp), %eax
	addl	$4, %eax
	cmpl	-20(%rbp), %eax
	ja	.LBB1_2
# %bb.1:
	movl	-24(%rbp), %eax
	addl	$4, %eax
	cmpl	-24(%rbp), %eax
	jae	.LBB1_3
.LBB1_2:
	movq	-32(%rbp), %rax
	movl	$1, (%rax)
	movl	$0, -4(%rbp)
	jmp	.LBB1_4
.LBB1_3:
	movq	-16(%rbp), %rax
	movl	-24(%rbp), %ecx
                                        # kill: def $rcx killed $ecx
	movzbl	(%rax,%rcx), %eax
	shll	$24, %eax
	movq	-16(%rbp), %rcx
	movl	-24(%rbp), %edx
	addl	$1, %edx
	movl	%edx, %edx
                                        # kill: def $rdx killed $edx
	movzbl	(%rcx,%rdx), %ecx
	shll	$16, %ecx
	orl	%ecx, %eax
	movq	-16(%rbp), %rcx
	movl	-24(%rbp), %edx
	addl	$2, %edx
	movl	%edx, %edx
                                        # kill: def $rdx killed $edx
	movzbl	(%rcx,%rdx), %ecx
	shll	$8, %ecx
	orl	%ecx, %eax
	movq	-16(%rbp), %rcx
	movl	-24(%rbp), %edx
	addl	$3, %edx
	movl	%edx, %edx
                                        # kill: def $rdx killed $edx
	movzbl	(%rcx,%rdx), %ecx
	orl	%ecx, %eax
	movl	%eax, -4(%rbp)
.LBB1_4:
	movl	-4(%rbp), %eax
	popq	%rbp
	retq
.Lfunc_end1:
	.size	load_word, .Lfunc_end1-load_word
                                        # -- End function
	.p2align	4                               # -- Begin function load_half
	.type	load_half,@function
load_half:                              # @load_half
# %bb.0:
	pushq	%rbp
	movq	%rsp, %rbp
	movq	%rdi, -16(%rbp)
	movl	%esi, -20(%rbp)
	movl	%edx, -24(%rbp)
	movq	%rcx, -32(%rbp)
	movl	-24(%rbp), %eax
	addl	$2, %eax
	cmpl	-20(%rbp), %eax
	ja	.LBB2_2
# %bb.1:
	movl	-24(%rbp), %eax
	addl	$2, %eax
	cmpl	-24(%rbp), %eax
	jae	.LBB2_3
.LBB2_2:
	movq	-32(%rbp), %rax
	movl	$1, (%rax)
	movl	$0, -4(%rbp)
	jmp	.LBB2_4
.LBB2_3:
	movq	-16(%rbp), %rax
	movl	-24(%rbp), %ecx
                                        # kill: def $rcx killed $ecx
	movzbl	(%rax,%rcx), %eax
	shll	$8, %eax
	movq	-16(%rbp), %rcx
	movl	-24(%rbp), %edx
	addl	$1, %edx
	movl	%edx, %edx
                                        # kill: def $rdx killed $edx
	movzbl	(%rcx,%rdx), %ecx
	orl	%ecx, %eax
	movl	%eax, -4(%rbp)
.LBB2_4:
	movl	-4(%rbp), %eax
	popq	%rbp
	retq
.Lfunc_end2:
	.size	load_half, .Lfunc_end2-load_half
                                        # -- End function
	.p2align	4                               # -- Begin function load_byte
	.type	load_byte,@function
load_byte:                              # @load_byte
# %bb.0:
	pushq	%rbp
	movq	%rsp, %rbp
	movq	%rdi, -16(%rbp)
	movl	%esi, -20(%rbp)
	movl	%edx, -24(%rbp)
	movq	%rcx, -32(%rbp)
	movl	-24(%rbp), %eax
	addl	$1, %eax
	cmpl	-20(%rbp), %eax
	ja	.LBB3_2
# %bb.1:
	movl	-24(%rbp), %eax
	addl	$1, %eax
	cmpl	-24(%rbp), %eax
	jae	.LBB3_3
.LBB3_2:
	movq	-32(%rbp), %rax
	movl	$1, (%rax)
	movl	$0, -4(%rbp)
	jmp	.LBB3_4
.LBB3_3:
	movq	-16(%rbp), %rax
	movl	-24(%rbp), %ecx
                                        # kill: def $rcx killed $ecx
	movzbl	(%rax,%rcx), %eax
	movl	%eax, -4(%rbp)
.LBB3_4:
	movl	-4(%rbp), %eax
	popq	%rbp
	retq
.Lfunc_end3:
	.size	load_byte, .Lfunc_end3-load_byte
                                        # -- End function
	.ident	"clang version 20.1.8"
	.section	".note.GNU-stack","",@progbits
	.addrsig
	.addrsig_sym load_word
	.addrsig_sym load_half
	.addrsig_sym load_byte
