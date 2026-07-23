	.file	"emit.c"
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
	.globl	emit_prologue                   # -- Begin function emit_prologue
	.p2align	4
	.type	emit_prologue,@function
emit_prologue:                          # @emit_prologue
# %bb.0:
	movq	8(%rdi), %rax
	movq	16(%rdi), %rcx
	cmpq	%rcx, %rax
	jae	.LBB3_1
# %bb.2:
	movq	(%rdi), %rcx
	leaq	1(%rax), %rdx
	movq	%rdx, 8(%rdi)
	movb	$85, (%rcx,%rax)
	movq	8(%rdi), %rax
	movq	16(%rdi), %rcx
	cmpq	%rcx, %rax
	jb	.LBB3_5
.LBB3_4:
	movl	$1, 24(%rdi)
	cmpq	%rcx, %rax
	jb	.LBB3_8
.LBB3_7:
	movl	$1, 24(%rdi)
	cmpq	%rcx, %rax
	jb	.LBB3_11
.LBB3_10:
	movl	$1, 24(%rdi)
	cmpq	%rcx, %rax
	jb	.LBB3_14
.LBB3_13:
	movl	$1, 24(%rdi)
	cmpq	%rcx, %rax
	jb	.LBB3_17
.LBB3_16:
	movl	$1, 24(%rdi)
	cmpq	%rcx, %rax
	jb	.LBB3_20
.LBB3_19:
	movl	$1, 24(%rdi)
	cmpq	%rcx, %rax
	jb	.LBB3_23
.LBB3_22:
	movl	$1, 24(%rdi)
	cmpq	%rcx, %rax
	jb	.LBB3_26
.LBB3_25:
	movl	$1, 24(%rdi)
	cmpq	%rcx, %rax
	jb	.LBB3_29
.LBB3_28:
	movl	$1, 24(%rdi)
	cmpq	%rcx, %rax
	jb	.LBB3_32
.LBB3_31:
	movl	$1, 24(%rdi)
	cmpq	%rcx, %rax
	jb	.LBB3_35
.LBB3_34:
	movl	$1, 24(%rdi)
	cmpq	%rcx, %rax
	jb	.LBB3_38
.LBB3_37:
	movl	$1, 24(%rdi)
	cmpq	%rcx, %rax
	jb	.LBB3_41
.LBB3_40:
	movl	$1, 24(%rdi)
	cmpq	%rcx, %rax
	jb	.LBB3_44
.LBB3_43:
	movl	$1, 24(%rdi)
	cmpq	%rcx, %rax
	jb	.LBB3_47
.LBB3_46:
	movl	$1, 24(%rdi)
	cmpq	%rcx, %rax
	jb	.LBB3_50
.LBB3_49:
	movl	$1, 24(%rdi)
	cmpq	%rcx, %rax
	jb	.LBB3_53
.LBB3_52:
	movl	$1, 24(%rdi)
	cmpq	%rcx, %rax
	jb	.LBB3_56
.LBB3_55:
	movl	$1, 24(%rdi)
	cmpq	%rcx, %rax
	jb	.LBB3_59
.LBB3_58:
	movl	$1, 24(%rdi)
	cmpq	%rcx, %rax
	jb	.LBB3_62
.LBB3_61:
	movl	$1, 24(%rdi)
	cmpq	%rcx, %rax
	jb	.LBB3_65
.LBB3_64:
	movl	$1, 24(%rdi)
	retq
.LBB3_1:
	movl	$1, 24(%rdi)
	cmpq	%rcx, %rax
	jae	.LBB3_4
.LBB3_5:
	movq	(%rdi), %rcx
	leaq	1(%rax), %rdx
	movq	%rdx, 8(%rdi)
	movb	$72, (%rcx,%rax)
	movq	8(%rdi), %rax
	movq	16(%rdi), %rcx
	cmpq	%rcx, %rax
	jae	.LBB3_7
.LBB3_8:
	movq	(%rdi), %rcx
	leaq	1(%rax), %rdx
	movq	%rdx, 8(%rdi)
	movb	$-119, (%rcx,%rax)
	movq	8(%rdi), %rax
	movq	16(%rdi), %rcx
	cmpq	%rcx, %rax
	jae	.LBB3_10
.LBB3_11:
	movq	(%rdi), %rcx
	leaq	1(%rax), %rdx
	movq	%rdx, 8(%rdi)
	movb	$-27, (%rcx,%rax)
	movq	8(%rdi), %rax
	movq	16(%rdi), %rcx
	cmpq	%rcx, %rax
	jae	.LBB3_13
