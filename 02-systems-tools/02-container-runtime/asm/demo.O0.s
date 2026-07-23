	.file	"demo.c"
	.text
	.globl	bpf_run                         # -- Begin function bpf_run
	.p2align	4
	.type	bpf_run,@function
bpf_run:                                # @bpf_run
# %bb.0:
	pushq	%rbp
	movq	%rsp, %rbp
	movq	%rdi, -16(%rbp)
	movl	%esi, -20(%rbp)
	movq	%rdx, -32(%rbp)
	movl	$0, -36(%rbp)
	movl	$0, -40(%rbp)
.LBB0_1:                                # =>This Inner Loop Header: Depth=1
	movl	-40(%rbp), %eax
	cmpl	-20(%rbp), %eax
	jb	.LBB0_3
# %bb.2:
	movl	$0, -4(%rbp)
	jmp	.LBB0_19
.LBB0_3:                                #   in Loop: Header=BB0_1 Depth=1
	movq	-16(%rbp), %rax
	movl	-40(%rbp), %ecx
                                        # kill: def $rcx killed $ecx
	movq	(%rax,%rcx,8), %rax
	movq	%rax, -48(%rbp)
	movzwl	-48(%rbp), %eax
	movl	%eax, -52(%rbp)                 # 4-byte Spill
	subl	$5, %eax
	je	.LBB0_15
	jmp	.LBB0_20
.LBB0_20:                               #   in Loop: Header=BB0_1 Depth=1
	movl	-52(%rbp), %eax                 # 4-byte Reload
	subl	$6, %eax
	je	.LBB0_16
	jmp	.LBB0_21
.LBB0_21:                               #   in Loop: Header=BB0_1 Depth=1
	movl	-52(%rbp), %eax                 # 4-byte Reload
	subl	$21, %eax
	je	.LBB0_11
	jmp	.LBB0_22
.LBB0_22:                               #   in Loop: Header=BB0_1 Depth=1
	movl	-52(%rbp), %eax                 # 4-byte Reload
	subl	$32, %eax
	jne	.LBB0_17
	jmp	.LBB0_4
.LBB0_4:                                #   in Loop: Header=BB0_1 Depth=1
	cmpl	$0, -44(%rbp)
	jne	.LBB0_6
# %bb.5:                                #   in Loop: Header=BB0_1 Depth=1
	movq	-32(%rbp), %rax
	movl	(%rax), %eax
	movl	%eax, -36(%rbp)
	jmp	.LBB0_10
.LBB0_6:                                #   in Loop: Header=BB0_1 Depth=1
	cmpl	$4, -44(%rbp)
	jne	.LBB0_8
# %bb.7:                                #   in Loop: Header=BB0_1 Depth=1
	movq	-32(%rbp), %rax
	movl	4(%rax), %eax
	movl	%eax, -36(%rbp)
	jmp	.LBB0_9
.LBB0_8:                                #   in Loop: Header=BB0_1 Depth=1
	movl	$0, -36(%rbp)
.LBB0_9:                                #   in Loop: Header=BB0_1 Depth=1
	jmp	.LBB0_10
.LBB0_10:                               #   in Loop: Header=BB0_1 Depth=1
	movl	-40(%rbp), %eax
	addl	$1, %eax
	movl	%eax, -40(%rbp)
	jmp	.LBB0_18
.LBB0_11:                               #   in Loop: Header=BB0_1 Depth=1
	movl	-36(%rbp), %eax
	cmpl	-44(%rbp), %eax
	jne	.LBB0_13
# %bb.12:                               #   in Loop: Header=BB0_1 Depth=1
	movl	-40(%rbp), %eax
	addl	$1, %eax
	movzbl	-46(%rbp), %ecx
	addl	%ecx, %eax
	movl	%eax, -40(%rbp)
	jmp	.LBB0_14
.LBB0_13:                               #   in Loop: Header=BB0_1 Depth=1
	movl	-40(%rbp), %eax
	addl	$1, %eax
	movzbl	-45(%rbp), %ecx
	addl	%ecx, %eax
	movl	%eax, -40(%rbp)
.LBB0_14:                               #   in Loop: Header=BB0_1 Depth=1
	jmp	.LBB0_18
.LBB0_15:                               #   in Loop: Header=BB0_1 Depth=1
	movl	-40(%rbp), %eax
	addl	$1, %eax
	addl	-44(%rbp), %eax
	movl	%eax, -40(%rbp)
	jmp	.LBB0_18
.LBB0_16:
	movl	-44(%rbp), %eax
	movl	%eax, -4(%rbp)
	jmp	.LBB0_19
.LBB0_17:
	movl	$0, -4(%rbp)
	jmp	.LBB0_19
.LBB0_18:                               #   in Loop: Header=BB0_1 Depth=1
	jmp	.LBB0_1
.LBB0_19:
	movl	-4(%rbp), %eax
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
	movl	%edi, -4(%rbp)
	movl	-4(%rbp), %eax
	movl	%eax, -12(%rbp)
	movl	$-1073741762, -8(%rbp)          # imm = 0xC000003E
	leaq	allowlist(%rip), %rdi
	movl	$9, %esi
	leaq	-12(%rbp), %rdx
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
	subq	$16, %rsp
	movl	$0, -4(%rbp)
	movl	$0, -8(%rbp)
	xorl	%edi, %edi
	callq	classify
	movl	%eax, %edx
	xorl	%eax, %eax
	movl	$1, %ecx
	cmpl	$2147418112, %edx               # imm = 0x7FFF0000
	cmovel	%ecx, %eax
	orl	-8(%rbp), %eax
	movl	%eax, -8(%rbp)
	movl	$1, %edi
	callq	classify
	movl	%eax, %edx
	xorl	%eax, %eax
	movl	$2, %ecx
	cmpl	$2147418112, %edx               # imm = 0x7FFF0000
	cmovel	%ecx, %eax
	orl	-8(%rbp), %eax
	movl	%eax, -8(%rbp)
	movl	$101, %edi
	callq	classify
	movl	%eax, %edx
	xorl	%eax, %eax
	movl	$4, %ecx
	cmpl	$2147418112, %edx               # imm = 0x7FFF0000
	cmovnel	%ecx, %eax
	orl	-8(%rbp), %eax
	movl	%eax, -8(%rbp)
	movl	-8(%rbp), %eax
	addq	$16, %rsp
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
	.addrsig_sym bpf_run
	.addrsig_sym classify
	.addrsig_sym allowlist
