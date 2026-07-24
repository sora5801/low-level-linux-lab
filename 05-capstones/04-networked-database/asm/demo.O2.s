	.file	"demo.c"
	.section	.rodata.cst16,"aM",@progbits,16
	.p2align	4, 0x0                          # -- Begin function crc32_ieee
.LCPI0_0:
	.long	4                               # 0x4
	.long	8                               # 0x8
	.long	16                              # 0x10
	.long	32                              # 0x20
.LCPI0_1:
	.long	124634137                       # 0x76dc419
	.long	249268274                       # 0xedb8832
	.long	498536548                       # 0x1db71064
	.long	997073096                       # 0x3b6e20c8
	.text
	.globl	crc32_ieee
	.p2align	4
	.type	crc32_ieee,@function
crc32_ieee:                             # @crc32_ieee
# %bb.0:
	testq	%rsi, %rsi
	je	.LBB0_1
# %bb.3:
	movl	$-1, %eax
	xorl	%ecx, %ecx
	movdqa	.LCPI0_0(%rip), %xmm0           # xmm0 = [4,8,16,32]
	movdqa	.LCPI0_1(%rip), %xmm1           # xmm1 = [124634137,249268274,498536548,997073096]
	.p2align	4
.LBB0_4:                                # =>This Inner Loop Header: Depth=1
	movzbl	(%rdi,%rcx), %edx
	xorl	%eax, %edx
	movl	%edx, %r8d
	shrl	%r8d
	movl	%edx, %eax
	andl	$1, %eax
	negl	%eax
	andl	$-306674912, %eax               # imm = 0xEDB88320
	xorl	%r8d, %eax
	movl	%eax, %r8d
	shrl	%r8d
	movd	%edx, %xmm2
	shll	$30, %edx
	sarl	$31, %edx
	andl	$-306674912, %edx               # imm = 0xEDB88320
	xorl	%r8d, %edx
	movl	%edx, %r8d
	shrl	$6, %r8d
	pshufd	$0, %xmm2, %xmm2                # xmm2 = xmm2[0,0,0,0]
	pand	%xmm0, %xmm2
	pcmpeqd	%xmm0, %xmm2
	pand	%xmm1, %xmm2
	shll	$26, %eax
	sarl	$31, %eax
	andl	$1994146192, %eax               # imm = 0x76DC4190
	shll	$26, %edx
	sarl	$31, %edx
	andl	$-306674912, %edx               # imm = 0xEDB88320
	xorl	%r8d, %edx
	xorl	%eax, %edx
	pshufd	$238, %xmm2, %xmm3              # xmm3 = xmm2[2,3,2,3]
	pxor	%xmm2, %xmm3
	pshufd	$85, %xmm3, %xmm2               # xmm2 = xmm3[1,1,1,1]
	pxor	%xmm3, %xmm2
	movd	%xmm2, %eax
	xorl	%edx, %eax
	incq	%rcx
	cmpq	%rcx, %rsi
	jne	.LBB0_4
# %bb.5:
	notl	%eax
	retq
.LBB0_1:
	xorl	%eax, %eax
	retq
.Lfunc_end0:
	.size	crc32_ieee, .Lfunc_end0-crc32_ieee
                                        # -- End function
	.section	.rodata.cst16,"aM",@progbits,16
	.p2align	4, 0x0                          # -- Begin function wal_frame_record
.LCPI1_0:
	.long	8                               # 0x8
	.long	4                               # 0x4
	.long	16                              # 0x10
	.long	32                              # 0x20
.LCPI1_1:
	.long	249268274                       # 0xedb8832
	.long	124634137                       # 0x76dc419
	.long	498536548                       # 0x1db71064
	.long	997073096                       # 0x3b6e20c8
	.text
	.globl	wal_frame_record
	.p2align	4
	.type	wal_frame_record,@function
wal_frame_record:                       # @wal_frame_record
# %bb.0:
	pushq	%rbp
	pushq	%rbx
	xorl	%r11d, %r11d
	cmpl	$2, %esi
	cmovel	%r11d, %r9d
	leaq	8(%rdi), %r10
	movb	%sil, 8(%rdi)
	movb	%cl, 9(%rdi)
	movb	%ch, 10(%rdi)
	movl	%ecx, %eax
	shrl	$16, %eax
	movb	%al, 11(%rdi)
	movl	%ecx, %eax
	shrl	$24, %eax
	movb	%al, 12(%rdi)
	testl	%ecx, %ecx
	je	.LBB1_19
