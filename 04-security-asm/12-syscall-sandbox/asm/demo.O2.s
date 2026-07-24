	.file	"demo.c"
	.text
	.globl	bpf_load_u32                    # -- Begin function bpf_load_u32
	.p2align	4
	.type	bpf_load_u32,@function
bpf_load_u32:                           # @bpf_load_u32
# %bb.0:
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
	jle	.LBB1_16
# %bb.1:
	movabsq	$17179869216, %r8               # imm = 0x400000020
	movq	%r8, (%rdi)
	cmpl	$1, %esi
	je	.LBB1_16
# %bb.2:
	movabsq	$-4611685752139349995, %r8      # imm = 0xC000003E00010015
	movq	%r8, 8(%rdi)
	cmpl	$3, %esi
	jb	.LBB1_16
# %bb.3:
	movabsq	$-9223372036854775802, %r8      # imm = 0x8000000000000006
	movq	%r8, 16(%rdi)
	je	.LBB1_16
# %bb.4:
	movq	$32, 24(%rdi)
	cmpl	$5, %esi
	jb	.LBB1_16
# %bb.5:
	movabsq	$4611686018444165173, %r9       # imm = 0x4000000001000035
	movq	%r9, 32(%rdi)
	je	.LBB1_16
# %bb.6:
	pushq	%r15
	pushq	%r14
	pushq	%rbx
	movq	%r8, 40(%rdi)
	testl	%ecx, %ecx
	setg	%r10b
	setle	%r11b
	cmpl	$7, %esi
	setb	%bl
	movl	$6, %r9d
	orb	%r11b, %bl
	jne	.LBB1_12
# %bb.7:
	movl	%ecx, %ecx
	movl	%esi, %r11d
	movl	$7, %r15d
	movl	$1, %ebx
	movabsq	$9223090561878065158, %r14      # imm = 0x7FFF000000000006
.LBB1_8:                                # =>This Inner Loop Header: Depth=1
	movl	$16777237, -8(%rdi,%r15,8)      # imm = 0x1000015
	movl	-14(%rdx,%r15,2), %r9d
	movl	%r9d, -4(%rdi,%r15,8)
	cmpq	%r11, %r15
	jae	.LBB1_15
# %bb.9:                                #   in Loop: Header=BB1_8 Depth=1
	movq	%r14, (%rdi,%r15,8)
	leaq	2(%r15), %r9
	cmpq	%rcx, %rbx
	setb	%r10b
	jae	.LBB1_11
# %bb.10:                               #   in Loop: Header=BB1_8 Depth=1
	incl	%r15d
	incq	%rbx
	cmpl	%r15d, %esi
	movq	%r9, %r15
	jg	.LBB1_8
.LBB1_11:
	decl	%r9d
.LBB1_12:
	testb	%r10b, %r10b
	jne	.LBB1_15
# %bb.13:
	cmpl	%esi, %r9d
	jge	.LBB1_15
# %bb.14:
	movslq	%r9d, %rax
	movq	%r8, (%rdi,%rax,8)
	incl	%r9d
	movl	%r9d, %eax
.LBB1_15:
	popq	%rbx
	popq	%r14
	popq	%r15
.LBB1_16:
	retq
.Lfunc_end1:
	.size	build_allowlist, .Lfunc_end1-build_allowlist
                                        # -- End function
	.globl	seccomp_run                     # -- Begin function seccomp_run
	.p2align	4
	.type	seccomp_run,@function
seccomp_run:                            # @seccomp_run
# %bb.0:
	movl	$-2147483648, %eax              # imm = 0x80000000
	testl	%esi, %esi
	jle	.LBB2_14
# %bb.1:
	xorl	%r8d, %r8d
	xorl	%r9d, %r9d
	jmp	.LBB2_5
	.p2align	4
.LBB2_2:                                #   in Loop: Header=BB2_5 Depth=1
	cmpl	4(%r10), %r9d
	adcq	$2, %r10
	movzbl	(%r10), %r10d
.LBB2_3:                                #   in Loop: Header=BB2_5 Depth=1
	addl	%r10d, %r8d
