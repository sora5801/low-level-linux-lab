	.file	"match.c"
	.text
	.globl	wildcard_match                  # -- Begin function wildcard_match
	.p2align	4
	.type	wildcard_match,@function
wildcard_match:                         # @wildcard_match
# %bb.0:
	pushq	%rbp
	movq	%rsp, %rbp
	subq	$64, %rsp
	movq	%rdi, -16(%rbp)
	movq	%rsi, -24(%rbp)
	movq	$0, -32(%rbp)
	movq	$0, -40(%rbp)
.LBB0_1:                                # =>This Inner Loop Header: Depth=1
	movq	-24(%rbp), %rax
	movsbl	(%rax), %eax
	cmpl	$0, %eax
	je	.LBB0_43
# %bb.2:                                #   in Loop: Header=BB0_1 Depth=1
	movq	-24(%rbp), %rax
	movb	(%rax), %al
	movb	%al, -41(%rbp)
	movq	-16(%rbp), %rax
	movsbl	(%rax), %eax
	cmpl	$63, %eax
	jne	.LBB0_4
# %bb.3:                                #   in Loop: Header=BB0_1 Depth=1
	movq	-16(%rbp), %rax
	addq	$1, %rax
	movq	%rax, -16(%rbp)
	movq	-24(%rbp), %rax
	addq	$1, %rax
	movq	%rax, -24(%rbp)
	jmp	.LBB0_42
.LBB0_4:                                #   in Loop: Header=BB0_1 Depth=1
	movq	-16(%rbp), %rax
	movsbl	(%rax), %eax
	cmpl	$91, %eax
	jne	.LBB0_21
# %bb.5:                                #   in Loop: Header=BB0_1 Depth=1
	movq	-16(%rbp), %rdi
	leaq	-56(%rbp), %rdx
	movzbl	-41(%rbp), %esi
	callq	match_bracket
	movl	%eax, -60(%rbp)
	cmpl	$1, -60(%rbp)
	jne	.LBB0_7
# %bb.6:                                #   in Loop: Header=BB0_1 Depth=1
	movq	-56(%rbp), %rax
	movq	%rax, -16(%rbp)
	movq	-24(%rbp), %rax
	addq	$1, %rax
	movq	%rax, -24(%rbp)
	jmp	.LBB0_20
.LBB0_7:                                #   in Loop: Header=BB0_1 Depth=1
	cmpl	$0, -60(%rbp)
	jne	.LBB0_12
# %bb.8:                                #   in Loop: Header=BB0_1 Depth=1
	cmpq	$0, -32(%rbp)
	je	.LBB0_10
# %bb.9:                                #   in Loop: Header=BB0_1 Depth=1
	movq	-32(%rbp), %rax
	movq	%rax, -16(%rbp)
	movq	-40(%rbp), %rax
	addq	$1, %rax
	movq	%rax, -40(%rbp)
	movq	%rax, -24(%rbp)
	jmp	.LBB0_11
.LBB0_10:
	movl	$0, -4(%rbp)
	jmp	.LBB0_47
.LBB0_11:                               #   in Loop: Header=BB0_1 Depth=1
	jmp	.LBB0_19
.LBB0_12:                               #   in Loop: Header=BB0_1 Depth=1
	movzbl	-41(%rbp), %eax
	cmpl	$91, %eax
	jne	.LBB0_14
# %bb.13:                               #   in Loop: Header=BB0_1 Depth=1
	movq	-16(%rbp), %rax
	addq	$1, %rax
	movq	%rax, -16(%rbp)
	movq	-24(%rbp), %rax
	addq	$1, %rax
	movq	%rax, -24(%rbp)
	jmp	.LBB0_18
.LBB0_14:                               #   in Loop: Header=BB0_1 Depth=1
	cmpq	$0, -32(%rbp)
	je	.LBB0_16
# %bb.15:                               #   in Loop: Header=BB0_1 Depth=1
	movq	-32(%rbp), %rax
	movq	%rax, -16(%rbp)
	movq	-40(%rbp), %rax
	addq	$1, %rax
	movq	%rax, -40(%rbp)
	movq	%rax, -24(%rbp)
	jmp	.LBB0_17
