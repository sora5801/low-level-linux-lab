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
	.globl	emit_prologue                   # -- Begin function emit_prologue
	.p2align	4
	.type	emit_prologue,@function
emit_prologue:                          # @emit_prologue
# %bb.0:
	pushq	%rbp
	movq	%rsp, %rbp
	movq	8(%rdi), %rax
	cmpq	16(%rdi), %rax
	jae	.LBB3_1
# %bb.2:
	movq	(%rdi), %rcx
	leaq	1(%rax), %rdx
	movq	%rdx, 8(%rdi)
	movb	$85, (%rcx,%rax)
	jmp	.LBB3_3
.LBB3_1:
	movl	$1, 24(%rdi)
.LBB3_3:
	movq	8(%rdi), %rax
	cmpq	16(%rdi), %rax
	jae	.LBB3_4
# %bb.5:
	movq	(%rdi), %rcx
	leaq	1(%rax), %rdx
	movq	%rdx, 8(%rdi)
	movb	$72, (%rcx,%rax)
	jmp	.LBB3_6
.LBB3_4:
	movl	$1, 24(%rdi)
.LBB3_6:
	movq	8(%rdi), %rax
	cmpq	16(%rdi), %rax
	jae	.LBB3_7
# %bb.8:
	movq	(%rdi), %rcx
	leaq	1(%rax), %rdx
	movq	%rdx, 8(%rdi)
	movb	$-119, (%rcx,%rax)
	jmp	.LBB3_9
.LBB3_7:
	movl	$1, 24(%rdi)
.LBB3_9:
	movq	8(%rdi), %rax
	cmpq	16(%rdi), %rax
	jae	.LBB3_10
# %bb.11:
	movq	(%rdi), %rcx
	leaq	1(%rax), %rdx
	movq	%rdx, 8(%rdi)
	movb	$-27, (%rcx,%rax)
	jmp	.LBB3_12
.LBB3_10:
	movl	$1, 24(%rdi)
.LBB3_12:
	movq	8(%rdi), %rax
	cmpq	16(%rdi), %rax
	jae	.LBB3_13
# %bb.14:
	movq	(%rdi), %rcx
	leaq	1(%rax), %rdx
	movq	%rdx, 8(%rdi)
	movb	$83, (%rcx,%rax)
	jmp	.LBB3_15
.LBB3_13:
	movl	$1, 24(%rdi)
.LBB3_15:
	movq	8(%rdi), %rax
	cmpq	16(%rdi), %rax
	jae	.LBB3_16
# %bb.17:
	movq	(%rdi), %rcx
	leaq	1(%rax), %rdx
	movq	%rdx, 8(%rdi)
	movb	$65, (%rcx,%rax)
	jmp	.LBB3_18
.LBB3_16:
	movl	$1, 24(%rdi)
.LBB3_18:
	movq	8(%rdi), %rax
	cmpq	16(%rdi), %rax
	jae	.LBB3_19
# %bb.20:
	movq	(%rdi), %rcx
	leaq	1(%rax), %rdx
	movq	%rdx, 8(%rdi)
	movb	$86, (%rcx,%rax)
	jmp	.LBB3_21
.LBB3_19:
	movl	$1, 24(%rdi)
.LBB3_21:
	movq	8(%rdi), %rax
	cmpq	16(%rdi), %rax
	jae	.LBB3_22
# %bb.23:
	movq	(%rdi), %rcx
	leaq	1(%rax), %rdx
	movq	%rdx, 8(%rdi)
	movb	$65, (%rcx,%rax)
	jmp	.LBB3_24
.LBB3_22:
	movl	$1, 24(%rdi)
.LBB3_24:
	movq	8(%rdi), %rax
	cmpq	16(%rdi), %rax
	jae	.LBB3_25
# %bb.26:
	movq	(%rdi), %rcx
	leaq	1(%rax), %rdx
	movq	%rdx, 8(%rdi)
	movb	$87, (%rcx,%rax)
	jmp	.LBB3_27
.LBB3_25:
	movl	$1, 24(%rdi)
.LBB3_27:
	movq	8(%rdi), %rax
	cmpq	16(%rdi), %rax
	jae	.LBB3_28
