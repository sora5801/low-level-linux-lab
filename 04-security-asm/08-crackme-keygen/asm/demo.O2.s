	.file	"demo.c"
	.text
	.globl	key_from_name                   # -- Begin function key_from_name
	.p2align	4
	.type	key_from_name,@function
key_from_name:                          # @key_from_name
# %bb.0:
	movabsq	$-3750763034362895579, %r8      # imm = 0xCBF29CE484222325
	movzbl	(%rdi), %esi
	testb	%sil, %sil
	je	.LBB0_1
# %bb.3:
	incq	%rdi
	movabsq	$1099511628211, %rax            # imm = 0x100000001B3
	movabsq	$25214903917, %rcx              # imm = 0x5DEECE66D
	.p2align	4
.LBB0_4:                                # =>This Inner Loop Header: Depth=1
	movzbl	%sil, %edx
	xorq	%r8, %rdx
	imulq	%rax, %rdx
	rolq	$7, %rdx
	xorq	%rcx, %rdx
	movzbl	(%rdi), %esi
	incq	%rdi
	movq	%rdx, %r8
	testb	%sil, %sil
	jne	.LBB0_4
	jmp	.LBB0_2
.LBB0_1:
	movq	%r8, %rdx
.LBB0_2:
	movq	%rdx, %rax
	shrq	$33, %rax
	xorq	%rdx, %rax
	movabsq	$-49064778989728563, %rcx       # imm = 0xFF51AFD7ED558CCD
	imulq	%rax, %rcx
	movq	%rcx, %rax
	shrq	$29, %rax
	xorq	%rcx, %rax
	movabsq	$-4265267296055464877, %rcx     # imm = 0xC4CEB9FE1A85EC53
	imulq	%rax, %rcx
	movq	%rcx, %rax
	shrq	$33, %rax
	xorq	%rcx, %rax
	retq
.Lfunc_end0:
	.size	key_from_name, .Lfunc_end0-key_from_name
                                        # -- End function
	.globl	validate                        # -- Begin function validate
	.p2align	4
	.type	validate,@function
validate:                               # @validate
# %bb.0:
	pushq	%rbp
	pushq	%r15
	pushq	%r14
	pushq	%r13
	pushq	%r12
	pushq	%rbx
	movabsq	$-3750763034362895579, %r9      # imm = 0xCBF29CE484222325
	movzbl	(%rdi), %r8d
	testb	%r8b, %r8b
	je	.LBB1_1
# %bb.2:
	incq	%rdi
	movabsq	$1099511628211, %rax            # imm = 0x100000001B3
	movabsq	$25214903917, %rcx              # imm = 0x5DEECE66D
	.p2align	4
.LBB1_3:                                # =>This Inner Loop Header: Depth=1
	movzbl	%r8b, %edx
	xorq	%r9, %rdx
	imulq	%rax, %rdx
	rolq	$7, %rdx
	xorq	%rcx, %rdx
	movzbl	(%rdi), %r8d
	incq	%rdi
	movq	%rdx, %r9
	testb	%r8b, %r8b
	jne	.LBB1_3
	jmp	.LBB1_4
.LBB1_1:
	movq	%r9, %rdx
