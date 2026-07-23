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
	pushq	%rbp
	movq	%rsp, %rbp
	movq	(%rdi), %rcx
	leaq	1(%rax), %rdx
	movq	%rdx, 8(%rdi)
	movb	%sil, (%rcx,%rax)
	popq	%rbp
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
	pushq	%rbp
	movq	%rsp, %rbp
	movl	%esi, %eax
	movq	8(%rdi), %rcx
	cmpq	16(%rdi), %rcx
	jae	.LBB1_1
# %bb.2:
	movq	(%rdi), %rdx
	leaq	1(%rcx), %rsi
	movq	%rsi, 8(%rdi)
	movb	%al, (%rdx,%rcx)
	jmp	.LBB1_3
.LBB1_1:
	movl	$1, 24(%rdi)
.LBB1_3:
	movq	8(%rdi), %rcx
	cmpq	16(%rdi), %rcx
	jae	.LBB1_4
# %bb.5:
	movq	(%rdi), %rdx
	leaq	1(%rcx), %rsi
	movq	%rsi, 8(%rdi)
	movb	%ah, (%rdx,%rcx)
	jmp	.LBB1_6
.LBB1_4:
	movl	$1, 24(%rdi)
.LBB1_6:
	movq	8(%rdi), %rcx
	cmpq	16(%rdi), %rcx
	jae	.LBB1_7
# %bb.8:
	movl	%eax, %edx
	shrl	$16, %edx
	movq	(%rdi), %rsi
	leaq	1(%rcx), %r8
	movq	%r8, 8(%rdi)
	movb	%dl, (%rsi,%rcx)
	jmp	.LBB1_9
.LBB1_7:
	movl	$1, 24(%rdi)
.LBB1_9:
	movq	8(%rdi), %rcx
	cmpq	16(%rdi), %rcx
	jae	.LBB1_10
# %bb.11:
	shrl	$24, %eax
	movq	(%rdi), %rdx
	leaq	1(%rcx), %rsi
	movq	%rsi, 8(%rdi)
	movb	%al, (%rdx,%rcx)
	popq	%rbp
	retq
.LBB1_10:
	movl	$1, 24(%rdi)
	popq	%rbp
	retq
.Lfunc_end1:
	.size	emit_u32, .Lfunc_end1-emit_u32
                                        # -- End function
	.globl	modrm                           # -- Begin function modrm
	.p2align	4
	.type	modrm,@function
modrm:                                  # @modrm
# %bb.0:
	pushq	%rbp
	movq	%rsp, %rbp
                                        # kill: def $esi killed $esi def $rsi
	shlb	$6, %dil
	leal	(,%rsi,8), %eax
	andb	$56, %al
	orb	%dil, %al
	andb	$7, %dl
	orb	%dl, %al
                                        # kill: def $al killed $al killed $eax
	popq	%rbp
	retq
.Lfunc_end2:
	.size	modrm, .Lfunc_end2-modrm
                                        # -- End function
	.globl	emit_add_ptr                    # -- Begin function emit_add_ptr
	.p2align	4
	.type	emit_add_ptr,@function
emit_add_ptr:                           # @emit_add_ptr
# %bb.0:
	pushq	%rbp
	movq	%rsp, %rbp
	movl	%esi, %eax
	movq	8(%rdi), %rcx
	cmpq	16(%rdi), %rcx
	jae	.LBB3_1
# %bb.2:
	movq	(%rdi), %rdx
	leaq	1(%rcx), %rsi
	movq	%rsi, 8(%rdi)
	movb	$72, (%rdx,%rcx)
	jmp	.LBB3_3
.LBB3_1:
	movl	$1, 24(%rdi)
.LBB3_3:
	movq	8(%rdi), %rcx
	cmpq	16(%rdi), %rcx
	jae	.LBB3_4
# %bb.5:
	movq	(%rdi), %rdx
	leaq	1(%rcx), %rsi
	movq	%rsi, 8(%rdi)
	movb	$-127, (%rdx,%rcx)
	jmp	.LBB3_6
.LBB3_4:
	movl	$1, 24(%rdi)
.LBB3_6:
	movq	8(%rdi), %rcx
	cmpq	16(%rdi), %rcx
	jae	.LBB3_7