.LBB3_14:
	movq	(%rdi), %rcx
	leaq	1(%rax), %rdx
	movq	%rdx, 8(%rdi)
	movb	$83, (%rcx,%rax)
	movq	8(%rdi), %rax
	movq	16(%rdi), %rcx
	cmpq	%rcx, %rax
	jae	.LBB3_16
.LBB3_17:
	movq	(%rdi), %rcx
	leaq	1(%rax), %rdx
	movq	%rdx, 8(%rdi)
	movb	$65, (%rcx,%rax)
	movq	8(%rdi), %rax
	movq	16(%rdi), %rcx
	cmpq	%rcx, %rax
	jae	.LBB3_19
.LBB3_20:
	movq	(%rdi), %rcx
	leaq	1(%rax), %rdx
	movq	%rdx, 8(%rdi)
	movb	$86, (%rcx,%rax)
	movq	8(%rdi), %rax
	movq	16(%rdi), %rcx
	cmpq	%rcx, %rax
	jae	.LBB3_22
.LBB3_23:
	movq	(%rdi), %rcx
	leaq	1(%rax), %rdx
	movq	%rdx, 8(%rdi)
	movb	$65, (%rcx,%rax)
	movq	8(%rdi), %rax
	movq	16(%rdi), %rcx
	cmpq	%rcx, %rax
	jae	.LBB3_25
.LBB3_26:
	movq	(%rdi), %rcx
	leaq	1(%rax), %rdx
	movq	%rdx, 8(%rdi)
	movb	$87, (%rcx,%rax)
	movq	8(%rdi), %rax
	movq	16(%rdi), %rcx
	cmpq	%rcx, %rax
	jae	.LBB3_28
.LBB3_29:
	movq	(%rdi), %rcx
	leaq	1(%rax), %rdx
	movq	%rdx, 8(%rdi)
	movb	$72, (%rcx,%rax)
	movq	8(%rdi), %rax
	movq	16(%rdi), %rcx
	cmpq	%rcx, %rax
	jae	.LBB3_31
.LBB3_32:
	movq	(%rdi), %rcx
	leaq	1(%rax), %rdx
	movq	%rdx, 8(%rdi)
	movb	$-125, (%rcx,%rax)
	movq	8(%rdi), %rax
	movq	16(%rdi), %rcx
	cmpq	%rcx, %rax
	jae	.LBB3_34
.LBB3_35:
	movq	(%rdi), %rcx
	leaq	1(%rax), %rdx
	movq	%rdx, 8(%rdi)
	movb	$-20, (%rcx,%rax)
	movq	8(%rdi), %rax
	movq	16(%rdi), %rcx
	cmpq	%rcx, %rax
	jae	.LBB3_37
.LBB3_38:
	movq	(%rdi), %rcx
	leaq	1(%rax), %rdx
	movq	%rdx, 8(%rdi)
	movb	$8, (%rcx,%rax)
	movq	8(%rdi), %rax
	movq	16(%rdi), %rcx
	cmpq	%rcx, %rax
	jae	.LBB3_40
.LBB3_41:
	movq	(%rdi), %rcx
	leaq	1(%rax), %rdx
	movq	%rdx, 8(%rdi)
	movb	$72, (%rcx,%rax)
	movq	8(%rdi), %rax
	movq	16(%rdi), %rcx
	cmpq	%rcx, %rax
	jae	.LBB3_43
.LBB3_44:
	movq	(%rdi), %rcx
	leaq	1(%rax), %rdx
	movq	%rdx, 8(%rdi)
	movb	$-119, (%rcx,%rax)
	movq	8(%rdi), %rax
	movq	16(%rdi), %rcx
	cmpq	%rcx, %rax
	jae	.LBB3_46
.LBB3_47:
	movq	(%rdi), %rcx
	leaq	1(%rax), %rdx
	movq	%rdx, 8(%rdi)
	movb	$-5, (%rcx,%rax)
	movq	8(%rdi), %rax
	movq	16(%rdi), %rcx
	cmpq	%rcx, %rax
	jae	.LBB3_49
.LBB3_50:
	movq	(%rdi), %rcx
	leaq	1(%rax), %rdx
	movq	%rdx, 8(%rdi)
	movb	$73, (%rcx,%rax)
	movq	8(%rdi), %rax
	movq	16(%rdi), %rcx
	cmpq	%rcx, %rax
	jae	.LBB3_52
.LBB3_53:
	movq	(%rdi), %rcx
	leaq	1(%rax), %rdx
	movq	%rdx, 8(%rdi)
	movb	$-119, (%rcx,%rax)
	movq	8(%rdi), %rax
	movq	16(%rdi), %rcx
	cmpq	%rcx, %rax
	jae	.LBB3_55
