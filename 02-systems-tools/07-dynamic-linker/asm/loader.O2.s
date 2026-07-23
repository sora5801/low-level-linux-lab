	.file	"loader.c"
                                        # Start of file scope inline assembly
	.text
	.globl	_start
	.type	_start,@function
_start:
	xorl	%ebp, %ebp
	movq	%rsp, %rdi
	andq	$-16, %rsp
	callq	loader_main
	hlt

                                        # End of file scope inline assembly
	.globl	memset                          # -- Begin function memset
	.p2align	4
	.type	memset,@function
memset:                                 # @memset
# %bb.0:
	movq	%rdi, %rax
	testq	%rdx, %rdx
	je	.LBB0_8
# %bb.1:
	movq	%rdx, %r8
	movq	%rax, %rcx
	movq	%rdx, %rdi
	andq	$7, %r8
	je	.LBB0_5
# %bb.2:
	xorl	%r9d, %r9d
	.p2align	4
.LBB0_3:                                # =>This Inner Loop Header: Depth=1
	movb	%sil, (%rax,%r9)
	incq	%r9
	cmpq	%r9, %r8
	jne	.LBB0_3
# %bb.4:
	leaq	(%rax,%r9), %rcx
	movq	%rdx, %rdi
	subq	%r9, %rdi
.LBB0_5:
	cmpq	$8, %rdx
	jb	.LBB0_8
# %bb.6:
	xorl	%edx, %edx
	.p2align	4
.LBB0_7:                                # =>This Inner Loop Header: Depth=1
	movb	%sil, (%rcx,%rdx)
	movb	%sil, 1(%rcx,%rdx)
	movb	%sil, 2(%rcx,%rdx)
	movb	%sil, 3(%rcx,%rdx)
	movb	%sil, 4(%rcx,%rdx)
	movb	%sil, 5(%rcx,%rdx)
	movb	%sil, 6(%rcx,%rdx)
	movb	%sil, 7(%rcx,%rdx)
	addq	$8, %rdx
	cmpq	%rdx, %rdi
	jne	.LBB0_7
.LBB0_8:
	retq
.Lfunc_end0:
	.size	memset, .Lfunc_end0-memset
                                        # -- End function
	.globl	memcpy                          # -- Begin function memcpy
	.p2align	4
	.type	memcpy,@function
memcpy:                                 # @memcpy
# %bb.0:
	movq	%rdi, %rax
	testq	%rdx, %rdx
	je	.LBB1_9
# %bb.1:
	movq	%rdx, %rcx
	andq	$7, %rcx
	je	.LBB1_2
# %bb.3:
	xorl	%r8d, %r8d
	.p2align	4
.LBB1_4:                                # =>This Inner Loop Header: Depth=1
	movzbl	(%rsi,%r8), %edi
	movb	%dil, (%rax,%r8)
	incq	%r8
	cmpq	%r8, %rcx
	jne	.LBB1_4
# %bb.5:
	addq	%r8, %rsi
	leaq	(%rax,%r8), %rcx
	movq	%rdx, %rdi
	subq	%r8, %rdi
	cmpq	$8, %rdx
	jae	.LBB1_7
	jmp	.LBB1_9
.LBB1_2:
	movq	%rax, %rcx
	movq	%rdx, %rdi
	cmpq	$8, %rdx
	jb	.LBB1_9
.LBB1_7:
	xorl	%edx, %edx
	.p2align	4
.LBB1_8:                                # =>This Inner Loop Header: Depth=1
	movzbl	(%rsi,%rdx), %r8d
	movb	%r8b, (%rcx,%rdx)
	movzbl	1(%rsi,%rdx), %r8d
	movb	%r8b, 1(%rcx,%rdx)
	movzbl	2(%rsi,%rdx), %r8d
	movb	%r8b, 2(%rcx,%rdx)
	movzbl	3(%rsi,%rdx), %r8d
	movb	%r8b, 3(%rcx,%rdx)
	movzbl	4(%rsi,%rdx), %r8d
	movb	%r8b, 4(%rcx,%rdx)
	movzbl	5(%rsi,%rdx), %r8d
	movb	%r8b, 5(%rcx,%rdx)
	movzbl	6(%rsi,%rdx), %r8d
	movb	%r8b, 6(%rcx,%rdx)
	movzbl	7(%rsi,%rdx), %r8d
	movb	%r8b, 7(%rcx,%rdx)
	addq	$8, %rdx
	cmpq	%rdx, %rdi
	jne	.LBB1_8
.LBB1_9:
	retq
.Lfunc_end1:
	.size	memcpy, .Lfunc_end1-memcpy
                                        # -- End function
	.globl	loader_main                     # -- Begin function loader_main
	.p2align	4
	.type	loader_main,@function
loader_main:                            # @loader_main
# %bb.0:
	pushq	%rbp
	pushq	%r15
	pushq	%r14
	pushq	%r13
	pushq	%r12
	pushq	%rbx
	subq	$56, %rsp
	movq	(%rdi), %r9
	leaq	8(%rdi), %r14
	movq	%rdi, %r11
	leaq	(%rdi,%r9,8), %r10
	addq	$16, %r10
	movq	%r10, (%rsp)                    # 8-byte Spill
	.p2align	4
.LBB2_1:                                # =>This Inner Loop Header: Depth=1
	cmpq	$0, (%r10)
	leaq	8(%r10), %r10
	jne	.LBB2_1
# %bb.2:
	movq	(%rsp), %rax                    # 8-byte Reload
	movq	(%rax), %rax
	testq	%rax, %rax
	jne	.LBB2_3
.LBB2_31:
	cmpq	$2, %r9
	jge	.LBB2_32
# %bb.162:
	leaq	.L.str.2(%rip), %rdi
	callq	die
.LBB2_3:
	leaq	.L.str(%rip), %rcx
	movq	(%rsp), %rdx                    # 8-byte Reload
	jmp	.LBB2_4
	.p2align	4
.LBB2_30:                               #   in Loop: Header=BB2_4 Depth=1
	movq	8(%rdx), %rax
	addq	$8, %rdx
	testq	%rax, %rax
	je	.LBB2_31
.LBB2_4:                                # =>This Loop Header: Depth=1
                                        #     Child Loop BB2_11 Depth 2
	cmpb	$76, (%rax)
	jne	.LBB2_30
# %bb.5:                                #   in Loop: Header=BB2_4 Depth=1
	cmpb	$68, 1(%rax)
	jne	.LBB2_30
# %bb.6:                                #   in Loop: Header=BB2_4 Depth=1
	cmpb	$76, 2(%rax)
	jne	.LBB2_30
# %bb.7:                                #   in Loop: Header=BB2_4 Depth=1
	cmpb	$65, 3(%rax)
	jne	.LBB2_30
# %bb.8:                                #   in Loop: Header=BB2_4 Depth=1
	cmpb	$66, 4(%rax)
	jne	.LBB2_30
# %bb.9:                                #   in Loop: Header=BB2_4 Depth=1
	cmpb	$95, 5(%rax)
	jne	.LBB2_30
# %bb.10:                               #   in Loop: Header=BB2_4 Depth=1
	movb	$76, %sil
	xorl	%edi, %edi
	.p2align	4
.LBB2_11:                               #   Parent Loop BB2_4 Depth=1
                                        # =>  This Inner Loop Header: Depth=2
	movzbl	(%rdi,%rcx), %r8d
	cmpb	%r8b, %sil
	jne	.LBB2_14
# %bb.12:                               #   in Loop: Header=BB2_11 Depth=2
	movzbl	1(%rax,%rdi), %esi
	incq	%rdi
	testb	%sil, %sil
	jne	.LBB2_11
# %bb.13:                               #   in Loop: Header=BB2_4 Depth=1
	movzbl	(%rdi,%rcx), %r8d
	xorl	%esi, %esi
.LBB2_14:                               #   in Loop: Header=BB2_4 Depth=1
	cmpb	%r8b, %sil
	jne	.LBB2_16
# %bb.15:                               #   in Loop: Header=BB2_4 Depth=1
	movb	$1, g_debug(%rip)
.LBB2_16:                               #   in Loop: Header=BB2_4 Depth=1
	cmpb	$76, 6(%rax)
	jne	.LBB2_30
# %bb.17:                               #   in Loop: Header=BB2_4 Depth=1
	cmpb	$73, 7(%rax)
	jne	.LBB2_30
# %bb.18:                               #   in Loop: Header=BB2_4 Depth=1
	cmpb	$66, 8(%rax)
	jne	.LBB2_30
# %bb.19:                               #   in Loop: Header=BB2_4 Depth=1
	cmpb	$82, 9(%rax)
	jne	.LBB2_30
# %bb.20:                               #   in Loop: Header=BB2_4 Depth=1
	cmpb	$65, 10(%rax)
	jne	.LBB2_30
# %bb.21:                               #   in Loop: Header=BB2_4 Depth=1
	cmpb	$82, 11(%rax)
	jne	.LBB2_30
# %bb.22:                               #   in Loop: Header=BB2_4 Depth=1
	cmpb	$89, 12(%rax)
	jne	.LBB2_30
# %bb.23:                               #   in Loop: Header=BB2_4 Depth=1
	cmpb	$95, 13(%rax)
	jne	.LBB2_30
# %bb.24:                               #   in Loop: Header=BB2_4 Depth=1
	cmpb	$80, 14(%rax)
	jne	.LBB2_30
# %bb.25:                               #   in Loop: Header=BB2_4 Depth=1
	cmpb	$65, 15(%rax)
	jne	.LBB2_30
# %bb.26:                               #   in Loop: Header=BB2_4 Depth=1
	cmpb	$84, 16(%rax)
	jne	.LBB2_30
# %bb.27:                               #   in Loop: Header=BB2_4 Depth=1
	cmpb	$72, 17(%rax)
	jne	.LBB2_30
# %bb.28:                               #   in Loop: Header=BB2_4 Depth=1
	cmpb	$61, 18(%rax)
	jne	.LBB2_30
# %bb.29:                               #   in Loop: Header=BB2_4 Depth=1
	addq	$19, %rax
	movq	%rax, g_env_libpath(%rip)
	jmp	.LBB2_30
.LBB2_32:
	movq	%r10, 48(%rsp)                  # 8-byte Spill
	movq	%r9, 32(%rsp)                   # 8-byte Spill
	movq	16(%r11), %rbx
	addq	$16, %r11
	movq	%r11, 40(%rsp)                  # 8-byte Spill
	movq	$-1, %rdx
	xorl	%eax, %eax
	jmp	.LBB2_33
	.p2align	4
.LBB2_43:                               #   in Loop: Header=BB2_33 Depth=1
	movq	%rax, %rdx
.LBB2_44:                               #   in Loop: Header=BB2_33 Depth=1
	incq	%rax
.LBB2_33:                               # =>This Inner Loop Header: Depth=1
	movzbl	(%rbx,%rax), %ecx
	cmpl	$47, %ecx
	je	.LBB2_43
# %bb.34:                               #   in Loop: Header=BB2_33 Depth=1
	testl	%ecx, %ecx
	jne	.LBB2_44
# %bb.35:
	testq	%rdx, %rdx
	jle	.LBB2_55
# %bb.36:
	cmpq	$4095, %rdx                     # imm = 0xFFF
	movl	$4095, %eax                     # imm = 0xFFF
	cmovbq	%rdx, %rax
	cmpq	$4, %rdx
	setae	%sil
	leaq	g_prog_dir(%rip), %rcx
	movq	%rcx, %rdi
	subq	%rbx, %rdi
	cmpq	$32, %rdi
	setae	%dil
	testb	%dil, %sil
	jne	.LBB2_45
# %bb.37:
	xorl	%esi, %esi
	jmp	.LBB2_38
.LBB2_45:
	cmpq	$32, %rdx
	jae	.LBB2_47
# %bb.46:
	xorl	%esi, %esi
	jmp	.LBB2_51
.LBB2_47:
	movl	%eax, %esi
	andl	$4064, %esi                     # imm = 0xFE0
	xorl	%edi, %edi
	.p2align	4
.LBB2_48:                               # =>This Inner Loop Header: Depth=1
	movups	(%rbx,%rdi), %xmm0
	movups	16(%rbx,%rdi), %xmm1
	movaps	%xmm0, (%rdi,%rcx)
	movaps	%xmm1, 16(%rdi,%rcx)
	addq	$32, %rdi
	cmpq	%rdi, %rsi
	jne	.LBB2_48
# %bb.49:
	cmpq	%rsi, %rdx
	je	.LBB2_54
# %bb.50:
	testb	$28, %al
	je	.LBB2_38
.LBB2_51:
	movq	%rsi, %rdi
	movl	%eax, %esi
	andl	$4092, %esi                     # imm = 0xFFC
	.p2align	4
.LBB2_52:                               # =>This Inner Loop Header: Depth=1
	movl	(%rbx,%rdi), %r8d
	movl	%r8d, (%rdi,%rcx)
	addq	$4, %rdi
	cmpq	%rdi, %rsi
	jne	.LBB2_52
# %bb.53:
	cmpq	%rsi, %rdx
	je	.LBB2_54
.LBB2_38:
	movq	%rax, %rdi
	movq	%rsi, %rdx
	andq	$3, %rdi
	je	.LBB2_41
# %bb.39:
	movq	%rsi, %rdx
	.p2align	4
.LBB2_40:                               # =>This Inner Loop Header: Depth=1
	movzbl	(%rbx,%rdx), %r8d
	movb	%r8b, (%rdx,%rcx)
	incq	%rdx
	decq	%rdi
	jne	.LBB2_40
.LBB2_41:
	subq	%rax, %rsi
	cmpq	$-4, %rsi
	ja	.LBB2_54
	.p2align	4
.LBB2_42:                               # =>This Inner Loop Header: Depth=1
	movzbl	(%rbx,%rdx), %esi
	movb	%sil, (%rdx,%rcx)
	movzbl	1(%rbx,%rdx), %esi
	movb	%sil, 1(%rdx,%rcx)
	movzbl	2(%rbx,%rdx), %esi
	movb	%sil, 2(%rdx,%rcx)
	movzbl	3(%rbx,%rdx), %esi
	movb	%sil, 3(%rdx,%rcx)
	addq	$4, %rdx
	cmpq	%rdx, %rax
	jne	.LBB2_42
.LBB2_54:
	movb	$0, (%rax,%rcx)
.LBB2_55:
	cmpb	$1, g_debug(%rip)
	jne	.LBB2_57
# %bb.56:
	leaq	.L.str.7(%rip), %rsi
	movl	$1, %eax
	movl	$2, %edi
	movl	$5, %edx
	xorl	%r10d, %r10d
	xorl	%r8d, %r8d
	xorl	%r9d, %r9d
	#APP
	syscall
	#NO_APP
	leaq	.L.str.3(%rip), %rsi
	movl	$1, %eax
	movl	$20, %edx
	xorl	%r10d, %r10d
	xorl	%r8d, %r8d
	xorl	%r9d, %r9d
	#APP
	syscall
	#NO_APP
	leaq	.L.str.5(%rip), %rsi
	movl	$1, %eax
	movl	$1, %edx
	xorl	%r10d, %r10d
	xorl	%r8d, %r8d
	xorl	%r9d, %r9d
	#APP
	syscall
	#NO_APP
.LBB2_57:
	movq	%rbx, %rdi
	callq	load_path
	movq	%rax, %rdi
	movl	g_nobjs(%rip), %eax
	testl	%eax, %eax
	jle	.LBB2_158
