	.file	"demo.c"
	.text
	.globl	align_up                        # -- Begin function align_up
	.p2align	4
	.type	align_up,@function
align_up:                               # @align_up
# %bb.0:
	leaq	(%rdi,%rsi), %rax
	decq	%rax
	negq	%rsi
	andq	%rsi, %rax
	retq
.Lfunc_end0:
	.size	align_up, .Lfunc_end0-align_up
                                        # -- End function
	.globl	le16                            # -- Begin function le16
	.p2align	4
	.type	le16,@function
le16:                                   # @le16
# %bb.0:
	movzwl	(%rdi), %eax
	retq
.Lfunc_end1:
	.size	le16, .Lfunc_end1-le16
                                        # -- End function
	.globl	le32                            # -- Begin function le32
	.p2align	4
	.type	le32,@function
le32:                                   # @le32
# %bb.0:
	movl	(%rdi), %eax
	retq
.Lfunc_end2:
	.size	le32, .Lfunc_end2-le32
                                        # -- End function
	.globl	key_cmp                         # -- Begin function key_cmp
	.p2align	4
	.type	key_cmp,@function
key_cmp:                                # @key_cmp
# %bb.0:
	cmpw	%cx, %si
	movl	%ecx, %eax
	cmovbl	%esi, %eax
	testl	%eax, %eax
	je	.LBB3_5
# %bb.1:
	movzwl	%ax, %eax
	xorl	%r8d, %r8d
	.p2align	4
.LBB3_3:                                # =>This Inner Loop Header: Depth=1
	movzbl	(%rdi,%r8), %r9d
	movzbl	(%rdx,%r8), %r10d
	cmpb	%r10b, %r9b
	jne	.LBB3_4
# %bb.2:                                #   in Loop: Header=BB3_3 Depth=1
	incq	%r8
	cmpq	%r8, %rax
	jne	.LBB3_3
.LBB3_5:
	cmpw	%cx, %si
	seta	%al
	sbbb	$0, %al
	movsbl	%al, %eax
	retq
.LBB3_4:
	xorl	%eax, %eax
	cmpb	%r10b, %r9b
	sbbl	%eax, %eax
	orl	$1, %eax
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
	pushq	%rbx
	movzwl	%si, %esi
	xorl	%eax, %eax
	jmp	.LBB4_4
	.p2align	4
.LBB4_8:                                #   in Loop: Header=BB4_4 Depth=1
	cmpw	%cx, %r10w
.LBB4_9:                                #   in Loop: Header=BB4_4 Depth=1
	setb	%r9b
	sarl	%r8d
	leal	1(%r8), %r10d
	testb	%r9b, %r9b
	cmovnel	%esi, %r8d
	cmovnel	%r10d, %eax
	movl	%r8d, %esi
	cmpl	%r8d, %eax
	jge	.LBB4_10
.LBB4_4:                                # =>This Loop Header: Depth=1
                                        #     Child Loop BB4_7 Depth 2
	leal	(%rax,%rsi), %r8d
	movl	%r8d, %r9d
	andl	$-2, %r9d
	movzwl	16(%rdi,%r9), %r9d
	movzwl	(%rdi,%r9), %r10d
	cmpw	%cx, %r10w
	movl	%r10d, %r11d
	cmovael	%ecx, %r11d
	testw	%r11w, %r11w
	je	.LBB4_8
# %bb.5:                                #   in Loop: Header=BB4_4 Depth=1
	addq	%rdi, %r9
	movzwl	%r11w, %r11d
	xorl	%ebx, %ebx
	.p2align	4
.LBB4_7:                                #   Parent Loop BB4_4 Depth=1
                                        # =>  This Inner Loop Header: Depth=2
	movzbl	(%rdx,%rbx), %ebp
	cmpb	%bpl, 6(%r9,%rbx)
	jne	.LBB4_9
# %bb.6:                                #   in Loop: Header=BB4_7 Depth=2
	incq	%rbx
	cmpq	%rbx, %r11
	jne	.LBB4_7
	jmp	.LBB4_8
.LBB4_10:
	popq	%rbx
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
	xorl	%esi, %edi
	movl	%edi, %eax
	shrl	%eax
	movl	%eax, %ecx
	xorl	$-306674912, %ecx               # imm = 0xEDB88320
	testb	$1, %dil
	cmovel	%eax, %ecx
	movl	%ecx, %eax
	shrl	%eax
	movl	%eax, %edx
	xorl	$-306674912, %edx               # imm = 0xEDB88320
	testb	$1, %cl
	cmovel	%eax, %edx
	movl	%edx, %eax
	shrl	%eax
	movl	%eax, %ecx
	xorl	$-306674912, %ecx               # imm = 0xEDB88320
	testb	$1, %dl
	cmovel	%eax, %ecx
	movl	%ecx, %eax
	shrl	%eax
	movl	%eax, %edx
	xorl	$-306674912, %edx               # imm = 0xEDB88320
	testb	$1, %cl
	cmovel	%eax, %edx
	movl	%edx, %eax
	shrl	%eax
	movl	%eax, %ecx
	xorl	$-306674912, %ecx               # imm = 0xEDB88320
	testb	$1, %dl
	cmovel	%eax, %ecx
	movl	%ecx, %eax
	shrl	%eax
	movl	%eax, %edx
	xorl	$-306674912, %edx               # imm = 0xEDB88320
	testb	$1, %cl
	cmovel	%eax, %edx
	movl	%edx, %eax
	shrl	%eax
	movl	%eax, %ecx
	xorl	$-306674912, %ecx               # imm = 0xEDB88320
	testb	$1, %dl
	cmovel	%eax, %ecx
	movl	%ecx, %edx
	shrl	%edx
	movl	%edx, %eax
	xorl	$-306674912, %eax               # imm = 0xEDB88320
	testb	$1, %cl
	cmovel	%edx, %eax
	retq
.Lfunc_end5:
	.size	crc32_byte, .Lfunc_end5-crc32_byte
                                        # -- End function
	.ident	"clang version 20.1.8"
	.section	".note.GNU-stack","",@progbits
	.addrsig
