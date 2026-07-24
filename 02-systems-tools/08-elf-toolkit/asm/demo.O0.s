	.file	"demo.c"
	.text
	.globl	sym_by_addr                     # -- Begin function sym_by_addr
	.p2align	4
	.type	sym_by_addr,@function
sym_by_addr:                            # @sym_by_addr
# %bb.0:
	pushq	%rbp
	movq	%rsp, %rbp
	movq	%rdi, -8(%rbp)
	movl	%esi, -12(%rbp)
	movq	%rdx, -24(%rbp)
	movl	$0, -28(%rbp)
	movl	-12(%rbp), %eax
	subl	$1, %eax
	movl	%eax, -32(%rbp)
	movl	$-1, -36(%rbp)
.LBB0_1:                                # =>This Inner Loop Header: Depth=1
	movl	-28(%rbp), %eax
	cmpl	-32(%rbp), %eax
	jg	.LBB0_6
# %bb.2:                                #   in Loop: Header=BB0_1 Depth=1
	movl	-28(%rbp), %eax
	movl	%eax, -44(%rbp)                 # 4-byte Spill
	movl	-32(%rbp), %eax
	subl	-28(%rbp), %eax
	movl	$2, %ecx
	cltd
	idivl	%ecx
	movl	%eax, %ecx
	movl	-44(%rbp), %eax                 # 4-byte Reload
	addl	%ecx, %eax
	movl	%eax, -40(%rbp)
	movq	-8(%rbp), %rax
	movslq	-40(%rbp), %rcx
	shlq	$4, %rcx
	addq	%rcx, %rax
	movq	(%rax), %rax
	cmpq	-24(%rbp), %rax
	ja	.LBB0_4
# %bb.3:                                #   in Loop: Header=BB0_1 Depth=1
	movl	-40(%rbp), %eax
	movl	%eax, -36(%rbp)
	movl	-40(%rbp), %eax
	addl	$1, %eax
	movl	%eax, -28(%rbp)
	jmp	.LBB0_5
.LBB0_4:                                #   in Loop: Header=BB0_1 Depth=1
	movl	-40(%rbp), %eax
	subl	$1, %eax
	movl	%eax, -32(%rbp)
.LBB0_5:                                #   in Loop: Header=BB0_1 Depth=1
	jmp	.LBB0_1
.LBB0_6:
	movl	-36(%rbp), %eax
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
	movq	%rdi, -16(%rbp)
	movl	%esi, -20(%rbp)
	movq	%rdx, -32(%rbp)
	cmpl	$1, -20(%rbp)
	jae	.LBB1_2
# %bb.1:
	movq	-32(%rbp), %rax
	movl	$0, (%rax)
	movl	$0, -4(%rbp)
	jmp	.LBB1_21
.LBB1_2:
	movq	-16(%rbp), %rax
	movb	(%rax), %al
	movb	%al, -33(%rbp)
	movzbl	-33(%rbp), %eax
	sarl	$6, %eax
	andl	$3, %eax
	movl	%eax, -40(%rbp)
	movzbl	-33(%rbp), %eax
	andl	$7, %eax
	movl	%eax, -44(%rbp)
	movl	$1, -48(%rbp)
	cmpl	$3, -40(%rbp)
	jne	.LBB1_4
# %bb.3:
	movl	$1, -4(%rbp)
	jmp	.LBB1_21
.LBB1_4:
	cmpl	$4, -44(%rbp)
	jne	.LBB1_11
# %bb.5:
	cmpl	$2, -20(%rbp)
	jae	.LBB1_7
# %bb.6:
	movq	-32(%rbp), %rax
	movl	$0, (%rax)
	movl	-48(%rbp), %eax
	movl	%eax, -4(%rbp)
	jmp	.LBB1_21
.LBB1_7:
	movq	-16(%rbp), %rax
	movb	1(%rax), %al
	movb	%al, -49(%rbp)
	movl	$2, -48(%rbp)
	cmpl	$0, -40(%rbp)
	jne	.LBB1_10
# %bb.8:
	movzbl	-49(%rbp), %eax
	andl	$7, %eax
	cmpl	$5, %eax
	jne	.LBB1_10
# %bb.9:
	movl	-48(%rbp), %eax
	addl	$4, %eax
	movl	%eax, -48(%rbp)
.LBB1_10:
	jmp	.LBB1_15
.LBB1_11:
	cmpl	$0, -40(%rbp)
	jne	.LBB1_14
# %bb.12:
	cmpl	$5, -44(%rbp)
	jne	.LBB1_14
# %bb.13:
	movl	-48(%rbp), %eax
	addl	$4, %eax
	movl	%eax, -48(%rbp)
