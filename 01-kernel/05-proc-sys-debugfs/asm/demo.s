	.file	"demo.c"
	.text
	.globl	u64_to_dec                      # -- Begin function u64_to_dec
	.p2align	4
	.type	u64_to_dec,@function
u64_to_dec:                             # @u64_to_dec
# %bb.0:
	testq	%rdi, %rdi
	je	.LBB0_8
# %bb.1:
	pushq	%rbp
	movq	%rsp, %rbp
	xorl	%ecx, %ecx
	movabsq	$-3689348814741910323, %r8      # imm = 0xCCCCCCCCCCCCCCCD
	.p2align	4
.LBB0_2:                                # =>This Inner Loop Header: Depth=1
	movq	%rdi, %rax
	mulq	%r8
	shrq	$3, %rdx
	leal	(%rdx,%rdx), %eax
	leal	(%rax,%rax,4), %eax
	movl	%edi, %r9d
	subl	%eax, %r9d
	orb	$48, %r9b
	movl	%ecx, %eax
	incl	%ecx
	movb	%r9b, -32(%rbp,%rax)
	cmpq	$10, %rdi
	movq	%rdx, %rdi
	jae	.LBB0_2
# %bb.3:
	movl	%ecx, %edx
	movl	%ecx, %eax
	subl	$1, %edx
	jb	.LBB0_6
# %bb.4:
	xorl	%edi, %edi
	.p2align	4
.LBB0_5:                                # =>This Inner Loop Header: Depth=1
	movl	%edx, %r8d
	movzbl	-32(%rbp,%r8), %r8d
	movb	%r8b, (%rsi,%rdi)
	incq	%rdi
	decl	%edx
	cmpq	%rdi, %rax
	jne	.LBB0_5
.LBB0_6:
	popq	%rbp
	jmp	.LBB0_7
.LBB0_8:
	movb	$48, (%rsi)
	movl	$1, %ecx
	movl	$1, %eax
.LBB0_7:
	movb	$0, (%rsi,%rax)
	movl	%ecx, %eax
	retq
.Lfunc_end0:
	.size	u64_to_dec, .Lfunc_end0-u64_to_dec
                                        # -- End function
	.globl	format_field                    # -- Begin function format_field
	.p2align	4
	.type	format_field,@function
format_field:                           # @format_field
# %bb.0:
	pushq	%rbp
	movq	%rsp, %rbp
	pushq	%r15
	pushq	%r14
	pushq	%rbx
	subq	$56, %rsp
	movq	%rdx, %rbx
	movl	%esi, %ecx
	testq	%rdi, %rdi
	je	.LBB1_14
# %bb.1:
	xorl	%r14d, %r14d
	movabsq	$-3689348814741910323, %rsi     # imm = 0xCCCCCCCCCCCCCCCD
	.p2align	4
.LBB1_2:                                # =>This Inner Loop Header: Depth=1
	movq	%rdi, %rax
	mulq	%rsi
	shrq	$3, %rdx
	leal	(%rdx,%rdx), %eax
	leal	(%rax,%rax,4), %eax
	movl	%edi, %r8d
	subl	%eax, %r8d
	orb	$48, %r8b
	movl	%r14d, %eax
	incl	%r14d
	movb	%r8b, -80(%rbp,%rax)
	cmpq	$10, %rdi
	movq	%rdx, %rdi
	jae	.LBB1_2
# %bb.3:
	movl	%r14d, %edx
	movl	%r14d, %eax
	subl	$1, %edx
	jb	.LBB1_6
# %bb.4:
	xorl	%esi, %esi
	.p2align	4
.LBB1_5:                                # =>This Inner Loop Header: Depth=1
	movl	%edx, %edi
	movzbl	-80(%rbp,%rdi), %edi
	movb	%dil, -48(%rbp,%rsi)
	incq	%rsi
	decl	%edx
	cmpq	%rsi, %rax
	jne	.LBB1_5
	jmp	.LBB1_6
