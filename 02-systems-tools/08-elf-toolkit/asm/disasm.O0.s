	.file	"disasm.c"
	.text
	.globl	x86_decode                      # -- Begin function x86_decode
	.p2align	4
	.type	x86_decode,@function
x86_decode:                             # @x86_decode
# %bb.0:
	pushq	%rbp
	movq	%rsp, %rbp
	subq	$1872, %rsp                     # imm = 0x750
	movq	%rdi, -16(%rbp)
	movl	%esi, -20(%rbp)
	movq	%rdx, -32(%rbp)
	movq	%rcx, -40(%rbp)
	movq	-16(%rbp), %rax
	movq	%rax, -64(%rbp)
	leaq	-64(%rbp), %rax
	addq	$8, %rax
	movq	%rax, -1608(%rbp)               # 8-byte Spill
	cmpl	$15, -20(%rbp)
	jae	.LBB0_2
# %bb.1:
	movl	-20(%rbp), %eax
	movl	%eax, -1612(%rbp)               # 4-byte Spill
	jmp	.LBB0_3
.LBB0_2:
	movl	$15, %eax
	movl	%eax, -1612(%rbp)               # 4-byte Spill
	jmp	.LBB0_3
.LBB0_3:
	movq	-1608(%rbp), %rax               # 8-byte Reload
	movl	-1612(%rbp), %ecx               # 4-byte Reload
	movl	%ecx, (%rax)
	movl	$0, -52(%rbp)
	movl	$0, -48(%rbp)
	leaq	-64(%rbp), %rdi
	addq	$20, %rdi
	xorl	%esi, %esi
	movl	$4, %edx
	callq	memset@PLT
	movq	-40(%rbp), %rax
	addq	$24, %rax
	movq	%rax, -80(%rbp)
	movl	$64, -72(%rbp)
	movl	$0, -68(%rbp)
	movq	-40(%rbp), %rax
	movb	$0, 24(%rax)
	movq	-32(%rbp), %rcx
	movq	-40(%rbp), %rax
	movq	%rcx, (%rax)
	movq	-40(%rbp), %rax
	movl	$0, 12(%rax)
	movq	-40(%rbp), %rax
	movq	$0, 16(%rax)
	movl	$0, -84(%rbp)
	movl	$0, -88(%rbp)
	movl	$0, -92(%rbp)
	movl	$0, -96(%rbp)
.LBB0_4:                                # =>This Inner Loop Header: Depth=1
	movl	-52(%rbp), %eax
	cmpl	-56(%rbp), %eax
	jae	.LBB0_6
# %bb.5:                                #   in Loop: Header=BB0_4 Depth=1
	movq	-64(%rbp), %rax
	movl	-52(%rbp), %ecx
                                        # kill: def $rcx killed $ecx
	movzbl	(%rax,%rcx), %eax
	movl	%eax, -1616(%rbp)               # 4-byte Spill
	jmp	.LBB0_7
.LBB0_6:                                #   in Loop: Header=BB0_4 Depth=1
	movl	$256, %eax                      # imm = 0x100
	movl	%eax, -1616(%rbp)               # 4-byte Spill
	jmp	.LBB0_7
.LBB0_7:                                #   in Loop: Header=BB0_4 Depth=1
	movl	-1616(%rbp), %eax               # 4-byte Reload
	movl	%eax, -100(%rbp)
	cmpl	$102, -100(%rbp)
	jne	.LBB0_9
# %bb.8:                                #   in Loop: Header=BB0_4 Depth=1
	movl	$1, -84(%rbp)
	movl	-52(%rbp), %eax
	addl	$1, %eax
	movl	%eax, -52(%rbp)
	jmp	.LBB0_30
.LBB0_9:                                #   in Loop: Header=BB0_4 Depth=1
	cmpl	$103, -100(%rbp)
	jne	.LBB0_11
# %bb.10:                               #   in Loop: Header=BB0_4 Depth=1
	movl	-52(%rbp), %eax
	addl	$1, %eax
	movl	%eax, -52(%rbp)
	jmp	.LBB0_29
.LBB0_11:                               #   in Loop: Header=BB0_4 Depth=1
	cmpl	$240, -100(%rbp)
	jne	.LBB0_13
# %bb.12:                               #   in Loop: Header=BB0_4 Depth=1
	movl	$1, -96(%rbp)
	movl	-52(%rbp), %eax
	addl	$1, %eax
	movl	%eax, -52(%rbp)
	jmp	.LBB0_28
.LBB0_13:                               #   in Loop: Header=BB0_4 Depth=1
	cmpl	$243, -100(%rbp)
	jne	.LBB0_15
# %bb.14:                               #   in Loop: Header=BB0_4 Depth=1
	movl	$1, -88(%rbp)
	movl	-52(%rbp), %eax
	addl	$1, %eax
	movl	%eax, -52(%rbp)
	jmp	.LBB0_27
.LBB0_15:                               #   in Loop: Header=BB0_4 Depth=1
	cmpl	$242, -100(%rbp)
	jne	.LBB0_17
# %bb.16:                               #   in Loop: Header=BB0_4 Depth=1
	movl	$1, -92(%rbp)
	movl	-52(%rbp), %eax
	addl	$1, %eax
	movl	%eax, -52(%rbp)
	jmp	.LBB0_26
.LBB0_17:                               #   in Loop: Header=BB0_4 Depth=1
	cmpl	$46, -100(%rbp)
	je	.LBB0_23
# %bb.18:                               #   in Loop: Header=BB0_4 Depth=1
	cmpl	$54, -100(%rbp)
	je	.LBB0_23
# %bb.19:                               #   in Loop: Header=BB0_4 Depth=1
	cmpl	$62, -100(%rbp)
	je	.LBB0_23
# %bb.20:                               #   in Loop: Header=BB0_4 Depth=1
	cmpl	$38, -100(%rbp)
	je	.LBB0_23
# %bb.21:                               #   in Loop: Header=BB0_4 Depth=1
	cmpl	$100, -100(%rbp)
	je	.LBB0_23
# %bb.22:                               #   in Loop: Header=BB0_4 Depth=1
	cmpl	$101, -100(%rbp)
	jne	.LBB0_24
.LBB0_23:                               #   in Loop: Header=BB0_4 Depth=1
	movl	-52(%rbp), %eax
	addl	$1, %eax
	movl	%eax, -52(%rbp)
	jmp	.LBB0_25
.LBB0_24:
	jmp	.LBB0_31
.LBB0_25:                               #   in Loop: Header=BB0_4 Depth=1
	jmp	.LBB0_26
.LBB0_26:                               #   in Loop: Header=BB0_4 Depth=1
	jmp	.LBB0_27
.LBB0_27:                               #   in Loop: Header=BB0_4 Depth=1
	jmp	.LBB0_28
.LBB0_28:                               #   in Loop: Header=BB0_4 Depth=1
	jmp	.LBB0_29
.LBB0_29:                               #   in Loop: Header=BB0_4 Depth=1
	jmp	.LBB0_30
.LBB0_30:                               #   in Loop: Header=BB0_4 Depth=1
	jmp	.LBB0_4
.LBB0_31:
	movl	$0, -104(%rbp)
	movl	$0, -108(%rbp)
	movl	$0, -112(%rbp)
	movl	$0, -116(%rbp)
	movl	$0, -120(%rbp)
	movl	-52(%rbp), %eax
	cmpl	-56(%rbp), %eax
	jae	.LBB0_33
# %bb.32:
	movq	-64(%rbp), %rax
	movl	-52(%rbp), %ecx
                                        # kill: def $rcx killed $ecx
	movzbl	(%rax,%rcx), %eax
	movl	%eax, -1620(%rbp)               # 4-byte Spill
	jmp	.LBB0_34
.LBB0_33:
	movl	$256, %eax                      # imm = 0x100
	movl	%eax, -1620(%rbp)               # 4-byte Spill
	jmp	.LBB0_34
.LBB0_34:
	movl	-1620(%rbp), %eax               # 4-byte Reload
	movl	%eax, -124(%rbp)
	cmpl	$64, -124(%rbp)
	jb	.LBB0_37
# %bb.35:
	cmpl	$79, -124(%rbp)
	ja	.LBB0_37
# %bb.36:
	movl	$1, -120(%rbp)
	movl	-124(%rbp), %eax
	shrl	$3, %eax
	andl	$1, %eax
	movl	%eax, -104(%rbp)
	movl	-124(%rbp), %eax
	shrl	$2, %eax
	andl	$1, %eax
	movl	%eax, -108(%rbp)
	movl	-124(%rbp), %eax
	shrl	%eax
	andl	$1, %eax
	movl	%eax, -112(%rbp)
	movl	-124(%rbp), %eax
	andl	$1, %eax
	movl	%eax, -116(%rbp)
	movl	-52(%rbp), %eax
	addl	$1, %eax
	movl	%eax, -52(%rbp)
.LBB0_37:
	cmpl	$0, -104(%rbp)
	je	.LBB0_39
# %bb.38:
	movl	$8, %eax
	movl	%eax, -1624(%rbp)               # 4-byte Spill
	jmp	.LBB0_40
.LBB0_39:
	movl	-84(%rbp), %edx
	movl	$4, %eax
	movl	$2, %ecx
	cmpl	$0, %edx
	cmovnel	%ecx, %eax
	movl	%eax, -1624(%rbp)               # 4-byte Spill
.LBB0_40:
	movl	-1624(%rbp), %eax               # 4-byte Reload
	movl	%eax, -128(%rbp)
	movl	$0, -132(%rbp)
	movq	$0, -144(%rbp)
	cmpl	$0, -96(%rbp)
	je	.LBB0_42
# %bb.41:
	leaq	-80(%rbp), %rdi
	leaq	.L.str(%rip), %rsi
	callq	sb_puts
.LBB0_42:
	leaq	-64(%rbp), %rdi
	callq	rd8
	movl	%eax, -148(%rbp)
	cmpl	$15, -148(%rbp)
	jne	.LBB0_188
# %bb.43:
	leaq	-64(%rbp), %rdi
	callq	rd8
	movl	%eax, -152(%rbp)
	cmpl	$30, -152(%rbp)
	jne	.LBB0_52
# %bb.44:
	cmpl	$0, -88(%rbp)
	je	.LBB0_52
# %bb.45:
	leaq	-64(%rbp), %rdi
	callq	rd8
	movl	%eax, -156(%rbp)
	cmpl	$250, -156(%rbp)
	jne	.LBB0_47
# %bb.46:
	leaq	-80(%rbp), %rdi
	leaq	.L.str.1(%rip), %rsi
	callq	sb_puts
	jmp	.LBB0_51
.LBB0_47:
	cmpl	$251, -156(%rbp)
	jne	.LBB0_49
# %bb.48:
	leaq	-80(%rbp), %rdi
	leaq	.L.str.2(%rip), %rsi
	callq	sb_puts
	jmp	.LBB0_50
.LBB0_49:
	leaq	-80(%rbp), %rdi
	leaq	.L.str.3(%rip), %rsi
	callq	sb_puts
.LBB0_50:
	jmp	.LBB0_51
.LBB0_51:
	jmp	.LBB0_430
.LBB0_52:
	cmpl	$5, -152(%rbp)
	jne	.LBB0_54
# %bb.53:
	leaq	-80(%rbp), %rdi
	leaq	.L.str.4(%rip), %rsi
	callq	sb_puts
	jmp	.LBB0_430
.LBB0_54:
	cmpl	$11, -152(%rbp)
	jne	.LBB0_56
# %bb.55:
	leaq	-80(%rbp), %rdi
	leaq	.L.str.5(%rip), %rsi
	callq	sb_puts
	jmp	.LBB0_430
.LBB0_56:
	cmpl	$49, -152(%rbp)
	jne	.LBB0_58
# %bb.57:
	leaq	-80(%rbp), %rdi
	leaq	.L.str.6(%rip), %rsi
	callq	sb_puts
	jmp	.LBB0_430
.LBB0_58:
	cmpl	$162, -152(%rbp)
	jne	.LBB0_60
# %bb.59:
	leaq	-80(%rbp), %rdi
	leaq	.L.str.7(%rip), %rsi
	callq	sb_puts
	jmp	.LBB0_430
.LBB0_60:
	cmpl	$31, -152(%rbp)
	jne	.LBB0_62
# %bb.61:
	movl	-108(%rbp), %esi
	movl	-112(%rbp), %edx
	movl	-116(%rbp), %ecx
	leaq	-64(%rbp), %rdi
	leaq	-216(%rbp), %r8
	callq	parse_modrm
	leaq	-80(%rbp), %rdi
	leaq	.L.str.3(%rip), %rsi
	callq	sb_puts
	jmp	.LBB0_430
.LBB0_62:
	cmpl	$128, -152(%rbp)
	jb	.LBB0_65
# %bb.63:
	cmpl	$143, -152(%rbp)
	ja	.LBB0_65
# %bb.64:
	leaq	-64(%rbp), %rdi
	movl	$4, %esi
	callq	rd_imm_sext
	movq	%rax, -224(%rbp)
	leaq	-80(%rbp), %rdi
	movl	$106, %esi
	callq	sb_putc
	movl	-152(%rbp), %eax
	andl	$15, %eax
	movl	%eax, %eax
	movl	%eax, %ecx
	leaq	CC(%rip), %rax
	movq	(%rax,%rcx,8), %rsi
	leaq	-80(%rbp), %rdi
	callq	sb_puts
	leaq	-80(%rbp), %rdi
	movl	$32, %esi
	callq	sb_putc
	movq	-40(%rbp), %rax
	movl	$1, 12(%rax)
	movq	-32(%rbp), %rcx
	movl	-52(%rbp), %eax
                                        # kill: def $rax killed $eax
	addq	%rax, %rcx
	addq	-224(%rbp), %rcx
	movq	-40(%rbp), %rax
	movq	%rcx, 16(%rax)
	movq	-40(%rbp), %rax
	movq	16(%rax), %rsi
	leaq	-80(%rbp), %rdi
	callq	sb_0xhex
	jmp	.LBB0_430
.LBB0_65:
	cmpl	$144, -152(%rbp)
	jb	.LBB0_68
# %bb.66:
	cmpl	$159, -152(%rbp)
	ja	.LBB0_68
# %bb.67:
	movl	-108(%rbp), %esi
	movl	-112(%rbp), %edx
	movl	-116(%rbp), %ecx
	leaq	-64(%rbp), %rdi
	leaq	-280(%rbp), %r8
	callq	parse_modrm
	leaq	-80(%rbp), %rdi
	leaq	.L.str.8(%rip), %rsi
	callq	sb_puts
	movl	-152(%rbp), %eax
	andl	$15, %eax
	movl	%eax, %eax
	movl	%eax, %ecx
	leaq	CC(%rip), %rax
	movq	(%rax,%rcx,8), %rsi
	leaq	-80(%rbp), %rdi
	callq	sb_puts
	leaq	-80(%rbp), %rdi
	movl	$32, %esi
	callq	sb_putc
	movl	-120(%rbp), %ecx
	leaq	-80(%rbp), %rdi
	leaq	-280(%rbp), %rsi
	movl	$1, %r8d
	xorl	%r9d, %r9d
	movl	%r8d, %edx
	callq	render_rm
	jmp	.LBB0_430
.LBB0_68:
	cmpl	$64, -152(%rbp)
	jb	.LBB0_71
# %bb.69:
	cmpl	$79, -152(%rbp)
	ja	.LBB0_71
# %bb.70:
	movl	-108(%rbp), %esi
	movl	-112(%rbp), %edx
	movl	-116(%rbp), %ecx
	leaq	-64(%rbp), %rdi
	leaq	-336(%rbp), %r8
	callq	parse_modrm
	leaq	-80(%rbp), %rdi
	leaq	.L.str.9(%rip), %rsi
	callq	sb_puts
	movl	-152(%rbp), %eax
	andl	$15, %eax
	movl	%eax, %eax
	movl	%eax, %ecx
	leaq	CC(%rip), %rax
	movq	(%rax,%rcx,8), %rsi
	leaq	-80(%rbp), %rdi
	callq	sb_puts
	leaq	-80(%rbp), %rdi
	movl	$32, %esi
	callq	sb_putc
	movl	-332(%rbp), %edi
	movl	-128(%rbp), %esi
	movl	-120(%rbp), %edx
	callq	reg_name
	movq	%rax, %rsi
	leaq	-80(%rbp), %rdi
	callq	sb_puts
	leaq	-80(%rbp), %rdi
	leaq	.L.str.10(%rip), %rsi
	callq	sb_puts
	movl	-128(%rbp), %edx
	movl	-120(%rbp), %ecx
	leaq	-80(%rbp), %rdi
	leaq	-336(%rbp), %rsi
	xorl	%r9d, %r9d
	movl	%r9d, %r8d
	callq	render_rm
	jmp	.LBB0_430