.LBB1_14:
	jmp	.LBB1_15
.LBB1_15:
	cmpl	$1, -40(%rbp)
	jne	.LBB1_17
# %bb.16:
	movl	-48(%rbp), %eax
	addl	$1, %eax
	movl	%eax, -48(%rbp)
	jmp	.LBB1_20
.LBB1_17:
	cmpl	$2, -40(%rbp)
	jne	.LBB1_19
# %bb.18:
	movl	-48(%rbp), %eax
	addl	$4, %eax
	movl	%eax, -48(%rbp)
.LBB1_19:
	jmp	.LBB1_20
.LBB1_20:
	movl	-48(%rbp), %eax
	movl	%eax, -4(%rbp)
.LBB1_21:
	movl	-4(%rbp), %eax
	popq	%rbp
	retq
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
	subq	$176, %rsp
	movq	%rdi, -16(%rbp)
	movl	%esi, -20(%rbp)
	movl	$0, -24(%rbp)
	movl	$0, -28(%rbp)
	movl	$0, -32(%rbp)
.LBB2_1:                                # =>This Inner Loop Header: Depth=1
	movl	-24(%rbp), %eax
	cmpl	-20(%rbp), %eax
	jb	.LBB2_3
# %bb.2:
	movl	$0, -4(%rbp)
	jmp	.LBB2_156
.LBB2_3:                                #   in Loop: Header=BB2_1 Depth=1
	movq	-16(%rbp), %rax
	movl	-24(%rbp), %ecx
                                        # kill: def $rcx killed $ecx
	movb	(%rax,%rcx), %al
	movb	%al, -33(%rbp)
	movzbl	-33(%rbp), %eax
	cmpl	$102, %eax
	jne	.LBB2_5
# %bb.4:                                #   in Loop: Header=BB2_1 Depth=1
	movl	$1, -28(%rbp)
	movl	-24(%rbp), %eax
	addl	$1, %eax
	movl	%eax, -24(%rbp)
	jmp	.LBB2_18
.LBB2_5:                                #   in Loop: Header=BB2_1 Depth=1
	movzbl	-33(%rbp), %eax
	cmpl	$103, %eax
	je	.LBB2_15
# %bb.6:                                #   in Loop: Header=BB2_1 Depth=1
	movzbl	-33(%rbp), %eax
	cmpl	$240, %eax
	je	.LBB2_15
# %bb.7:                                #   in Loop: Header=BB2_1 Depth=1
	movzbl	-33(%rbp), %eax
	cmpl	$242, %eax
	je	.LBB2_15
# %bb.8:                                #   in Loop: Header=BB2_1 Depth=1
	movzbl	-33(%rbp), %eax
	cmpl	$243, %eax
	je	.LBB2_15
# %bb.9:                                #   in Loop: Header=BB2_1 Depth=1
	movzbl	-33(%rbp), %eax
	cmpl	$46, %eax
	je	.LBB2_15
# %bb.10:                               #   in Loop: Header=BB2_1 Depth=1
	movzbl	-33(%rbp), %eax
	cmpl	$54, %eax
	je	.LBB2_15
# %bb.11:                               #   in Loop: Header=BB2_1 Depth=1
	movzbl	-33(%rbp), %eax
	cmpl	$62, %eax
	je	.LBB2_15
# %bb.12:                               #   in Loop: Header=BB2_1 Depth=1
	movzbl	-33(%rbp), %eax
	cmpl	$38, %eax
	je	.LBB2_15
# %bb.13:                               #   in Loop: Header=BB2_1 Depth=1
	movzbl	-33(%rbp), %eax
	cmpl	$100, %eax
	je	.LBB2_15
# %bb.14:                               #   in Loop: Header=BB2_1 Depth=1
	movzbl	-33(%rbp), %eax
	cmpl	$101, %eax
	jne	.LBB2_16
.LBB2_15:                               #   in Loop: Header=BB2_1 Depth=1
	movl	-24(%rbp), %eax
	addl	$1, %eax
	movl	%eax, -24(%rbp)
	jmp	.LBB2_17
.LBB2_16:
	jmp	.LBB2_19
.LBB2_17:                               #   in Loop: Header=BB2_1 Depth=1
	jmp	.LBB2_18
.LBB2_18:                               #   in Loop: Header=BB2_1 Depth=1
	jmp	.LBB2_1
.LBB2_19:
	movl	-24(%rbp), %eax
	cmpl	-20(%rbp), %eax
	jae	.LBB2_23
# %bb.20:
	movq	-16(%rbp), %rax
	movl	-24(%rbp), %ecx
                                        # kill: def $rcx killed $ecx
	movzbl	(%rax,%rcx), %eax
	cmpl	$64, %eax
	jl	.LBB2_23