# %bb.58:
	movq	%rdi, 8(%rsp)                   # 8-byte Spill
	movq	%r14, 16(%rsp)                  # 8-byte Spill
	xorl	%edx, %edx
	leaq	g_objs(%rip), %r15
	leaq	g_prog_dir(%rip), %r14
	jmp	.LBB2_59
	.p2align	4
.LBB2_81:                               #   in Loop: Header=BB2_59 Depth=1
	movl	g_nobjs(%rip), %eax
	movq	24(%rsp), %rdx                  # 8-byte Reload
.LBB2_82:                               #   in Loop: Header=BB2_59 Depth=1
	incq	%rdx
	movslq	%eax, %rcx
	cmpq	%rcx, %rdx
	jge	.LBB2_70
.LBB2_59:                               # =>This Loop Header: Depth=1
                                        #     Child Loop BB2_61 Depth 2
                                        #       Child Loop BB2_63 Depth 3
                                        #       Child Loop BB2_67 Depth 3
                                        #         Child Loop BB2_84 Depth 4
                                        #         Child Loop BB2_92 Depth 4
                                        #       Child Loop BB2_98 Depth 3
                                        #       Child Loop BB2_107 Depth 3
                                        #         Child Loop BB2_109 Depth 4
                                        #       Child Loop BB2_116 Depth 3
                                        #         Child Loop BB2_118 Depth 4
                                        #       Child Loop BB2_124 Depth 3
	movq	%rdx, %rcx
	shlq	$8, %rcx
	leaq	(%rcx,%rdx,8), %rbx
	cmpl	$0, 248(%r15,%rbx)
	jle	.LBB2_82
# %bb.60:                               #   in Loop: Header=BB2_59 Depth=1
	movq	%rdx, 24(%rsp)                  # 8-byte Spill
	addq	%r15, %rbx
	xorl	%ebp, %ebp
	jmp	.LBB2_61
	.p2align	4
.LBB2_101:                              #   in Loop: Header=BB2_61 Depth=2
	movq	%r13, %rdi
	callq	load_path
.LBB2_127:                              #   in Loop: Header=BB2_61 Depth=2
	incq	%rbp
	movslq	248(%rbx), %rax
	cmpq	%rax, %rbp
	jge	.LBB2_81
.LBB2_61:                               #   Parent Loop BB2_59 Depth=1
                                        # =>  This Loop Header: Depth=2
                                        #       Child Loop BB2_63 Depth 3
                                        #       Child Loop BB2_67 Depth 3
                                        #         Child Loop BB2_84 Depth 4
                                        #         Child Loop BB2_92 Depth 4
                                        #       Child Loop BB2_98 Depth 3
                                        #       Child Loop BB2_107 Depth 3
                                        #         Child Loop BB2_109 Depth 4
                                        #       Child Loop BB2_116 Depth 3
                                        #         Child Loop BB2_118 Depth 4
                                        #       Child Loop BB2_124 Depth 3
	movl	184(%rbx,%rbp,4), %r13d
	addq	56(%rbx), %r13
	cmpb	$1, g_debug(%rip)
	jne	.LBB2_65
# %bb.62:                               #   in Loop: Header=BB2_61 Depth=2
	movl	$1, %eax
	movl	$2, %edi
	movl	$15, %edx
	leaq	.L.str.4(%rip), %rsi
	xorl	%r10d, %r10d
	xorl	%r8d, %r8d
	xorl	%r9d, %r9d
	#APP
	syscall
	#NO_APP
	movq	$-1, %rdx
	.p2align	4
.LBB2_63:                               #   Parent Loop BB2_59 Depth=1
                                        #     Parent Loop BB2_61 Depth=2
                                        # =>    This Inner Loop Header: Depth=3
	cmpb	$0, 1(%r13,%rdx)
	leaq	1(%rdx), %rdx
	jne	.LBB2_63
# %bb.64:                               #   in Loop: Header=BB2_61 Depth=2
	movl	$1, %eax
	movl	$2, %edi
	movq	%r13, %rsi
	xorl	%r10d, %r10d
	xorl	%r8d, %r8d
	xorl	%r9d, %r9d
	#APP
	syscall
	#NO_APP
	movl	$1, %eax
	movl	$1, %edx
	leaq	.L.str.5(%rip), %rsi
	xorl	%r10d, %r10d
	xorl	%r8d, %r8d
	xorl	%r9d, %r9d
	#APP
	syscall
	#NO_APP
.LBB2_65:                               #   in Loop: Header=BB2_61 Depth=2
	movslq	g_nobjs(%rip), %rax
	testq	%rax, %rax
	jle	.LBB2_97
# %bb.66:                               #   in Loop: Header=BB2_61 Depth=2
	xorl	%ecx, %ecx
	jmp	.LBB2_67
	.p2align	4
.LBB2_90:                               #   in Loop: Header=BB2_67 Depth=3
	xorl	%edx, %edx
	movq	%r13, %rsi
.LBB2_95:                               #   in Loop: Header=BB2_67 Depth=3
	cmpb	(%rsi), %dl
	je	.LBB2_127
.LBB2_96:                               #   in Loop: Header=BB2_67 Depth=3
	incq	%rcx
	cmpq	%rax, %rcx
	je	.LBB2_97
.LBB2_67:                               #   Parent Loop BB2_59 Depth=1
                                        #     Parent Loop BB2_61 Depth=2
                                        # =>    This Loop Header: Depth=3
                                        #         Child Loop BB2_84 Depth 4
                                        #         Child Loop BB2_92 Depth 4
	movq	%rcx, %rdx
	shlq	$8, %rdx
	leaq	(%rdx,%rcx,8), %rdx
	movq	8(%r15,%rdx), %r8
	testq	%r8, %r8
	je	.LBB2_88
# %bb.68:                               #   in Loop: Header=BB2_67 Depth=3
	movzbl	(%r8), %esi
	testb	%sil, %sil
	je	.LBB2_69
# %bb.83:                               #   in Loop: Header=BB2_67 Depth=3
	incq	%r8
	movq	%r13, %rdi
	.p2align	4
.LBB2_84:                               #   Parent Loop BB2_59 Depth=1
                                        #     Parent Loop BB2_61 Depth=2
                                        #       Parent Loop BB2_67 Depth=3
                                        # =>      This Inner Loop Header: Depth=4
	cmpb	(%rdi), %sil
	jne	.LBB2_87
# %bb.85:                               #   in Loop: Header=BB2_84 Depth=4
	incq	%rdi
	movzbl	(%r8), %esi
	incq	%r8
	testb	%sil, %sil
	jne	.LBB2_84
# %bb.86:                               #   in Loop: Header=BB2_67 Depth=3
	xorl	%esi, %esi
	jmp	.LBB2_87
	.p2align	4
.LBB2_69:                               #   in Loop: Header=BB2_67 Depth=3
	xorl	%esi, %esi
	movq	%r13, %rdi
.LBB2_87:                               #   in Loop: Header=BB2_67 Depth=3
	cmpb	(%rdi), %sil
	je	.LBB2_127
.LBB2_88:                               #   in Loop: Header=BB2_67 Depth=3
	addq	%r15, %rdx
	movq	(%rdx), %rdi
	testq	%rdi, %rdi
	je	.LBB2_96
# %bb.89:                               #   in Loop: Header=BB2_67 Depth=3
	movzbl	(%rdi), %edx
	testb	%dl, %dl
	je	.LBB2_90
# %bb.91:                               #   in Loop: Header=BB2_67 Depth=3
	incq	%rdi
	movq	%r13, %rsi
	.p2align	4
.LBB2_92:                               #   Parent Loop BB2_59 Depth=1
                                        #     Parent Loop BB2_61 Depth=2
                                        #       Parent Loop BB2_67 Depth=3
                                        # =>      This Inner Loop Header: Depth=4
	cmpb	(%rsi), %dl
	jne	.LBB2_95
# %bb.93:                               #   in Loop: Header=BB2_92 Depth=4
	incq	%rsi
	movzbl	(%rdi), %edx
	incq	%rdi
	testb	%dl, %dl
	jne	.LBB2_92
# %bb.94:                               #   in Loop: Header=BB2_67 Depth=3
	xorl	%edx, %edx
	jmp	.LBB2_95
	.p2align	4
.LBB2_97:                               #   in Loop: Header=BB2_61 Depth=2
	movq	$-1, %rax
	xorl	%ecx, %ecx
	jmp	.LBB2_98
	.p2align	4
.LBB2_102:                              #   in Loop: Header=BB2_98 Depth=3
	movq	%rcx, %rax
.LBB2_103:                              #   in Loop: Header=BB2_98 Depth=3
	incq	%rcx
.LBB2_98:                               #   Parent Loop BB2_59 Depth=1
                                        #     Parent Loop BB2_61 Depth=2
                                        # =>    This Inner Loop Header: Depth=3
	movzbl	(%r13,%rcx), %edx
	cmpl	$47, %edx
	je	.LBB2_102
# %bb.99:                               #   in Loop: Header=BB2_98 Depth=3
	testl	%edx, %edx
	jne	.LBB2_103
# %bb.100:                              #   in Loop: Header=BB2_61 Depth=2
	testq	%rax, %rax
	jns	.LBB2_101
# %bb.104:                              #   in Loop: Header=BB2_61 Depth=2
	movq	176(%rbx), %rdi
	testq	%rdi, %rdi
	je	.LBB2_113
# %bb.105:                              #   in Loop: Header=BB2_61 Depth=2
	movzbl	(%rdi), %eax
	testb	%al, %al
	je	.LBB2_113
	.p2align	4
.LBB2_107:                              #   Parent Loop BB2_59 Depth=1
                                        #     Parent Loop BB2_61 Depth=2
                                        # =>    This Loop Header: Depth=3
                                        #         Child Loop BB2_109 Depth 4
	xorl	%esi, %esi
	movq	%rdi, %r12
	testb	%al, %al
	je	.LBB2_111
	.p2align	4
.LBB2_109:                              #   Parent Loop BB2_59 Depth=1
                                        #     Parent Loop BB2_61 Depth=2
                                        #       Parent Loop BB2_107 Depth=3
                                        # =>      This Inner Loop Header: Depth=4
	movzbl	%al, %eax
	cmpl	$58, %eax
	je	.LBB2_111
# %bb.110:                              #   in Loop: Header=BB2_109 Depth=4
	movzbl	1(%r12), %eax
	incq	%r12
	incq	%rsi
	testb	%al, %al
	jne	.LBB2_109
.LBB2_111:                              #   in Loop: Header=BB2_107 Depth=3
	movq	%r13, %rdx
	callq	try_dir
	testq	%rax, %rax
	jne	.LBB2_127
# %bb.112:                              #   in Loop: Header=BB2_107 Depth=3
	xorl	%eax, %eax
	cmpb	$58, (%r12)
	sete	%al
	leaq	(%r12,%rax), %rdi
	movzbl	(%r12,%rax), %eax
	testb	%al, %al
	jne	.LBB2_107
.LBB2_113:                              #   in Loop: Header=BB2_61 Depth=2
	movq	g_env_libpath(%rip), %rdi
	testq	%rdi, %rdi
	je	.LBB2_122
# %bb.114:                              #   in Loop: Header=BB2_61 Depth=2
	movzbl	(%rdi), %eax
	testb	%al, %al
	je	.LBB2_122
	.p2align	4
.LBB2_116:                              #   Parent Loop BB2_59 Depth=1
                                        #     Parent Loop BB2_61 Depth=2
                                        # =>    This Loop Header: Depth=3
                                        #         Child Loop BB2_118 Depth 4
	xorl	%esi, %esi
	movq	%rdi, %r12
	testb	%al, %al
	je	.LBB2_120
	.p2align	4
.LBB2_118:                              #   Parent Loop BB2_59 Depth=1
                                        #     Parent Loop BB2_61 Depth=2
                                        #       Parent Loop BB2_116 Depth=3
                                        # =>      This Inner Loop Header: Depth=4
	movzbl	%al, %eax
	cmpl	$58, %eax
	je	.LBB2_120
# %bb.119:                              #   in Loop: Header=BB2_118 Depth=4
	movzbl	1(%r12), %eax
	incq	%r12
	incq	%rsi
	testb	%al, %al
	jne	.LBB2_118
.LBB2_120:                              #   in Loop: Header=BB2_116 Depth=3
	movq	%r13, %rdx
	callq	try_dir
	testq	%rax, %rax
	jne	.LBB2_127
# %bb.121:                              #   in Loop: Header=BB2_116 Depth=3
	xorl	%eax, %eax
	cmpb	$58, (%r12)
	sete	%al
	leaq	(%r12,%rax), %rdi
	movzbl	(%r12,%rax), %eax
	testb	%al, %al
	jne	.LBB2_116
.LBB2_122:                              #   in Loop: Header=BB2_61 Depth=2
	cmpb	$0, g_prog_dir(%rip)
	je	.LBB2_126
# %bb.123:                              #   in Loop: Header=BB2_61 Depth=2
	xorl	%esi, %esi
	.p2align	4
.LBB2_124:                              #   Parent Loop BB2_59 Depth=1
                                        #     Parent Loop BB2_61 Depth=2
                                        # =>    This Inner Loop Header: Depth=3
	cmpb	$0, 1(%rsi,%r14)
	leaq	1(%rsi), %rsi
	jne	.LBB2_124
# %bb.125:                              #   in Loop: Header=BB2_61 Depth=2
	movq	%r14, %rdi
	movq	%r13, %rdx
	callq	try_dir
	testq	%rax, %rax
	jne	.LBB2_127
.LBB2_126:                              #   in Loop: Header=BB2_61 Depth=2
	movl	$1, %esi
	leaq	.L.str.28(%rip), %rdi
	movq	%r13, %rdx
	callq	try_dir
	testq	%rax, %rax
	jne	.LBB2_127
# %bb.163:
	leaq	.L.str.29(%rip), %rdi
	callq	dstr
	movq	%r13, %rdi
	callq	dstr
	leaq	.L.str.5(%rip), %rdi
	callq	dstr
	callq	sys_exit
.LBB2_70:
	testl	%eax, %eax
	movq	16(%rsp), %r14                  # 8-byte Reload
	movq	8(%rsp), %rdi                   # 8-byte Reload
	jle	.LBB2_158
# %bb.71:
	xorl	%r14d, %r14d
	leaq	.L.str.7(%rip), %r12
	leaq	.L.str.5(%rip), %rbx
	movabsq	$-6148914691236517205, %r13     # imm = 0xAAAAAAAAAAAAAAAB
	jmp	.LBB2_72
	.p2align	4
.LBB2_143:                              #   in Loop: Header=BB2_72 Depth=1
	incq	%r14
	movslq	g_nobjs(%rip), %rax
	cmpq	%rax, %r14
	jge	.LBB2_128
.LBB2_72:                               # =>This Loop Header: Depth=1
                                        #     Child Loop BB2_78 Depth 2
                                        #       Child Loop BB2_134 Depth 3
	movq	%r14, %rax
	shlq	$8, %rax
	leaq	(%rax,%r14,8), %rbp
	cmpl	$0, 252(%r15,%rbp)
	jne	.LBB2_143
# %bb.73:                               #   in Loop: Header=BB2_72 Depth=1
	addq	%r15, %rbp
	movl	$1, 252(%rbp)
	cmpb	$1, g_debug(%rip)
	jne	.LBB2_75