.LBB1_4:
	movq	%rdx, %rax
	shrq	$33, %rax
	xorq	%rdx, %rax
	movabsq	$-49064778989728563, %rcx       # imm = 0xFF51AFD7ED558CCD
	imulq	%rax, %rcx
	movq	%rcx, %rax
	shrq	$29, %rax
	xorq	%rcx, %rax
	movabsq	$-4265267296055464877, %rcx     # imm = 0xC4CEB9FE1A85EC53
	imulq	%rax, %rcx
	movq	%rcx, %rax
	shrq	$33, %rax
	xorq	%rcx, %rax
	movq	%rcx, %rdx
	shrq	$48, %rdx
	movq	%rcx, %rdi
	shrq	$60, %rdi
	leaq	format_serial.HEX(%rip), %r14
	movzbl	(%rdi,%r14), %edi
	movb	%dil, -1(%rsp)                  # 1-byte Spill
	movq	%rcx, %rdi
	shrq	$56, %rdi
	andl	$15, %edi
	movzbl	(%rdi,%r14), %edi
	movb	%dil, -2(%rsp)                  # 1-byte Spill
	movq	%rcx, %rdi
	shrq	$52, %rdi
	andl	$15, %edi
	movzbl	(%rdi,%r14), %edi
	movb	%dil, -3(%rsp)                  # 1-byte Spill
	andl	$15, %edx
	movzbl	(%rdx,%r14), %edx
	movb	%dl, -4(%rsp)                   # 1-byte Spill
	movq	%rcx, %rdx
	shrq	$32, %rdx
	movq	%rcx, %rdi
	shrq	$44, %rdi
	andl	$15, %edi
	movzbl	(%rdi,%r14), %r9d
	movq	%rcx, %rdi
	shrq	$40, %rdi
	andl	$15, %edi
	movzbl	(%rdi,%r14), %r10d
	shrq	$36, %rcx
	andl	$15, %ecx
	movzbl	(%rcx,%r14), %r11d
	andl	$15, %edx
	movzbl	(%rdx,%r14), %r12d
	movl	%eax, %edi
	shrl	$16, %edi
	movl	%eax, %ecx
	shrl	$28, %ecx
	movzbl	(%rcx,%r14), %r13d
	movl	%eax, %ecx
	shrl	$24, %ecx
	andl	$15, %ecx
	movzbl	(%rcx,%r14), %ecx
	movl	%eax, %edx
	shrl	$20, %edx
	andl	$15, %edx
	movzbl	(%rdx,%r14), %edx
	andl	$15, %edi
	movzbl	(%rdi,%r14), %edi
	movl	%eax, %r8d
	shrl	$12, %r8d
	andl	$15, %r8d
	movzbl	(%r8,%r14), %r8d
	movl	%eax, %ebx
	shrl	$8, %ebx
	andl	$15, %ebx
	movzbl	(%rbx,%r14), %r15d
	movl	%eax, %ebx
	shrl	$4, %ebx
	andl	$15, %ebx
	movzbl	(%rbx,%r14), %ebx
	andl	$15, %eax
	movzbl	(%rax,%r14), %ebp
	movq	$-1, %r14
	.p2align	4
.LBB1_5:                                # =>This Inner Loop Header: Depth=1
	cmpb	$0, 1(%rsi,%r14)
	leaq	1(%r14), %r14
	jne	.LBB1_5
# %bb.6:
	xorl	%eax, %eax
	cmpl	$19, %r14d
	jne	.LBB1_8