.LBB2_4:                                #   in Loop: Header=BB2_5 Depth=1
	incl	%r8d
	cmpl	%esi, %r8d
	jge	.LBB2_14
.LBB2_5:                                # =>This Inner Loop Header: Depth=1
	movslq	%r8d, %r11
	leaq	(%rdi,%r11,8), %r10
	movzwl	(%rdi,%r11,8), %r11d
	cmpl	$31, %r11d
	jle	.LBB2_10
# %bb.6:                                #   in Loop: Header=BB2_5 Depth=1
	cmpl	$53, %r11d
	je	.LBB2_2
# %bb.7:                                #   in Loop: Header=BB2_5 Depth=1
	cmpl	$32, %r11d
	jne	.LBB2_14
# %bb.8:                                #   in Loop: Header=BB2_5 Depth=1
	movl	4(%r10), %r9d
	leal	4(%r9), %r10d
	cmpl	%ecx, %r10d
	ja	.LBB2_14
# %bb.9:                                #   in Loop: Header=BB2_5 Depth=1
	movzbl	(%rdx,%r9), %r10d
	leal	1(%r9), %r11d
	movzbl	(%rdx,%r11), %r11d
	shll	$8, %r11d
	orl	%r10d, %r11d
	leal	2(%r9), %r10d
	movzbl	(%rdx,%r10), %r10d
	shll	$16, %r10d
	orl	%r11d, %r10d
	addl	$3, %r9d
	movzbl	(%rdx,%r9), %r9d
	shll	$24, %r9d
	orl	%r10d, %r9d
	jmp	.LBB2_4
	.p2align	4
.LBB2_10:                               #   in Loop: Header=BB2_5 Depth=1
	cmpl	$21, %r11d
	jne	.LBB2_12
# %bb.11:                               #   in Loop: Header=BB2_5 Depth=1
	xorl	%r11d, %r11d
	cmpl	4(%r10), %r9d
	sete	%r11b
	xorq	$3, %r11
	movzbl	(%r10,%r11), %r10d
	jmp	.LBB2_3
.LBB2_12:
	cmpl	$6, %r11d
	jne	.LBB2_14
# %bb.13:
	movl	4(%r10), %eax
.LBB2_14:
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
.LCPI3_4:
	.long	16777237                        # 0x1000015
	.long	1                               # 0x1
	.long	6                               # 0x6
	.long	2147418112                      # 0x7fff0000
.LCPI3_5:
	.long	16777237                        # 0x1000015
	.long	231                             # 0xe7
	.long	6                               # 0x6
	.long	2147418112                      # 0x7fff0000
	.text
	.globl	demo_selftest
	.p2align	4
	.type	demo_selftest,@function
demo_selftest:                          # @demo_selftest
# %bb.0:
	subq	$200, %rsp
	movaps	.LCPI3_0(%rip), %xmm0           # xmm0 = [32,4,65557,3221225534]
	movaps	%xmm0, -64(%rsp)
	movaps	.LCPI3_1(%rip), %xmm0           # xmm0 = [6,2147483648,32,0]
	movaps	%xmm0, -48(%rsp)
	movaps	.LCPI3_2(%rip), %xmm0           # xmm0 = [16777269,1073741824,6,2147483648]
	movaps	%xmm0, -32(%rsp)
	movaps	.LCPI3_3(%rip), %xmm0           # xmm0 = [16777237,0,6,2147418112]
	movaps	%xmm0, -16(%rsp)
	movaps	.LCPI3_4(%rip), %xmm0           # xmm0 = [16777237,1,6,2147418112]
	movaps	%xmm0, (%rsp)
	movaps	.LCPI3_5(%rip), %xmm0           # xmm0 = [16777237,231,6,2147418112]
	movaps	%xmm0, 16(%rsp)
	movabsq	$-9223372036854775802, %rax     # imm = 0x8000000000000006
	movq	%rax, 32(%rsp)
	xorps	%xmm0, %xmm0
	movups	%xmm0, -88(%rsp)
	movups	%xmm0, -104(%rsp)
	movups	%xmm0, -120(%rsp)
	movq	$0, -72(%rsp)
	movabsq	$-4611685752139415551, %rax     # imm = 0xC000003E00000001
	movq	%rax, -128(%rsp)
	xorl	%ecx, %ecx
	xorl	%edx, %edx
	jmp	.LBB3_1
	.p2align	4