.LBB0_16:
	movl	$0, -4(%rbp)
	jmp	.LBB0_47
.LBB0_17:                               #   in Loop: Header=BB0_1 Depth=1
	jmp	.LBB0_18
.LBB0_18:                               #   in Loop: Header=BB0_1 Depth=1
	jmp	.LBB0_19
.LBB0_19:                               #   in Loop: Header=BB0_1 Depth=1
	jmp	.LBB0_20
.LBB0_20:                               #   in Loop: Header=BB0_1 Depth=1
	jmp	.LBB0_41
.LBB0_21:                               #   in Loop: Header=BB0_1 Depth=1
	movq	-16(%rbp), %rax
	movsbl	(%rax), %eax
	cmpl	$42, %eax
	jne	.LBB0_23
# %bb.22:                               #   in Loop: Header=BB0_1 Depth=1
	movq	-16(%rbp), %rax
	addq	$1, %rax
	movq	%rax, -16(%rbp)
	movq	%rax, -32(%rbp)
	movq	-24(%rbp), %rax
	movq	%rax, -40(%rbp)
	jmp	.LBB0_40
.LBB0_23:                               #   in Loop: Header=BB0_1 Depth=1
	movq	-16(%rbp), %rax
	movsbl	(%rax), %eax
	cmpl	$92, %eax
	jne	.LBB0_32
# %bb.24:                               #   in Loop: Header=BB0_1 Depth=1
	movq	-16(%rbp), %rax
	movsbl	1(%rax), %eax
	cmpl	$0, %eax
	je	.LBB0_32
# %bb.25:                               #   in Loop: Header=BB0_1 Depth=1
	movq	-16(%rbp), %rax
	movzbl	1(%rax), %eax
	movzbl	-41(%rbp), %ecx
	cmpl	%ecx, %eax
	jne	.LBB0_27
# %bb.26:                               #   in Loop: Header=BB0_1 Depth=1
	movq	-16(%rbp), %rax
	addq	$2, %rax
	movq	%rax, -16(%rbp)
	movq	-24(%rbp), %rax
	addq	$1, %rax
	movq	%rax, -24(%rbp)
	jmp	.LBB0_31
.LBB0_27:                               #   in Loop: Header=BB0_1 Depth=1
	cmpq	$0, -32(%rbp)
	je	.LBB0_29
# %bb.28:                               #   in Loop: Header=BB0_1 Depth=1
	movq	-32(%rbp), %rax
	movq	%rax, -16(%rbp)
	movq	-40(%rbp), %rax
	addq	$1, %rax
	movq	%rax, -40(%rbp)
	movq	%rax, -24(%rbp)
	jmp	.LBB0_30
.LBB0_29:
	movl	$0, -4(%rbp)
	jmp	.LBB0_47
.LBB0_30:                               #   in Loop: Header=BB0_1 Depth=1
	jmp	.LBB0_31
.LBB0_31:                               #   in Loop: Header=BB0_1 Depth=1
	jmp	.LBB0_39
.LBB0_32:                               #   in Loop: Header=BB0_1 Depth=1
	movq	-16(%rbp), %rax
	movzbl	(%rax), %eax
	movzbl	-41(%rbp), %ecx
	cmpl	%ecx, %eax
	jne	.LBB0_34
# %bb.33:                               #   in Loop: Header=BB0_1 Depth=1
	movq	-16(%rbp), %rax
	addq	$1, %rax
	movq	%rax, -16(%rbp)
	movq	-24(%rbp), %rax
	addq	$1, %rax
	movq	%rax, -24(%rbp)
	jmp	.LBB0_38
.LBB0_34:                               #   in Loop: Header=BB0_1 Depth=1
	cmpq	$0, -32(%rbp)
	je	.LBB0_36
# %bb.35:                               #   in Loop: Header=BB0_1 Depth=1
	movq	-32(%rbp), %rax
	movq	%rax, -16(%rbp)
	movq	-40(%rbp), %rax
	addq	$1, %rax
	movq	%rax, -40(%rbp)
	movq	%rax, -24(%rbp)
	jmp	.LBB0_37