# %bb.21:
	movq	-16(%rbp), %rax
	movl	-24(%rbp), %ecx
                                        # kill: def $rcx killed $ecx
	movzbl	(%rax,%rcx), %eax
	cmpl	$79, %eax
	jg	.LBB2_23
# %bb.22:
	movq	-16(%rbp), %rax
	movl	-24(%rbp), %ecx
                                        # kill: def $rcx killed $ecx
	movzbl	(%rax,%rcx), %eax
	sarl	$3, %eax
	andl	$1, %eax
	movl	%eax, -32(%rbp)
	movl	-24(%rbp), %eax
	addl	$1, %eax
	movl	%eax, -24(%rbp)
.LBB2_23:
	movl	-24(%rbp), %eax
	cmpl	-20(%rbp), %eax
	jb	.LBB2_25
# %bb.24:
	movl	$0, -4(%rbp)
	jmp	.LBB2_156
.LBB2_25:
	movq	-16(%rbp), %rax
	movl	-24(%rbp), %ecx
	movl	%ecx, %edx
	addl	$1, %edx
	movl	%edx, -24(%rbp)
	movl	%ecx, %ecx
                                        # kill: def $rcx killed $ecx
	movb	(%rax,%rcx), %al
	movb	%al, -34(%rbp)
	movzbl	-34(%rbp), %eax
	cmpl	$15, %eax
	jne	.LBB2_37
# %bb.26:
	movl	-24(%rbp), %eax
	cmpl	-20(%rbp), %eax
	jb	.LBB2_28
# %bb.27:
	movl	$0, -4(%rbp)
	jmp	.LBB2_156
.LBB2_28:
	movq	-16(%rbp), %rax
	movl	-24(%rbp), %ecx
	movl	%ecx, %edx
	addl	$1, %edx
	movl	%edx, -24(%rbp)
	movl	%ecx, %ecx
                                        # kill: def $rcx killed $ecx
	movb	(%rax,%rcx), %al
	movb	%al, -35(%rbp)
	movzbl	-35(%rbp), %eax
	cmpl	$128, %eax
	jl	.LBB2_34
# %bb.29:
	movzbl	-35(%rbp), %eax
	cmpl	$143, %eax
	jg	.LBB2_34
# %bb.30:
	movl	-24(%rbp), %eax
	addl	$4, %eax
	cmpl	-20(%rbp), %eax
	ja	.LBB2_32
# %bb.31:
	movl	-24(%rbp), %eax
	addl	$4, %eax
	movl	%eax, -112(%rbp)                # 4-byte Spill
	jmp	.LBB2_33
.LBB2_32:
	xorl	%eax, %eax
	movl	%eax, -112(%rbp)                # 4-byte Spill
	jmp	.LBB2_33
.LBB2_33:
	movl	-112(%rbp), %eax                # 4-byte Reload
	movl	%eax, -4(%rbp)
	jmp	.LBB2_156
.LBB2_34:
	movl	$1, -40(%rbp)
	movq	-16(%rbp), %rdi
	movl	-24(%rbp), %eax
                                        # kill: def $rax killed $eax
	addq	%rax, %rdi
	movl	-20(%rbp), %esi
	subl	-24(%rbp), %esi
	leaq	-40(%rbp), %rdx
	callq	modrm_bytes
	movl	%eax, -44(%rbp)
	cmpl	$0, -40(%rbp)
	jne	.LBB2_36
# %bb.35:
	movl	$0, -4(%rbp)
	jmp	.LBB2_156
.LBB2_36:
	movl	-24(%rbp), %eax
	addl	-44(%rbp), %eax
	movl	%eax, -4(%rbp)
	jmp	.LBB2_156
.LBB2_37:
	movzbl	-34(%rbp), %eax
	cmpl	$80, %eax
	jl	.LBB2_39
# %bb.38:
	movzbl	-34(%rbp), %eax
	cmpl	$95, %eax
	jle	.LBB2_46
.LBB2_39:
	movzbl	-34(%rbp), %eax
	cmpl	$195, %eax
	je	.LBB2_46
# %bb.40:
	movzbl	-34(%rbp), %eax
	cmpl	$201, %eax
	je	.LBB2_46
# %bb.41:
	movzbl	-34(%rbp), %eax
	cmpl	$144, %eax
	je	.LBB2_46
# %bb.42:
	movzbl	-34(%rbp), %eax
	cmpl	$204, %eax
	je	.LBB2_46
# %bb.43:
	movzbl	-34(%rbp), %eax
	cmpl	$244, %eax
	je	.LBB2_46