.LBB3_15:                               #   in Loop: Header=BB3_1 Depth=1
	cmpl	4(%rsi), %edx
	adcq	$2, %rsi
	movzbl	(%rsi), %esi
.LBB3_16:                               #   in Loop: Header=BB3_1 Depth=1
	addl	%esi, %ecx
.LBB3_17:                               #   in Loop: Header=BB3_1 Depth=1
	leal	1(%rcx), %esi
	cmpl	$12, %ecx
	movl	%esi, %ecx
	jge	.LBB3_53
.LBB3_1:                                # =>This Inner Loop Header: Depth=1
	movslq	%ecx, %rax
	leaq	(%rsp,%rax,8), %rsi
	addq	$-64, %rsi
	movzwl	-64(%rsp,%rax,8), %edi
	movl	$2, %eax
	cmpl	$31, %edi
	jle	.LBB3_2
# %bb.10:                               #   in Loop: Header=BB3_1 Depth=1
	cmpl	$53, %edi
	je	.LBB3_15
# %bb.11:                               #   in Loop: Header=BB3_1 Depth=1
	cmpl	$32, %edi
	jne	.LBB3_53
# %bb.12:                               #   in Loop: Header=BB3_1 Depth=1
	movl	4(%rsi), %edx
	leal	-61(%rdx), %esi
	cmpl	$-65, %esi
	jb	.LBB3_53
# %bb.13:                               #   in Loop: Header=BB3_1 Depth=1
	movzbl	-128(%rsp,%rdx), %esi
	leal	1(%rdx), %edi
	movzbl	-128(%rsp,%rdi), %edi
	shll	$8, %edi
	orl	%esi, %edi
	leal	2(%rdx), %esi
	movzbl	-128(%rsp,%rsi), %esi
	shll	$16, %esi
	orl	%edi, %esi
	addl	$3, %edx
	movzbl	-128(%rsp,%rdx), %edx
	shll	$24, %edx
	orl	%esi, %edx
	jmp	.LBB3_17
	.p2align	4
.LBB3_2:                                #   in Loop: Header=BB3_1 Depth=1
	cmpl	$21, %edi
	jne	.LBB3_3
# %bb.14:                               #   in Loop: Header=BB3_1 Depth=1
	xorl	%edi, %edi
	cmpl	4(%rsi), %edx
	sete	%dil
	xorq	$3, %rdi
	movzbl	(%rsi,%rdi), %esi
	jmp	.LBB3_16
.LBB3_3:
	cmpl	$6, %edi
	jne	.LBB3_53
# %bb.4:
	cmpl	$2147418112, 4(%rsi)            # imm = 0x7FFF0000
	jne	.LBB3_53
# %bb.5:
	movabsq	$-4611685752139415511, %rax     # imm = 0xC000003E00000029
	movq	%rax, -128(%rsp)
	xorl	%eax, %eax
	xorl	%ecx, %ecx
	jmp	.LBB3_6
	.p2align	4
.LBB3_23:                               #   in Loop: Header=BB3_6 Depth=1
	cmpl	4(%rdx), %ecx
	adcq	$2, %rdx
	movzbl	(%rdx), %edx
.LBB3_24:                               #   in Loop: Header=BB3_6 Depth=1
	addl	%edx, %eax
.LBB3_25:                               #   in Loop: Header=BB3_6 Depth=1
	leal	1(%rax), %edx
	cmpl	$12, %eax
	movl	%edx, %eax
	jge	.LBB3_26
.LBB3_6:                                # =>This Inner Loop Header: Depth=1
	movslq	%eax, %rsi
	leaq	(%rsp,%rsi,8), %rdx
	addq	$-64, %rdx
	movzwl	-64(%rsp,%rsi,8), %esi
	cmpl	$31, %esi
	jle	.LBB3_7