.LBB0_71:
	cmpl	$182, -152(%rbp)
	je	.LBB0_75
# %bb.72:
	cmpl	$183, -152(%rbp)
	je	.LBB0_75
# %bb.73:
	cmpl	$190, -152(%rbp)
	je	.LBB0_75
# %bb.74:
	cmpl	$191, -152(%rbp)
	jne	.LBB0_81
.LBB0_75:
	movl	-152(%rbp), %edx
	andl	$1, %edx
	movl	$1, %eax
	movl	$2, %ecx
	cmpl	$0, %edx
	cmovnel	%ecx, %eax
	movl	%eax, -340(%rbp)
	movl	-108(%rbp), %esi
	movl	-112(%rbp), %edx
	movl	-116(%rbp), %ecx
	leaq	-64(%rbp), %rdi
	leaq	-400(%rbp), %r8
	callq	parse_modrm
	movl	-152(%rbp), %ecx
	andl	$8, %ecx
	leaq	.L.str.12(%rip), %rsi
	leaq	.L.str.11(%rip), %rax
	cmpl	$0, %ecx
	cmovneq	%rax, %rsi
	leaq	-80(%rbp), %rdi
	callq	sb_puts
	movl	-396(%rbp), %edi
	movl	-128(%rbp), %esi
	movl	-120(%rbp), %edx
	callq	reg_name
	movq	%rax, %rsi
	leaq	-80(%rbp), %rdi
	callq	sb_puts
	leaq	-80(%rbp), %rdi
	leaq	.L.str.10(%rip), %rsi
	callq	sb_puts
	movl	-340(%rbp), %eax
	movl	%eax, -1632(%rbp)               # 4-byte Spill
	movl	-120(%rbp), %eax
	movl	%eax, -1628(%rbp)               # 4-byte Spill
	cmpl	$0, -400(%rbp)
	je	.LBB0_77
# %bb.76:
	xorl	%eax, %eax
	movl	%eax, -1636(%rbp)               # 4-byte Spill
	jmp	.LBB0_78
.LBB0_77:
	movl	-340(%rbp), %eax
	movl	%eax, -1636(%rbp)               # 4-byte Spill
.LBB0_78:
	movl	-1628(%rbp), %ecx               # 4-byte Reload
	movl	-1632(%rbp), %edx               # 4-byte Reload
	movl	-1636(%rbp), %r8d               # 4-byte Reload
	leaq	-80(%rbp), %rdi
	leaq	-400(%rbp), %rsi
	xorl	%r9d, %r9d
	callq	render_rm
	cmpl	$0, -388(%rbp)
	je	.LBB0_80
# %bb.79:
	movl	$1, -132(%rbp)
	movq	-360(%rbp), %rax
	movq	%rax, -144(%rbp)
.LBB0_80:
	jmp	.LBB0_430
.LBB0_81:
	cmpl	$175, -152(%rbp)
	jne	.LBB0_85
# %bb.82:
	movl	-108(%rbp), %esi
	movl	-112(%rbp), %edx
	movl	-116(%rbp), %ecx
	leaq	-64(%rbp), %rdi
	leaq	-456(%rbp), %r8
	callq	parse_modrm
	leaq	-80(%rbp), %rdi
	leaq	.L.str.13(%rip), %rsi
	callq	sb_puts
	movl	-452(%rbp), %edi
	movl	-128(%rbp), %esi
	movl	-120(%rbp), %edx
	callq	reg_name
	movq	%rax, %rsi
	leaq	-80(%rbp), %rdi
	callq	sb_puts
	leaq	-80(%rbp), %rdi
	leaq	.L.str.10(%rip), %rsi
	callq	sb_puts
	movl	-128(%rbp), %edx
	movl	-120(%rbp), %ecx
	leaq	-80(%rbp), %rdi
	leaq	-456(%rbp), %rsi
	xorl	%r9d, %r9d
	movl	%r9d, %r8d
	callq	render_rm
	cmpl	$0, -444(%rbp)
	je	.LBB0_84
# %bb.83:
	movl	$1, -132(%rbp)
	movq	-416(%rbp), %rax
	movq	%rax, -144(%rbp)
.LBB0_84:
	jmp	.LBB0_430
.LBB0_85:
	cmpl	$16, -152(%rbp)
	je	.LBB0_110
# %bb.86:
	cmpl	$17, -152(%rbp)
	je	.LBB0_110
# %bb.87:
	cmpl	$40, -152(%rbp)
	je	.LBB0_110
# %bb.88:
	cmpl	$41, -152(%rbp)
	je	.LBB0_110
# %bb.89:
	cmpl	$42, -152(%rbp)
	je	.LBB0_110
# %bb.90:
	cmpl	$44, -152(%rbp)
	je	.LBB0_110
# %bb.91:
	cmpl	$45, -152(%rbp)
	je	.LBB0_110
# %bb.92:
	cmpl	$46, -152(%rbp)
	je	.LBB0_110
# %bb.93:
	cmpl	$47, -152(%rbp)
	je	.LBB0_110
# %bb.94:
	cmpl	$81, -152(%rbp)
	je	.LBB0_110
# %bb.95:
	cmpl	$84, -152(%rbp)
	je	.LBB0_110
# %bb.96:
	cmpl	$87, -152(%rbp)
	je	.LBB0_110
# %bb.97:
	cmpl	$88, -152(%rbp)
	je	.LBB0_110
# %bb.98:
	cmpl	$89, -152(%rbp)
	je	.LBB0_110
# %bb.99:
	cmpl	$90, -152(%rbp)
	je	.LBB0_110
# %bb.100:
	cmpl	$92, -152(%rbp)
	je	.LBB0_110
# %bb.101:
	cmpl	$93, -152(%rbp)
	je	.LBB0_110
# %bb.102:
	cmpl	$94, -152(%rbp)
	je	.LBB0_110
# %bb.103:
	cmpl	$95, -152(%rbp)
	je	.LBB0_110
# %bb.104:
	cmpl	$110, -152(%rbp)
	je	.LBB0_110
# %bb.105:
	cmpl	$111, -152(%rbp)
	je	.LBB0_110
# %bb.106:
	cmpl	$126, -152(%rbp)
	je	.LBB0_110
# %bb.107:
	cmpl	$127, -152(%rbp)
	je	.LBB0_110
# %bb.108:
	cmpl	$214, -152(%rbp)
	je	.LBB0_110
# %bb.109:
	cmpl	$239, -152(%rbp)
	jne	.LBB0_187
.LBB0_110:
	movl	-108(%rbp), %esi
	movl	-112(%rbp), %edx
	movl	-116(%rbp), %ecx
	leaq	-64(%rbp), %rdi
	leaq	-512(%rbp), %r8
	callq	parse_modrm
	leaq	.L.str.14(%rip), %rax
	movq	%rax, -520(%rbp)
	cmpl	$16, -152(%rbp)
	je	.LBB0_112
# %bb.111:
	cmpl	$17, -152(%rbp)
	jne	.LBB0_116
.LBB0_112:
	cmpl	$0, -88(%rbp)
	je	.LBB0_114
# %bb.113:
	leaq	.L.str.15(%rip), %rax
	movq	%rax, -1648(%rbp)               # 8-byte Spill
	jmp	.LBB0_115
.LBB0_114:
	movl	-92(%rbp), %edx
	leaq	.L.str.17(%rip), %rax
	leaq	.L.str.16(%rip), %rcx
	cmpl	$0, %edx
	cmovneq	%rcx, %rax
	movq	%rax, -1648(%rbp)               # 8-byte Spill
.LBB0_115:
	movq	-1648(%rbp), %rax               # 8-byte Reload
	movq	%rax, -520(%rbp)
	jmp	.LBB0_184
.LBB0_116:
	cmpl	$40, -152(%rbp)
	je	.LBB0_118
# %bb.117:
	cmpl	$41, -152(%rbp)
	jne	.LBB0_119
.LBB0_118:
	leaq	.L.str.18(%rip), %rax
	movq	%rax, -520(%rbp)
	jmp	.LBB0_183
.LBB0_119:
	cmpl	$42, -152(%rbp)
	jne	.LBB0_124
# %bb.120:
	cmpl	$0, -88(%rbp)
	je	.LBB0_122
# %bb.121:
	leaq	.L.str.19(%rip), %rax
	movq	%rax, -1656(%rbp)               # 8-byte Spill
	jmp	.LBB0_123
.LBB0_122:
	movl	-92(%rbp), %edx
	leaq	.L.str.21(%rip), %rax
	leaq	.L.str.20(%rip), %rcx
	cmpl	$0, %edx
	cmovneq	%rcx, %rax
	movq	%rax, -1656(%rbp)               # 8-byte Spill
.LBB0_123:
	movq	-1656(%rbp), %rax               # 8-byte Reload
	movq	%rax, -520(%rbp)
	jmp	.LBB0_182
.LBB0_124:
	cmpl	$44, -152(%rbp)
	jne	.LBB0_126
# %bb.125:
	movl	-88(%rbp), %edx
	leaq	.L.str.23(%rip), %rax
	leaq	.L.str.22(%rip), %rcx
	cmpl	$0, %edx
	cmovneq	%rcx, %rax
	movq	%rax, -520(%rbp)
	jmp	.LBB0_181
.LBB0_126:
	cmpl	$46, -152(%rbp)
	jne	.LBB0_128
# %bb.127:
	leaq	.L.str.24(%rip), %rax
	movq	%rax, -520(%rbp)
	jmp	.LBB0_180
.LBB0_128:
	cmpl	$47, -152(%rbp)
	jne	.LBB0_130
# %bb.129:
	leaq	.L.str.25(%rip), %rax
	movq	%rax, -520(%rbp)
	jmp	.LBB0_179
.LBB0_130:
	cmpl	$87, -152(%rbp)
	jne	.LBB0_132
# %bb.131:
	leaq	.L.str.26(%rip), %rax
	movq	%rax, -520(%rbp)
	jmp	.LBB0_178
.LBB0_132:
	cmpl	$84, -152(%rbp)
	jne	.LBB0_134
# %bb.133:
	leaq	.L.str.27(%rip), %rax
	movq	%rax, -520(%rbp)
	jmp	.LBB0_177
.LBB0_134:
	cmpl	$88, -152(%rbp)
	jne	.LBB0_139
# %bb.135:
	cmpl	$0, -88(%rbp)
	je	.LBB0_137
# %bb.136:
	leaq	.L.str.28(%rip), %rax
	movq	%rax, -1664(%rbp)               # 8-byte Spill
	jmp	.LBB0_138
.LBB0_137:
	movl	-92(%rbp), %edx
	leaq	.L.str.30(%rip), %rax
	leaq	.L.str.29(%rip), %rcx
	cmpl	$0, %edx
	cmovneq	%rcx, %rax
	movq	%rax, -1664(%rbp)               # 8-byte Spill
.LBB0_138:
	movq	-1664(%rbp), %rax               # 8-byte Reload
	movq	%rax, -520(%rbp)
	jmp	.LBB0_176
.LBB0_139:
	cmpl	$89, -152(%rbp)
	jne	.LBB0_144
# %bb.140:
	cmpl	$0, -88(%rbp)
	je	.LBB0_142
# %bb.141:
	leaq	.L.str.31(%rip), %rax
	movq	%rax, -1672(%rbp)               # 8-byte Spill
	jmp	.LBB0_143
.LBB0_142:
	movl	-92(%rbp), %edx
	leaq	.L.str.33(%rip), %rax
	leaq	.L.str.32(%rip), %rcx
	cmpl	$0, %edx
	cmovneq	%rcx, %rax
	movq	%rax, -1672(%rbp)               # 8-byte Spill
.LBB0_143:
	movq	-1672(%rbp), %rax               # 8-byte Reload
	movq	%rax, -520(%rbp)
	jmp	.LBB0_175
.LBB0_144:
	cmpl	$92, -152(%rbp)
	jne	.LBB0_149
# %bb.145:
	cmpl	$0, -88(%rbp)
	je	.LBB0_147
# %bb.146:
	leaq	.L.str.34(%rip), %rax
	movq	%rax, -1680(%rbp)               # 8-byte Spill
	jmp	.LBB0_148
.LBB0_147:
	movl	-92(%rbp), %edx
	leaq	.L.str.36(%rip), %rax
	leaq	.L.str.35(%rip), %rcx
	cmpl	$0, %edx
	cmovneq	%rcx, %rax
	movq	%rax, -1680(%rbp)               # 8-byte Spill
.LBB0_148:
	movq	-1680(%rbp), %rax               # 8-byte Reload
	movq	%rax, -520(%rbp)
	jmp	.LBB0_174
.LBB0_149:
	cmpl	$94, -152(%rbp)
	jne	.LBB0_154
# %bb.150:
	cmpl	$0, -88(%rbp)
	je	.LBB0_152
# %bb.151:
	leaq	.L.str.37(%rip), %rax
	movq	%rax, -1688(%rbp)               # 8-byte Spill
	jmp	.LBB0_153
.LBB0_152:
	movl	-92(%rbp), %edx
	leaq	.L.str.39(%rip), %rax
	leaq	.L.str.38(%rip), %rcx
	cmpl	$0, %edx
	cmovneq	%rcx, %rax
	movq	%rax, -1688(%rbp)               # 8-byte Spill
.LBB0_153:
	movq	-1688(%rbp), %rax               # 8-byte Reload
	movq	%rax, -520(%rbp)
	jmp	.LBB0_173
.LBB0_154:
	cmpl	$90, -152(%rbp)
	jne	.LBB0_159
# %bb.155:
	cmpl	$0, -88(%rbp)
	je	.LBB0_157
# %bb.156:
	leaq	.L.str.40(%rip), %rax
	movq	%rax, -1696(%rbp)               # 8-byte Spill
	jmp	.LBB0_158
.LBB0_157:
	movl	-92(%rbp), %edx
	leaq	.L.str.42(%rip), %rax
	leaq	.L.str.41(%rip), %rcx
	cmpl	$0, %edx
	cmovneq	%rcx, %rax
	movq	%rax, -1696(%rbp)               # 8-byte Spill
.LBB0_158:
	movq	-1696(%rbp), %rax               # 8-byte Reload
	movq	%rax, -520(%rbp)
	jmp	.LBB0_172
.LBB0_159:
	cmpl	$110, -152(%rbp)
	jne	.LBB0_161
# %bb.160:
	leaq	.L.str.43(%rip), %rax
	movq	%rax, -520(%rbp)
	jmp	.LBB0_171
.LBB0_161:
	cmpl	$126, -152(%rbp)
	jne	.LBB0_163
# %bb.162:
	movl	-88(%rbp), %edx
	leaq	.L.str.43(%rip), %rax
	leaq	.L.str.44(%rip), %rcx
	cmpl	$0, %edx
	cmovneq	%rcx, %rax
	movq	%rax, -520(%rbp)
	jmp	.LBB0_170
.LBB0_163:
	cmpl	$111, -152(%rbp)
	je	.LBB0_165
# %bb.164:
	cmpl	$127, -152(%rbp)
	jne	.LBB0_166
.LBB0_165:
	movl	-88(%rbp), %edx
	leaq	.L.str.46(%rip), %rax
	leaq	.L.str.45(%rip), %rcx
	cmpl	$0, %edx
	cmovneq	%rcx, %rax
	movq	%rax, -520(%rbp)
	jmp	.LBB0_169
.LBB0_166:
	cmpl	$239, -152(%rbp)
	jne	.LBB0_168
# %bb.167:
	leaq	.L.str.47(%rip), %rax
	movq	%rax, -520(%rbp)
.LBB0_168:
	jmp	.LBB0_169
.LBB0_169:
	jmp	.LBB0_170
.LBB0_170:
	jmp	.LBB0_171
.LBB0_171:
	jmp	.LBB0_172
.LBB0_172:
	jmp	.LBB0_173