# %bb.74:                               #   in Loop: Header=BB2_72 Depth=1
	movl	$1, %eax
	movl	$2, %edi
	movl	$5, %edx
	movq	%r12, %rsi
	xorl	%r10d, %r10d
	xorl	%r8d, %r8d
	xorl	%r9d, %r9d
	#APP
	syscall
	#NO_APP
	movl	$1, %eax
	movl	$10, %edx
	leaq	.L.str.30(%rip), %rsi
	xorl	%r10d, %r10d
	xorl	%r8d, %r8d
	xorl	%r9d, %r9d
	#APP
	syscall
	#NO_APP
	movl	$1, %eax
	movl	$1, %edx
	movq	%rbx, %rsi
	xorl	%r10d, %r10d
	xorl	%r8d, %r8d
	xorl	%r9d, %r9d
	#APP
	syscall
	#NO_APP
.LBB2_75:                               #   in Loop: Header=BB2_72 Depth=1
	movq	104(%rbp), %rax
	testq	%rax, %rax
	je	.LBB2_139
# %bb.76:                               #   in Loop: Header=BB2_72 Depth=1
	movq	112(%rbp), %rcx
	cmpq	$8, %rcx
	jae	.LBB2_77
.LBB2_139:                              #   in Loop: Header=BB2_72 Depth=1
	movq	72(%rbp), %rsi
	testq	%rsi, %rsi
	je	.LBB2_141
# %bb.140:                              #   in Loop: Header=BB2_72 Depth=1
	movq	%r13, %rax
	mulq	80(%rbp)
	shrq	$4, %rdx
	movq	%rbp, %rdi
	callq	apply_rela
.LBB2_141:                              #   in Loop: Header=BB2_72 Depth=1
	movq	88(%rbp), %rsi
	testq	%rsi, %rsi
	je	.LBB2_143
# %bb.142:                              #   in Loop: Header=BB2_72 Depth=1
	movq	%r13, %rax
	mulq	96(%rbp)
	shrq	$4, %rdx
	movq	%rbp, %rdi
	callq	apply_rela
	jmp	.LBB2_143
.LBB2_77:                               #   in Loop: Header=BB2_72 Depth=1
	shrq	$3, %rcx
	xorl	%esi, %esi
	xorl	%edx, %edx
	jmp	.LBB2_78
	.p2align	4
.LBB2_137:                              #   in Loop: Header=BB2_78 Depth=2
	movq	16(%rbp), %rsi
	addq	%rsi, (%rsi,%rdi)
	addq	%rdi, %rsi
	addq	$8, %rsi
.LBB2_138:                              #   in Loop: Header=BB2_78 Depth=2
	incq	%rdx
	cmpq	%rcx, %rdx
	je	.LBB2_139
.LBB2_78:                               #   Parent Loop BB2_72 Depth=1
                                        # =>  This Loop Header: Depth=2
                                        #       Child Loop BB2_134 Depth 3
	movq	(%rax,%rdx,8), %rdi
	testb	$1, %dil
	je	.LBB2_137
# %bb.79:                               #   in Loop: Header=BB2_78 Depth=2
	movq	%rsi, %r8
	cmpq	$2, %rdi
	jae	.LBB2_134
.LBB2_80:                               #   in Loop: Header=BB2_78 Depth=2
	addq	$504, %rsi                      # imm = 0x1F8
	jmp	.LBB2_138
	.p2align	4
.LBB2_136:                              #   in Loop: Header=BB2_134 Depth=3
	movq	%rdi, %r9
	shrq	%r9
	addq	$8, %r8
	cmpq	$4, %rdi
	movq	%r9, %rdi
	jb	.LBB2_80
.LBB2_134:                              #   Parent Loop BB2_72 Depth=1
                                        #     Parent Loop BB2_78 Depth=2
                                        # =>    This Inner Loop Header: Depth=3
	testb	$2, %dil
	je	.LBB2_136
# %bb.135:                              #   in Loop: Header=BB2_134 Depth=3
	movq	16(%rbp), %r9
	addq	%r9, (%r8)
	jmp	.LBB2_136
.LBB2_128:
	testl	%eax, %eax
	movq	16(%rsp), %r14                  # 8-byte Reload
	movq	8(%rsp), %rdi                   # 8-byte Reload
	jle	.LBB2_158
# %bb.129:
	leaq	g_objs+168(%rip), %rbx
	xorl	%r14d, %r14d
	leaq	.L.str.7(%rip), %r12
	leaq	.L.str.5(%rip), %r13
	jmp	.LBB2_130
	.p2align	4
.LBB2_161:                              #   in Loop: Header=BB2_130 Depth=1
	incq	%r14
	movslq	g_nobjs(%rip), %rbp
	addq	$264, %rbx                      # imm = 0x108
	cmpq	%rbp, %r14
	jge	.LBB2_144
.LBB2_130:                              # =>This Inner Loop Header: Depth=1
	movq	(%rbx), %rax
	testq	%rax, %rax
	je	.LBB2_161
# %bb.131:                              #   in Loop: Header=BB2_130 Depth=1
	movq	-8(%rbx), %rdi
	addq	-152(%rbx), %rdi
	leaq	(%rax,%rdi), %rsi
	addq	$4095, %rsi                     # imm = 0xFFF
	andq	$-4096, %rdi                    # imm = 0xF000
	andq	$-4096, %rsi                    # imm = 0xF000
	subq	%rdi, %rsi
	jbe	.LBB2_161
# %bb.132:                              #   in Loop: Header=BB2_130 Depth=1
	movl	$10, %eax
	movl	$1, %edx
	xorl	%r10d, %r10d
	xorl	%r8d, %r8d
	xorl	%r9d, %r9d
	#APP
	syscall
	#NO_APP
	cmpq	$-4095, %rax                    # imm = 0xF001
	jae	.LBB2_133
# %bb.159:                              #   in Loop: Header=BB2_130 Depth=1
	cmpb	$1, g_debug(%rip)
	jne	.LBB2_161
# %bb.160:                              #   in Loop: Header=BB2_130 Depth=1
	movl	$1, %eax
	movl	$2, %edi
	movl	$5, %edx
	movq	%r12, %rsi
	xorl	%r10d, %r10d
	xorl	%r8d, %r8d
	xorl	%r9d, %r9d
	#APP
	syscall
	#NO_APP
	movl	$1, %eax
	movl	$33, %edx
	leaq	.L.str.36(%rip), %rsi
	xorl	%r10d, %r10d
	xorl	%r8d, %r8d
	xorl	%r9d, %r9d
	#APP
	syscall
	#NO_APP
	movl	$1, %eax
	movl	$1, %edx
	movq	%r13, %rsi
	xorl	%r10d, %r10d
	xorl	%r8d, %r8d
	xorl	%r9d, %r9d
	#APP
	syscall
	#NO_APP
	jmp	.LBB2_161
.LBB2_144:
	testl	%ebp, %ebp
	movq	16(%rsp), %r14                  # 8-byte Reload
	movq	8(%rsp), %rdi                   # 8-byte Reload
	jle	.LBB2_158
# %bb.145:
	movq	32(%rsp), %rax                  # 8-byte Reload
	decl	%eax
	movl	%eax, 24(%rsp)                  # 4-byte Spill
	jmp	.LBB2_146
	.p2align	4
.LBB2_156:                              #   in Loop: Header=BB2_146 Depth=1
	cmpq	$1, %r13
	jle	.LBB2_157
.LBB2_146:                              # =>This Loop Header: Depth=1
                                        #     Child Loop BB2_153 Depth 2
	movq	%rbp, %r13
	decq	%rbp
	movq	%rbp, %rax
	shlq	$8, %rax
	leaq	(%rax,%r13,8), %r14
	addq	$-8, %r14
	cmpl	$0, 256(%r15,%r14)
	jne	.LBB2_156
# %bb.147:                              #   in Loop: Header=BB2_146 Depth=1
	addq	%r15, %r14
	movl	$1, 256(%r14)
	movq	136(%r14), %rbx
	testq	%rbx, %rbx
	je	.LBB2_151
# %bb.148:                              #   in Loop: Header=BB2_146 Depth=1
	cmpb	$1, g_debug(%rip)
	jne	.LBB2_150
# %bb.149:                              #   in Loop: Header=BB2_146 Depth=1
	movl	$1, %eax
	movl	$2, %edi
	movl	$5, %edx
	leaq	.L.str.7(%rip), %rsi
	xorl	%r10d, %r10d
	xorl	%r8d, %r8d
	xorl	%r9d, %r9d
	#APP
	syscall
	#NO_APP
	movl	$1, %eax
	movl	$7, %edx
	leaq	.L.str.37(%rip), %rsi
	xorl	%r10d, %r10d
	xorl	%r8d, %r8d
	xorl	%r9d, %r9d
	#APP
	syscall
	#NO_APP
	movl	$1, %eax
	movl	$1, %edx
	leaq	.L.str.9(%rip), %rsi
	xorl	%r10d, %r10d
	xorl	%r8d, %r8d
	xorl	%r9d, %r9d
	#APP
	syscall
	#NO_APP
	movq	%rbx, %rdi
	callq	dhex
	movl	$1, %eax
	movl	$2, %edi
	movl	$1, %edx
	leaq	.L.str.5(%rip), %rsi
	xorl	%r10d, %r10d
	xorl	%r8d, %r8d
	xorl	%r9d, %r9d
	#APP
	syscall
	#NO_APP
	movq	136(%r14), %rbx
.LBB2_150:                              #   in Loop: Header=BB2_146 Depth=1
	movl	24(%rsp), %edi                  # 4-byte Reload
	movq	40(%rsp), %rsi                  # 8-byte Reload
	movq	(%rsp), %rdx                    # 8-byte Reload
	callq	*%rbx
.LBB2_151:                              #   in Loop: Header=BB2_146 Depth=1
	movq	152(%r14), %r12
	cmpq	$8, %r12
	jb	.LBB2_156
# %bb.152:                              #   in Loop: Header=BB2_146 Depth=1
	shrq	$3, %r12
	xorl	%ebx, %ebx
	jmp	.LBB2_153
	.p2align	4
.LBB2_155:                              #   in Loop: Header=BB2_153 Depth=2
	incq	%rbx
	cmpq	%rbx, %r12
	je	.LBB2_156
.LBB2_153:                              #   Parent Loop BB2_146 Depth=1
                                        # =>  This Inner Loop Header: Depth=2
	movq	144(%r14), %rax
	movq	(%rax,%rbx,8), %rax
	leaq	-1(%rax), %rcx
	cmpq	$-3, %rcx
	ja	.LBB2_155
# %bb.154:                              #   in Loop: Header=BB2_153 Depth=2
	movl	24(%rsp), %edi                  # 4-byte Reload
	movq	40(%rsp), %rsi                  # 8-byte Reload
	movq	(%rsp), %rdx                    # 8-byte Reload
	callq	*%rax
	jmp	.LBB2_155
.LBB2_157:
	movq	16(%rsp), %r14                  # 8-byte Reload
	movq	8(%rsp), %rdi                   # 8-byte Reload
.LBB2_158:
	movq	32(%rsp), %rsi                  # 8-byte Reload
                                        # kill: def $esi killed $esi killed $rsi
	movq	%r14, %rdx
	movq	(%rsp), %rcx                    # 8-byte Reload
	movq	48(%rsp), %r8                   # 8-byte Reload
	callq	handoff
.LBB2_133:
	negq	%rax
	leaq	.L.str.35(%rip), %rdi
	movq	%rax, %rsi
	callq	die2
.Lfunc_end2:
	.size	loader_main, .Lfunc_end2-loader_main
                                        # -- End function
	.p2align	4                               # -- Begin function die
	.type	die,@function
die:                                    # @die
# %bb.0:
	pushq	%rbx
	movq	%rdi, %rbx
	leaq	.L.str.6(%rip), %rdi
	callq	dstr
	movq	%rbx, %rdi
	callq	dstr
	leaq	.L.str.5(%rip), %rdi
	callq	dstr
	callq	sys_exit
.Lfunc_end3:
	.size	die, .Lfunc_end3-die
                                        # -- End function
	.p2align	4                               # -- Begin function load_path
	.type	load_path,@function
load_path:                              # @load_path
# %bb.0:
	pushq	%r14
	pushq	%rbx
	pushq	%rax
	movq	%rdi, %rbx
	cmpb	$0, g_debug(%rip)
	je	.LBB4_4
# %bb.1:
	leaq	.L.str.7(%rip), %rsi
	movl	$1, %eax
	movl	$2, %edi
	movl	$5, %edx
	xorl	%r10d, %r10d
	xorl	%r8d, %r8d
	xorl	%r9d, %r9d
	#APP
	syscall
	#NO_APP
	movq	$-1, %rdx
	.p2align	4
.LBB4_2:                                # =>This Inner Loop Header: Depth=1
	cmpb	$0, 1(%rbx,%rdx)
	leaq	1(%rdx), %rdx
	jne	.LBB4_2
# %bb.3:
	movl	$1, %eax
	movl	$2, %edi
	movq	%rbx, %rsi
	xorl	%r10d, %r10d
	xorl	%r8d, %r8d
	xorl	%r9d, %r9d
	#APP
	syscall
	#NO_APP
	leaq	.L.str.5(%rip), %rsi
	movl	$1, %eax
	movl	$1, %edx
	xorl	%r10d, %r10d
	xorl	%r8d, %r8d
	xorl	%r9d, %r9d
	#APP
	syscall
	#NO_APP
.LBB4_4:
	movl	$257, %eax                      # imm = 0x101
	movq	$-100, %rdi
	movq	%rbx, %rsi
	xorl	%edx, %edx
	xorl	%r10d, %r10d
	xorl	%r8d, %r8d
	xorl	%r9d, %r9d
	#APP
	syscall
	#NO_APP
	movq	%rax, %r14
	testq	%rax, %rax
	js	.LBB4_6
# %bb.5:
	movl	%r14d, %edi
	movq	%rbx, %rsi
	callq	load_from_fd
	movq	%rax, %rbx
	movslq	%r14d, %rdi
	movl	$3, %eax
	xorl	%esi, %esi
	xorl	%edx, %edx
	xorl	%r10d, %r10d
	xorl	%r8d, %r8d
	xorl	%r9d, %r9d
	#APP
	syscall
	#NO_APP
	movq	%rbx, %rax
	addq	$8, %rsp
	popq	%rbx
	popq	%r14
	retq
.LBB4_6:
	negq	%r14
	leaq	.L.str.8(%rip), %rdi
	movq	%r14, %rsi
	callq	die2
.Lfunc_end4:
	.size	load_path, .Lfunc_end4-load_path
                                        # -- End function
	.p2align	4                               # -- Begin function dstr
	.type	dstr,@function
dstr:                                   # @dstr
# %bb.0:
	movq	%rdi, %rsi
	movq	$-1, %rdx
	.p2align	4
.LBB5_1:                                # =>This Inner Loop Header: Depth=1
	cmpb	$0, 1(%rsi,%rdx)
	leaq	1(%rdx), %rdx
	jne	.LBB5_1
# %bb.2:
	movl	$1, %eax
	movl	$2, %edi
	xorl	%r10d, %r10d
	xorl	%r8d, %r8d
	xorl	%r9d, %r9d
	#APP
	syscall
	#NO_APP
	retq
.Lfunc_end5:
	.size	dstr, .Lfunc_end5-dstr
                                        # -- End function
	.p2align	4                               # -- Begin function handoff
	.type	handoff,@function