# %bb.29:
	movq	(%rdi), %rcx
	leaq	1(%rax), %rdx
	movq	%rdx, 8(%rdi)
	movb	$72, (%rcx,%rax)
	jmp	.LBB3_30
.LBB3_28:
	movl	$1, 24(%rdi)
.LBB3_30:
	movq	8(%rdi), %rax
	cmpq	16(%rdi), %rax
	jae	.LBB3_31
# %bb.32:
	movq	(%rdi), %rcx
	leaq	1(%rax), %rdx
	movq	%rdx, 8(%rdi)
	movb	$-125, (%rcx,%rax)
	jmp	.LBB3_33
.LBB3_31:
	movl	$1, 24(%rdi)
.LBB3_33:
	movq	8(%rdi), %rax
	cmpq	16(%rdi), %rax
	jae	.LBB3_34
# %bb.35:
	movq	(%rdi), %rcx
	leaq	1(%rax), %rdx
	movq	%rdx, 8(%rdi)
	movb	$-20, (%rcx,%rax)
	jmp	.LBB3_36
.LBB3_34:
	movl	$1, 24(%rdi)
.LBB3_36:
	movq	8(%rdi), %rax
	cmpq	16(%rdi), %rax
	jae	.LBB3_37
# %bb.38:
	movq	(%rdi), %rcx
	leaq	1(%rax), %rdx
	movq	%rdx, 8(%rdi)
	movb	$8, (%rcx,%rax)
	jmp	.LBB3_39
.LBB3_37:
	movl	$1, 24(%rdi)
.LBB3_39:
	movq	8(%rdi), %rax
	cmpq	16(%rdi), %rax
	jae	.LBB3_40
# %bb.41:
	movq	(%rdi), %rcx
	leaq	1(%rax), %rdx
	movq	%rdx, 8(%rdi)
	movb	$72, (%rcx,%rax)
	jmp	.LBB3_42
.LBB3_40:
	movl	$1, 24(%rdi)
.LBB3_42:
	movq	8(%rdi), %rax
	cmpq	16(%rdi), %rax
	jae	.LBB3_43
# %bb.44:
	movq	(%rdi), %rcx
	leaq	1(%rax), %rdx
	movq	%rdx, 8(%rdi)
	movb	$-119, (%rcx,%rax)
	jmp	.LBB3_45
.LBB3_43:
	movl	$1, 24(%rdi)
.LBB3_45:
	movq	8(%rdi), %rax
	cmpq	16(%rdi), %rax
	jae	.LBB3_46
# %bb.47:
	movq	(%rdi), %rcx
	leaq	1(%rax), %rdx
	movq	%rdx, 8(%rdi)
	movb	$-5, (%rcx,%rax)
	jmp	.LBB3_48
.LBB3_46:
	movl	$1, 24(%rdi)
.LBB3_48:
	movq	8(%rdi), %rax
	cmpq	16(%rdi), %rax
	jae	.LBB3_49
# %bb.50:
	movq	(%rdi), %rcx
	leaq	1(%rax), %rdx
	movq	%rdx, 8(%rdi)
	movb	$73, (%rcx,%rax)
	jmp	.LBB3_51
.LBB3_49:
	movl	$1, 24(%rdi)
.LBB3_51:
	movq	8(%rdi), %rax
	cmpq	16(%rdi), %rax
	jae	.LBB3_52
# %bb.53:
	movq	(%rdi), %rcx
	leaq	1(%rax), %rdx
	movq	%rdx, 8(%rdi)
	movb	$-119, (%rcx,%rax)
	jmp	.LBB3_54
.LBB3_52:
	movl	$1, 24(%rdi)
.LBB3_54:
	movq	8(%rdi), %rax
	cmpq	16(%rdi), %rax
	jae	.LBB3_55
# %bb.56:
	movq	(%rdi), %rcx
	leaq	1(%rax), %rdx
	movq	%rdx, 8(%rdi)
	movb	$-10, (%rcx,%rax)
	jmp	.LBB3_57
.LBB3_55:
	movl	$1, 24(%rdi)
.LBB3_57:
	movq	8(%rdi), %rax
	cmpq	16(%rdi), %rax
	jae	.LBB3_58
