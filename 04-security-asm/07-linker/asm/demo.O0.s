	.file	"demo.c"
	.text
	.globl	apply_reloc                     # -- Begin function apply_reloc
	.p2align	4
	.type	apply_reloc,@function
apply_reloc:                            # @apply_reloc
# %bb.0:
	pushq	%rbp
	movq	%rsp, %rbp
	subq	$80, %rsp
	movq	%rdi, -16(%rbp)
	movl	%esi, -20(%rbp)
	movq	%rdx, -32(%rbp)
	movq	%rcx, -40(%rbp)
	movq	%r8, -48(%rbp)
	movl	-20(%rbp), %eax
	movl	%eax, -76(%rbp)                 # 4-byte Spill
	subl	$1, %eax
	je	.LBB0_1
	jmp	.LBB0_15
.LBB0_15:
	movl	-76(%rbp), %eax                 # 4-byte Reload
	subl	$2, %eax
	je	.LBB0_2
	jmp	.LBB0_16
.LBB0_16:
	movl	-76(%rbp), %eax                 # 4-byte Reload
	subl	$4, %eax
	je	.LBB0_2
	jmp	.LBB0_17
.LBB0_17:
	movl	-76(%rbp), %eax                 # 4-byte Reload
	subl	$10, %eax
	je	.LBB0_6
	jmp	.LBB0_18
.LBB0_18:
	movl	-76(%rbp), %eax                 # 4-byte Reload
	subl	$11, %eax
	je	.LBB0_9
	jmp	.LBB0_13
.LBB0_1:
	movq	-16(%rbp), %rdi
	movq	-32(%rbp), %rsi
	addq	-40(%rbp), %rsi
	callq	put64
	movl	$0, -4(%rbp)
	jmp	.LBB0_14
.LBB0_2:
	movq	-32(%rbp), %rax
	addq	-40(%rbp), %rax
	subq	-48(%rbp), %rax
	movq	%rax, -56(%rbp)
	cmpq	$-2147483648, -56(%rbp)         # imm = 0x80000000
	jl	.LBB0_4
# %bb.3:
	cmpq	$2147483647, -56(%rbp)          # imm = 0x7FFFFFFF
	jle	.LBB0_5
.LBB0_4:
	movl	$1, -4(%rbp)
	jmp	.LBB0_14
.LBB0_5:
	movq	-16(%rbp), %rdi
	movq	-56(%rbp), %rax
	movl	%eax, %esi
	callq	put32
	movl	$0, -4(%rbp)
	jmp	.LBB0_14
.LBB0_6:
	movq	-32(%rbp), %rax
	addq	-40(%rbp), %rax
	movq	%rax, -64(%rbp)
	movl	$4294967295, %eax               # imm = 0xFFFFFFFF
	cmpq	%rax, -64(%rbp)
	jbe	.LBB0_8
# %bb.7:
	movl	$1, -4(%rbp)
	jmp	.LBB0_14
.LBB0_8:
	movq	-16(%rbp), %rdi
	movq	-64(%rbp), %rax
	movl	%eax, %esi
	callq	put32
	movl	$0, -4(%rbp)
	jmp	.LBB0_14
.LBB0_9:
	movq	-32(%rbp), %rax
	addq	-40(%rbp), %rax
	movq	%rax, -72(%rbp)
	cmpq	$-2147483648, -72(%rbp)         # imm = 0x80000000
	jl	.LBB0_11
# %bb.10:
	cmpq	$2147483647, -72(%rbp)          # imm = 0x7FFFFFFF
	jle	.LBB0_12
.LBB0_11:
	movl	$1, -4(%rbp)
	jmp	.LBB0_14
.LBB0_12:
	movq	-16(%rbp), %rdi
	movq	-72(%rbp), %rax
	movl	%eax, %esi
	callq	put32
	movl	$0, -4(%rbp)
	jmp	.LBB0_14
.LBB0_13:
	movl	$2, -4(%rbp)
.LBB0_14:
	movl	-4(%rbp), %eax
	addq	$80, %rsp
	popq	%rbp
	retq
.Lfunc_end0:
	.size	apply_reloc, .Lfunc_end0-apply_reloc
                                        # -- End function
	.p2align	4                               # -- Begin function put64
	.type	put64,@function