.LBB0_36:
	movl	$0, -4(%rbp)
	jmp	.LBB0_47
.LBB0_37:                               #   in Loop: Header=BB0_1 Depth=1
	jmp	.LBB0_38
.LBB0_38:                               #   in Loop: Header=BB0_1 Depth=1
	jmp	.LBB0_39
.LBB0_39:                               #   in Loop: Header=BB0_1 Depth=1
	jmp	.LBB0_40
.LBB0_40:                               #   in Loop: Header=BB0_1 Depth=1
	jmp	.LBB0_41
.LBB0_41:                               #   in Loop: Header=BB0_1 Depth=1
	jmp	.LBB0_42
.LBB0_42:                               #   in Loop: Header=BB0_1 Depth=1
	jmp	.LBB0_1
.LBB0_43:
	jmp	.LBB0_44
.LBB0_44:                               # =>This Inner Loop Header: Depth=1
	movq	-16(%rbp), %rax
	movsbl	(%rax), %eax
	cmpl	$42, %eax
	jne	.LBB0_46
# %bb.45:                               #   in Loop: Header=BB0_44 Depth=1
	movq	-16(%rbp), %rax
	addq	$1, %rax
	movq	%rax, -16(%rbp)
	jmp	.LBB0_44
.LBB0_46:
	movq	-16(%rbp), %rax
	movsbl	(%rax), %eax
	cmpl	$0, %eax
	sete	%al
	andb	$1, %al
	movzbl	%al, %eax
	movl	%eax, -4(%rbp)
.LBB0_47:
	movl	-4(%rbp), %eax
	addq	$64, %rsp
	popq	%rbp
	retq
.Lfunc_end0:
	.size	wildcard_match, .Lfunc_end0-wildcard_match
                                        # -- End function
	.p2align	4                               # -- Begin function match_bracket
	.type	match_bracket,@function
match_bracket:                          # @match_bracket
# %bb.0:
	pushq	%rbp
	movq	%rsp, %rbp
	movb	%sil, %al
	movq	%rdi, -16(%rbp)
	movb	%al, -17(%rbp)
	movq	%rdx, -32(%rbp)
	movq	-16(%rbp), %rax
	addq	$1, %rax
	movq	%rax, -40(%rbp)
	movl	$0, -44(%rbp)
	movq	-40(%rbp), %rax
	movsbl	(%rax), %eax
	cmpl	$33, %eax
	je	.LBB1_2
# %bb.1:
	movq	-40(%rbp), %rax
	movsbl	(%rax), %eax
	cmpl	$94, %eax
	jne	.LBB1_3
.LBB1_2:
	movl	$1, -44(%rbp)
	movq	-40(%rbp), %rax
	addq	$1, %rax
	movq	%rax, -40(%rbp)
.LBB1_3:
	movl	$0, -48(%rbp)
	movl	$1, -52(%rbp)
.LBB1_4:                                # =>This Inner Loop Header: Depth=1
	movq	-40(%rbp), %rax
	movsbl	(%rax), %ecx
	xorl	%eax, %eax
                                        # kill: def $al killed $al killed $eax
	cmpl	$0, %ecx
	movb	%al, -55(%rbp)                  # 1-byte Spill
	je	.LBB1_8
# %bb.5:                                #   in Loop: Header=BB1_4 Depth=1
	movq	-40(%rbp), %rax
	movsbl	(%rax), %ecx
	movb	$1, %al
	cmpl	$93, %ecx
	movb	%al, -56(%rbp)                  # 1-byte Spill
	jne	.LBB1_7
# %bb.6:                                #   in Loop: Header=BB1_4 Depth=1
	cmpl	$0, -52(%rbp)
	setne	%al
	movb	%al, -56(%rbp)                  # 1-byte Spill
.LBB1_7:                                #   in Loop: Header=BB1_4 Depth=1
	movb	-56(%rbp), %al                  # 1-byte Reload
	movb	%al, -55(%rbp)                  # 1-byte Spill
.LBB1_8:                                #   in Loop: Header=BB1_4 Depth=1
	movb	-55(%rbp), %al                  # 1-byte Reload
	testb	$1, %al
	jne	.LBB1_9
	jmp	.LBB1_21