# %bb.1:
	movl	%ecx, %r11d
	cmpl	$4, %ecx
	jb	.LBB1_2
# %bb.3:
	movq	%rdi, %rax
	subq	%rdx, %rax
	addq	$13, %rax
	cmpq	$32, %rax
	jae	.LBB1_5
.LBB1_2:
	xorl	%eax, %eax
.LBB1_14:
	movq	%r11, %rsi
	movq	%rax, %rcx
	andq	$3, %rsi
	je	.LBB1_17
# %bb.15:
	movq	%rax, %rcx
	.p2align	4
.LBB1_16:                               # =>This Inner Loop Header: Depth=1
	movzbl	(%rdx,%rcx), %ebx
	movb	%bl, 13(%rdi,%rcx)
	incq	%rcx
	decq	%rsi
	jne	.LBB1_16
.LBB1_17:
	subq	%r11, %rax
	cmpq	$-4, %rax
	ja	.LBB1_19
	.p2align	4
.LBB1_18:                               # =>This Inner Loop Header: Depth=1
	movzbl	(%rdx,%rcx), %eax
	movb	%al, 13(%rdi,%rcx)
	movzbl	1(%rdx,%rcx), %eax
	movb	%al, 14(%rdi,%rcx)
	movzbl	2(%rdx,%rcx), %eax
	movb	%al, 15(%rdi,%rcx)
	movzbl	3(%rdx,%rcx), %eax
	movb	%al, 16(%rdi,%rcx)
	addq	$4, %rcx
	cmpq	%rcx, %r11
	jne	.LBB1_18
.LBB1_19:
	movl	%r9d, 5(%r10,%r11)
	leaq	9(%r11), %rax
	testl	%r9d, %r9d
	je	.LBB1_22
# %bb.20:
	leaq	(%r10,%r11), %rdx
	addq	$9, %rdx
	movl	%r9d, %ecx
	cmpl	$4, %r9d
	jb	.LBB1_21
# %bb.23:
	leaq	(%r11,%rdi), %rsi
	subq	%r8, %rsi
	addq	$17, %rsi
	cmpq	$32, %rsi
	jae	.LBB1_25
.LBB1_21:
	xorl	%esi, %esi
.LBB1_34:
	movq	%rcx, %rbx
	movq	%rsi, %r9
	andq	$3, %rbx
	je	.LBB1_37
# %bb.35:
	movq	%rsi, %r9
	.p2align	4
.LBB1_36:                               # =>This Inner Loop Header: Depth=1
	movzbl	(%r8,%r9), %ebp
	movb	%bpl, (%rdx,%r9)
	incq	%r9
	decq	%rbx
	jne	.LBB1_36
.LBB1_37:
	subq	%rcx, %rsi
	cmpq	$-4, %rsi
	ja	.LBB1_40
# %bb.38:
	leaq	(%r11,%rdi), %rdx
	addq	$20, %rdx
	.p2align	4
.LBB1_39:                               # =>This Inner Loop Header: Depth=1
	movzbl	(%r8,%r9), %esi
	movb	%sil, -3(%rdx,%r9)
	movzbl	1(%r8,%r9), %esi
	movb	%sil, -2(%rdx,%r9)
	movzbl	2(%r8,%r9), %esi
	movb	%sil, -1(%rdx,%r9)
	movzbl	3(%r8,%r9), %esi
	movb	%sil, (%rdx,%r9)
	addq	$4, %r9
	cmpq	%r9, %rcx
	jne	.LBB1_39
	jmp	.LBB1_40
.LBB1_22:
	xorl	%ecx, %ecx
.LBB1_40:
	addq	%rcx, %rax
	movl	%eax, (%rdi)
	addq	%r11, %rcx
	addq	$9, %rcx
	movl	$-1, %r8d
	xorl	%edx, %edx
	movdqa	.LCPI1_0(%rip), %xmm0           # xmm0 = [8,4,16,32]
	movdqa	.LCPI1_1(%rip), %xmm1           # xmm1 = [249268274,124634137,498536548,997073096]
	.p2align	4