.LBB0_173:
	jmp	.LBB0_174
.LBB0_174:
	jmp	.LBB0_175
.LBB0_175:
	jmp	.LBB0_176
.LBB0_176:
	jmp	.LBB0_177
.LBB0_177:
	jmp	.LBB0_178
.LBB0_178:
	jmp	.LBB0_179
.LBB0_179:
	jmp	.LBB0_180
.LBB0_180:
	jmp	.LBB0_181
.LBB0_181:
	jmp	.LBB0_182
.LBB0_182:
	jmp	.LBB0_183
.LBB0_183:
	jmp	.LBB0_184
.LBB0_184:
	movq	-520(%rbp), %rsi
	leaq	-80(%rbp), %rdi
	callq	sb_puts
	leaq	-80(%rbp), %rdi
	movl	$32, %esi
	callq	sb_putc
	movl	-508(%rbp), %eax
	andl	$15, %eax
	movslq	%eax, %rcx
	leaq	XMM(%rip), %rax
	movq	(%rax,%rcx,8), %rsi
	leaq	-80(%rbp), %rdi
	callq	sb_puts
	leaq	-80(%rbp), %rdi
	leaq	.L.str.10(%rip), %rsi
	callq	sb_puts
	movl	-128(%rbp), %edx
	movl	-120(%rbp), %ecx
	leaq	-80(%rbp), %rdi
	leaq	-512(%rbp), %rsi
	xorl	%r8d, %r8d
	movl	$1, %r9d
	callq	render_rm
	cmpl	$0, -500(%rbp)
	je	.LBB0_186
# %bb.185:
	movl	$1, -132(%rbp)
	movq	-472(%rbp), %rax
	movq	%rax, -144(%rbp)
.LBB0_186:
	jmp	.LBB0_430
.LBB0_187:
	leaq	-80(%rbp), %rdi
	leaq	.L.str.48(%rip), %rsi
	callq	sb_puts
	jmp	.LBB0_430
.LBB0_188:
	cmpl	$64, -148(%rbp)
	jae	.LBB0_208
# %bb.189:
	movl	-148(%rbp), %eax
	andl	$7, %eax
	cmpl	$6, %eax
	jae	.LBB0_208
# %bb.190:
	movl	-148(%rbp), %eax
	shrl	$3, %eax
	andl	$7, %eax
	movl	%eax, %eax
	movl	%eax, %ecx
	leaq	ALU(%rip), %rax
	movq	(%rax,%rcx,8), %rax
	movq	%rax, -528(%rbp)
	movl	-148(%rbp), %eax
	andl	$7, %eax
	movl	%eax, -532(%rbp)
	cmpl	$4, -532(%rbp)
	jne	.LBB0_192
# %bb.191:
	leaq	-64(%rbp), %rdi
	movl	$1, %esi
	callq	rd_imm_sext
	movq	%rax, -544(%rbp)
	movq	-528(%rbp), %rsi
	leaq	-80(%rbp), %rdi
	callq	sb_puts
	leaq	-80(%rbp), %rdi
	leaq	.L.str.49(%rip), %rsi
	callq	sb_puts
	movq	-544(%rbp), %rsi
	leaq	-80(%rbp), %rdi
	callq	sb_0xhex
	jmp	.LBB0_207
.LBB0_192:
	cmpl	$5, -532(%rbp)
	jne	.LBB0_194
# %bb.193:
	movl	-128(%rbp), %ecx
	movl	$4, %esi
	movl	$2, %eax
	cmpl	$2, %ecx
	cmovel	%eax, %esi
	leaq	-64(%rbp), %rdi
	callq	rd_imm_sext
	movq	%rax, -552(%rbp)
	movq	-528(%rbp), %rsi
	leaq	-80(%rbp), %rdi
	callq	sb_puts
	leaq	-80(%rbp), %rdi
	movl	$32, %esi
	callq	sb_putc
	movl	-128(%rbp), %esi
	movl	-120(%rbp), %edx
	xorl	%edi, %edi
	callq	reg_name
	movq	%rax, %rsi
	leaq	-80(%rbp), %rdi
	callq	sb_puts
	leaq	-80(%rbp), %rdi
	leaq	.L.str.10(%rip), %rsi
	callq	sb_puts
	movq	-552(%rbp), %rsi
	leaq	-80(%rbp), %rdi
	callq	sb_0xhex
	jmp	.LBB0_206
.LBB0_194:
	cmpl	$0, -532(%rbp)
	je	.LBB0_196
# %bb.195:
	cmpl	$2, -532(%rbp)
	jne	.LBB0_197
.LBB0_196:
	movl	$1, %eax
	movl	%eax, -1700(%rbp)               # 4-byte Spill
	jmp	.LBB0_198
.LBB0_197:
	movl	-128(%rbp), %eax
	movl	%eax, -1700(%rbp)               # 4-byte Spill
.LBB0_198:
	movl	-1700(%rbp), %eax               # 4-byte Reload
	movl	%eax, -556(%rbp)
	movb	$1, %al
	cmpl	$2, -532(%rbp)
	movb	%al, -1701(%rbp)                # 1-byte Spill
	je	.LBB0_200
# %bb.199:
	cmpl	$3, -532(%rbp)
	sete	%al
	movb	%al, -1701(%rbp)                # 1-byte Spill
.LBB0_200:
	movb	-1701(%rbp), %al                # 1-byte Reload
	andb	$1, %al
	movzbl	%al, %eax
	movl	%eax, -560(%rbp)
	movl	-108(%rbp), %esi
	movl	-112(%rbp), %edx
	movl	-116(%rbp), %ecx
	leaq	-64(%rbp), %rdi
	leaq	-616(%rbp), %r8
	callq	parse_modrm
	movq	-528(%rbp), %rsi
	leaq	-80(%rbp), %rdi
	callq	sb_puts
	leaq	-80(%rbp), %rdi
	movl	$32, %esi
	callq	sb_putc
	cmpl	$0, -560(%rbp)
	je	.LBB0_202
# %bb.201:
	movl	-612(%rbp), %edi
	movl	-556(%rbp), %esi
	movl	-120(%rbp), %edx
	callq	reg_name
	movq	%rax, %rsi
	leaq	-80(%rbp), %rdi
	callq	sb_puts
	leaq	-80(%rbp), %rdi
	leaq	.L.str.10(%rip), %rsi
	callq	sb_puts
	movl	-556(%rbp), %edx
	movl	-120(%rbp), %ecx
	leaq	-80(%rbp), %rdi
	leaq	-616(%rbp), %rsi
	xorl	%r9d, %r9d
	movl	%r9d, %r8d
	callq	render_rm
	jmp	.LBB0_203
.LBB0_202:
	movl	-556(%rbp), %edx
	movl	-120(%rbp), %ecx
	leaq	-80(%rbp), %rdi
	leaq	-616(%rbp), %rsi
	xorl	%r9d, %r9d
	movl	%r9d, %r8d
	callq	render_rm
	leaq	-80(%rbp), %rdi
	leaq	.L.str.10(%rip), %rsi
	callq	sb_puts
	movl	-612(%rbp), %edi
	movl	-556(%rbp), %esi
	movl	-120(%rbp), %edx
	callq	reg_name
	movq	%rax, %rsi
	leaq	-80(%rbp), %rdi
	callq	sb_puts
.LBB0_203:
	cmpl	$0, -604(%rbp)
	je	.LBB0_205
# %bb.204:
	movl	$1, -132(%rbp)
	movq	-576(%rbp), %rax
	movq	%rax, -144(%rbp)
.LBB0_205:
	jmp	.LBB0_206
.LBB0_206:
	jmp	.LBB0_207
.LBB0_207:
	jmp	.LBB0_430
.LBB0_208:
	cmpl	$80, -148(%rbp)
	jb	.LBB0_211
# %bb.209:
	cmpl	$87, -148(%rbp)
	ja	.LBB0_211
# %bb.210:
	leaq	-80(%rbp), %rdi
	leaq	.L.str.50(%rip), %rsi
	callq	sb_puts
	movl	-148(%rbp), %eax
	subl	$80, %eax
	movl	-116(%rbp), %esi
	xorl	%ecx, %ecx
	movl	$8, %edx
	cmpl	$0, %esi
	cmovnel	%edx, %ecx
	orl	%ecx, %eax
	movl	%eax, %eax
	movl	%eax, %ecx
	leaq	R64(%rip), %rax
	movq	(%rax,%rcx,8), %rsi
	leaq	-80(%rbp), %rdi
	callq	sb_puts
	jmp	.LBB0_430
.LBB0_211:
	cmpl	$88, -148(%rbp)
	jb	.LBB0_214
# %bb.212:
	cmpl	$95, -148(%rbp)
	ja	.LBB0_214
# %bb.213:
	leaq	-80(%rbp), %rdi
	leaq	.L.str.51(%rip), %rsi
	callq	sb_puts
	movl	-148(%rbp), %eax
	subl	$88, %eax
	movl	-116(%rbp), %esi
	xorl	%ecx, %ecx
	movl	$8, %edx
	cmpl	$0, %esi
	cmovnel	%edx, %ecx
	orl	%ecx, %eax
	movl	%eax, %eax
	movl	%eax, %ecx
	leaq	R64(%rip), %rax
	movq	(%rax,%rcx,8), %rsi
	leaq	-80(%rbp), %rdi
	callq	sb_puts
	jmp	.LBB0_430
.LBB0_214:
	cmpl	$99, -148(%rbp)
	jne	.LBB0_218
# %bb.215:
	movl	-108(%rbp), %esi
	movl	-112(%rbp), %edx
	movl	-116(%rbp), %ecx
	leaq	-64(%rbp), %rdi
	leaq	-672(%rbp), %r8
	callq	parse_modrm
	leaq	-80(%rbp), %rdi
	leaq	.L.str.52(%rip), %rsi
	callq	sb_puts
	movl	-668(%rbp), %edi
	movl	-120(%rbp), %edx
	movl	$8, %esi
	callq	reg_name
	movq	%rax, %rsi
	leaq	-80(%rbp), %rdi
	callq	sb_puts
	leaq	-80(%rbp), %rdi
	leaq	.L.str.10(%rip), %rsi
	callq	sb_puts
	movl	-120(%rbp), %ecx
	leaq	-80(%rbp), %rdi
	leaq	-672(%rbp), %rsi
	movl	$4, %edx
	xorl	%r9d, %r9d
	movl	%r9d, %r8d
	callq	render_rm
	cmpl	$0, -660(%rbp)
	je	.LBB0_217
# %bb.216:
	movl	$1, -132(%rbp)
	movq	-632(%rbp), %rax
	movq	%rax, -144(%rbp)
.LBB0_217:
	jmp	.LBB0_430
.LBB0_218:
	cmpl	$104, -148(%rbp)
	je	.LBB0_220
# %bb.219:
	cmpl	$106, -148(%rbp)
	jne	.LBB0_221
.LBB0_220:
	movl	-148(%rbp), %ecx
	movl	$1, %esi
	movl	$4, %eax
	cmpl	$104, %ecx
	cmovel	%eax, %esi
	leaq	-64(%rbp), %rdi
	callq	rd_imm_sext
	movq	%rax, -680(%rbp)
	leaq	-80(%rbp), %rdi
	leaq	.L.str.50(%rip), %rsi
	callq	sb_puts
	movq	-680(%rbp), %rsi
	leaq	-80(%rbp), %rdi
	callq	sb_0xhex
	jmp	.LBB0_430
.LBB0_221:
	cmpl	$105, -148(%rbp)
	je	.LBB0_223
# %bb.222:
	cmpl	$107, -148(%rbp)
	jne	.LBB0_229
.LBB0_223:
	movl	-108(%rbp), %esi
	movl	-112(%rbp), %edx
	movl	-116(%rbp), %ecx
	leaq	-64(%rbp), %rdi
	leaq	-736(%rbp), %r8
	callq	parse_modrm
	cmpl	$105, -148(%rbp)
	jne	.LBB0_225
# %bb.224:
	movl	-128(%rbp), %edx
	movl	$4, %eax
	movl	$2, %ecx
	cmpl	$2, %edx
	cmovel	%ecx, %eax
	movl	%eax, -1708(%rbp)               # 4-byte Spill
	jmp	.LBB0_226
.LBB0_225:
	movl	$1, %eax
	movl	%eax, -1708(%rbp)               # 4-byte Spill
	jmp	.LBB0_226
.LBB0_226:
	movl	-1708(%rbp), %esi               # 4-byte Reload
	leaq	-64(%rbp), %rdi
	callq	rd_imm_sext
	movq	%rax, -744(%rbp)
	leaq	-80(%rbp), %rdi
	leaq	.L.str.13(%rip), %rsi
	callq	sb_puts
	movl	-732(%rbp), %edi
	movl	-128(%rbp), %esi
	movl	-120(%rbp), %edx
	callq	reg_name
	movq	%rax, %rsi
	leaq	-80(%rbp), %rdi
	callq	sb_puts
	leaq	-80(%rbp), %rdi
	leaq	.L.str.10(%rip), %rsi
	callq	sb_puts
	movl	-128(%rbp), %edx
	movl	-120(%rbp), %ecx
	leaq	-80(%rbp), %rdi
	leaq	-736(%rbp), %rsi
	xorl	%r9d, %r9d
	movl	%r9d, %r8d
	callq	render_rm
	leaq	-80(%rbp), %rdi
	leaq	.L.str.10(%rip), %rsi
	callq	sb_puts
	movq	-744(%rbp), %rsi
	leaq	-80(%rbp), %rdi
	callq	sb_0xhex
	cmpl	$0, -724(%rbp)
	je	.LBB0_228
# %bb.227:
	movl	$1, -132(%rbp)
	movq	-696(%rbp), %rax
	movq	%rax, -144(%rbp)
.LBB0_228:
	jmp	.LBB0_430
.LBB0_229:
	cmpl	$112, -148(%rbp)
	jb	.LBB0_232
# %bb.230:
	cmpl	$127, -148(%rbp)
	ja	.LBB0_232
# %bb.231:
	leaq	-64(%rbp), %rdi
	movl	$1, %esi
	callq	rd_imm_sext
	movq	%rax, -752(%rbp)
	leaq	-80(%rbp), %rdi
	movl	$106, %esi
	callq	sb_putc
	movl	-148(%rbp), %eax
	andl	$15, %eax
	movl	%eax, %eax
	movl	%eax, %ecx
	leaq	CC(%rip), %rax
	movq	(%rax,%rcx,8), %rsi
	leaq	-80(%rbp), %rdi
	callq	sb_puts
	leaq	-80(%rbp), %rdi
	movl	$32, %esi
	callq	sb_putc
	movq	-40(%rbp), %rax
	movl	$1, 12(%rax)
	movq	-32(%rbp), %rcx
	movl	-52(%rbp), %eax
                                        # kill: def $rax killed $eax
	addq	%rax, %rcx
	addq	-752(%rbp), %rcx
	movq	-40(%rbp), %rax
	movq	%rcx, 16(%rax)
	movq	-40(%rbp), %rax
	movq	16(%rax), %rsi
	leaq	-80(%rbp), %rdi
	callq	sb_0xhex
	jmp	.LBB0_430
.LBB0_232:
	cmpl	$128, -148(%rbp)
	je	.LBB0_235
# %bb.233:
	cmpl	$129, -148(%rbp)
	je	.LBB0_235
# %bb.234:
	cmpl	$131, -148(%rbp)
	jne	.LBB0_247
.LBB0_235:
	cmpl	$128, -148(%rbp)
	jne	.LBB0_237
# %bb.236:
	movl	$1, %eax
	movl	%eax, -1712(%rbp)               # 4-byte Spill
	jmp	.LBB0_238
.LBB0_237:
	movl	-128(%rbp), %eax
	movl	%eax, -1712(%rbp)               # 4-byte Spill
.LBB0_238:
	movl	-1712(%rbp), %eax               # 4-byte Reload
	movl	%eax, -756(%rbp)
	movl	-108(%rbp), %esi
	movl	-112(%rbp), %edx
	movl	-116(%rbp), %ecx
	leaq	-64(%rbp), %rdi
	leaq	-816(%rbp), %r8
	callq	parse_modrm
	cmpl	$129, -148(%rbp)
	jne	.LBB0_240
