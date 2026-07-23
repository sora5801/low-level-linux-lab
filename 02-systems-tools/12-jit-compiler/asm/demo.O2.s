	.file	"demo.c"
	.text
	.globl	emit_u8                         # -- Begin function emit_u8
	.p2align	4
	.type	emit_u8,@function
emit_u8:                                # @emit_u8
# %bb.0:
	movq	8(%rdi), %rax
	cmpq	16(%rdi), %rax
	jae	.LBB0_1
# %bb.3:
	movq	(%rdi), %rcx
	leaq	1(%rax), %rdx
	movq	%rdx, 8(%rdi)
	movb	%sil, (%rcx,%rax)
	retq
.LBB0_1:
	movl	$1, 24(%rdi)
	retq
.Lfunc_end0:
	.size	emit_u8, .Lfunc_end0-emit_u8
                                        # -- End function
	.globl	emit_u32                        # -- Begin function emit_u32
	.p2align	4
	.type	emit_u32,@function
emit_u32:                               # @emit_u32
# %bb.0:
	movl	%esi, %eax
	movq	8(%rdi), %rcx
	movq	16(%rdi), %rdx
	cmpq	%rdx, %rcx
	jae	.LBB1_1
# %bb.2:
	movq	(%rdi), %rdx
	leaq	1(%rcx), %rsi
	movq	%rsi, 8(%rdi)
	movb	%al, (%rdx,%rcx)
	movq	8(%rdi), %rcx
	movq	16(%rdi), %rdx
	cmpq	%rdx, %rcx
	jb	.LBB1_5
.LBB1_4:
	movl	$1, 24(%rdi)
	cmpq	%rdx, %rcx
	jb	.LBB1_8
.LBB1_7:
	movl	$1, 24(%rdi)
	cmpq	%rdx, %rcx
	jb	.LBB1_11
.LBB1_10:
	movl	$1, 24(%rdi)
	retq
.LBB1_1:
	movl	$1, 24(%rdi)
	cmpq	%rdx, %rcx
	jae	.LBB1_4
.LBB1_5:
	movq	(%rdi), %rdx
	leaq	1(%rcx), %rsi
	movq	%rsi, 8(%rdi)
	movb	%ah, (%rdx,%rcx)
	movq	8(%rdi), %rcx
	movq	16(%rdi), %rdx
	cmpq	%rdx, %rcx
	jae	.LBB1_7
.LBB1_8:
	movl	%eax, %edx
	shrl	$16, %edx
	movq	(%rdi), %rsi
	leaq	1(%rcx), %r8
	movq	%r8, 8(%rdi)
	movb	%dl, (%rsi,%rcx)
	movq	8(%rdi), %rcx
	movq	16(%rdi), %rdx
	cmpq	%rdx, %rcx
	jae	.LBB1_10
.LBB1_11:
	shrl	$24, %eax
	movq	(%rdi), %rdx
	leaq	1(%rcx), %rsi
	movq	%rsi, 8(%rdi)
	movb	%al, (%rdx,%rcx)
	retq
.Lfunc_end1:
	.size	emit_u32, .Lfunc_end1-emit_u32
                                        # -- End function
	.globl	modrm                           # -- Begin function modrm
	.p2align	4
	.type	modrm,@function
modrm:                                  # @modrm
# %bb.0:
                                        # kill: def $esi killed $esi def $rsi
	shlb	$6, %dil
	leal	(,%rsi,8), %eax
	andb	$56, %al
	orb	%dil, %al
	andb	$7, %dl
	orb	%dl, %al
                                        # kill: def $al killed $al killed $eax
	retq
.Lfunc_end2:
	.size	modrm, .Lfunc_end2-modrm
                                        # -- End function
	.globl	emit_add_ptr                    # -- Begin function emit_add_ptr
	.p2align	4
	.type	emit_add_ptr,@function
emit_add_ptr:                           # @emit_add_ptr
# %bb.0:
	movl	%esi, %eax
	movq	8(%rdi), %rcx
	movq	16(%rdi), %rdx
	cmpq	%rdx, %rcx
	jae	.LBB3_1
# %bb.2:
	movq	(%rdi), %rdx
	leaq	1(%rcx), %rsi
	movq	%rsi, 8(%rdi)
	movb	$72, (%rdx,%rcx)
	movq	8(%rdi), %rcx
	movq	16(%rdi), %rdx
	cmpq	%rdx, %rcx
	jb	.LBB3_5
.LBB3_4:
	movl	$1, 24(%rdi)
	cmpq	%rdx, %rcx
	jb	.LBB3_8
.LBB3_7:
	movl	$1, 24(%rdi)
	cmpq	%rdx, %rcx
	jb	.LBB3_11
.LBB3_10:
	movl	$1, 24(%rdi)
	cmpq	%rdx, %rcx
	jb	.LBB3_14
.LBB3_13:
	movl	$1, 24(%rdi)
	cmpq	%rdx, %rcx
	jb	.LBB3_17
.LBB3_16:
	movl	$1, 24(%rdi)
	cmpq	%rdx, %rcx
	jb	.LBB3_20