.LBB3_56:
	movq	(%rdi), %rcx
	leaq	1(%rax), %rdx
	movq	%rdx, 8(%rdi)
	movb	$-10, (%rcx,%rax)
	movq	8(%rdi), %rax
	movq	16(%rdi), %rcx
	cmpq	%rcx, %rax
	jae	.LBB3_58
.LBB3_59:
	movq	(%rdi), %rcx
	leaq	1(%rax), %rdx
	movq	%rdx, 8(%rdi)
	movb	$73, (%rcx,%rax)
	movq	8(%rdi), %rax
	movq	16(%rdi), %rcx
	cmpq	%rcx, %rax
	jae	.LBB3_61
.LBB3_62:
	movq	(%rdi), %rcx
	leaq	1(%rax), %rdx
	movq	%rdx, 8(%rdi)
	movb	$-119, (%rcx,%rax)
	movq	8(%rdi), %rax
	movq	16(%rdi), %rcx
	cmpq	%rcx, %rax
	jae	.LBB3_64
.LBB3_65:
	movq	(%rdi), %rcx
	leaq	1(%rax), %rdx
	movq	%rdx, 8(%rdi)
	movb	$-41, (%rcx,%rax)
	retq
.Lfunc_end3:
	.size	emit_prologue, .Lfunc_end3-emit_prologue
                                        # -- End function
	.globl	emit_epilogue                   # -- Begin function emit_epilogue
	.p2align	4
	.type	emit_epilogue,@function
emit_epilogue:                          # @emit_epilogue
# %bb.0:
	movq	8(%rdi), %rax
	movq	16(%rdi), %rcx
	cmpq	%rcx, %rax
	jae	.LBB4_1
# %bb.2:
	movq	(%rdi), %rcx
	leaq	1(%rax), %rdx
	movq	%rdx, 8(%rdi)
	movb	$72, (%rcx,%rax)
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
	cmpq	%rcx, %rax
	jb	.LBB4_11
.LBB4_10:
	movl	$1, 24(%rdi)
	cmpq	%rcx, %rax
	jb	.LBB4_14
.LBB4_13:
	movl	$1, 24(%rdi)
	cmpq	%rcx, %rax
	jb	.LBB4_17
.LBB4_16:
	movl	$1, 24(%rdi)
	cmpq	%rcx, %rax
	jb	.LBB4_20
.LBB4_19:
	movl	$1, 24(%rdi)
	cmpq	%rcx, %rax
	jb	.LBB4_23
.LBB4_22:
	movl	$1, 24(%rdi)
	cmpq	%rcx, %rax
	jb	.LBB4_26
.LBB4_25:
	movl	$1, 24(%rdi)
	cmpq	%rcx, %rax
	jb	.LBB4_29
.LBB4_28:
	movl	$1, 24(%rdi)
	cmpq	%rcx, %rax
	jb	.LBB4_32
.LBB4_31:
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
	movb	$-125, (%rcx,%rax)
	movq	8(%rdi), %rax
	movq	16(%rdi), %rcx
	cmpq	%rcx, %rax
	jae	.LBB4_7
.LBB4_8:
	movq	(%rdi), %rcx
	leaq	1(%rax), %rdx
	movq	%rdx, 8(%rdi)
	movb	$-60, (%rcx,%rax)
	movq	8(%rdi), %rax
	movq	16(%rdi), %rcx
	cmpq	%rcx, %rax
	jae	.LBB4_10
.LBB4_11:
	movq	(%rdi), %rcx
	leaq	1(%rax), %rdx
	movq	%rdx, 8(%rdi)
	movb	$8, (%rcx,%rax)
	movq	8(%rdi), %rax
	movq	16(%rdi), %rcx
	cmpq	%rcx, %rax
	jae	.LBB4_13
.LBB4_14:
	movq	(%rdi), %rcx
	leaq	1(%rax), %rdx
	movq	%rdx, 8(%rdi)
	movb	$65, (%rcx,%rax)
	movq	8(%rdi), %rax
	movq	16(%rdi), %rcx
	cmpq	%rcx, %rax
	jae	.LBB4_16
.LBB4_17:
	movq	(%rdi), %rcx
	leaq	1(%rax), %rdx
	movq	%rdx, 8(%rdi)
	movb	$95, (%rcx,%rax)
	movq	8(%rdi), %rax
	movq	16(%rdi), %rcx
	cmpq	%rcx, %rax
	jae	.LBB4_19
.LBB4_20:
	movq	(%rdi), %rcx
	leaq	1(%rax), %rdx
	movq	%rdx, 8(%rdi)
	movb	$65, (%rcx,%rax)
	movq	8(%rdi), %rax
	movq	16(%rdi), %rcx
	cmpq	%rcx, %rax
	jae	.LBB4_22