# %bb.239:
	movl	-128(%rbp), %edx
	movl	$4, %eax
	movl	$2, %ecx
	cmpl	$2, %edx
	cmovel	%ecx, %eax
	movl	%eax, -1716(%rbp)               # 4-byte Spill
	jmp	.LBB0_241
.LBB0_240:
	movl	$1, %eax
	movl	%eax, -1716(%rbp)               # 4-byte Spill
	jmp	.LBB0_241
.LBB0_241:
	movl	-1716(%rbp), %eax               # 4-byte Reload
	movl	%eax, -820(%rbp)
	movl	-820(%rbp), %esi
	leaq	-64(%rbp), %rdi
	callq	rd_imm_sext
	movq	%rax, -832(%rbp)
	movslq	-768(%rbp), %rcx
	leaq	ALU(%rip), %rax
	movq	(%rax,%rcx,8), %rsi
	leaq	-80(%rbp), %rdi
	callq	sb_puts
	leaq	-80(%rbp), %rdi
	movl	$32, %esi
	callq	sb_putc
	movl	-756(%rbp), %eax
	movl	%eax, -1724(%rbp)               # 4-byte Spill
	movl	-120(%rbp), %eax
	movl	%eax, -1720(%rbp)               # 4-byte Spill
	cmpl	$0, -816(%rbp)
	je	.LBB0_243
# %bb.242:
	xorl	%eax, %eax
	movl	%eax, -1728(%rbp)               # 4-byte Spill
	jmp	.LBB0_244
.LBB0_243:
	movl	-756(%rbp), %eax
	movl	%eax, -1728(%rbp)               # 4-byte Spill
.LBB0_244:
	movl	-1720(%rbp), %ecx               # 4-byte Reload
	movl	-1724(%rbp), %edx               # 4-byte Reload
	movl	-1728(%rbp), %r8d               # 4-byte Reload
	leaq	-80(%rbp), %rdi
	leaq	-816(%rbp), %rsi
	xorl	%r9d, %r9d
	callq	render_rm
	leaq	-80(%rbp), %rdi
	leaq	.L.str.10(%rip), %rsi
	callq	sb_puts
	movq	-832(%rbp), %rsi
	leaq	-80(%rbp), %rdi
	callq	sb_0xhex
	cmpl	$0, -804(%rbp)
	je	.LBB0_246
# %bb.245:
	movl	$1, -132(%rbp)
	movq	-776(%rbp), %rax
	movq	%rax, -144(%rbp)
.LBB0_246:
	jmp	.LBB0_430
.LBB0_247:
	cmpl	$132, -148(%rbp)
	je	.LBB0_249
# %bb.248:
	cmpl	$133, -148(%rbp)
	jne	.LBB0_255
.LBB0_249:
	cmpl	$132, -148(%rbp)
	jne	.LBB0_251
# %bb.250:
	movl	$1, %eax
	movl	%eax, -1732(%rbp)               # 4-byte Spill
	jmp	.LBB0_252
.LBB0_251:
	movl	-128(%rbp), %eax
	movl	%eax, -1732(%rbp)               # 4-byte Spill
.LBB0_252:
	movl	-1732(%rbp), %eax               # 4-byte Reload
	movl	%eax, -836(%rbp)
	movl	-108(%rbp), %esi
	movl	-112(%rbp), %edx
	movl	-116(%rbp), %ecx
	leaq	-64(%rbp), %rdi
	leaq	-896(%rbp), %r8
	callq	parse_modrm
	leaq	-80(%rbp), %rdi
	leaq	.L.str.53(%rip), %rsi
	callq	sb_puts
	movl	-836(%rbp), %edx
	movl	-120(%rbp), %ecx
	leaq	-80(%rbp), %rdi
	leaq	-896(%rbp), %rsi
	xorl	%r9d, %r9d
	movl	%r9d, %r8d
	callq	render_rm
	leaq	-80(%rbp), %rdi
	leaq	.L.str.10(%rip), %rsi
	callq	sb_puts
	movl	-892(%rbp), %edi
	movl	-836(%rbp), %esi
	movl	-120(%rbp), %edx
	callq	reg_name
	movq	%rax, %rsi
	leaq	-80(%rbp), %rdi
	callq	sb_puts
	cmpl	$0, -884(%rbp)
	je	.LBB0_254
# %bb.253:
	movl	$1, -132(%rbp)
	movq	-856(%rbp), %rax
	movq	%rax, -144(%rbp)
.LBB0_254:
	jmp	.LBB0_430
.LBB0_255:
	cmpl	$134, -148(%rbp)
	je	.LBB0_257
# %bb.256:
	cmpl	$135, -148(%rbp)
	jne	.LBB0_261
.LBB0_257:
	cmpl	$134, -148(%rbp)
	jne	.LBB0_259
# %bb.258:
	movl	$1, %eax
	movl	%eax, -1736(%rbp)               # 4-byte Spill
	jmp	.LBB0_260
.LBB0_259:
	movl	-128(%rbp), %eax
	movl	%eax, -1736(%rbp)               # 4-byte Spill
.LBB0_260:
	movl	-1736(%rbp), %eax               # 4-byte Reload
	movl	%eax, -900(%rbp)
	movl	-108(%rbp), %esi
	movl	-112(%rbp), %edx
	movl	-116(%rbp), %ecx
	leaq	-64(%rbp), %rdi
	leaq	-960(%rbp), %r8
	callq	parse_modrm
	leaq	-80(%rbp), %rdi
	leaq	.L.str.54(%rip), %rsi
	callq	sb_puts
	movl	-900(%rbp), %edx
	movl	-120(%rbp), %ecx
	leaq	-80(%rbp), %rdi
	leaq	-960(%rbp), %rsi
	xorl	%r9d, %r9d
	movl	%r9d, %r8d
	callq	render_rm
	leaq	-80(%rbp), %rdi
	leaq	.L.str.10(%rip), %rsi
	callq	sb_puts
	movl	-956(%rbp), %edi
	movl	-900(%rbp), %esi
	movl	-120(%rbp), %edx
	callq	reg_name
	movq	%rax, %rsi
	leaq	-80(%rbp), %rdi
	callq	sb_puts
	jmp	.LBB0_430
.LBB0_261:
	cmpl	$136, -148(%rbp)
	jb	.LBB0_272
# %bb.262:
	cmpl	$139, -148(%rbp)
	ja	.LBB0_272
# %bb.263:
	movl	-148(%rbp), %eax
	andl	$1, %eax
	cmpl	$0, %eax
	je	.LBB0_265
# %bb.264:
	movl	-128(%rbp), %eax
	movl	%eax, -1740(%rbp)               # 4-byte Spill
	jmp	.LBB0_266
.LBB0_265:
	movl	$1, %eax
	movl	%eax, -1740(%rbp)               # 4-byte Spill
	jmp	.LBB0_266
.LBB0_266:
	movl	-1740(%rbp), %eax               # 4-byte Reload
	movl	%eax, -964(%rbp)
	movl	-148(%rbp), %eax
	andl	$2, %eax
	movl	%eax, -968(%rbp)
	movl	-108(%rbp), %esi
	movl	-112(%rbp), %edx
	movl	-116(%rbp), %ecx
	leaq	-64(%rbp), %rdi
	leaq	-1024(%rbp), %r8
	callq	parse_modrm
	leaq	-80(%rbp), %rdi
	leaq	.L.str.55(%rip), %rsi
	callq	sb_puts
	cmpl	$0, -968(%rbp)
	je	.LBB0_268
# %bb.267:
	movl	-1020(%rbp), %edi
	movl	-964(%rbp), %esi
	movl	-120(%rbp), %edx
	callq	reg_name
	movq	%rax, %rsi
	leaq	-80(%rbp), %rdi
	callq	sb_puts
	leaq	-80(%rbp), %rdi
	leaq	.L.str.10(%rip), %rsi
	callq	sb_puts
	movl	-964(%rbp), %edx
	movl	-120(%rbp), %ecx
	leaq	-80(%rbp), %rdi
	leaq	-1024(%rbp), %rsi
	xorl	%r9d, %r9d
	movl	%r9d, %r8d
	callq	render_rm
	jmp	.LBB0_269
.LBB0_268:
	movl	-964(%rbp), %edx
	movl	-120(%rbp), %ecx
	leaq	-80(%rbp), %rdi
	leaq	-1024(%rbp), %rsi
	xorl	%r9d, %r9d
	movl	%r9d, %r8d
	callq	render_rm
	leaq	-80(%rbp), %rdi
	leaq	.L.str.10(%rip), %rsi
	callq	sb_puts
	movl	-1020(%rbp), %edi
	movl	-964(%rbp), %esi
	movl	-120(%rbp), %edx
	callq	reg_name
	movq	%rax, %rsi
	leaq	-80(%rbp), %rdi
	callq	sb_puts
.LBB0_269:
	cmpl	$0, -1012(%rbp)
	je	.LBB0_271
# %bb.270:
	movl	$1, -132(%rbp)
	movq	-984(%rbp), %rax
	movq	%rax, -144(%rbp)
.LBB0_271:
	jmp	.LBB0_430
.LBB0_272:
	cmpl	$141, -148(%rbp)
	jne	.LBB0_276
# %bb.273:
	movl	-108(%rbp), %esi
	movl	-112(%rbp), %edx
	movl	-116(%rbp), %ecx
	leaq	-64(%rbp), %rdi
	leaq	-1080(%rbp), %r8
	callq	parse_modrm
	leaq	-80(%rbp), %rdi
	leaq	.L.str.56(%rip), %rsi
	callq	sb_puts
	movl	-1076(%rbp), %edi
	movl	-128(%rbp), %esi
	movl	-120(%rbp), %edx
	callq	reg_name
	movq	%rax, %rsi
	leaq	-80(%rbp), %rdi
	callq	sb_puts
	leaq	-80(%rbp), %rdi
	leaq	.L.str.10(%rip), %rsi
	callq	sb_puts
	leaq	-80(%rbp), %rdi
	leaq	-1080(%rbp), %rsi
	callq	render_mem
	cmpl	$0, -1068(%rbp)
	je	.LBB0_275
# %bb.274:
	movl	$1, -132(%rbp)
	movq	-1040(%rbp), %rax
	movq	%rax, -144(%rbp)
.LBB0_275:
	jmp	.LBB0_430
.LBB0_276:
	cmpl	$143, -148(%rbp)
	jne	.LBB0_278
# %bb.277:
	movl	-108(%rbp), %esi
	movl	-112(%rbp), %edx
	movl	-116(%rbp), %ecx
	leaq	-64(%rbp), %rdi
	leaq	-1136(%rbp), %r8
	callq	parse_modrm
	leaq	-80(%rbp), %rdi
	leaq	.L.str.51(%rip), %rsi
	callq	sb_puts
	movl	-120(%rbp), %ecx
	movl	-1136(%rbp), %edx
	movl	$8, %r8d
	xorl	%eax, %eax
	cmpl	$0, %edx
	cmovnel	%eax, %r8d
	leaq	-80(%rbp), %rdi
	leaq	-1136(%rbp), %rsi
	movl	$8, %edx
	xorl	%r9d, %r9d
	callq	render_rm
	jmp	.LBB0_430
.LBB0_278:
	cmpl	$144, -148(%rbp)
	jne	.LBB0_280
# %bb.279:
	movl	-88(%rbp), %ecx
	leaq	.L.str.3(%rip), %rsi
	leaq	.L.str.57(%rip), %rax
	cmpl	$0, %ecx
	cmovneq	%rax, %rsi
	leaq	-80(%rbp), %rdi
	callq	sb_puts
	jmp	.LBB0_430
.LBB0_280:
	cmpl	$145, -148(%rbp)
	jb	.LBB0_283
# %bb.281:
	cmpl	$151, -148(%rbp)
	ja	.LBB0_283
# %bb.282:
	leaq	-80(%rbp), %rdi
	leaq	.L.str.54(%rip), %rsi
	callq	sb_puts
	movl	-128(%rbp), %esi
	movl	-120(%rbp), %edx
	xorl	%edi, %edi
	callq	reg_name
	movq	%rax, %rsi
	leaq	-80(%rbp), %rdi
	callq	sb_puts
	leaq	-80(%rbp), %rdi
	leaq	.L.str.10(%rip), %rsi
	callq	sb_puts
	movl	-148(%rbp), %edi
	subl	$144, %edi
	movl	-116(%rbp), %edx
	xorl	%eax, %eax
	movl	$8, %ecx
	cmpl	$0, %edx
	cmovnel	%ecx, %eax
	orl	%eax, %edi
	movl	-128(%rbp), %esi
	movl	-120(%rbp), %edx
	callq	reg_name
	movq	%rax, %rsi
	leaq	-80(%rbp), %rdi
	callq	sb_puts
	jmp	.LBB0_430
.LBB0_283:
	cmpl	$152, -148(%rbp)
	jne	.LBB0_288
# %bb.284:
	cmpl	$0, -104(%rbp)
	je	.LBB0_286
# %bb.285:
	leaq	.L.str.58(%rip), %rax
	movq	%rax, -1752(%rbp)               # 8-byte Spill
	jmp	.LBB0_287
.LBB0_286:
	movl	-84(%rbp), %edx
	leaq	.L.str.60(%rip), %rax
	leaq	.L.str.59(%rip), %rcx
	cmpl	$0, %edx
	cmovneq	%rcx, %rax
	movq	%rax, -1752(%rbp)               # 8-byte Spill
.LBB0_287:
	movq	-1752(%rbp), %rsi               # 8-byte Reload
	leaq	-80(%rbp), %rdi
	callq	sb_puts
	jmp	.LBB0_430
.LBB0_288:
	cmpl	$153, -148(%rbp)
	jne	.LBB0_293
# %bb.289:
	cmpl	$0, -104(%rbp)
	je	.LBB0_291
# %bb.290:
	leaq	.L.str.61(%rip), %rax
	movq	%rax, -1760(%rbp)               # 8-byte Spill
	jmp	.LBB0_292
.LBB0_291:
	movl	-84(%rbp), %edx
	leaq	.L.str.63(%rip), %rax
	leaq	.L.str.62(%rip), %rcx
	cmpl	$0, %edx
	cmovneq	%rcx, %rax
	movq	%rax, -1760(%rbp)               # 8-byte Spill
.LBB0_292:
	movq	-1760(%rbp), %rsi               # 8-byte Reload
	leaq	-80(%rbp), %rdi
	callq	sb_puts
	jmp	.LBB0_430
.LBB0_293:
	cmpl	$164, -148(%rbp)
	je	.LBB0_299
# %bb.294:
	cmpl	$165, -148(%rbp)
	je	.LBB0_299
# %bb.295:
	cmpl	$166, -148(%rbp)
	je	.LBB0_299
# %bb.296:
	cmpl	$167, -148(%rbp)
	je	.LBB0_299
# %bb.297:
	cmpl	$170, -148(%rbp)
	jb	.LBB0_323
# %bb.298:
	cmpl	$175, -148(%rbp)
	ja	.LBB0_323
.LBB0_299:
	cmpl	$0, -88(%rbp)
	je	.LBB0_301
# %bb.300:
	leaq	-80(%rbp), %rdi
	leaq	.L.str.64(%rip), %rsi
	callq	sb_puts
	jmp	.LBB0_304
.LBB0_301:
	cmpl	$0, -92(%rbp)
	je	.LBB0_303
# %bb.302:
	leaq	-80(%rbp), %rdi
	leaq	.L.str.65(%rip), %rsi
	callq	sb_puts
.LBB0_303:
	jmp	.LBB0_304
.LBB0_304:
	movl	-148(%rbp), %eax
	andl	$1, %eax
	cmpl	$0, %eax
	setne	%al
	xorb	$-1, %al
	andb	$1, %al
	movzbl	%al, %eax
	movl	%eax, -1140(%rbp)
	leaq	.L.str.66(%rip), %rax
	movq	%rax, -1152(%rbp)
	cmpl	$166, -148(%rbp)
	je	.LBB0_306
# %bb.305:
	cmpl	$167, -148(%rbp)
	jne	.LBB0_307
.LBB0_306:
	leaq	.L.str.67(%rip), %rax
	movq	%rax, -1152(%rbp)
	jmp	.LBB0_319
.LBB0_307:
	cmpl	$170, -148(%rbp)
	je	.LBB0_309
# %bb.308:
	cmpl	$171, -148(%rbp)
	jne	.LBB0_310
