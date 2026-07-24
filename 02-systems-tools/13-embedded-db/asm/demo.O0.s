	.file	"demo.c"
	.text
	.globl	align_up                        # -- Begin function align_up
	.p2align	4
	.type	align_up,@function
align_up:                               # @align_up
# %bb.0:
	pushq	%rbp
	movq	%rsp, %rbp
	movq	%rdi, -8(%rbp)
	movq	%rsi, -16(%rbp)
	movq	-8(%rbp), %rax
	movq	-16(%rbp), %rcx
	subq	$1, %rcx
	addq	%rcx, %rax
	movq	-16(%rbp), %rcx
	subq	$1, %rcx
	xorq	$-1, %rcx
	andq	%rcx, %rax
	popq	%rbp
	retq
.Lfunc_end0:
	.size	align_up, .Lfunc_end0-align_up
                                        # -- End function
	.globl	le16                            # -- Begin function le16
	.p2align	4
	.type	le16,@function
le16:                                   # @le16
# %bb.0:
	pushq	%rbp
	movq	%rsp, %rbp
	movq	%rdi, -8(%rbp)
	movq	-8(%rbp), %rax
	movzbl	(%rax), %eax
                                        # kill: def $ax killed $ax killed $eax
	movzwl	%ax, %eax
	movq	-8(%rbp), %rcx
	movzbl	1(%rcx), %ecx
                                        # kill: def $cx killed $cx killed $ecx
	movzwl	%cx, %ecx
	shll	$8, %ecx
	orl	%ecx, %eax
                                        # kill: def $ax killed $ax killed $eax
	popq	%rbp
	retq
.Lfunc_end1:
	.size	le16, .Lfunc_end1-le16
                                        # -- End function
	.globl	le32                            # -- Begin function le32
	.p2align	4
	.type	le32,@function
le32:                                   # @le32
# %bb.0:
	pushq	%rbp
	movq	%rsp, %rbp
	movq	%rdi, -8(%rbp)
	movq	-8(%rbp), %rax
	movzbl	(%rax), %eax
	movq	-8(%rbp), %rcx
	movzbl	1(%rcx), %ecx
	shll	$8, %ecx
	orl	%ecx, %eax
	movq	-8(%rbp), %rcx
	movzbl	2(%rcx), %ecx
	shll	$16, %ecx
	orl	%ecx, %eax
	movq	-8(%rbp), %rcx
	movzbl	3(%rcx), %ecx
	shll	$24, %ecx
	orl	%ecx, %eax
	popq	%rbp
	retq
.Lfunc_end2:
	.size	le32, .Lfunc_end2-le32
                                        # -- End function
	.globl	key_cmp                         # -- Begin function key_cmp
	.p2align	4
	.type	key_cmp,@function
key_cmp:                                # @key_cmp
# %bb.0:
	pushq	%rbp
	movq	%rsp, %rbp
	movw	%cx, %ax
	movw	%si, %cx
	movq	%rdi, -16(%rbp)
	movw	%cx, -18(%rbp)
	movq	%rdx, -32(%rbp)
	movw	%ax, -34(%rbp)
	movzwl	-18(%rbp), %eax
	movzwl	-34(%rbp), %ecx
	cmpl	%ecx, %eax
	jge	.LBB3_2
# %bb.1:
	movzwl	-18(%rbp), %eax
	movl	%eax, -44(%rbp)                 # 4-byte Spill
	jmp	.LBB3_3
.LBB3_2:
	movzwl	-34(%rbp), %eax
	movl	%eax, -44(%rbp)                 # 4-byte Spill
.LBB3_3:
	movl	-44(%rbp), %eax                 # 4-byte Reload
                                        # kill: def $ax killed $ax killed $eax
	movw	%ax, -36(%rbp)
	movw	$0, -38(%rbp)
.LBB3_4:                                # =>This Inner Loop Header: Depth=1
	movzwl	-38(%rbp), %eax
	movzwl	-36(%rbp), %ecx
	cmpl	%ecx, %eax
	jge	.LBB3_9
# %bb.5:                                #   in Loop: Header=BB3_4 Depth=1
	movq	-16(%rbp), %rax
	movzwl	-38(%rbp), %ecx
                                        # kill: def $rcx killed $ecx
	movzbl	(%rax,%rcx), %eax
	movq	-32(%rbp), %rcx
	movzwl	-38(%rbp), %edx
                                        # kill: def $rdx killed $edx
	movzbl	(%rcx,%rdx), %ecx
	cmpl	%ecx, %eax
	je	.LBB3_7
# %bb.6:
	movq	-16(%rbp), %rax
	movzwl	-38(%rbp), %ecx
                                        # kill: def $rcx killed $ecx
	movzbl	(%rax,%rcx), %edx
	movq	-32(%rbp), %rax
	movzwl	-38(%rbp), %ecx
                                        # kill: def $rcx killed $ecx
	movzbl	(%rax,%rcx), %esi
	movl	$1, %eax
	movl	$4294967295, %ecx               # imm = 0xFFFFFFFF
	cmpl	%esi, %edx
	cmovll	%ecx, %eax
	movl	%eax, -4(%rbp)
	jmp	.LBB3_12
.LBB3_7:                                #   in Loop: Header=BB3_4 Depth=1
	jmp	.LBB3_8
.LBB3_8:                                #   in Loop: Header=BB3_4 Depth=1
	movw	-38(%rbp), %ax
	addw	$1, %ax
	movw	%ax, -38(%rbp)
	jmp	.LBB3_4
.LBB3_9:
	movzwl	-18(%rbp), %eax
	movzwl	-34(%rbp), %ecx
	cmpl	%ecx, %eax
	je	.LBB3_11
