	.file	"demo.c"
	.text
	.globl	cov_update                      # -- Begin function cov_update
	.p2align	4
	.type	cov_update,@function
cov_update:                             # @cov_update
# %bb.0:
	pushq	%rbp
	movq	%rsp, %rbp
	movq	%rdi, -8(%rbp)
	movl	%esi, -12(%rbp)
	movl	%edx, -16(%rbp)
	movl	-16(%rbp), %eax
	xorl	-12(%rbp), %eax
	andl	$65535, %eax                    # imm = 0xFFFF
	movl	%eax, -20(%rbp)
	movq	-8(%rbp), %rax
	movl	-20(%rbp), %ecx
                                        # kill: def $rcx killed $ecx
	movzbl	(%rax,%rcx), %eax
	cmpl	$255, %eax
	je	.LBB0_2
# %bb.1:
	movq	-8(%rbp), %rax
	movl	-20(%rbp), %ecx
                                        # kill: def $rcx killed $ecx
	movzbl	(%rax,%rcx), %eax
	addl	$1, %eax
	movb	%al, %dl
	movq	-8(%rbp), %rax
	movl	-20(%rbp), %ecx
                                        # kill: def $rcx killed $ecx
	movb	%dl, (%rax,%rcx)
.LBB0_2:
	movl	-16(%rbp), %eax
	shrl	%eax
	popq	%rbp
	retq
.Lfunc_end0:
	.size	cov_update, .Lfunc_end0-cov_update
                                        # -- End function
	.globl	classify_count                  # -- Begin function classify_count
	.p2align	4
	.type	classify_count,@function
classify_count:                         # @classify_count
# %bb.0:
	pushq	%rbp
	movq	%rsp, %rbp
	movb	%dil, %al
	movb	%al, -2(%rbp)
	movzbl	-2(%rbp), %eax
	cmpl	$0, %eax
	jne	.LBB1_2
# %bb.1:
	movb	$0, -1(%rbp)
	jmp	.LBB1_17
.LBB1_2:
	movzbl	-2(%rbp), %eax
	cmpl	$1, %eax
	jne	.LBB1_4
# %bb.3:
	movb	$1, -1(%rbp)
	jmp	.LBB1_17
.LBB1_4:
	movzbl	-2(%rbp), %eax
	cmpl	$2, %eax
	jne	.LBB1_6
# %bb.5:
	movb	$2, -1(%rbp)
	jmp	.LBB1_17
.LBB1_6:
	movzbl	-2(%rbp), %eax
	cmpl	$3, %eax
	jne	.LBB1_8
# %bb.7:
	movb	$4, -1(%rbp)
	jmp	.LBB1_17
.LBB1_8:
	movzbl	-2(%rbp), %eax
	cmpl	$7, %eax
	jg	.LBB1_10
# %bb.9:
	movb	$8, -1(%rbp)
	jmp	.LBB1_17
.LBB1_10:
	movzbl	-2(%rbp), %eax
	cmpl	$15, %eax
	jg	.LBB1_12
# %bb.11:
	movb	$16, -1(%rbp)
	jmp	.LBB1_17
.LBB1_12:
	movzbl	-2(%rbp), %eax
	cmpl	$31, %eax
	jg	.LBB1_14
# %bb.13:
	movb	$32, -1(%rbp)
	jmp	.LBB1_17
.LBB1_14:
	movzbl	-2(%rbp), %eax
	cmpl	$127, %eax
	jg	.LBB1_16
# %bb.15:
	movb	$64, -1(%rbp)
	jmp	.LBB1_17
.LBB1_16:
	movb	$-128, -1(%rbp)
.LBB1_17:
	movb	-1(%rbp), %al
	popq	%rbp
	retq
.Lfunc_end1:
	.size	classify_count, .Lfunc_end1-classify_count
                                        # -- End function
	.globl	has_new_bits                    # -- Begin function has_new_bits
	.p2align	4
	.type	has_new_bits,@function
has_new_bits:                           # @has_new_bits
# %bb.0:
	pushq	%rbp
	movq	%rsp, %rbp
	subq	$48, %rsp
	movq	%rdi, -8(%rbp)
	movq	%rsi, -16(%rbp)
	movq	%rdx, -24(%rbp)
	movl	$0, -28(%rbp)
	movq	$0, -40(%rbp)
.LBB2_1:                                # =>This Inner Loop Header: Depth=1
	movq	-40(%rbp), %rax
	cmpq	-24(%rbp), %rax
	jae	.LBB2_8
# %bb.2:                                #   in Loop: Header=BB2_1 Depth=1
	movq	-16(%rbp), %rax
	movq	-40(%rbp), %rcx
	movzbl	(%rax,%rcx), %edi
	callq	classify_count
	movb	%al, -41(%rbp)
	movzbl	-41(%rbp), %eax
	cmpl	$0, %eax
	jne	.LBB2_4
# %bb.3:                                #   in Loop: Header=BB2_1 Depth=1
	jmp	.LBB2_7