# %bb.59:
	movq	(%rdi), %rcx
	leaq	1(%rax), %rdx
	movq	%rdx, 8(%rdi)
	movb	$73, (%rcx,%rax)
	jmp	.LBB3_60
.LBB3_58:
	movl	$1, 24(%rdi)
.LBB3_60:
	movq	8(%rdi), %rax
	cmpq	16(%rdi), %rax
	jae	.LBB3_61
# %bb.62:
	movq	(%rdi), %rcx
	leaq	1(%rax), %rdx
	movq	%rdx, 8(%rdi)
	movb	$-119, (%rcx,%rax)
	jmp	.LBB3_63
.LBB3_61:
	movl	$1, 24(%rdi)
.LBB3_63:
	movq	8(%rdi), %rax
	cmpq	16(%rdi), %rax
	jae	.LBB3_64
# %bb.65:
	movq	(%rdi), %rcx
	leaq	1(%rax), %rdx
	movq	%rdx, 8(%rdi)
	movb	$-41, (%rcx,%rax)
	popq	%rbp
	retq
.LBB3_64:
	movl	$1, 24(%rdi)
	popq	%rbp
	retq
.Lfunc_end3:
	.size	emit_prologue, .Lfunc_end3-emit_prologue
                                        # -- End function
	.globl	emit_epilogue                   # -- Begin function emit_epilogue
	.p2align	4
	.type	emit_epilogue,@function
emit_epilogue:                          # @emit_epilogue
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
	movb	$72, (%rcx,%rax)
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
	movb	$-125, (%rcx,%rax)
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
	movb	$-60, (%rcx,%rax)
	jmp	.LBB4_9
.LBB4_7:
	movl	$1, 24(%rdi)
.LBB4_9:
	movq	8(%rdi), %rax
	cmpq	16(%rdi), %rax
	jae	.LBB4_10
# %bb.11:
	movq	(%rdi), %rcx
	leaq	1(%rax), %rdx
	movq	%rdx, 8(%rdi)
	movb	$8, (%rcx,%rax)
	jmp	.LBB4_12
.LBB4_10:
	movl	$1, 24(%rdi)
.LBB4_12:
	movq	8(%rdi), %rax
	cmpq	16(%rdi), %rax
	jae	.LBB4_13
# %bb.14:
	movq	(%rdi), %rcx
	leaq	1(%rax), %rdx
	movq	%rdx, 8(%rdi)
	movb	$65, (%rcx,%rax)
	jmp	.LBB4_15
.LBB4_13:
	movl	$1, 24(%rdi)
.LBB4_15:
	movq	8(%rdi), %rax
	cmpq	16(%rdi), %rax
	jae	.LBB4_16
# %bb.17:
	movq	(%rdi), %rcx
	leaq	1(%rax), %rdx
	movq	%rdx, 8(%rdi)
	movb	$95, (%rcx,%rax)
	jmp	.LBB4_18
.LBB4_16:
	movl	$1, 24(%rdi)
.LBB4_18:
	movq	8(%rdi), %rax
	cmpq	16(%rdi), %rax
	jae	.LBB4_19
# %bb.20:
	movq	(%rdi), %rcx
	leaq	1(%rax), %rdx
	movq	%rdx, 8(%rdi)
	movb	$65, (%rcx,%rax)
	jmp	.LBB4_21
.LBB4_19:
	movl	$1, 24(%rdi)
.LBB4_21:
	movq	8(%rdi), %rax
	cmpq	16(%rdi), %rax
	jae	.LBB4_22
# %bb.23:
	movq	(%rdi), %rcx
	leaq	1(%rax), %rdx
	movq	%rdx, 8(%rdi)
	movb	$94, (%rcx,%rax)
	jmp	.LBB4_24
.LBB4_22:
	movl	$1, 24(%rdi)
.LBB4_24:
	movq	8(%rdi), %rax
	cmpq	16(%rdi), %rax
	jae	.LBB4_25
# %bb.26:
	movq	(%rdi), %rcx
	leaq	1(%rax), %rdx
	movq	%rdx, 8(%rdi)
	movb	$91, (%rcx,%rax)
	jmp	.LBB4_27
.LBB4_25:
	movl	$1, 24(%rdi)
