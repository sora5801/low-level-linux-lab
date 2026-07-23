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
	pushq	%rbp
	movq	%rsp, %rbp
	movq	%rdi, %rax
	testq	%rdx, %rdx
	je	.LBB0_3
# %bb.1:
	xorl	%ecx, %ecx
	.p2align	4
.LBB0_2:                                # =>This Inner Loop Header: Depth=1
	movb	%sil, (%rax,%rcx)
	incq	%rcx
	cmpq	%rcx, %rdx
	jne	.LBB0_2
.LBB0_3:
	popq	%rbp
	retq
.Lfunc_end0:
	.size	memset, .Lfunc_end0-memset
                                        # -- End function
	.globl	memcpy                          # -- Begin function memcpy
	.p2align	4
	.type	memcpy,@function
memcpy:                                 # @memcpy
# %bb.0:
	pushq	%rbp
	movq	%rsp, %rbp
	movq	%rdi, %rax
	testq	%rdx, %rdx
	je	.LBB1_3
# %bb.1:
	xorl	%ecx, %ecx
	.p2align	4
.LBB1_2:                                # =>This Inner Loop Header: Depth=1
	movzbl	(%rsi,%rcx), %edi
	movb	%dil, (%rax,%rcx)
	incq	%rcx
	cmpq	%rcx, %rdx
	jne	.LBB1_2
.LBB1_3:
	popq	%rbp
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
	movq	%rsp, %rbp
	pushq	%r15
	pushq	%r14
	pushq	%r13
	pushq	%r12
	pushq	%rbx
	subq	$72, %rsp
	movq	(%rdi), %r11
	leaq	8(%rdi), %r10
	movq	%rdi, %r14
	leaq	(%rdi,%r11,8), %rbx
	addq	$16, %rbx
	movq	%rbx, -48(%rbp)                 # 8-byte Spill
	.p2align	4
.LBB2_1:                                # =>This Inner Loop Header: Depth=1
	cmpq	$0, (%rbx)
	leaq	8(%rbx), %rbx
	jne	.LBB2_1
# %bb.2:
	movq	-48(%rbp), %rax                 # 8-byte Reload
	movq	(%rax), %rax
	testq	%rax, %rax
	jne	.LBB2_3
.LBB2_20:
	cmpq	$2, %r11
	jge	.LBB2_21
# %bb.139:
	leaq	.L.str.2(%rip), %rdi
	callq	die
.LBB2_3:
	leaq	.L.str(%rip), %rcx
	leaq	.L.str.1(%rip), %rdx
	movq	-48(%rbp), %rsi                 # 8-byte Reload
	jmp	.LBB2_4
.LBB2_18:                               #   in Loop: Header=BB2_4 Depth=1
	movq	%rdi, g_env_libpath(%rip)
	.p2align	4
.LBB2_19:                               #   in Loop: Header=BB2_4 Depth=1
	movq	8(%rsi), %rax
	addq	$8, %rsi
	testq	%rax, %rax
	je	.LBB2_20
.LBB2_4:                                # =>This Loop Header: Depth=1
                                        #     Child Loop BB2_11 Depth 2
                                        #     Child Loop BB2_16 Depth 2
	movzbl	(%rax), %edi
	cmpb	$76, %dil
	jne	.LBB2_19
# %bb.5:                                #   in Loop: Header=BB2_4 Depth=1
	cmpb	$68, 1(%rax)
	jne	.LBB2_19
# %bb.6:                                #   in Loop: Header=BB2_4 Depth=1
	cmpb	$76, 2(%rax)
	jne	.LBB2_19
# %bb.7:                                #   in Loop: Header=BB2_4 Depth=1
	cmpb	$65, 3(%rax)
	jne	.LBB2_19
# %bb.8:                                #   in Loop: Header=BB2_4 Depth=1
	cmpb	$66, 4(%rax)
	jne	.LBB2_19
# %bb.9:                                #   in Loop: Header=BB2_4 Depth=1
	cmpb	$95, 5(%rax)
	jne	.LBB2_19
# %bb.10:                               #   in Loop: Header=BB2_4 Depth=1
	leaq	1(%rax), %r9
	movq	%rcx, %r8
	.p2align	4
.LBB2_11:                               #   Parent Loop BB2_4 Depth=1
                                        # =>  This Inner Loop Header: Depth=2
	cmpb	(%r8), %dil
	jne	.LBB2_13
# %bb.12:                               #   in Loop: Header=BB2_11 Depth=2
	incq	%r8
	movzbl	(%r9), %edi
	incq	%r9
	testb	%dil, %dil
	jne	.LBB2_11
.LBB2_13:                               #   in Loop: Header=BB2_4 Depth=1
	cmpb	(%r8), %dil
	jne	.LBB2_15
# %bb.14:                               #   in Loop: Header=BB2_4 Depth=1
	movb	$1, g_debug(%rip)
.LBB2_15:                               #   in Loop: Header=BB2_4 Depth=1
	leaq	19(%rax), %rdi
	movb	$76, %r9b
	xorl	%r8d, %r8d
	.p2align	4
.LBB2_16:                               #   Parent Loop BB2_4 Depth=1
                                        # =>  This Inner Loop Header: Depth=2
	cmpb	%r9b, (%rax,%r8)
	jne	.LBB2_19
# %bb.17:                               #   in Loop: Header=BB2_16 Depth=2
	movzbl	1(%r8,%rdx), %r9d
	incq	%r8
	cmpq	$19, %r8
	jne	.LBB2_16
	jmp	.LBB2_18
.LBB2_21:
	movq	%rbx, -88(%rbp)                 # 8-byte Spill
	movq	%r11, -64(%rbp)                 # 8-byte Spill
	movq	%r10, -96(%rbp)                 # 8-byte Spill
	movq	16(%r14), %rbx
	addq	$16, %r14
	movq	%r14, -72(%rbp)                 # 8-byte Spill
	movq	$-1, %rax
	xorl	%ecx, %ecx
	jmp	.LBB2_22
	.p2align	4
.LBB2_43:                               #   in Loop: Header=BB2_22 Depth=1
	movq	%rcx, %rax
.LBB2_44:                               #   in Loop: Header=BB2_22 Depth=1
	incq	%rcx
.LBB2_22:                               # =>This Inner Loop Header: Depth=1
	movzbl	(%rbx,%rcx), %edx
	cmpl	$47, %edx
	je	.LBB2_43
# %bb.23:                               #   in Loop: Header=BB2_22 Depth=1
	testl	%edx, %edx
	jne	.LBB2_44
# %bb.24:
	testq	%rax, %rax
	jle	.LBB2_28
# %bb.25:
	cmpq	$4095, %rax                     # imm = 0xFFF
	movl	$4095, %ecx                     # imm = 0xFFF
	cmovbq	%rax, %rcx
	xorl	%eax, %eax
	leaq	g_prog_dir(%rip), %rdx
	.p2align	4
.LBB2_26:                               # =>This Inner Loop Header: Depth=1
	movzbl	(%rbx,%rax), %esi
	movb	%sil, (%rax,%rdx)
	incq	%rax
	cmpq	%rax, %rcx
	jne	.LBB2_26
# %bb.27:
	movb	$0, (%rcx,%rdx)
.LBB2_28:
	cmpb	$1, g_debug(%rip)
	jne	.LBB2_30
# %bb.29:
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
.LBB2_30:
	movq	%rbx, %rdi
	callq	load_path
	movq	%rax, -80(%rbp)                 # 8-byte Spill
	cmpl	$0, g_nobjs(%rip)
	jle	.LBB2_45
# %bb.31:
	xorl	%ecx, %ecx
	leaq	g_objs(%rip), %r15
	leaq	.L.str.4(%rip), %rbx
	leaq	.L.str.5(%rip), %r14
	jmp	.LBB2_32
	.p2align	4
.LBB2_107:                              #   in Loop: Header=BB2_32 Depth=1
	movq	-104(%rbp), %rcx                # 8-byte Reload
	incq	%rcx
	movslq	g_nobjs(%rip), %rax
	cmpq	%rax, %rcx
	jge	.LBB2_45
.LBB2_32:                               # =>This Loop Header: Depth=1
                                        #     Child Loop BB2_34 Depth 2
                                        #       Child Loop BB2_36 Depth 3
                                        #       Child Loop BB2_40 Depth 3
                                        #         Child Loop BB2_57 Depth 4
                                        #         Child Loop BB2_64 Depth 4
                                        #       Child Loop BB2_69 Depth 3
                                        #       Child Loop BB2_77 Depth 3
                                        #         Child Loop BB2_80 Depth 4
                                        #       Child Loop BB2_90 Depth 3
                                        #         Child Loop BB2_93 Depth 4
                                        #       Child Loop BB2_103 Depth 3
	movq	%rcx, %rax
	shlq	$8, %rax
	movq	%rcx, -104(%rbp)                # 8-byte Spill
	leaq	(%rax,%rcx,8), %rcx
	cmpl	$0, 248(%r15,%rcx)
	jle	.LBB2_107
# %bb.33:                               #   in Loop: Header=BB2_32 Depth=1
	addq	%r15, %rcx
	xorl	%r12d, %r12d
	movq	%rcx, -56(%rbp)                 # 8-byte Spill
	jmp	.LBB2_34
	.p2align	4
.LBB2_72:                               #   in Loop: Header=BB2_34 Depth=2
	movq	%r13, %rdi
	callq	load_path
.LBB2_106:                              #   in Loop: Header=BB2_34 Depth=2
	incq	%r12
	movq	-56(%rbp), %rcx                 # 8-byte Reload
	movslq	248(%rcx), %rax
	cmpq	%rax, %r12
	jge	.LBB2_107
.LBB2_34:                               #   Parent Loop BB2_32 Depth=1
                                        # =>  This Loop Header: Depth=2
                                        #       Child Loop BB2_36 Depth 3
                                        #       Child Loop BB2_40 Depth 3
                                        #         Child Loop BB2_57 Depth 4
                                        #         Child Loop BB2_64 Depth 4
                                        #       Child Loop BB2_69 Depth 3
                                        #       Child Loop BB2_77 Depth 3
                                        #         Child Loop BB2_80 Depth 4
                                        #       Child Loop BB2_90 Depth 3
                                        #         Child Loop BB2_93 Depth 4
                                        #       Child Loop BB2_103 Depth 3
	movl	184(%rcx,%r12,4), %r13d
	addq	56(%rcx), %r13
	cmpb	$1, g_debug(%rip)
	jne	.LBB2_38
# %bb.35:                               #   in Loop: Header=BB2_34 Depth=2
	movl	$1, %eax
	movl	$2, %edi
	movl	$15, %edx
	movq	%rbx, %rsi
	xorl	%r10d, %r10d
	xorl	%r8d, %r8d
	xorl	%r9d, %r9d
	#APP
	syscall
	#NO_APP
	movq	$-1, %rdx
	.p2align	4