.LBB4_23:
	movq	(%rdi), %rcx
	leaq	1(%rax), %rdx
	movq	%rdx, 8(%rdi)
	movb	$94, (%rcx,%rax)
	movq	8(%rdi), %rax
	movq	16(%rdi), %rcx
	cmpq	%rcx, %rax
	jae	.LBB4_25
.LBB4_26:
	movq	(%rdi), %rcx
	leaq	1(%rax), %rdx
	movq	%rdx, 8(%rdi)
	movb	$91, (%rcx,%rax)
	movq	8(%rdi), %rax
	movq	16(%rdi), %rcx
	cmpq	%rcx, %rax
	jae	.LBB4_28
.LBB4_29:
	movq	(%rdi), %rcx
	leaq	1(%rax), %rdx
	movq	%rdx, 8(%rdi)
	movb	$93, (%rcx,%rax)
	movq	8(%rdi), %rax
	movq	16(%rdi), %rcx
	cmpq	%rcx, %rax
	jae	.LBB4_31
.LBB4_32:
	movq	(%rdi), %rcx
	leaq	1(%rax), %rdx
	movq	%rdx, 8(%rdi)
	movb	$-61, (%rcx,%rax)
	retq
.Lfunc_end4:
	.size	emit_epilogue, .Lfunc_end4-emit_epilogue
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
	jae	.LBB5_1
# %bb.2:
	movq	(%rdi), %rdx
	leaq	1(%rcx), %rsi
	movq	%rsi, 8(%rdi)
	movb	$72, (%rdx,%rcx)
	movq	8(%rdi), %rcx
	movq	16(%rdi), %rdx
	cmpq	%rdx, %rcx
	jb	.LBB5_5
.LBB5_4:
	movl	$1, 24(%rdi)
	cmpq	%rdx, %rcx
	jb	.LBB5_8
.LBB5_7:
	movl	$1, 24(%rdi)
	cmpq	%rdx, %rcx
	jb	.LBB5_11
.LBB5_10:
	movl	$1, 24(%rdi)
	cmpq	%rdx, %rcx
	jb	.LBB5_14
.LBB5_13:
	movl	$1, 24(%rdi)
	cmpq	%rdx, %rcx
	jb	.LBB5_17
.LBB5_16:
	movl	$1, 24(%rdi)
	cmpq	%rdx, %rcx
	jb	.LBB5_20
.LBB5_19:
	movl	$1, 24(%rdi)
	retq
.LBB5_1:
	movl	$1, 24(%rdi)
	cmpq	%rdx, %rcx
	jae	.LBB5_4
.LBB5_5:
	movq	(%rdi), %rdx
	leaq	1(%rcx), %rsi
	movq	%rsi, 8(%rdi)
	movb	$-127, (%rdx,%rcx)
	movq	8(%rdi), %rcx
	movq	16(%rdi), %rdx
	cmpq	%rdx, %rcx
	jae	.LBB5_7
.LBB5_8:
	movq	(%rdi), %rdx
	leaq	1(%rcx), %rsi
	movq	%rsi, 8(%rdi)
	movb	$-61, (%rdx,%rcx)
	movq	8(%rdi), %rcx
	movq	16(%rdi), %rdx
	cmpq	%rdx, %rcx
	jae	.LBB5_10
.LBB5_11:
	movq	(%rdi), %rdx
	leaq	1(%rcx), %rsi
	movq	%rsi, 8(%rdi)
	movb	%al, (%rdx,%rcx)
	movq	8(%rdi), %rcx
	movq	16(%rdi), %rdx
	cmpq	%rdx, %rcx
	jae	.LBB5_13
.LBB5_14:
	movq	(%rdi), %rdx
	leaq	1(%rcx), %rsi
	movq	%rsi, 8(%rdi)
	movb	%ah, (%rdx,%rcx)
	movq	8(%rdi), %rcx
	movq	16(%rdi), %rdx
	cmpq	%rdx, %rcx
	jae	.LBB5_16
.LBB5_17:
	movl	%eax, %edx
	shrl	$16, %edx
	movq	(%rdi), %rsi
	leaq	1(%rcx), %r8
	movq	%r8, 8(%rdi)
	movb	%dl, (%rsi,%rcx)
	movq	8(%rdi), %rcx
	movq	16(%rdi), %rdx
	cmpq	%rdx, %rcx
	jae	.LBB5_19
.LBB5_20:
	shrl	$24, %eax
	movq	(%rdi), %rdx
	leaq	1(%rcx), %rsi
	movq	%rsi, 8(%rdi)
	movb	%al, (%rdx,%rcx)
	retq