# %bb.44:
	movzbl	-34(%rbp), %eax
	cmpl	$152, %eax
	je	.LBB2_46
# %bb.45:
	movzbl	-34(%rbp), %eax
	cmpl	$153, %eax
	jne	.LBB2_47
.LBB2_46:
	movl	-24(%rbp), %eax
	movl	%eax, -4(%rbp)
	jmp	.LBB2_156
.LBB2_47:
	movzbl	-34(%rbp), %eax
	cmpl	$64, %eax
	jge	.LBB2_63
# %bb.48:
	movzbl	-34(%rbp), %eax
	andl	$7, %eax
	cmpl	$6, %eax
	jge	.LBB2_63
# %bb.49:
	movzbl	-34(%rbp), %eax
	andl	$7, %eax
	movl	%eax, -48(%rbp)
	cmpl	$4, -48(%rbp)
	jne	.LBB2_54
# %bb.50:
	movl	-24(%rbp), %eax
	addl	$1, %eax
	cmpl	-20(%rbp), %eax
	ja	.LBB2_52
# %bb.51:
	movl	-24(%rbp), %eax
	addl	$1, %eax
	movl	%eax, -116(%rbp)                # 4-byte Spill
	jmp	.LBB2_53
.LBB2_52:
	xorl	%eax, %eax
	movl	%eax, -116(%rbp)                # 4-byte Spill
	jmp	.LBB2_53
.LBB2_53:
	movl	-116(%rbp), %eax                # 4-byte Reload
	movl	%eax, -4(%rbp)
	jmp	.LBB2_156
.LBB2_54:
	cmpl	$5, -48(%rbp)
	jne	.LBB2_59
# %bb.55:
	movl	-28(%rbp), %edi
	callq	imm_z
	movl	%eax, -52(%rbp)
	movl	-24(%rbp), %eax
	addl	-52(%rbp), %eax
	cmpl	-20(%rbp), %eax
	ja	.LBB2_57
# %bb.56:
	movl	-24(%rbp), %eax
	addl	-52(%rbp), %eax
	movl	%eax, -120(%rbp)                # 4-byte Spill
	jmp	.LBB2_58
.LBB2_57:
	xorl	%eax, %eax
	movl	%eax, -120(%rbp)                # 4-byte Spill
	jmp	.LBB2_58
.LBB2_58:
	movl	-120(%rbp), %eax                # 4-byte Reload
	movl	%eax, -4(%rbp)
	jmp	.LBB2_156
.LBB2_59:
	movl	$1, -56(%rbp)
	movq	-16(%rbp), %rdi
	movl	-24(%rbp), %eax
                                        # kill: def $rax killed $eax
	addq	%rax, %rdi
	movl	-20(%rbp), %esi
	subl	-24(%rbp), %esi
	leaq	-56(%rbp), %rdx
	callq	modrm_bytes
	movl	%eax, -60(%rbp)
	cmpl	$0, -56(%rbp)
	je	.LBB2_61
# %bb.60:
	movl	-24(%rbp), %eax
	addl	-60(%rbp), %eax
	movl	%eax, -124(%rbp)                # 4-byte Spill
	jmp	.LBB2_62
.LBB2_61:
	xorl	%eax, %eax
	movl	%eax, -124(%rbp)                # 4-byte Spill
	jmp	.LBB2_62
.LBB2_62:
	movl	-124(%rbp), %eax                # 4-byte Reload
	movl	%eax, -4(%rbp)
	jmp	.LBB2_156
.LBB2_63:
	movzbl	-34(%rbp), %eax
	cmpl	$136, %eax
	jl	.LBB2_65
# %bb.64:
	movzbl	-34(%rbp), %eax
	cmpl	$139, %eax
	jle	.LBB2_75
.LBB2_65:
	movzbl	-34(%rbp), %eax
	cmpl	$141, %eax
	je	.LBB2_75
# %bb.66:
	movzbl	-34(%rbp), %eax
	cmpl	$132, %eax
	je	.LBB2_75
# %bb.67:
	movzbl	-34(%rbp), %eax
	cmpl	$133, %eax
	je	.LBB2_75
# %bb.68:
	movzbl	-34(%rbp), %eax
	cmpl	$99, %eax
	je	.LBB2_75
# %bb.69:
	movzbl	-34(%rbp), %eax
	cmpl	$255, %eax
	je	.LBB2_75
# %bb.70:
	movzbl	-34(%rbp), %eax
	cmpl	$208, %eax
	jl	.LBB2_72
# %bb.71:
	movzbl	-34(%rbp), %eax
	cmpl	$211, %eax
	jle	.LBB2_75
