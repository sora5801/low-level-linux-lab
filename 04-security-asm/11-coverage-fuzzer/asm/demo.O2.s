	.file	"demo.c"
	.text
	.globl	cov_update                      # -- Begin function cov_update
	.p2align	4
	.type	cov_update,@function
cov_update:                             # @cov_update
# %bb.0:
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
	retq
.Lfunc_end0:
	.size	cov_update, .Lfunc_end0-cov_update
                                        # -- End function
	.globl	classify_count                  # -- Begin function classify_count
	.p2align	4
	.type	classify_count,@function
classify_count:                         # @classify_count
# %bb.0:
	cmpl	$3, %edi
	jae	.LBB1_2
# %bb.1:
	movl	%edi, %eax
                                        # kill: def $al killed $al killed $eax
	retq
.LBB1_2:
	jne	.LBB1_4
# %bb.3:
	movb	$4, %al
                                        # kill: def $al killed $al killed $eax
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
.LBB1_8:
                                        # kill: def $al killed $al killed $eax
	retq
.Lfunc_end1:
	.size	classify_count, .Lfunc_end1-classify_count
                                        # -- End function
	.globl	has_new_bits                    # -- Begin function has_new_bits
	.p2align	4
	.type	has_new_bits,@function
has_new_bits:                           # @has_new_bits
# %bb.0:
	xorl	%eax, %eax
	testq	%rdx, %rdx
	je	.LBB2_14
# %bb.1:
	movl	$64, %ecx
	xorl	%r8d, %r8d
	jmp	.LBB2_2
	.p2align	4
.LBB2_13:                               #   in Loop: Header=BB2_2 Depth=1
	incq	%r8
	cmpq	%r8, %rdx
	je	.LBB2_14
.LBB2_2:                                # =>This Inner Loop Header: Depth=1
	movzbl	(%rsi,%r8), %r10d
	movzbl	%r10b, %r9d
	leal	-1(%r9), %r11d
	cmpl	$2, %r11d
	jb	.LBB2_3
# %bb.4:                                #   in Loop: Header=BB2_2 Depth=1
	testl	%r9d, %r9d
	je	.LBB2_13
# %bb.5:                                #   in Loop: Header=BB2_2 Depth=1
	cmpl	$3, %r9d
	jne	.LBB2_7
# %bb.6:                                #   in Loop: Header=BB2_2 Depth=1
	movb	$4, %r9b
	jmp	.LBB2_11
	.p2align	4
.LBB2_3:                                #   in Loop: Header=BB2_2 Depth=1
	movl	%r10d, %r9d
.LBB2_11:                               #   in Loop: Header=BB2_2 Depth=1
	movzbl	(%rdi,%r8), %r10d
	testb	%r9b, %r10b
	je	.LBB2_13
# %bb.12:                               #   in Loop: Header=BB2_2 Depth=1
	notb	%r9b
	andb	%r9b, %r10b
	movb	%r10b, (%rdi,%r8)
	movl	$1, %eax
	jmp	.LBB2_13
.LBB2_7:                                #   in Loop: Header=BB2_2 Depth=1
	movb	$8, %r9b
	cmpb	$8, %r10b
	jb	.LBB2_11
# %bb.8:                                #   in Loop: Header=BB2_2 Depth=1
	movb	$16, %r9b
	cmpb	$16, %r10b
	jb	.LBB2_11
# %bb.9:                                #   in Loop: Header=BB2_2 Depth=1
	movb	$32, %r9b
	cmpb	$32, %r10b
	jb	.LBB2_11
# %bb.10:                               #   in Loop: Header=BB2_2 Depth=1
	testb	%r10b, %r10b
	movl	$128, %r9d
	cmovnsl	%ecx, %r9d
	jmp	.LBB2_11
.LBB2_14:
                                        # kill: def $eax killed $eax killed $rax
	retq
.Lfunc_end2:
	.size	has_new_bits, .Lfunc_end2-has_new_bits
                                        # -- End function
	.globl	demo_selftest                   # -- Begin function demo_selftest
	.p2align	4
	.type	demo_selftest,@function
demo_selftest:                          # @demo_selftest
# %bb.0:
	pushq	%r15
	pushq	%r14
	pushq	%rbx
	leaq	demo_selftest.map(%rip), %rbx
	xorl	%r15d, %r15d
	movl	$65536, %edx                    # imm = 0x10000
	movq	%rbx, %rdi
	xorl	%esi, %esi
	callq	memset@PLT
	leaq	demo_selftest.virgin(%rip), %r14
	movl	$65536, %edx                    # imm = 0x10000
	movq	%r14, %rdi
	movl	$255, %esi
	callq	memset@PLT
	movb	$1, demo_selftest.map+10(%rip)
	movb	$1, demo_selftest.map+17(%rip)
	movb	$1, demo_selftest.map+30(%rip)
	movb	$1, demo_selftest.map+20(%rip)
	movl	$64, %eax
	xorl	%ecx, %ecx
	jmp	.LBB3_1
	.p2align	4