.LBB0_309:
	leaq	.L.str.68(%rip), %rax
	movq	%rax, -1152(%rbp)
	jmp	.LBB0_318
.LBB0_310:
	cmpl	$172, -148(%rbp)
	je	.LBB0_312
# %bb.311:
	cmpl	$173, -148(%rbp)
	jne	.LBB0_313
.LBB0_312:
	leaq	.L.str.69(%rip), %rax
	movq	%rax, -1152(%rbp)
	jmp	.LBB0_317
.LBB0_313:
	cmpl	$174, -148(%rbp)
	je	.LBB0_315
# %bb.314:
	cmpl	$175, -148(%rbp)
	jne	.LBB0_316
.LBB0_315:
	leaq	.L.str.70(%rip), %rax
	movq	%rax, -1152(%rbp)
.LBB0_316:
	jmp	.LBB0_317
.LBB0_317:
	jmp	.LBB0_318
.LBB0_318:
	jmp	.LBB0_319
.LBB0_319:
	movq	-1152(%rbp), %rsi
	leaq	-80(%rbp), %rdi
	callq	sb_puts
	cmpl	$0, -1140(%rbp)
	je	.LBB0_321
# %bb.320:
	movl	$98, %eax
	movl	%eax, -1764(%rbp)               # 4-byte Spill
	jmp	.LBB0_322
.LBB0_321:
	movl	-128(%rbp), %edx
	movl	$100, %eax
	movl	$113, %ecx
	cmpl	$8, %edx
	cmovel	%ecx, %eax
	movl	%eax, -1764(%rbp)               # 4-byte Spill
.LBB0_322:
	movl	-1764(%rbp), %eax               # 4-byte Reload
                                        # kill: def $al killed $al killed $eax
	leaq	-80(%rbp), %rdi
	movsbl	%al, %esi
	callq	sb_putc
	jmp	.LBB0_430
.LBB0_323:
	cmpl	$168, -148(%rbp)
	je	.LBB0_325
# %bb.324:
	cmpl	$169, -148(%rbp)
	jne	.LBB0_332
.LBB0_325:
	cmpl	$168, -148(%rbp)
	jne	.LBB0_327
# %bb.326:
	movl	$1, %eax
	movl	%eax, -1768(%rbp)               # 4-byte Spill
	jmp	.LBB0_328
.LBB0_327:
	movl	-128(%rbp), %eax
	movl	%eax, -1768(%rbp)               # 4-byte Spill
.LBB0_328:
	movl	-1768(%rbp), %eax               # 4-byte Reload
	movl	%eax, -1156(%rbp)
	cmpl	$1, -1156(%rbp)
	jne	.LBB0_330
# %bb.329:
	movl	$1, %eax
	movl	%eax, -1772(%rbp)               # 4-byte Spill
	jmp	.LBB0_331
.LBB0_330:
	movl	-128(%rbp), %edx
	movl	$4, %eax
	movl	$2, %ecx
	cmpl	$2, %edx
	cmovel	%ecx, %eax
	movl	%eax, -1772(%rbp)               # 4-byte Spill
.LBB0_331:
	movl	-1772(%rbp), %esi               # 4-byte Reload
	leaq	-64(%rbp), %rdi
	callq	rd_imm_sext
	movq	%rax, -1168(%rbp)
	leaq	-80(%rbp), %rdi
	leaq	.L.str.53(%rip), %rsi
	callq	sb_puts
	movl	-1156(%rbp), %esi
	movl	-120(%rbp), %edx
	xorl	%edi, %edi
	callq	reg_name
	movq	%rax, %rsi
	leaq	-80(%rbp), %rdi
	callq	sb_puts
	leaq	-80(%rbp), %rdi
	leaq	.L.str.10(%rip), %rsi
	callq	sb_puts
	movq	-1168(%rbp), %rsi
	leaq	-80(%rbp), %rdi
	callq	sb_0xhex
	jmp	.LBB0_430
.LBB0_332:
	cmpl	$176, -148(%rbp)
	jb	.LBB0_335
# %bb.333:
	cmpl	$183, -148(%rbp)
	ja	.LBB0_335
# %bb.334:
	leaq	-64(%rbp), %rdi
	movl	$1, %esi
	callq	rd_imm_sext
	movq	%rax, -1176(%rbp)
	leaq	-80(%rbp), %rdi
	leaq	.L.str.55(%rip), %rsi
	callq	sb_puts
	movl	-148(%rbp), %edi
	subl	$176, %edi
	movl	-116(%rbp), %edx
	xorl	%eax, %eax
	movl	$8, %ecx
	cmpl	$0, %edx
	cmovnel	%ecx, %eax
	orl	%eax, %edi
	movl	-120(%rbp), %edx
	movl	$1, %esi
	callq	reg_name
	movq	%rax, %rsi
	leaq	-80(%rbp), %rdi
	callq	sb_puts
	leaq	-80(%rbp), %rdi
	leaq	.L.str.10(%rip), %rsi
	callq	sb_puts
	movq	-1176(%rbp), %rsi
	andq	$255, %rsi
	leaq	-80(%rbp), %rdi
	callq	sb_0xhex
	jmp	.LBB0_430
.LBB0_335:
	cmpl	$184, -148(%rbp)
	jb	.LBB0_341
# %bb.336:
	cmpl	$191, -148(%rbp)
	ja	.LBB0_341
# %bb.337:
	movl	-148(%rbp), %eax
	subl	$184, %eax
	movl	-116(%rbp), %esi
	xorl	%ecx, %ecx
	movl	$8, %edx
	cmpl	$0, %esi
	cmovnel	%edx, %ecx
	orl	%ecx, %eax
	movl	%eax, -1180(%rbp)
	cmpl	$0, -104(%rbp)
	je	.LBB0_339
# %bb.338:
	leaq	-64(%rbp), %rdi
	callq	rd64
	movq	%rax, -1192(%rbp)
	leaq	-80(%rbp), %rdi
	leaq	.L.str.71(%rip), %rsi
	callq	sb_puts
	movslq	-1180(%rbp), %rcx
	leaq	R64(%rip), %rax
	movq	(%rax,%rcx,8), %rsi
	leaq	-80(%rbp), %rdi
	callq	sb_puts
	leaq	-80(%rbp), %rdi
	leaq	.L.str.10(%rip), %rsi
	callq	sb_puts
	movq	-1192(%rbp), %rsi
	leaq	-80(%rbp), %rdi
	callq	sb_0xhex
	jmp	.LBB0_340
.LBB0_339:
	movl	-128(%rbp), %ecx
	movl	$4, %esi
	movl	$2, %eax
	cmpl	$2, %ecx
	cmovel	%eax, %esi
	leaq	-64(%rbp), %rdi
	callq	rd_imm_sext
	movq	%rax, -1200(%rbp)
	leaq	-80(%rbp), %rdi
	leaq	.L.str.55(%rip), %rsi
	callq	sb_puts
	movl	-1180(%rbp), %edi
	movl	-128(%rbp), %esi
	movl	-120(%rbp), %edx
	callq	reg_name
	movq	%rax, %rsi
	leaq	-80(%rbp), %rdi
	callq	sb_puts
	leaq	-80(%rbp), %rdi
	leaq	.L.str.10(%rip), %rsi
	callq	sb_puts
	movq	-1200(%rbp), %rax
                                        # kill: def $eax killed $eax killed $rax
	movl	%eax, %eax
	movl	%eax, %esi
	leaq	-80(%rbp), %rdi
	callq	sb_0xhex
.LBB0_340:
	jmp	.LBB0_430
.LBB0_341:
	cmpl	$192, -148(%rbp)
	je	.LBB0_347
# %bb.342:
	cmpl	$193, -148(%rbp)
	je	.LBB0_347
# %bb.343:
	cmpl	$208, -148(%rbp)
	je	.LBB0_347
# %bb.344:
	cmpl	$209, -148(%rbp)
	je	.LBB0_347
# %bb.345:
	cmpl	$210, -148(%rbp)
	je	.LBB0_347
# %bb.346:
	cmpl	$211, -148(%rbp)
	jne	.LBB0_364
.LBB0_347:
	movl	-148(%rbp), %eax
	andl	$1, %eax
	cmpl	$0, %eax
	je	.LBB0_349
# %bb.348:
	movl	-128(%rbp), %eax
	movl	%eax, -1776(%rbp)               # 4-byte Spill
	jmp	.LBB0_350
.LBB0_349:
	movl	$1, %eax
	movl	%eax, -1776(%rbp)               # 4-byte Spill
	jmp	.LBB0_350
.LBB0_350:
	movl	-1776(%rbp), %eax               # 4-byte Reload
	movl	%eax, -1204(%rbp)
	movl	-108(%rbp), %esi
	movl	-112(%rbp), %edx
	movl	-116(%rbp), %ecx
	leaq	-64(%rbp), %rdi
	leaq	-1264(%rbp), %r8
	callq	parse_modrm
	movslq	-1216(%rbp), %rcx
	leaq	SHIFT(%rip), %rax
	movq	(%rax,%rcx,8), %rsi
	leaq	-80(%rbp), %rdi
	callq	sb_puts
	leaq	-80(%rbp), %rdi
	movl	$32, %esi
	callq	sb_putc
	movl	-1204(%rbp), %eax
	movl	%eax, -1784(%rbp)               # 4-byte Spill
	movl	-120(%rbp), %eax
	movl	%eax, -1780(%rbp)               # 4-byte Spill
	cmpl	$0, -1264(%rbp)
	je	.LBB0_352
# %bb.351:
	xorl	%eax, %eax
	movl	%eax, -1788(%rbp)               # 4-byte Spill
	jmp	.LBB0_353
.LBB0_352:
	movl	-1204(%rbp), %eax
	movl	%eax, -1788(%rbp)               # 4-byte Spill
.LBB0_353:
	movl	-1780(%rbp), %ecx               # 4-byte Reload
	movl	-1784(%rbp), %edx               # 4-byte Reload
	movl	-1788(%rbp), %r8d               # 4-byte Reload
	leaq	-80(%rbp), %rdi
	leaq	-1264(%rbp), %rsi
	xorl	%r9d, %r9d
	callq	render_rm
	cmpl	$192, -148(%rbp)
	je	.LBB0_355
# %bb.354:
	cmpl	$193, -148(%rbp)
	jne	.LBB0_356
.LBB0_355:
	leaq	-64(%rbp), %rdi
	movl	$1, %esi
	callq	rd_imm_sext
	movq	%rax, -1272(%rbp)
	leaq	-80(%rbp), %rdi
	leaq	.L.str.10(%rip), %rsi
	callq	sb_puts
	movq	-1272(%rbp), %rsi
	andq	$255, %rsi
	leaq	-80(%rbp), %rdi
	callq	sb_0xhex
	jmp	.LBB0_361
.LBB0_356:
	cmpl	$208, -148(%rbp)
	je	.LBB0_358
# %bb.357:
	cmpl	$209, -148(%rbp)
	jne	.LBB0_359
.LBB0_358:
	leaq	-80(%rbp), %rdi
	leaq	.L.str.72(%rip), %rsi
	callq	sb_puts
	jmp	.LBB0_360
.LBB0_359:
	leaq	-80(%rbp), %rdi
	leaq	.L.str.73(%rip), %rsi
	callq	sb_puts
.LBB0_360:
	jmp	.LBB0_361
.LBB0_361:
	cmpl	$0, -1252(%rbp)
	je	.LBB0_363
# %bb.362:
	movl	$1, -132(%rbp)
	movq	-1224(%rbp), %rax
	movq	%rax, -144(%rbp)
.LBB0_363:
	jmp	.LBB0_430
.LBB0_364:
	cmpl	$194, -148(%rbp)
	jne	.LBB0_366
# %bb.365:
	leaq	-64(%rbp), %rdi
	callq	rd16
	movl	%eax, -1276(%rbp)
	leaq	-80(%rbp), %rdi
	leaq	.L.str.74(%rip), %rsi
	callq	sb_puts
	movl	-1276(%rbp), %eax
	movl	%eax, %esi
	leaq	-80(%rbp), %rdi
	callq	sb_0xhex
	jmp	.LBB0_430
.LBB0_366:
	cmpl	$195, -148(%rbp)
	jne	.LBB0_368
# %bb.367:
	leaq	-80(%rbp), %rdi
	leaq	.L.str.75(%rip), %rsi
	callq	sb_puts
	jmp	.LBB0_430
.LBB0_368:
	cmpl	$201, -148(%rbp)
	jne	.LBB0_370
# %bb.369:
	leaq	-80(%rbp), %rdi
	leaq	.L.str.76(%rip), %rsi
	callq	sb_puts
	jmp	.LBB0_430
.LBB0_370:
	cmpl	$204, -148(%rbp)
	jne	.LBB0_372
# %bb.371:
	leaq	-80(%rbp), %rdi
	leaq	.L.str.77(%rip), %rsi
	callq	sb_puts
	jmp	.LBB0_430
.LBB0_372:
	cmpl	$205, -148(%rbp)
	jne	.LBB0_374
# %bb.373:
	leaq	-64(%rbp), %rdi
	callq	rd8
	movl	%eax, -1280(%rbp)
	leaq	-80(%rbp), %rdi
	leaq	.L.str.78(%rip), %rsi
	callq	sb_puts
	movl	-1280(%rbp), %eax
	movl	%eax, %esi
	leaq	-80(%rbp), %rdi
	callq	sb_0xhex
	jmp	.LBB0_430
.LBB0_374:
	cmpl	$198, -148(%rbp)
	je	.LBB0_376
# %bb.375:
	cmpl	$199, -148(%rbp)
	jne	.LBB0_388
.LBB0_376:
	cmpl	$198, -148(%rbp)
	jne	.LBB0_378
# %bb.377:
	movl	$1, %eax
	movl	%eax, -1792(%rbp)               # 4-byte Spill
	jmp	.LBB0_379
.LBB0_378:
	movl	-128(%rbp), %eax
	movl	%eax, -1792(%rbp)               # 4-byte Spill
.LBB0_379:
	movl	-1792(%rbp), %eax               # 4-byte Reload
	movl	%eax, -1284(%rbp)
	movl	-108(%rbp), %esi
	movl	-112(%rbp), %edx
	movl	-116(%rbp), %ecx
	leaq	-64(%rbp), %rdi
	leaq	-1344(%rbp), %r8
	callq	parse_modrm
	cmpl	$198, -148(%rbp)
	jne	.LBB0_381
# %bb.380:
	movl	$1, %eax
	movl	%eax, -1796(%rbp)               # 4-byte Spill
	jmp	.LBB0_382
.LBB0_381:
	movl	-128(%rbp), %edx
	movl	$4, %eax
	movl	$2, %ecx
	cmpl	$2, %edx
	cmovel	%ecx, %eax
	movl	%eax, -1796(%rbp)               # 4-byte Spill
.LBB0_382:
	movl	-1796(%rbp), %eax               # 4-byte Reload
	movl	%eax, -1348(%rbp)
	movl	-1348(%rbp), %esi
	leaq	-64(%rbp), %rdi
	callq	rd_imm_sext
	movq	%rax, -1360(%rbp)
	leaq	-80(%rbp), %rdi
	leaq	.L.str.55(%rip), %rsi
	callq	sb_puts
	movl	-1284(%rbp), %eax
	movl	%eax, -1804(%rbp)               # 4-byte Spill
	movl	-120(%rbp), %eax
	movl	%eax, -1800(%rbp)               # 4-byte Spill
	cmpl	$0, -1344(%rbp)
	je	.LBB0_384
# %bb.383:
	xorl	%eax, %eax
	movl	%eax, -1808(%rbp)               # 4-byte Spill
	jmp	.LBB0_385
.LBB0_384:
	movl	-1284(%rbp), %eax
	movl	%eax, -1808(%rbp)               # 4-byte Spill
.LBB0_385:
	movl	-1800(%rbp), %ecx               # 4-byte Reload
	movl	-1804(%rbp), %edx               # 4-byte Reload
	movl	-1808(%rbp), %r8d               # 4-byte Reload
	leaq	-80(%rbp), %rdi
	leaq	-1344(%rbp), %rsi
	xorl	%r9d, %r9d
	callq	render_rm
	leaq	-80(%rbp), %rdi
	leaq	.L.str.10(%rip), %rsi
	callq	sb_puts
	movq	-1360(%rbp), %rsi
	leaq	-80(%rbp), %rdi
	callq	sb_0xhex
	cmpl	$0, -1332(%rbp)
	je	.LBB0_387
