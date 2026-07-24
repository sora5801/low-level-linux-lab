	.file	"demo.c"
	.text
	.globl	cov_update                      # -- Begin function cov_update
	.p2align	4
	.type	cov_update,@function
cov_update:                             # @cov_update
# %bb.0:
	pushq	%rbp
	movq	%rsp, %rbp
	movl	%edx, %eax
	xorl	%edx, %esi
	movzwl	%si, %ecx
	movzbl	(%rdi,%rcx), %edx
	cmpb	$-1, %dl
	je	.LBB0_2
# %bb.1:
	incb	%dl
	movb	%dl, (%rdi,%rcx)
.LBB0_2:
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
	cmpl	$3, %edi
	jae	.LBB1_2
# %bb.1:
	movl	%edi, %eax
.LBB1_8:
                                        # kill: def $al killed $al killed $eax
	popq	%rbp
	retq
.LBB1_2:
	jne	.LBB1_4
# %bb.3:
	movb	$4, %al
                                        # kill: def $al killed $al killed $eax
	popq	%rbp
	retq
.LBB1_4:
	movb	$8, %al
	cmpb	$8, %dil
	jb	.LBB1_8
# %bb.5:
	movb	$16, %al
	cmpb	$16, %dil
	jb	.LBB1_8
# %bb.6:
	movb	$32, %al
	cmpb	$32, %dil
	jb	.LBB1_8
# %bb.7:
	testb	%dil, %dil
	movl	$64, %ecx
	movl	$128, %eax
	cmovnsl	%ecx, %eax
                                        # kill: def $al killed $al killed $eax
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
	xorl	%eax, %eax
	testq	%rdx, %rdx
	je	.LBB2_15
# %bb.1:
	xorl	%ecx, %ecx
	jmp	.LBB2_2
	.p2align	4
.LBB2_14:                               #   in Loop: Header=BB2_2 Depth=1
	incq	%rcx
	cmpq	%rcx, %rdx
	je	.LBB2_15
.LBB2_2:                                # =>This Inner Loop Header: Depth=1
	movzbl	(%rsi,%rcx), %r9d
	cmpl	$3, %r9d
	jae	.LBB2_4
# %bb.3:                                #   in Loop: Header=BB2_2 Depth=1
	movl	%r9d, %r8d
	jmp	.LBB2_11
	.p2align	4
.LBB2_4:                                #   in Loop: Header=BB2_2 Depth=1
	jne	.LBB2_6
# %bb.5:                                #   in Loop: Header=BB2_2 Depth=1
	movb	$4, %r8b
	jmp	.LBB2_11
.LBB2_6:                                #   in Loop: Header=BB2_2 Depth=1
	movb	$8, %r8b
	cmpb	$8, %r9b
	jb	.LBB2_11
# %bb.7:                                #   in Loop: Header=BB2_2 Depth=1
	movb	$16, %r8b
	cmpb	$16, %r9b
	jb	.LBB2_11
# %bb.8:                                #   in Loop: Header=BB2_2 Depth=1
	movb	$32, %r8b
	cmpb	$32, %r9b
	jb	.LBB2_11
# %bb.9:                                #   in Loop: Header=BB2_2 Depth=1
	movl	$64, %r8d
	testb	%r9b, %r9b
	jns	.LBB2_11
# %bb.10:                               #   in Loop: Header=BB2_2 Depth=1
	movl	$128, %r8d
	.p2align	4
.LBB2_11:                               #   in Loop: Header=BB2_2 Depth=1
	testb	%r8b, %r8b
	je	.LBB2_14
# %bb.12:                               #   in Loop: Header=BB2_2 Depth=1
	movzbl	(%rdi,%rcx), %r9d
	testb	%r8b, %r9b
	je	.LBB2_14
# %bb.13:                               #   in Loop: Header=BB2_2 Depth=1
	notb	%r8b
	andb	%r8b, %r9b
	movb	%r9b, (%rdi,%rcx)
	movl	$1, %eax
	jmp	.LBB2_14
.LBB2_15:
                                        # kill: def $eax killed $eax killed $rax
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
	pushq	%r14
	pushq	%rbx
	leaq	demo_selftest.map(%rip), %rbx
	movl	$65536, %edx                    # imm = 0x10000
	movq	%rbx, %rdi
	xorl	%esi, %esi
	callq	memset@PLT
	leaq	demo_selftest.virgin(%rip), %r14
	movl	$65536, %edx                    # imm = 0x10000
	movq	%r14, %rdi
	movl	$255, %esi
	callq	memset@PLT
	movzbl	demo_selftest.map+10(%rip), %eax
	cmpb	$-1, %al
	je	.LBB3_2
# %bb.1:
	incb	%al
	movb	%al, demo_selftest.map+10(%rip)
.LBB3_2:
	movzbl	demo_selftest.map+17(%rip), %eax
	cmpb	$-1, %al
	je	.LBB3_4
# %bb.3:
	incb	%al
	movb	%al, demo_selftest.map+17(%rip)
.LBB3_4:
	movzbl	demo_selftest.map+30(%rip), %eax
	cmpb	$-1, %al
	je	.LBB3_6