.LBB1_14:
	movb	$48, -48(%rbp)
	movl	$1, %r14d
	movl	$1, %eax
.LBB1_6:
	movb	$0, -48(%rbp,%rax)
	xorl	%eax, %eax
	movl	%ecx, %r15d
	subl	%r14d, %r15d
	cmovbl	%eax, %r15d
	jbe	.LBB1_9
# %bb.7:
	movl	%r14d, %eax
	notl	%eax
	addl	%eax, %ecx
	incq	%rcx
	movq	%rbx, %rdi
	movl	$32, %esi
	movq	%rcx, %rdx
	callq	memset@PLT
	xorl	%eax, %eax
	.p2align	4
.LBB1_8:                                # =>This Inner Loop Header: Depth=1
	incl	%eax
	cmpl	%eax, %r15d
	ja	.LBB1_8
.LBB1_9:
	testl	%r14d, %r14d
	je	.LBB1_13
# %bb.10:
	movl	%r14d, %ecx
	movl	%eax, %esi
	xorl	%edx, %edx
	.p2align	4
.LBB1_11:                               # =>This Inner Loop Header: Depth=1
	leal	(%rsi,%rdx), %edi
	movzbl	-48(%rbp,%rdx), %r8d
	movb	%r8b, (%rbx,%rdi)
	incq	%rdx
	cmpq	%rdx, %rcx
	jne	.LBB1_11
# %bb.12:
	addl	%edx, %eax
.LBB1_13:
	movl	%eax, %ecx
	movb	$0, (%rbx,%rcx)
	addq	$56, %rsp
	popq	%rbx
	popq	%r14
	popq	%r15
	popq	%rbp
	retq
.Lfunc_end1:
	.size	format_field, .Lfunc_end1-format_field
                                        # -- End function
	.globl	checksum8                       # -- Begin function checksum8
	.p2align	4
	.type	checksum8,@function
checksum8:                              # @checksum8
# %bb.0:
	pushq	%rbp
	movq	%rsp, %rbp
	testl	%esi, %esi
	je	.LBB2_1
# %bb.2:
	movl	%esi, %eax
	xorl	%ecx, %ecx
	xorl	%edx, %edx
	.p2align	4
.LBB2_3:                                # =>This Inner Loop Header: Depth=1
	movzbl	(%rdi,%rcx), %esi
	addl	%esi, %edx
	incq	%rcx
	cmpq	%rcx, %rax
	jne	.LBB2_3
# %bb.4:
	movzbl	%dl, %eax
	popq	%rbp
	retq
.LBB2_1:
	xorl	%eax, %eax
	popq	%rbp
	retq
.Lfunc_end2:
	.size	checksum8, .Lfunc_end2-checksum8
                                        # -- End function
	.globl	render_line                     # -- Begin function render_line
	.p2align	4
	.type	render_line,@function
render_line:                            # @render_line
# %bb.0:
	pushq	%rbp
	movq	%rsp, %rbp
	pushq	%r15
	pushq	%r14
	pushq	%r12
	pushq	%rbx
	subq	$64, %rsp
	movq	%rdx, %rbx
	movabsq	$-3689348814741910323, %r8      # imm = 0xCCCCCCCCCCCCCCCD
	testl	%edi, %edi
	je	.LBB3_19
# %bb.1:
	movl	%edi, %ecx
	xorl	%r14d, %r14d
	.p2align	4
.LBB3_2:                                # =>This Inner Loop Header: Depth=1
	movq	%rcx, %rax
	mulq	%r8
	shrq	$3, %rdx
	leal	(%rdx,%rdx), %eax
	leal	(%rax,%rax,4), %eax
	movl	%ecx, %edi
	subl	%eax, %edi
	orb	$48, %dil
	movl	%r14d, %eax
	incl	%r14d
	movb	%dil, -64(%rbp,%rax)
	cmpq	$10, %rcx
	movq	%rdx, %rcx
	jae	.LBB3_2