# %bb.7:
	movdqu	(%rsi), %xmm0
	movl	$45, %eax
	movd	%eax, %xmm1
	movzbl	%r8b, %eax
	movd	%eax, %xmm2
	movdqa	%xmm1, %xmm3
	punpcklbw	%xmm2, %xmm3            # xmm3 = xmm3[0],xmm2[0],xmm3[1],xmm2[1],xmm3[2],xmm2[2],xmm3[3],xmm2[3],xmm3[4],xmm2[4],xmm3[5],xmm2[5],xmm3[6],xmm2[6],xmm3[7],xmm2[7]
	movzbl	%dil, %eax
	movd	%eax, %xmm2
	movzbl	%dl, %eax
	movd	%eax, %xmm4
	punpcklbw	%xmm2, %xmm4            # xmm4 = xmm4[0],xmm2[0],xmm4[1],xmm2[1],xmm4[2],xmm2[2],xmm4[3],xmm2[3],xmm4[4],xmm2[4],xmm4[5],xmm2[5],xmm4[6],xmm2[6],xmm4[7],xmm2[7]
	punpcklwd	%xmm3, %xmm4            # xmm4 = xmm4[0],xmm3[0],xmm4[1],xmm3[1],xmm4[2],xmm3[2],xmm4[3],xmm3[3]
	movzbl	%cl, %eax
	movd	%eax, %xmm2
	movzbl	%r13b, %eax
	movd	%eax, %xmm3
	punpcklbw	%xmm2, %xmm3            # xmm3 = xmm3[0],xmm2[0],xmm3[1],xmm2[1],xmm3[2],xmm2[2],xmm3[3],xmm2[3],xmm3[4],xmm2[4],xmm3[5],xmm2[5],xmm3[6],xmm2[6],xmm3[7],xmm2[7]
	movzbl	%r12b, %eax
	movd	%eax, %xmm2
	punpcklbw	%xmm1, %xmm2            # xmm2 = xmm2[0],xmm1[0],xmm2[1],xmm1[1],xmm2[2],xmm1[2],xmm2[3],xmm1[3],xmm2[4],xmm1[4],xmm2[5],xmm1[5],xmm2[6],xmm1[6],xmm2[7],xmm1[7]
	punpcklwd	%xmm3, %xmm2            # xmm2 = xmm2[0],xmm3[0],xmm2[1],xmm3[1],xmm2[2],xmm3[2],xmm2[3],xmm3[3]
	punpckldq	%xmm4, %xmm2            # xmm2 = xmm2[0],xmm4[0],xmm2[1],xmm4[1]
	movzbl	%r11b, %eax
	movd	%eax, %xmm3
	movzbl	%r10b, %eax
	movd	%eax, %xmm4
	punpcklbw	%xmm3, %xmm4            # xmm4 = xmm4[0],xmm3[0],xmm4[1],xmm3[1],xmm4[2],xmm3[2],xmm4[3],xmm3[3],xmm4[4],xmm3[4],xmm4[5],xmm3[5],xmm4[6],xmm3[6],xmm4[7],xmm3[7]
	movzbl	%r9b, %eax
	movd	%eax, %xmm3
	punpcklbw	%xmm3, %xmm1            # xmm1 = xmm1[0],xmm3[0],xmm1[1],xmm3[1],xmm1[2],xmm3[2],xmm1[3],xmm3[3],xmm1[4],xmm3[4],xmm1[5],xmm3[5],xmm1[6],xmm3[6],xmm1[7],xmm3[7]
	punpcklwd	%xmm4, %xmm1            # xmm1 = xmm1[0],xmm4[0],xmm1[1],xmm4[1],xmm1[2],xmm4[2],xmm1[3],xmm4[3]
	movzbl	-4(%rsp), %eax                  # 1-byte Folded Reload
	movd	%eax, %xmm3
	movzbl	-3(%rsp), %eax                  # 1-byte Folded Reload
	movd	%eax, %xmm4
	punpcklbw	%xmm3, %xmm4            # xmm4 = xmm4[0],xmm3[0],xmm4[1],xmm3[1],xmm4[2],xmm3[2],xmm4[3],xmm3[3],xmm4[4],xmm3[4],xmm4[5],xmm3[5],xmm4[6],xmm3[6],xmm4[7],xmm3[7]
	movzbl	-2(%rsp), %eax                  # 1-byte Folded Reload
	movd	%eax, %xmm3
	movzbl	-1(%rsp), %eax                  # 1-byte Folded Reload
	movd	%eax, %xmm5
	punpcklbw	%xmm3, %xmm5            # xmm5 = xmm5[0],xmm3[0],xmm5[1],xmm3[1],xmm5[2],xmm3[2],xmm5[3],xmm3[3],xmm5[4],xmm3[4],xmm5[5],xmm3[5],xmm5[6],xmm3[6],xmm5[7],xmm3[7]
	punpcklwd	%xmm4, %xmm5            # xmm5 = xmm5[0],xmm4[0],xmm5[1],xmm4[1],xmm5[2],xmm4[2],xmm5[3],xmm4[3]
	punpckldq	%xmm1, %xmm5            # xmm5 = xmm5[0],xmm1[0],xmm5[1],xmm1[1]
	punpcklqdq	%xmm2, %xmm5            # xmm5 = xmm5[0],xmm2[0]
	pcmpeqb	%xmm0, %xmm5
	cmpb	16(%rsi), %r15b
	sete	%al
	xorb	17(%rsi), %bl
	xorb	18(%rsi), %bpl
	pmovmskb	%xmm5, %ecx
	xorl	$65535, %ecx                    # imm = 0xFFFF
	sete	%cl
	orb	%bl, %bpl
	sete	%dl
	andb	%al, %dl
	andb	%cl, %dl
	movzbl	%dl, %eax
.LBB1_8:
	popq	%rbx
	popq	%r12
	popq	%r13
	popq	%r14
	popq	%r15
	popq	%rbp
	retq
.Lfunc_end1:
	.size	validate, .Lfunc_end1-validate
                                        # -- End function
	.type	format_serial.HEX,@object       # @format_serial.HEX
	.section	.rodata.str1.16,"aMS",@progbits,1
	.p2align	4, 0x0
format_serial.HEX:
	.asciz	"0123456789ABCDEF"
	.size	format_serial.HEX, 17

	.ident	"clang version 20.1.8"
	.section	".note.GNU-stack","",@progbits
	.addrsig
