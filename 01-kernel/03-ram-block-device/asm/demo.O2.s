	.file	"demo.c"
	.text
	.globl	sector_to_bytes                 # -- Begin function sector_to_bytes
	.p2align	4
	.type	sector_to_bytes,@function
sector_to_bytes:                        # @sector_to_bytes
# %bb.0:
	movq	%rdi, %rax
	shlq	$9, %rax
	retq
.Lfunc_end0:
	.size	sector_to_bytes, .Lfunc_end0-sector_to_bytes
                                        # -- End function
	.globl	ramblk_range_ok                 # -- Begin function ramblk_range_ok
	.p2align	4
	.type	ramblk_range_ok,@function
ramblk_range_ok:                        # @ramblk_range_ok
# %bb.0:
	shlq	$9, %rdi
	subq	%rdi, %rdx
	setb	%r8b
	movl	%esi, %eax
	cmpq	%rax, %rdx
	setb	%dl
	xorl	%eax, %eax
	orb	%r8b, %dl
	jne	.LBB1_2
# %bb.1:
	movq	%rdi, (%rcx)
	movl	$1, %eax
.LBB1_2:
	retq
.Lfunc_end1:
	.size	ramblk_range_ok, .Lfunc_end1-ramblk_range_ok
                                        # -- End function
	.globl	ramblk_advance                  # -- Begin function ramblk_advance
	.p2align	4
	.type	ramblk_advance,@function
ramblk_advance:                         # @ramblk_advance
# %bb.0:
	movl	%esi, %eax
	addq	%rdi, %rax
	retq
.Lfunc_end2:
	.size	ramblk_advance, .Lfunc_end2-ramblk_advance
                                        # -- End function
	.ident	"clang version 20.1.8"
	.section	".note.GNU-stack","",@progbits
	.addrsig