.LBB1_9:                                #   in Loop: Header=BB1_4 Depth=1
	movl	$0, -52(%rbp)
	movq	-40(%rbp), %rax
	movsbl	(%rax), %eax
	cmpl	$0, %eax
	je	.LBB1_17
# %bb.10:                               #   in Loop: Header=BB1_4 Depth=1
	movq	-40(%rbp), %rax
	movsbl	1(%rax), %eax
	cmpl	$45, %eax
	jne	.LBB1_17
# %bb.11:                               #   in Loop: Header=BB1_4 Depth=1
	movq	-40(%rbp), %rax
	movsbl	2(%rax), %eax
	cmpl	$0, %eax
	je	.LBB1_17
# %bb.12:                               #   in Loop: Header=BB1_4 Depth=1
	movq	-40(%rbp), %rax
	movsbl	2(%rax), %eax
	cmpl	$93, %eax
	je	.LBB1_17
# %bb.13:                               #   in Loop: Header=BB1_4 Depth=1
	movq	-40(%rbp), %rax
	movb	(%rax), %al
	movb	%al, -53(%rbp)
	movq	-40(%rbp), %rax
	movb	2(%rax), %al
	movb	%al, -54(%rbp)
	movzbl	-53(%rbp), %eax
	movzbl	-17(%rbp), %ecx
	cmpl	%ecx, %eax
	jg	.LBB1_16
# %bb.14:                               #   in Loop: Header=BB1_4 Depth=1
	movzbl	-17(%rbp), %eax
	movzbl	-54(%rbp), %ecx
	cmpl	%ecx, %eax
	jg	.LBB1_16
# %bb.15:                               #   in Loop: Header=BB1_4 Depth=1
	movl	$1, -48(%rbp)
.LBB1_16:                               #   in Loop: Header=BB1_4 Depth=1
	movq	-40(%rbp), %rax
	addq	$3, %rax
	movq	%rax, -40(%rbp)
	jmp	.LBB1_20
.LBB1_17:                               #   in Loop: Header=BB1_4 Depth=1
	movq	-40(%rbp), %rax
	movzbl	(%rax), %eax
	movzbl	-17(%rbp), %ecx
	cmpl	%ecx, %eax
	jne	.LBB1_19
# %bb.18:                               #   in Loop: Header=BB1_4 Depth=1
	movl	$1, -48(%rbp)
.LBB1_19:                               #   in Loop: Header=BB1_4 Depth=1
	movq	-40(%rbp), %rax
	addq	$1, %rax
	movq	%rax, -40(%rbp)
.LBB1_20:                               #   in Loop: Header=BB1_4 Depth=1
	jmp	.LBB1_4
.LBB1_21:
	movq	-40(%rbp), %rax
	movsbl	(%rax), %eax
	cmpl	$93, %eax
	je	.LBB1_23
# %bb.22:
	movl	$-1, -4(%rbp)
	jmp	.LBB1_27
.LBB1_23:
	movq	-40(%rbp), %rcx
	addq	$1, %rcx
	movq	-32(%rbp), %rax
	movq	%rcx, (%rax)
	cmpl	$0, -44(%rbp)
	je	.LBB1_25
# %bb.24:
	cmpl	$0, -48(%rbp)
	setne	%al
	xorb	$-1, %al
	andb	$1, %al
	movzbl	%al, %eax
	movl	%eax, -60(%rbp)                 # 4-byte Spill
	jmp	.LBB1_26
.LBB1_25:
	movl	-48(%rbp), %eax
	movl	%eax, -60(%rbp)                 # 4-byte Spill
.LBB1_26:
	movl	-60(%rbp), %eax                 # 4-byte Reload
	movl	%eax, -4(%rbp)
.LBB1_27:
	movl	-4(%rbp), %eax
	popq	%rbp
	retq
.Lfunc_end1:
	.size	match_bracket, .Lfunc_end1-match_bracket
                                        # -- End function
	.ident	"clang version 20.1.8"
	.section	".note.GNU-stack","",@progbits
	.addrsig
	.addrsig_sym match_bracket