.LBB2_36:                               #   Parent Loop BB2_32 Depth=1
                                        #     Parent Loop BB2_34 Depth=2
                                        # =>    This Inner Loop Header: Depth=3
	cmpb	$0, 1(%r13,%rdx)
	leaq	1(%rdx), %rdx
	jne	.LBB2_36
# %bb.37:                               #   in Loop: Header=BB2_34 Depth=2
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
	movq	%r14, %rsi
	xorl	%r10d, %r10d
	xorl	%r8d, %r8d
	xorl	%r9d, %r9d
	#APP
	syscall
	#NO_APP
.LBB2_38:                               #   in Loop: Header=BB2_34 Depth=2
	movslq	g_nobjs(%rip), %rax
	testq	%rax, %rax
	jle	.LBB2_68
# %bb.39:                               #   in Loop: Header=BB2_34 Depth=2
	xorl	%ecx, %ecx
	jmp	.LBB2_40
	.p2align	4
.LBB2_62:                               #   in Loop: Header=BB2_40 Depth=3
	movq	%r13, %rdi
.LBB2_66:                               #   in Loop: Header=BB2_40 Depth=3
	cmpb	(%rdi), %dl
	je	.LBB2_106
.LBB2_67:                               #   in Loop: Header=BB2_40 Depth=3
	incq	%rcx
	cmpq	%rax, %rcx
	je	.LBB2_68
.LBB2_40:                               #   Parent Loop BB2_32 Depth=1
                                        #     Parent Loop BB2_34 Depth=2
                                        # =>    This Loop Header: Depth=3
                                        #         Child Loop BB2_57 Depth 4
                                        #         Child Loop BB2_64 Depth 4
	movq	%rcx, %rdx
	shlq	$8, %rdx
	leaq	(%rdx,%rcx,8), %rdx
	movq	8(%r15,%rdx), %rdi
	testq	%rdi, %rdi
	je	.LBB2_60
# %bb.41:                               #   in Loop: Header=BB2_40 Depth=3
	movzbl	(%rdi), %esi
	testb	%sil, %sil
	je	.LBB2_42
# %bb.56:                               #   in Loop: Header=BB2_40 Depth=3
	incq	%rdi
	movq	%r13, %r8
	.p2align	4
.LBB2_57:                               #   Parent Loop BB2_32 Depth=1
                                        #     Parent Loop BB2_34 Depth=2
                                        #       Parent Loop BB2_40 Depth=3
                                        # =>      This Inner Loop Header: Depth=4
	cmpb	(%r8), %sil
	jne	.LBB2_59
# %bb.58:                               #   in Loop: Header=BB2_57 Depth=4
	incq	%r8
	movzbl	(%rdi), %esi
	incq	%rdi
	testb	%sil, %sil
	jne	.LBB2_57
	jmp	.LBB2_59
	.p2align	4
.LBB2_42:                               #   in Loop: Header=BB2_40 Depth=3
	movq	%r13, %r8
.LBB2_59:                               #   in Loop: Header=BB2_40 Depth=3
	cmpb	(%r8), %sil
	je	.LBB2_106
.LBB2_60:                               #   in Loop: Header=BB2_40 Depth=3
	addq	%r15, %rdx
	movq	(%rdx), %rsi
	testq	%rsi, %rsi
	je	.LBB2_67
# %bb.61:                               #   in Loop: Header=BB2_40 Depth=3
	movzbl	(%rsi), %edx
	testb	%dl, %dl
	je	.LBB2_62
# %bb.63:                               #   in Loop: Header=BB2_40 Depth=3
	incq	%rsi
	movq	%r13, %rdi
	.p2align	4
.LBB2_64:                               #   Parent Loop BB2_32 Depth=1
                                        #     Parent Loop BB2_34 Depth=2
                                        #       Parent Loop BB2_40 Depth=3
                                        # =>      This Inner Loop Header: Depth=4
	cmpb	(%rdi), %dl
	jne	.LBB2_66
# %bb.65:                               #   in Loop: Header=BB2_64 Depth=4
	incq	%rdi
	movzbl	(%rsi), %edx
	incq	%rsi
	testb	%dl, %dl
	jne	.LBB2_64
	jmp	.LBB2_66
	.p2align	4
.LBB2_68:                               #   in Loop: Header=BB2_34 Depth=2
	movq	$-1, %rax
	xorl	%ecx, %ecx
	jmp	.LBB2_69
	.p2align	4
.LBB2_73:                               #   in Loop: Header=BB2_69 Depth=3
	movq	%rcx, %rax
.LBB2_74:                               #   in Loop: Header=BB2_69 Depth=3
	incq	%rcx
.LBB2_69:                               #   Parent Loop BB2_32 Depth=1
                                        #     Parent Loop BB2_34 Depth=2
                                        # =>    This Inner Loop Header: Depth=3
	movzbl	(%r13,%rcx), %edx
	cmpl	$47, %edx
	je	.LBB2_73
# %bb.70:                               #   in Loop: Header=BB2_69 Depth=3
	testl	%edx, %edx
	jne	.LBB2_74
# %bb.71:                               #   in Loop: Header=BB2_34 Depth=2
	testq	%rax, %rax
	jns	.LBB2_72
# %bb.75:                               #   in Loop: Header=BB2_34 Depth=2
	movq	-56(%rbp), %rax                 # 8-byte Reload
	movq	176(%rax), %rdi
	testq	%rdi, %rdi
	je	.LBB2_88
# %bb.76:                               #   in Loop: Header=BB2_34 Depth=2
                                        # implicit-def: $r14
	jmp	.LBB2_77
	.p2align	4
.LBB2_85:                               #   in Loop: Header=BB2_77 Depth=3
	xorl	%ecx, %ecx
	cmpb	$58, (%rbx)
	sete	%cl
	addq	%rcx, %rbx
.LBB2_86:                               #   in Loop: Header=BB2_77 Depth=3
	movq	%rbx, %rdi
	testq	%rax, %rax
	jne	.LBB2_87
.LBB2_77:                               #   Parent Loop BB2_32 Depth=1
                                        #     Parent Loop BB2_34 Depth=2
                                        # =>    This Loop Header: Depth=3
                                        #         Child Loop BB2_80 Depth 4
	cmpb	$0, (%rdi)
	je	.LBB2_78
# %bb.79:                               #   in Loop: Header=BB2_77 Depth=3
	xorl	%esi, %esi
	movq	%rdi, %rbx
	.p2align	4
.LBB2_80:                               #   Parent Loop BB2_32 Depth=1
                                        #     Parent Loop BB2_34 Depth=2
                                        #       Parent Loop BB2_77 Depth=3
                                        # =>      This Inner Loop Header: Depth=4
	movzbl	(%rbx), %eax
	testl	%eax, %eax
	je	.LBB2_83
# %bb.81:                               #   in Loop: Header=BB2_80 Depth=4
	cmpl	$58, %eax
	je	.LBB2_83
# %bb.82:                               #   in Loop: Header=BB2_80 Depth=4
	incq	%rbx
	incq	%rsi
	jmp	.LBB2_80
	.p2align	4
.LBB2_83:                               #   in Loop: Header=BB2_77 Depth=3
	movq	%r13, %rdx
	callq	try_dir
	testq	%rax, %rax
	je	.LBB2_85
# %bb.84:                               #   in Loop: Header=BB2_77 Depth=3
	movq	%rax, %r14
	jmp	.LBB2_86
.LBB2_78:                               #   in Loop: Header=BB2_34 Depth=2
	xorl	%r14d, %r14d
.LBB2_87:                               #   in Loop: Header=BB2_34 Depth=2
	testq	%r14, %r14
	leaq	.L.str.4(%rip), %rbx
	leaq	.L.str.5(%rip), %r14
	jne	.LBB2_106
.LBB2_88:                               #   in Loop: Header=BB2_34 Depth=2
	movq	g_env_libpath(%rip), %rdi
	testq	%rdi, %rdi
	je	.LBB2_101
# %bb.89:                               #   in Loop: Header=BB2_34 Depth=2
                                        # implicit-def: $r14
	jmp	.LBB2_90
	.p2align	4
.LBB2_96:                               #   in Loop: Header=BB2_90 Depth=3
	movq	%r13, %rdx
	callq	try_dir
	testq	%rax, %rax
	je	.LBB2_98
# %bb.97:                               #   in Loop: Header=BB2_90 Depth=3
	movq	%rax, %r14
.LBB2_99:                               #   in Loop: Header=BB2_90 Depth=3
	movq	%rbx, %rdi
	testq	%rax, %rax
	jne	.LBB2_100
.LBB2_90:                               #   Parent Loop BB2_32 Depth=1
                                        #     Parent Loop BB2_34 Depth=2
                                        # =>    This Loop Header: Depth=3
                                        #         Child Loop BB2_93 Depth 4
	cmpb	$0, (%rdi)
	je	.LBB2_91
# %bb.92:                               #   in Loop: Header=BB2_90 Depth=3
	xorl	%esi, %esi
	movq	%rdi, %rbx
	.p2align	4
.LBB2_93:                               #   Parent Loop BB2_32 Depth=1
                                        #     Parent Loop BB2_34 Depth=2
                                        #       Parent Loop BB2_90 Depth=3
                                        # =>      This Inner Loop Header: Depth=4
	movzbl	(%rbx), %eax
	testl	%eax, %eax
	je	.LBB2_96
# %bb.94:                               #   in Loop: Header=BB2_93 Depth=4
	cmpl	$58, %eax
	je	.LBB2_96
# %bb.95:                               #   in Loop: Header=BB2_93 Depth=4
	incq	%rbx
	incq	%rsi
	jmp	.LBB2_93
	.p2align	4
.LBB2_98:                               #   in Loop: Header=BB2_90 Depth=3
	xorl	%ecx, %ecx
	cmpb	$58, (%rbx)
	sete	%cl
	addq	%rcx, %rbx
	jmp	.LBB2_99
.LBB2_91:                               #   in Loop: Header=BB2_34 Depth=2
	xorl	%r14d, %r14d
.LBB2_100:                              #   in Loop: Header=BB2_34 Depth=2
	testq	%r14, %r14
	leaq	.L.str.4(%rip), %rbx
	leaq	.L.str.5(%rip), %r14
	jne	.LBB2_106
.LBB2_101:                              #   in Loop: Header=BB2_34 Depth=2
	cmpb	$0, g_prog_dir(%rip)
	je	.LBB2_105
# %bb.102:                              #   in Loop: Header=BB2_34 Depth=2
	movq	$-1, %rsi
	leaq	g_prog_dir(%rip), %rdi
	.p2align	4
.LBB2_103:                              #   Parent Loop BB2_32 Depth=1
                                        #     Parent Loop BB2_34 Depth=2
                                        # =>    This Inner Loop Header: Depth=3
	cmpb	$0, 1(%rsi,%rdi)
	leaq	1(%rsi), %rsi
	jne	.LBB2_103
