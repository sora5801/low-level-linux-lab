	.file	"demo.c"
	.text
	.globl	bpf_load_u32                    # -- Begin function bpf_load_u32
	.p2align	4
	.type	bpf_load_u32,@function
bpf_load_u32:                           # @bpf_load_u32
# %bb.0:
	pushq	%rbp
	movq	%rsp, %rbp
                                        # kill: def $esi killed $esi def $rsi
	movl	%esi, %eax
	movzbl	(%rdi,%rax), %eax
	leal	1(%rsi), %ecx
	movzbl	(%rdi,%rcx), %ecx
	shll	$8, %ecx
	orl	%eax, %ecx
	leal	2(%rsi), %eax
	movzbl	(%rdi,%rax), %edx
	shll	$16, %edx
	orl	%ecx, %edx
	addl	$3, %esi
	movzbl	(%rdi,%rsi), %eax
	shll	$24, %eax
	orl	%edx, %eax
	popq	%rbp
	retq
.Lfunc_end0:
	.size	bpf_load_u32, .Lfunc_end0-bpf_load_u32
                                        # -- End function
	.globl	build_allowlist                 # -- Begin function build_allowlist
	.p2align	4
	.type	build_allowlist,@function
build_allowlist:                        # @build_allowlist
# %bb.0:
	movl	$-1, %eax
	testl	%esi, %esi
	jle	.LBB1_15
# %bb.1:
	movabsq	$17179869216, %r8               # imm = 0x400000020
	movq	%r8, (%rdi)
	cmpl	$1, %esi
	je	.LBB1_15
# %bb.2:
	movabsq	$-4611685752139349995, %r8      # imm = 0xC000003E00010015
	movq	%r8, 8(%rdi)
	cmpl	$3, %esi
	jl	.LBB1_15
# %bb.3:
	movabsq	$-9223372036854775802, %r8      # imm = 0x8000000000000006
	movq	%r8, 16(%rdi)
	je	.LBB1_15
# %bb.4:
	movq	$32, 24(%rdi)
	cmpl	$5, %esi
	jl	.LBB1_15
# %bb.5:
	movabsq	$4611686018444165173, %r9       # imm = 0x4000000001000035
	movq	%r9, 32(%rdi)
	je	.LBB1_15
# %bb.6:
	pushq	%rbp
	movq	%rsp, %rbp
	pushq	%r15
	pushq	%r14
	pushq	%r12
	pushq	%rbx
	movq	%r8, 40(%rdi)
	testl	%ecx, %ecx
	setg	%r10b
	setle	%r11b
	cmpl	$7, %esi
	setl	%bl
	movl	$6, %r9d
	orb	%r11b, %bl
	jne	.LBB1_13
# %bb.7:
	movl	$16777237, 48(%rdi)             # imm = 0x1000015
	movl	(%rdx), %r9d
	movl	%r9d, 52(%rdi)
	movl	$7, %r9d
	cmpl	$8, %esi
	jb	.LBB1_13
# %bb.8:
	movl	%ecx, %ecx
	movl	%esi, %ebx
	movl	$7, %r11d
	movl	$8, %r9d
	movl	$1, %r14d
	movabsq	$9223090561878065158, %r15      # imm = 0x7FFF000000000006
.LBB1_10:                               # =>This Inner Loop Header: Depth=1
	movq	%r15, (%rdi,%r11,8)
	cmpq	%rcx, %r14
	setb	%r10b
	jae	.LBB1_13
# %bb.11:                               #   in Loop: Header=BB1_10 Depth=1
	cmpl	%r9d, %esi
	jle	.LBB1_13
# %bb.9:                                #   in Loop: Header=BB1_10 Depth=1
	movl	$16777237, 8(%rdi,%r11,8)       # imm = 0x1000015
	movl	-10(%rdx,%r11,2), %r12d
	movl	%r12d, 12(%rdi,%r11,8)
	addq	$2, %r11
	addl	$2, %r9d
	incq	%r14
	cmpq	%rbx, %r11
	jb	.LBB1_10
# %bb.12:
	movl	%r11d, %r9d