# %bb.386:
	movl	$1, -132(%rbp)
	movq	-1304(%rbp), %rax
	movq	%rax, -144(%rbp)
.LBB0_387:
	jmp	.LBB0_430
.LBB0_388:
	cmpl	$232, -148(%rbp)
	je	.LBB0_390
# %bb.389:
	cmpl	$233, -148(%rbp)
	jne	.LBB0_391
.LBB0_390:
	leaq	-64(%rbp), %rdi
	movl	$4, %esi
	callq	rd_imm_sext
	movq	%rax, -1368(%rbp)
	movl	-148(%rbp), %ecx
	leaq	.L.str.80(%rip), %rsi
	leaq	.L.str.79(%rip), %rax
	cmpl	$232, %ecx
	cmoveq	%rax, %rsi
	leaq	-80(%rbp), %rdi
	callq	sb_puts
	movq	-40(%rbp), %rax
	movl	$1, 12(%rax)
	movq	-32(%rbp), %rcx
	movl	-52(%rbp), %eax
                                        # kill: def $rax killed $eax
	addq	%rax, %rcx
	addq	-1368(%rbp), %rcx
	movq	-40(%rbp), %rax
	movq	%rcx, 16(%rax)
	movq	-40(%rbp), %rax
	movq	16(%rax), %rsi
	leaq	-80(%rbp), %rdi
	callq	sb_0xhex
	jmp	.LBB0_430
.LBB0_391:
	cmpl	$235, -148(%rbp)
	jne	.LBB0_393
# %bb.392:
	leaq	-64(%rbp), %rdi
	movl	$1, %esi
	callq	rd_imm_sext
	movq	%rax, -1376(%rbp)
	leaq	-80(%rbp), %rdi
	leaq	.L.str.80(%rip), %rsi
	callq	sb_puts
	movq	-40(%rbp), %rax
	movl	$1, 12(%rax)
	movq	-32(%rbp), %rcx
	movl	-52(%rbp), %eax
                                        # kill: def $rax killed $eax
	addq	%rax, %rcx
	addq	-1376(%rbp), %rcx
	movq	-40(%rbp), %rax
	movq	%rcx, 16(%rax)
	movq	-40(%rbp), %rax
	movq	16(%rax), %rsi
	leaq	-80(%rbp), %rdi
	callq	sb_0xhex
	jmp	.LBB0_430
.LBB0_393:
	cmpl	$244, -148(%rbp)
	jne	.LBB0_395
# %bb.394:
	leaq	-80(%rbp), %rdi
	leaq	.L.str.81(%rip), %rsi
	callq	sb_puts
	jmp	.LBB0_430
.LBB0_395:
	cmpl	$245, -148(%rbp)
	jne	.LBB0_397
# %bb.396:
	leaq	-80(%rbp), %rdi
	leaq	.L.str.82(%rip), %rsi
	callq	sb_puts
	jmp	.LBB0_430
.LBB0_397:
	cmpl	$246, -148(%rbp)
	je	.LBB0_399
# %bb.398:
	cmpl	$247, -148(%rbp)
	jne	.LBB0_413
.LBB0_399:
	cmpl	$246, -148(%rbp)
	jne	.LBB0_401
# %bb.400:
	movl	$1, %eax
	movl	%eax, -1812(%rbp)               # 4-byte Spill
	jmp	.LBB0_402
.LBB0_401:
	movl	-128(%rbp), %eax
	movl	%eax, -1812(%rbp)               # 4-byte Spill
.LBB0_402:
	movl	-1812(%rbp), %eax               # 4-byte Reload
	movl	%eax, -1380(%rbp)
	movl	-108(%rbp), %esi
	movl	-112(%rbp), %edx
	movl	-116(%rbp), %ecx
	leaq	-64(%rbp), %rdi
	leaq	-1440(%rbp), %r8
	callq	parse_modrm
	movslq	-1392(%rbp), %rcx
	leaq	x86_decode.G3(%rip), %rax
	movq	(%rax,%rcx,8), %rsi
	leaq	-80(%rbp), %rdi
	callq	sb_puts
	leaq	-80(%rbp), %rdi
	movl	$32, %esi
	callq	sb_putc
	movl	-1380(%rbp), %eax
	movl	%eax, -1820(%rbp)               # 4-byte Spill
	movl	-120(%rbp), %eax
	movl	%eax, -1816(%rbp)               # 4-byte Spill
	cmpl	$0, -1440(%rbp)
	je	.LBB0_404
# %bb.403:
	xorl	%eax, %eax
	movl	%eax, -1824(%rbp)               # 4-byte Spill
	jmp	.LBB0_405
.LBB0_404:
	movl	-1380(%rbp), %eax
	movl	%eax, -1824(%rbp)               # 4-byte Spill
.LBB0_405:
	movl	-1816(%rbp), %ecx               # 4-byte Reload
	movl	-1820(%rbp), %edx               # 4-byte Reload
	movl	-1824(%rbp), %r8d               # 4-byte Reload
	leaq	-80(%rbp), %rdi
	leaq	-1440(%rbp), %rsi
	xorl	%r9d, %r9d
	callq	render_rm
	cmpl	$1, -1392(%rbp)
	jg	.LBB0_410
# %bb.406:
	cmpl	$246, -148(%rbp)
	jne	.LBB0_408
# %bb.407:
	movl	$1, %eax
	movl	%eax, -1828(%rbp)               # 4-byte Spill
	jmp	.LBB0_409
.LBB0_408:
	movl	-128(%rbp), %edx
	movl	$4, %eax
	movl	$2, %ecx
	cmpl	$2, %edx
	cmovel	%ecx, %eax
	movl	%eax, -1828(%rbp)               # 4-byte Spill
.LBB0_409:
	movl	-1828(%rbp), %eax               # 4-byte Reload
	movl	%eax, -1444(%rbp)
	movl	-1444(%rbp), %esi
	leaq	-64(%rbp), %rdi
	callq	rd_imm_sext
	movq	%rax, -1456(%rbp)
	leaq	-80(%rbp), %rdi
	leaq	.L.str.10(%rip), %rsi
	callq	sb_puts
	movq	-1456(%rbp), %rsi
	leaq	-80(%rbp), %rdi
	callq	sb_0xhex
.LBB0_410:
	cmpl	$0, -1428(%rbp)
	je	.LBB0_412
# %bb.411:
	movl	$1, -132(%rbp)
	movq	-1400(%rbp), %rax
	movq	%rax, -144(%rbp)
.LBB0_412:
	jmp	.LBB0_430
.LBB0_413:
	cmpl	$254, -148(%rbp)
	jne	.LBB0_418
# %bb.414:
	movl	-108(%rbp), %esi
	movl	-112(%rbp), %edx
	movl	-116(%rbp), %ecx
	leaq	-64(%rbp), %rdi
	leaq	-1512(%rbp), %r8
	callq	parse_modrm
	cmpl	$0, -1464(%rbp)
	jne	.LBB0_416
# %bb.415:
	leaq	.L.str.90(%rip), %rax
	movq	%rax, -1840(%rbp)               # 8-byte Spill
	jmp	.LBB0_417
.LBB0_416:
	movl	-1464(%rbp), %edx
	leaq	.L.str.92(%rip), %rax
	leaq	.L.str.91(%rip), %rcx
	cmpl	$1, %edx
	cmoveq	%rcx, %rax
	movq	%rax, -1840(%rbp)               # 8-byte Spill
.LBB0_417:
	movq	-1840(%rbp), %rsi               # 8-byte Reload
	leaq	-80(%rbp), %rdi
	callq	sb_puts
	movl	-120(%rbp), %ecx
	movl	-1512(%rbp), %edx
	movl	$1, %r8d
	xorl	%eax, %eax
	cmpl	$0, %edx
	cmovnel	%eax, %r8d
	leaq	-80(%rbp), %rdi
	leaq	-1512(%rbp), %rsi
	movl	$1, %edx
	xorl	%r9d, %r9d
	callq	render_rm
	jmp	.LBB0_430
.LBB0_418:
	cmpl	$255, -148(%rbp)
	jne	.LBB0_429
# %bb.419:
	movl	-108(%rbp), %esi
	movl	-112(%rbp), %edx
	movl	-116(%rbp), %ecx
	leaq	-64(%rbp), %rdi
	leaq	-1568(%rbp), %r8
	callq	parse_modrm
	movl	-128(%rbp), %eax
	movl	%eax, -1572(%rbp)
	cmpl	$2, -1520(%rbp)
	je	.LBB0_422
# %bb.420:
	cmpl	$4, -1520(%rbp)
	je	.LBB0_422
# %bb.421:
	cmpl	$6, -1520(%rbp)
	jne	.LBB0_423
.LBB0_422:
	movl	$8, -1572(%rbp)
.LBB0_423:
	movslq	-1520(%rbp), %rcx
	leaq	x86_decode.G5(%rip), %rax
	movq	(%rax,%rcx,8), %rsi
	leaq	-80(%rbp), %rdi
	callq	sb_puts
	leaq	-80(%rbp), %rdi
	movl	$32, %esi
	callq	sb_putc
	movl	-1572(%rbp), %eax
	movl	%eax, -1848(%rbp)               # 4-byte Spill
	movl	-120(%rbp), %eax
	movl	%eax, -1844(%rbp)               # 4-byte Spill
	cmpl	$0, -1568(%rbp)
	je	.LBB0_425
# %bb.424:
	xorl	%eax, %eax
	movl	%eax, -1852(%rbp)               # 4-byte Spill
	jmp	.LBB0_426
.LBB0_425:
	movl	-1572(%rbp), %eax
	movl	%eax, -1852(%rbp)               # 4-byte Spill
.LBB0_426:
	movl	-1844(%rbp), %ecx               # 4-byte Reload
	movl	-1848(%rbp), %edx               # 4-byte Reload
	movl	-1852(%rbp), %r8d               # 4-byte Reload
	leaq	-80(%rbp), %rdi
	leaq	-1568(%rbp), %rsi
	xorl	%r9d, %r9d
	callq	render_rm
	cmpl	$0, -1556(%rbp)
	je	.LBB0_428
# %bb.427:
	movl	$1, -132(%rbp)
	movq	-1528(%rbp), %rax
	movq	%rax, -144(%rbp)
.LBB0_428:
	jmp	.LBB0_430
.LBB0_429:
	leaq	-80(%rbp), %rdi
	leaq	.L.str.48(%rip), %rsi
	callq	sb_puts
.LBB0_430:
	cmpl	$0, -48(%rbp)
	je	.LBB0_435
# %bb.431:
	movq	-40(%rbp), %rax
	movb	$0, 24(%rax)
	movq	-40(%rbp), %rax
	addq	$24, %rax
	movq	%rax, -1592(%rbp)
	movl	$64, -1584(%rbp)
	movl	$0, -1580(%rbp)
	leaq	-1592(%rbp), %rdi
	leaq	.L.str.48(%rip), %rsi
	callq	sb_puts
	movq	-40(%rbp), %rax
	movl	$0, 12(%rax)
	cmpl	$0, -20(%rbp)
	je	.LBB0_433
# %bb.432:
	movl	-20(%rbp), %eax
	movl	%eax, -1856(%rbp)               # 4-byte Spill
	jmp	.LBB0_434
.LBB0_433:
	movl	$1, %eax
	movl	%eax, -1856(%rbp)               # 4-byte Spill
	jmp	.LBB0_434
.LBB0_434:
	movl	-1856(%rbp), %ecx               # 4-byte Reload
	movq	-40(%rbp), %rax
	movl	%ecx, 8(%rax)
	movq	-40(%rbp), %rax
	movl	8(%rax), %eax
	movl	%eax, -4(%rbp)
	jmp	.LBB0_441
.LBB0_435:
	cmpl	$0, -132(%rbp)
	je	.LBB0_437
# %bb.436:
	movq	-32(%rbp), %rax
	movl	-52(%rbp), %ecx
                                        # kill: def $rcx killed $ecx
	addq	%rcx, %rax
	addq	-144(%rbp), %rax
	movq	%rax, -1600(%rbp)
	leaq	-80(%rbp), %rdi
	leaq	.L.str.100(%rip), %rsi
	callq	sb_puts
	movq	-1600(%rbp), %rsi
	leaq	-80(%rbp), %rdi
	callq	sb_0xhex
.LBB0_437:
	cmpl	$0, -52(%rbp)
	je	.LBB0_439
# %bb.438:
	movl	-52(%rbp), %eax
	movl	%eax, -1860(%rbp)               # 4-byte Spill
	jmp	.LBB0_440
.LBB0_439:
	movl	$1, %eax
	movl	%eax, -1860(%rbp)               # 4-byte Spill
	jmp	.LBB0_440
.LBB0_440:
	movl	-1860(%rbp), %ecx               # 4-byte Reload
	movq	-40(%rbp), %rax
	movl	%ecx, 8(%rax)
	movq	-40(%rbp), %rax
	movl	8(%rax), %eax
	movl	%eax, -4(%rbp)
.LBB0_441:
	movl	-4(%rbp), %eax
	addq	$1872, %rsp                     # imm = 0x750
	popq	%rbp
	retq
.Lfunc_end0:
	.size	x86_decode, .Lfunc_end0-x86_decode
                                        # -- End function
	.p2align	4                               # -- Begin function sb_puts
	.type	sb_puts,@function
sb_puts:                                # @sb_puts
# %bb.0:
	pushq	%rbp
	movq	%rsp, %rbp
	subq	$16, %rsp
	movq	%rdi, -8(%rbp)
	movq	%rsi, -16(%rbp)
.LBB1_1:                                # =>This Inner Loop Header: Depth=1
	movq	-16(%rbp), %rax
	cmpb	$0, (%rax)
	je	.LBB1_3
# %bb.2:                                #   in Loop: Header=BB1_1 Depth=1
	movq	-8(%rbp), %rdi
	movq	-16(%rbp), %rax
	movq	%rax, %rcx
	addq	$1, %rcx
	movq	%rcx, -16(%rbp)
	movsbl	(%rax), %esi
	callq	sb_putc
	jmp	.LBB1_1
.LBB1_3:
	addq	$16, %rsp
	popq	%rbp
	retq
.Lfunc_end1:
	.size	sb_puts, .Lfunc_end1-sb_puts
                                        # -- End function
	.p2align	4                               # -- Begin function rd8
	.type	rd8,@function
rd8:                                    # @rd8
# %bb.0:
	pushq	%rbp
	movq	%rsp, %rbp
	movq	%rdi, -16(%rbp)
	movq	-16(%rbp), %rax
	movl	12(%rax), %eax
	movq	-16(%rbp), %rcx
	cmpl	8(%rcx), %eax
	jb	.LBB2_2
# %bb.1:
	movq	-16(%rbp), %rax
	movl	$1, 16(%rax)
	movl	$0, -4(%rbp)
	jmp	.LBB2_3
.LBB2_2:
	movq	-16(%rbp), %rax
	movq	(%rax), %rax
	movq	-16(%rbp), %rdx
	movl	12(%rdx), %ecx
	movl	%ecx, %esi
	addl	$1, %esi
	movl	%esi, 12(%rdx)
	movl	%ecx, %ecx
                                        # kill: def $rcx killed $ecx
	movzbl	(%rax,%rcx), %eax
	movl	%eax, -4(%rbp)
.LBB2_3:
	movl	-4(%rbp), %eax
	popq	%rbp
	retq
.Lfunc_end2:
	.size	rd8, .Lfunc_end2-rd8
                                        # -- End function
	.p2align	4                               # -- Begin function parse_modrm
	.type	parse_modrm,@function