.LBB4_27:
	movq	8(%rdi), %rax
	cmpq	16(%rdi), %rax
	jae	.LBB4_28
# %bb.29:
	movq	(%rdi), %rcx
	leaq	1(%rax), %rdx
	movq	%rdx, 8(%rdi)
	movb	$93, (%rcx,%rax)
	jmp	.LBB4_30
.LBB4_28:
	movl	$1, 24(%rdi)
.LBB4_30:
	movq	8(%rdi), %rax
	cmpq	16(%rdi), %rax
	jae	.LBB4_31
# %bb.32:
	movq	(%rdi), %rcx
	leaq	1(%rax), %rdx
	movq	%rdx, 8(%rdi)
	movb	$-61, (%rcx,%rax)
	popq	%rbp
	retq
.LBB4_31:
	movl	$1, 24(%rdi)
	popq	%rbp
	retq
.Lfunc_end4:
	.size	emit_epilogue, .Lfunc_end4-emit_epilogue
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
	jae	.LBB5_1
# %bb.2:
	movq	(%rdi), %rdx
	leaq	1(%rcx), %rsi
	movq	%rsi, 8(%rdi)
	movb	$72, (%rdx,%rcx)
	jmp	.LBB5_3
.LBB5_1:
	movl	$1, 24(%rdi)
.LBB5_3:
	movq	8(%rdi), %rcx
	cmpq	16(%rdi), %rcx
	jae	.LBB5_4
# %bb.5:
	movq	(%rdi), %rdx
	leaq	1(%rcx), %rsi
	movq	%rsi, 8(%rdi)
	movb	$-127, (%rdx,%rcx)
	jmp	.LBB5_6
.LBB5_4:
	movl	$1, 24(%rdi)
.LBB5_6:
	movq	8(%rdi), %rcx
	cmpq	16(%rdi), %rcx
	jae	.LBB5_7
# %bb.8:
	movq	(%rdi), %rdx
	leaq	1(%rcx), %rsi
	movq	%rsi, 8(%rdi)
	movb	$-61, (%rdx,%rcx)
	jmp	.LBB5_9
.LBB5_7:
	movl	$1, 24(%rdi)
.LBB5_9:
	movq	8(%rdi), %rcx
	cmpq	16(%rdi), %rcx
	jae	.LBB5_10
# %bb.11:
	movq	(%rdi), %rdx
	leaq	1(%rcx), %rsi
	movq	%rsi, 8(%rdi)
	movb	%al, (%rdx,%rcx)
	jmp	.LBB5_12
.LBB5_10:
	movl	$1, 24(%rdi)
.LBB5_12:
	movq	8(%rdi), %rcx
	cmpq	16(%rdi), %rcx
	jae	.LBB5_13
# %bb.14:
	movq	(%rdi), %rdx
	leaq	1(%rcx), %rsi
	movq	%rsi, 8(%rdi)
	movb	%ah, (%rdx,%rcx)
	jmp	.LBB5_15
.LBB5_13:
	movl	$1, 24(%rdi)
.LBB5_15:
	movq	8(%rdi), %rcx
	cmpq	16(%rdi), %rcx
	jae	.LBB5_16
# %bb.17:
	movl	%eax, %edx
	shrl	$16, %edx
	movq	(%rdi), %rsi
	leaq	1(%rcx), %r8
	movq	%r8, 8(%rdi)
	movb	%dl, (%rsi,%rcx)
	jmp	.LBB5_18
.LBB5_16:
	movl	$1, 24(%rdi)
.LBB5_18:
	movq	8(%rdi), %rcx
	cmpq	16(%rdi), %rcx
	jae	.LBB5_19
# %bb.20:
	shrl	$24, %eax
	movq	(%rdi), %rdx
	leaq	1(%rcx), %rsi
	movq	%rsi, 8(%rdi)
	movb	%al, (%rdx,%rcx)
	popq	%rbp
	retq
.LBB5_19:
	movl	$1, 24(%rdi)
	popq	%rbp
	retq
.Lfunc_end5:
	.size	emit_add_ptr, .Lfunc_end5-emit_add_ptr
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
	jae	.LBB6_1