.LBB1_13:
	cmpl	%esi, %r9d
	setge	%cl
	orb	%r10b, %cl
	popq	%rbx
	popq	%r12
	popq	%r14
	popq	%r15
	popq	%rbp
	jne	.LBB1_15
# %bb.14:
	movslq	%r9d, %rax
	movq	%r8, (%rdi,%rax,8)
	incl	%r9d
	movl	%r9d, %eax
.LBB1_15:
	retq
.Lfunc_end1:
	.size	build_allowlist, .Lfunc_end1-build_allowlist
                                        # -- End function
	.globl	seccomp_run                     # -- Begin function seccomp_run
	.p2align	4
	.type	seccomp_run,@function
seccomp_run:                            # @seccomp_run
# %bb.0:
	testl	%esi, %esi
	jle	.LBB2_19
# %bb.1:
	pushq	%rbp
	movq	%rsp, %rbp
	pushq	%rbx
	xorl	%r8d, %r8d
                                        # implicit-def: $eax
	xorl	%r9d, %r9d
	.p2align	4
.LBB2_2:                                # =>This Inner Loop Header: Depth=1
	movslq	%r8d, %r10
	leaq	(%rdi,%r10,8), %r11
	movzwl	(%rdi,%r10,8), %ebx
	xorl	%r10d, %r10d
	cmpl	$31, %ebx
	jg	.LBB2_6
# %bb.3:                                #   in Loop: Header=BB2_2 Depth=1
	cmpl	$6, %ebx
	je	.LBB2_10
# %bb.4:                                #   in Loop: Header=BB2_2 Depth=1
	cmpl	$21, %ebx
	jne	.LBB2_17
# %bb.5:                                #   in Loop: Header=BB2_2 Depth=1
	xorl	%r10d, %r10d
	cmpl	4(%r11), %r9d
	sete	%r10b
	xorq	$3, %r10
	movzbl	(%r11,%r10), %r10d
	jmp	.LBB2_12
	.p2align	4
.LBB2_6:                                #   in Loop: Header=BB2_2 Depth=1
	cmpl	$53, %ebx
	je	.LBB2_11
# %bb.7:                                #   in Loop: Header=BB2_2 Depth=1
	cmpl	$32, %ebx
	jne	.LBB2_17
# %bb.8:                                #   in Loop: Header=BB2_2 Depth=1
	movl	4(%r11), %r10d
	leal	4(%r10), %r11d
	cmpl	%ecx, %r11d
	jbe	.LBB2_18
# %bb.9:                                #   in Loop: Header=BB2_2 Depth=1
	xorl	%r10d, %r10d
	movl	$-2147483648, %eax              # imm = 0x80000000
	jmp	.LBB2_14
.LBB2_17:                               #   in Loop: Header=BB2_2 Depth=1
	movl	$-2147483648, %eax              # imm = 0x80000000
	jmp	.LBB2_14
.LBB2_10:                               #   in Loop: Header=BB2_2 Depth=1
	movl	4(%r11), %eax
	xorl	%r10d, %r10d
	jmp	.LBB2_14
.LBB2_11:                               #   in Loop: Header=BB2_2 Depth=1
	cmpl	4(%r11), %r9d
	adcq	$2, %r11
	movzbl	(%r11), %r10d
.LBB2_12:                               #   in Loop: Header=BB2_2 Depth=1
	addl	%r10d, %r8d
.LBB2_13:                               #   in Loop: Header=BB2_2 Depth=1
	movb	$1, %r10b
.LBB2_14:                               #   in Loop: Header=BB2_2 Depth=1
	testb	%r10b, %r10b
	je	.LBB2_21
# %bb.15:                               #   in Loop: Header=BB2_2 Depth=1
	incl	%r8d
	cmpl	%esi, %r8d
	jl	.LBB2_2
	jmp	.LBB2_20
.LBB2_18:                               #   in Loop: Header=BB2_2 Depth=1
	movzbl	(%rdx,%r10), %r9d
	leal	1(%r10), %r11d
	movzbl	(%rdx,%r11), %r11d
	shll	$8, %r11d
	orl	%r9d, %r11d
	leal	2(%r10), %r9d
	movzbl	(%rdx,%r9), %ebx
	shll	$16, %ebx
	orl	%r11d, %ebx
	leal	3(%r10), %r9d
	movzbl	(%rdx,%r9), %r9d
	shll	$24, %r9d
	orl	%ebx, %r9d
	jmp	.LBB2_13