parse_modrm:                            # @parse_modrm
# %bb.0:
	pushq	%rbp
	movq	%rsp, %rbp
	subq	$80, %rsp
	movq	%rdi, -8(%rbp)
	movl	%esi, -12(%rbp)
	movl	%edx, -16(%rbp)
	movl	%ecx, -20(%rbp)
	movq	%r8, -32(%rbp)
	movq	-8(%rbp), %rdi
	callq	rd8
	movl	%eax, -36(%rbp)
	movl	-36(%rbp), %eax
	shrl	$6, %eax
	andl	$3, %eax
	movl	%eax, -40(%rbp)
	movl	-36(%rbp), %eax
	shrl	$3, %eax
	andl	$7, %eax
	movl	%eax, -44(%rbp)
	movl	-36(%rbp), %eax
	andl	$7, %eax
	movl	%eax, -48(%rbp)
	movq	-32(%rbp), %rax
	movl	$0, (%rax)
	movq	-32(%rbp), %rax
	movl	$0, 12(%rax)
	movq	-32(%rbp), %rax
	movl	$0, 16(%rax)
	movq	-32(%rbp), %rax
	movl	$0, 24(%rax)
	movq	-32(%rbp), %rax
	movl	$1, 32(%rax)
	movq	-32(%rbp), %rax
	movq	$0, 40(%rax)
	movl	-44(%rbp), %ecx
	movq	-32(%rbp), %rax
	movl	%ecx, 48(%rax)
	movl	-44(%rbp), %ecx
	movl	-12(%rbp), %esi
	xorl	%eax, %eax
	movl	$8, %edx
	cmpl	$0, %esi
	cmovnel	%edx, %eax
	orl	%eax, %ecx
	movq	-32(%rbp), %rax
	movl	%ecx, 4(%rax)
	cmpl	$3, -40(%rbp)
	jne	.LBB3_2
# %bb.1:
	movq	-32(%rbp), %rax
	movl	$1, (%rax)
	movl	-48(%rbp), %ecx
	movl	-20(%rbp), %esi
	xorl	%eax, %eax
	movl	$8, %edx
	cmpl	$0, %esi
	cmovnel	%edx, %eax
	orl	%eax, %ecx
	movq	-32(%rbp), %rax
	movl	%ecx, 8(%rax)
	jmp	.LBB3_23
.LBB3_2:
	movl	$0, -52(%rbp)
	cmpl	$4, -48(%rbp)
	jne	.LBB3_11
# %bb.3:
	movq	-8(%rbp), %rdi
	callq	rd8
	movl	%eax, -56(%rbp)
	movl	-56(%rbp), %eax
	shrl	$6, %eax
	andl	$3, %eax
	movl	%eax, -60(%rbp)
	movl	-56(%rbp), %eax
	shrl	$3, %eax
	andl	$7, %eax
	movl	%eax, -64(%rbp)
	movl	-56(%rbp), %eax
	andl	$7, %eax
	movl	%eax, -68(%rbp)
	cmpl	$4, -64(%rbp)
	jne	.LBB3_5
# %bb.4:
	cmpl	$0, -16(%rbp)
	je	.LBB3_6
.LBB3_5:
	movq	-32(%rbp), %rax
	movl	$1, 24(%rax)
	movl	-64(%rbp), %ecx
	movl	-16(%rbp), %esi
	xorl	%eax, %eax
	movl	$8, %edx
	cmpl	$0, %esi
	cmovnel	%edx, %eax
	orl	%eax, %ecx
	movq	-32(%rbp), %rax
	movl	%ecx, 28(%rax)
	movl	-60(%rbp), %ecx
	movl	$1, %eax
                                        # kill: def $cl killed $ecx
	shll	%cl, %eax
	movl	%eax, %ecx
	movq	-32(%rbp), %rax
	movl	%ecx, 32(%rax)
.LBB3_6:
	cmpl	$5, -68(%rbp)
	jne	.LBB3_9
# %bb.7:
	cmpl	$0, -40(%rbp)
	jne	.LBB3_9
# %bb.8:
	movl	$4, -52(%rbp)
	jmp	.LBB3_10
.LBB3_9:
	movq	-32(%rbp), %rax
	movl	$1, 16(%rax)
	movl	-68(%rbp), %ecx
	movl	-20(%rbp), %esi
	xorl	%eax, %eax
	movl	$8, %edx
	cmpl	$0, %esi
	cmovnel	%edx, %eax
	orl	%eax, %ecx
	movq	-32(%rbp), %rax
	movl	%ecx, 20(%rax)
.LBB3_10:
	jmp	.LBB3_16
.LBB3_11:
	cmpl	$5, -48(%rbp)
	jne	.LBB3_14
# %bb.12:
	cmpl	$0, -40(%rbp)
	jne	.LBB3_14
# %bb.13:
	movq	-32(%rbp), %rax
	movl	$1, 12(%rax)
	movl	$4, -52(%rbp)
	jmp	.LBB3_15
.LBB3_14:
	movq	-32(%rbp), %rax
	movl	$1, 16(%rax)
	movl	-48(%rbp), %ecx
	movl	-20(%rbp), %esi
	xorl	%eax, %eax
	movl	$8, %edx
	cmpl	$0, %esi
	cmovnel	%edx, %eax
	orl	%eax, %ecx
	movq	-32(%rbp), %rax
	movl	%ecx, 20(%rax)
.LBB3_15:
	jmp	.LBB3_16
.LBB3_16:
	cmpl	$1, -40(%rbp)
	jne	.LBB3_18
# %bb.17:
	movl	$1, -52(%rbp)
	jmp	.LBB3_21
.LBB3_18:
	cmpl	$2, -40(%rbp)
	jne	.LBB3_20
# %bb.19:
	movl	$4, -52(%rbp)
.LBB3_20:
	jmp	.LBB3_21
.LBB3_21:
	cmpl	$0, -52(%rbp)
	je	.LBB3_23
# %bb.22:
	movq	-8(%rbp), %rdi
	movl	-52(%rbp), %esi
	callq	rd_imm_sext
	movq	%rax, %rcx
	movq	-32(%rbp), %rax
	movq	%rcx, 40(%rax)
.LBB3_23:
	addq	$80, %rsp
	popq	%rbp
	retq
.Lfunc_end3:
	.size	parse_modrm, .Lfunc_end3-parse_modrm
                                        # -- End function
	.p2align	4                               # -- Begin function rd_imm_sext
	.type	rd_imm_sext,@function
rd_imm_sext:                            # @rd_imm_sext
# %bb.0:
	pushq	%rbp
	movq	%rsp, %rbp
	subq	$32, %rsp
	movq	%rdi, -16(%rbp)
	movl	%esi, -20(%rbp)
	movl	-20(%rbp), %eax
	movl	%eax, -24(%rbp)                 # 4-byte Spill
	subl	$1, %eax
	je	.LBB4_1
	jmp	.LBB4_6
.LBB4_6:
	movl	-24(%rbp), %eax                 # 4-byte Reload
	subl	$2, %eax
	je	.LBB4_2
	jmp	.LBB4_7
.LBB4_7:
	movl	-24(%rbp), %eax                 # 4-byte Reload
	subl	$4, %eax
	je	.LBB4_3
	jmp	.LBB4_4
.LBB4_1:
	movq	-16(%rbp), %rdi
	callq	rd8
                                        # kill: def $al killed $al killed $eax
	movsbq	%al, %rax
	movq	%rax, -8(%rbp)
	jmp	.LBB4_5
.LBB4_2:
	movq	-16(%rbp), %rdi
	callq	rd16
                                        # kill: def $ax killed $ax killed $eax
	movswq	%ax, %rax
	movq	%rax, -8(%rbp)
	jmp	.LBB4_5
.LBB4_3:
	movq	-16(%rbp), %rdi
	callq	rd32
	cltq
	movq	%rax, -8(%rbp)
	jmp	.LBB4_5
.LBB4_4:
	movq	-16(%rbp), %rdi
	callq	rd64
	movq	%rax, -8(%rbp)
.LBB4_5:
	movq	-8(%rbp), %rax
	addq	$32, %rsp
	popq	%rbp
	retq
.Lfunc_end4:
	.size	rd_imm_sext, .Lfunc_end4-rd_imm_sext
                                        # -- End function
	.p2align	4                               # -- Begin function sb_putc
	.type	sb_putc,@function
sb_putc:                                # @sb_putc
# %bb.0:
	pushq	%rbp
	movq	%rsp, %rbp
	movb	%sil, %al
	movq	%rdi, -8(%rbp)
	movb	%al, -9(%rbp)
	movq	-8(%rbp), %rax
	movl	12(%rax), %eax
	addl	$1, %eax
	movq	-8(%rbp), %rcx
	cmpl	8(%rcx), %eax
	jae	.LBB5_2
# %bb.1:
	movb	-9(%rbp), %dl
	movq	-8(%rbp), %rax
	movq	(%rax), %rax
	movq	-8(%rbp), %rsi
	movl	12(%rsi), %ecx
	movl	%ecx, %edi
	addl	$1, %edi
	movl	%edi, 12(%rsi)
	movl	%ecx, %ecx
                                        # kill: def $rcx killed $ecx
	movb	%dl, (%rax,%rcx)
	movq	-8(%rbp), %rax
	movq	(%rax), %rax
	movq	-8(%rbp), %rcx
	movl	12(%rcx), %ecx
                                        # kill: def $rcx killed $ecx
	movb	$0, (%rax,%rcx)
.LBB5_2:
	popq	%rbp
	retq
.Lfunc_end5:
	.size	sb_putc, .Lfunc_end5-sb_putc
                                        # -- End function
	.p2align	4                               # -- Begin function sb_0xhex
	.type	sb_0xhex,@function
sb_0xhex:                               # @sb_0xhex
# %bb.0:
	pushq	%rbp
	movq	%rsp, %rbp
	subq	$16, %rsp
	movq	%rdi, -8(%rbp)
	movq	%rsi, -16(%rbp)
	movq	-8(%rbp), %rdi
	leaq	.L.str.117(%rip), %rsi
	callq	sb_puts
	movq	-8(%rbp), %rdi
	movq	-16(%rbp), %rsi
	callq	sb_hex
	addq	$16, %rsp
	popq	%rbp
	retq
.Lfunc_end6:
	.size	sb_0xhex, .Lfunc_end6-sb_0xhex
                                        # -- End function
	.p2align	4                               # -- Begin function render_rm
	.type	render_rm,@function
render_rm:                              # @render_rm
# %bb.0:
	pushq	%rbp
	movq	%rsp, %rbp
	subq	$48, %rsp
	movq	%rdi, -8(%rbp)
	movq	%rsi, -16(%rbp)
	movl	%edx, -20(%rbp)
	movl	%ecx, -24(%rbp)
	movl	%r8d, -28(%rbp)
	movl	%r9d, -32(%rbp)
	movq	-16(%rbp), %rax
	cmpl	$0, (%rax)
	je	.LBB7_5
# %bb.1:
	cmpl	$0, -32(%rbp)
	je	.LBB7_3
# %bb.2:
	movq	-8(%rbp), %rdi
	movq	-16(%rbp), %rax
	movl	8(%rax), %eax
	andl	$15, %eax
	movslq	%eax, %rcx
	leaq	XMM(%rip), %rax
	movq	(%rax,%rcx,8), %rsi
	callq	sb_puts
	jmp	.LBB7_4
.LBB7_3:
	movq	-8(%rbp), %rax
	movq	%rax, -40(%rbp)                 # 8-byte Spill
	movq	-16(%rbp), %rax
	movl	8(%rax), %edi
	movl	-20(%rbp), %esi
	movl	-24(%rbp), %edx
	callq	reg_name
	movq	-40(%rbp), %rdi                 # 8-byte Reload
	movq	%rax, %rsi
	callq	sb_puts
.LBB7_4:
	jmp	.LBB7_8
.LBB7_5:
	cmpl	$0, -28(%rbp)
	je	.LBB7_7
# %bb.6:
	movq	-8(%rbp), %rax
	movq	%rax, -48(%rbp)                 # 8-byte Spill
	movl	-28(%rbp), %edi
	callq	ptr_kw
	movq	-48(%rbp), %rdi                 # 8-byte Reload
	movq	%rax, %rsi
	callq	sb_puts
.LBB7_7:
	movq	-8(%rbp), %rdi
	movq	-16(%rbp), %rsi
	callq	render_mem
.LBB7_8:
	addq	$48, %rsp
	popq	%rbp
	retq
.Lfunc_end7:
	.size	render_rm, .Lfunc_end7-render_rm
                                        # -- End function
	.p2align	4                               # -- Begin function reg_name
	.type	reg_name,@function
reg_name:                               # @reg_name
# %bb.0:
	pushq	%rbp
	movq	%rsp, %rbp
	movl	%edi, -12(%rbp)
	movl	%esi, -16(%rbp)
	movl	%edx, -20(%rbp)
	movl	-16(%rbp), %eax
	movl	%eax, -24(%rbp)                 # 4-byte Spill
	subl	$2, %eax
	je	.LBB8_3
	jmp	.LBB8_8
.LBB8_8:
	movl	-24(%rbp), %eax                 # 4-byte Reload
	subl	$4, %eax
	je	.LBB8_2
	jmp	.LBB8_9
.LBB8_9:
	movl	-24(%rbp), %eax                 # 4-byte Reload
	subl	$8, %eax
	jne	.LBB8_4
	jmp	.LBB8_1
.LBB8_1:
	movl	-12(%rbp), %eax
	andl	$15, %eax
	movslq	%eax, %rcx
	leaq	R64(%rip), %rax
	movq	(%rax,%rcx,8), %rax
	movq	%rax, -8(%rbp)
	jmp	.LBB8_7
.LBB8_2:
	movl	-12(%rbp), %eax
	andl	$15, %eax
	movslq	%eax, %rcx
	leaq	R32(%rip), %rax
	movq	(%rax,%rcx,8), %rax
	movq	%rax, -8(%rbp)
	jmp	.LBB8_7
.LBB8_3:
	movl	-12(%rbp), %eax
	andl	$15, %eax
	movslq	%eax, %rcx
	leaq	R16(%rip), %rax
	movq	(%rax,%rcx,8), %rax
	movq	%rax, -8(%rbp)
	jmp	.LBB8_7
.LBB8_4:
	cmpl	$0, -20(%rbp)
	je	.LBB8_6
# %bb.5:
	movl	-12(%rbp), %eax
	andl	$15, %eax
	movslq	%eax, %rcx
	leaq	R8L(%rip), %rax
	movq	(%rax,%rcx,8), %rax
	movq	%rax, -8(%rbp)
	jmp	.LBB8_7
.LBB8_6:
	movl	-12(%rbp), %eax
	andl	$7, %eax
	movslq	%eax, %rcx
	leaq	R8H(%rip), %rax
	movq	(%rax,%rcx,8), %rax
	movq	%rax, -8(%rbp)
.LBB8_7:
	movq	-8(%rbp), %rax
	popq	%rbp
	retq
.Lfunc_end8:
	.size	reg_name, .Lfunc_end8-reg_name
                                        # -- End function
	.p2align	4                               # -- Begin function render_mem
	.type	render_mem,@function
render_mem:                             # @render_mem
# %bb.0:
	pushq	%rbp
	movq	%rsp, %rbp
	subq	$32, %rsp
	movq	%rdi, -8(%rbp)
	movq	%rsi, -16(%rbp)
	movq	-8(%rbp), %rdi
	movl	$91, %esi
	callq	sb_putc
	movl	$1, -20(%rbp)
	movq	-16(%rbp), %rax
	cmpl	$0, 12(%rax)
	je	.LBB9_2
# %bb.1:
	movq	-8(%rbp), %rdi
	leaq	.L.str.214(%rip), %rsi
	callq	sb_puts
	movl	$0, -20(%rbp)
.LBB9_2:
	movq	-16(%rbp), %rax
	cmpl	$0, 16(%rax)
	je	.LBB9_4
# %bb.3:
	movq	-8(%rbp), %rdi
	movq	-16(%rbp), %rax
	movl	20(%rax), %eax
	andl	$15, %eax
	movslq	%eax, %rcx
	leaq	R64(%rip), %rax
	movq	(%rax,%rcx,8), %rsi
	callq	sb_puts
	movl	$0, -20(%rbp)
.LBB9_4:
	movq	-16(%rbp), %rax
	cmpl	$0, 24(%rax)
	je	.LBB9_8
# %bb.5:
	cmpl	$0, -20(%rbp)
	jne	.LBB9_7
# %bb.6:
	movq	-8(%rbp), %rdi
	movl	$43, %esi
	callq	sb_putc