handoff:                                # @handoff
# %bb.0:
	pushq	%rbp
	pushq	%r15
	pushq	%r14
	pushq	%r13
	pushq	%r12
	pushq	%rbx
	subq	$40, %rsp
	movq	%r8, %r14
	movq	%rcx, %r13
                                        # kill: def $esi killed $esi def $rsi
	movq	%rdi, 32(%rsp)                  # 8-byte Spill
	movabsq	$-4294967296, %rax              # imm = 0xFFFFFFFF00000000
	movq	%rsi, 24(%rsp)                  # 8-byte Spill
	leal	-1(%rsi), %ecx
	movl	%ecx, 12(%rsp)                  # 4-byte Spill
	movq	$-1, %rbx
	movq	%rax, %r15
	.p2align	4
.LBB6_1:                                # =>This Inner Loop Header: Depth=1
	addq	%rax, %r15
	cmpq	$0, 8(%r13,%rbx,8)
	leaq	1(%rbx), %rbx
	jne	.LBB6_1
# %bb.2:
	movq	%rdx, 16(%rsp)                  # 8-byte Spill
	movq	$-1, %r12
	xorl	%ebp, %ebp
	.p2align	4
.LBB6_3:                                # =>This Inner Loop Header: Depth=1
	incq	%r12
	cmpq	$0, (%r14,%rbp,8)
	leaq	2(%rbp), %rbp
	jne	.LBB6_3
# %bb.4:
	movl	$9, %eax
	movl	$262144, %esi                   # imm = 0x40000
	movl	$3, %edx
	movl	$34, %r10d
	xorl	%edi, %edi
	movq	$-1, %r8
	xorl	%r9d, %r9d
	#APP
	syscall
	#NO_APP
	cmpq	$-4095, %rax                    # imm = 0xF001
	jb	.LBB6_5
# %bb.43:
	negq	%rax
	leaq	.L.str.38(%rip), %rdi
	movq	%rax, %rsi
	callq	die2
.LBB6_5:
	movq	24(%rsp), %rdx                  # 8-byte Reload
	movslq	%edx, %rcx
	sarq	$32, %r15
	andl	$-2, %ebp
	addq	%rcx, %rbp
	subq	%rbp, %r15
	leaq	(%rax,%r15,8), %r15
	addq	$262144, %r15                   # imm = 0x40000
	andq	$-16, %r15
	movslq	12(%rsp), %rax                  # 4-byte Folded Reload
	movq	%rax, (%r15)
	leaq	8(%r15), %rax
	cmpl	$2, %edx
	jl	.LBB6_12
# %bb.6:
	movl	12(%rsp), %ecx                  # 4-byte Reload
	cmpl	$5, 24(%rsp)                    # 4-byte Folded Reload
	jae	.LBB6_9
# %bb.7:
	xorl	%edx, %edx
	movq	16(%rsp), %r8                   # 8-byte Reload
	jmp	.LBB6_8
.LBB6_9:
	movl	%ecx, %edx
	andl	$2147483644, %edx               # imm = 0x7FFFFFFC
	leaq	(,%rdx,8), %rsi
	leaq	(%rax,%rdx,8), %rax
	xorl	%edi, %edi
	movq	16(%rsp), %r8                   # 8-byte Reload
.LBB6_10:                               # =>This Inner Loop Header: Depth=1
	movups	8(%r8,%rdi), %xmm0
	movups	24(%r8,%rdi), %xmm1
	movups	%xmm0, 8(%r15,%rdi)
	movups	%xmm1, 24(%r15,%rdi)
	addq	$32, %rdi
	cmpq	%rdi, %rsi
	jne	.LBB6_10
# %bb.11:
	cmpl	%ecx, %edx
	je	.LBB6_12
	.p2align	4
.LBB6_8:                                # =>This Inner Loop Header: Depth=1
	movq	8(%r8,%rdx,8), %rsi
	movq	%rsi, (%rax)
	incq	%rdx
	addq	$8, %rax
	cmpq	%rdx, %rcx
	jne	.LBB6_8
.LBB6_12:
	movq	$0, (%rax)
	leaq	8(%rax), %rcx
	testq	%rbx, %rbx
	je	.LBB6_22
# %bb.13:
	cmpl	$2, %ebx
	movl	$1, %edx
	movl	$1, %edi
	cmovgel	%ebx, %edi
	cmpl	$4, %ebx
	jge	.LBB6_15
# %bb.14:
	xorl	%esi, %esi
	jmp	.LBB6_18
.LBB6_15:
	movl	%edi, %esi
	andl	$2147483644, %esi               # imm = 0x7FFFFFFC
	leaq	(%rcx,%rsi,8), %rcx
	cmpl	$2, %ebx
	movl	$1, %r8d
	cmovgel	%ebx, %r8d
	andl	$2147483644, %r8d               # imm = 0x7FFFFFFC
	xorl	%r9d, %r9d
	.p2align	4
.LBB6_16:                               # =>This Inner Loop Header: Depth=1
	movups	(%r13,%r9,8), %xmm0
	movups	16(%r13,%r9,8), %xmm1
	movups	%xmm0, 8(%rax,%r9,8)
	movups	%xmm1, 24(%rax,%r9,8)
	addq	$4, %r9
	cmpq	%r9, %r8
	jne	.LBB6_16
# %bb.17:
	cmpl	%edi, %esi
	je	.LBB6_21
.LBB6_18:
	cmpl	$2, %ebx
	cmovgel	%ebx, %edx
	leaq	(,%rsi,8), %rax
	addq	%r13, %rax
	subq	%rsi, %rdx
	xorl	%esi, %esi
	xorl	%edi, %edi
	.p2align	4
.LBB6_19:                               # =>This Inner Loop Header: Depth=1
	movq	(%rax,%rdi,8), %r8
	movq	%r8, (%rcx,%rdi,8)
	incq	%rdi
	addq	$-8, %rsi
	cmpq	%rdi, %rdx
	jne	.LBB6_19
# %bb.20:
	subq	%rsi, %rcx
.LBB6_21:
	leaq	-8(%rcx), %rax
.LBB6_22:
	movq	$0, (%rcx)
	leaq	16(%rax), %rcx
	testq	%r12, %r12
	je	.LBB6_30
# %bb.23:
	addq	$8, %r14
	movq	32(%rsp), %rsi                  # 8-byte Reload
	movq	16(%rsp), %rdi                  # 8-byte Reload
	jmp	.LBB6_24
.LBB6_38:                               #   in Loop: Header=BB6_24 Depth=1
	movq	24(%rsi), %rdx
	.p2align	4
.LBB6_42:                               #   in Loop: Header=BB6_24 Depth=1
	movq	%rax, (%rcx)
	movq	%rdx, 8(%rcx)
	addq	$16, %rcx
	addq	$16, %r14
	decq	%r12
	je	.LBB6_29
.LBB6_24:                               # =>This Inner Loop Header: Depth=1
	movq	-8(%r14), %rax
	movq	(%r14), %rdx
	cmpq	$5, %rax
	jle	.LBB6_25
# %bb.31:                               #   in Loop: Header=BB6_24 Depth=1
	cmpq	$8, %rax
	jg	.LBB6_35
# %bb.32:                               #   in Loop: Header=BB6_24 Depth=1
	cmpq	$6, %rax
	je	.LBB6_41
# %bb.33:                               #   in Loop: Header=BB6_24 Depth=1
	cmpq	$7, %rax
	jne	.LBB6_42
# %bb.34:                               #   in Loop: Header=BB6_24 Depth=1
	xorl	%edx, %edx
	jmp	.LBB6_42
	.p2align	4
.LBB6_25:                               #   in Loop: Header=BB6_24 Depth=1
	cmpq	$3, %rax
	je	.LBB6_38
# %bb.26:                               #   in Loop: Header=BB6_24 Depth=1
	cmpq	$4, %rax
	je	.LBB6_39
# %bb.27:                               #   in Loop: Header=BB6_24 Depth=1
	cmpq	$5, %rax
	jne	.LBB6_42
# %bb.28:                               #   in Loop: Header=BB6_24 Depth=1
	movq	32(%rsi), %rdx
	jmp	.LBB6_42
.LBB6_35:                               #   in Loop: Header=BB6_24 Depth=1
	cmpq	$9, %rax
	je	.LBB6_40
# %bb.36:                               #   in Loop: Header=BB6_24 Depth=1
	cmpq	$31, %rax
	jne	.LBB6_42
# %bb.37:                               #   in Loop: Header=BB6_24 Depth=1
	movq	8(%rdi), %rdx
	jmp	.LBB6_42
.LBB6_41:                               #   in Loop: Header=BB6_24 Depth=1
	movl	$4096, %edx                     # imm = 0x1000
	jmp	.LBB6_42
.LBB6_40:                               #   in Loop: Header=BB6_24 Depth=1
	movq	40(%rsi), %rdx
	jmp	.LBB6_42
.LBB6_39:                               #   in Loop: Header=BB6_24 Depth=1
	movl	$56, %edx
	jmp	.LBB6_42
.LBB6_29:
	leaq	-16(%rcx), %rax
.LBB6_30:
	movq	$0, (%rcx)
	movq	$0, 24(%rax)
	movq	32(%rsp), %rbx                  # 8-byte Reload
	movq	40(%rbx), %rsi
	leaq	.L.str.39(%rip), %rdi
	callq	trace2
	leaq	.L.str.40(%rip), %rdi
	movq	%r15, %rsi
	callq	trace2
	movq	40(%rbx), %rdi
	movq	%r15, %rsi
	#APP
	movq	%rsi, %rsp
	xorl	%edx, %edx
	xorl	%ebp, %ebp
	jmpq	*%rdi

	#NO_APP
.Lfunc_end6:
	.size	handoff, .Lfunc_end6-handoff
                                        # -- End function
	.p2align	4                               # -- Begin function sys_exit
	.type	sys_exit,@function
sys_exit:                               # @sys_exit
# %bb.0:
	pushq	%rax
	movl	$231, %edi
	movl	$127, %esi
	callq	syscall1
.Lfunc_end7:
	.size	sys_exit, .Lfunc_end7-sys_exit
                                        # -- End function
	.p2align	4                               # -- Begin function syscall1
	.type	syscall1,@function
syscall1:                               # @syscall1
# %bb.0:
	movq	%rdi, %rax
	movq	%rsi, %rdi
	xorl	%esi, %esi
	xorl	%edx, %edx
	xorl	%r10d, %r10d
	xorl	%r8d, %r8d
	xorl	%r9d, %r9d
	#APP
	syscall
	#NO_APP
	retq
.Lfunc_end8:
	.size	syscall1, .Lfunc_end8-syscall1
                                        # -- End function
	.p2align	4                               # -- Begin function die2
	.type	die2,@function
die2:                                   # @die2
# %bb.0:
	pushq	%r14
	pushq	%rbx
	pushq	%rax
	movq	%rsi, %rbx
	movq	%rdi, %r14
	leaq	.L.str.6(%rip), %rdi
	callq	dstr
	movq	%r14, %rdi
	callq	dstr
	leaq	.L.str.9(%rip), %rdi
	callq	dstr
	movq	%rbx, %rdi
	callq	dhex
	leaq	.L.str.5(%rip), %rdi
	callq	dstr
	callq	sys_exit
.Lfunc_end9:
	.size	die2, .Lfunc_end9-die2
                                        # -- End function
	.section	.rodata.cst16,"aM",@progbits,16
	.p2align	4, 0x0                          # -- Begin function load_from_fd
.LCPI10_0:
	.long	2147483648                      # 0x80000000
	.long	2147483648                      # 0x80000000
	.long	2147483648                      # 0x80000000
	.long	2147483648                      # 0x80000000
	.text
	.p2align	4
	.type	load_from_fd,@function
load_from_fd:                           # @load_from_fd
# %bb.0:
	pushq	%rbp
	pushq	%r15
	pushq	%r14
	pushq	%r13
	pushq	%r12
	pushq	%rbx
	subq	$88, %rsp
	movslq	g_nobjs(%rip), %rax
	cmpq	$32, %rax
	jge	.LBB10_123
# %bb.1:
	leal	1(%rax), %ecx
	movl	%ecx, g_nobjs(%rip)
	movq	%rax, %rcx
	shlq	$8, %rcx
	leaq	(%rcx,%rax,8), %r14
	leaq	g_objs(%rip), %rbx
	pxor	%xmm0, %xmm0
	movdqu	%xmm0, 248(%rbx,%r14)
	movdqu	%xmm0, 232(%rbx,%r14)
	movdqu	%xmm0, 216(%rbx,%r14)
	movdqu	%xmm0, 200(%rbx,%r14)
	movdqu	%xmm0, 184(%rbx,%r14)
	movdqu	%xmm0, 168(%rbx,%r14)
	movdqu	%xmm0, 152(%rbx,%r14)
	movdqu	%xmm0, 136(%rbx,%r14)
	movdqu	%xmm0, 120(%rbx,%r14)
	movdqu	%xmm0, 104(%rbx,%r14)
	movdqu	%xmm0, 88(%rbx,%r14)
	movdqu	%xmm0, 72(%rbx,%r14)
	movdqu	%xmm0, 56(%rbx,%r14)
	movdqu	%xmm0, 40(%rbx,%r14)
	movdqu	%xmm0, 24(%rbx,%r14)
	movdqu	%xmm0, 8(%rbx,%r14)
	movq	%rsi, (%rbx,%r14)
	movslq	%edi, %rdi
	leaq	16(%rsp), %rsi
	movl	$17, %eax
	movl	$64, %edx
	movq	%rdi, 8(%rsp)                   # 8-byte Spill
	xorl	%r10d, %r10d
	xorl	%r8d, %r8d
	xorl	%r9d, %r9d
	#APP
	syscall
	#NO_APP
	cmpq	$64, %rax
	jne	.LBB10_124
# %bb.2:
	movq	16(%rsp), %rax
	cmpl	$1179403647, %eax               # imm = 0x464C457F
	jne	.LBB10_125
# %bb.3:
	cmpb	$2, 20(%rsp)
	jne	.LBB10_126
# %bb.4:
	cmpb	$1, 21(%rsp)
	jne	.LBB10_127
# %bb.5:
	cmpw	$62, 34(%rsp)
	jne	.LBB10_128
# %bb.6:
	movl	32(%rsp), %eax
	addl	$-4, %eax
	cmpw	$-3, %ax
	jbe	.LBB10_129
# %bb.7:
	cmpw	$56, 70(%rsp)
	jne	.LBB10_130
# %bb.8:
	movzwl	72(%rsp), %eax
	leal	-257(%rax), %ecx
	movzwl	%cx, %ecx
	cmpl	$65279, %ecx                    # imm = 0xFEFF
	jbe	.LBB10_131
# %bb.9:
	imulq	$56, %rax, %rdx
	movq	48(%rsp), %r10
	leaq	load_from_fd.ph(%rip), %r15
	movl	$17, %eax
	movq	8(%rsp), %rdi                   # 8-byte Reload
	movq	%r15, %rsi
	xorl	%r8d, %r8d
	xorl	%r9d, %r9d
	#APP
	syscall
	#NO_APP
	cmpq	%rdx, %rax
	jne	.LBB10_132
# %bb.10:
	movzwl	72(%rsp), %eax
	testq	%rax, %rax
	je	.LBB10_133
# %bb.11:
	addq	%r14, %rbx
	imulq	$56, %rax, %rax
	movq	$-1, %r14
	xorl	%ecx, %ecx
	xorl	%esi, %esi
	jmp	.LBB10_12
	.p2align	4
