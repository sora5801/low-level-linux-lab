	.file	"demo.c"
	.text
	.globl	compose_clone_flags             # -- Begin function compose_clone_flags
	.p2align	4
	.type	compose_clone_flags,@function
compose_clone_flags:                    # @compose_clone_flags
# %bb.0:
	pushq	%rbp
	movq	%rsp, %rbp
	movl	%edi, -4(%rbp)
	movl	$0, -8(%rbp)
	movl	$0, -12(%rbp)
.LBB0_1:                                # =>This Inner Loop Header: Depth=1
	movl	-12(%rbp), %eax
                                        # kill: def $rax killed $eax
	cmpq	$7, %rax
	jae	.LBB0_6
# %bb.2:                                #   in Loop: Header=BB0_1 Depth=1
	movl	-4(%rbp), %eax
	movl	-12(%rbp), %ecx
	movl	%ecx, %edx
	leaq	kNsMap(%rip), %rcx
	andl	(%rcx,%rdx,8), %eax
	cmpl	$0, %eax
	je	.LBB0_4
# %bb.3:                                #   in Loop: Header=BB0_1 Depth=1
	movl	-12(%rbp), %eax
	movl	%eax, %ecx
	leaq	kNsMap(%rip), %rax
	movl	4(%rax,%rcx,8), %eax
	orl	-8(%rbp), %eax
	movl	%eax, -8(%rbp)
.LBB0_4:                                #   in Loop: Header=BB0_1 Depth=1
	jmp	.LBB0_5
.LBB0_5:                                #   in Loop: Header=BB0_1 Depth=1
	movl	-12(%rbp), %eax
	addl	$1, %eax
	movl	%eax, -12(%rbp)
	jmp	.LBB0_1
.LBB0_6:
	movl	-8(%rbp), %eax
	popq	%rbp
	retq
.Lfunc_end0:
	.size	compose_clone_flags, .Lfunc_end0-compose_clone_flags
                                        # -- End function
	.globl	cap_keep_mask                   # -- Begin function cap_keep_mask
	.p2align	4
	.type	cap_keep_mask,@function
cap_keep_mask:                          # @cap_keep_mask
# %bb.0:
	pushq	%rbp
	movq	%rsp, %rbp
	movq	$0, -8(%rbp)
	movl	$0, -12(%rbp)
.LBB1_1:                                # =>This Inner Loop Header: Depth=1
	movl	-12(%rbp), %eax
                                        # kill: def $rax killed $eax
	cmpq	$14, %rax
	jae	.LBB1_4
# %bb.2:                                #   in Loop: Header=BB1_1 Depth=1
	movl	-12(%rbp), %eax
	movl	%eax, %ecx
	leaq	kKeep(%rip), %rax
	movzbl	(%rax,%rcx), %eax
	movl	%eax, %eax
	movl	%eax, %ecx
	movl	$1, %eax
                                        # kill: def $cl killed $rcx
	shlq	%cl, %rax
	orq	-8(%rbp), %rax
	movq	%rax, -8(%rbp)
# %bb.3:                                #   in Loop: Header=BB1_1 Depth=1
	movl	-12(%rbp), %eax
	addl	$1, %eax
	movl	%eax, -12(%rbp)
	jmp	.LBB1_1
.LBB1_4:
	movq	-8(%rbp), %rax
	popq	%rbp
	retq
.Lfunc_end1:
	.size	cap_keep_mask, .Lfunc_end1-cap_keep_mask
                                        # -- End function
	.globl	cap_drop_mask                   # -- Begin function cap_drop_mask
	.p2align	4
	.type	cap_drop_mask,@function
cap_drop_mask:                          # @cap_drop_mask
# %bb.0:
	pushq	%rbp
	movq	%rsp, %rbp
	movq	%rdi, -8(%rbp)
	movabsq	$2199023255551, %rax            # imm = 0x1FFFFFFFFFF
	movq	%rax, -16(%rbp)
	movq	-16(%rbp), %rax
	movq	-8(%rbp), %rcx
	xorq	$-1, %rcx
	andq	%rcx, %rax
	popq	%rbp
	retq
.Lfunc_end2:
	.size	cap_drop_mask, .Lfunc_end2-cap_drop_mask
                                        # -- End function
	.globl	main                            # -- Begin function main
	.p2align	4
	.type	main,@function