.LBB3_12:                               #   in Loop: Header=BB3_1 Depth=1
	incq	%rcx
	cmpq	$65536, %rcx                    # imm = 0x10000
	je	.LBB3_13
.LBB3_1:                                # =>This Inner Loop Header: Depth=1
	movzbl	(%rcx,%rbx), %esi
	movzbl	%sil, %edx
	leal	-1(%rdx), %edi
	cmpl	$2, %edi
	jb	.LBB3_2
# %bb.3:                                #   in Loop: Header=BB3_1 Depth=1
	testl	%edx, %edx
	je	.LBB3_12
# %bb.4:                                #   in Loop: Header=BB3_1 Depth=1
	cmpl	$3, %edx
	jne	.LBB3_6
# %bb.5:                                #   in Loop: Header=BB3_1 Depth=1
	movb	$4, %dl
	jmp	.LBB3_10
	.p2align	4
.LBB3_2:                                #   in Loop: Header=BB3_1 Depth=1
	movl	%esi, %edx
.LBB3_10:                               #   in Loop: Header=BB3_1 Depth=1
	movzbl	(%rcx,%r14), %esi
	testb	%dl, %sil
	je	.LBB3_12
# %bb.11:                               #   in Loop: Header=BB3_1 Depth=1
	notb	%dl
	andb	%dl, %sil
	movb	%sil, (%rcx,%r14)
	movl	$1, %r15d
	jmp	.LBB3_12
.LBB3_6:                                #   in Loop: Header=BB3_1 Depth=1
	movb	$8, %dl
	cmpb	$8, %sil
	jb	.LBB3_10
# %bb.7:                                #   in Loop: Header=BB3_1 Depth=1
	movb	$16, %dl
	cmpb	$16, %sil
	jb	.LBB3_10
# %bb.8:                                #   in Loop: Header=BB3_1 Depth=1
	movb	$32, %dl
	cmpb	$32, %sil
	jb	.LBB3_10
# %bb.9:                                #   in Loop: Header=BB3_1 Depth=1
	testb	%sil, %sil
	movl	$128, %edx
	cmovnsl	%eax, %edx
	jmp	.LBB3_10
.LBB3_13:
	xorl	%ecx, %ecx
	movl	$64, %eax
	xorl	%edx, %edx
	jmp	.LBB3_14
	.p2align	4
.LBB3_25:                               #   in Loop: Header=BB3_14 Depth=1
	incq	%rdx
	cmpq	$65536, %rdx                    # imm = 0x10000
	je	.LBB3_26
.LBB3_14:                               # =>This Inner Loop Header: Depth=1
	movzbl	(%rdx,%rbx), %edi
	movzbl	%dil, %esi
	leal	-1(%rsi), %r8d
	cmpl	$2, %r8d
	jb	.LBB3_15
# %bb.16:                               #   in Loop: Header=BB3_14 Depth=1
	testl	%esi, %esi
	je	.LBB3_25
# %bb.17:                               #   in Loop: Header=BB3_14 Depth=1
	cmpl	$3, %esi
	jne	.LBB3_19
# %bb.18:                               #   in Loop: Header=BB3_14 Depth=1
	movb	$4, %sil
	jmp	.LBB3_23
	.p2align	4
.LBB3_15:                               #   in Loop: Header=BB3_14 Depth=1
	movl	%edi, %esi
.LBB3_23:                               #   in Loop: Header=BB3_14 Depth=1
	movzbl	(%rdx,%r14), %edi
	testb	%sil, %dil
	je	.LBB3_25
# %bb.24:                               #   in Loop: Header=BB3_14 Depth=1
	notb	%sil
	andb	%sil, %dil
	movb	%dil, (%rdx,%r14)
	movl	$1, %ecx
	jmp	.LBB3_25
.LBB3_19:                               #   in Loop: Header=BB3_14 Depth=1
	movb	$8, %sil
	cmpb	$8, %dil
	jb	.LBB3_23
# %bb.20:                               #   in Loop: Header=BB3_14 Depth=1
	movb	$16, %sil
	cmpb	$16, %dil
	jb	.LBB3_23
# %bb.21:                               #   in Loop: Header=BB3_14 Depth=1
	movb	$32, %sil
	cmpb	$32, %dil
	jb	.LBB3_23
# %bb.22:                               #   in Loop: Header=BB3_14 Depth=1
	testb	%dil, %dil
	movl	$128, %esi
	cmovnsl	%eax, %esi
	jmp	.LBB3_23
.LBB3_26:
	leal	(%rcx,%r15,2), %eax
	addl	$60, %eax
	popq	%rbx
	popq	%r14
	popq	%r15
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