.LBB10_14:                              #   in Loop: Header=BB10_12 Depth=1
	addq	$56, %rcx
	cmpq	%rcx, %rax
	je	.LBB10_15
.LBB10_12:                              # =>This Inner Loop Header: Depth=1
	cmpl	$1, (%rcx,%r15)
	jne	.LBB10_14
# %bb.13:                               #   in Loop: Header=BB10_12 Depth=1
	movq	16(%rcx,%r15), %rdx
	movq	40(%rcx,%r15), %rdi
	addq	%rdx, %rdi
	addq	$4095, %rdi                     # imm = 0xFFF
	andq	$-4096, %rdx                    # imm = 0xF000
	andq	$-4096, %rdi                    # imm = 0xF000
	cmpq	%r14, %rdx
	cmovbq	%rdx, %r14
	cmpq	%rsi, %rdi
	cmovaq	%rdi, %rsi
	jmp	.LBB10_14
.LBB10_15:
	cmpq	$-1, %r14
	je	.LBB10_133
# %bb.16:
	subq	%r14, %rsi
	xorl	%edi, %edi
	xorl	%r10d, %r10d
	cmpw	$2, 32(%rsp)
	sete	%r10b
	cmoveq	%r14, %rdi
	shll	$4, %r10d
	orq	$34, %r10
	movl	$9, %eax
	xorl	%edx, %edx
	movq	$-1, %r8
	xorl	%r9d, %r9d
	#APP
	syscall
	#NO_APP
	cmpq	$-4095, %rax                    # imm = 0xF001
	jae	.LBB10_134
# %bb.17:
	subq	%r14, %rax
	xorl	%r12d, %r12d
	cmpw	$2, 32(%rsp)
	cmovneq	%rax, %r12
	cmpb	$1, g_debug(%rip)
	jne	.LBB10_19
# %bb.18:
	leaq	.L.str.7(%rip), %rsi
	movl	$1, %eax
	movl	$2, %edi
	movl	$5, %edx
	xorl	%r10d, %r10d
	xorl	%r8d, %r8d
	xorl	%r9d, %r9d
	#APP
	syscall
	#NO_APP
	leaq	.L.str.23(%rip), %rsi
	movl	$1, %eax
	movl	$9, %edx
	xorl	%r10d, %r10d
	xorl	%r8d, %r8d
	xorl	%r9d, %r9d
	#APP
	syscall
	#NO_APP
	leaq	.L.str.9(%rip), %rsi
	movl	$1, %eax
	movl	$1, %edx
	xorl	%r10d, %r10d
	xorl	%r8d, %r8d
	xorl	%r9d, %r9d
	#APP
	syscall
	#NO_APP
	movq	%r12, %rdi
	callq	dhex
	leaq	.L.str.5(%rip), %rsi
	movl	$1, %eax
	movl	$2, %edi
	movl	$1, %edx
	xorl	%r10d, %r10d
	xorl	%r8d, %r8d
	xorl	%r9d, %r9d
	#APP
	syscall
	#NO_APP
.LBB10_19:
	cmpw	$0, 72(%rsp)
	je	.LBB10_20
# %bb.28:
	movq	%rbx, 80(%rsp)                  # 8-byte Spill
	leaq	4095(%r12), %rbp
	xorl	%r14d, %r14d
	jmp	.LBB10_29
	.p2align	4
.LBB10_36:                              #   in Loop: Header=BB10_29 Depth=1
	incq	%r14
	movzwl	72(%rsp), %eax
	movzwl	%ax, %ecx
	addq	$56, %r15
	cmpq	%rcx, %r14
	jae	.LBB10_22
.LBB10_29:                              # =>This Inner Loop Header: Depth=1
	cmpl	$1, (%r15)
	jne	.LBB10_36
# %bb.30:                               #   in Loop: Header=BB10_29 Depth=1
	movzbl	4(%r15), %ebx
	rolb	$4, %bl
	movl	%ebx, %eax
	shrb	$2, %al
	movl	%ebx, %ecx
	andb	$16, %cl
	shlb	$2, %cl
	orb	%al, %cl
	andb	$80, %cl
	addb	%cl, %cl
	addb	%bl, %bl
	andb	$64, %bl
	orb	%cl, %bl
	shrb	$5, %bl
	movq	16(%r15), %rsi
	leaq	(%rsi,%r12), %rdi
	movq	32(%r15), %rax
	leaq	(%rax,%rdi), %r13
	addq	$4095, %r13                     # imm = 0xFFF
	andq	$-4096, %rdi                    # imm = 0xF000
	andq	$-4096, %r13                    # imm = 0xF000
	movq	%r13, %rcx
	subq	%rdi, %rcx
	jbe	.LBB10_33
# %bb.31:                               #   in Loop: Header=BB10_29 Depth=1
	movq	8(%r15), %r9
	movq	$-4096, %rax                    # imm = 0xF000
	andq	%rax, %r9
	movzbl	%bl, %edx
	movl	$9, %eax
	movl	$18, %r10d
	movq	%rcx, %rsi
	movq	8(%rsp), %r8                    # 8-byte Reload
	#APP
	syscall
	#NO_APP
	cmpq	$-4096, %rax                    # imm = 0xF000
	ja	.LBB10_135
# %bb.32:                               #   in Loop: Header=BB10_29 Depth=1
	movq	16(%r15), %rsi
.LBB10_33:                              #   in Loop: Header=BB10_29 Depth=1
	addq	%rbp, %rsi
	addq	40(%r15), %rsi
	andq	$-4096, %rsi                    # imm = 0xF000
	subq	%r13, %rsi
	jbe	.LBB10_36
# %bb.34:                               #   in Loop: Header=BB10_29 Depth=1
	testb	$2, 4(%r15)
	je	.LBB10_36
# %bb.35:                               #   in Loop: Header=BB10_29 Depth=1
	movzbl	%bl, %edx
	movl	$9, %eax
	movl	$50, %r10d
	movq	%r13, %rdi
	movq	$-1, %r8
	xorl	%r9d, %r9d
	#APP
	syscall
	#NO_APP
	cmpq	$-4095, %rax                    # imm = 0xF001
	jb	.LBB10_36
# %bb.136:
	negq	%rax
	leaq	.L.str.25(%rip), %rdi
	movq	%rax, %rsi
	callq	die2
.LBB10_22:
	movq	80(%rsp), %rbx                  # 8-byte Reload
	movq	$0, 24(%rbx)
	testw	%ax, %ax
	je	.LBB10_21
# %bb.23:
	movq	48(%rsp), %rdx
	imulq	$56, %rcx, %r8
	xorl	%esi, %esi
	leaq	load_from_fd.ph(%rip), %rdi
	jmp	.LBB10_24
	.p2align	4
.LBB10_37:                              #   in Loop: Header=BB10_24 Depth=1
	addq	$56, %rsi
	cmpq	%rsi, %r8
	je	.LBB10_38
.LBB10_24:                              # =>This Inner Loop Header: Depth=1
	cmpl	$1, (%rsi,%rdi)
	jne	.LBB10_37
# %bb.25:                               #   in Loop: Header=BB10_24 Depth=1
	movq	8(%rsi,%rdi), %r9
	cmpq	%rdx, %r9
	ja	.LBB10_37
# %bb.26:                               #   in Loop: Header=BB10_24 Depth=1
	movq	32(%rsi,%rdi), %r10
	addq	%r9, %r10
	cmpq	%r10, %rdx
	jae	.LBB10_37
# %bb.27:
	movq	%r12, %r8
	subq	%r9, %r8
	addq	%rdx, %r8
	addq	16(%rsi,%rdi), %r8
	movq	%r8, 24(%rbx)
.LBB10_38:
	movq	%rcx, 32(%rbx)
	movq	%r12, 16(%rbx)
	addq	40(%rsp), %r12
	movq	%r12, 40(%rbx)
	leaq	load_from_fd.ph+40(%rip), %rbp
	xorl	%r14d, %r14d
	leaq	.L.str.20(%rip), %r15
	leaq	.L.str.5(%rip), %r12
	xorl	%r13d, %r13d
	jmp	.LBB10_39
	.p2align	4
.LBB10_43:                              #   in Loop: Header=BB10_39 Depth=1
	movq	-24(%rbp), %rcx
	movq	%rcx, 160(%rbx)
	movq	(%rbp), %rcx
	movq	%rcx, 168(%rbx)
.LBB10_47:                              #   in Loop: Header=BB10_39 Depth=1
	incq	%r14
	movzwl	%ax, %ecx
	addq	$56, %rbp
	cmpq	%rcx, %r14
	jae	.LBB10_48
.LBB10_39:                              # =>This Inner Loop Header: Depth=1
	movl	-40(%rbp), %ecx
	cmpl	$1685382482, %ecx               # imm = 0x6474E552
	je	.LBB10_43
# %bb.40:                               #   in Loop: Header=BB10_39 Depth=1
	cmpl	$7, %ecx
	je	.LBB10_44
# %bb.41:                               #   in Loop: Header=BB10_39 Depth=1
	cmpl	$2, %ecx
	jne	.LBB10_47
# %bb.42:                               #   in Loop: Header=BB10_39 Depth=1
	movq	-24(%rbp), %r13
	addq	16(%rbx), %r13
	jmp	.LBB10_47
	.p2align	4
.LBB10_44:                              #   in Loop: Header=BB10_39 Depth=1
	cmpq	$0, (%rbp)
	je	.LBB10_47
# %bb.45:                               #   in Loop: Header=BB10_39 Depth=1
	cmpb	$1, g_debug(%rip)
	jne	.LBB10_47
# %bb.46:                               #   in Loop: Header=BB10_39 Depth=1
	movl	$1, %eax
	movl	$2, %edi
	movl	$5, %edx
	leaq	.L.str.7(%rip), %rsi
	xorl	%r10d, %r10d
	xorl	%r8d, %r8d
	xorl	%r9d, %r9d
	#APP
	syscall
	#NO_APP
	movl	$1, %eax
	movl	$55, %edx
	movq	%r15, %rsi
	xorl	%r10d, %r10d
	xorl	%r8d, %r8d
	xorl	%r9d, %r9d
	#APP
	syscall
	#NO_APP
	movl	$1, %eax
	movl	$1, %edx
	movq	%r12, %rsi
	xorl	%r10d, %r10d
	xorl	%r8d, %r8d
	xorl	%r9d, %r9d
	#APP
	syscall
	#NO_APP
	movzwl	72(%rsp), %eax
	jmp	.LBB10_47
.LBB10_48:
	testq	%r13, %r13
	je	.LBB10_122
# %bb.49:
	movq	(%r13), %rdx
	testq	%rdx, %rdx
	je	.LBB10_99
# %bb.50:
	addq	$16, %r13
	movl	$7, %eax
	jmp	.LBB10_51
.LBB10_80:                              #   in Loop: Header=BB10_51 Depth=1
	cmpq	$27, %rdx
	je	.LBB10_93
# %bb.81:                               #   in Loop: Header=BB10_51 Depth=1
	cmpq	$29, %rdx
	jne	.LBB10_98
.LBB10_82:                              #   in Loop: Header=BB10_51 Depth=1
	movq	%rcx, 176(%rbx)
	.p2align	4
.LBB10_98:                              #   in Loop: Header=BB10_51 Depth=1
	movq	(%r13), %rdx
	addq	$16, %r13
	testq	%rdx, %rdx
	je	.LBB10_57
.LBB10_51:                              # =>This Inner Loop Header: Depth=1
	movq	-8(%r13), %rcx
	cmpq	$14, %rdx
	jg	.LBB10_71
# %bb.52:                               #   in Loop: Header=BB10_51 Depth=1
	cmpq	$5, %rdx
	jle	.LBB10_53
# %bb.63:                               #   in Loop: Header=BB10_51 Depth=1
	cmpq	$7, %rdx
	jle	.LBB10_64
# %bb.67:                               #   in Loop: Header=BB10_51 Depth=1
	cmpq	$8, %rdx
	je	.LBB10_87
# %bb.68:                               #   in Loop: Header=BB10_51 Depth=1
	cmpq	$12, %rdx
	je	.LBB10_92
# %bb.69:                               #   in Loop: Header=BB10_51 Depth=1
	cmpq	$14, %rdx
	jne	.LBB10_98
# %bb.70:                               #   in Loop: Header=BB10_51 Depth=1
	movq	%rcx, 8(%rbx)
	jmp	.LBB10_98
	.p2align	4
.LBB10_71:                              #   in Loop: Header=BB10_51 Depth=1
	cmpq	$26, %rdx
	jle	.LBB10_72
# %bb.79:                               #   in Loop: Header=BB10_51 Depth=1
	cmpq	$1879047924, %rdx               # imm = 0x6FFFFEF4
	jle	.LBB10_80
# %bb.83:                               #   in Loop: Header=BB10_51 Depth=1
	cmpq	$1879047925, %rdx               # imm = 0x6FFFFEF5
	je	.LBB10_91
# %bb.84:                               #   in Loop: Header=BB10_51 Depth=1
	cmpq	$1879048174, %rdx               # imm = 0x6FFFFFEE
	je	.LBB10_89
# %bb.85:                               #   in Loop: Header=BB10_51 Depth=1
	cmpq	$1879048175, %rdx               # imm = 0x6FFFFFEF
	jne	.LBB10_98
# %bb.86:                               #   in Loop: Header=BB10_51 Depth=1
	addq	16(%rbx), %rcx
	movq	%rcx, 104(%rbx)
	jmp	.LBB10_98
	.p2align	4
.LBB10_53:                              #   in Loop: Header=BB10_51 Depth=1
	cmpq	$3, %rdx
	jg	.LBB10_60
# %bb.54:                               #   in Loop: Header=BB10_51 Depth=1
	cmpq	$1, %rdx
	je	.LBB10_94
# %bb.55:                               #   in Loop: Header=BB10_51 Depth=1
	cmpq	$2, %rdx
	jne	.LBB10_98
# %bb.56:                               #   in Loop: Header=BB10_51 Depth=1
	movq	%rcx, 96(%rbx)
	jmp	.LBB10_98
	.p2align	4
.LBB10_72:                              #   in Loop: Header=BB10_51 Depth=1
	cmpq	$22, %rdx
	jg	.LBB10_76
# %bb.73:                               #   in Loop: Header=BB10_51 Depth=1
	cmpq	$15, %rdx
	je	.LBB10_82
# %bb.74:                               #   in Loop: Header=BB10_51 Depth=1
	cmpq	$20, %rdx
	jne	.LBB10_98
# %bb.75:                               #   in Loop: Header=BB10_51 Depth=1
	movq	%rcx, %rax
	jmp	.LBB10_98
.LBB10_60:                              #   in Loop: Header=BB10_51 Depth=1
	cmpq	$4, %rdx
	je	.LBB10_90
# %bb.61:                               #   in Loop: Header=BB10_51 Depth=1
	cmpq	$5, %rdx
	jne	.LBB10_98
# %bb.62:                               #   in Loop: Header=BB10_51 Depth=1
	addq	16(%rbx), %rcx
	movq	%rcx, 56(%rbx)
	jmp	.LBB10_98
.LBB10_76:                              #   in Loop: Header=BB10_51 Depth=1
	cmpq	$23, %rdx
	je	.LBB10_88
# %bb.77:                               #   in Loop: Header=BB10_51 Depth=1
	cmpq	$25, %rdx
	jne	.LBB10_98