main:                                   # @main
# %bb.0:
	pushq	%rbp
	movq	%rsp, %rbp
	subq	$48, %rsp
	movl	$0, -4(%rbp)
	movl	$0, -8(%rbp)
	movl	$63, %edi
	callq	compose_clone_flags
	movl	%eax, -12(%rbp)
	movl	-12(%rbp), %ecx
	andl	$268435456, %ecx                # imm = 0x10000000
	xorl	%eax, %eax
                                        # kill: def $al killed $al killed $eax
	cmpl	$0, %ecx
	movb	%al, -33(%rbp)                  # 1-byte Spill
	je	.LBB3_2
# %bb.1:
	movl	-12(%rbp), %eax
	andl	$1073741824, %eax               # imm = 0x40000000
	cmpl	$0, %eax
	setne	%al
	movb	%al, -33(%rbp)                  # 1-byte Spill
.LBB3_2:
	movb	-33(%rbp), %dl                  # 1-byte Reload
	xorl	%eax, %eax
	movl	$1, %ecx
	testb	$1, %dl
	cmovnel	%ecx, %eax
	orl	-8(%rbp), %eax
	movl	%eax, -8(%rbp)
	callq	cap_keep_mask
	movq	%rax, -24(%rbp)
	movq	-24(%rbp), %rdi
	callq	cap_drop_mask
	movq	%rax, -32(%rbp)
	movq	-24(%rbp), %rdi
	movl	$13, %esi
	callq	cap_is_kept
	movl	%eax, %edx
	xorl	%eax, %eax
	movl	$2, %ecx
	cmpl	$0, %edx
	cmovnel	%ecx, %eax
	orl	-8(%rbp), %eax
	movl	%eax, -8(%rbp)
	movq	-32(%rbp), %rdx
	shrq	$21, %rdx
	andq	$1, %rdx
	xorl	%eax, %eax
	movl	$4, %ecx
	cmpq	$0, %rdx
	cmovnel	%ecx, %eax
	orl	-8(%rbp), %eax
	movl	%eax, -8(%rbp)
	movl	-8(%rbp), %eax
	addq	$48, %rsp
	popq	%rbp
	retq
.Lfunc_end3:
	.size	main, .Lfunc_end3-main
                                        # -- End function
	.p2align	4                               # -- Begin function cap_is_kept
	.type	cap_is_kept,@function
cap_is_kept:                            # @cap_is_kept
# %bb.0:
	pushq	%rbp
	movq	%rsp, %rbp
	movq	%rdi, -8(%rbp)
	movl	%esi, -12(%rbp)
	movq	-8(%rbp), %rax
	movl	-12(%rbp), %ecx
                                        # kill: def $rcx killed $ecx
                                        # kill: def $cl killed $rcx
	shrq	%cl, %rax
	andq	$1, %rax
                                        # kill: def $eax killed $eax killed $rax
	popq	%rbp
	retq
.Lfunc_end4:
	.size	cap_is_kept, .Lfunc_end4-cap_is_kept
                                        # -- End function
	.type	kNsMap,@object                  # @kNsMap
	.section	.rodata,"a",@progbits
	.p2align	4, 0x0
kNsMap:
	.long	1                               # 0x1
	.long	268435456                       # 0x10000000
	.long	2                               # 0x2
	.long	131072                          # 0x20000
	.long	4                               # 0x4
	.long	536870912                       # 0x20000000
	.long	8                               # 0x8
	.long	1073741824                      # 0x40000000
	.long	16                              # 0x10
	.long	67108864                        # 0x4000000
	.long	32                              # 0x20
	.long	134217728                       # 0x8000000
	.long	64                              # 0x40
	.long	33554432                        # 0x2000000
	.size	kNsMap, 56

	.type	kKeep,@object                   # @kKeep
kKeep:
	.ascii	"\000\001\003\004\005\006\007\b\n\r\022\033\035\037"
	.size	kKeep, 14

	.ident	"clang version 20.1.8"
	.section	".note.GNU-stack","",@progbits
	.addrsig
	.addrsig_sym compose_clone_flags
	.addrsig_sym cap_keep_mask
	.addrsig_sym cap_drop_mask
	.addrsig_sym cap_is_kept
	.addrsig_sym kNsMap
	.addrsig_sym kKeep