# %bb.18:                               #   in Loop: Header=BB3_6 Depth=1
	cmpl	$53, %esi
	je	.LBB3_23
# %bb.19:                               #   in Loop: Header=BB3_6 Depth=1
	cmpl	$32, %esi
	jne	.LBB3_26
# %bb.20:                               #   in Loop: Header=BB3_6 Depth=1
	movl	4(%rdx), %ecx
	leal	-61(%rcx), %edx
	cmpl	$-65, %edx
	jb	.LBB3_26
# %bb.21:                               #   in Loop: Header=BB3_6 Depth=1
	movzbl	-128(%rsp,%rcx), %edx
	leal	1(%rcx), %esi
	movzbl	-128(%rsp,%rsi), %esi
	shll	$8, %esi
	orl	%edx, %esi
	leal	2(%rcx), %edx
	movzbl	-128(%rsp,%rdx), %edx
	shll	$16, %edx
	orl	%esi, %edx
	addl	$3, %ecx
	movzbl	-128(%rsp,%rcx), %ecx
	shll	$24, %ecx
	orl	%edx, %ecx
	jmp	.LBB3_25
	.p2align	4
.LBB3_7:                                #   in Loop: Header=BB3_6 Depth=1
	cmpl	$21, %esi
	jne	.LBB3_8
# %bb.22:                               #   in Loop: Header=BB3_6 Depth=1
	xorl	%esi, %esi
	cmpl	4(%rdx), %ecx
	sete	%sil
	xorq	$3, %rsi
	movzbl	(%rdx,%rsi), %edx
	jmp	.LBB3_24
.LBB3_8:
	cmpl	$6, %esi
	jne	.LBB3_26
# %bb.9:
	movl	$3, %eax
	xorl	%ecx, %ecx
	cmpl	4(%rdx), %ecx
	jno	.LBB3_53
.LBB3_26:
	movabsq	$-4611685747844448255, %rax     # imm = 0xC000003F00000001
	movq	%rax, -128(%rsp)
	xorl	%eax, %eax
	xorl	%ecx, %ecx
	jmp	.LBB3_27
	.p2align	4
.LBB3_36:                               #   in Loop: Header=BB3_27 Depth=1
	cmpl	4(%rdx), %ecx
	adcq	$2, %rdx
	movzbl	(%rdx), %edx
.LBB3_37:                               #   in Loop: Header=BB3_27 Depth=1
	addl	%edx, %eax
.LBB3_38:                               #   in Loop: Header=BB3_27 Depth=1
	leal	1(%rax), %edx
	cmpl	$12, %eax
	movl	%edx, %eax
	jge	.LBB3_39
.LBB3_27:                               # =>This Inner Loop Header: Depth=1
	movslq	%eax, %rsi
	leaq	(%rsp,%rsi,8), %rdx
	addq	$-64, %rdx
	movzwl	-64(%rsp,%rsi,8), %esi
	cmpl	$31, %esi
	jle	.LBB3_28
# %bb.31:                               #   in Loop: Header=BB3_27 Depth=1
	cmpl	$53, %esi
	je	.LBB3_36
# %bb.32:                               #   in Loop: Header=BB3_27 Depth=1
	cmpl	$32, %esi
	jne	.LBB3_39
# %bb.33:                               #   in Loop: Header=BB3_27 Depth=1
	movl	4(%rdx), %ecx
	leal	-61(%rcx), %edx
	cmpl	$-65, %edx
	jb	.LBB3_39
# %bb.34:                               #   in Loop: Header=BB3_27 Depth=1
	movzbl	-128(%rsp,%rcx), %edx
	leal	1(%rcx), %esi
	movzbl	-128(%rsp,%rsi), %esi
	shll	$8, %esi
	orl	%edx, %esi
	leal	2(%rcx), %edx
	movzbl	-128(%rsp,%rdx), %edx
	shll	$16, %edx
	orl	%esi, %edx
	addl	$3, %ecx
	movzbl	-128(%rsp,%rcx), %ecx
	shll	$24, %ecx
	orl	%edx, %ecx
	jmp	.LBB3_38
	.p2align	4