.LBB2_19:
	movl	$-2147483648, %eax              # imm = 0x80000000
	retq
.LBB2_20:
	movl	$-2147483648, %eax              # imm = 0x80000000
.LBB2_21:
	popq	%rbx
	popq	%rbp
	retq
.Lfunc_end2:
	.size	seccomp_run, .Lfunc_end2-seccomp_run
                                        # -- End function
	.section	.rodata.cst16,"aM",@progbits,16
	.p2align	4, 0x0                          # -- Begin function demo_selftest
.LCPI3_0:
	.long	32                              # 0x20
	.long	4                               # 0x4
	.long	65557                           # 0x10015
	.long	3221225534                      # 0xc000003e
.LCPI3_1:
	.long	6                               # 0x6
	.long	2147483648                      # 0x80000000
	.long	32                              # 0x20
	.long	0                               # 0x0
.LCPI3_2:
	.long	16777269                        # 0x1000035
	.long	1073741824                      # 0x40000000
	.long	6                               # 0x6
	.long	2147483648                      # 0x80000000
.LCPI3_3:
	.long	16777237                        # 0x1000015
	.long	0                               # 0x0
	.long	6                               # 0x6
	.long	2147418112                      # 0x7fff0000
	.text
	.globl	demo_selftest
	.p2align	4
	.type	demo_selftest,@function
demo_selftest:                          # @demo_selftest
# %bb.0:
	pushq	%rbp
	movq	%rsp, %rbp
	subq	$192, %rsp
	movaps	.LCPI3_0(%rip), %xmm0           # xmm0 = [32,4,65557,3221225534]
	movaps	%xmm0, -320(%rbp)
	movaps	.LCPI3_1(%rip), %xmm0           # xmm0 = [6,2147483648,32,0]
	movaps	%xmm0, -304(%rbp)
	movaps	.LCPI3_2(%rip), %xmm0           # xmm0 = [16777269,1073741824,6,2147483648]
	movaps	%xmm0, -288(%rbp)
	movaps	.LCPI3_3(%rip), %xmm0           # xmm0 = [16777237,0,6,2147418112]
	movaps	%xmm0, -272(%rbp)
	movl	$19, %eax
	leaq	.L__const.demo_selftest.allow(%rip), %rcx
	movabsq	$9223090561878065158, %rdx      # imm = 0x7FFF000000000006
	.p2align	4
.LBB3_1:                                # =>This Inner Loop Header: Depth=1
	movl	$16777237, -332(%rbp,%rax,4)    # imm = 0x1000015
	movl	-15(%rax,%rcx), %esi
	movl	%esi, -328(%rbp,%rax,4)
	movq	%rdx, -324(%rbp,%rax,4)
	addq	$4, %rax
	cmpq	$27, %rax
	jne	.LBB3_1
# %bb.2:
	movabsq	$-9223372036854775802, %rax     # imm = 0x8000000000000006
	movq	%rax, -224(%rbp)
	xorps	%xmm0, %xmm0
	movaps	%xmm0, -64(%rbp)
	movaps	%xmm0, -16(%rbp)
	movaps	%xmm0, -32(%rbp)
	movaps	%xmm0, -48(%rbp)
	movabsq	$-4611685752139415551, %rax     # imm = 0xC000003E00000001
	movq	%rax, -64(%rbp)
	xorl	%eax, %eax
                                        # implicit-def: $ecx
	xorl	%edx, %edx
	.p2align	4
.LBB3_3:                                # =>This Inner Loop Header: Depth=1
	movslq	%eax, %rsi
	leaq	-320(,%rsi,8), %rdi
	addq	%rbp, %rdi
	movzwl	-320(%rbp,%rsi,8), %r8d
	xorl	%esi, %esi
	cmpl	$31, %r8d
	jg	.LBB3_7
# %bb.4:                                #   in Loop: Header=BB3_3 Depth=1
	cmpl	$6, %r8d
	je	.LBB3_11
