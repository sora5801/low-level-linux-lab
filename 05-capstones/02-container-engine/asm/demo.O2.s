	.file	"demo.c"
	.text
	.globl	compose_clone_flags             # -- Begin function compose_clone_flags
	.p2align	4
	.type	compose_clone_flags,@function
compose_clone_flags:                    # @compose_clone_flags
# %bb.0:
	movl	%edi, %eax
	andl	$1, %eax
	shll	$28, %eax
	movl	%edi, %ecx
	andl	$2, %ecx
	shll	$16, %ecx
	orl	%eax, %ecx
	movl	%edi, %edx
	shll	$27, %edx
	movl	%edx, %eax
	andl	$536870912, %eax                # imm = 0x20000000
	orl	%ecx, %eax
	andl	$1073741824, %edx               # imm = 0x40000000
	orl	%eax, %edx
	movl	%edi, %eax
	shll	$22, %eax
	movl	%eax, %ecx
	andl	$67108864, %ecx                 # imm = 0x4000000
	orl	%edx, %ecx
	andl	$134217728, %eax                # imm = 0x8000000
	orl	%ecx, %eax
	andl	$64, %edi
	shll	$19, %edi
	orl	%edi, %eax
	retq
.Lfunc_end0:
	.size	compose_clone_flags, .Lfunc_end0-compose_clone_flags
                                        # -- End function
	.globl	cap_keep_mask                   # -- Begin function cap_keep_mask
	.p2align	4
	.type	cap_keep_mask,@function
cap_keep_mask:                          # @cap_keep_mask
# %bb.0:
	movl	$2818844155, %eax               # imm = 0xA80425FB
	retq
.Lfunc_end1:
	.size	cap_keep_mask, .Lfunc_end1-cap_keep_mask
                                        # -- End function
	.globl	cap_drop_mask                   # -- Begin function cap_drop_mask
	.p2align	4
	.type	cap_drop_mask,@function
cap_drop_mask:                          # @cap_drop_mask
# %bb.0:
	notq	%rdi
	movabsq	$2199023255551, %rax            # imm = 0x1FFFFFFFFFF
	andq	%rdi, %rax
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
	pushq	%rbx
	pushq	%rax
	movl	$63, %edi
	callq	compose_clone_flags
	notl	%eax
	xorl	%ebp, %ebp
	testl	$1342177280, %eax               # imm = 0x50000000
	sete	%bpl
	callq	cap_keep_mask
	movq	%rax, %rbx
	movq	%rax, %rdi
	callq	cap_drop_mask
	shrl	$12, %ebx
	andl	$2, %ebx
	orl	%ebp, %ebx
	shrl	$19, %eax
	andl	$4, %eax
	orl	%ebx, %eax
                                        # kill: def $eax killed $eax killed $rax
	addq	$8, %rsp
	popq	%rbx
	popq	%rbp
	retq
.Lfunc_end3:
	.size	main, .Lfunc_end3-main
                                        # -- End function
	.ident	"clang version 20.1.8"
	.section	".note.GNU-stack","",@progbits
	.addrsig
