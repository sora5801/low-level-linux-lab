	.file	"demo.c"
	.text
	.globl	sector_to_bytes                 # -- Begin function sector_to_bytes
	.p2align	4
	.type	sector_to_bytes,@function
sector_to_bytes:                        # @sector_to_bytes
# %bb.0:
	pushq	%rbp
	movq	%rsp, %rbp
	movq	%rdi, -8(%rbp)
	movq	-8(%rbp), %rax
	shlq	$9, %rax
	popq	%rbp
	retq
.Lfunc_end0:
	.size	sector_to_bytes, .Lfunc_end0-sector_to_bytes
                                        # -- End function
	.globl	ramblk_range_ok                 # -- Begin function ramblk_range_ok
	.p2align	4
	.type	ramblk_range_ok,@function
ramblk_range_ok:                        # @ramblk_range_ok
# %bb.0:
	pushq	%rbp
	movq	%rsp, %rbp
	movq	%rdi, -16(%rbp)
	movl	%esi, -20(%rbp)
	movq	%rdx, -32(%rbp)
	movq	%rcx, -40(%rbp)
	movq	-16(%rbp), %rax
	shlq	$9, %rax
	movq	%rax, -48(%rbp)
	movq	-48(%rbp), %rax
	cmpq	-32(%rbp), %rax
	jbe	.LBB1_2
# %bb.1:
	movl	$0, -4(%rbp)
	jmp	.LBB1_5
.LBB1_2:
	movl	-20(%rbp), %eax
                                        # kill: def $rax killed $eax
	movq	-32(%rbp), %rcx
	subq	-48(%rbp), %rcx
	cmpq	%rcx, %rax
	jbe	.LBB1_4
# %bb.3:
	movl	$0, -4(%rbp)
	jmp	.LBB1_5
.LBB1_4:
	movq	-48(%rbp), %rcx
	movq	-40(%rbp), %rax
	movq	%rcx, (%rax)
	movl	$1, -4(%rbp)
.LBB1_5:
	movl	-4(%rbp), %eax
	popq	%rbp
	retq
.Lfunc_end1:
	.size	ramblk_range_ok, .Lfunc_end1-ramblk_range_ok
                                        # -- End function
	.globl	ramblk_advance                  # -- Begin function ramblk_advance
	.p2align	4
	.type	ramblk_advance,@function
ramblk_advance:                         # @ramblk_advance
# %bb.0:
	pushq	%rbp
	movq	%rsp, %rbp
	movq	%rdi, -8(%rbp)
	movl	%esi, -12(%rbp)
	movq	-8(%rbp), %rax
	movl	-12(%rbp), %ecx
                                        # kill: def $rcx killed $ecx
	addq	%rcx, %rax
	popq	%rbp
	retq
.Lfunc_end2:
	.size	ramblk_advance, .Lfunc_end2-ramblk_advance
                                        # -- End function
	.ident	"clang version 20.1.8"
	.section	".note.GNU-stack","",@progbits
	.addrsig