# %bb.104:                              #   in Loop: Header=BB2_34 Depth=2
	movq	%r13, %rdx
	callq	try_dir
	testq	%rax, %rax
	jne	.LBB2_106
.LBB2_105:                              #   in Loop: Header=BB2_34 Depth=2
	movl	$1, %esi
	leaq	.L.str.28(%rip), %rdi
	movq	%r13, %rdx
	callq	try_dir
	testq	%rax, %rax
	jne	.LBB2_106
# %bb.140:
	leaq	.L.str.29(%rip), %rdi
	callq	dstr
	movq	%r13, %rdi
	callq	dstr
	leaq	.L.str.5(%rip), %rdi
	callq	dstr
	callq	sys_exit
.LBB2_45:
	cmpl	$0, g_nobjs(%rip)
	jle	.LBB2_108
# %bb.46:
	xorl	%r12d, %r12d
	leaq	g_objs(%rip), %r14
	leaq	.L.str.5(%rip), %r15
	movabsq	$-6148914691236517205, %r13     # imm = 0xAAAAAAAAAAAAAAAB
	jmp	.LBB2_47
	.p2align	4
.LBB2_123:                              #   in Loop: Header=BB2_47 Depth=1
	incq	%r12
	movslq	g_nobjs(%rip), %rax
	cmpq	%rax, %r12
	jge	.LBB2_108
.LBB2_47:                               # =>This Loop Header: Depth=1
                                        #     Child Loop BB2_53 Depth 2
                                        #       Child Loop BB2_114 Depth 3
	movq	%r12, %rax
	shlq	$8, %rax
	leaq	(%rax,%r12,8), %rbx
	cmpl	$0, 252(%r14,%rbx)
	jne	.LBB2_123
# %bb.48:                               #   in Loop: Header=BB2_47 Depth=1
	addq	%r14, %rbx
	movl	$1, 252(%rbx)
	cmpb	$1, g_debug(%rip)
	jne	.LBB2_50
# %bb.49:                               #   in Loop: Header=BB2_47 Depth=1
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
	movq	%r15, %rsi
	xorl	%r10d, %r10d
	xorl	%r8d, %r8d
	xorl	%r9d, %r9d
	#APP
	syscall
	#NO_APP
.LBB2_50:                               #   in Loop: Header=BB2_47 Depth=1
	movq	104(%rbx), %rax
	testq	%rax, %rax
	je	.LBB2_119
# %bb.51:                               #   in Loop: Header=BB2_47 Depth=1
	movq	112(%rbx), %rcx
	cmpq	$8, %rcx
	jae	.LBB2_52
.LBB2_119:                              #   in Loop: Header=BB2_47 Depth=1
	movq	72(%rbx), %rsi
	testq	%rsi, %rsi
	je	.LBB2_121
# %bb.120:                              #   in Loop: Header=BB2_47 Depth=1
	movq	%r13, %rax
	mulq	80(%rbx)
	shrq	$4, %rdx
	movq	%rbx, %rdi
	callq	apply_rela
.LBB2_121:                              #   in Loop: Header=BB2_47 Depth=1
	movq	88(%rbx), %rsi
	testq	%rsi, %rsi
	je	.LBB2_123
# %bb.122:                              #   in Loop: Header=BB2_47 Depth=1
	movq	%r13, %rax
	mulq	96(%rbx)
	shrq	$4, %rdx
	movq	%rbx, %rdi
	callq	apply_rela
	jmp	.LBB2_123
.LBB2_52:                               #   in Loop: Header=BB2_47 Depth=1
	shrq	$3, %rcx
	xorl	%esi, %esi
	xorl	%edx, %edx
	jmp	.LBB2_53
	.p2align	4
.LBB2_117:                              #   in Loop: Header=BB2_53 Depth=2
	movq	16(%rbx), %rsi
	addq	%rsi, (%rsi,%rdi)
	addq	%rdi, %rsi
	addq	$8, %rsi
.LBB2_118:                              #   in Loop: Header=BB2_53 Depth=2
	incq	%rdx
	cmpq	%rcx, %rdx
	je	.LBB2_119
.LBB2_53:                               #   Parent Loop BB2_47 Depth=1
                                        # =>  This Loop Header: Depth=2
                                        #       Child Loop BB2_114 Depth 3
	movq	(%rax,%rdx,8), %rdi
	testb	$1, %dil
	je	.LBB2_117
# %bb.54:                               #   in Loop: Header=BB2_53 Depth=2
	movq	%rsi, %r8
	cmpq	$2, %rdi
	jae	.LBB2_114
.LBB2_55:                               #   in Loop: Header=BB2_53 Depth=2
	addq	$504, %rsi                      # imm = 0x1F8
	jmp	.LBB2_118
	.p2align	4
.LBB2_116:                              #   in Loop: Header=BB2_114 Depth=3
	movq	%rdi, %r9
	shrq	%r9
	addq	$8, %r8
	cmpq	$4, %rdi
	movq	%r9, %rdi
	jb	.LBB2_55
.LBB2_114:                              #   Parent Loop BB2_47 Depth=1
                                        #     Parent Loop BB2_53 Depth=2
                                        # =>    This Inner Loop Header: Depth=3
	testb	$2, %dil
	je	.LBB2_116
# %bb.115:                              #   in Loop: Header=BB2_114 Depth=3
	movq	16(%rbx), %r9
	addq	%r9, (%r8)
	jmp	.LBB2_116
.LBB2_108:
	movl	g_nobjs(%rip), %eax
	testl	%eax, %eax
	jle	.LBB2_124
# %bb.109:
	leaq	g_objs+168(%rip), %r12
	xorl	%r13d, %r13d
	leaq	.L.str.7(%rip), %rbx
	leaq	.L.str.36(%rip), %r14
	leaq	.L.str.5(%rip), %r15
	jmp	.LBB2_110
	.p2align	4
.LBB2_138:                              #   in Loop: Header=BB2_110 Depth=1
	incq	%r13
	movslq	g_nobjs(%rip), %rax
	addq	$264, %r12                      # imm = 0x108
	cmpq	%rax, %r13
	jge	.LBB2_124
.LBB2_110:                              # =>This Inner Loop Header: Depth=1
	movq	(%r12), %rax
	testq	%rax, %rax
	je	.LBB2_138
# %bb.111:                              #   in Loop: Header=BB2_110 Depth=1
	movq	-8(%r12), %rdi
	addq	-152(%r12), %rdi
	leaq	(%rax,%rdi), %rsi
	addq	$4095, %rsi                     # imm = 0xFFF
	andq	$-4096, %rdi                    # imm = 0xF000
	andq	$-4096, %rsi                    # imm = 0xF000
	subq	%rdi, %rsi
	jbe	.LBB2_138
# %bb.112:                              #   in Loop: Header=BB2_110 Depth=1
	movl	$10, %eax
	movl	$1, %edx
	xorl	%r10d, %r10d
	xorl	%r8d, %r8d
	xorl	%r9d, %r9d
	#APP
	syscall
	#NO_APP
	cmpq	$-4095, %rax                    # imm = 0xF001
	jae	.LBB2_113
# %bb.136:                              #   in Loop: Header=BB2_110 Depth=1
	cmpb	$1, g_debug(%rip)
	jne	.LBB2_138
# %bb.137:                              #   in Loop: Header=BB2_110 Depth=1
	movl	$1, %eax
	movl	$2, %edi
	movl	$5, %edx
	movq	%rbx, %rsi
	xorl	%r10d, %r10d
	xorl	%r8d, %r8d
	xorl	%r9d, %r9d
	#APP
	syscall
	#NO_APP
	movl	$1, %eax
	movl	$33, %edx
	movq	%r14, %rsi
	xorl	%r10d, %r10d
	xorl	%r8d, %r8d
	xorl	%r9d, %r9d
	#APP
	syscall
	#NO_APP
	movl	$1, %eax
	movl	$1, %edx
	movq	%r15, %rsi
	xorl	%r10d, %r10d
	xorl	%r8d, %r8d
	xorl	%r9d, %r9d
	#APP
	syscall
	#NO_APP
	jmp	.LBB2_138
.LBB2_124:
	testl	%eax, %eax
	jle	.LBB2_135
# %bb.125:
	movq	-64(%rbp), %rcx                 # 8-byte Reload
	leal	-1(%rcx), %ebx
	movl	%eax, %r13d
	jmp	.LBB2_126
	.p2align	4
.LBB2_134:                              #   in Loop: Header=BB2_126 Depth=1
	cmpq	$1, %r13
	movq	-56(%rbp), %r13                 # 8-byte Reload
	jle	.LBB2_135
.LBB2_126:                              # =>This Loop Header: Depth=1
                                        #     Child Loop BB2_131 Depth 2
	leaq	-1(%r13), %rax
	movq	%rax, -56(%rbp)                 # 8-byte Spill
	shlq	$8, %rax
	leaq	(%rax,%r13,8), %r14
	addq	$-8, %r14
	leaq	g_objs(%rip), %rax
	cmpl	$0, 256(%rax,%r14)
	jne	.LBB2_134
# %bb.127:                              #   in Loop: Header=BB2_126 Depth=1
	leaq	g_objs(%rip), %rax
	addq	%rax, %r14
	movl	$1, 256(%r14)
	movq	136(%r14), %rsi
	testq	%rsi, %rsi
	je	.LBB2_129
# %bb.128:                              #   in Loop: Header=BB2_126 Depth=1
	leaq	.L.str.37(%rip), %rdi
	callq	trace2
	movl	%ebx, %edi
	movq	-72(%rbp), %rsi                 # 8-byte Reload
	movq	-48(%rbp), %rdx                 # 8-byte Reload
	callq	*136(%r14)
.LBB2_129:                              #   in Loop: Header=BB2_126 Depth=1
	movq	152(%r14), %r12
	cmpq	$8, %r12
	jb	.LBB2_134
# %bb.130:                              #   in Loop: Header=BB2_126 Depth=1
	shrq	$3, %r12
	xorl	%r15d, %r15d
	jmp	.LBB2_131
	.p2align	4
.LBB2_133:                              #   in Loop: Header=BB2_131 Depth=2
	incq	%r15
	cmpq	%r15, %r12
	je	.LBB2_134
.LBB2_131:                              #   Parent Loop BB2_126 Depth=1
                                        # =>  This Inner Loop Header: Depth=2
	movq	144(%r14), %rax
	movq	(%rax,%r15,8), %rax
	leaq	-1(%rax), %rcx
	cmpq	$-3, %rcx
	ja	.LBB2_133
# %bb.132:                              #   in Loop: Header=BB2_131 Depth=2
	movl	%ebx, %edi
	movq	-72(%rbp), %rsi                 # 8-byte Reload
	movq	-48(%rbp), %rdx                 # 8-byte Reload
	callq	*%rax
	jmp	.LBB2_133
