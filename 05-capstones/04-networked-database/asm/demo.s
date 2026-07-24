	.file	"demo.c"
	.text
	.globl	crc32_ieee                      # -- Begin function crc32_ieee
	.p2align	4
	.type	crc32_ieee,@function
crc32_ieee:                             # @crc32_ieee
# %bb.0:
	pushq	%rbp
	movq	%rsp, %rbp
	testq	%rsi, %rsi
	je	.LBB0_1
# %bb.4:
	movl	$-1, %eax
	xorl	%ecx, %ecx
	.p2align	4
.LBB0_5:                                # =>This Loop Header: Depth=1
                                        #     Child Loop BB0_6 Depth 2
	movzbl	(%rdi,%rcx), %edx
	xorl	%edx, %eax
	movl	$8, %edx
	.p2align	4
.LBB0_6:                                #   Parent Loop BB0_5 Depth=1
                                        # =>  This Inner Loop Header: Depth=2
	movl	%eax, %r8d
	shrl	%r8d
	andl	$1, %eax
	negl	%eax
	andl	$-306674912, %eax               # imm = 0xEDB88320
	xorl	%r8d, %eax
	decl	%edx
	jne	.LBB0_6
# %bb.7:                                #   in Loop: Header=BB0_5 Depth=1
	incq	%rcx
	cmpq	%rsi, %rcx
	jne	.LBB0_5
# %bb.2:
	notl	%eax
	popq	%rbp
	retq
.LBB0_1:
	xorl	%eax, %eax
	popq	%rbp
	retq
.Lfunc_end0:
	.size	crc32_ieee, .Lfunc_end0-crc32_ieee
                                        # -- End function
	.globl	wal_frame_record                # -- Begin function wal_frame_record
	.p2align	4
	.type	wal_frame_record,@function
wal_frame_record:                       # @wal_frame_record
# %bb.0:
	pushq	%rbp
	movq	%rsp, %rbp
	xorl	%r10d, %r10d
	cmpl	$2, %esi
	cmovnel	%r9d, %r10d
	leaq	8(%rdi), %r9
	movb	%sil, 8(%rdi)
	movb	%cl, 9(%rdi)
	movb	%ch, 10(%rdi)
	movl	%ecx, %eax
	shrl	$16, %eax
	movb	%al, 11(%rdi)
	movl	%ecx, %eax
	shrl	$24, %eax
	movb	%al, 12(%rdi)
	movl	%ecx, %esi
	testl	%ecx, %ecx
	je	.LBB1_3
# %bb.1:
	xorl	%eax, %eax
	.p2align	4
.LBB1_2:                                # =>This Inner Loop Header: Depth=1
	movzbl	(%rdx,%rax), %ecx
	movb	%cl, 13(%rdi,%rax)
	incq	%rax
	cmpq	%rax, %rsi
	jne	.LBB1_2
.LBB1_3:
	movl	%r10d, 5(%r9,%rsi)
	leaq	9(%rsi), %rax
	movl	%r10d, %ecx
	testl	%r10d, %r10d
	je	.LBB1_6
# %bb.4:
	leaq	(%r9,%rsi), %rdx
	addq	$9, %rdx
	xorl	%esi, %esi
	.p2align	4
.LBB1_5:                                # =>This Inner Loop Header: Depth=1
	movzbl	(%r8,%rsi), %r10d
	movb	%r10b, (%rdx,%rsi)
	incq	%rsi
	cmpq	%rsi, %rcx
	jne	.LBB1_5
.LBB1_6:
	addq	%rcx, %rax
	movl	%eax, (%rdi)
	movl	$-1, %ecx
	xorl	%edx, %edx
	.p2align	4
.LBB1_7:                                # =>This Loop Header: Depth=1
                                        #     Child Loop BB1_8 Depth 2
	movzbl	(%r9,%rdx), %esi
	xorl	%esi, %ecx
	movl	$8, %esi
	.p2align	4
.LBB1_8:                                #   Parent Loop BB1_7 Depth=1
                                        # =>  This Inner Loop Header: Depth=2
	movl	%ecx, %r8d
	shrl	%r8d
	andl	$1, %ecx
	negl	%ecx
	andl	$-306674912, %ecx               # imm = 0xEDB88320
	xorl	%r8d, %ecx
	decl	%esi
	jne	.LBB1_8
# %bb.9:                                #   in Loop: Header=BB1_7 Depth=1
	incq	%rdx
	cmpq	%rax, %rdx
	jne	.LBB1_7
# %bb.10:
	notl	%ecx
	movl	%ecx, 4(%rdi)
	addq	$8, %rax
	popq	%rbp
	retq
.Lfunc_end1:
	.size	wal_frame_record, .Lfunc_end1-wal_frame_record
                                        # -- End function
	.ident	"clang version 20.1.8"
	.section	".note.GNU-stack","",@progbits
	.addrsig
