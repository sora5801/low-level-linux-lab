	.file	"demo.c"
	.text
	.globl	compose_clone_flags             # -- Begin function compose_clone_flags
	.p2align	4
	.type	compose_clone_flags,@function
compose_clone_flags:                    # @compose_clone_flags
# %bb.0:
	pushq	%rbp
	movq	%rsp, %rbp
	xorl	%ecx, %ecx
	leaq	kNsMap(%rip), %rdx
	xorl	%eax, %eax
	jmp	.LBB0_1
	.p2align	4
.LBB0_3:                                #   in Loop: Header=BB0_1 Depth=1
	incq	%rcx
	cmpq	$7, %rcx
	je	.LBB0_4
.LBB0_1:                                # =>This Inner Loop Header: Depth=1
	testl	%edi, (%rdx,%rcx,8)
	je	.LBB0_3
# %bb.2:                                #   in Loop: Header=BB0_1 Depth=1
	orl	4(%rdx,%rcx,8), %eax
	jmp	.LBB0_3
.LBB0_4:
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
	movl	$2818844155, %eax               # imm = 0xA80425FB
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
	notq	%rdi
	movabsq	$2199023255551, %rax            # imm = 0x1FFFFFFFFFF
	andq	%rdi, %rax
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
	pushq	%r14
	pushq	%rbx
	movl	$63, %edi
	callq	compose_clone_flags
	notl	%eax
	xorl	%r14d, %r14d
	testl	$1342177280, %eax               # imm = 0x50000000
	sete	%r14b
	callq	cap_keep_mask
	movq	%rax, %rbx
	movq	%rax, %rdi
	callq	cap_drop_mask
	shrl	$12, %ebx
	andl	$2, %ebx
	orl	%r14d, %ebx
	shrl	$19, %eax
	andl	$4, %eax
	orl	%ebx, %eax
                                        # kill: def $eax killed $eax killed $rax
	popq	%rbx
	popq	%r14
	popq	%rbp
	retq
.Lfunc_end3:
	.size	main, .Lfunc_end3-main
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

	.ident	"clang version 20.1.8"
	.section	".note.GNU-stack","",@progbits
	.addrsig