.LBB2_135:
	movq	-80(%rbp), %rdi                 # 8-byte Reload
	movq	-64(%rbp), %rsi                 # 8-byte Reload
                                        # kill: def $esi killed $esi killed $rsi
	movq	-96(%rbp), %rdx                 # 8-byte Reload
	movq	-48(%rbp), %rcx                 # 8-byte Reload
	movq	-88(%rbp), %r8                  # 8-byte Reload
	callq	handoff
.LBB2_113:
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
	pushq	%rbp
	movq	%rsp, %rbp
	pushq	%rbx
	pushq	%rax
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
	pushq	%rbp
	movq	%rsp, %rbp
	pushq	%r14
	pushq	%rbx
	movq	%rdi, %rbx
	cmpb	$1, g_debug(%rip)
	jne	.LBB4_4
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
	popq	%rbx
	popq	%r14
	popq	%rbp
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
	pushq	%rbp
	movq	%rsp, %rbp
	movl	$1, %eax
	movl	$2, %edi
	xorl	%r10d, %r10d
	xorl	%r8d, %r8d
	xorl	%r9d, %r9d
	#APP
	syscall
	#NO_APP
	popq	%rbp
	retq
.Lfunc_end5:
	.size	dstr, .Lfunc_end5-dstr
                                        # -- End function
	.p2align	4                               # -- Begin function handoff
	.type	handoff,@function
handoff:                                # @handoff
# %bb.0:
	pushq	%rbp
	movq	%rsp, %rbp
	pushq	%r15
	pushq	%r14
	pushq	%r13
	pushq	%r12
	pushq	%rbx
	subq	$40, %rsp
	movq	%rcx, %r13
	movq	%rdx, -56(%rbp)                 # 8-byte Spill
                                        # kill: def $esi killed $esi def $rsi
	movq	%rdi, -64(%rbp)                 # 8-byte Spill
	movabsq	$-4294967296, %rax              # imm = 0xFFFFFFFF00000000
	movq	%rsi, -72(%rbp)                 # 8-byte Spill
	leal	-1(%rsi), %ecx
	movl	%ecx, -44(%rbp)                 # 4-byte Spill
	movq	$-1, %r15
	movq	%rax, %r12
	.p2align	4
.LBB6_1:                                # =>This Inner Loop Header: Depth=1
	addq	%rax, %r12
	cmpq	$0, 8(%r13,%r15,8)
	leaq	1(%r15), %r15
	jne	.LBB6_1
# %bb.2:
	movq	$-1, %rbx
	xorl	%r14d, %r14d
	.p2align	4
.LBB6_3:                                # =>This Inner Loop Header: Depth=1
	incq	%rbx
	cmpq	$0, (%r8,%r14,8)
	leaq	2(%r14), %r14
	jne	.LBB6_3
# %bb.4:
	movq	%r8, -80(%rbp)                  # 8-byte Spill
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
# %bb.34:
	negq	%rax
	leaq	.L.str.38(%rip), %rdi
	movq	%rax, %rsi
	callq	die2
.LBB6_5:
	movq	-72(%rbp), %rdx                 # 8-byte Reload
	movslq	%edx, %rcx
	sarq	$32, %r12
	andl	$-2, %r14d
	addq	%rcx, %r14
	subq	%r14, %r12
	leaq	(%rax,%r12,8), %r14
	addq	$262144, %r14                   # imm = 0x40000
	andq	$-16, %r14
	movslq	-44(%rbp), %rax                 # 4-byte Folded Reload
	movq	%rax, (%r14)
	leaq	8(%r14), %rcx
	cmpl	$2, %edx
	jl	.LBB6_9
# %bb.6:
	movl	-44(%rbp), %eax                 # 4-byte Reload
	shlq	$3, %rax
	xorl	%edx, %edx
	movq	-56(%rbp), %rdi                 # 8-byte Reload
	.p2align	4
.LBB6_7:                                # =>This Inner Loop Header: Depth=1
	movq	8(%rdi,%rdx), %rsi
	movq	%rsi, (%rcx,%rdx)
	addq	$8, %rdx
	cmpq	%rdx, %rax
	jne	.LBB6_7
# %bb.8:
	addq	%rdx, %rcx
.LBB6_9:
	movq	$0, (%rcx)
	leaq	8(%rcx), %rax
	testq	%r15, %r15
	je	.LBB6_13
# %bb.10:
	cmpl	$2, %r15d
	movl	$1, %ecx
	cmovgel	%r15d, %ecx
	xorl	%edx, %edx
	.p2align	4
.LBB6_11:                               # =>This Inner Loop Header: Depth=1
	movq	(%r13,%rdx,8), %rsi
	movq	%rsi, (%rax)
	incq	%rdx
	addq	$8, %rax
	cmpq	%rdx, %rcx
	jne	.LBB6_11
# %bb.12:
	leaq	-8(%rax), %rcx
.LBB6_13:
	movq	$0, (%rax)
	leaq	16(%rcx), %rax
	testq	%rbx, %rbx
	je	.LBB6_21
# %bb.14:
	movq	-80(%rbp), %rdi                 # 8-byte Reload
	addq	$8, %rdi
	movq	-64(%rbp), %rsi                 # 8-byte Reload
	movq	-56(%rbp), %r8                  # 8-byte Reload
	jmp	.LBB6_15
.LBB6_29:                               #   in Loop: Header=BB6_15 Depth=1
	movq	24(%rsi), %rdx
	.p2align	4
.LBB6_33:                               #   in Loop: Header=BB6_15 Depth=1
	movq	%rcx, (%rax)
	movq	%rdx, 8(%rax)
	addq	$16, %rax
	addq	$16, %rdi
	decq	%rbx
	je	.LBB6_20
.LBB6_15:                               # =>This Inner Loop Header: Depth=1
	movq	-8(%rdi), %rcx
	movq	(%rdi), %rdx
	cmpq	$5, %rcx
	jle	.LBB6_16
# %bb.22:                               #   in Loop: Header=BB6_15 Depth=1
	cmpq	$8, %rcx
	jg	.LBB6_26
# %bb.23:                               #   in Loop: Header=BB6_15 Depth=1
	cmpq	$6, %rcx
	je	.LBB6_32
# %bb.24:                               #   in Loop: Header=BB6_15 Depth=1
	cmpq	$7, %rcx
	jne	.LBB6_33
# %bb.25:                               #   in Loop: Header=BB6_15 Depth=1
	xorl	%edx, %edx
	jmp	.LBB6_33
	.p2align	4
.LBB6_16:                               #   in Loop: Header=BB6_15 Depth=1
	cmpq	$3, %rcx
	je	.LBB6_29
# %bb.17:                               #   in Loop: Header=BB6_15 Depth=1
	cmpq	$4, %rcx
	je	.LBB6_30
# %bb.18:                               #   in Loop: Header=BB6_15 Depth=1
	cmpq	$5, %rcx
	jne	.LBB6_33
# %bb.19:                               #   in Loop: Header=BB6_15 Depth=1
	movq	32(%rsi), %rdx
	jmp	.LBB6_33
.LBB6_26:                               #   in Loop: Header=BB6_15 Depth=1
	cmpq	$9, %rcx
	je	.LBB6_31
# %bb.27:                               #   in Loop: Header=BB6_15 Depth=1
	cmpq	$31, %rcx
	jne	.LBB6_33
# %bb.28:                               #   in Loop: Header=BB6_15 Depth=1
	movq	8(%r8), %rdx
	jmp	.LBB6_33
.LBB6_32:                               #   in Loop: Header=BB6_15 Depth=1
	movl	$4096, %edx                     # imm = 0x1000
	jmp	.LBB6_33
.LBB6_31:                               #   in Loop: Header=BB6_15 Depth=1
	movq	40(%rsi), %rdx
	jmp	.LBB6_33
.LBB6_30:                               #   in Loop: Header=BB6_15 Depth=1
	movl	$56, %edx
	jmp	.LBB6_33
.LBB6_20:
	leaq	-16(%rax), %rcx
.LBB6_21:
	movq	$0, (%rax)
	movq	$0, 24(%rcx)
	movq	-64(%rbp), %rbx                 # 8-byte Reload
	movq	40(%rbx), %rsi
	leaq	.L.str.39(%rip), %rdi
	callq	trace2
	leaq	.L.str.40(%rip), %rdi
	movq	%r14, %rsi
	callq	trace2
	movq	40(%rbx), %rdi
	movq	%r14, %rsi
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
	pushq	%rbp
	movq	%rsp, %rbp
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
	pushq	%rbp
	movq	%rsp, %rbp
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
	popq	%rbp
	retq
.Lfunc_end8:
	.size	syscall1, .Lfunc_end8-syscall1
                                        # -- End function
	.p2align	4                               # -- Begin function die2
	.type	die2,@function
die2:                                   # @die2
# %bb.0:
	pushq	%rbp
	movq	%rsp, %rbp
	pushq	%r14
	pushq	%rbx
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
	.p2align	4                               # -- Begin function load_from_fd
	.type	load_from_fd,@function
load_from_fd:                           # @load_from_fd
# %bb.0:
	pushq	%rbp
	movq	%rsp, %rbp
	pushq	%r15
	pushq	%r14
	pushq	%r13
	pushq	%r12
	pushq	%rbx
	subq	$88, %rsp
	movslq	g_nobjs(%rip), %rax
	cmpq	$32, %rax
	jge	.LBB10_122
# %bb.1:
	movq	%rsi, %r15
	movl	%edi, %r14d
	leal	1(%rax), %ecx
	movl	%ecx, g_nobjs(%rip)
	movq	%rax, %rcx
	shlq	$8, %rcx
	leaq	(%rcx,%rax,8), %rbx
	leaq	g_objs(%rip), %r12
	leaq	(%r12,%rbx), %rdi
	movl	$264, %edx                      # imm = 0x108
	movq	%rdi, -48(%rbp)                 # 8-byte Spill
	xorl	%esi, %esi
	callq	memset@PLT
	movq	%r15, (%r12,%rbx)
	movslq	%r14d, %rdi
	leaq	-120(%rbp), %rsi
	movl	$17, %eax
	movl	$64, %edx
	movq	%rdi, -56(%rbp)                 # 8-byte Spill
	xorl	%r10d, %r10d
	xorl	%r8d, %r8d
	xorl	%r9d, %r9d
	#APP
	syscall
	#NO_APP
	cmpq	$64, %rax
	jne	.LBB10_123
# %bb.2:
	cmpb	$127, -120(%rbp)
	jne	.LBB10_124
# %bb.3:
	cmpb	$69, -119(%rbp)
	jne	.LBB10_124
# %bb.4:
	cmpb	$76, -118(%rbp)
	jne	.LBB10_124
# %bb.5:
	cmpb	$70, -117(%rbp)
	jne	.LBB10_124
# %bb.6:
	cmpb	$2, -116(%rbp)
	jne	.LBB10_125
