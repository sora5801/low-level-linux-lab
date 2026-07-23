	.file	"demo.c"
	.text
	.globl	bpf_run                         # -- Begin function bpf_run
	.p2align	4
	.type	bpf_run,@function
bpf_run:                                # @bpf_run
# %bb.0:
	pushq	%rbp
	movq	%rsp, %rbp
	xorl	%ecx, %ecx
	xorl	%r8d, %r8d
                                        # implicit-def: $eax
	jmp	.LBB0_3
.LBB0_1:                                #   in Loop: Header=BB0_3 Depth=1
	xorl	%r9d, %r9d
	xorl	%eax, %eax
	.p2align	4
.LBB0_2:                                #   in Loop: Header=BB0_3 Depth=1
	testb	%r9b, %r9b
	je	.LBB0_21
.LBB0_3:                                # =>This Inner Loop Header: Depth=1
	cmpl	%esi, %ecx
	jae	.LBB0_20
# %bb.4:                                #   in Loop: Header=BB0_3 Depth=1
	movl	%ecx, %r9d
	movzwl	(%rdi,%r9,8), %r11d
	movl	4(%rdi,%r9,8), %r10d
	cmpl	$20, %r11d
	jg	.LBB0_8
# %bb.5:                                #   in Loop: Header=BB0_3 Depth=1
	cmpl	$5, %r11d
	je	.LBB0_13
# %bb.6:                                #   in Loop: Header=BB0_3 Depth=1
	cmpl	$6, %r11d
	jne	.LBB0_1
# %bb.7:                                #   in Loop: Header=BB0_3 Depth=1
	xorl	%r9d, %r9d
	movl	%r10d, %eax
	jmp	.LBB0_2
	.p2align	4
.LBB0_8:                                #   in Loop: Header=BB0_3 Depth=1
	cmpl	$21, %r11d
	je	.LBB0_14
# %bb.9:                                #   in Loop: Header=BB0_3 Depth=1
	cmpl	$32, %r11d
	jne	.LBB0_1
# %bb.10:                               #   in Loop: Header=BB0_3 Depth=1
	cmpl	$4, %r10d
	je	.LBB0_18
# %bb.11:                               #   in Loop: Header=BB0_3 Depth=1
	xorl	%r8d, %r8d
	testl	%r10d, %r10d
	jne	.LBB0_19
# %bb.12:                               #   in Loop: Header=BB0_3 Depth=1
	movl	(%rdx), %r8d
	jmp	.LBB0_19
.LBB0_13:                               #   in Loop: Header=BB0_3 Depth=1
	addl	%r10d, %ecx
	incl	%ecx
	movb	$1, %r9b
	jmp	.LBB0_2
.LBB0_14:                               #   in Loop: Header=BB0_3 Depth=1
	incl	%ecx
	cmpl	%r10d, %r8d
	jne	.LBB0_16
# %bb.15:                               #   in Loop: Header=BB0_3 Depth=1
	movzbl	2(%rdi,%r9,8), %r9d
	jmp	.LBB0_17
.LBB0_16:                               #   in Loop: Header=BB0_3 Depth=1
	movzbl	3(%rdi,%r9,8), %r9d
.LBB0_17:                               #   in Loop: Header=BB0_3 Depth=1
	addl	%r9d, %ecx
	movb	$1, %r9b
	jmp	.LBB0_2
.LBB0_18:                               #   in Loop: Header=BB0_3 Depth=1
	movl	4(%rdx), %r8d
.LBB0_19:                               #   in Loop: Header=BB0_3 Depth=1
	incl	%ecx
	movb	$1, %r9b
	jmp	.LBB0_2
.LBB0_20:
	xorl	%eax, %eax
.LBB0_21:
	popq	%rbp
	retq
.Lfunc_end0:
	.size	bpf_run, .Lfunc_end0-bpf_run
                                        # -- End function
	.globl	classify                        # -- Begin function classify
	.p2align	4
	.type	classify,@function