# %bb.5:                                #   in Loop: Header=BB3_3 Depth=1
	cmpl	$21, %r8d
	jne	.LBB3_18
# %bb.6:                                #   in Loop: Header=BB3_3 Depth=1
	xorl	%esi, %esi
	cmpl	4(%rdi), %edx
	sete	%sil
	xorq	$3, %rsi
	movzbl	(%rdi,%rsi), %esi
	jmp	.LBB3_13
	.p2align	4
.LBB3_7:                                #   in Loop: Header=BB3_3 Depth=1
	cmpl	$53, %r8d
	je	.LBB3_12
# %bb.8:                                #   in Loop: Header=BB3_3 Depth=1
	cmpl	$32, %r8d
	jne	.LBB3_18
# %bb.9:                                #   in Loop: Header=BB3_3 Depth=1
	movl	4(%rdi), %esi
	leal	-61(%rsi), %edi
	cmpl	$-65, %edi
	jae	.LBB3_19
# %bb.10:                               #   in Loop: Header=BB3_3 Depth=1
	xorl	%esi, %esi
	movl	$-2147483648, %ecx              # imm = 0x80000000
	jmp	.LBB3_15
.LBB3_18:                               #   in Loop: Header=BB3_3 Depth=1
	movl	$-2147483648, %ecx              # imm = 0x80000000
	jmp	.LBB3_15
.LBB3_11:                               #   in Loop: Header=BB3_3 Depth=1
	movl	4(%rdi), %ecx
	xorl	%esi, %esi
	jmp	.LBB3_15
.LBB3_12:                               #   in Loop: Header=BB3_3 Depth=1
	cmpl	4(%rdi), %edx
	adcq	$2, %rdi
	movzbl	(%rdi), %esi
.LBB3_13:                               #   in Loop: Header=BB3_3 Depth=1
	addl	%esi, %eax
.LBB3_14:                               #   in Loop: Header=BB3_3 Depth=1
	movb	$1, %sil
.LBB3_15:                               #   in Loop: Header=BB3_3 Depth=1
	testb	%sil, %sil
	je	.LBB3_21
# %bb.16:                               #   in Loop: Header=BB3_3 Depth=1
	leal	1(%rax), %esi
	cmpl	$11, %eax
	movl	%esi, %eax
	jle	.LBB3_3
	jmp	.LBB3_20
.LBB3_19:                               #   in Loop: Header=BB3_3 Depth=1
	movzbl	-64(%rbp,%rsi), %edx
	leal	1(%rsi), %edi
	movzbl	-64(%rbp,%rdi), %edi
	shll	$8, %edi
	orl	%edx, %edi
	leal	2(%rsi), %edx
	movzbl	-64(%rbp,%rdx), %r8d
	shll	$16, %r8d
	orl	%edi, %r8d
	leal	3(%rsi), %edx
	movzbl	-64(%rbp,%rdx), %edx
	shll	$24, %edx
	orl	%r8d, %edx
	jmp	.LBB3_14
.LBB3_20:
	movl	$-2147483648, %ecx              # imm = 0x80000000
.LBB3_21:
	movl	$2, %eax
	cmpl	$2147418112, %ecx               # imm = 0x7FFF0000
	jne	.LBB3_82
# %bb.22:
	xorps	%xmm0, %xmm0
	movaps	%xmm0, -64(%rbp)
	movaps	%xmm0, -16(%rbp)
	movaps	%xmm0, -32(%rbp)
	movaps	%xmm0, -48(%rbp)
	movabsq	$-4611685752139415511, %rax     # imm = 0xC000003E00000029
	movq	%rax, -64(%rbp)
	xorl	%eax, %eax
                                        # implicit-def: $ecx
	xorl	%edx, %edx
	.p2align	4
.LBB3_23:                               # =>This Inner Loop Header: Depth=1
	movslq	%eax, %rsi
	leaq	-320(,%rsi,8), %rdi
	addq	%rbp, %rdi
	movzwl	-320(%rbp,%rsi,8), %r8d
	xorl	%esi, %esi
	cmpl	$31, %r8d
	jg	.LBB3_27