# %bb.10:
	movzwl	-18(%rbp), %edx
	movzwl	-34(%rbp), %esi
	movl	$1, %eax
	movl	$4294967295, %ecx               # imm = 0xFFFFFFFF
	cmpl	%esi, %edx
	cmovll	%ecx, %eax
	movl	%eax, -4(%rbp)
	jmp	.LBB3_12
.LBB3_11:
	movl	$0, -4(%rbp)
.LBB3_12:
	movl	-4(%rbp), %eax
	popq	%rbp
	retq
.Lfunc_end3:
	.size	key_cmp, .Lfunc_end3-key_cmp
                                        # -- End function
	.globl	slot_lower_bound                # -- Begin function slot_lower_bound
	.p2align	4
	.type	slot_lower_bound,@function
slot_lower_bound:                       # @slot_lower_bound
# %bb.0:
	pushq	%rbp
	movq	%rsp, %rbp
	subq	$64, %rsp
	movw	%cx, %ax
	movw	%si, %cx
	movq	%rdi, -8(%rbp)
	movw	%cx, -10(%rbp)
	movq	%rdx, -24(%rbp)
	movw	%ax, -26(%rbp)
	movl	$0, -32(%rbp)
	movzwl	-10(%rbp), %eax
	movl	%eax, -36(%rbp)
.LBB4_1:                                # =>This Inner Loop Header: Depth=1
	movl	-32(%rbp), %eax
	cmpl	-36(%rbp), %eax
	jge	.LBB4_6
# %bb.2:                                #   in Loop: Header=BB4_1 Depth=1
	movl	-32(%rbp), %eax
	addl	-36(%rbp), %eax
	sarl	%eax
	movl	%eax, -40(%rbp)
	movq	-8(%rbp), %rdi
	addq	$16, %rdi
	movl	-40(%rbp), %eax
	shll	%eax
	movl	%eax, %eax
                                        # kill: def $rax killed $eax
	addq	%rax, %rdi
	callq	le16
	movw	%ax, -42(%rbp)
	movq	-8(%rbp), %rdi
	movzwl	-42(%rbp), %eax
	cltq
	addq	%rax, %rdi
	callq	le16
	movw	%ax, -44(%rbp)
	movq	-8(%rbp), %rax
	movzwl	-42(%rbp), %ecx
	movslq	%ecx, %rcx
	addq	%rcx, %rax
	addq	$6, %rax
	movq	%rax, -56(%rbp)
	movq	-56(%rbp), %rdi
	movw	-44(%rbp), %ax
	movq	-24(%rbp), %rdx
	movzwl	%ax, %esi
	movzwl	-26(%rbp), %ecx
	callq	key_cmp
	cmpl	$0, %eax
	jge	.LBB4_4
# %bb.3:                                #   in Loop: Header=BB4_1 Depth=1
	movl	-40(%rbp), %eax
	addl	$1, %eax
	movl	%eax, -32(%rbp)
	jmp	.LBB4_5
.LBB4_4:                                #   in Loop: Header=BB4_1 Depth=1
	movl	-40(%rbp), %eax
	movl	%eax, -36(%rbp)
.LBB4_5:                                #   in Loop: Header=BB4_1 Depth=1
	jmp	.LBB4_1
.LBB4_6:
	movl	-32(%rbp), %eax
	addq	$64, %rsp
	popq	%rbp
	retq
.Lfunc_end4:
	.size	slot_lower_bound, .Lfunc_end4-slot_lower_bound
                                        # -- End function
	.globl	crc32_byte                      # -- Begin function crc32_byte
	.p2align	4
	.type	crc32_byte,@function
crc32_byte:                             # @crc32_byte
# %bb.0:
	pushq	%rbp
	movq	%rsp, %rbp
	movb	%sil, %al
	movl	%edi, -4(%rbp)
	movb	%al, -5(%rbp)
	movzbl	-5(%rbp), %eax
	xorl	-4(%rbp), %eax
	movl	%eax, -4(%rbp)
	movl	$0, -12(%rbp)
.LBB5_1:                                # =>This Inner Loop Header: Depth=1
	cmpl	$8, -12(%rbp)
	jge	.LBB5_7
# %bb.2:                                #   in Loop: Header=BB5_1 Depth=1
	movl	-4(%rbp), %eax
	andl	$1, %eax
	cmpl	$0, %eax
	je	.LBB5_4
# %bb.3:                                #   in Loop: Header=BB5_1 Depth=1
	movl	-4(%rbp), %eax
	shrl	%eax
	xorl	$3988292384, %eax               # imm = 0xEDB88320
	movl	%eax, -4(%rbp)
	jmp	.LBB5_5
.LBB5_4:                                #   in Loop: Header=BB5_1 Depth=1
	movl	-4(%rbp), %eax
	shrl	%eax
	movl	%eax, -4(%rbp)
.LBB5_5:                                #   in Loop: Header=BB5_1 Depth=1
	jmp	.LBB5_6
.LBB5_6:                                #   in Loop: Header=BB5_1 Depth=1
	movl	-12(%rbp), %eax
	addl	$1, %eax
	movl	%eax, -12(%rbp)
	jmp	.LBB5_1
.LBB5_7:
	movl	-4(%rbp), %eax
	popq	%rbp
	retq
.Lfunc_end5:
	.size	crc32_byte, .Lfunc_end5-crc32_byte
                                        # -- End function
	.ident	"clang version 20.1.8"
	.section	".note.GNU-stack","",@progbits
	.addrsig
	.addrsig_sym le16
	.addrsig_sym key_cmp
