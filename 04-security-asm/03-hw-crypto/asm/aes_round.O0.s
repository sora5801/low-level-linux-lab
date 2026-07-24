	.file	"aes_round.c"
	.text
	.globl	aes128_encrypt_block            # -- Begin function aes128_encrypt_block
	.p2align	4
	.type	aes128_encrypt_block,@function
aes128_encrypt_block:                   # @aes128_encrypt_block
# %bb.0:
	pushq	%rbp
	movq	%rsp, %rbp
	subq	$320, %rsp                      # imm = 0x140
	movq	%rdi, -400(%rbp)
	movq	%rsi, -408(%rbp)
	movq	%rdx, -416(%rbp)
	movq	-416(%rbp), %rax
	movq	%rax, -424(%rbp)
	movq	-400(%rbp), %rax
	movq	%rax, -392(%rbp)
	movq	-392(%rbp), %rax
	movdqu	(%rax), %xmm0
	movdqa	%xmm0, -448(%rbp)
	movdqa	-448(%rbp), %xmm1
	movq	-424(%rbp), %rax
	movdqa	(%rax), %xmm0
	movdqa	%xmm1, -368(%rbp)
	movdqa	%xmm0, -384(%rbp)
	movdqa	-368(%rbp), %xmm0
	pxor	-384(%rbp), %xmm0
	movdqa	%xmm0, -448(%rbp)
	movdqa	-448(%rbp), %xmm1
	movq	-424(%rbp), %rax
	movdqa	16(%rax), %xmm0
	movdqa	%xmm1, -80(%rbp)
	movdqa	%xmm0, -96(%rbp)
	movdqa	-80(%rbp), %xmm0
	movdqa	-96(%rbp), %xmm1
	aesenc	%xmm1, %xmm0
	movdqa	%xmm0, -448(%rbp)
	movdqa	-448(%rbp), %xmm1
	movq	-424(%rbp), %rax
	movdqa	32(%rax), %xmm0
	movdqa	%xmm1, -112(%rbp)
	movdqa	%xmm0, -128(%rbp)
	movdqa	-112(%rbp), %xmm0
	movdqa	-128(%rbp), %xmm1
	aesenc	%xmm1, %xmm0
	movdqa	%xmm0, -448(%rbp)
	movdqa	-448(%rbp), %xmm1
	movq	-424(%rbp), %rax
	movdqa	48(%rax), %xmm0
	movdqa	%xmm1, -144(%rbp)
	movdqa	%xmm0, -160(%rbp)
	movdqa	-144(%rbp), %xmm0
	movdqa	-160(%rbp), %xmm1
	aesenc	%xmm1, %xmm0
	movdqa	%xmm0, -448(%rbp)
	movdqa	-448(%rbp), %xmm1
	movq	-424(%rbp), %rax
	movdqa	64(%rax), %xmm0
	movdqa	%xmm1, -176(%rbp)
	movdqa	%xmm0, -192(%rbp)
	movdqa	-176(%rbp), %xmm0
	movdqa	-192(%rbp), %xmm1
	aesenc	%xmm1, %xmm0
	movdqa	%xmm0, -448(%rbp)
	movdqa	-448(%rbp), %xmm1
	movq	-424(%rbp), %rax
	movdqa	80(%rax), %xmm0
	movdqa	%xmm1, -208(%rbp)
	movdqa	%xmm0, -224(%rbp)
	movdqa	-208(%rbp), %xmm0
	movdqa	-224(%rbp), %xmm1
	aesenc	%xmm1, %xmm0
	movdqa	%xmm0, -448(%rbp)
	movdqa	-448(%rbp), %xmm1
	movq	-424(%rbp), %rax
	movdqa	96(%rax), %xmm0
	movdqa	%xmm1, -240(%rbp)
	movdqa	%xmm0, -256(%rbp)
	movdqa	-240(%rbp), %xmm0
	movdqa	-256(%rbp), %xmm1
	aesenc	%xmm1, %xmm0
	movdqa	%xmm0, -448(%rbp)
	movdqa	-448(%rbp), %xmm1
	movq	-424(%rbp), %rax
	movdqa	112(%rax), %xmm0
	movdqa	%xmm1, -272(%rbp)
	movdqa	%xmm0, -288(%rbp)
	movdqa	-272(%rbp), %xmm0
	movdqa	-288(%rbp), %xmm1
	aesenc	%xmm1, %xmm0
	movdqa	%xmm0, -448(%rbp)
	movdqa	-448(%rbp), %xmm1
	movq	-424(%rbp), %rax
	movdqa	128(%rax), %xmm0
	movdqa	%xmm1, -304(%rbp)
	movdqa	%xmm0, -320(%rbp)
	movdqa	-304(%rbp), %xmm0
	movdqa	-320(%rbp), %xmm1
	aesenc	%xmm1, %xmm0
	movdqa	%xmm0, -448(%rbp)
	movdqa	-448(%rbp), %xmm1
	movq	-424(%rbp), %rax
	movdqa	144(%rax), %xmm0
	movdqa	%xmm1, -336(%rbp)
	movdqa	%xmm0, -352(%rbp)
	movdqa	-336(%rbp), %xmm0
	movdqa	-352(%rbp), %xmm1
	aesenc	%xmm1, %xmm0
	movdqa	%xmm0, -448(%rbp)
	movdqa	-448(%rbp), %xmm1
	movq	-424(%rbp), %rax
	movdqa	160(%rax), %xmm0
	movdqa	%xmm1, -48(%rbp)
	movdqa	%xmm0, -64(%rbp)
	movdqa	-48(%rbp), %xmm0
	movdqa	-64(%rbp), %xmm1
	aesenclast	%xmm1, %xmm0
	movdqa	%xmm0, -448(%rbp)
	movq	-408(%rbp), %rax
	movdqa	-448(%rbp), %xmm0
	movq	%rax, -8(%rbp)
	movdqa	%xmm0, -32(%rbp)
	movdqa	-32(%rbp), %xmm0
	movq	-8(%rbp), %rax
	movdqu	%xmm0, (%rax)
	addq	$320, %rsp                      # imm = 0x140
	popq	%rbp
	retq
.Lfunc_end0:
	.size	aes128_encrypt_block, .Lfunc_end0-aes128_encrypt_block
                                        # -- End function
	.ident	"clang version 20.1.8"
	.section	".note.GNU-stack","",@progbits
	.addrsig