# %bb.24:                               #   in Loop: Header=BB3_23 Depth=1
	cmpl	$6, %r8d
	je	.LBB3_31
# %bb.25:                               #   in Loop: Header=BB3_23 Depth=1
	cmpl	$21, %r8d
	jne	.LBB3_38
# %bb.26:                               #   in Loop: Header=BB3_23 Depth=1
	xorl	%esi, %esi
	cmpl	4(%rdi), %edx
	sete	%sil
	xorq	$3, %rsi
	movzbl	(%rdi,%rsi), %esi
	jmp	.LBB3_33
	.p2align	4
.LBB3_27:                               #   in Loop: Header=BB3_23 Depth=1
	cmpl	$53, %r8d
	je	.LBB3_32
# %bb.28:                               #   in Loop: Header=BB3_23 Depth=1
	cmpl	$32, %r8d
	jne	.LBB3_38
# %bb.29:                               #   in Loop: Header=BB3_23 Depth=1
	movl	4(%rdi), %esi
	leal	-61(%rsi), %edi
	cmpl	$-65, %edi
	jae	.LBB3_39
# %bb.30:                               #   in Loop: Header=BB3_23 Depth=1
	xorl	%esi, %esi
	movl	$-2147483648, %ecx              # imm = 0x80000000
	jmp	.LBB3_35
.LBB3_38:                               #   in Loop: Header=BB3_23 Depth=1
	movl	$-2147483648, %ecx              # imm = 0x80000000
	jmp	.LBB3_35
.LBB3_31:                               #   in Loop: Header=BB3_23 Depth=1
	movl	4(%rdi), %ecx
	xorl	%esi, %esi
	jmp	.LBB3_35
.LBB3_32:                               #   in Loop: Header=BB3_23 Depth=1
	cmpl	4(%rdi), %edx
	adcq	$2, %rdi
	movzbl	(%rdi), %esi
.LBB3_33:                               #   in Loop: Header=BB3_23 Depth=1
	addl	%esi, %eax
.LBB3_34:                               #   in Loop: Header=BB3_23 Depth=1
	movb	$1, %sil
.LBB3_35:                               #   in Loop: Header=BB3_23 Depth=1
	testb	%sil, %sil
	je	.LBB3_41
# %bb.36:                               #   in Loop: Header=BB3_23 Depth=1
	leal	1(%rax), %esi
	cmpl	$11, %eax
	movl	%esi, %eax
	jle	.LBB3_23
	jmp	.LBB3_40
.LBB3_39:                               #   in Loop: Header=BB3_23 Depth=1
	movzbl	-64(%rbp,%rsi), %edx
	leal	1(%rsi), %edi
	movzbl	-64(%rbp,%rdi), %edi
	shll	$8, %edi
	orl	%edx, %edi
	leal	2(%rsi), %edx
	movzbl	-64(%rbp,%rdx), %r8d
	shll	$16, %r8d
	orl	%edi, %r8d
	leal	3(%rsi), %edx
	movzbl	-64(%rbp,%rdx), %edx
	shll	$24, %edx
	orl	%r8d, %edx
	jmp	.LBB3_34
.LBB3_40:
	movl	$-2147483648, %ecx              # imm = 0x80000000
.LBB3_41:
	movl	$3, %eax
	negl	%ecx
	jno	.LBB3_82
# %bb.42:
	xorps	%xmm0, %xmm0
	movaps	%xmm0, -64(%rbp)
	movaps	%xmm0, -16(%rbp)
	movaps	%xmm0, -32(%rbp)
	movaps	%xmm0, -48(%rbp)
	movabsq	$-4611685747844448255, %rax     # imm = 0xC000003F00000001
	movq	%rax, -64(%rbp)
	xorl	%eax, %eax
                                        # implicit-def: $ecx
	xorl	%edx, %edx
	.p2align	4
.LBB3_43:                               # =>This Inner Loop Header: Depth=1
	movslq	%eax, %rsi
	leaq	-320(,%rsi,8), %rdi
	addq	%rbp, %rdi
	movzwl	-320(%rbp,%rsi,8), %r8d
	xorl	%esi, %esi
	cmpl	$31, %r8d
	jg	.LBB3_47