# %bb.7:
	cmpb	$1, -115(%rbp)
	jne	.LBB10_126
# %bb.8:
	cmpw	$62, -102(%rbp)
	jne	.LBB10_127
# %bb.9:
	movl	-104(%rbp), %eax
	addl	$-4, %eax
	cmpw	$-3, %ax
	jbe	.LBB10_128
# %bb.10:
	cmpw	$56, -66(%rbp)
	jne	.LBB10_129
# %bb.11:
	movzwl	-64(%rbp), %eax
	leal	-257(%rax), %ecx
	movzwl	%cx, %ecx
	cmpl	$65279, %ecx                    # imm = 0xFEFF
	jbe	.LBB10_130
# %bb.12:
	imulq	$56, %rax, %rdx
	movq	-88(%rbp), %r10
	leaq	load_from_fd.ph(%rip), %r15
	movl	$17, %eax
	movq	-56(%rbp), %rdi                 # 8-byte Reload
	movq	%r15, %rsi
	xorl	%r8d, %r8d
	xorl	%r9d, %r9d
	#APP
	syscall
	#NO_APP
	cmpq	%rdx, %rax
	jne	.LBB10_131
# %bb.13:
	movzwl	-64(%rbp), %eax
	testq	%rax, %rax
	je	.LBB10_14
# %bb.17:
	imulq	$56, %rax, %rax
	movq	$-1, %rbx
	xorl	%ecx, %ecx
	xorl	%esi, %esi
	jmp	.LBB10_18
	.p2align	4
.LBB10_20:                              #   in Loop: Header=BB10_18 Depth=1
	addq	$56, %rcx
	cmpq	%rcx, %rax
	je	.LBB10_15
.LBB10_18:                              # =>This Inner Loop Header: Depth=1
	cmpl	$1, (%rcx,%r15)
	jne	.LBB10_20
# %bb.19:                               #   in Loop: Header=BB10_18 Depth=1
	movq	16(%rcx,%r15), %rdx
	movq	40(%rcx,%r15), %rdi
	addq	%rdx, %rdi
	addq	$4095, %rdi                     # imm = 0xFFF
	andq	$-4096, %rdx                    # imm = 0xF000
	andq	$-4096, %rdi                    # imm = 0xF000
	cmpq	%rbx, %rdx
	cmovbq	%rdx, %rbx
	cmpq	%rsi, %rdi
	cmovaq	%rdi, %rsi
	jmp	.LBB10_20
.LBB10_14:
	xorl	%esi, %esi
	movq	$-1, %rbx
.LBB10_15:
	cmpq	$-1, %rbx
	je	.LBB10_16
# %bb.21:
	subq	%rbx, %rsi
	xorl	%edi, %edi
	xorl	%r10d, %r10d
	cmpw	$2, -104(%rbp)
	sete	%r10b
	cmoveq	%rbx, %rdi
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
	jae	.LBB10_132
# %bb.22:
	subq	%rbx, %rax
	xorl	%r12d, %r12d
	cmpw	$2, -104(%rbp)
	cmovneq	%rax, %r12
	leaq	.L.str.23(%rip), %rdi
	movq	%r12, %rsi
	callq	trace2
	movzwl	-64(%rbp), %eax
	testq	%rax, %rax
	je	.LBB10_28
# %bb.23:
	leaq	4095(%r12), %rax
	movq	%rax, -128(%rbp)                # 8-byte Spill
	xorl	%ebx, %ebx
	jmp	.LBB10_24
	.p2align	4
.LBB10_37:                              #   in Loop: Header=BB10_24 Depth=1
	incq	%rbx
	movzwl	-64(%rbp), %eax
	addq	$56, %r15
	cmpq	%rax, %rbx
	jae	.LBB10_28
.LBB10_24:                              # =>This Inner Loop Header: Depth=1
	cmpl	$1, (%r15)
	jne	.LBB10_37
# %bb.25:                               #   in Loop: Header=BB10_24 Depth=1
	movzbl	4(%r15), %r14d
	rolb	$4, %r14b
	movl	%r14d, %eax
	shrb	$2, %al
	movl	%r14d, %ecx
	andb	$16, %cl
	shlb	$2, %cl
	orb	%al, %cl
	andb	$80, %cl
	addb	%cl, %cl
	addb	%r14b, %r14b
	andb	$64, %r14b
	orb	%cl, %r14b
	shrb	$5, %r14b
	movq	16(%r15), %rdi
	addq	%r12, %rdi
	movq	32(%r15), %rax
	leaq	(%rax,%rdi), %r13
	addq	$4095, %r13                     # imm = 0xFFF
	andq	$-4096, %rdi                    # imm = 0xF000
	andq	$-4096, %r13                    # imm = 0xF000
	movq	%r13, %rsi
	subq	%rdi, %rsi
	jbe	.LBB10_34
# %bb.26:                               #   in Loop: Header=BB10_24 Depth=1
	movq	8(%r15), %r9
	movq	$-4096, %rax                    # imm = 0xF000
	andq	%rax, %r9
	movzbl	%r14b, %edx
	movl	$9, %eax
	movl	$18, %r10d
	movq	-56(%rbp), %r8                  # 8-byte Reload
	#APP
	syscall
	#NO_APP
	cmpq	$-4095, %rax                    # imm = 0xF001
	jae	.LBB10_27
.LBB10_34:                              #   in Loop: Header=BB10_24 Depth=1
	movq	16(%r15), %rsi
	addq	-128(%rbp), %rsi                # 8-byte Folded Reload
	addq	40(%r15), %rsi
	andq	$-4096, %rsi                    # imm = 0xF000
	subq	%r13, %rsi
	jbe	.LBB10_37
# %bb.35:                               #   in Loop: Header=BB10_24 Depth=1
	testb	$2, 4(%r15)
	je	.LBB10_37
# %bb.36:                               #   in Loop: Header=BB10_24 Depth=1
	movzbl	%r14b, %edx
	movl	$9, %eax
	movl	$50, %r10d
	movq	%r13, %rdi
	movq	$-1, %r8
	xorl	%r9d, %r9d
	#APP
	syscall
	#NO_APP
	cmpq	$-4095, %rax                    # imm = 0xF001
	jb	.LBB10_37
# %bb.133:
	negq	%rax
	leaq	.L.str.25(%rip), %rdi
	movq	%rax, %rsi
	callq	die2
.LBB10_28:
	movq	-48(%rbp), %rcx                 # 8-byte Reload
	movq	$0, 24(%rcx)
	testq	%rax, %rax
	je	.LBB10_39
# %bb.29:
	movq	-88(%rbp), %rcx
	imulq	$56, %rax, %rdi
	xorl	%edx, %edx
	leaq	load_from_fd.ph(%rip), %rsi
	jmp	.LBB10_30
	.p2align	4
.LBB10_38:                              #   in Loop: Header=BB10_30 Depth=1
	addq	$56, %rdx
	cmpq	%rdx, %rdi
	je	.LBB10_39
.LBB10_30:                              # =>This Inner Loop Header: Depth=1
	cmpl	$1, (%rdx,%rsi)
	jne	.LBB10_38
# %bb.31:                               #   in Loop: Header=BB10_30 Depth=1
	movq	8(%rdx,%rsi), %r8
	cmpq	%rcx, %r8
	ja	.LBB10_38
# %bb.32:                               #   in Loop: Header=BB10_30 Depth=1
	movq	32(%rdx,%rsi), %r9
	addq	%r8, %r9
	cmpq	%r9, %rcx
	jae	.LBB10_38
# %bb.33:
	movq	%r12, %rdi
	subq	%r8, %rdi
	addq	%rcx, %rdi
	addq	16(%rdx,%rsi), %rdi
	movq	-48(%rbp), %rcx                 # 8-byte Reload
	movq	%rdi, 24(%rcx)
.LBB10_39:
	movq	-48(%rbp), %rsi                 # 8-byte Reload
	movq	%rax, 32(%rsi)
	movq	%r12, 16(%rsi)
	addq	-96(%rbp), %r12
	movq	%r12, 40(%rsi)
	cmpw	$0, -64(%rbp)
	je	.LBB10_40
# %bb.44:
	leaq	load_from_fd.ph+40(%rip), %rbx
	xorl	%r14d, %r14d
	leaq	.L.str.20(%rip), %r15
	leaq	.L.str.5(%rip), %r12
	xorl	%r13d, %r13d
	jmp	.LBB10_45
	.p2align	4
.LBB10_49:                              #   in Loop: Header=BB10_45 Depth=1
	movq	-24(%rbx), %rax
	movq	%rax, 160(%rsi)
	movq	(%rbx), %rax
	movq	%rax, 168(%rsi)
.LBB10_53:                              #   in Loop: Header=BB10_45 Depth=1
	incq	%r14
	movzwl	-64(%rbp), %eax
	addq	$56, %rbx
	cmpq	%rax, %r14
	jae	.LBB10_41
.LBB10_45:                              # =>This Inner Loop Header: Depth=1
	movl	-40(%rbx), %eax
	cmpl	$1685382482, %eax               # imm = 0x6474E552
	je	.LBB10_49
# %bb.46:                               #   in Loop: Header=BB10_45 Depth=1
	cmpl	$7, %eax
	je	.LBB10_50
# %bb.47:                               #   in Loop: Header=BB10_45 Depth=1
	cmpl	$2, %eax
	jne	.LBB10_53
# %bb.48:                               #   in Loop: Header=BB10_45 Depth=1
	movq	-24(%rbx), %r13
	addq	16(%rsi), %r13
	jmp	.LBB10_53
	.p2align	4
.LBB10_50:                              #   in Loop: Header=BB10_45 Depth=1
	cmpq	$0, (%rbx)
	je	.LBB10_53
# %bb.51:                               #   in Loop: Header=BB10_45 Depth=1
	cmpb	$1, g_debug(%rip)
	jne	.LBB10_53
# %bb.52:                               #   in Loop: Header=BB10_45 Depth=1
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
	movq	-48(%rbp), %rsi                 # 8-byte Reload
	jmp	.LBB10_53
.LBB10_40:
	xorl	%r13d, %r13d
.LBB10_41:
	testq	%r13, %r13
	je	.LBB10_121
# %bb.42:
	movq	(%r13), %rdx
	testq	%rdx, %rdx
	je	.LBB10_43
# %bb.58:
	addq	$16, %r13
	movl	$7, %eax
	jmp	.LBB10_59
.LBB10_85:                              #   in Loop: Header=BB10_59 Depth=1
	cmpq	$27, %rdx
	je	.LBB10_98
# %bb.86:                               #   in Loop: Header=BB10_59 Depth=1
	cmpq	$29, %rdx
	jne	.LBB10_103
.LBB10_87:                              #   in Loop: Header=BB10_59 Depth=1
	movq	%rcx, 176(%rsi)
	.p2align	4