# %bb.3:
	movl	%r14d, %ecx
	movl	%r14d, %eax
	subl	$1, %ecx
	jb	.LBB3_6
# %bb.4:
	xorl	%edx, %edx
	.p2align	4
.LBB3_5:                                # =>This Inner Loop Header: Depth=1
	movl	%ecx, %edi
	movzbl	-64(%rbp,%rdi), %edi
	movb	%dil, (%rbx,%rdx)
	incq	%rdx
	decl	%ecx
	cmpq	%rdx, %rax
	jne	.LBB3_5
	jmp	.LBB3_6
.LBB3_19:
	movb	$48, (%rbx)
	movl	$1, %r14d
	movl	$1, %eax
.LBB3_6:
	movb	$0, (%rbx,%rax)
	movl	%r14d, %eax
	incl	%r14d
	movb	$32, (%rbx,%rax)
	testq	%rsi, %rsi
	je	.LBB3_20
# %bb.7:
	xorl	%r12d, %r12d
	.p2align	4
.LBB3_8:                                # =>This Inner Loop Header: Depth=1
	movq	%rsi, %rax
	mulq	%r8
	shrq	$3, %rdx
	leal	(%rdx,%rdx), %eax
	leal	(%rax,%rax,4), %eax
	movl	%esi, %ecx
	subl	%eax, %ecx
	orb	$48, %cl
	movl	%r12d, %eax
	incl	%r12d
	movb	%cl, -96(%rbp,%rax)
	cmpq	$10, %rsi
	movq	%rdx, %rsi
	jae	.LBB3_8
# %bb.9:
	movl	%r12d, %ecx
	movl	%r12d, %eax
	subl	$1, %ecx
	jb	.LBB3_12
# %bb.10:
	xorl	%edx, %edx
	.p2align	4
.LBB3_11:                               # =>This Inner Loop Header: Depth=1
	movl	%ecx, %esi
	movzbl	-96(%rbp,%rsi), %esi
	movb	%sil, -64(%rbp,%rdx)
	incq	%rdx
	decl	%ecx
	cmpq	%rdx, %rax
	jne	.LBB3_11
	jmp	.LBB3_12
.LBB3_20:
	movb	$48, -64(%rbp)
	movl	$1, %r12d
	movl	$1, %eax
.LBB3_12:
	addq	%r14, %rbx
	movb	$0, -64(%rbp,%rax)
	xorl	%r15d, %r15d
	cmpl	$19, %r12d
	ja	.LBB3_14
# %bb.13:
	movl	$20, %r15d
	subl	%r12d, %r15d
	movq	%rbx, %rdi
	movl	$32, %esi
	movq	%r15, %rdx
	callq	memset@PLT
.LBB3_14:
	testl	%r12d, %r12d
	je	.LBB3_18
# %bb.15:
	movl	%r12d, %eax
	movl	%r15d, %edx
	xorl	%ecx, %ecx
	.p2align	4
.LBB3_16:                               # =>This Inner Loop Header: Depth=1
	leal	(%rdx,%rcx), %esi
	movzbl	-64(%rbp,%rcx), %edi
	movb	%dil, (%rbx,%rsi)
	incq	%rcx
	cmpq	%rcx, %rax
	jne	.LBB3_16
# %bb.17:
	addl	%ecx, %r15d
.LBB3_18:
	movl	%r15d, %eax
	movb	$0, (%rbx,%rax)
	addl	%r15d, %r14d
	movl	%r14d, %eax
	addq	$64, %rsp
	popq	%rbx
	popq	%r12
	popq	%r14
	popq	%r15
	popq	%rbp
	retq
.Lfunc_end3:
	.size	render_line, .Lfunc_end3-render_line
                                        # -- End function
	.ident	"clang version 20.1.8"
	.section	".note.GNU-stack","",@progbits
	.addrsig