# %bb.44:                               #   in Loop: Header=BB3_43 Depth=1
	cmpl	$6, %r8d
	je	.LBB3_51
# %bb.45:                               #   in Loop: Header=BB3_43 Depth=1
	cmpl	$21, %r8d
	jne	.LBB3_58
# %bb.46:                               #   in Loop: Header=BB3_43 Depth=1
	xorl	%esi, %esi
	cmpl	4(%rdi), %edx
	sete	%sil
	xorq	$3, %rsi
	movzbl	(%rdi,%rsi), %esi
	jmp	.LBB3_53
	.p2align	4
.LBB3_47:                               #   in Loop: Header=BB3_43 Depth=1
	cmpl	$53, %r8d
	je	.LBB3_52
# %bb.48:                               #   in Loop: Header=BB3_43 Depth=1
	cmpl	$32, %r8d
	jne	.LBB3_58
# %bb.49:                               #   in Loop: Header=BB3_43 Depth=1
	movl	4(%rdi), %esi
	leal	-61(%rsi), %edi
	cmpl	$-65, %edi
	jae	.LBB3_59
# %bb.50:                               #   in Loop: Header=BB3_43 Depth=1
	xorl	%esi, %esi
	movl	$-2147483648, %ecx              # imm = 0x80000000
	jmp	.LBB3_55
.LBB3_58:                               #   in Loop: Header=BB3_43 Depth=1
	movl	$-2147483648, %ecx              # imm = 0x80000000
	jmp	.LBB3_55
.LBB3_51:                               #   in Loop: Header=BB3_43 Depth=1
	movl	4(%rdi), %ecx
	xorl	%esi, %esi
	jmp	.LBB3_55
.LBB3_52:                               #   in Loop: Header=BB3_43 Depth=1
	cmpl	4(%rdi), %edx
	adcq	$2, %rdi
	movzbl	(%rdi), %esi
.LBB3_53:                               #   in Loop: Header=BB3_43 Depth=1
	addl	%esi, %eax
.LBB3_54:                               #   in Loop: Header=BB3_43 Depth=1
	movb	$1, %sil
.LBB3_55:                               #   in Loop: Header=BB3_43 Depth=1
	testb	%sil, %sil
	je	.LBB3_61
# %bb.56:                               #   in Loop: Header=BB3_43 Depth=1
	leal	1(%rax), %esi
	cmpl	$11, %eax
	movl	%esi, %eax
	jle	.LBB3_43
	jmp	.LBB3_60
.LBB3_59:                               #   in Loop: Header=BB3_43 Depth=1
	movzbl	-64(%rbp,%rsi), %edx
	leal	1(%rsi), %edi
	movzbl	-64(%rbp,%rdi), %edi
	shll	$8, %edi
	orl	%edx, %edi
	leal	2(%rsi), %edx
	movzbl	-64(%rbp,%rdx), %r8d
	shll	$16, %r8d
	orl	%edi, %r8d
	leal	3(%rsi), %edx
	movzbl	-64(%rbp,%rdx), %edx
	shll	$24, %edx
	orl	%r8d, %edx
	jmp	.LBB3_54
.LBB3_60:
	movl	$-2147483648, %ecx              # imm = 0x80000000
.LBB3_61:
	movl	$4, %eax
	negl	%ecx
	jno	.LBB3_82
# %bb.62:
	xorps	%xmm0, %xmm0
	movaps	%xmm0, -64(%rbp)
	movaps	%xmm0, -16(%rbp)
	movaps	%xmm0, -32(%rbp)
	movaps	%xmm0, -48(%rbp)
	movabsq	$-4611685751065673727, %rax     # imm = 0xC000003E40000001
	movq	%rax, -64(%rbp)
	xorl	%eax, %eax
                                        # implicit-def: $ecx
	xorl	%edx, %edx
	.p2align	4
.LBB3_63:                               # =>This Inner Loop Header: Depth=1
	movslq	%eax, %rsi
	leaq	-320(,%rsi,8), %rdi
	addq	%rbp, %rdi
	movzwl	-320(%rbp,%rsi,8), %r8d
	xorl	%esi, %esi
	cmpl	$31, %r8d
	jg	.LBB3_67
