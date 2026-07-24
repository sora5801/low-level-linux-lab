	.file	"demo.c"
	.text
	.globl	align_up                        # -- Begin function align_up
	.p2align	4
	.type	align_up,@function
align_up:                               # @align_up
# %bb.0:
	pushq	%rbp
	movq	%rsp, %rbp
	leaq	(%rdi,%rsi), %rax
	decq	%rax
	negq	%rsi
	andq	%rsi, %rax
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
	movzwl	(%rdi), %eax
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
	movl	(%rdi), %eax
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
	cmpw	%cx, %si
	movl	%ecx, %eax
	cmovbl	%esi, %eax
	testl	%eax, %eax
	je	.LBB3_1
# %bb.3:
	movzwl	%cx, %r8d
	movzwl	%si, %eax
	cmpq	%rax, %r8
	cmovbq	%r8, %rax
	xorl	%r8d, %r8d
	.p2align	4
.LBB3_4:                                # =>This Inner Loop Header: Depth=1
	movzbl	(%rdi,%r8), %r9d
	movzbl	(%rdx,%r8), %r10d
	cmpb	%r10b, %r9b
	jne	.LBB3_5
# %bb.2:                                #   in Loop: Header=BB3_4 Depth=1
	incq	%r8
	cmpw	%r8w, %ax
	jne	.LBB3_4
.LBB3_1:
	xorl	%edi, %edi
                                        # implicit-def: $edx
	jmp	.LBB3_6
.LBB3_5:
	xorl	%edx, %edx
	cmpb	%r10b, %r9b
	sbbl	%edx, %edx
	orl	$1, %edx
	movb	$1, %dil
.LBB3_6:
	cmpw	%cx, %si
	seta	%al
	sbbb	$0, %al
	movsbl	%al, %eax
	testb	%dil, %dil
	cmovnel	%edx, %eax
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
	testl	%esi, %esi
	je	.LBB4_1
# %bb.3:
	pushq	%rbp
	movq	%rsp, %rbp
	pushq	%r14
	pushq	%rbx
	movzwl	%si, %esi
	xorl	%eax, %eax
	jmp	.LBB4_4
	.p2align	4
.LBB4_5:                                #   in Loop: Header=BB4_4 Depth=1
	xorl	%r10d, %r10d
	xorl	%r11d, %r11d
.LBB4_10:                               #   in Loop: Header=BB4_4 Depth=1
	sarl	%r8d
	xorl	%ebx, %ebx
	cmpw	%cx, %r9w
	setb	%bl
	movzbl	%r11b, %r9d
	testb	%r10b, %r10b
	cmovel	%ebx, %r9d
	leal	1(%r8), %r10d
	testb	%r9b, %r9b
	cmovnel	%esi, %r8d
	cmovnel	%r10d, %eax
	movl	%r8d, %esi
	cmpl	%r8d, %eax
	jge	.LBB4_11
.LBB4_4:                                # =>This Loop Header: Depth=1
                                        #     Child Loop BB4_8 Depth 2
	leal	(%rax,%rsi), %r8d
	movl	%r8d, %r9d
	andl	$-2, %r9d
	movzwl	16(%rdi,%r9), %r10d
	movzwl	(%rdi,%r10), %r9d
	cmpw	%cx, %r9w
	movl	%r9d, %r11d
	cmovael	%ecx, %r11d
	testw	%r11w, %r11w
	je	.LBB4_5
# %bb.7:                                #   in Loop: Header=BB4_4 Depth=1
	addq	%rdi, %r10
	movzwl	%r11w, %r11d
	xorl	%ebx, %ebx
	.p2align	4
.LBB4_8:                                #   Parent Loop BB4_4 Depth=1
                                        # =>  This Inner Loop Header: Depth=2
	movzbl	(%rdx,%rbx), %r14d
	cmpb	%r14b, 6(%r10,%rbx)
	jne	.LBB4_9
# %bb.6:                                #   in Loop: Header=BB4_8 Depth=2
	incq	%rbx
	cmpq	%rbx, %r11
	jne	.LBB4_8
	jmp	.LBB4_5
	.p2align	4
.LBB4_9:                                #   in Loop: Header=BB4_4 Depth=1
	setb	%r11b
	movb	$1, %r10b
	jmp	.LBB4_10
.LBB4_11:
	popq	%rbx
	popq	%r14
	popq	%rbp
                                        # kill: def $eax killed $eax killed $rax
	retq
.LBB4_1:
	xorl	%eax, %eax
                                        # kill: def $eax killed $eax killed $rax
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
	movl	%edi, %eax
	xorl	%esi, %eax
	movl	$8, %ecx
	.p2align	4
.LBB5_1:                                # =>This Inner Loop Header: Depth=1
	movl	%eax, %edx
	shrl	%edx
	movl	%edx, %esi
	xorl	$-306674912, %esi               # imm = 0xEDB88320
	testb	$1, %al
	movl	%esi, %eax
	cmovel	%edx, %eax
	decl	%ecx
	jne	.LBB5_1
# %bb.2:
	popq	%rbp
	retq
.Lfunc_end5:
	.size	crc32_byte, .Lfunc_end5-crc32_byte
                                        # -- End function
	.ident	"clang version 20.1.8"
	.section	".note.GNU-stack","",@progbits
	.addrsig