# %bb.78:                               #   in Loop: Header=BB10_51 Depth=1
	addq	16(%rbx), %rcx
	movq	%rcx, 144(%rbx)
	jmp	.LBB10_98
.LBB10_64:                              #   in Loop: Header=BB10_51 Depth=1
	cmpq	$6, %rdx
	je	.LBB10_97
# %bb.65:                               #   in Loop: Header=BB10_51 Depth=1
	cmpq	$7, %rdx
	jne	.LBB10_98
# %bb.66:                               #   in Loop: Header=BB10_51 Depth=1
	addq	16(%rbx), %rcx
	movq	%rcx, 72(%rbx)
	jmp	.LBB10_98
.LBB10_94:                              #   in Loop: Header=BB10_51 Depth=1
	movslq	248(%rbx), %rdx
	cmpq	$15, %rdx
	jg	.LBB10_96
# %bb.95:                               #   in Loop: Header=BB10_51 Depth=1
	leal	1(%rdx), %esi
	movl	%esi, 248(%rbx)
	movl	%ecx, 184(%rbx,%rdx,4)
	jmp	.LBB10_98
.LBB10_90:                              #   in Loop: Header=BB10_51 Depth=1
	addq	16(%rbx), %rcx
	movq	%rcx, 120(%rbx)
	jmp	.LBB10_98
.LBB10_88:                              #   in Loop: Header=BB10_51 Depth=1
	addq	16(%rbx), %rcx
	movq	%rcx, 88(%rbx)
	jmp	.LBB10_98
.LBB10_92:                              #   in Loop: Header=BB10_51 Depth=1
	addq	16(%rbx), %rcx
	movq	%rcx, 136(%rbx)
	jmp	.LBB10_98
.LBB10_89:                              #   in Loop: Header=BB10_51 Depth=1
	movq	%rcx, 112(%rbx)
	jmp	.LBB10_98
.LBB10_87:                              #   in Loop: Header=BB10_51 Depth=1
	movq	%rcx, 80(%rbx)
	jmp	.LBB10_98
.LBB10_91:                              #   in Loop: Header=BB10_51 Depth=1
	addq	16(%rbx), %rcx
	movq	%rcx, 128(%rbx)
	jmp	.LBB10_98
.LBB10_93:                              #   in Loop: Header=BB10_51 Depth=1
	movq	%rcx, 152(%rbx)
	jmp	.LBB10_98
.LBB10_97:                              #   in Loop: Header=BB10_51 Depth=1
	addq	16(%rbx), %rcx
	movq	%rcx, 48(%rbx)
	jmp	.LBB10_98
.LBB10_20:
	movq	$0, 24(%rbx)
.LBB10_21:
	movq	$0, 32(%rbx)
	movq	%r12, 16(%rbx)
	addq	40(%rsp), %r12
	movl	$40, %eax
	jmp	.LBB10_121
.LBB10_57:
	cmpq	$0, 88(%rbx)
	je	.LBB10_99
# %bb.58:
	cmpq	$7, %rax
	jne	.LBB10_59
.LBB10_99:
	movq	56(%rbx), %rax
	testq	%rax, %rax
	je	.LBB10_104
# %bb.100:
	movq	8(%rbx), %rcx
	testq	%rcx, %rcx
	je	.LBB10_102
# %bb.101:
	addq	%rax, %rcx
	movq	%rcx, 8(%rbx)
.LBB10_102:
	movq	176(%rbx), %rcx
	testq	%rcx, %rcx
	je	.LBB10_104
# %bb.103:
	addq	%rcx, %rax
	movq	%rax, 176(%rbx)
.LBB10_104:
	movq	120(%rbx), %rax
	testq	%rax, %rax
	je	.LBB10_106
# %bb.105:
	movl	4(%rax), %r8d
.LBB10_120:
	movl	%r8d, %r12d
	movl	$64, %eax
.LBB10_121:
	movq	%r12, (%rbx,%rax)
.LBB10_122:
	movq	%rbx, %rax
	addq	$88, %rsp
	popq	%rbx
	popq	%r12
	popq	%r13
	popq	%r14
	popq	%r15
	popq	%rbp
	retq
.LBB10_106:
	movq	128(%rbx), %rcx
	testq	%rcx, %rcx
	je	.LBB10_122
# %bb.107:
	movl	(%rcx), %edx
	movl	4(%rcx), %eax
	movl	8(%rcx), %esi
	testq	%rdx, %rdx
	je	.LBB10_108
# %bb.111:
	cmpl	$8, %edx
	jae	.LBB10_113
# %bb.112:
	xorl	%edi, %edi
	xorl	%r8d, %r8d
	jmp	.LBB10_116
.LBB10_108:
	xorl	%r8d, %r8d
	jmp	.LBB10_109
.LBB10_113:
	movl	%edx, %edi
	andl	$-8, %edi
	leaq	(%rcx,%rsi,8), %r8
	addq	$32, %r8
	leaq	(,%rdx,4), %r9
	andq	$-32, %r9
	pxor	%xmm1, %xmm1
	xorl	%r10d, %r10d
	movdqa	.LCPI10_0(%rip), %xmm0          # xmm0 = [2147483648,2147483648,2147483648,2147483648]
	pxor	%xmm2, %xmm2
	.p2align	4
.LBB10_114:                             # =>This Inner Loop Header: Depth=1
	movdqu	-16(%r8,%r10), %xmm3
	movdqu	(%r8,%r10), %xmm4
	movdqa	%xmm1, %xmm5
	pxor	%xmm0, %xmm5
	movdqa	%xmm3, %xmm6
	pxor	%xmm0, %xmm6
	pcmpgtd	%xmm5, %xmm6
	pand	%xmm6, %xmm3
	pandn	%xmm1, %xmm6
	movdqa	%xmm6, %xmm1
	por	%xmm3, %xmm1
	movdqa	%xmm2, %xmm3
	pxor	%xmm0, %xmm3
	movdqa	%xmm4, %xmm5
	pxor	%xmm0, %xmm5
	pcmpgtd	%xmm3, %xmm5
	pand	%xmm5, %xmm4
	pandn	%xmm2, %xmm5
	movdqa	%xmm5, %xmm2
	por	%xmm4, %xmm2
	addq	$32, %r10
	cmpq	%r10, %r9
	jne	.LBB10_114
# %bb.115:
	movdqa	%xmm2, %xmm3
	pxor	%xmm0, %xmm3
	movdqa	%xmm1, %xmm4
	pxor	%xmm0, %xmm4
	pcmpgtd	%xmm3, %xmm4
	pand	%xmm4, %xmm1
	pandn	%xmm2, %xmm4
	por	%xmm1, %xmm4
	pshufd	$238, %xmm4, %xmm1              # xmm1 = xmm4[2,3,2,3]
	movdqa	%xmm4, %xmm2
	pxor	%xmm0, %xmm2
	movdqa	%xmm1, %xmm3
	pxor	%xmm0, %xmm3
	pcmpgtd	%xmm3, %xmm2
	pand	%xmm2, %xmm4
	pandn	%xmm1, %xmm2
	por	%xmm4, %xmm2
	pshufd	$85, %xmm2, %xmm1               # xmm1 = xmm2[1,1,1,1]
	movdqa	%xmm2, %xmm3
	pxor	%xmm0, %xmm3
	pxor	%xmm1, %xmm0
	pcmpgtd	%xmm0, %xmm3
	pand	%xmm3, %xmm2
	pandn	%xmm1, %xmm3
	por	%xmm2, %xmm3
	movd	%xmm3, %r8d
	cmpl	%edx, %edi
	je	.LBB10_109
.LBB10_116:
	leaq	(%rcx,%rsi,8), %r9
	addq	$16, %r9
	.p2align	4
.LBB10_117:                             # =>This Inner Loop Header: Depth=1
	movl	(%r9,%rdi,4), %r10d
	cmpl	%r8d, %r10d
	cmoval	%r10d, %r8d
	incq	%rdi
	cmpq	%rdi, %rdx
	jne	.LBB10_117
.LBB10_109:
	cmpl	%eax, %r8d
	jae	.LBB10_118
# %bb.110:
	movl	%eax, %r8d
	jmp	.LBB10_120
.LBB10_118:
	leaq	(%rcx,%rsi,8), %rcx
	leaq	(%rcx,%rdx,4), %rcx
	addq	$16, %rcx
	negl	%eax
	.p2align	4
.LBB10_119:                             # =>This Inner Loop Header: Depth=1
	leal	(%rax,%r8), %edx
	incl	%r8d
	testb	$1, (%rcx,%rdx,4)
	je	.LBB10_119
	jmp	.LBB10_120
.LBB10_135:
	negq	%rax
	leaq	.L.str.24(%rip), %rdi
	movq	%rax, %rsi
	callq	die2
.LBB10_133:
	leaq	.L.str.21(%rip), %rdi
	callq	die
.LBB10_123:
	leaq	.L.str.10(%rip), %rdi
	callq	die
.LBB10_124:
	leaq	.L.str.11(%rip), %rdi
	callq	die
.LBB10_125:
	leaq	.L.str.12(%rip), %rdi
	callq	die
.LBB10_126:
	leaq	.L.str.13(%rip), %rdi
	callq	die
.LBB10_127:
	leaq	.L.str.14(%rip), %rdi
	callq	die
.LBB10_128:
	leaq	.L.str.15(%rip), %rdi
	callq	die
.LBB10_129:
	leaq	.L.str.16(%rip), %rdi
	callq	die
.LBB10_130:
	leaq	.L.str.17(%rip), %rdi
	callq	die
.LBB10_131:
	leaq	.L.str.18(%rip), %rdi
	callq	die
.LBB10_132:
	leaq	.L.str.19(%rip), %rdi
	callq	die
.LBB10_134:
	negq	%rax
	leaq	.L.str.22(%rip), %rdi
	movq	%rax, %rsi
	callq	die2
.LBB10_96:
	leaq	.L.str.26(%rip), %rdi
	callq	die
.LBB10_59:
	leaq	.L.str.27(%rip), %rdi
	callq	die
.Lfunc_end10:
	.size	load_from_fd, .Lfunc_end10-load_from_fd
                                        # -- End function
	.p2align	4                               # -- Begin function dhex
	.type	dhex,@function
dhex:                                   # @dhex
# %bb.0:
	movw	$30768, -24(%rsp)               # imm = 0x7830
	movq	%rdi, %rcx
	shrq	$60, %rcx
	leaq	dhex.H(%rip), %rax
	movzbl	(%rcx,%rax), %ecx
	movb	%cl, -22(%rsp)
	movq	%rdi, %rcx
	shrq	$56, %rcx
	andl	$15, %ecx
	movzbl	(%rcx,%rax), %ecx
	movb	%cl, -21(%rsp)
	movq	%rdi, %rcx
	shrq	$52, %rcx
	andl	$15, %ecx
	movzbl	(%rcx,%rax), %ecx
	movb	%cl, -20(%rsp)
	movq	%rdi, %rcx
	shrq	$48, %rcx
	andl	$15, %ecx
	movzbl	(%rcx,%rax), %ecx
	movb	%cl, -19(%rsp)
	movq	%rdi, %rcx
	shrq	$44, %rcx
	andl	$15, %ecx
	movzbl	(%rcx,%rax), %ecx
	movb	%cl, -18(%rsp)
	movq	%rdi, %rcx
	shrq	$40, %rcx
	andl	$15, %ecx
	movzbl	(%rcx,%rax), %ecx
	movb	%cl, -17(%rsp)
	movq	%rdi, %rcx
	shrq	$36, %rcx
	andl	$15, %ecx
	movzbl	(%rcx,%rax), %ecx
	movb	%cl, -16(%rsp)
	movq	%rdi, %rcx
	shrq	$32, %rcx
	andl	$15, %ecx
	movzbl	(%rcx,%rax), %ecx
	movb	%cl, -15(%rsp)
	movl	%edi, %ecx
	shrl	$28, %ecx
	movzbl	(%rcx,%rax), %ecx
	movb	%cl, -14(%rsp)
	movl	%edi, %ecx
	shrl	$24, %ecx
	andl	$15, %ecx
	movzbl	(%rcx,%rax), %ecx
	movb	%cl, -13(%rsp)
	movl	%edi, %ecx
	shrl	$20, %ecx
	andl	$15, %ecx
	movzbl	(%rcx,%rax), %ecx
	movb	%cl, -12(%rsp)
	movl	%edi, %ecx
	shrl	$16, %ecx
	andl	$15, %ecx
	movzbl	(%rcx,%rax), %ecx
	movb	%cl, -11(%rsp)
	movl	%edi, %ecx
	shrl	$12, %ecx
	andl	$15, %ecx
	movzbl	(%rcx,%rax), %ecx
	movb	%cl, -10(%rsp)
	movl	%edi, %ecx
	shrl	$8, %ecx
	andl	$15, %ecx
	movzbl	(%rcx,%rax), %ecx
	movb	%cl, -9(%rsp)
	movl	%edi, %ecx
	shrl	$4, %ecx
	andl	$15, %ecx
	movzbl	(%rcx,%rax), %ecx
	movb	%cl, -8(%rsp)
	andl	$15, %edi
	movzbl	(%rdi,%rax), %eax
	movb	%al, -7(%rsp)
	movb	$0, -6(%rsp)
	movq	$-1, %rdx
	.p2align	4
.LBB11_1:                               # =>This Inner Loop Header: Depth=1
	cmpb	$0, -23(%rsp,%rdx)
	leaq	1(%rdx), %rdx
	jne	.LBB11_1
# %bb.2:
	leaq	-24(%rsp), %rsi
	movl	$1, %eax
	movl	$2, %edi
	xorl	%r10d, %r10d
	xorl	%r8d, %r8d
	xorl	%r9d, %r9d
	#APP
	syscall
	#NO_APP
	retq
.Lfunc_end11:
	.size	dhex, .Lfunc_end11-dhex
                                        # -- End function
	.p2align	4                               # -- Begin function trace2
	.type	trace2,@function
trace2:                                 # @trace2
# %bb.0:
	cmpb	$1, g_debug(%rip)
	jne	.LBB12_4
# %bb.1:
	pushq	%r14
	pushq	%rbx
	pushq	%rax
	movq	%rsi, %rbx
	movq	%rdi, %r14
	leaq	.L.str.7(%rip), %rsi
	movl	$1, %eax
	movl	$2, %edi
	movl	$5, %edx
	xorl	%r10d, %r10d
	xorl	%r8d, %r8d
	xorl	%r9d, %r9d
	#APP
	syscall
	#NO_APP
	movq	$-1, %rdx
	.p2align	4
.LBB12_2:                               # =>This Inner Loop Header: Depth=1
	cmpb	$0, 1(%r14,%rdx)
	leaq	1(%rdx), %rdx
	jne	.LBB12_2
# %bb.3:
	movl	$1, %eax
	movl	$2, %edi
	movq	%r14, %rsi
	xorl	%r10d, %r10d
	xorl	%r8d, %r8d
	xorl	%r9d, %r9d
	#APP
	syscall
	#NO_APP
	leaq	.L.str.9(%rip), %rsi
	movl	$1, %eax
	movl	$1, %edx
	xorl	%r10d, %r10d
	xorl	%r8d, %r8d
	xorl	%r9d, %r9d
	#APP
	syscall
	#NO_APP
	movq	%rbx, %rdi
	callq	dhex
	leaq	.L.str.5(%rip), %rsi
	movl	$1, %eax
	movl	$2, %edi
	movl	$1, %edx
	xorl	%r10d, %r10d
	xorl	%r8d, %r8d
	xorl	%r9d, %r9d
	#APP
	syscall
	#NO_APP
	addq	$8, %rsp
	popq	%rbx
	popq	%r14