put64:                                  # @put64
# %bb.0:
	pushq	%rbp
	movq	%rsp, %rbp
	subq	$16, %rsp
	movq	%rdi, -8(%rbp)
	movq	%rsi, -16(%rbp)
	movq	-8(%rbp), %rdi
	movq	-16(%rbp), %rax
	movl	%eax, %esi
	callq	put32
	movq	-8(%rbp), %rdi
	addq	$4, %rdi
	movq	-16(%rbp), %rax
	shrq	$32, %rax
	movl	%eax, %esi
	callq	put32
	addq	$16, %rsp
	popq	%rbp
	retq
.Lfunc_end1:
	.size	put64, .Lfunc_end1-put64
                                        # -- End function
	.p2align	4                               # -- Begin function put32
	.type	put32,@function
put32:                                  # @put32
# %bb.0:
	pushq	%rbp
	movq	%rsp, %rbp
	movq	%rdi, -8(%rbp)
	movl	%esi, -12(%rbp)
	movl	-12(%rbp), %eax
	movb	%al, %cl
	movq	-8(%rbp), %rax
	movb	%cl, (%rax)
	movl	-12(%rbp), %eax
	shrl	$8, %eax
	movb	%al, %cl
	movq	-8(%rbp), %rax
	movb	%cl, 1(%rax)
	movl	-12(%rbp), %eax
	shrl	$16, %eax
	movb	%al, %cl
	movq	-8(%rbp), %rax
	movb	%cl, 2(%rax)
	movl	-12(%rbp), %eax
	shrl	$24, %eax
	movb	%al, %cl
	movq	-8(%rbp), %rax
	movb	%cl, 3(%rax)
	popq	%rbp
	retq
.Lfunc_end2:
	.size	put32, .Lfunc_end2-put32
                                        # -- End function
	.globl	demo_selfcheck                  # -- Begin function demo_selfcheck
	.p2align	4
	.type	demo_selfcheck,@function
demo_selfcheck:                         # @demo_selfcheck
# %bb.0:
	pushq	%rbp
	movq	%rsp, %rbp
	subq	$48, %rsp
	movl	$0, -20(%rbp)
.LBB3_1:                                # =>This Inner Loop Header: Depth=1
	cmpl	$16, -20(%rbp)
	jge	.LBB3_4
# %bb.2:                                #   in Loop: Header=BB3_1 Depth=1
	movslq	-20(%rbp), %rax
	movb	$0, -16(%rbp,%rax)
# %bb.3:                                #   in Loop: Header=BB3_1 Depth=1
	movl	-20(%rbp), %eax
	addl	$1, %eax
	movl	%eax, -20(%rbp)
	jmp	.LBB3_1
.LBB3_4:
	movb	$-24, -16(%rbp)
	leaq	-16(%rbp), %rdi
	addq	$1, %rdi
	movl	$2, %esi
	movl	$4198964, %edx                  # imm = 0x401234
	movq	$-4, %rcx
	movl	$4198401, %r8d                  # imm = 0x401001
	callq	apply_reloc
	leaq	-16(%rbp), %rdi
	addq	$8, %rdi
	movl	$1, %esi
	movl	$4198964, %edx                  # imm = 0x401234
	xorl	%eax, %eax
	movl	%eax, %r8d
	movq	%r8, %rcx
	callq	apply_reloc
	movq	$0, -32(%rbp)
	movl	$0, -36(%rbp)
.LBB3_5:                                # =>This Inner Loop Header: Depth=1
	cmpl	$16, -36(%rbp)
	jge	.LBB3_8
# %bb.6:                                #   in Loop: Header=BB3_5 Depth=1
	imulq	$131, -32(%rbp), %rax
	movslq	-36(%rbp), %rcx
	movzbl	-16(%rbp,%rcx), %ecx
                                        # kill: def $rcx killed $ecx
	addq	%rcx, %rax
	movq	%rax, -32(%rbp)
# %bb.7:                                #   in Loop: Header=BB3_5 Depth=1
	movl	-36(%rbp), %eax
	addl	$1, %eax
	movl	%eax, -36(%rbp)
	jmp	.LBB3_5
.LBB3_8:
	movq	-32(%rbp), %rax
	addq	$48, %rsp
	popq	%rbp
	retq
.Lfunc_end3:
	.size	demo_selfcheck, .Lfunc_end3-demo_selfcheck
                                        # -- End function
	.ident	"clang version 20.1.8"
	.section	".note.GNU-stack","",@progbits
	.addrsig
	.addrsig_sym apply_reloc
	.addrsig_sym put64
	.addrsig_sym put32