classify:                               # @classify
# %bb.0:
	pushq	%rbp
	movq	%rsp, %rbp
	subq	$16, %rsp
	movl	%edi, -8(%rbp)
	movl	$-1073741762, -4(%rbp)          # imm = 0xC000003E
	leaq	allowlist(%rip), %rdi
	leaq	-8(%rbp), %rdx
	movl	$9, %esi
	callq	bpf_run
	addq	$16, %rsp
	popq	%rbp
	retq
.Lfunc_end1:
	.size	classify, .Lfunc_end1-classify
                                        # -- End function
	.globl	main                            # -- Begin function main
	.p2align	4
	.type	main,@function
main:                                   # @main
# %bb.0:
	pushq	%rbp
	movq	%rsp, %rbp
	pushq	%r14
	pushq	%rbx
	subq	$16, %rsp
	movabsq	$-4611685752139415552, %rax     # imm = 0xC000003E00000000
	movq	%rax, -24(%rbp)
	leaq	allowlist(%rip), %rbx
	leaq	-24(%rbp), %rdx
	movq	%rbx, %rdi
	movl	$9, %esi
	callq	bpf_run
	xorl	%r14d, %r14d
	cmpl	$2147418112, %eax               # imm = 0x7FFF0000
	sete	%r14b
	movabsq	$-4611685752139415551, %rax     # imm = 0xC000003E00000001
	movq	%rax, -24(%rbp)
	leaq	-24(%rbp), %rdx
	movq	%rbx, %rdi
	movl	$9, %esi
	callq	bpf_run
	xorl	%ecx, %ecx
	cmpl	$2147418112, %eax               # imm = 0x7FFF0000
	sete	%cl
	leal	(%r14,%rcx,2), %r14d
	movabsq	$-4611685752139415451, %rax     # imm = 0xC000003E00000065
	movq	%rax, -24(%rbp)
	leaq	-24(%rbp), %rdx
	movq	%rbx, %rdi
	movl	$9, %esi
	callq	bpf_run
	xorl	%ecx, %ecx
	cmpl	$2147418112, %eax               # imm = 0x7FFF0000
	setne	%cl
	leal	(%r14,%rcx,4), %eax
	addq	$16, %rsp
	popq	%rbx
	popq	%r14
	popq	%rbp
	retq
.Lfunc_end2:
	.size	main, .Lfunc_end2-main
                                        # -- End function
	.type	allowlist,@object               # @allowlist
	.section	.rodata,"a",@progbits
	.p2align	4, 0x0
allowlist:
	.short	32                              # 0x20
	.byte	0                               # 0x0
	.byte	0                               # 0x0
	.long	4                               # 0x4
	.short	21                              # 0x15
	.byte	1                               # 0x1
	.byte	0                               # 0x0
	.long	3221225534                      # 0xc000003e
	.short	6                               # 0x6
	.byte	0                               # 0x0
	.byte	0                               # 0x0
	.long	0                               # 0x0
	.short	32                              # 0x20
	.byte	0                               # 0x0
	.byte	0                               # 0x0
	.long	0                               # 0x0
	.short	21                              # 0x15
	.byte	0                               # 0x0
	.byte	1                               # 0x1
	.long	0                               # 0x0
	.short	6                               # 0x6
	.byte	0                               # 0x0
	.byte	0                               # 0x0
	.long	2147418112                      # 0x7fff0000
	.short	21                              # 0x15
	.byte	0                               # 0x0
	.byte	1                               # 0x1
	.long	1                               # 0x1
	.short	6                               # 0x6
	.byte	0                               # 0x0
	.byte	0                               # 0x0
	.long	2147418112                      # 0x7fff0000
	.short	6                               # 0x6
	.byte	0                               # 0x0
	.byte	0                               # 0x0
	.long	327681                          # 0x50001
	.size	allowlist, 72

	.ident	"clang version 20.1.8"
	.section	".note.GNU-stack","",@progbits
	.addrsig
	.addrsig_sym allowlist