.Lfunc_end5:
	.size	emit_add_ptr, .Lfunc_end5-emit_add_ptr
                                        # -- End function
	.globl	emit_add_cell                   # -- Begin function emit_add_cell
	.p2align	4
	.type	emit_add_cell,@function
emit_add_cell:                          # @emit_add_cell
# %bb.0:
	movq	8(%rdi), %rax
	movq	16(%rdi), %rcx
	cmpq	%rcx, %rax
	jae	.LBB6_1
# %bb.2:
	movq	(%rdi), %rcx
	leaq	1(%rax), %rdx
	movq	%rdx, 8(%rdi)
	movb	$-128, (%rcx,%rax)
	movq	8(%rdi), %rax
	movq	16(%rdi), %rcx
	cmpq	%rcx, %rax
	jb	.LBB6_5
.LBB6_4:
	movl	$1, 24(%rdi)
	cmpq	%rcx, %rax
	jb	.LBB6_8
.LBB6_7:
	movl	$1, 24(%rdi)
	retq
.LBB6_1:
	movl	$1, 24(%rdi)
	cmpq	%rcx, %rax
	jae	.LBB6_4
.LBB6_5:
	movq	(%rdi), %rcx
	leaq	1(%rax), %rdx
	movq	%rdx, 8(%rdi)
	movb	$3, (%rcx,%rax)
	movq	8(%rdi), %rax
	movq	16(%rdi), %rcx
	cmpq	%rcx, %rax
	jae	.LBB6_7
.LBB6_8:
	movq	(%rdi), %rcx
	leaq	1(%rax), %rdx
	movq	%rdx, 8(%rdi)
	movb	%sil, (%rcx,%rax)
	retq
.Lfunc_end6:
	.size	emit_add_cell, .Lfunc_end6-emit_add_cell
                                        # -- End function
	.globl	emit_set_cell                   # -- Begin function emit_set_cell
	.p2align	4
	.type	emit_set_cell,@function
emit_set_cell:                          # @emit_set_cell
# %bb.0:
	movq	8(%rdi), %rax
	movq	16(%rdi), %rcx
	cmpq	%rcx, %rax
	jae	.LBB7_1
# %bb.2:
	movq	(%rdi), %rcx
	leaq	1(%rax), %rdx
	movq	%rdx, 8(%rdi)
	movb	$-58, (%rcx,%rax)
	movq	8(%rdi), %rax
	movq	16(%rdi), %rcx
	cmpq	%rcx, %rax
	jb	.LBB7_5
.LBB7_4:
	movl	$1, 24(%rdi)
	cmpq	%rcx, %rax
	jb	.LBB7_8
.LBB7_7:
	movl	$1, 24(%rdi)
	retq
.LBB7_1:
	movl	$1, 24(%rdi)
	cmpq	%rcx, %rax
	jae	.LBB7_4
.LBB7_5:
	movq	(%rdi), %rcx
	leaq	1(%rax), %rdx
	movq	%rdx, 8(%rdi)
	movb	$3, (%rcx,%rax)
	movq	8(%rdi), %rax
	movq	16(%rdi), %rcx
	cmpq	%rcx, %rax
	jae	.LBB7_7
.LBB7_8:
	movq	(%rdi), %rcx
	leaq	1(%rax), %rdx
	movq	%rdx, 8(%rdi)
	movb	%sil, (%rcx,%rax)
	retq
.Lfunc_end7:
	.size	emit_set_cell, .Lfunc_end7-emit_set_cell
                                        # -- End function
	.globl	emit_output                     # -- Begin function emit_output
	.p2align	4
	.type	emit_output,@function
emit_output:                            # @emit_output
# %bb.0:
	movq	8(%rdi), %rax
	movq	16(%rdi), %rcx
	cmpq	%rcx, %rax
	jae	.LBB8_1
# %bb.2:
	movq	(%rdi), %rcx
	leaq	1(%rax), %rdx
	movq	%rdx, 8(%rdi)
	movb	$15, (%rcx,%rax)
	movq	8(%rdi), %rax
	movq	16(%rdi), %rcx
	cmpq	%rcx, %rax
	jb	.LBB8_5
.LBB8_4:
	movl	$1, 24(%rdi)
	cmpq	%rcx, %rax
	jb	.LBB8_8
.LBB8_7:
	movl	$1, 24(%rdi)
	cmpq	%rcx, %rax
	jb	.LBB8_11
.LBB8_10:
	movl	$1, 24(%rdi)
	cmpq	%rcx, %rax
	jb	.LBB8_14
.LBB8_13:
	movl	$1, 24(%rdi)
	cmpq	%rcx, %rax
	jb	.LBB8_17
.LBB8_16:
	movl	$1, 24(%rdi)
	retq
.LBB8_1:
	movl	$1, 24(%rdi)
	cmpq	%rcx, %rax
	jae	.LBB8_4