# %bb.5:
	incb	%al
	movb	%al, demo_selftest.map+30(%rip)
.LBB3_6:
	movzbl	demo_selftest.map+20(%rip), %eax
	cmpb	$-1, %al
	je	.LBB3_8
# %bb.7:
	incb	%al
	movb	%al, demo_selftest.map+20(%rip)
.LBB3_8:
	xorl	%eax, %eax
	xorl	%ecx, %ecx
	jmp	.LBB3_9
	.p2align	4
.LBB3_21:                               #   in Loop: Header=BB3_9 Depth=1
	incq	%rcx
	cmpq	$65536, %rcx                    # imm = 0x10000
	je	.LBB3_22
.LBB3_9:                                # =>This Inner Loop Header: Depth=1
	movzbl	(%rcx,%rbx), %esi
	cmpl	$3, %esi
	jae	.LBB3_11
# %bb.10:                               #   in Loop: Header=BB3_9 Depth=1
	movl	%esi, %edx
	jmp	.LBB3_18
	.p2align	4
.LBB3_11:                               #   in Loop: Header=BB3_9 Depth=1
	jne	.LBB3_13
# %bb.12:                               #   in Loop: Header=BB3_9 Depth=1
	movb	$4, %dl
	jmp	.LBB3_18
.LBB3_13:                               #   in Loop: Header=BB3_9 Depth=1
	movb	$8, %dl
	cmpb	$8, %sil
	jb	.LBB3_18
# %bb.14:                               #   in Loop: Header=BB3_9 Depth=1
	movb	$16, %dl
	cmpb	$16, %sil
	jb	.LBB3_18
# %bb.15:                               #   in Loop: Header=BB3_9 Depth=1
	movb	$32, %dl
	cmpb	$32, %sil
	jb	.LBB3_18
# %bb.16:                               #   in Loop: Header=BB3_9 Depth=1
	movl	$64, %edx
	testb	%sil, %sil
	jns	.LBB3_18
# %bb.17:                               #   in Loop: Header=BB3_9 Depth=1
	movl	$128, %edx
	.p2align	4
.LBB3_18:                               #   in Loop: Header=BB3_9 Depth=1
	testb	%dl, %dl
	je	.LBB3_21
# %bb.19:                               #   in Loop: Header=BB3_9 Depth=1
	movzbl	(%rcx,%r14), %esi
	testb	%dl, %sil
	je	.LBB3_21
# %bb.20:                               #   in Loop: Header=BB3_9 Depth=1
	notb	%dl
	andb	%dl, %sil
	movb	%sil, (%rcx,%r14)
	movl	$1, %eax
	jmp	.LBB3_21
.LBB3_22:
	xorl	%ecx, %ecx
	xorl	%edx, %edx
	jmp	.LBB3_23
	.p2align	4
.LBB3_35:                               #   in Loop: Header=BB3_23 Depth=1
	incq	%rdx
	cmpq	$65536, %rdx                    # imm = 0x10000
	je	.LBB3_36
.LBB3_23:                               # =>This Inner Loop Header: Depth=1
	movzbl	(%rdx,%rbx), %edi
	cmpl	$3, %edi
	jae	.LBB3_25
# %bb.24:                               #   in Loop: Header=BB3_23 Depth=1
	movl	%edi, %esi
	jmp	.LBB3_32
	.p2align	4
.LBB3_25:                               #   in Loop: Header=BB3_23 Depth=1
	jne	.LBB3_27
# %bb.26:                               #   in Loop: Header=BB3_23 Depth=1
	movb	$4, %sil
	jmp	.LBB3_32
.LBB3_27:                               #   in Loop: Header=BB3_23 Depth=1
	movb	$8, %sil
	cmpb	$8, %dil
	jb	.LBB3_32
# %bb.28:                               #   in Loop: Header=BB3_23 Depth=1
	movb	$16, %sil
	cmpb	$16, %dil
	jb	.LBB3_32
# %bb.29:                               #   in Loop: Header=BB3_23 Depth=1
	movb	$32, %sil
	cmpb	$32, %dil
	jb	.LBB3_32
# %bb.30:                               #   in Loop: Header=BB3_23 Depth=1
	movl	$64, %esi
	testb	%dil, %dil
	jns	.LBB3_32
# %bb.31:                               #   in Loop: Header=BB3_23 Depth=1
	movl	$128, %esi
	.p2align	4
.LBB3_32:                               #   in Loop: Header=BB3_23 Depth=1
	testb	%sil, %sil
	je	.LBB3_35
# %bb.33:                               #   in Loop: Header=BB3_23 Depth=1
	movzbl	(%rdx,%r14), %edi
	testb	%sil, %dil
	je	.LBB3_35
# %bb.34:                               #   in Loop: Header=BB3_23 Depth=1
	notb	%sil
	andb	%sil, %dil
	movb	%dil, (%rdx,%r14)
	movl	$1, %ecx
	jmp	.LBB3_35
.LBB3_36:
	leal	(%rcx,%rax,2), %eax
	addl	$60, %eax
	popq	%rbx
	popq	%r14
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