.LBB1_41:                               # =>This Inner Loop Header: Depth=1
	movzbl	(%r10,%rdx), %esi
	xorl	%r8d, %esi
	movl	%esi, %r9d
	shrl	%r9d
	movl	%esi, %r8d
	andl	$1, %r8d
	negl	%r8d
	andl	$-306674912, %r8d               # imm = 0xEDB88320
	xorl	%r9d, %r8d
	movl	%r8d, %r9d
	shrl	%r9d
	movd	%esi, %xmm2
	shll	$30, %esi
	sarl	$31, %esi
	andl	$-306674912, %esi               # imm = 0xEDB88320
	xorl	%r9d, %esi
	movl	%esi, %r9d
	shrl	$6, %r9d
	pshufd	$0, %xmm2, %xmm2                # xmm2 = xmm2[0,0,0,0]
	pand	%xmm0, %xmm2
	pcmpeqd	%xmm0, %xmm2
	pand	%xmm1, %xmm2
	shll	$26, %r8d
	sarl	$31, %r8d
	andl	$1994146192, %r8d               # imm = 0x76DC4190
	shll	$26, %esi
	sarl	$31, %esi
	andl	$-306674912, %esi               # imm = 0xEDB88320
	xorl	%r9d, %esi
	xorl	%r8d, %esi
	pshufd	$238, %xmm2, %xmm3              # xmm3 = xmm2[2,3,2,3]
	pxor	%xmm2, %xmm3
	pshufd	$85, %xmm3, %xmm2               # xmm2 = xmm3[1,1,1,1]
	pxor	%xmm3, %xmm2
	movd	%xmm2, %r8d
	xorl	%esi, %r8d
	incq	%rdx
	cmpq	%rdx, %rcx
	jne	.LBB1_41
# %bb.42:
	notl	%r8d
	movl	%r8d, 4(%rdi)
	addq	$8, %rax
	popq	%rbx
	popq	%rbp
	retq
.LBB1_5:
	cmpl	$32, %ecx
	jae	.LBB1_7
# %bb.6:
	xorl	%eax, %eax
	jmp	.LBB1_11
.LBB1_25:
	cmpl	$32, %r9d
	jae	.LBB1_27
# %bb.26:
	xorl	%esi, %esi
	jmp	.LBB1_31
.LBB1_7:
	movl	%r11d, %eax
	andl	$-32, %eax
	xorl	%ecx, %ecx
	.p2align	4
.LBB1_8:                                # =>This Inner Loop Header: Depth=1
	movups	(%rdx,%rcx), %xmm0
	movups	16(%rdx,%rcx), %xmm1
	movups	%xmm0, 13(%rdi,%rcx)
	movups	%xmm1, 29(%rdi,%rcx)
	addq	$32, %rcx
	cmpq	%rcx, %rax
	jne	.LBB1_8
# %bb.9:
	cmpl	%r11d, %eax
	je	.LBB1_19
# %bb.10:
	testb	$28, %r11b
	je	.LBB1_14
.LBB1_11:
	movq	%rax, %rcx
	movl	%r11d, %eax
	andl	$-4, %eax
	.p2align	4
.LBB1_12:                               # =>This Inner Loop Header: Depth=1
	movl	(%rdx,%rcx), %esi
	movl	%esi, 13(%rdi,%rcx)
	addq	$4, %rcx
	cmpq	%rcx, %rax
	jne	.LBB1_12
# %bb.13:
	cmpl	%r11d, %eax
	je	.LBB1_19
	jmp	.LBB1_14
.LBB1_27:
	movl	%ecx, %esi
	andl	$-32, %esi
	leaq	(%r11,%rdi), %r9
	addq	$33, %r9
	xorl	%ebx, %ebx
	.p2align	4
.LBB1_28:                               # =>This Inner Loop Header: Depth=1
	movups	(%r8,%rbx), %xmm0
	movups	16(%r8,%rbx), %xmm1
	movups	%xmm0, -16(%r9,%rbx)
	movups	%xmm1, (%r9,%rbx)
	addq	$32, %rbx
	cmpq	%rbx, %rsi
	jne	.LBB1_28
# %bb.29:
	cmpl	%ecx, %esi
	je	.LBB1_40
# %bb.30:
	testb	$28, %cl
	je	.LBB1_34
.LBB1_31:
	movq	%rsi, %r9
	movl	%ecx, %esi
	andl	$-4, %esi
	.p2align	4
.LBB1_32:                               # =>This Inner Loop Header: Depth=1
	movl	(%r8,%r9), %ebx
	movl	%ebx, (%rdx,%r9)
	addq	$4, %r9
	cmpq	%r9, %rsi
	jne	.LBB1_32
# %bb.33:
	cmpl	%ecx, %esi
	je	.LBB1_40
	jmp	.LBB1_34
.Lfunc_end1:
	.size	wal_frame_record, .Lfunc_end1-wal_frame_record
                                        # -- End function
	.ident	"clang version 20.1.8"
	.section	".note.GNU-stack","",@progbits
	.addrsig