.LBB10_103:                             #   in Loop: Header=BB10_59 Depth=1
	movq	(%r13), %rdx
	addq	$16, %r13
	testq	%rdx, %rdx
	je	.LBB10_54
.LBB10_59:                              # =>This Inner Loop Header: Depth=1
	movq	-8(%r13), %rcx
	cmpq	$14, %rdx
	jg	.LBB10_76
# %bb.60:                               #   in Loop: Header=BB10_59 Depth=1
	cmpq	$5, %rdx
	jle	.LBB10_61
# %bb.68:                               #   in Loop: Header=BB10_59 Depth=1
	cmpq	$7, %rdx
	jle	.LBB10_69
# %bb.72:                               #   in Loop: Header=BB10_59 Depth=1
	cmpq	$8, %rdx
	je	.LBB10_92
# %bb.73:                               #   in Loop: Header=BB10_59 Depth=1
	cmpq	$12, %rdx
	je	.LBB10_97
# %bb.74:                               #   in Loop: Header=BB10_59 Depth=1
	cmpq	$14, %rdx
	jne	.LBB10_103
# %bb.75:                               #   in Loop: Header=BB10_59 Depth=1
	movq	%rcx, 8(%rsi)
	jmp	.LBB10_103
	.p2align	4
.LBB10_76:                              #   in Loop: Header=BB10_59 Depth=1
	cmpq	$26, %rdx
	jle	.LBB10_77
# %bb.84:                               #   in Loop: Header=BB10_59 Depth=1
	cmpq	$1879047924, %rdx               # imm = 0x6FFFFEF4
	jle	.LBB10_85
# %bb.88:                               #   in Loop: Header=BB10_59 Depth=1
	cmpq	$1879047925, %rdx               # imm = 0x6FFFFEF5
	je	.LBB10_96
# %bb.89:                               #   in Loop: Header=BB10_59 Depth=1
	cmpq	$1879048174, %rdx               # imm = 0x6FFFFFEE
	je	.LBB10_94
# %bb.90:                               #   in Loop: Header=BB10_59 Depth=1
	cmpq	$1879048175, %rdx               # imm = 0x6FFFFFEF
	jne	.LBB10_103
# %bb.91:                               #   in Loop: Header=BB10_59 Depth=1
	addq	16(%rsi), %rcx
	movq	%rcx, 104(%rsi)
	jmp	.LBB10_103
	.p2align	4
.LBB10_61:                              #   in Loop: Header=BB10_59 Depth=1
	cmpq	$3, %rdx
	jg	.LBB10_65
# %bb.62:                               #   in Loop: Header=BB10_59 Depth=1
	cmpq	$1, %rdx
	je	.LBB10_99
# %bb.63:                               #   in Loop: Header=BB10_59 Depth=1
	cmpq	$2, %rdx
	jne	.LBB10_103
# %bb.64:                               #   in Loop: Header=BB10_59 Depth=1
	movq	%rcx, 96(%rsi)
	jmp	.LBB10_103
	.p2align	4
.LBB10_77:                              #   in Loop: Header=BB10_59 Depth=1
	cmpq	$22, %rdx
	jg	.LBB10_81
# %bb.78:                               #   in Loop: Header=BB10_59 Depth=1
	cmpq	$15, %rdx
	je	.LBB10_87
# %bb.79:                               #   in Loop: Header=BB10_59 Depth=1
	cmpq	$20, %rdx
	jne	.LBB10_103
# %bb.80:                               #   in Loop: Header=BB10_59 Depth=1
	movq	%rcx, %rax
	jmp	.LBB10_103
.LBB10_65:                              #   in Loop: Header=BB10_59 Depth=1
	cmpq	$4, %rdx
	je	.LBB10_95
# %bb.66:                               #   in Loop: Header=BB10_59 Depth=1
	cmpq	$5, %rdx
	jne	.LBB10_103
# %bb.67:                               #   in Loop: Header=BB10_59 Depth=1
	addq	16(%rsi), %rcx
	movq	%rcx, 56(%rsi)
	jmp	.LBB10_103
.LBB10_81:                              #   in Loop: Header=BB10_59 Depth=1
	cmpq	$23, %rdx
	je	.LBB10_93
# %bb.82:                               #   in Loop: Header=BB10_59 Depth=1
	cmpq	$25, %rdx
	jne	.LBB10_103
# %bb.83:                               #   in Loop: Header=BB10_59 Depth=1
	addq	16(%rsi), %rcx
	movq	%rcx, 144(%rsi)
	jmp	.LBB10_103
.LBB10_69:                              #   in Loop: Header=BB10_59 Depth=1
	cmpq	$6, %rdx
	je	.LBB10_102
# %bb.70:                               #   in Loop: Header=BB10_59 Depth=1
	cmpq	$7, %rdx
	jne	.LBB10_103
# %bb.71:                               #   in Loop: Header=BB10_59 Depth=1
	addq	16(%rsi), %rcx
	movq	%rcx, 72(%rsi)
	jmp	.LBB10_103
.LBB10_99:                              #   in Loop: Header=BB10_59 Depth=1
	movslq	248(%rsi), %rdx
	cmpq	$15, %rdx
	jg	.LBB10_101
# %bb.100:                              #   in Loop: Header=BB10_59 Depth=1
	movq	%rsi, %rdi
	leal	1(%rdx), %esi
	movl	%esi, 248(%rdi)
	movl	%ecx, 184(%rdi,%rdx,4)
	movq	%rdi, %rsi
	jmp	.LBB10_103
.LBB10_95:                              #   in Loop: Header=BB10_59 Depth=1
	addq	16(%rsi), %rcx
	movq	%rcx, 120(%rsi)
	jmp	.LBB10_103
.LBB10_93:                              #   in Loop: Header=BB10_59 Depth=1
	addq	16(%rsi), %rcx
	movq	%rcx, 88(%rsi)
	jmp	.LBB10_103
.LBB10_97:                              #   in Loop: Header=BB10_59 Depth=1
	addq	16(%rsi), %rcx
	movq	%rcx, 136(%rsi)
	jmp	.LBB10_103
.LBB10_94:                              #   in Loop: Header=BB10_59 Depth=1
	movq	%rcx, 112(%rsi)
	jmp	.LBB10_103
.LBB10_92:                              #   in Loop: Header=BB10_59 Depth=1
	movq	%rcx, 80(%rsi)
	jmp	.LBB10_103
.LBB10_96:                              #   in Loop: Header=BB10_59 Depth=1
	addq	16(%rsi), %rcx
	movq	%rcx, 128(%rsi)
	jmp	.LBB10_103
.LBB10_98:                              #   in Loop: Header=BB10_59 Depth=1
	movq	%rcx, 152(%rsi)
	jmp	.LBB10_103
.LBB10_102:                             #   in Loop: Header=BB10_59 Depth=1
	addq	16(%rsi), %rcx
	movq	%rcx, 48(%rsi)
	jmp	.LBB10_103
.LBB10_54:
	cmpq	$7, %rax
	setne	%al
	cmpq	$0, 88(%rsi)
	jne	.LBB10_56
	jmp	.LBB10_104
.LBB10_43:
	xorl	%eax, %eax
	cmpq	$0, 88(%rsi)
	je	.LBB10_104
.LBB10_56:
	testb	%al, %al
	jne	.LBB10_57
.LBB10_104:
	movq	56(%rsi), %rax
	testq	%rax, %rax
	je	.LBB10_109
# %bb.105:
	movq	8(%rsi), %rcx
	testq	%rcx, %rcx
	je	.LBB10_107
# %bb.106:
	addq	%rax, %rcx
	movq	%rcx, 8(%rsi)
.LBB10_107:
	movq	176(%rsi), %rcx
	testq	%rcx, %rcx
	je	.LBB10_109
# %bb.108:
	addq	%rcx, %rax
	movq	%rax, 176(%rsi)
.LBB10_109:
	movq	120(%rsi), %rax
	testq	%rax, %rax
	je	.LBB10_111
# %bb.110:
	movl	4(%rax), %ecx
.LBB10_120:
	movl	%ecx, %eax
	movq	-48(%rbp), %rsi                 # 8-byte Reload
	movq	%rax, 64(%rsi)
.LBB10_121:
	movq	%rsi, %rax
	addq	$88, %rsp
	popq	%rbx
	popq	%r12
	popq	%r13
	popq	%r14
	popq	%r15
	popq	%rbp
	retq
.LBB10_111:
	movq	128(%rsi), %rdx
	testq	%rdx, %rdx
	je	.LBB10_121
# %bb.112:
	movl	(%rdx), %esi
	movl	4(%rdx), %eax
	movl	8(%rdx), %edi
	testq	%rsi, %rsi
	je	.LBB10_113
# %bb.116:
	leaq	(%rdx,%rdi,8), %r8
	addq	$16, %r8
	xorl	%r9d, %r9d
	xorl	%ecx, %ecx
	.p2align	4
.LBB10_117:                             # =>This Inner Loop Header: Depth=1
	movl	(%r8,%r9,4), %r10d
	cmpl	%ecx, %r10d
	cmoval	%r10d, %ecx
	incq	%r9
	cmpq	%r9, %rsi
	jne	.LBB10_117
# %bb.114:
	cmpl	%eax, %ecx
	jae	.LBB10_118
.LBB10_115:
	movl	%eax, %ecx
	jmp	.LBB10_120
.LBB10_113:
	xorl	%ecx, %ecx
	cmpl	%eax, %ecx
	jb	.LBB10_115
.LBB10_118:
	leaq	(%rdx,%rdi,8), %rdx
	leaq	(%rdx,%rsi,4), %rdx
	addq	$16, %rdx
	negl	%eax
	.p2align	4
.LBB10_119:                             # =>This Inner Loop Header: Depth=1
	leal	(%rax,%rcx), %esi
	incl	%ecx
	testb	$1, (%rdx,%rsi,4)
	je	.LBB10_119
	jmp	.LBB10_120
.LBB10_27:
	negq	%rax
	leaq	.L.str.24(%rip), %rdi
	movq	%rax, %rsi
	callq	die2
.LBB10_122:
	leaq	.L.str.10(%rip), %rdi
	callq	die
.LBB10_123:
	leaq	.L.str.11(%rip), %rdi
	callq	die
.LBB10_124:
	leaq	.L.str.12(%rip), %rdi
	callq	die
.LBB10_125:
	leaq	.L.str.13(%rip), %rdi
	callq	die
.LBB10_126:
	leaq	.L.str.14(%rip), %rdi
	callq	die
.LBB10_127:
	leaq	.L.str.15(%rip), %rdi
	callq	die
.LBB10_128:
	leaq	.L.str.16(%rip), %rdi
	callq	die
.LBB10_129:
	leaq	.L.str.17(%rip), %rdi
	callq	die
.LBB10_130:
	leaq	.L.str.18(%rip), %rdi
	callq	die
.LBB10_131:
	leaq	.L.str.19(%rip), %rdi
	callq	die