.LBB9_7:
	movq	-8(%rbp), %rdi
	movq	-16(%rbp), %rax
	movl	28(%rax), %eax
	andl	$15, %eax
	movslq	%eax, %rcx
	leaq	R64(%rip), %rax
	movq	(%rax,%rcx,8), %rsi
	callq	sb_puts
	movq	-8(%rbp), %rdi
	movl	$42, %esi
	callq	sb_putc
	movq	-8(%rbp), %rdi
	movq	-16(%rbp), %rax
	movl	32(%rax), %eax
	addl	$48, %eax
                                        # kill: def $al killed $al killed $eax
	movsbl	%al, %esi
	callq	sb_putc
	movl	$0, -20(%rbp)
.LBB9_8:
	movq	-16(%rbp), %rax
	cmpq	$0, 40(%rax)
	jne	.LBB9_10
# %bb.9:
	cmpl	$0, -20(%rbp)
	je	.LBB9_17
.LBB9_10:
	cmpl	$0, -20(%rbp)
	je	.LBB9_12
# %bb.11:
	movq	-8(%rbp), %rdi
	movq	-16(%rbp), %rax
	movq	40(%rax), %rsi
	callq	sb_0xhex
	jmp	.LBB9_16
.LBB9_12:
	movq	-16(%rbp), %rax
	cmpq	$0, 40(%rax)
	jge	.LBB9_14
# %bb.13:
	movq	-8(%rbp), %rdi
	leaq	.L.str.215(%rip), %rsi
	callq	sb_puts
	movq	-8(%rbp), %rdi
	movq	-16(%rbp), %rax
	xorl	%ecx, %ecx
	movl	%ecx, %esi
	subq	40(%rax), %rsi
	callq	sb_hex
	jmp	.LBB9_15
.LBB9_14:
	movq	-8(%rbp), %rdi
	leaq	.L.str.216(%rip), %rsi
	callq	sb_puts
	movq	-8(%rbp), %rdi
	movq	-16(%rbp), %rax
	movq	40(%rax), %rsi
	callq	sb_hex
.LBB9_15:
	jmp	.LBB9_16
.LBB9_16:
	jmp	.LBB9_17
.LBB9_17:
	movq	-8(%rbp), %rdi
	movl	$93, %esi
	callq	sb_putc
	addq	$32, %rsp
	popq	%rbp
	retq
.Lfunc_end9:
	.size	render_mem, .Lfunc_end9-render_mem
                                        # -- End function
	.p2align	4                               # -- Begin function rd64
	.type	rd64,@function
rd64:                                   # @rd64
# %bb.0:
	pushq	%rbp
	movq	%rsp, %rbp
	subq	$32, %rsp
	movq	%rdi, -8(%rbp)
	movq	-8(%rbp), %rdi
	callq	rd32
	movl	%eax, %eax
                                        # kill: def $rax killed $eax
	movq	%rax, -16(%rbp)
	movq	-8(%rbp), %rdi
	callq	rd32
	movl	%eax, %eax
                                        # kill: def $rax killed $eax
	movq	%rax, -24(%rbp)
	movq	-16(%rbp), %rax
	movq	-24(%rbp), %rcx
	shlq	$32, %rcx
	orq	%rcx, %rax
	addq	$32, %rsp
	popq	%rbp
	retq
.Lfunc_end10:
	.size	rd64, .Lfunc_end10-rd64
                                        # -- End function
	.p2align	4                               # -- Begin function rd16
	.type	rd16,@function
rd16:                                   # @rd16
# %bb.0:
	pushq	%rbp
	movq	%rsp, %rbp
	subq	$16, %rsp
	movq	%rdi, -8(%rbp)
	movq	-8(%rbp), %rdi
	callq	rd8
	movl	%eax, -12(%rbp)
	movl	-12(%rbp), %eax
	movl	%eax, -16(%rbp)                 # 4-byte Spill
	movq	-8(%rbp), %rdi
	callq	rd8
	movl	%eax, %ecx
	movl	-16(%rbp), %eax                 # 4-byte Reload
	shll	$8, %ecx
	orl	%ecx, %eax
	addq	$16, %rsp
	popq	%rbp
	retq
.Lfunc_end11:
	.size	rd16, .Lfunc_end11-rd16
                                        # -- End function
	.p2align	4                               # -- Begin function rd32
	.type	rd32,@function
rd32:                                   # @rd32
# %bb.0:
	pushq	%rbp
	movq	%rsp, %rbp
	subq	$32, %rsp
	movq	%rdi, -8(%rbp)
	movq	-8(%rbp), %rdi
	callq	rd8
	movl	%eax, -12(%rbp)
	movq	-8(%rbp), %rdi
	callq	rd8
	movl	%eax, -16(%rbp)
	movq	-8(%rbp), %rdi
	callq	rd8
	movl	%eax, -20(%rbp)
	movq	-8(%rbp), %rdi
	callq	rd8
	movl	%eax, -24(%rbp)
	movl	-12(%rbp), %eax
	movl	-16(%rbp), %ecx
	shll	$8, %ecx
	orl	%ecx, %eax
	movl	-20(%rbp), %ecx
	shll	$16, %ecx
	orl	%ecx, %eax
	movl	-24(%rbp), %ecx
	shll	$24, %ecx
	orl	%ecx, %eax
	addq	$32, %rsp
	popq	%rbp
	retq
.Lfunc_end12:
	.size	rd32, .Lfunc_end12-rd32
                                        # -- End function
	.p2align	4                               # -- Begin function sb_hex
	.type	sb_hex,@function
sb_hex:                                 # @sb_hex
# %bb.0:
	pushq	%rbp
	movq	%rsp, %rbp
	subq	$48, %rsp
	movq	%rdi, -8(%rbp)
	movq	%rsi, -16(%rbp)
	movl	$0, -36(%rbp)
	cmpq	$0, -16(%rbp)
	jne	.LBB13_2
# %bb.1:
	movq	-8(%rbp), %rdi
	movl	$48, %esi
	callq	sb_putc
	jmp	.LBB13_11
.LBB13_2:
	jmp	.LBB13_3
.LBB13_3:                               # =>This Inner Loop Header: Depth=1
	cmpq	$0, -16(%rbp)
	je	.LBB13_8
# %bb.4:                                #   in Loop: Header=BB13_3 Depth=1
	movq	-16(%rbp), %rax
	andq	$15, %rax
                                        # kill: def $eax killed $eax killed $rax
	movl	%eax, -40(%rbp)
	cmpl	$10, -40(%rbp)
	jae	.LBB13_6
# %bb.5:                                #   in Loop: Header=BB13_3 Depth=1
	movl	-40(%rbp), %eax
	addl	$48, %eax
	movl	%eax, -44(%rbp)                 # 4-byte Spill
	jmp	.LBB13_7
.LBB13_6:                               #   in Loop: Header=BB13_3 Depth=1
	movl	-40(%rbp), %eax
	subl	$10, %eax
	addl	$97, %eax
	movl	%eax, -44(%rbp)                 # 4-byte Spill
.LBB13_7:                               #   in Loop: Header=BB13_3 Depth=1
	movl	-44(%rbp), %eax                 # 4-byte Reload
	movb	%al, %cl
	movl	-36(%rbp), %eax
	movl	%eax, %edx
	addl	$1, %edx
	movl	%edx, -36(%rbp)
	cltq
	movb	%cl, -32(%rbp,%rax)
	movq	-16(%rbp), %rax
	shrq	$4, %rax
	movq	%rax, -16(%rbp)
	jmp	.LBB13_3
.LBB13_8:
	jmp	.LBB13_9
.LBB13_9:                               # =>This Inner Loop Header: Depth=1
	cmpl	$0, -36(%rbp)
	je	.LBB13_11
# %bb.10:                               #   in Loop: Header=BB13_9 Depth=1
	movq	-8(%rbp), %rdi
	movl	-36(%rbp), %eax
	addl	$-1, %eax
	movl	%eax, -36(%rbp)
	cltq
	movsbl	-32(%rbp,%rax), %esi
	callq	sb_putc
	jmp	.LBB13_9
.LBB13_11:
	addq	$48, %rsp
	popq	%rbp
	retq
.Lfunc_end13:
	.size	sb_hex, .Lfunc_end13-sb_hex
                                        # -- End function
	.p2align	4                               # -- Begin function ptr_kw
	.type	ptr_kw,@function
ptr_kw:                                 # @ptr_kw
# %bb.0:
	pushq	%rbp
	movq	%rsp, %rbp
	movl	%edi, -12(%rbp)
	movl	-12(%rbp), %eax
	movl	%eax, -16(%rbp)                 # 4-byte Spill
	subl	$1, %eax
	je	.LBB14_1
	jmp	.LBB14_6
.LBB14_6:
	movl	-16(%rbp), %eax                 # 4-byte Reload
	subl	$2, %eax
	je	.LBB14_2
	jmp	.LBB14_7
.LBB14_7:
	movl	-16(%rbp), %eax                 # 4-byte Reload
	subl	$4, %eax
	je	.LBB14_3
	jmp	.LBB14_4
.LBB14_1:
	leaq	.L.str.118(%rip), %rax
	movq	%rax, -8(%rbp)
	jmp	.LBB14_5
.LBB14_2:
	leaq	.L.str.119(%rip), %rax
	movq	%rax, -8(%rbp)
	jmp	.LBB14_5
.LBB14_3:
	leaq	.L.str.120(%rip), %rax
	movq	%rax, -8(%rbp)
	jmp	.LBB14_5
.LBB14_4:
	leaq	.L.str.121(%rip), %rax
	movq	%rax, -8(%rbp)
.LBB14_5:
	movq	-8(%rbp), %rax
	popq	%rbp
	retq
.Lfunc_end14:
	.size	ptr_kw, .Lfunc_end14-ptr_kw
                                        # -- End function
	.type	.L.str,@object                  # @.str
	.section	.rodata.str1.1,"aMS",@progbits,1
.L.str:
	.asciz	"lock "
	.size	.L.str, 6

	.type	.L.str.1,@object                # @.str.1
.L.str.1:
	.asciz	"endbr64"
	.size	.L.str.1, 8

	.type	.L.str.2,@object                # @.str.2
.L.str.2:
	.asciz	"endbr32"
	.size	.L.str.2, 8

	.type	.L.str.3,@object                # @.str.3
.L.str.3:
	.asciz	"nop"
	.size	.L.str.3, 4

	.type	.L.str.4,@object                # @.str.4
.L.str.4:
	.asciz	"syscall"
	.size	.L.str.4, 8

	.type	.L.str.5,@object                # @.str.5
.L.str.5:
	.asciz	"ud2"
	.size	.L.str.5, 4

	.type	.L.str.6,@object                # @.str.6
.L.str.6:
	.asciz	"rdtsc"
	.size	.L.str.6, 6

	.type	.L.str.7,@object                # @.str.7
.L.str.7:
	.asciz	"cpuid"
	.size	.L.str.7, 6

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

	.type	.L.str.8,@object                # @.str.8
	.section	.rodata.str1.1,"aMS",@progbits,1
.L.str.8:
	.asciz	"set"
	.size	.L.str.8, 4

	.type	.L.str.9,@object                # @.str.9
.L.str.9:
	.asciz	"cmov"
	.size	.L.str.9, 5

	.type	.L.str.10,@object               # @.str.10
.L.str.10:
	.asciz	", "
	.size	.L.str.10, 3

	.type	.L.str.11,@object               # @.str.11
.L.str.11:
	.asciz	"movsx "
	.size	.L.str.11, 7

	.type	.L.str.12,@object               # @.str.12
.L.str.12:
	.asciz	"movzx "
	.size	.L.str.12, 7

	.type	.L.str.13,@object               # @.str.13
.L.str.13:
	.asciz	"imul "
	.size	.L.str.13, 6

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

	.type	.L.str.49,@object               # @.str.49
	.section	.rodata.str1.1,"aMS",@progbits,1
.L.str.49:
	.asciz	" al, "
	.size	.L.str.49, 6

	.type	.L.str.50,@object               # @.str.50
.L.str.50:
	.asciz	"push "
	.size	.L.str.50, 6

	.type	R64,@object                     # @R64
	.section	.data.rel.ro,"aw",@progbits
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

	.type	.L.str.51,@object               # @.str.51
	.section	.rodata.str1.1,"aMS",@progbits,1
.L.str.51:
	.asciz	"pop "
	.size	.L.str.51, 5

	.type	.L.str.52,@object               # @.str.52
.L.str.52:
	.asciz	"movsxd "
	.size	.L.str.52, 8

	.type	.L.str.53,@object               # @.str.53
.L.str.53:
	.asciz	"test "
	.size	.L.str.53, 6

	.type	.L.str.54,@object               # @.str.54
.L.str.54:
	.asciz	"xchg "
	.size	.L.str.54, 6

	.type	.L.str.55,@object               # @.str.55
.L.str.55:
	.asciz	"mov "
	.size	.L.str.55, 5

	.type	.L.str.56,@object               # @.str.56
.L.str.56:
	.asciz	"lea "
	.size	.L.str.56, 5

	.type	.L.str.57,@object               # @.str.57
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

	.type	.L.str.64,@object               # @.str.64
.L.str.64:
	.asciz	"rep "
	.size	.L.str.64, 5

	.type	.L.str.65,@object               # @.str.65
.L.str.65:
	.asciz	"repnz "
	.size	.L.str.65, 7

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

	.type	.L.str.71,@object               # @.str.71
.L.str.71:
	.asciz	"movabs "
	.size	.L.str.71, 8

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

	.type	.L.str.72,@object               # @.str.72
	.section	.rodata.str1.1,"aMS",@progbits,1
.L.str.72:
	.asciz	", 1"
	.size	.L.str.72, 4

	.type	.L.str.73,@object               # @.str.73
.L.str.73:
	.asciz	", cl"
	.size	.L.str.73, 5

	.type	.L.str.74,@object               # @.str.74
.L.str.74:
	.asciz	"ret "
	.size	.L.str.74, 5

	.type	.L.str.75,@object               # @.str.75
.L.str.75:
	.asciz	"ret"
	.size	.L.str.75, 4

	.type	.L.str.76,@object               # @.str.76
.L.str.76:
	.asciz	"leave"
	.size	.L.str.76, 6

	.type	.L.str.77,@object               # @.str.77
.L.str.77:
	.asciz	"int3"
	.size	.L.str.77, 5

	.type	.L.str.78,@object               # @.str.78
.L.str.78:
	.asciz	"int "
	.size	.L.str.78, 5

	.type	.L.str.79,@object               # @.str.79
.L.str.79:
	.asciz	"call "
	.size	.L.str.79, 6

	.type	.L.str.80,@object               # @.str.80
.L.str.80:
	.asciz	"jmp "
	.size	.L.str.80, 5

	.type	.L.str.81,@object               # @.str.81
.L.str.81:
	.asciz	"hlt"
	.size	.L.str.81, 4

	.type	.L.str.82,@object               # @.str.82
.L.str.82:
	.asciz	"cmc"
	.size	.L.str.82, 4

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

	.type	.L.str.100,@object              # @.str.100
.L.str.100:
	.asciz	"        # "
	.size	.L.str.100, 11

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

	.type	.L.str.117,@object              # @.str.117
.L.str.117:
	.asciz	"0x"
	.size	.L.str.117, 3

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

	.type	.L.str.214,@object              # @.str.214
.L.str.214:
	.asciz	"rip"
	.size	.L.str.214, 4

	.type	.L.str.215,@object              # @.str.215
.L.str.215:
	.asciz	"-0x"
	.size	.L.str.215, 4

	.type	.L.str.216,@object              # @.str.216
.L.str.216:
	.asciz	"+0x"
	.size	.L.str.216, 4

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
	.addrsig_sym sb_puts
	.addrsig_sym rd8
	.addrsig_sym parse_modrm
	.addrsig_sym rd_imm_sext
	.addrsig_sym sb_putc
	.addrsig_sym sb_0xhex
	.addrsig_sym render_rm
	.addrsig_sym reg_name
	.addrsig_sym render_mem
	.addrsig_sym rd64
	.addrsig_sym rd16
	.addrsig_sym rd32
	.addrsig_sym sb_hex
	.addrsig_sym ptr_kw
	.addrsig_sym CC
	.addrsig_sym XMM
	.addrsig_sym ALU
	.addrsig_sym R64
	.addrsig_sym SHIFT
	.addrsig_sym x86_decode.G3
	.addrsig_sym x86_decode.G5
	.addrsig_sym R32
	.addrsig_sym R16
	.addrsig_sym R8L
	.addrsig_sym R8H