.LBB2_72:
	movzbl	-34(%rbp), %eax
	cmpl	$134, %eax
	je	.LBB2_75
# %bb.73:
	movzbl	-34(%rbp), %eax
	cmpl	$135, %eax
	je	.LBB2_75
# %bb.74:
	movzbl	-34(%rbp), %eax
	cmpl	$143, %eax
	jne	.LBB2_79
.LBB2_75:
	movl	$1, -64(%rbp)
	movq	-16(%rbp), %rdi
	movl	-24(%rbp), %eax
                                        # kill: def $rax killed $eax
	addq	%rax, %rdi
	movl	-20(%rbp), %esi
	subl	-24(%rbp), %esi
	leaq	-64(%rbp), %rdx
	callq	modrm_bytes
	movl	%eax, -68(%rbp)
	cmpl	$0, -64(%rbp)
	je	.LBB2_77
# %bb.76:
	movl	-24(%rbp), %eax
	addl	-68(%rbp), %eax
	movl	%eax, -128(%rbp)                # 4-byte Spill
	jmp	.LBB2_78
.LBB2_77:
	xorl	%eax, %eax
	movl	%eax, -128(%rbp)                # 4-byte Spill
	jmp	.LBB2_78
.LBB2_78:
	movl	-128(%rbp), %eax                # 4-byte Reload
	movl	%eax, -4(%rbp)
	jmp	.LBB2_156
.LBB2_79:
	movzbl	-34(%rbp), %eax
	cmpl	$128, %eax
	je	.LBB2_85
# %bb.80:
	movzbl	-34(%rbp), %eax
	cmpl	$131, %eax
	je	.LBB2_85
# %bb.81:
	movzbl	-34(%rbp), %eax
	cmpl	$192, %eax
	je	.LBB2_85
# %bb.82:
	movzbl	-34(%rbp), %eax
	cmpl	$193, %eax
	je	.LBB2_85
# %bb.83:
	movzbl	-34(%rbp), %eax
	cmpl	$198, %eax
	je	.LBB2_85
# %bb.84:
	movzbl	-34(%rbp), %eax
	cmpl	$107, %eax
	jne	.LBB2_91
.LBB2_85:
	movl	$1, -72(%rbp)
	movq	-16(%rbp), %rdi
	movl	-24(%rbp), %eax
                                        # kill: def $rax killed $eax
	addq	%rax, %rdi
	movl	-20(%rbp), %esi
	subl	-24(%rbp), %esi
	leaq	-72(%rbp), %rdx
	callq	modrm_bytes
	movl	%eax, -76(%rbp)
	cmpl	$0, -72(%rbp)
	jne	.LBB2_87
# %bb.86:
	movl	$0, -4(%rbp)
	jmp	.LBB2_156
.LBB2_87:
	movl	-24(%rbp), %eax
	addl	-76(%rbp), %eax
	addl	$1, %eax
	cmpl	-20(%rbp), %eax
	ja	.LBB2_89
# %bb.88:
	movl	-24(%rbp), %eax
	addl	-76(%rbp), %eax
	addl	$1, %eax
	movl	%eax, -132(%rbp)                # 4-byte Spill
	jmp	.LBB2_90
.LBB2_89:
	xorl	%eax, %eax
	movl	%eax, -132(%rbp)                # 4-byte Spill
	jmp	.LBB2_90
.LBB2_90:
	movl	-132(%rbp), %eax                # 4-byte Reload
	movl	%eax, -4(%rbp)
	jmp	.LBB2_156
.LBB2_91:
	movzbl	-34(%rbp), %eax
	cmpl	$129, %eax
	je	.LBB2_94
# %bb.92:
	movzbl	-34(%rbp), %eax
	cmpl	$199, %eax
	je	.LBB2_94
# %bb.93:
	movzbl	-34(%rbp), %eax
	cmpl	$105, %eax
	jne	.LBB2_100
.LBB2_94:
	movl	$1, -80(%rbp)
	movq	-16(%rbp), %rdi
	movl	-24(%rbp), %eax
                                        # kill: def $rax killed $eax
	addq	%rax, %rdi
	movl	-20(%rbp), %esi
	subl	-24(%rbp), %esi
	leaq	-80(%rbp), %rdx
	callq	modrm_bytes
	movl	%eax, -84(%rbp)
	cmpl	$0, -80(%rbp)
	jne	.LBB2_96
# %bb.95:
	movl	$0, -4(%rbp)
	jmp	.LBB2_156
.LBB2_96:
	movl	-28(%rbp), %edi
	callq	imm_z
	movl	%eax, -88(%rbp)
	movl	-24(%rbp), %eax
	addl	-84(%rbp), %eax
	addl	-88(%rbp), %eax
	cmpl	-20(%rbp), %eax
	ja	.LBB2_98
