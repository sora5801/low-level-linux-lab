	.file	"aes_round.c"
	.text
	.globl	aes128_encrypt_block            # -- Begin function aes128_encrypt_block
	.p2align	4
	.type	aes128_encrypt_block,@function
aes128_encrypt_block:                   # @aes128_encrypt_block
# %bb.0:
	movdqu	(%rdi), %xmm0
	pxor	(%rdx), %xmm0
	aesenc	16(%rdx), %xmm0
	aesenc	32(%rdx), %xmm0
	aesenc	48(%rdx), %xmm0
	aesenc	64(%rdx), %xmm0
	aesenc	80(%rdx), %xmm0
	aesenc	96(%rdx), %xmm0
	aesenc	112(%rdx), %xmm0
	aesenc	128(%rdx), %xmm0
	aesenc	144(%rdx), %xmm0
	aesenclast	160(%rdx), %xmm0
	movdqu	%xmm0, (%rsi)
	retq
.Lfunc_end0:
	.size	aes128_encrypt_block, .Lfunc_end0-aes128_encrypt_block
                                        # -- End function
	.ident	"clang version 20.1.8"
	.section	".note.GNU-stack","",@progbits
	.addrsig