.LBB8_5:
	movq	(%rdi), %rcx
	leaq	1(%rax), %rdx
	movq	%rdx, 8(%rdi)
	movb	$-74, (%rcx,%rax)
	movq	8(%rdi), %rax
	movq	16(%rdi), %rcx
	cmpq	%rcx, %rax
	jae	.LBB8_7
.LBB8_8:
	movq	(%rdi), %rcx
	leaq	1(%rax), %rdx
	movq	%rdx, 8(%rdi)
	movb	$59, (%rcx,%rax)
	movq	8(%rdi), %rax
	movq	16(%rdi), %rcx
	cmpq	%rcx, %rax
	jae	.LBB8_10
.LBB8_11:
	movq	(%rdi), %rcx
	leaq	1(%rax), %rdx
	movq	%rdx, 8(%rdi)
	movb	$65, (%rcx,%rax)
	movq	8(%rdi), %rax
	movq	16(%rdi), %rcx
	cmpq	%rcx, %rax
	jae	.LBB8_13
.LBB8_14:
	movq	(%rdi), %rcx
	leaq	1(%rax), %rdx
	movq	%rdx, 8(%rdi)
	movb	$-1, (%rcx,%rax)
	movq	8(%rdi), %rax
	movq	16(%rdi), %rcx
	cmpq	%rcx, %rax
	jae	.LBB8_16
.LBB8_17:
	movq	(%rdi), %rcx
	leaq	1(%rax), %rdx
	movq	%rdx, 8(%rdi)
	movb	$-41, (%rcx,%rax)
	retq
.Lfunc_end8:
	.size	emit_output, .Lfunc_end8-emit_output
                                        # -- End function
	.globl	emit_input                      # -- Begin function emit_input
	.p2align	4
	.type	emit_input,@function
emit_input:                             # @emit_input
# %bb.0:
	movq	8(%rdi), %rax
	movq	16(%rdi), %rcx
	cmpq	%rcx, %rax
	jae	.LBB9_1
# %bb.2:
	movq	(%rdi), %rcx
	leaq	1(%rax), %rdx
	movq	%rdx, 8(%rdi)
	movb	$65, (%rcx,%rax)
	movq	8(%rdi), %rax
	movq	16(%rdi), %rcx
	cmpq	%rcx, %rax
	jb	.LBB9_5
.LBB9_4:
	movl	$1, 24(%rdi)
	cmpq	%rcx, %rax
	jb	.LBB9_8
.LBB9_7:
	movl	$1, 24(%rdi)
	cmpq	%rcx, %rax
	jb	.LBB9_11
.LBB9_10:
	movl	$1, 24(%rdi)
	cmpq	%rcx, %rax
	jb	.LBB9_14
.LBB9_13:
	movl	$1, 24(%rdi)
	retq
.LBB9_1:
	movl	$1, 24(%rdi)
	cmpq	%rcx, %rax
	jae	.LBB9_4
.LBB9_5:
	movq	(%rdi), %rcx
	leaq	1(%rax), %rdx
	movq	%rdx, 8(%rdi)
	movb	$-1, (%rcx,%rax)
	movq	8(%rdi), %rax
	movq	16(%rdi), %rcx
	cmpq	%rcx, %rax
	jae	.LBB9_7
.LBB9_8:
	movq	(%rdi), %rcx
	leaq	1(%rax), %rdx
	movq	%rdx, 8(%rdi)
	movb	$-42, (%rcx,%rax)
	movq	8(%rdi), %rax
	movq	16(%rdi), %rcx
	cmpq	%rcx, %rax
	jae	.LBB9_10
.LBB9_11:
	movq	(%rdi), %rcx
	leaq	1(%rax), %rdx
	movq	%rdx, 8(%rdi)
	movb	$-120, (%rcx,%rax)
	movq	8(%rdi), %rax
	movq	16(%rdi), %rcx
	cmpq	%rcx, %rax
	jae	.LBB9_13
.LBB9_14:
	movq	(%rdi), %rcx
	leaq	1(%rax), %rdx
	movq	%rdx, 8(%rdi)
	movb	$3, (%rcx,%rax)
	retq
.Lfunc_end9:
	.size	emit_input, .Lfunc_end9-emit_input
                                        # -- End function
	.globl	emit_loop_open                  # -- Begin function emit_loop_open
	.p2align	4
	.type	emit_loop_open,@function
emit_loop_open:                         # @emit_loop_open
# %bb.0:
	movq	8(%rdi), %rax
	movq	16(%rdi), %rcx
	cmpq	%rcx, %rax
	jae	.LBB10_1