.LBB3_28:                               #   in Loop: Header=BB3_27 Depth=1
	cmpl	$21, %esi
	jne	.LBB3_29
# %bb.35:                               #   in Loop: Header=BB3_27 Depth=1
	xorl	%esi, %esi
	cmpl	4(%rdx), %ecx
	sete	%sil
	xorq	$3, %rsi
	movzbl	(%rdx,%rsi), %edx
	jmp	.LBB3_37
.LBB3_29:
	cmpl	$6, %esi
	jne	.LBB3_39
# %bb.30:
	movl	$4, %eax
	xorl	%ecx, %ecx
	cmpl	4(%rdx), %ecx
	jno	.LBB3_53
.LBB3_39:
	movabsq	$-4611685751065673727, %rax     # imm = 0xC000003E40000001
	movq	%rax, -128(%rsp)
	xorl	%eax, %eax
	xorl	%ecx, %ecx
	jmp	.LBB3_40
	.p2align	4
.LBB3_49:                               #   in Loop: Header=BB3_40 Depth=1
	cmpl	4(%rdx), %ecx
	adcq	$2, %rdx
	movzbl	(%rdx), %edx
.LBB3_50:                               #   in Loop: Header=BB3_40 Depth=1
	addl	%edx, %eax
.LBB3_51:                               #   in Loop: Header=BB3_40 Depth=1
	leal	1(%rax), %edx
	cmpl	$12, %eax
	movl	%edx, %eax
	jge	.LBB3_52
.LBB3_40:                               # =>This Inner Loop Header: Depth=1
	movslq	%eax, %rsi
	leaq	(%rsp,%rsi,8), %rdx
	addq	$-64, %rdx
	movzwl	-64(%rsp,%rsi,8), %esi
	cmpl	$31, %esi
	jle	.LBB3_41
# %bb.44:                               #   in Loop: Header=BB3_40 Depth=1
	cmpl	$53, %esi
	je	.LBB3_49
# %bb.45:                               #   in Loop: Header=BB3_40 Depth=1
	cmpl	$32, %esi
	jne	.LBB3_52
# %bb.46:                               #   in Loop: Header=BB3_40 Depth=1
	movl	4(%rdx), %ecx
	leal	-61(%rcx), %edx
	cmpl	$-65, %edx
	jb	.LBB3_52
# %bb.47:                               #   in Loop: Header=BB3_40 Depth=1
	movzbl	-128(%rsp,%rcx), %edx
	leal	1(%rcx), %esi
	movzbl	-128(%rsp,%rsi), %esi
	shll	$8, %esi
	orl	%edx, %esi
	leal	2(%rcx), %edx
	movzbl	-128(%rsp,%rdx), %edx
	shll	$16, %edx
	orl	%esi, %edx
	addl	$3, %ecx
	movzbl	-128(%rsp,%rcx), %ecx
	shll	$24, %ecx
	orl	%edx, %ecx
	jmp	.LBB3_51
	.p2align	4
.LBB3_41:                               #   in Loop: Header=BB3_40 Depth=1
	cmpl	$21, %esi
	jne	.LBB3_42
# %bb.48:                               #   in Loop: Header=BB3_40 Depth=1
	xorl	%esi, %esi
	cmpl	4(%rdx), %ecx
	sete	%sil
	xorq	$3, %rsi
	movzbl	(%rdx,%rsi), %edx
	jmp	.LBB3_50
.LBB3_42:
	cmpl	$6, %esi
	jne	.LBB3_52
# %bb.43:
	movl	$5, %eax
	xorl	%ecx, %ecx
	cmpl	4(%rdx), %ecx
	jno	.LBB3_53
.LBB3_52:
	xorl	%eax, %eax
.LBB3_53:
	addq	$200, %rsp
	retq
.Lfunc_end3:
	.size	demo_selftest, .Lfunc_end3-demo_selftest
                                        # -- End function
	.ident	"clang version 20.1.8"
	.section	".note.GNU-stack","",@progbits
	.addrsig