.LBB10_16:
	leaq	.L.str.21(%rip), %rdi
	callq	die
.LBB10_132:
	negq	%rax
	leaq	.L.str.22(%rip), %rdi
	movq	%rax, %rsi
	callq	die2
.LBB10_101:
	leaq	.L.str.26(%rip), %rdi
	callq	die
.LBB10_57:
	leaq	.L.str.27(%rip), %rdi
	callq	die
.Lfunc_end10:
	.size	load_from_fd, .Lfunc_end10-load_from_fd
                                        # -- End function
	.p2align	4                               # -- Begin function dhex
	.type	dhex,@function
dhex:                                   # @dhex
# %bb.0:
	pushq	%rbp
	movq	%rsp, %rbp
	movw	$30768, -32(%rbp)               # imm = 0x7830
	movl	$60, %ecx
	leaq	-30(%rbp), %rax
	leaq	dhex.H(%rip), %rdx
	.p2align	4
.LBB11_1:                               # =>This Inner Loop Header: Depth=1
	movq	%rdi, %rsi
	shrq	%cl, %rsi
	andl	$15, %esi
	movzbl	(%rsi,%rdx), %esi
	movb	%sil, (%rax)
	incq	%rax
	addq	$-4, %rcx
	cmpq	$-4, %rcx
	jne	.LBB11_1
# %bb.2:
	movb	$0, -14(%rbp)
	movq	$-1, %rdx
	.p2align	4
.LBB11_3:                               # =>This Inner Loop Header: Depth=1
	cmpb	$0, -31(%rbp,%rdx)
	leaq	1(%rdx), %rdx
	jne	.LBB11_3
# %bb.4:
	leaq	-32(%rbp), %rsi
	movl	$1, %eax
	movl	$2, %edi
	xorl	%r10d, %r10d
	xorl	%r8d, %r8d
	xorl	%r9d, %r9d
	#APP
	syscall
	#NO_APP
	popq	%rbp
	retq
.Lfunc_end11:
	.size	dhex, .Lfunc_end11-dhex
                                        # -- End function
	.p2align	4                               # -- Begin function trace2
	.type	trace2,@function
trace2:                                 # @trace2
# %bb.0:
	cmpb	$1, g_debug(%rip)
	jne	.LBB12_8
# %bb.1:
	pushq	%rbp
	movq	%rsp, %rbp
	pushq	%r14
	pushq	%rbx
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
	movw	$30768, -48(%rbp)               # imm = 0x7830
	movl	$60, %ecx
	leaq	-46(%rbp), %rax
	leaq	dhex.H(%rip), %rdx
	.p2align	4
.LBB12_4:                               # =>This Inner Loop Header: Depth=1
	movq	%rbx, %rsi
	shrq	%cl, %rsi
	andl	$15, %esi
	movzbl	(%rsi,%rdx), %esi
	movb	%sil, (%rax)
	addq	$-4, %rcx
	incq	%rax
	cmpq	$-4, %rcx
	jne	.LBB12_4
# %bb.5:
	movb	$0, -30(%rbp)
	movq	$-1, %rdx
	.p2align	4
.LBB12_6:                               # =>This Inner Loop Header: Depth=1
	cmpb	$0, -47(%rbp,%rdx)
	leaq	1(%rdx), %rdx
	jne	.LBB12_6
# %bb.7:
	leaq	-48(%rbp), %rsi
	movl	$1, %eax
	movl	$2, %edi
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
	popq	%rbx
	popq	%r14
	popq	%rbp
.LBB12_8:
	retq
.Lfunc_end12:
	.size	trace2, .Lfunc_end12-trace2
                                        # -- End function
	.p2align	4                               # -- Begin function try_dir
	.type	try_dir,@function
try_dir:                                # @try_dir
# %bb.0:
	pushq	%rbp
	movq	%rsp, %rbp
	pushq	%r15
	pushq	%r14
	pushq	%rbx
	subq	$4104, %rsp                     # imm = 0x1008
	movq	%rdx, %rbx
	testq	%rsi, %rsi
	je	.LBB13_1
# %bb.5:
	movq	%rdi, %rax
	decq	%rsi
	cmpq	$4094, %rsi                     # imm = 0xFFE
	movl	$4094, %r15d                    # imm = 0xFFE
	cmovbq	%rsi, %r15
	leaq	1(%r15), %r14
	leaq	-4128(%rbp), %rdi
	movq	%rax, %rsi
	movq	%r14, %rdx
	callq	memcpy@PLT
	xorl	%eax, %eax
	.p2align	4
.LBB13_6:                               # =>This Inner Loop Header: Depth=1
	incq	%rax
	cmpq	%rax, %r14
	jne	.LBB13_6
# %bb.2:
	cmpq	$4095, %rax                     # imm = 0xFFF
	setb	%al
	addq	$2, %r15
	testq	%r14, %r14
	je	.LBB13_4
.LBB13_7:
	leaq	-4128(%rbp), %rcx
	cmpb	$47, -1(%r14,%rcx)
	setne	%cl
	andb	%al, %cl
	cmpb	$1, %cl
	jne	.LBB13_8
# %bb.9:
	movb	$47, -4128(%rbp,%r14)
	jmp	.LBB13_10
.LBB13_1:
	movb	$1, %al
	movl	$1, %r15d
	xorl	%r14d, %r14d
	testq	%r14, %r14
	jne	.LBB13_7
.LBB13_4:
	xorl	%r15d, %r15d
	jmp	.LBB13_10
.LBB13_8:
	movq	%r14, %r15
.LBB13_10:
	leaq	(%r15,%rbp), %rax
	addq	$-4128, %rax                    # imm = 0xEFE0
	movzbl	(%rbx), %edx
	xorl	%ecx, %ecx
	testb	%dl, %dl
	je	.LBB13_15
# %bb.11:
	cmpq	$4094, %r15                     # imm = 0xFFE
	ja	.LBB13_15
# %bb.12:
	movl	$4096, %esi                     # imm = 0x1000
	subq	%r15, %rsi
	xorl	%edi, %edi
	.p2align	4
.LBB13_13:                              # =>This Inner Loop Header: Depth=1
	movb	%dl, (%rax,%rdi)
	leaq	2(%rdi), %rdx
	leaq	1(%rdi), %rcx
	cmpq	%rsi, %rdx
	jae	.LBB13_15
# %bb.14:                               #   in Loop: Header=BB13_13 Depth=1
	movzbl	1(%rbx,%rdi), %edx
	movq	%rcx, %rdi
	testb	%dl, %dl
	jne	.LBB13_13
.LBB13_15:
	movb	$0, (%rax,%rcx)
	addq	%rcx, %r15
	movb	$0, -4128(%rbp,%r15)
	xorl	%r14d, %r14d
	leaq	-4128(%rbp), %rsi
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
	js	.LBB13_24
# %bb.16:
	movslq	try_dir.nnames(%rip), %rcx
	cmpq	$31, %rcx
	jg	.LBB13_23
# %bb.17:
	movq	%rcx, %rdx
	shlq	$12, %rdx
	leaq	try_dir.names(%rip), %rbx
	addq	%rdx, %rbx
	movzbl	-4128(%rbp), %esi
	testb	%sil, %sil
	je	.LBB13_18
# %bb.19:
	xorl	%edi, %edi
	.p2align	4
.LBB13_20:                              # =>This Inner Loop Header: Depth=1
	leaq	1(%rdi), %rdx
	movb	%sil, (%rbx,%rdi)
	cmpq	$4094, %rdx                     # imm = 0xFFE
	ja	.LBB13_22
# %bb.21:                               #   in Loop: Header=BB13_20 Depth=1
	movzbl	-4127(%rbp,%rdi), %esi
	movq	%rdx, %rdi
	testb	%sil, %sil
	jne	.LBB13_20
	jmp	.LBB13_22
.LBB13_18:
	xorl	%edx, %edx
.LBB13_22:
	movb	$0, (%rbx,%rdx)
	incl	%ecx
	movl	%ecx, try_dir.nnames(%rip)
.LBB13_23:
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
.LBB13_24:
	movq	%r14, %rax
	addq	$4104, %rsp                     # imm = 0x1008
	popq	%rbx
	popq	%r14
	popq	%r15
	popq	%rbp
	retq
.Lfunc_end13:
	.size	try_dir, .Lfunc_end13-try_dir
                                        # -- End function
	.p2align	4                               # -- Begin function apply_rela
	.type	apply_rela,@function
apply_rela:                             # @apply_rela
# %bb.0:
	pushq	%rbp
	movq	%rsp, %rbp
	pushq	%r15
	pushq	%r14
	pushq	%r13
	pushq	%r12
	pushq	%rbx
	subq	$24, %rsp
	testq	%rdx, %rdx
	je	.LBB14_22
# %bb.1:
	movq	%rdx, %rbx
	movq	%rsi, %r14
	movq	%rdi, %r15
	jmp	.LBB14_2
	.p2align	4
.LBB14_7:                               #   in Loop: Header=BB14_2 Depth=1
	cmpl	$5, %eax
	je	.LBB14_17
# %bb.8:                                #   in Loop: Header=BB14_2 Depth=1
	cmpl	$6, %eax
	jne	.LBB14_9
.LBB14_6:                               #   in Loop: Header=BB14_2 Depth=1
	movq	%r15, %rdi
	movq	%r14, %rsi
	callq	resolve_reloc_symbol
	addq	%r12, %rax
.LBB14_20:                              #   in Loop: Header=BB14_2 Depth=1
	movq	%rax, (%r13)
.LBB14_21:                              #   in Loop: Header=BB14_2 Depth=1
	addq	$24, %r14
	decq	%rbx
	je	.LBB14_22
.LBB14_2:                               # =>This Inner Loop Header: Depth=1
	movq	8(%r14), %rax
	movq	16(%r14), %r12
	movq	16(%r15), %rcx
	movq	(%r14), %r13
	addq	%rcx, %r13
	cmpl	$6, %eax
	jg	.LBB14_10
# %bb.3:                                #   in Loop: Header=BB14_2 Depth=1
	cmpl	$4, %eax
	jg	.LBB14_7
# %bb.4:                                #   in Loop: Header=BB14_2 Depth=1
	testl	%eax, %eax
	je	.LBB14_21
# %bb.5:                                #   in Loop: Header=BB14_2 Depth=1
	cmpl	$1, %eax
	je	.LBB14_6
	jmp	.LBB14_9
	.p2align	4
.LBB14_10:                              #   in Loop: Header=BB14_2 Depth=1
	cmpl	$15, %eax
	jg	.LBB14_14
# %bb.11:                               #   in Loop: Header=BB14_2 Depth=1
	cmpl	$7, %eax
	je	.LBB14_6
# %bb.12:                               #   in Loop: Header=BB14_2 Depth=1
	cmpl	$8, %eax
	jne	.LBB14_9