# %bb.2:
	movq	(%rdi), %rcx
	leaq	1(%rax), %rdx
	movq	%rdx, 8(%rdi)
	movb	$-128, (%rcx,%rax)
	movq	8(%rdi), %rax
	movq	16(%rdi), %rcx
	cmpq	%rcx, %rax
	jb	.LBB10_5
.LBB10_4:
	movl	$1, 24(%rdi)
	cmpq	%rcx, %rax
	jb	.LBB10_8
.LBB10_7:
	movl	$1, 24(%rdi)
	cmpq	%rcx, %rax
	jb	.LBB10_11
.LBB10_10:
	movl	$1, 24(%rdi)
	cmpq	%rcx, %rax
	jb	.LBB10_14
.LBB10_13:
	movl	$1, 24(%rdi)
	cmpq	%rcx, %rax
	jb	.LBB10_17
.LBB10_16:
	movl	$1, 24(%rdi)
	movq	%rax, %rdx
	cmpq	%rcx, %rdx
	jb	.LBB10_20
.LBB10_19:
	movl	$1, 24(%rdi)
	cmpq	%rcx, %rdx
	jb	.LBB10_23
.LBB10_22:
	movl	$1, 24(%rdi)
	cmpq	%rcx, %rdx
	jb	.LBB10_26
.LBB10_25:
	movl	$1, 24(%rdi)
	retq
.LBB10_1:
	movl	$1, 24(%rdi)
	cmpq	%rcx, %rax
	jae	.LBB10_4
.LBB10_5:
	movq	(%rdi), %rcx
	leaq	1(%rax), %rdx
	movq	%rdx, 8(%rdi)
	movb	$59, (%rcx,%rax)
	movq	8(%rdi), %rax
	movq	16(%rdi), %rcx
	cmpq	%rcx, %rax
	jae	.LBB10_7
.LBB10_8:
	movq	(%rdi), %rcx
	leaq	1(%rax), %rdx
	movq	%rdx, 8(%rdi)
	movb	$0, (%rcx,%rax)
	movq	8(%rdi), %rax
	movq	16(%rdi), %rcx
	cmpq	%rcx, %rax
	jae	.LBB10_10
.LBB10_11:
	movq	(%rdi), %rcx
	leaq	1(%rax), %rdx
	movq	%rdx, 8(%rdi)
	movb	$15, (%rcx,%rax)
	movq	8(%rdi), %rax
	movq	16(%rdi), %rcx
	cmpq	%rcx, %rax
	jae	.LBB10_13
.LBB10_14:
	movq	(%rdi), %rcx
	leaq	1(%rax), %rdx
	movq	%rdx, 8(%rdi)
	movb	$-124, (%rcx,%rax)
	movq	8(%rdi), %rax
	movq	16(%rdi), %rcx
	cmpq	%rcx, %rax
	jae	.LBB10_16
.LBB10_17:
	movq	(%rdi), %rcx
	leaq	1(%rax), %rdx
	movq	%rdx, 8(%rdi)
	movb	$0, (%rcx,%rax)
	movq	8(%rdi), %rdx
	movq	16(%rdi), %rcx
	cmpq	%rcx, %rdx
	jae	.LBB10_19
.LBB10_20:
	movq	(%rdi), %rcx
	leaq	1(%rdx), %rsi
	movq	%rsi, 8(%rdi)
	movb	$0, (%rcx,%rdx)
	movq	8(%rdi), %rdx
	movq	16(%rdi), %rcx
	cmpq	%rcx, %rdx
	jae	.LBB10_22
.LBB10_23:
	movq	(%rdi), %rcx
	leaq	1(%rdx), %rsi
	movq	%rsi, 8(%rdi)
	movb	$0, (%rcx,%rdx)
	movq	8(%rdi), %rdx
	movq	16(%rdi), %rcx
	cmpq	%rcx, %rdx
	jae	.LBB10_25
.LBB10_26:
	movq	(%rdi), %rcx
	leaq	1(%rdx), %rsi
	movq	%rsi, 8(%rdi)
	movb	$0, (%rcx,%rdx)
	retq
.Lfunc_end10:
	.size	emit_loop_open, .Lfunc_end10-emit_loop_open
                                        # -- End function
	.globl	emit_loop_close                 # -- Begin function emit_loop_close
	.p2align	4
	.type	emit_loop_close,@function
emit_loop_close:                        # @emit_loop_close
# %bb.0:
	movq	8(%rdi), %rax
	movq	16(%rdi), %rcx
	cmpq	%rcx, %rax
	jae	.LBB11_1
# %bb.2:
	movq	(%rdi), %rcx
	leaq	1(%rax), %rdx
	movq	%rdx, 8(%rdi)
	movb	$-128, (%rcx,%rax)
	movq	8(%rdi), %rax
	movq	16(%rdi), %rcx
	cmpq	%rcx, %rax
	jb	.LBB11_5