# %bb.2:
	movq	(%rdi), %rcx
	leaq	1(%rax), %rdx
	movq	%rdx, 8(%rdi)
	movb	$-128, (%rcx,%rax)
	jmp	.LBB6_3
.LBB6_1:
	movl	$1, 24(%rdi)
.LBB6_3:
	movq	8(%rdi), %rax
	cmpq	16(%rdi), %rax
	jae	.LBB6_4
# %bb.5:
	movq	(%rdi), %rcx
	leaq	1(%rax), %rdx
	movq	%rdx, 8(%rdi)
	movb	$3, (%rcx,%rax)
	jmp	.LBB6_6
.LBB6_4:
	movl	$1, 24(%rdi)
.LBB6_6:
	movq	8(%rdi), %rax
	cmpq	16(%rdi), %rax
	jae	.LBB6_7
# %bb.8:
	movq	(%rdi), %rcx
	leaq	1(%rax), %rdx
	movq	%rdx, 8(%rdi)
	movb	%sil, (%rcx,%rax)
	popq	%rbp
	retq
.LBB6_7:
	movl	$1, 24(%rdi)
	popq	%rbp
	retq
.Lfunc_end6:
	.size	emit_add_cell, .Lfunc_end6-emit_add_cell
                                        # -- End function
	.globl	emit_set_cell                   # -- Begin function emit_set_cell
	.p2align	4
	.type	emit_set_cell,@function
emit_set_cell:                          # @emit_set_cell
# %bb.0:
	pushq	%rbp
	movq	%rsp, %rbp
	movq	8(%rdi), %rax
	cmpq	16(%rdi), %rax
	jae	.LBB7_1
# %bb.2:
	movq	(%rdi), %rcx
	leaq	1(%rax), %rdx
	movq	%rdx, 8(%rdi)
	movb	$-58, (%rcx,%rax)
	jmp	.LBB7_3
.LBB7_1:
	movl	$1, 24(%rdi)
.LBB7_3:
	movq	8(%rdi), %rax
	cmpq	16(%rdi), %rax
	jae	.LBB7_4
# %bb.5:
	movq	(%rdi), %rcx
	leaq	1(%rax), %rdx
	movq	%rdx, 8(%rdi)
	movb	$3, (%rcx,%rax)
	jmp	.LBB7_6
.LBB7_4:
	movl	$1, 24(%rdi)
.LBB7_6:
	movq	8(%rdi), %rax
	cmpq	16(%rdi), %rax
	jae	.LBB7_7
# %bb.8:
	movq	(%rdi), %rcx
	leaq	1(%rax), %rdx
	movq	%rdx, 8(%rdi)
	movb	%sil, (%rcx,%rax)
	popq	%rbp
	retq
.LBB7_7:
	movl	$1, 24(%rdi)
	popq	%rbp
	retq
.Lfunc_end7:
	.size	emit_set_cell, .Lfunc_end7-emit_set_cell
                                        # -- End function
	.globl	emit_output                     # -- Begin function emit_output
	.p2align	4
	.type	emit_output,@function
emit_output:                            # @emit_output
# %bb.0:
	pushq	%rbp
	movq	%rsp, %rbp
	movq	8(%rdi), %rax
	cmpq	16(%rdi), %rax
	jae	.LBB8_1
# %bb.2:
	movq	(%rdi), %rcx
	leaq	1(%rax), %rdx
	movq	%rdx, 8(%rdi)
	movb	$15, (%rcx,%rax)
	jmp	.LBB8_3
.LBB8_1:
	movl	$1, 24(%rdi)
.LBB8_3:
	movq	8(%rdi), %rax
	cmpq	16(%rdi), %rax
	jae	.LBB8_4
# %bb.5:
	movq	(%rdi), %rcx
	leaq	1(%rax), %rdx
	movq	%rdx, 8(%rdi)
	movb	$-74, (%rcx,%rax)
	jmp	.LBB8_6
.LBB8_4:
	movl	$1, 24(%rdi)
.LBB8_6:
	movq	8(%rdi), %rax
	cmpq	16(%rdi), %rax
	jae	.LBB8_7
# %bb.8:
	movq	(%rdi), %rcx
	leaq	1(%rax), %rdx
	movq	%rdx, 8(%rdi)
	movb	$59, (%rcx,%rax)
	jmp	.LBB8_9