# %bb.97:
	movl	-24(%rbp), %eax
	addl	-84(%rbp), %eax
	addl	-88(%rbp), %eax
	movl	%eax, -136(%rbp)                # 4-byte Spill
	jmp	.LBB2_99
.LBB2_98:
	xorl	%eax, %eax
	movl	%eax, -136(%rbp)                # 4-byte Spill
	jmp	.LBB2_99
.LBB2_99:
	movl	-136(%rbp), %eax                # 4-byte Reload
	movl	%eax, -4(%rbp)
	jmp	.LBB2_156
.LBB2_100:
	movzbl	-34(%rbp), %eax
	cmpl	$246, %eax
	je	.LBB2_102
# %bb.101:
	movzbl	-34(%rbp), %eax
	cmpl	$247, %eax
	jne	.LBB2_113
.LBB2_102:
	movl	$1, -92(%rbp)
	movq	-16(%rbp), %rdi
	movl	-24(%rbp), %eax
                                        # kill: def $rax killed $eax
	addq	%rax, %rdi
	movl	-20(%rbp), %esi
	subl	-24(%rbp), %esi
	leaq	-92(%rbp), %rdx
	callq	modrm_bytes
	movl	%eax, -96(%rbp)
	cmpl	$0, -92(%rbp)
	jne	.LBB2_104
# %bb.103:
	movl	$0, -4(%rbp)
	jmp	.LBB2_156
.LBB2_104:
	movq	-16(%rbp), %rax
	movl	-24(%rbp), %ecx
                                        # kill: def $rcx killed $ecx
	movzbl	(%rax,%rcx), %eax
	sarl	$3, %eax
	andl	$7, %eax
	movl	%eax, -100(%rbp)
	movl	$0, -104(%rbp)
	cmpl	$1, -100(%rbp)
	jg	.LBB2_109
# %bb.105:
	movzbl	-34(%rbp), %eax
	cmpl	$246, %eax
	jne	.LBB2_107
# %bb.106:
	movl	$1, %eax
	movl	%eax, -140(%rbp)                # 4-byte Spill
	jmp	.LBB2_108
.LBB2_107:
	movl	-28(%rbp), %edi
	callq	imm_z
	movl	%eax, -140(%rbp)                # 4-byte Spill
.LBB2_108:
	movl	-140(%rbp), %eax                # 4-byte Reload
	movl	%eax, -104(%rbp)
.LBB2_109:
	movl	-24(%rbp), %eax
	addl	-96(%rbp), %eax
	addl	-104(%rbp), %eax
	cmpl	-20(%rbp), %eax
	ja	.LBB2_111
# %bb.110:
	movl	-24(%rbp), %eax
	addl	-96(%rbp), %eax
	addl	-104(%rbp), %eax
	movl	%eax, -144(%rbp)                # 4-byte Spill
	jmp	.LBB2_112
.LBB2_111:
	xorl	%eax, %eax
	movl	%eax, -144(%rbp)                # 4-byte Spill
	jmp	.LBB2_112
.LBB2_112:
	movl	-144(%rbp), %eax                # 4-byte Reload
	movl	%eax, -4(%rbp)
	jmp	.LBB2_156
.LBB2_113:
	movzbl	-34(%rbp), %eax
	cmpl	$112, %eax
	jl	.LBB2_119
# %bb.114:
	movzbl	-34(%rbp), %eax
	cmpl	$127, %eax
	jg	.LBB2_119
# %bb.115:
	movl	-24(%rbp), %eax
	addl	$1, %eax
	cmpl	-20(%rbp), %eax
	ja	.LBB2_117
# %bb.116:
	movl	-24(%rbp), %eax
	addl	$1, %eax
	movl	%eax, -148(%rbp)                # 4-byte Spill
	jmp	.LBB2_118
.LBB2_117:
	xorl	%eax, %eax
	movl	%eax, -148(%rbp)                # 4-byte Spill
	jmp	.LBB2_118
.LBB2_118:
	movl	-148(%rbp), %eax                # 4-byte Reload
	movl	%eax, -4(%rbp)
	jmp	.LBB2_156
.LBB2_119:
	movzbl	-34(%rbp), %eax
	cmpl	$235, %eax
	jne	.LBB2_124
# %bb.120:
	movl	-24(%rbp), %eax
	addl	$1, %eax
	cmpl	-20(%rbp), %eax
	ja	.LBB2_122
# %bb.121:
	movl	-24(%rbp), %eax
	addl	$1, %eax
	movl	%eax, -152(%rbp)                # 4-byte Spill
	jmp	.LBB2_123
