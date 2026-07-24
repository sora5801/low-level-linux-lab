	.file	"demo.c"
	.text
	.globl	ring_next                       # -- Begin function ring_next
	.p2align	4
	.type	ring_next,@function
ring_next:                              # @ring_next
# %bb.0:
                                        # kill: def $esi killed $esi def $rsi
                                        # kill: def $edi killed $edi def $rdi
	leal	1(%rdi), %ecx
	leal	-1(%rsi), %eax
	andl	%ecx, %eax
                                        # kill: def $ax killed $ax killed $eax
	retq
.Lfunc_end0:
	.size	ring_next, .Lfunc_end0-ring_next
                                        # -- End function
	.globl	rx_poll                         # -- Begin function rx_poll
	.p2align	4
	.type	rx_poll,@function
rx_poll:                                # @rx_poll
# %bb.0:
	movzwl	(%rdx), %r9d
	testl	%ecx, %ecx
	je	.LBB1_1
# %bb.3:
	decl	%esi
	xorl	%r10d, %r10d
	xorl	%eax, %eax
	.p2align	4
.LBB1_4:                                # =>This Inner Loop Header: Depth=1
	movzwl	%r9w, %r11d
	shll	$4, %r11d
	testl	$1, 8(%rdi,%r11)
	je	.LBB1_2
# %bb.5:                                #   in Loop: Header=BB1_4 Depth=1
	addq	%rdi, %r11
	#APP
	#NO_APP
	addl	12(%r11), %r10d
	incl	%r9d
	andl	%esi, %r9d
	incl	%eax
	cmpl	%eax, %ecx
	jne	.LBB1_4
# %bb.6:
	movl	%ecx, %eax
	jmp	.LBB1_2
.LBB1_1:
	xorl	%eax, %eax
	xorl	%r10d, %r10d
.LBB1_2:
	movw	%r9w, (%rdx)
	movl	%r10d, (%r8)
	retq
.Lfunc_end1:
	.size	rx_poll, .Lfunc_end1-rx_poll
                                        # -- End function
	.globl	tx_clean                        # -- Begin function tx_clean
	.p2align	4
	.type	tx_clean,@function
tx_clean:                               # @tx_clean
# %bb.0:
	pushq	%rbp
	pushq	%rbx
                                        # kill: def $r8d killed $r8d def $r8
	movzwl	(%rdx), %r11d
	decl	%esi
	leal	-1(%r8), %r9d
	xorl	%ebx, %ebx
	.p2align	4
.LBB2_1:                                # =>This Inner Loop Header: Depth=1
	movl	%r11d, %r10d
	movl	%ebx, %eax
	movl	%ecx, %r11d
	subl	%r10d, %r11d
	andl	%esi, %r11d
	cmpw	%r8w, %r11w
	jb	.LBB2_3
# %bb.2:                                #   in Loop: Header=BB2_1 Depth=1
	leal	(%r10,%r8), %r11d
	leal	(%r10,%r9), %ebx
	andl	%esi, %ebx
	movzwl	%bx, %ebx
	shll	$4, %ebx
	movl	8(%rdi,%rbx), %ebp
	andl	%esi, %r11d
	leal	(%rax,%r8), %ebx
	testb	$1, %bpl
	jne	.LBB2_1
.LBB2_3:
	movw	%r10w, (%rdx)
                                        # kill: def $eax killed $eax killed $rax
	popq	%rbx
	popq	%rbp
	retq
.Lfunc_end2:
	.size	tx_clean, .Lfunc_end2-tx_clean
                                        # -- End function
	.ident	"clang version 20.1.8"
	.section	".note.GNU-stack","",@progbits
	.addrsig
