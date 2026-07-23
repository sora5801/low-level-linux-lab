	.file	"demo.c"
	.text
	.globl	bpf_run                         # -- Begin function bpf_run
	.p2align	4
	.type	bpf_run,@function
bpf_run:                                # @bpf_run
# %bb.0:
	xorl	%eax, %eax
	testl	%esi, %esi
	je	.LBB0_19
# %bb.1:
	xorl	%r8d, %r8d
	xorl	%ecx, %ecx
	jmp	.LBB0_5
.LBB0_2:                                #   in Loop: Header=BB0_5 Depth=1
	movzbl	3(%rdi,%r10,8), %r9d
.LBB0_3:                                #   in Loop: Header=BB0_5 Depth=1
	addl	%r9d, %ecx
.LBB0_4:                                #   in Loop: Header=BB0_5 Depth=1
	cmpl	%esi, %ecx
	jae	.LBB0_19
.LBB0_5:                                # =>This Inner Loop Header: Depth=1
	movl	%ecx, %r10d
	movzwl	(%rdi,%r10,8), %r11d
	movl	4(%rdi,%r10,8), %r9d
	cmpl	$20, %r11d
	jle	.LBB0_11
# %bb.6:                                #   in Loop: Header=BB0_5 Depth=1
	cmpl	$21, %r11d
	je	.LBB0_13
# %bb.7:                                #   in Loop: Header=BB0_5 Depth=1
	cmpl	$32, %r11d
	jne	.LBB0_19
# %bb.8:                                #   in Loop: Header=BB0_5 Depth=1
	cmpl	$4, %r9d
	je	.LBB0_15
# %bb.9:                                #   in Loop: Header=BB0_5 Depth=1
	xorl	%r8d, %r8d
	testl	%r9d, %r9d
	jne	.LBB0_16
# %bb.10:                               #   in Loop: Header=BB0_5 Depth=1
	movl	(%rdx), %r8d
	incl	%ecx
	jmp	.LBB0_4
	.p2align	4
.LBB0_11:                               #   in Loop: Header=BB0_5 Depth=1
	cmpl	$5, %r11d
	jne	.LBB0_17
# %bb.12:                               #   in Loop: Header=BB0_5 Depth=1
	addl	%r9d, %ecx
	incl	%ecx
	jmp	.LBB0_4
	.p2align	4
.LBB0_13:                               #   in Loop: Header=BB0_5 Depth=1
	incl	%ecx
	cmpl	%r9d, %r8d
	jne	.LBB0_2
# %bb.14:                               #   in Loop: Header=BB0_5 Depth=1
	movzbl	2(%rdi,%r10,8), %r9d
	jmp	.LBB0_3
.LBB0_15:                               #   in Loop: Header=BB0_5 Depth=1
	movl	4(%rdx), %r8d
.LBB0_16:                               #   in Loop: Header=BB0_5 Depth=1
	incl	%ecx
	jmp	.LBB0_4
.LBB0_17:
	cmpl	$6, %r11d
	jne	.LBB0_19
# %bb.18:
	movl	%r9d, %eax
.LBB0_19:
	retq
.Lfunc_end0:
	.size	bpf_run, .Lfunc_end0-bpf_run
                                        # -- End function
	.globl	classify                        # -- Begin function classify
	.p2align	4
	.type	classify,@function
classify:                               # @classify
# %bb.0:
	pushq	%rax
	movl	%edi, (%rsp)
	movl	$-1073741762, 4(%rsp)           # imm = 0xC000003E
	leaq	allowlist(%rip), %rdi
	movq	%rsp, %rdx
	movl	$9, %esi
	callq	bpf_run
	popq	%rcx
	retq
.Lfunc_end1:
	.size	classify, .Lfunc_end1-classify
                                        # -- End function
	.globl	main                            # -- Begin function main
	.p2align	4
	.type	main,@function
main:                                   # @main
# %bb.0:
	pushq	%r14
	pushq	%rbx
	pushq	%rax
	movabsq	$-4611685752139415552, %rax     # imm = 0xC000003E00000000
	movq	%rax, (%rsp)
	leaq	allowlist(%rip), %rbx
	movq	%rsp, %rdx
	movq	%rbx, %rdi
	movl	$9, %esi
	callq	bpf_run
	xorl	%r14d, %r14d
	cmpl	$2147418112, %eax               # imm = 0x7FFF0000
	sete	%r14b
	movabsq	$-4611685752139415551, %rax     # imm = 0xC000003E00000001
	movq	%rax, (%rsp)
	movq	%rsp, %rdx
	movq	%rbx, %rdi
	movl	$9, %esi
	callq	bpf_run
	xorl	%ecx, %ecx
	cmpl	$2147418112, %eax               # imm = 0x7FFF0000
	sete	%cl
	leal	(%r14,%rcx,2), %r14d
	movabsq	$-4611685752139415451, %rax     # imm = 0xC000003E00000065
	movq	%rax, (%rsp)
	movq	%rsp, %rdx
	movq	%rbx, %rdi
	movl	$9, %esi
	callq	bpf_run
	xorl	%ecx, %ecx
	cmpl	$2147418112, %eax               # imm = 0x7FFF0000
	setne	%cl
	leal	(%r14,%rcx,4), %eax
	addq	$8, %rsp
	popq	%rbx
	popq	%r14
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