.LBB11_4:
	movl	$1, 24(%rdi)
	cmpq	%rcx, %rax
	jb	.LBB11_8
.LBB11_7:
	movl	$1, 24(%rdi)
	cmpq	%rcx, %rax
	jb	.LBB11_11
.LBB11_10:
	movl	$1, 24(%rdi)
	cmpq	%rcx, %rax
	jb	.LBB11_14
.LBB11_13:
	movl	$1, 24(%rdi)
	cmpq	%rcx, %rax
	jb	.LBB11_17
.LBB11_16:
	movl	$1, 24(%rdi)
	movq	%rax, %rdx
	cmpq	%rcx, %rdx
	jb	.LBB11_20
.LBB11_19:
	movl	$1, 24(%rdi)
	cmpq	%rcx, %rdx
	jb	.LBB11_23
.LBB11_22:
	movl	$1, 24(%rdi)
	cmpq	%rcx, %rdx
	jb	.LBB11_26
.LBB11_25:
	movl	$1, 24(%rdi)
	retq
.LBB11_1:
	movl	$1, 24(%rdi)
	cmpq	%rcx, %rax
	jae	.LBB11_4
.LBB11_5:
	movq	(%rdi), %rcx
	leaq	1(%rax), %rdx
	movq	%rdx, 8(%rdi)
	movb	$59, (%rcx,%rax)
	movq	8(%rdi), %rax
	movq	16(%rdi), %rcx
	cmpq	%rcx, %rax
	jae	.LBB11_7
.LBB11_8:
	movq	(%rdi), %rcx
	leaq	1(%rax), %rdx
	movq	%rdx, 8(%rdi)
	movb	$0, (%rcx,%rax)
	movq	8(%rdi), %rax
	movq	16(%rdi), %rcx
	cmpq	%rcx, %rax
	jae	.LBB11_10
.LBB11_11:
	movq	(%rdi), %rcx
	leaq	1(%rax), %rdx
	movq	%rdx, 8(%rdi)
	movb	$15, (%rcx,%rax)
	movq	8(%rdi), %rax
	movq	16(%rdi), %rcx
	cmpq	%rcx, %rax
	jae	.LBB11_13
.LBB11_14:
	movq	(%rdi), %rcx
	leaq	1(%rax), %rdx
	movq	%rdx, 8(%rdi)
	movb	$-123, (%rcx,%rax)
	movq	8(%rdi), %rax
	movq	16(%rdi), %rcx
	cmpq	%rcx, %rax
	jae	.LBB11_16
.LBB11_17:
	movq	(%rdi), %rcx
	leaq	1(%rax), %rdx
	movq	%rdx, 8(%rdi)
	movb	$0, (%rcx,%rax)
	movq	8(%rdi), %rdx
	movq	16(%rdi), %rcx
	cmpq	%rcx, %rdx
	jae	.LBB11_19
.LBB11_20:
	movq	(%rdi), %rcx
	leaq	1(%rdx), %rsi
	movq	%rsi, 8(%rdi)
	movb	$0, (%rcx,%rdx)
	movq	8(%rdi), %rdx
	movq	16(%rdi), %rcx
	cmpq	%rcx, %rdx
	jae	.LBB11_22
.LBB11_23:
	movq	(%rdi), %rcx
	leaq	1(%rdx), %rsi
	movq	%rsi, 8(%rdi)
	movb	$0, (%rcx,%rdx)
	movq	8(%rdi), %rdx
	movq	16(%rdi), %rcx
	cmpq	%rcx, %rdx
	jae	.LBB11_25
.LBB11_26:
	movq	(%rdi), %rcx
	leaq	1(%rdx), %rsi
	movq	%rsi, 8(%rdi)
	movb	$0, (%rcx,%rdx)
	retq
.Lfunc_end11:
	.size	emit_loop_close, .Lfunc_end11-emit_loop_close
                                        # -- End function
	.globl	patch_rel32                     # -- Begin function patch_rel32
	.p2align	4
	.type	patch_rel32,@function
patch_rel32:                            # @patch_rel32
# %bb.0:
	leaq	4(%rsi), %rax
	cmpq	16(%rdi), %rax
	jbe	.LBB12_3
# %bb.1:
	movl	$1, 24(%rdi)
	retq
.LBB12_3:
	subl	%eax, %edx
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
.Lfunc_end12:
	.size	patch_rel32, .Lfunc_end12-patch_rel32
                                        # -- End function
	.ident	"clang version 20.1.8"
	.section	".note.GNU-stack","",@progbits
	.addrsig