.LBB3_19:
	movl	$1, 24(%rdi)
	retq
.LBB3_1:
	movl	$1, 24(%rdi)
	cmpq	%rdx, %rcx
	jae	.LBB3_4
.LBB3_5:
	movq	(%rdi), %rdx
	leaq	1(%rcx), %rsi
	movq	%rsi, 8(%rdi)
	movb	$-127, (%rdx,%rcx)
	movq	8(%rdi), %rcx
	movq	16(%rdi), %rdx
	cmpq	%rdx, %rcx
	jae	.LBB3_7
.LBB3_8:
	movq	(%rdi), %rdx
	leaq	1(%rcx), %rsi
	movq	%rsi, 8(%rdi)
	movb	$-61, (%rdx,%rcx)
	movq	8(%rdi), %rcx
	movq	16(%rdi), %rdx
	cmpq	%rdx, %rcx
	jae	.LBB3_10
.LBB3_11:
	movq	(%rdi), %rdx
	leaq	1(%rcx), %rsi
	movq	%rsi, 8(%rdi)
	movb	%al, (%rdx,%rcx)
	movq	8(%rdi), %rcx
	movq	16(%rdi), %rdx
	cmpq	%rdx, %rcx
	jae	.LBB3_13
.LBB3_14:
	movq	(%rdi), %rdx
	leaq	1(%rcx), %rsi
	movq	%rsi, 8(%rdi)
	movb	%ah, (%rdx,%rcx)
	movq	8(%rdi), %rcx
	movq	16(%rdi), %rdx
	cmpq	%rdx, %rcx
	jae	.LBB3_16
.LBB3_17:
	movl	%eax, %edx
	shrl	$16, %edx
	movq	(%rdi), %rsi
	leaq	1(%rcx), %r8
	movq	%r8, 8(%rdi)
	movb	%dl, (%rsi,%rcx)
	movq	8(%rdi), %rcx
	movq	16(%rdi), %rdx
	cmpq	%rdx, %rcx
	jae	.LBB3_19
.LBB3_20:
	shrl	$24, %eax
	movq	(%rdi), %rdx
	leaq	1(%rcx), %rsi
	movq	%rsi, 8(%rdi)
	movb	%al, (%rdx,%rcx)
	retq
.Lfunc_end3:
	.size	emit_add_ptr, .Lfunc_end3-emit_add_ptr
                                        # -- End function
	.globl	emit_add_cell                   # -- Begin function emit_add_cell
	.p2align	4
	.type	emit_add_cell,@function
emit_add_cell:                          # @emit_add_cell
# %bb.0:
	movq	8(%rdi), %rax
	movq	16(%rdi), %rcx
	cmpq	%rcx, %rax
	jae	.LBB4_1
# %bb.2:
	movq	(%rdi), %rcx
	leaq	1(%rax), %rdx
	movq	%rdx, 8(%rdi)
	movb	$-128, (%rcx,%rax)
	movq	8(%rdi), %rax
	movq	16(%rdi), %rcx
	cmpq	%rcx, %rax
	jb	.LBB4_5
.LBB4_4:
	movl	$1, 24(%rdi)
	cmpq	%rcx, %rax
	jb	.LBB4_8
.LBB4_7:
	movl	$1, 24(%rdi)
	retq
.LBB4_1:
	movl	$1, 24(%rdi)
	cmpq	%rcx, %rax
	jae	.LBB4_4
.LBB4_5:
	movq	(%rdi), %rcx
	leaq	1(%rax), %rdx
	movq	%rdx, 8(%rdi)
	movb	$3, (%rcx,%rax)
	movq	8(%rdi), %rax
	movq	16(%rdi), %rcx
	cmpq	%rcx, %rax
	jae	.LBB4_7
.LBB4_8:
	movq	(%rdi), %rcx
	leaq	1(%rax), %rdx
	movq	%rdx, 8(%rdi)
	movb	%sil, (%rcx,%rax)
	retq
.Lfunc_end4:
	.size	emit_add_cell, .Lfunc_end4-emit_add_cell
                                        # -- End function
	.globl	patch_rel32                     # -- Begin function patch_rel32
	.p2align	4
	.type	patch_rel32,@function
patch_rel32:                            # @patch_rel32
# %bb.0:
	subl	%esi, %edx
	addl	$-4, %edx
	movq	(%rdi), %rax
	movb	%dl, (%rax,%rsi)
	movq	(%rdi), %rax
	movb	%dh, 1(%rax,%rsi)
	movl	%edx, %eax
	shrl	$16, %eax
	movq	(%rdi), %rcx
	movb	%al, 2(%rcx,%rsi)
	shrl	$24, %edx
	movq	(%rdi), %rax
	movb	%dl, 3(%rax,%rsi)
	retq
.Lfunc_end5:
	.size	patch_rel32, .Lfunc_end5-patch_rel32
                                        # -- End function
	.ident	"clang version 20.1.8"
	.section	".note.GNU-stack","",@progbits
	.addrsig