# %bb.64:                               #   in Loop: Header=BB3_63 Depth=1
	cmpl	$6, %r8d
	je	.LBB3_71
# %bb.65:                               #   in Loop: Header=BB3_63 Depth=1
	cmpl	$21, %r8d
	jne	.LBB3_78
# %bb.66:                               #   in Loop: Header=BB3_63 Depth=1
	xorl	%esi, %esi
	cmpl	4(%rdi), %edx
	sete	%sil
	xorq	$3, %rsi
	movzbl	(%rdi,%rsi), %esi
	jmp	.LBB3_73
	.p2align	4
.LBB3_67:                               #   in Loop: Header=BB3_63 Depth=1
	cmpl	$53, %r8d
	je	.LBB3_72
# %bb.68:                               #   in Loop: Header=BB3_63 Depth=1
	cmpl	$32, %r8d
	jne	.LBB3_78
# %bb.69:                               #   in Loop: Header=BB3_63 Depth=1
	movl	4(%rdi), %esi
	leal	-61(%rsi), %edi
	cmpl	$-65, %edi
	jae	.LBB3_79
# %bb.70:                               #   in Loop: Header=BB3_63 Depth=1
	xorl	%esi, %esi
	movl	$-2147483648, %ecx              # imm = 0x80000000
	jmp	.LBB3_75
.LBB3_78:                               #   in Loop: Header=BB3_63 Depth=1
	movl	$-2147483648, %ecx              # imm = 0x80000000
	jmp	.LBB3_75
.LBB3_71:                               #   in Loop: Header=BB3_63 Depth=1
	movl	4(%rdi), %ecx
	xorl	%esi, %esi
	jmp	.LBB3_75
.LBB3_72:                               #   in Loop: Header=BB3_63 Depth=1
	cmpl	4(%rdi), %edx
	adcq	$2, %rdi
	movzbl	(%rdi), %esi
.LBB3_73:                               #   in Loop: Header=BB3_63 Depth=1
	addl	%esi, %eax
.LBB3_74:                               #   in Loop: Header=BB3_63 Depth=1
	movb	$1, %sil
.LBB3_75:                               #   in Loop: Header=BB3_63 Depth=1
	testb	%sil, %sil
	je	.LBB3_81
# %bb.76:                               #   in Loop: Header=BB3_63 Depth=1
	leal	1(%rax), %esi
	cmpl	$11, %eax
	movl	%esi, %eax
	jle	.LBB3_63
	jmp	.LBB3_80
.LBB3_79:                               #   in Loop: Header=BB3_63 Depth=1
	movzbl	-64(%rbp,%rsi), %edx
	leal	1(%rsi), %edi
	movzbl	-64(%rbp,%rdi), %edi
	shll	$8, %edi
	orl	%edx, %edi
	leal	2(%rsi), %edx
	movzbl	-64(%rbp,%rdx), %r8d
	shll	$16, %r8d
	orl	%edi, %r8d
	leal	3(%rsi), %edx
	movzbl	-64(%rbp,%rdx), %edx
	shll	$24, %edx
	orl	%r8d, %edx
	jmp	.LBB3_74
.LBB3_80:
	movl	$-2147483648, %ecx              # imm = 0x80000000
.LBB3_81:
	xorl	%eax, %eax
	negl	%ecx
	setno	%al
	leal	(%rax,%rax,4), %eax
.LBB3_82:
	addq	$192, %rsp
	popq	%rbp
	retq
.Lfunc_end3:
	.size	demo_selftest, .Lfunc_end3-demo_selftest
                                        # -- End function
	.type	.L__const.demo_selftest.allow,@object # @__const.demo_selftest.allow
	.section	.rodata,"a",@progbits
	.p2align	2, 0x0
.L__const.demo_selftest.allow:
	.long	0                               # 0x0
	.long	1                               # 0x1
	.long	231                             # 0xe7
	.size	.L__const.demo_selftest.allow, 12

	.ident	"clang version 20.1.8"
	.section	".note.GNU-stack","",@progbits
	.addrsig