# %bb.13:                               #   in Loop: Header=BB14_2 Depth=1
	addq	%rcx, %r12
	movq	%r12, (%r13)
	jmp	.LBB14_21
.LBB14_14:                              #   in Loop: Header=BB14_2 Depth=1
	cmpl	$37, %eax
	jne	.LBB14_15
# %bb.19:                               #   in Loop: Header=BB14_2 Depth=1
	addq	%rcx, %r12
	callq	*%r12
	jmp	.LBB14_20
.LBB14_17:                              #   in Loop: Header=BB14_2 Depth=1
	shrq	$32, %rax
	movq	48(%r15), %rcx
	leaq	(%rax,%rax,2), %rax
	movl	(%rcx,%rax,8), %edi
	addq	56(%r15), %rdi
	leaq	-56(%rbp), %rsi
	callq	global_lookup
	testl	%eax, %eax
	je	.LBB14_23
# %bb.18:                               #   in Loop: Header=BB14_2 Depth=1
	movq	-56(%rbp), %rax
	movq	-48(%rbp), %rcx
	movq	8(%rcx), %rsi
	addq	16(%rax), %rsi
	movq	16(%rcx), %rdx
	movq	%r13, %rdi
	callq	memcpy@PLT
	jmp	.LBB14_21
.LBB14_22:
	addq	$24, %rsp
	popq	%rbx
	popq	%r12
	popq	%r13
	popq	%r14
	popq	%r15
	popq	%rbp
	retq
.LBB14_15:
	leal	-16(%rax), %ecx
	cmpl	$3, %ecx
	jae	.LBB14_9
# %bb.16:
	leaq	.L.str.32(%rip), %rdi
	callq	die
.LBB14_23:
	leaq	.L.str.31(%rip), %rdi
	callq	die
.LBB14_9:
	movl	%eax, %esi
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
	movq	%rsp, %rbp
	pushq	%r14
	pushq	%rbx
	subq	$16, %rsp
	movl	12(%rsi), %ecx
	movq	48(%rdi), %rax
	leaq	(%rcx,%rcx,2), %rcx
	movl	(%rax,%rcx,8), %ebx
	addq	56(%rdi), %rbx
	movzbl	4(%rax,%rcx,8), %r14d
	cmpb	$15, %r14b
	ja	.LBB15_3
# %bb.1:
	leaq	(%rax,%rcx,8), %rax
	cmpw	$0, 6(%rax)
	je	.LBB15_3
# %bb.2:
	movq	8(%rax), %rax
	addq	16(%rdi), %rax
	jmp	.LBB15_8
.LBB15_3:
	leaq	-32(%rbp), %rsi
	movq	%rbx, %rdi
	callq	global_lookup
	testl	%eax, %eax
	je	.LBB15_6
# %bb.4:
	movq	-32(%rbp), %rcx
	movq	-24(%rbp), %rdx
	movq	8(%rdx), %rax
	addq	16(%rcx), %rax
	movzbl	4(%rdx), %ecx
	andb	$15, %cl
	cmpb	$10, %cl
	jne	.LBB15_8
# %bb.5:
	callq	*%rax
	jmp	.LBB15_8
.LBB15_6:
	andb	$-16, %r14b
	cmpb	$32, %r14b
	jne	.LBB15_9
# %bb.7:
	xorl	%eax, %eax
.LBB15_8:
	addq	$16, %rsp
	popq	%rbx
	popq	%r14
	popq	%rbp
	retq
.LBB15_9:
	leaq	.L.str.34(%rip), %rdi
	callq	dstr
	movq	%rbx, %rdi
	callq	dstr
	leaq	.L.str.5(%rip), %rdi
	callq	dstr
	callq	sys_exit
.Lfunc_end15:
	.size	resolve_reloc_symbol, .Lfunc_end15-resolve_reloc_symbol
                                        # -- End function
	.p2align	4                               # -- Begin function global_lookup
	.type	global_lookup,@function
global_lookup:                          # @global_lookup
# %bb.0:
	pushq	%rbp
	movq	%rsp, %rbp
	pushq	%r15
	pushq	%r14
	pushq	%r13
	pushq	%r12
	pushq	%rbx
	movq	%rsi, -56(%rbp)                 # 8-byte Spill
	movslq	g_nobjs(%rip), %r9
	testq	%r9, %r9
	jle	.LBB16_1
# %bb.2:
	movq	%rdi, %rax
	xorl	%r8d, %r8d
	leaq	g_objs(%rip), %r11
	xorl	%esi, %esi
	xorl	%edi, %edi
                                        # implicit-def: $rcx
                                        # kill: killed $rcx
                                        # implicit-def: $r10
	movq	%r9, -48(%rbp)                  # 8-byte Spill
	.p2align	4
.LBB16_3:                               # =>This Loop Header: Depth=1
                                        #     Child Loop BB16_7 Depth 2
                                        #       Child Loop BB16_12 Depth 3
	movq	%r8, %rcx
	shlq	$8, %rcx
	leaq	(%rcx,%r8,8), %r14
	movq	48(%r11,%r14), %r15
	movb	$1, %bl
	testq	%r15, %r15
	je	.LBB16_18
# %bb.4:                                #   in Loop: Header=BB16_3 Depth=1
	addq	%r11, %r14
	movq	56(%r14), %r12
	testq	%r12, %r12
	je	.LBB16_18
# %bb.5:                                #   in Loop: Header=BB16_3 Depth=1
	movq	64(%r14), %r13
	testq	%r13, %r13
	je	.LBB16_18
# %bb.6:                                #   in Loop: Header=BB16_3 Depth=1
	movq	%rdi, -64(%rbp)                 # 8-byte Spill
	movq	%rsi, -72(%rbp)                 # 8-byte Spill
	leaq	1(%r12), %rdx
	xorl	%esi, %esi
	jmp	.LBB16_7
.LBB16_10:                              #   in Loop: Header=BB16_7 Depth=2
	movq	%rax, %rdi
.LBB16_14:                              #   in Loop: Header=BB16_7 Depth=2
	cmpb	(%rdi), %cl
	je	.LBB16_15
.LBB16_16:                              #   in Loop: Header=BB16_7 Depth=2
	incq	%rsi
	cmpq	%r13, %rsi
	je	.LBB16_17
.LBB16_7:                               #   Parent Loop BB16_3 Depth=1
                                        # =>  This Loop Header: Depth=2
                                        #       Child Loop BB16_12 Depth 3
	leaq	(%rsi,%rsi,2), %rcx
	cmpw	$0, 6(%r15,%rcx,8)
	je	.LBB16_16
# %bb.8:                                #   in Loop: Header=BB16_7 Depth=2
	leaq	(%r15,%rcx,8), %r11
	movzbl	4(%r11), %ecx
	shrb	$4, %cl
	addb	$-3, %cl
	cmpb	$-2, %cl
	jb	.LBB16_16
# %bb.9:                                #   in Loop: Header=BB16_7 Depth=2
	movl	(%r11), %r9d
	movzbl	(%r12,%r9), %ecx
	testb	%cl, %cl
	je	.LBB16_10
# %bb.11:                               #   in Loop: Header=BB16_7 Depth=2
	addq	%rdx, %r9
	movq	%rax, %rdi
	.p2align	4
.LBB16_12:                              #   Parent Loop BB16_3 Depth=1
                                        #     Parent Loop BB16_7 Depth=2
                                        # =>    This Inner Loop Header: Depth=3
	cmpb	(%rdi), %cl
	jne	.LBB16_14
# %bb.13:                               #   in Loop: Header=BB16_12 Depth=3
	incq	%rdi
	movzbl	(%r9), %ecx
	incq	%r9
	testb	%cl, %cl
	jne	.LBB16_12
	jmp	.LBB16_14
.LBB16_15:                              #   in Loop: Header=BB16_3 Depth=1
	xorl	%ebx, %ebx
	movq	%r11, %r10
	movq	%r14, -80(%rbp)                 # 8-byte Spill
.LBB16_17:                              #   in Loop: Header=BB16_3 Depth=1
	movq	-72(%rbp), %rsi                 # 8-byte Reload
	movq	-64(%rbp), %rdi                 # 8-byte Reload
	movq	-48(%rbp), %r9                  # 8-byte Reload
	leaq	g_objs(%rip), %r11
.LBB16_18:                              #   in Loop: Header=BB16_3 Depth=1
	testb	%bl, %bl
	jne	.LBB16_22
# %bb.19:                               #   in Loop: Header=BB16_3 Depth=1
	movzbl	4(%r10), %ecx
	andb	$-16, %cl
	cmpb	$16, %cl
	jne	.LBB16_21
# %bb.20:                               #   in Loop: Header=BB16_3 Depth=1
	movq	-56(%rbp), %rcx                 # 8-byte Reload
	movq	-80(%rbp), %rdx                 # 8-byte Reload
	movq	%rdx, (%rcx)
	movq	%r10, 8(%rcx)
	movb	$1, %dl
	testb	%dl, %dl
	je	.LBB16_24
	jmp	.LBB16_25
	.p2align	4
.LBB16_21:                              #   in Loop: Header=BB16_3 Depth=1
	testq	%rsi, %rsi
	cmoveq	-80(%rbp), %rdi                 # 8-byte Folded Reload
	cmoveq	%r10, %rsi
.LBB16_22:                              #   in Loop: Header=BB16_3 Depth=1
	xorl	%edx, %edx
	testb	%dl, %dl
	jne	.LBB16_25
.LBB16_24:                              #   in Loop: Header=BB16_3 Depth=1
	incq	%r8
	cmpq	%r9, %r8
	jne	.LBB16_3
	jmp	.LBB16_25
.LBB16_1:
	xorl	%edx, %edx
	xorl	%edi, %edi
	xorl	%esi, %esi
.LBB16_25:
	movl	$1, %eax
	testb	%dl, %dl
	jne	.LBB16_29
# %bb.26:
	testq	%rsi, %rsi
	je	.LBB16_27
# %bb.28:
	movq	-56(%rbp), %rcx                 # 8-byte Reload
	movq	%rdi, (%rcx)
	movq	%rsi, 8(%rcx)
	jmp	.LBB16_29
.LBB16_27:
	xorl	%eax, %eax
.LBB16_29:
	popq	%rbx
	popq	%r12
	popq	%r13
	popq	%r14
	popq	%r15
	popq	%rbp
	retq
.Lfunc_end16:
	.size	global_lookup, .Lfunc_end16-global_lookup
                                        # -- End function
	.type	.L.str,@object                  # @.str
	.section	.rodata.str1.1,"aMS",@progbits,1
.L.str:
	.asciz	"LDLAB_DEBUG=1"
	.size	.L.str, 14

	.type	g_debug,@object                 # @g_debug
	.local	g_debug
	.comm	g_debug,1,4
	.type	.L.str.1,@object                # @.str.1
.L.str.1:
	.asciz	"LDLAB_LIBRARY_PATH="
	.size	.L.str.1, 20

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