# %bb.8:
	movq	(%rdi), %rdx
	leaq	1(%rcx), %rsi
	movq	%rsi, 8(%rdi)
	movb	$-61, (%rdx,%rcx)
	jmp	.LBB3_9
.LBB3_7:
	movl	$1, 24(%rdi)
.LBB3_9:
	movq	8(%rdi), %rcx
	cmpq	16(%rdi), %rcx
	jae	.LBB3_10
# %bb.11:
	movq	(%rdi), %rdx
	leaq	1(%rcx), %rsi
	movq	%rsi, 8(%rdi)
	movb	%al, (%rdx,%rcx)
	jmp	.LBB3_12
.LBB3_10:
	movl	$1, 24(%rdi)
.LBB3_12:
	movq	8(%rdi), %rcx
	cmpq	16(%rdi), %rcx
	jae	.LBB3_13
# %bb.14:
	movq	(%rdi), %rdx
	leaq	1(%rcx), %rsi
	movq	%rsi, 8(%rdi)
	movb	%ah, (%rdx,%rcx)
	jmp	.LBB3_15
.LBB3_13:
	movl	$1, 24(%rdi)
.LBB3_15:
	movq	8(%rdi), %rcx
	cmpq	16(%rdi), %rcx
	jae	.LBB3_16
# %bb.17:
	movl	%eax, %edx
	shrl	$16, %edx
	movq	(%rdi), %rsi
	leaq	1(%rcx), %r8
	movq	%r8, 8(%rdi)
	movb	%dl, (%rsi,%rcx)
	jmp	.LBB3_18
.LBB3_16:
	movl	$1, 24(%rdi)
.LBB3_18:
	movq	8(%rdi), %rcx
	cmpq	16(%rdi), %rcx
	jae	.LBB3_19
# %bb.20:
	shrl	$24, %eax
	movq	(%rdi), %rdx
	leaq	1(%rcx), %rsi
	movq	%rsi, 8(%rdi)
	movb	%al, (%rdx,%rcx)
	popq	%rbp
	retq
.LBB3_19:
	movl	$1, 24(%rdi)
	popq	%rbp
	retq
.Lfunc_end3:
	.size	emit_add_ptr, .Lfunc_end3-emit_add_ptr
                                        # -- End function
	.globl	emit_add_cell                   # -- Begin function emit_add_cell
	.p2align	4
	.type	emit_add_cell,@function
emit_add_cell:                          # @emit_add_cell
# %bb.0:
	pushq	%rbp
	movq	%rsp, %rbp
	movq	8(%rdi), %rax
	cmpq	16(%rdi), %rax
	jae	.LBB4_1
# %bb.2:
	movq	(%rdi), %rcx
	leaq	1(%rax), %rdx
	movq	%rdx, 8(%rdi)
	movb	$-128, (%rcx,%rax)
	jmp	.LBB4_3
.LBB4_1:
	movl	$1, 24(%rdi)
.LBB4_3:
	movq	8(%rdi), %rax
	cmpq	16(%rdi), %rax
	jae	.LBB4_4
# %bb.5:
	movq	(%rdi), %rcx
	leaq	1(%rax), %rdx
	movq	%rdx, 8(%rdi)
	movb	$3, (%rcx,%rax)
	jmp	.LBB4_6
.LBB4_4:
	movl	$1, 24(%rdi)
.LBB4_6:
	movq	8(%rdi), %rax
	cmpq	16(%rdi), %rax
	jae	.LBB4_7
# %bb.8:
	movq	(%rdi), %rcx
	leaq	1(%rax), %rdx
	movq	%rdx, 8(%rdi)
	movb	%sil, (%rcx,%rax)
	popq	%rbp
	retq
.LBB4_7:
	movl	$1, 24(%rdi)
	popq	%rbp
	retq
.Lfunc_end4:
	.size	emit_add_cell, .Lfunc_end4-emit_add_cell
                                        # -- End function
	.globl	patch_rel32                     # -- Begin function patch_rel32
	.p2align	4
	.type	patch_rel32,@function
patch_rel32:                            # @patch_rel32
# %bb.0:
	pushq	%rbp
	movq	%rsp, %rbp
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
	popq	%rbp
	retq
.Lfunc_end5:
	.size	patch_rel32, .Lfunc_end5-patch_rel32
                                        # -- End function
	.ident	"clang version 20.1.8"
	.section	".note.GNU-stack","",@progbits
	.addrsig