.LBB2_122:
	xorl	%eax, %eax
	movl	%eax, -152(%rbp)                # 4-byte Spill
	jmp	.LBB2_123
.LBB2_123:
	movl	-152(%rbp), %eax                # 4-byte Reload
	movl	%eax, -4(%rbp)
	jmp	.LBB2_156
.LBB2_124:
	movzbl	-34(%rbp), %eax
	cmpl	$232, %eax
	je	.LBB2_126
# %bb.125:
	movzbl	-34(%rbp), %eax
	cmpl	$233, %eax
	jne	.LBB2_130
.LBB2_126:
	movl	-24(%rbp), %eax
	addl	$4, %eax
	cmpl	-20(%rbp), %eax
	ja	.LBB2_128
# %bb.127:
	movl	-24(%rbp), %eax
	addl	$4, %eax
	movl	%eax, -156(%rbp)                # 4-byte Spill
	jmp	.LBB2_129
.LBB2_128:
	xorl	%eax, %eax
	movl	%eax, -156(%rbp)                # 4-byte Spill
	jmp	.LBB2_129
.LBB2_129:
	movl	-156(%rbp), %eax                # 4-byte Reload
	movl	%eax, -4(%rbp)
	jmp	.LBB2_156
.LBB2_130:
	movzbl	-34(%rbp), %eax
	cmpl	$176, %eax
	jl	.LBB2_136
# %bb.131:
	movzbl	-34(%rbp), %eax
	cmpl	$183, %eax
	jg	.LBB2_136
# %bb.132:
	movl	-24(%rbp), %eax
	addl	$1, %eax
	cmpl	-20(%rbp), %eax
	ja	.LBB2_134
# %bb.133:
	movl	-24(%rbp), %eax
	addl	$1, %eax
	movl	%eax, -160(%rbp)                # 4-byte Spill
	jmp	.LBB2_135
.LBB2_134:
	xorl	%eax, %eax
	movl	%eax, -160(%rbp)                # 4-byte Spill
	jmp	.LBB2_135
.LBB2_135:
	movl	-160(%rbp), %eax                # 4-byte Reload
	movl	%eax, -4(%rbp)
	jmp	.LBB2_156
.LBB2_136:
	movzbl	-34(%rbp), %eax
	cmpl	$184, %eax
	jl	.LBB2_145
# %bb.137:
	movzbl	-34(%rbp), %eax
	cmpl	$191, %eax
	jg	.LBB2_145
# %bb.138:
	cmpl	$0, -32(%rbp)
	je	.LBB2_140
# %bb.139:
	movl	$8, %eax
	movl	%eax, -164(%rbp)                # 4-byte Spill
	jmp	.LBB2_141
.LBB2_140:
	movl	-28(%rbp), %edi
	callq	imm_z
	movl	%eax, -164(%rbp)                # 4-byte Spill
.LBB2_141:
	movl	-164(%rbp), %eax                # 4-byte Reload
	movl	%eax, -108(%rbp)
	movl	-24(%rbp), %eax
	addl	-108(%rbp), %eax
	cmpl	-20(%rbp), %eax
	ja	.LBB2_143
# %bb.142:
	movl	-24(%rbp), %eax
	addl	-108(%rbp), %eax
	movl	%eax, -168(%rbp)                # 4-byte Spill
	jmp	.LBB2_144
.LBB2_143:
	xorl	%eax, %eax
	movl	%eax, -168(%rbp)                # 4-byte Spill
	jmp	.LBB2_144
.LBB2_144:
	movl	-168(%rbp), %eax                # 4-byte Reload
	movl	%eax, -4(%rbp)
	jmp	.LBB2_156
.LBB2_145:
	movzbl	-34(%rbp), %eax
	cmpl	$104, %eax
	jne	.LBB2_150
# %bb.146:
	movl	-24(%rbp), %eax
	addl	$4, %eax
	cmpl	-20(%rbp), %eax
	ja	.LBB2_148
# %bb.147:
	movl	-24(%rbp), %eax
	addl	$4, %eax
	movl	%eax, -172(%rbp)                # 4-byte Spill
	jmp	.LBB2_149
.LBB2_148:
	xorl	%eax, %eax
	movl	%eax, -172(%rbp)                # 4-byte Spill
	jmp	.LBB2_149
.LBB2_149:
	movl	-172(%rbp), %eax                # 4-byte Reload
	movl	%eax, -4(%rbp)
	jmp	.LBB2_156
.LBB2_150:
	movzbl	-34(%rbp), %eax
	cmpl	$106, %eax
	jne	.LBB2_155