.LBB12_4:
	retq
.Lfunc_end12:
	.size	trace2, .Lfunc_end12-trace2
                                        # -- End function
	.p2align	4                               # -- Begin function try_dir
	.type	try_dir,@function
try_dir:                                # @try_dir
# %bb.0:
	pushq	%r15
	pushq	%r14
	pushq	%r12
	pushq	%rbx
	subq	$4104, %rsp                     # imm = 0x1008
	movq	%rdx, %rbx
	testq	%rsi, %rsi
	je	.LBB13_1
# %bb.2:
	movq	%rdi, %rax
	leaq	-1(%rsi), %rcx
	cmpq	$4094, %rcx                     # imm = 0xFFE
	movl	$4094, %r15d                    # imm = 0xFFE
	cmovbq	%rcx, %r15
	leaq	1(%r15), %r14
	movq	%rsp, %rdi
	movq	%rsi, %r12
	movq	%rax, %rsi
	movq	%r14, %rdx
	callq	memcpy@PLT
	movl	%r14d, %eax
	andl	$7, %eax
	cmpq	$8, %r12
	jae	.LBB13_14
# %bb.3:
	movl	$1, %edx
                                        # implicit-def: $rcx
	testq	%rax, %rax
	jne	.LBB13_6
	jmp	.LBB13_9
.LBB13_1:
	xorl	%r14d, %r14d
	jmp	.LBB13_12
.LBB13_14:
	movl	%r14d, %edx
	andl	$8184, %edx                     # imm = 0x1FF8
	xorl	%ecx, %ecx
	.p2align	4
.LBB13_15:                              # =>This Inner Loop Header: Depth=1
	addq	$8, %rcx
	cmpq	%rcx, %rdx
	jne	.LBB13_15
# %bb.4:
	leaq	1(%rcx), %rdx
	testq	%rax, %rax
	je	.LBB13_9
.LBB13_6:
	negq	%rax
	xorl	%ecx, %ecx
	.p2align	4
.LBB13_7:                               # =>This Inner Loop Header: Depth=1
	decq	%rcx
	cmpq	%rcx, %rax
	jne	.LBB13_7
# %bb.8:
	notq	%rcx
	addq	%rdx, %rcx
.LBB13_9:
	cmpb	$47, (%rsp,%r15)
	je	.LBB13_12
# %bb.10:
	cmpq	$4095, %rcx                     # imm = 0xFFF
	jae	.LBB13_12
# %bb.11:
	movb	$47, 1(%rsp,%r15)
	addq	$2, %r15
	movq	%r15, %r14
.LBB13_12:
	leaq	(%rsp,%r14), %rax
	movzbl	(%rbx), %ecx
	testb	%cl, %cl
	sete	%dl
	cmpq	$4095, %r14                     # imm = 0xFFF
	setae	%sil
	orb	%dl, %sil
	jne	.LBB13_13
# %bb.16:
	movl	$4096, %edx                     # imm = 0x1000
	subq	%r14, %rdx
	xorl	%edi, %edi
	.p2align	4
.LBB13_17:                              # =>This Inner Loop Header: Depth=1
	movb	%cl, (%rax,%rdi)
	leaq	2(%rdi), %rcx
	leaq	1(%rdi), %rsi
	cmpq	%rdx, %rcx
	jae	.LBB13_19
# %bb.18:                               #   in Loop: Header=BB13_17 Depth=1
	movzbl	1(%rbx,%rdi), %ecx
	movq	%rsi, %rdi
	testb	%cl, %cl
	jne	.LBB13_17
	jmp	.LBB13_19
.LBB13_13:
	xorl	%esi, %esi
.LBB13_19:
	movb	$0, (%rax,%rsi)
	addq	%rsi, %r14
	movb	$0, (%rsp,%r14)
	xorl	%r14d, %r14d
	movq	%rsp, %rsi
	movl	$257, %eax                      # imm = 0x101
	movq	$-100, %rdi
	xorl	%edx, %edx
	xorl	%r10d, %r10d
	xorl	%r8d, %r8d
	xorl	%r9d, %r9d
	#APP
	syscall
	#NO_APP
	testq	%rax, %rax
	js	.LBB13_28
# %bb.20:
	movslq	try_dir.nnames(%rip), %rcx
	cmpq	$31, %rcx
	jg	.LBB13_27
# %bb.21:
	movq	%rcx, %rdx
	shlq	$12, %rdx
	leaq	try_dir.names(%rip), %rbx
	addq	%rdx, %rbx
	movzbl	(%rsp), %esi
	testb	%sil, %sil
	je	.LBB13_22
# %bb.23:
	xorl	%edi, %edi
	.p2align	4
.LBB13_24:                              # =>This Inner Loop Header: Depth=1
	leaq	1(%rdi), %rdx
	movb	%sil, (%rbx,%rdi)
	cmpq	$4094, %rdx                     # imm = 0xFFE
	ja	.LBB13_26
# %bb.25:                               #   in Loop: Header=BB13_24 Depth=1
	movzbl	1(%rsp,%rdi), %esi
	movq	%rdx, %rdi
	testb	%sil, %sil
	jne	.LBB13_24
	jmp	.LBB13_26
.LBB13_22:
	xorl	%edx, %edx
.LBB13_26:
	movb	$0, (%rbx,%rdx)
	incl	%ecx
	movl	%ecx, try_dir.nnames(%rip)
.LBB13_27:
	movl	%eax, %edi
	movq	%rbx, %rsi
	movq	%rax, %rbx
	callq	load_from_fd
	movq	%rax, %r14
	movslq	%ebx, %rdi
	movl	$3, %eax
	xorl	%esi, %esi
	xorl	%edx, %edx
	xorl	%r10d, %r10d
	xorl	%r8d, %r8d
	xorl	%r9d, %r9d
	#APP
	syscall
	#NO_APP
.LBB13_28:
	movq	%r14, %rax
	addq	$4104, %rsp                     # imm = 0x1008
	popq	%rbx
	popq	%r12
	popq	%r14
	popq	%r15
	retq
.Lfunc_end13:
	.size	try_dir, .Lfunc_end13-try_dir
                                        # -- End function
	.p2align	4                               # -- Begin function apply_rela
	.type	apply_rela,@function
apply_rela:                             # @apply_rela
# %bb.0:
	pushq	%rbp
	pushq	%r15
	pushq	%r14
	pushq	%r13
	pushq	%r12
	pushq	%rbx
	subq	$56, %rsp
	movq	%rsi, 48(%rsp)                  # 8-byte Spill
	movq	%rdi, 8(%rsp)                   # 8-byte Spill
	testq	%rdx, %rdx
	je	.LBB14_42
# %bb.1:
	movq	%rdx, %r14
	xorl	%r13d, %r13d
	movq	%rdx, 32(%rsp)                  # 8-byte Spill
	jmp	.LBB14_2
	.p2align	4
.LBB14_7:                               #   in Loop: Header=BB14_2 Depth=1
	cmpl	$5, %ecx
	je	.LBB14_17
# %bb.8:                                #   in Loop: Header=BB14_2 Depth=1
	cmpl	$6, %ecx
	jne	.LBB14_9
.LBB14_6:                               #   in Loop: Header=BB14_2 Depth=1
	movq	%rdx, %r15
	movq	8(%rsp), %rdi                   # 8-byte Reload
	callq	resolve_reloc_symbol
	addq	%rbx, %rax
.LBB14_40:                              #   in Loop: Header=BB14_2 Depth=1
	movq	%rax, (%r15)
.LBB14_41:                              #   in Loop: Header=BB14_2 Depth=1
	incq	%r13
	cmpq	%r14, %r13
	je	.LBB14_42
.LBB14_2:                               # =>This Loop Header: Depth=1
                                        #     Child Loop BB14_19 Depth 2
                                        #       Child Loop BB14_23 Depth 3
                                        #         Child Loop BB14_28 Depth 4
	leaq	(,%r13,2), %rdx
	addq	%r13, %rdx
	movq	48(%rsp), %rdi                  # 8-byte Reload
	leaq	(%rdi,%rdx,8), %rsi
	movq	8(%rdi,%rdx,8), %rcx
	movq	16(%rdi,%rdx,8), %rbx
	movq	8(%rsp), %rax                   # 8-byte Reload
	movq	16(%rax), %rax
	movq	(%rdi,%rdx,8), %rdx
	addq	%rax, %rdx
	cmpl	$6, %ecx
	jg	.LBB14_10
# %bb.3:                                #   in Loop: Header=BB14_2 Depth=1
	cmpl	$4, %ecx
	jg	.LBB14_7
# %bb.4:                                #   in Loop: Header=BB14_2 Depth=1
	testl	%ecx, %ecx
	je	.LBB14_41
# %bb.5:                                #   in Loop: Header=BB14_2 Depth=1
	cmpl	$1, %ecx
	je	.LBB14_6
	jmp	.LBB14_9
	.p2align	4
.LBB14_10:                              #   in Loop: Header=BB14_2 Depth=1
	cmpl	$15, %ecx
	jg	.LBB14_14
# %bb.11:                               #   in Loop: Header=BB14_2 Depth=1
	cmpl	$7, %ecx
	je	.LBB14_6
# %bb.12:                               #   in Loop: Header=BB14_2 Depth=1
	cmpl	$8, %ecx
	jne	.LBB14_9
# %bb.13:                               #   in Loop: Header=BB14_2 Depth=1
	addq	%rax, %rbx
	movq	%rbx, (%rdx)
	jmp	.LBB14_41
.LBB14_14:                              #   in Loop: Header=BB14_2 Depth=1
	cmpl	$37, %ecx
	jne	.LBB14_15
# %bb.39:                               #   in Loop: Header=BB14_2 Depth=1
	movq	%rdx, %r15
	addq	%rax, %rbx
	callq	*%rbx
	jmp	.LBB14_40
.LBB14_17:                              #   in Loop: Header=BB14_2 Depth=1
	movq	%rdx, 40(%rsp)                  # 8-byte Spill
	movslq	g_nobjs(%rip), %rax
	testq	%rax, %rax
	jle	.LBB14_43
# %bb.18:                               #   in Loop: Header=BB14_2 Depth=1
	shrq	$32, %rcx
	movq	8(%rsp), %rsi                   # 8-byte Reload
	movq	48(%rsi), %rdx
	leaq	(%rcx,%rcx,2), %rcx
	movl	(%rdx,%rcx,8), %r14d
	addq	56(%rsi), %r14
	xorl	%edi, %edi
	movq	$0, 24(%rsp)                    # 8-byte Folded Spill
	movq	$0, 16(%rsp)                    # 8-byte Folded Spill
	leaq	g_objs(%rip), %rdx
	jmp	.LBB14_19
.LBB14_33:                              #   in Loop: Header=BB14_19 Depth=2
	andb	$-16, %r15b
	cmpb	$16, %r15b
	je	.LBB14_38
# %bb.34:                               #   in Loop: Header=BB14_19 Depth=2
	movq	24(%rsp), %rdx                  # 8-byte Reload
	testq	%rdx, %rdx
	movq	16(%rsp), %rsi                  # 8-byte Reload
	cmoveq	%r8, %rsi
	movq	%rsi, 16(%rsp)                  # 8-byte Spill
	cmoveq	%rcx, %rdx
	movq	%rdx, 24(%rsp)                  # 8-byte Spill
.LBB14_35:                              #   in Loop: Header=BB14_19 Depth=2
	leaq	g_objs(%rip), %rdx
.LBB14_36:                              #   in Loop: Header=BB14_19 Depth=2
	incq	%rdi
	cmpq	%rax, %rdi
	je	.LBB14_37
.LBB14_19:                              #   Parent Loop BB14_2 Depth=1
                                        # =>  This Loop Header: Depth=2
                                        #       Child Loop BB14_23 Depth 3
                                        #         Child Loop BB14_28 Depth 4
	movq	%rdi, %rcx
	shlq	$8, %rcx
	leaq	(%rcx,%rdi,8), %r8
	movq	48(%rdx,%r8), %r9
	testq	%r9, %r9
	je	.LBB14_36
# %bb.20:                               #   in Loop: Header=BB14_19 Depth=2
	addq	%rdx, %r8
	movq	56(%r8), %r10
	testq	%r10, %r10
	je	.LBB14_36
# %bb.21:                               #   in Loop: Header=BB14_19 Depth=2
	movq	64(%r8), %r11
	testq	%r11, %r11
	je	.LBB14_36
# %bb.22:                               #   in Loop: Header=BB14_19 Depth=2
	leaq	1(%r10), %rbx
	xorl	%edx, %edx
	jmp	.LBB14_23
.LBB14_26:                              #   in Loop: Header=BB14_23 Depth=3
	xorl	%ebp, %ebp
	movq	%r14, %rsi
.LBB14_31:                              #   in Loop: Header=BB14_23 Depth=3
	cmpb	(%rsi), %bpl
	je	.LBB14_33
.LBB14_32:                              #   in Loop: Header=BB14_23 Depth=3
	incq	%rdx
	cmpq	%r11, %rdx
	je	.LBB14_35
.LBB14_23:                              #   Parent Loop BB14_2 Depth=1
                                        #     Parent Loop BB14_19 Depth=2
                                        # =>    This Loop Header: Depth=3
                                        #         Child Loop BB14_28 Depth 4
	leaq	(%rdx,%rdx,2), %rcx
	cmpw	$0, 6(%r9,%rcx,8)
	je	.LBB14_32
# %bb.24:                               #   in Loop: Header=BB14_23 Depth=3
	leaq	(%r9,%rcx,8), %rcx
	movzbl	4(%rcx), %r15d
	movl	%r15d, %esi
	shrb	$4, %sil
	addb	$-3, %sil
	cmpb	$-2, %sil
	jb	.LBB14_32
# %bb.25:                               #   in Loop: Header=BB14_23 Depth=3
	movl	(%rcx), %r12d
	movzbl	(%r10,%r12), %ebp
	testb	%bpl, %bpl
	je	.LBB14_26
# %bb.27:                               #   in Loop: Header=BB14_23 Depth=3
	addq	%rbx, %r12
	movq	%r14, %rsi
	.p2align	4
.LBB14_28:                              #   Parent Loop BB14_2 Depth=1
                                        #     Parent Loop BB14_19 Depth=2
                                        #       Parent Loop BB14_23 Depth=3
                                        # =>      This Inner Loop Header: Depth=4
	cmpb	(%rsi), %bpl
	jne	.LBB14_31
# %bb.29:                               #   in Loop: Header=BB14_28 Depth=4
	incq	%rsi
	movzbl	(%r12), %ebp
	incq	%r12
	testb	%bpl, %bpl
	jne	.LBB14_28
# %bb.30:                               #   in Loop: Header=BB14_23 Depth=3
	xorl	%ebp, %ebp
	jmp	.LBB14_31
.LBB14_37:                              #   in Loop: Header=BB14_2 Depth=1
	movq	24(%rsp), %rcx                  # 8-byte Reload
	testq	%rcx, %rcx
	movq	16(%rsp), %r8                   # 8-byte Reload
	je	.LBB14_43