.LBB8_7:
	movl	$1, 24(%rdi)
.LBB8_9:
	movq	8(%rdi), %rax
	cmpq	16(%rdi), %rax
	jae	.LBB8_10
# %bb.11:
	movq	(%rdi), %rcx
	leaq	1(%rax), %rdx
	movq	%rdx, 8(%rdi)
	movb	$65, (%rcx,%rax)
	jmp	.LBB8_12
.LBB8_10:
	movl	$1, 24(%rdi)
.LBB8_12:
	movq	8(%rdi), %rax
	cmpq	16(%rdi), %rax
	jae	.LBB8_13
# %bb.14:
	movq	(%rdi), %rcx
	leaq	1(%rax), %rdx
	movq	%rdx, 8(%rdi)
	movb	$-1, (%rcx,%rax)
	jmp	.LBB8_15
.LBB8_13:
	movl	$1, 24(%rdi)
.LBB8_15:
	movq	8(%rdi), %rax
	cmpq	16(%rdi), %rax
	jae	.LBB8_16
# %bb.17:
	movq	(%rdi), %rcx
	leaq	1(%rax), %rdx
	movq	%rdx, 8(%rdi)
	movb	$-41, (%rcx,%rax)
	popq	%rbp
	retq
.LBB8_16:
	movl	$1, 24(%rdi)
	popq	%rbp
	retq
.Lfunc_end8:
	.size	emit_output, .Lfunc_end8-emit_output
                                        # -- End function
	.globl	emit_input                      # -- Begin function emit_input
	.p2align	4
	.type	emit_input,@function
emit_input:                             # @emit_input
# %bb.0:
	pushq	%rbp
	movq	%rsp, %rbp
	movq	8(%rdi), %rax
	cmpq	16(%rdi), %rax
	jae	.LBB9_1
# %bb.2:
	movq	(%rdi), %rcx
	leaq	1(%rax), %rdx
	movq	%rdx, 8(%rdi)
	movb	$65, (%rcx,%rax)
	jmp	.LBB9_3
.LBB9_1:
	movl	$1, 24(%rdi)
.LBB9_3:
	movq	8(%rdi), %rax
	cmpq	16(%rdi), %rax
	jae	.LBB9_4
# %bb.5:
	movq	(%rdi), %rcx
	leaq	1(%rax), %rdx
	movq	%rdx, 8(%rdi)
	movb	$-1, (%rcx,%rax)
	jmp	.LBB9_6
.LBB9_4:
	movl	$1, 24(%rdi)
.LBB9_6:
	movq	8(%rdi), %rax
	cmpq	16(%rdi), %rax
	jae	.LBB9_7
# %bb.8:
	movq	(%rdi), %rcx
	leaq	1(%rax), %rdx
	movq	%rdx, 8(%rdi)
	movb	$-42, (%rcx,%rax)
	jmp	.LBB9_9
.LBB9_7:
	movl	$1, 24(%rdi)
.LBB9_9:
	movq	8(%rdi), %rax
	cmpq	16(%rdi), %rax
	jae	.LBB9_10
# %bb.11:
	movq	(%rdi), %rcx
	leaq	1(%rax), %rdx
	movq	%rdx, 8(%rdi)
	movb	$-120, (%rcx,%rax)
	jmp	.LBB9_12
.LBB9_10:
	movl	$1, 24(%rdi)
.LBB9_12:
	movq	8(%rdi), %rax
	cmpq	16(%rdi), %rax
	jae	.LBB9_13
# %bb.14:
	movq	(%rdi), %rcx
	leaq	1(%rax), %rdx
	movq	%rdx, 8(%rdi)
	movb	$3, (%rcx,%rax)
	popq	%rbp
	retq
.LBB9_13:
	movl	$1, 24(%rdi)
	popq	%rbp
	retq
.Lfunc_end9:
	.size	emit_input, .Lfunc_end9-emit_input
                                        # -- End function
	.globl	emit_loop_open                  # -- Begin function emit_loop_open
	.p2align	4
	.type	emit_loop_open,@function