# %bb.151:
	movl	-24(%rbp), %eax
	addl	$1, %eax
	cmpl	-20(%rbp), %eax
	ja	.LBB2_153
# %bb.152:
	movl	-24(%rbp), %eax
	addl	$1, %eax
	movl	%eax, -176(%rbp)                # 4-byte Spill
	jmp	.LBB2_154
.LBB2_153:
	xorl	%eax, %eax
	movl	%eax, -176(%rbp)                # 4-byte Spill
	jmp	.LBB2_154
.LBB2_154:
	movl	-176(%rbp), %eax                # 4-byte Reload
	movl	%eax, -4(%rbp)
	jmp	.LBB2_156
.LBB2_155:
	movl	$0, -4(%rbp)
.LBB2_156:
	movl	-4(%rbp), %eax
	addq	$176, %rsp
	popq	%rbp
	retq
.Lfunc_end2:
	.size	x86_insn_len, .Lfunc_end2-x86_insn_len
                                        # -- End function
	.p2align	4                               # -- Begin function imm_z
	.type	imm_z,@function
imm_z:                                  # @imm_z
# %bb.0:
	pushq	%rbp
	movq	%rsp, %rbp
	movl	%edi, -4(%rbp)
	movl	-4(%rbp), %edx
	movl	$4, %eax
	movl	$2, %ecx
	cmpl	$0, %edx
	cmovnel	%ecx, %eax
	popq	%rbp
	retq
.Lfunc_end3:
	.size	imm_z, .Lfunc_end3-imm_z
                                        # -- End function
	.globl	demo_run                        # -- Begin function demo_run
	.p2align	4
	.type	demo_run,@function
demo_run:                               # @demo_run
# %bb.0:
	pushq	%rbp
	movq	%rsp, %rbp
	subq	$48, %rsp
	leaq	demo_run.tab(%rip), %rdi
	movl	$4, %esi
	movl	$4240, %edx                     # imm = 0x1090
	callq	sym_by_addr
	movl	%eax, -4(%rbp)
	cmpl	$0, -4(%rbp)
	jl	.LBB4_2
# %bb.1:
	movslq	-4(%rbp), %rax
	leaq	demo_run.tab(%rip), %rcx
	shlq	$4, %rax
	addq	%rax, %rcx
	movl	$4240, %eax                     # imm = 0x1090
	subq	(%rcx), %rax
	movq	%rax, -40(%rbp)                 # 8-byte Spill
	jmp	.LBB4_3
.LBB4_2:
	xorl	%eax, %eax
                                        # kill: def $rax killed $eax
	movq	%rax, -40(%rbp)                 # 8-byte Spill
	jmp	.LBB4_3
.LBB4_3:
	movq	-40(%rbp), %rax                 # 8-byte Reload
	movq	%rax, -16(%rbp)
	movl	$0, -20(%rbp)
	movl	$0, -24(%rbp)
.LBB4_4:                                # =>This Inner Loop Header: Depth=1
	movl	-24(%rbp), %eax
                                        # kill: def $rax killed $eax
	cmpq	$9, %rax
	jae	.LBB4_8
# %bb.5:                                #   in Loop: Header=BB4_4 Depth=1
	movl	-24(%rbp), %eax
                                        # kill: def $rax killed $eax
	leaq	demo_run.code(%rip), %rdi
	addq	%rax, %rdi
	movl	-24(%rbp), %eax
	movl	%eax, %ecx
	movl	$9, %eax
	subq	%rcx, %rax
	movl	%eax, %esi
	callq	x86_insn_len
	movl	%eax, -28(%rbp)
	cmpl	$0, -28(%rbp)
	jne	.LBB4_7
# %bb.6:
	jmp	.LBB4_8
.LBB4_7:                                #   in Loop: Header=BB4_4 Depth=1
	movl	-28(%rbp), %eax
	addl	-20(%rbp), %eax
	movl	%eax, -20(%rbp)
	movl	-28(%rbp), %eax
	addl	-24(%rbp), %eax
	movl	%eax, -24(%rbp)
	jmp	.LBB4_4
.LBB4_8:
	movq	-16(%rbp), %rax
	movl	-20(%rbp), %ecx
                                        # kill: def $rcx killed $ecx
	addq	%rcx, %rax
                                        # kill: def $eax killed $eax killed $rax
	addq	$48, %rsp
	popq	%rbp
	retq
.Lfunc_end4:
	.size	demo_run, .Lfunc_end4-demo_run
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
	.addrsig_sym sym_by_addr
	.addrsig_sym modrm_bytes
	.addrsig_sym x86_insn_len
	.addrsig_sym imm_z
	.addrsig_sym demo_run.tab
	.addrsig_sym demo_run.code