.LBB14_38:                              #   in Loop: Header=BB14_2 Depth=1
	movq	8(%rcx), %rsi
	addq	16(%r8), %rsi
	movq	16(%rcx), %rdx
	movq	40(%rsp), %rdi                  # 8-byte Reload
	callq	memcpy@PLT
	movq	32(%rsp), %r14                  # 8-byte Reload
	jmp	.LBB14_41
.LBB14_42:
	addq	$56, %rsp
	popq	%rbx
	popq	%r12
	popq	%r13
	popq	%r14
	popq	%r15
	popq	%rbp
	retq
.LBB14_15:
	leal	-16(%rcx), %eax
	cmpl	$3, %eax
	jae	.LBB14_9
# %bb.16:
	leaq	.L.str.32(%rip), %rdi
	callq	die
.LBB14_43:
	leaq	.L.str.31(%rip), %rdi
	callq	die
.LBB14_9:
	movl	%ecx, %esi
	leaq	.L.str.33(%rip), %rdi
	callq	die2
.Lfunc_end14:
	.size	apply_rela, .Lfunc_end14-apply_rela
                                        # -- End function
	.p2align	4                               # -- Begin function resolve_reloc_symbol
	.type	resolve_reloc_symbol,@function
resolve_reloc_symbol:                   # @resolve_reloc_symbol
# %bb.0:
	pushq	%rbp
	pushq	%r15
	pushq	%r14
	pushq	%r13
	pushq	%r12
	pushq	%rbx
	subq	$24, %rsp
	movl	12(%rsi), %ecx
	movq	48(%rdi), %rax
	leaq	(%rcx,%rcx,2), %rcx
	movl	(%rax,%rcx,8), %edx
	addq	56(%rdi), %rdx
	movq	%rdx, 16(%rsp)                  # 8-byte Spill
	movzbl	4(%rax,%rcx,8), %r8d
	cmpb	$15, %r8b
	ja	.LBB15_3
# %bb.1:
	leaq	(%rax,%rcx,8), %rax
	cmpw	$0, 6(%rax)
	je	.LBB15_3
# %bb.2:
	movq	8(%rax), %rax
	addq	16(%rdi), %rax
	jmp	.LBB15_29
.LBB15_3:
	movslq	g_nobjs(%rip), %rsi
	testq	%rsi, %rsi
	jle	.LBB15_27
# %bb.4:
	movb	%r8b, 7(%rsp)                   # 1-byte Spill
	xorl	%edi, %edi
	leaq	g_objs(%rip), %r8
	xorl	%edx, %edx
	movq	$0, 8(%rsp)                     # 8-byte Folded Spill
	jmp	.LBB15_5
.LBB15_19:                              #   in Loop: Header=BB15_5 Depth=1
	movl	%ebp, %eax
	andb	$-16, %al
	cmpb	$16, %al
	je	.LBB15_20
# %bb.21:                               #   in Loop: Header=BB15_5 Depth=1
	testq	%rdx, %rdx
	movq	8(%rsp), %rax                   # 8-byte Reload
	cmoveq	%r9, %rax
	movq	%rax, 8(%rsp)                   # 8-byte Spill
	cmoveq	%r13, %rdx
	.p2align	4
.LBB15_22:                              #   in Loop: Header=BB15_5 Depth=1
	incq	%rdi
	cmpq	%rsi, %rdi
	je	.LBB15_23
.LBB15_5:                               # =>This Loop Header: Depth=1
                                        #     Child Loop BB15_9 Depth 2
                                        #       Child Loop BB15_14 Depth 3
	movq	%rdi, %rax
	shlq	$8, %rax
	leaq	(%rax,%rdi,8), %r9
	movq	48(%r8,%r9), %r10
	testq	%r10, %r10
	je	.LBB15_22
# %bb.6:                                #   in Loop: Header=BB15_5 Depth=1
	addq	%r8, %r9
	movq	56(%r9), %r11
	testq	%r11, %r11
	je	.LBB15_22
# %bb.7:                                #   in Loop: Header=BB15_5 Depth=1
	movq	64(%r9), %r14
	testq	%r14, %r14
	je	.LBB15_22
# %bb.8:                                #   in Loop: Header=BB15_5 Depth=1
	leaq	1(%r11), %r15
	xorl	%r12d, %r12d
	jmp	.LBB15_9
.LBB15_12:                              #   in Loop: Header=BB15_9 Depth=2
	xorl	%ecx, %ecx
	movq	16(%rsp), %rbx                  # 8-byte Reload
.LBB15_17:                              #   in Loop: Header=BB15_9 Depth=2
	cmpb	(%rbx), %cl
	je	.LBB15_19
.LBB15_18:                              #   in Loop: Header=BB15_9 Depth=2
	incq	%r12
	cmpq	%r14, %r12
	je	.LBB15_22
.LBB15_9:                               #   Parent Loop BB15_5 Depth=1
                                        # =>  This Loop Header: Depth=2
                                        #       Child Loop BB15_14 Depth 3
	leaq	(%r12,%r12,2), %rax
	cmpw	$0, 6(%r10,%rax,8)
	je	.LBB15_18
# %bb.10:                               #   in Loop: Header=BB15_9 Depth=2
	leaq	(%r10,%rax,8), %r13
	movzbl	4(%r13), %ebp
	movl	%ebp, %eax
	shrb	$4, %al
	addb	$-3, %al
	cmpb	$-2, %al
	jb	.LBB15_18
# %bb.11:                               #   in Loop: Header=BB15_9 Depth=2
	movl	(%r13), %eax
	movzbl	(%r11,%rax), %ecx
	testb	%cl, %cl
	je	.LBB15_12
# %bb.13:                               #   in Loop: Header=BB15_9 Depth=2
	addq	%r15, %rax
	movq	16(%rsp), %rbx                  # 8-byte Reload
	.p2align	4
.LBB15_14:                              #   Parent Loop BB15_5 Depth=1
                                        #     Parent Loop BB15_9 Depth=2
                                        # =>    This Inner Loop Header: Depth=3
	cmpb	(%rbx), %cl
	jne	.LBB15_17
# %bb.15:                               #   in Loop: Header=BB15_14 Depth=3
	incq	%rbx
	movzbl	(%rax), %ecx
	incq	%rax
	testb	%cl, %cl
	jne	.LBB15_14
# %bb.16:                               #   in Loop: Header=BB15_9 Depth=2
	xorl	%ecx, %ecx
	jmp	.LBB15_17
.LBB15_23:
	testq	%rdx, %rdx
	movzbl	7(%rsp), %r8d                   # 1-byte Folded Reload
	je	.LBB15_27
# %bb.24:
	movzbl	4(%rdx), %ebp
	movq	8(%rsp), %r9                    # 8-byte Reload
.LBB15_25:
	movq	8(%rdx), %rax
	addq	16(%r9), %rax
	andb	$15, %bpl
	cmpb	$10, %bpl
	jne	.LBB15_29
# %bb.26:
	addq	$24, %rsp
	popq	%rbx
	popq	%r12
	popq	%r13
	popq	%r14
	popq	%r15
	popq	%rbp
	jmpq	*%rax                           # TAILCALL
.LBB15_27:
	andb	$-16, %r8b
	cmpb	$32, %r8b
	jne	.LBB15_30
# %bb.28:
	xorl	%eax, %eax
.LBB15_29:
	addq	$24, %rsp
	popq	%rbx
	popq	%r12
	popq	%r13
	popq	%r14
	popq	%r15
	popq	%rbp
	retq
.LBB15_20:
	movq	%r13, %rdx
	jmp	.LBB15_25
.LBB15_30:
	leaq	.L.str.34(%rip), %rdi
	callq	dstr
	movq	16(%rsp), %rdi                  # 8-byte Reload
	callq	dstr
	leaq	.L.str.5(%rip), %rdi
	callq	dstr
	callq	sys_exit
.Lfunc_end15:
	.size	resolve_reloc_symbol, .Lfunc_end15-resolve_reloc_symbol
                                        # -- End function
	.type	.L.str,@object                  # @.str
	.section	.rodata.str1.1,"aMS",@progbits,1
.L.str:
	.asciz	"LDLAB_DEBUG=1"
	.size	.L.str, 14

	.type	g_debug,@object                 # @g_debug
	.local	g_debug
	.comm	g_debug,1,4
	.type	g_env_libpath,@object           # @g_env_libpath
	.local	g_env_libpath
	.comm	g_env_libpath,8,8
	.type	.L.str.2,@object                # @.str.2
.L.str.2:
	.asciz	"usage: loader ./program [args...]   (set LDLAB_DEBUG=1 to trace)"
	.size	.L.str.2, 65

	.type	g_prog_dir,@object              # @g_prog_dir
	.local	g_prog_dir
	.comm	g_prog_dir,4096,16
	.type	.L.str.3,@object                # @.str.3
.L.str.3:
	.asciz	"loading main program"
	.size	.L.str.3, 21

	.type	g_nobjs,@object                 # @g_nobjs
	.local	g_nobjs
	.comm	g_nobjs,4,4
	.type	g_objs,@object                  # @g_objs
	.local	g_objs
	.comm	g_objs,8448,16
	.type	.L.str.4,@object                # @.str.4
.L.str.4:
	.asciz	"[ld] DT_NEEDED "
	.size	.L.str.4, 16

	.type	.L.str.5,@object                # @.str.5
.L.str.5:
	.asciz	"\n"
	.size	.L.str.5, 2

	.type	.L.str.6,@object                # @.str.6
.L.str.6:
	.asciz	"loader: "
	.size	.L.str.6, 9

	.type	.L.str.7,@object                # @.str.7
.L.str.7:
	.asciz	"[ld] "
	.size	.L.str.7, 6

	.type	.L.str.8,@object                # @.str.8
.L.str.8:
	.asciz	"cannot open object"
	.size	.L.str.8, 19

	.type	.L.str.9,@object                # @.str.9
.L.str.9:
	.asciz	" "
	.size	.L.str.9, 2

	.type	dhex.H,@object                  # @dhex.H
	.section	.rodata.str1.16,"aMS",@progbits,1
	.p2align	4, 0x0
dhex.H:
	.asciz	"0123456789abcdef"
	.size	dhex.H, 17

	.type	.L.str.10,@object               # @.str.10
	.section	.rodata.str1.1,"aMS",@progbits,1
.L.str.10:
	.asciz	"too many shared objects"
	.size	.L.str.10, 24

	.type	.L.str.11,@object               # @.str.11
.L.str.11:
	.asciz	"short read of ELF header"
	.size	.L.str.11, 25

	.type	.L.str.12,@object               # @.str.12
.L.str.12:
	.asciz	"not an ELF file (bad magic)"
	.size	.L.str.12, 28

	.type	.L.str.13,@object               # @.str.13
.L.str.13:
	.asciz	"not ELFCLASS64"
	.size	.L.str.13, 15

	.type	.L.str.14,@object               # @.str.14
.L.str.14:
	.asciz	"not little-endian"
	.size	.L.str.14, 18

	.type	.L.str.15,@object               # @.str.15
.L.str.15:
	.asciz	"not x86-64"
	.size	.L.str.15, 11

	.type	.L.str.16,@object               # @.str.16
.L.str.16:
	.asciz	"not ET_DYN/ET_EXEC"
	.size	.L.str.16, 19

	.type	.L.str.17,@object               # @.str.17
.L.str.17:
	.asciz	"unexpected e_phentsize"
	.size	.L.str.17, 23

	.type	.L.str.18,@object               # @.str.18
.L.str.18:
	.asciz	"implausible e_phnum"
	.size	.L.str.18, 20

	.type	load_from_fd.ph,@object         # @load_from_fd.ph
	.local	load_from_fd.ph
	.comm	load_from_fd.ph,14336,16
	.type	.L.str.19,@object               # @.str.19
.L.str.19:
	.asciz	"short read of program headers"
	.size	.L.str.19, 30

	.type	.L.str.20,@object               # @.str.20
.L.str.20:
	.asciz	"note: PT_TLS present (TLS is not set up by this loader)"
	.size	.L.str.20, 56

	.type	.L.str.21,@object               # @.str.21
.L.str.21:
	.asciz	"no PT_LOAD segments"
	.size	.L.str.21, 20

	.type	.L.str.22,@object               # @.str.22
.L.str.22:
	.asciz	"reserve mmap failed"
	.size	.L.str.22, 20

	.type	.L.str.23,@object               # @.str.23
.L.str.23:
	.asciz	"load bias"
	.size	.L.str.23, 10

	.type	.L.str.24,@object               # @.str.24
.L.str.24:
	.asciz	"segment mmap failed"
	.size	.L.str.24, 20

	.type	.L.str.25,@object               # @.str.25
.L.str.25:
	.asciz	"bss mmap failed"
	.size	.L.str.25, 16

	.type	.L.str.26,@object               # @.str.26
.L.str.26:
	.asciz	"too many DT_NEEDED entries"
	.size	.L.str.26, 27

	.type	.L.str.27,@object               # @.str.27
.L.str.27:
	.asciz	"PLT relocations are DT_REL, not DT_RELA (unsupported)"
	.size	.L.str.27, 54

	.type	.L.str.28,@object               # @.str.28
.L.str.28:
	.asciz	"."
	.size	.L.str.28, 2

	.type	.L.str.29,@object               # @.str.29
.L.str.29:
	.asciz	"loader: cannot find shared object: "
	.size	.L.str.29, 36

	.type	try_dir.names,@object           # @try_dir.names
	.local	try_dir.names
	.comm	try_dir.names,131072,16
	.type	try_dir.nnames,@object          # @try_dir.nnames
	.local	try_dir.nnames
	.comm	try_dir.nnames,4,4
	.type	.L.str.30,@object               # @.str.30
.L.str.30:
	.asciz	"relocating"
	.size	.L.str.30, 11

	.type	.L.str.31,@object               # @.str.31
.L.str.31:
	.asciz	"R_X86_64_COPY: undefined symbol"
	.size	.L.str.31, 32

	.type	.L.str.32,@object               # @.str.32
.L.str.32:
	.asciz	"TLS relocation encountered (unsupported in this teaching core)"
	.size	.L.str.32, 63

	.type	.L.str.33,@object               # @.str.33
.L.str.33:
	.asciz	"unhandled relocation type"
	.size	.L.str.33, 26

	.type	.L.str.34,@object               # @.str.34
.L.str.34:
	.asciz	"loader: undefined symbol: "
	.size	.L.str.34, 27

	.type	.L.str.35,@object               # @.str.35
.L.str.35:
	.asciz	"relro mprotect failed"
	.size	.L.str.35, 22

	.type	.L.str.36,@object               # @.str.36
.L.str.36:
	.asciz	"applied RELRO (GOT now read-only)"
	.size	.L.str.36, 34

	.type	.L.str.37,@object               # @.str.37
.L.str.37:
	.asciz	"DT_INIT"
	.size	.L.str.37, 8

	.type	.L.str.38,@object               # @.str.38
.L.str.38:
	.asciz	"stack mmap failed"
	.size	.L.str.38, 18

	.type	.L.str.39,@object               # @.str.39
.L.str.39:
	.asciz	"entry"
	.size	.L.str.39, 6

	.type	.L.str.40,@object               # @.str.40
.L.str.40:
	.asciz	"new sp"
	.size	.L.str.40, 7

	.ident	"clang version 20.1.8"
	.section	".note.GNU-stack","",@progbits
	.addrsig
	.addrsig_sym memset
	.addrsig_sym memcpy
	.addrsig_sym g_prog_dir
	.addrsig_sym g_objs
	.addrsig_sym load_from_fd.ph
	.addrsig_sym try_dir.names