emit_loop_open:                         # @emit_loop_open
# %bb.0:
	pushq	%rbp
	movq	%rsp, %rbp
	movq	8(%rdi), %rax
	cmpq	16(%rdi), %rax
	jae	.LBB10_1
# %bb.2:
	movq	(%rdi), %rcx
	leaq	1(%rax), %rdx
	movq	%rdx, 8(%rdi)
	movb	$-128, (%rcx,%rax)
	jmp	.LBB10_3
.LBB10_1:
	movl	$1, 24(%rdi)
.LBB10_3:
	movq	8(%rdi), %rax
	cmpq	16(%rdi), %rax
	jae	.LBB10_4
# %bb.5:
	movq	(%rdi), %rcx
	leaq	1(%rax), %rdx
	movq	%rdx, 8(%rdi)
	movb	$59, (%rcx,%rax)
	jmp	.LBB10_6
.LBB10_4:
	movl	$1, 24(%rdi)
.LBB10_6:
	movq	8(%rdi), %rax
	cmpq	16(%rdi), %rax
	jae	.LBB10_7
# %bb.8:
	movq	(%rdi), %rcx
	leaq	1(%rax), %rdx
	movq	%rdx, 8(%rdi)
	movb	$0, (%rcx,%rax)
	jmp	.LBB10_9
.LBB10_7:
	movl	$1, 24(%rdi)
.LBB10_9:
	movq	8(%rdi), %rax
	cmpq	16(%rdi), %rax
	jae	.LBB10_10
# %bb.11:
	movq	(%rdi), %rcx
	leaq	1(%rax), %rdx
	movq	%rdx, 8(%rdi)
	movb	$15, (%rcx,%rax)
	jmp	.LBB10_12
.LBB10_10:
	movl	$1, 24(%rdi)
.LBB10_12:
	movq	8(%rdi), %rax
	cmpq	16(%rdi), %rax
	jae	.LBB10_13
# %bb.14:
	movq	(%rdi), %rcx
	leaq	1(%rax), %rdx
	movq	%rdx, 8(%rdi)
	movb	$-124, (%rcx,%rax)
	jmp	.LBB10_15
.LBB10_13:
	movl	$1, 24(%rdi)
.LBB10_15:
	movq	8(%rdi), %rax
	cmpq	16(%rdi), %rax
	jae	.LBB10_16
# %bb.17:
	movq	(%rdi), %rcx
	leaq	1(%rax), %rdx
	movq	%rdx, 8(%rdi)
	movb	$0, (%rcx,%rax)
	jmp	.LBB10_18
.LBB10_16:
	movl	$1, 24(%rdi)
.LBB10_18:
	movq	8(%rdi), %rcx
	cmpq	16(%rdi), %rcx
	jae	.LBB10_19
# %bb.20:
	movq	(%rdi), %rdx
	leaq	1(%rcx), %rsi
	movq	%rsi, 8(%rdi)
	movb	$0, (%rdx,%rcx)
	jmp	.LBB10_21
.LBB10_19:
	movl	$1, 24(%rdi)
.LBB10_21:
	movq	8(%rdi), %rcx
	cmpq	16(%rdi), %rcx
	jae	.LBB10_22
# %bb.23:
	movq	(%rdi), %rdx
	leaq	1(%rcx), %rsi
	movq	%rsi, 8(%rdi)
	movb	$0, (%rdx,%rcx)
	jmp	.LBB10_24
.LBB10_22:
	movl	$1, 24(%rdi)
.LBB10_24:
	movq	8(%rdi), %rcx
	cmpq	16(%rdi), %rcx
	jae	.LBB10_25
# %bb.26:
	movq	(%rdi), %rdx
	leaq	1(%rcx), %rsi
	movq	%rsi, 8(%rdi)
	movb	$0, (%rdx,%rcx)
	popq	%rbp
	retq
.LBB10_25:
	movl	$1, 24(%rdi)
	popq	%rbp
	retq
.Lfunc_end10:
	.size	emit_loop_open, .Lfunc_end10-emit_loop_open
                                        # -- End function
	.globl	emit_loop_close                 # -- Begin function emit_loop_close
	.p2align	4
	.type	emit_loop_close,@function