.LBB2_4:                                #   in Loop: Header=BB2_1 Depth=1
	movq	-8(%rbp), %rax
	movq	-40(%rbp), %rcx
	movzbl	(%rax,%rcx), %eax
	movzbl	-41(%rbp), %ecx
	andl	%ecx, %eax
	cmpl	$0, %eax
	je	.LBB2_6
# %bb.5:                                #   in Loop: Header=BB2_1 Depth=1
	movq	-8(%rbp), %rax
	movq	-40(%rbp), %rcx
	movzbl	(%rax,%rcx), %eax
	movzbl	-41(%rbp), %ecx
	xorl	$-1, %ecx
	andl	%ecx, %eax
	movb	%al, %dl
	movq	-8(%rbp), %rax
	movq	-40(%rbp), %rcx
	movb	%dl, (%rax,%rcx)
	movl	$1, -28(%rbp)
.LBB2_6:                                #   in Loop: Header=BB2_1 Depth=1
	jmp	.LBB2_7
.LBB2_7:                                #   in Loop: Header=BB2_1 Depth=1
	movq	-40(%rbp), %rax
	addq	$1, %rax
	movq	%rax, -40(%rbp)
	jmp	.LBB2_1
.LBB2_8:
	movl	-28(%rbp), %eax
	addq	$48, %rsp
	popq	%rbp
	retq
.Lfunc_end2:
	.size	has_new_bits, .Lfunc_end2-has_new_bits
                                        # -- End function
	.globl	demo_selftest                   # -- Begin function demo_selftest
	.p2align	4
	.type	demo_selftest,@function
demo_selftest:                          # @demo_selftest
# %bb.0:
	pushq	%rbp
	movq	%rsp, %rbp
	subq	$32, %rsp
	movq	$0, -8(%rbp)
.LBB3_1:                                # =>This Inner Loop Header: Depth=1
	cmpq	$65536, -8(%rbp)                # imm = 0x10000
	jae	.LBB3_4
# %bb.2:                                #   in Loop: Header=BB3_1 Depth=1
	movq	-8(%rbp), %rcx
	leaq	demo_selftest.map(%rip), %rax
	movb	$0, (%rax,%rcx)
	movq	-8(%rbp), %rcx
	leaq	demo_selftest.virgin(%rip), %rax
	movb	$-1, (%rax,%rcx)
# %bb.3:                                #   in Loop: Header=BB3_1 Depth=1
	movq	-8(%rbp), %rax
	addq	$1, %rax
	movq	%rax, -8(%rbp)
	jmp	.LBB3_1
.LBB3_4:
	movl	$0, -12(%rbp)
	movl	-12(%rbp), %esi
	leaq	demo_selftest.map(%rip), %rdi
	movl	$10, %edx
	callq	cov_update
	movl	%eax, -12(%rbp)
	movl	-12(%rbp), %esi
	leaq	demo_selftest.map(%rip), %rdi
	movl	$20, %edx
	callq	cov_update
	movl	%eax, -12(%rbp)
	movl	-12(%rbp), %esi
	leaq	demo_selftest.map(%rip), %rdi
	movl	$20, %edx
	callq	cov_update
	movl	%eax, -12(%rbp)
	movl	-12(%rbp), %esi
	leaq	demo_selftest.map(%rip), %rdi
	movl	$30, %edx
	callq	cov_update
	movl	%eax, -12(%rbp)
	leaq	demo_selftest.virgin(%rip), %rdi
	leaq	demo_selftest.map(%rip), %rsi
	movl	$65536, %edx                    # imm = 0x10000
	callq	has_new_bits
	movl	%eax, -16(%rbp)
	leaq	demo_selftest.virgin(%rip), %rdi
	leaq	demo_selftest.map(%rip), %rsi
	movl	$65536, %edx                    # imm = 0x10000
	callq	has_new_bits
	movl	%eax, -20(%rbp)
	movl	-16(%rbp), %eax
	shll	%eax
	orl	-20(%rbp), %eax
	movl	-12(%rbp), %ecx
	shll	$2, %ecx
	orl	%ecx, %eax
	addq	$32, %rsp
	popq	%rbp
	retq
.Lfunc_end3:
	.size	demo_selftest, .Lfunc_end3-demo_selftest
                                        # -- End function
	.type	demo_selftest.map,@object       # @demo_selftest.map
	.local	demo_selftest.map
	.comm	demo_selftest.map,65536,16
	.type	demo_selftest.virgin,@object    # @demo_selftest.virgin
	.local	demo_selftest.virgin
	.comm	demo_selftest.virgin,65536,16
	.ident	"clang version 20.1.8"
	.section	".note.GNU-stack","",@progbits
	.addrsig
	.addrsig_sym cov_update
	.addrsig_sym classify_count
	.addrsig_sym has_new_bits
	.addrsig_sym demo_selftest.map
	.addrsig_sym demo_selftest.virgin