emit_loop_close:                        # @emit_loop_close
# %bb.0:
	pushq	%rbp
	movq	%rsp, %rbp
	movq	8(%rdi), %rax
	cmpq	16(%rdi), %rax
	jae	.LBB11_1
# %bb.2:
	movq	(%rdi), %rcx
	leaq	1(%rax), %rdx
	movq	%rdx, 8(%rdi)
	movb	$-128, (%rcx,%rax)
	jmp	.LBB11_3
.LBB11_1:
	movl	$1, 24(%rdi)
.LBB11_3:
	movq	8(%rdi), %rax
	cmpq	16(%rdi), %rax
	jae	.LBB11_4
# %bb.5:
	movq	(%rdi), %rcx
	leaq	1(%rax), %rdx
	movq	%rdx, 8(%rdi)
	movb	$59, (%rcx,%rax)
	jmp	.LBB11_6
.LBB11_4:
	movl	$1, 24(%rdi)
.LBB11_6:
	movq	8(%rdi), %rax
	cmpq	16(%rdi), %rax
	jae	.LBB11_7
# %bb.8:
	movq	(%rdi), %rcx
	leaq	1(%rax), %rdx
	movq	%rdx, 8(%rdi)
	movb	$0, (%rcx,%rax)
	jmp	.LBB11_9
.LBB11_7:
	movl	$1, 24(%rdi)
.LBB11_9:
	movq	8(%rdi), %rax
	cmpq	16(%rdi), %rax
	jae	.LBB11_10
# %bb.11:
	movq	(%rdi), %rcx
	leaq	1(%rax), %rdx
	movq	%rdx, 8(%rdi)
	movb	$15, (%rcx,%rax)
	jmp	.LBB11_12
.LBB11_10:
	movl	$1, 24(%rdi)
.LBB11_12:
	movq	8(%rdi), %rax
	cmpq	16(%rdi), %rax
	jae	.LBB11_13
# %bb.14:
	movq	(%rdi), %rcx
	leaq	1(%rax), %rdx
	movq	%rdx, 8(%rdi)
	movb	$-123, (%rcx,%rax)
	jmp	.LBB11_15
.LBB11_13:
	movl	$1, 24(%rdi)
.LBB11_15:
	movq	8(%rdi), %rax
	cmpq	16(%rdi), %rax
	jae	.LBB11_16
# %bb.17:
	movq	(%rdi), %rcx
	leaq	1(%rax), %rdx
	movq	%rdx, 8(%rdi)
	movb	$0, (%rcx,%rax)
	jmp	.LBB11_18
.LBB11_16:
	movl	$1, 24(%rdi)
.LBB11_18:
	movq	8(%rdi), %rcx
	cmpq	16(%rdi), %rcx
	jae	.LBB11_19
# %bb.20:
	movq	(%rdi), %rdx
	leaq	1(%rcx), %rsi
	movq	%rsi, 8(%rdi)
	movb	$0, (%rdx,%rcx)
	jmp	.LBB11_21
.LBB11_19:
	movl	$1, 24(%rdi)
.LBB11_21:
	movq	8(%rdi), %rcx
	cmpq	16(%rdi), %rcx
	jae	.LBB11_22
# %bb.23:
	movq	(%rdi), %rdx
	leaq	1(%rcx), %rsi
	movq	%rsi, 8(%rdi)
	movb	$0, (%rdx,%rcx)
	jmp	.LBB11_24
.LBB11_22:
	movl	$1, 24(%rdi)
.LBB11_24:
	movq	8(%rdi), %rcx
	cmpq	16(%rdi), %rcx
	jae	.LBB11_25
# %bb.26:
	movq	(%rdi), %rdx
	leaq	1(%rcx), %rsi
	movq	%rsi, 8(%rdi)
	movb	$0, (%rdx,%rcx)
	popq	%rbp
	retq
.LBB11_25:
	movl	$1, 24(%rdi)
	popq	%rbp
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
	pushq	%rbp
	movq	%rsp, %rbp
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
	popq	%rbp
	retq
.Lfunc_end12:
	.size	patch_rel32, .Lfunc_end12-patch_rel32
                                        # -- End function
	.ident	"clang version 20.1.8"
	.section	".note.GNU-stack","",@progbits
	.addrsig
