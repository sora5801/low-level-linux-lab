	.file	"demo.c"
	.text
	.globl	chacha_quarter_round            # -- Begin function chacha_quarter_round
	.p2align	4
	.type	chacha_quarter_round,@function
chacha_quarter_round:                   # @chacha_quarter_round
# %bb.0:
	pushq	%rbp
	movq	%rsp, %rbp
	movl	%edx, %eax
	movl	%esi, %esi
	movl	(%rdi,%rsi,4), %edx
	addl	(%rdi,%rax,4), %edx
	movl	%edx, (%rdi,%rsi,4)
	movl	%r8d, %r8d
	xorl	(%rdi,%r8,4), %edx
	roll	$16, %edx
	movl	%edx, (%rdi,%r8,4)
	movl	%ecx, %ecx
	addl	(%rdi,%rcx,4), %edx
	movl	%edx, (%rdi,%rcx,4)
	xorl	(%rdi,%rax,4), %edx
	roll	$12, %edx
	movl	%edx, (%rdi,%rax,4)
	addl	(%rdi,%rsi,4), %edx
	movl	%edx, (%rdi,%rsi,4)
	xorl	(%rdi,%r8,4), %edx
	roll	$8, %edx
	movl	%edx, (%rdi,%r8,4)
	addl	(%rdi,%rcx,4), %edx
	movl	%edx, (%rdi,%rcx,4)
	xorl	(%rdi,%rax,4), %edx
	roll	$7, %edx
	movl	%edx, (%rdi,%rax,4)
	popq	%rbp
	retq
.Lfunc_end0:
	.size	chacha_quarter_round, .Lfunc_end0-chacha_quarter_round
                                        # -- End function
	.section	.rodata.cst16,"aM",@progbits,16
	.p2align	4, 0x0                          # -- Begin function chacha20_block
.LCPI1_0:
	.long	1634760805                      # 0x61707865
	.long	857760878                       # 0x3320646e
	.long	2036477234                      # 0x79622d32
	.long	1797285236                      # 0x6b206574
	.text
	.globl	chacha20_block
	.p2align	4
	.type	chacha20_block,@function
chacha20_block:                         # @chacha20_block
# %bb.0:
	pushq	%rbp
	movq	%rsp, %rbp
	pushq	%r15
	pushq	%r14
	pushq	%r13
	pushq	%r12
	pushq	%rbx
	subq	$56, %rsp
	movq	%rdi, -160(%rbp)                # 8-byte Spill
	movaps	.LCPI1_0(%rip), %xmm0           # xmm0 = [1634760805,857760878,2036477234,1797285236]
	movaps	%xmm0, -224(%rbp)
	xorl	%eax, %eax
	.p2align	4
.LBB1_1:                                # =>This Inner Loop Header: Depth=1
	movl	(%rsi,%rax,4), %edi
	movl	%edi, -208(%rbp,%rax,4)
	incq	%rax
	cmpq	$8, %rax
	jne	.LBB1_1
# %bb.2:
	movl	%edx, -176(%rbp)
	movl	(%rcx), %eax
	movl	%eax, -172(%rbp)
	movl	4(%rcx), %eax
	movl	%eax, -168(%rbp)
	movl	8(%rcx), %eax
	movl	%eax, -164(%rbp)
	movaps	-224(%rbp), %xmm0
	movaps	-208(%rbp), %xmm1
	movaps	-192(%rbp), %xmm2
	movaps	%xmm1, -128(%rbp)
	movaps	%xmm0, -144(%rbp)
	movl	-176(%rbp), %eax
	movl	%eax, -96(%rbp)
	movl	-172(%rbp), %eax
	movl	%eax, -92(%rbp)
	movl	-168(%rbp), %eax
	movl	%eax, -88(%rbp)
	movl	-164(%rbp), %eax
	movl	%eax, -84(%rbp)
	movaps	%xmm2, -112(%rbp)
	movl	-128(%rbp), %esi
	movl	-124(%rbp), %eax
	movl	-144(%rbp), %ecx
	movl	%ecx, -48(%rbp)                 # 4-byte Spill
	movl	-140(%rbp), %ecx
	movl	%ecx, -68(%rbp)                 # 4-byte Spill
	movl	-96(%rbp), %edi
	movl	-92(%rbp), %edx
	movl	-112(%rbp), %r10d
	movl	-108(%rbp), %ecx
	movl	%ecx, -44(%rbp)                 # 4-byte Spill
	movl	-120(%rbp), %r9d
	movl	-136(%rbp), %ecx
	movl	%ecx, -64(%rbp)                 # 4-byte Spill
	movl	-88(%rbp), %r11d
	movl	-104(%rbp), %ecx
	movl	%ecx, -60(%rbp)                 # 4-byte Spill
	movl	-116(%rbp), %r14d
	movl	-132(%rbp), %ecx
	movl	%ecx, -56(%rbp)                 # 4-byte Spill
	movl	-84(%rbp), %r12d
	movl	$10, %r8d
	movl	-100(%rbp), %ecx
	movl	%ecx, -52(%rbp)                 # 4-byte Spill
	.p2align	4
.LBB1_3:                                # =>This Inner Loop Header: Depth=1
	movl	%r8d, -148(%rbp)                # 4-byte Spill
	movl	-48(%rbp), %r8d                 # 4-byte Reload
	addl	%esi, %r8d
	xorl	%r8d, %edi
	roll	$16, %edi
	addl	%edi, %r10d
	xorl	%r10d, %esi
	roll	$12, %esi
	addl	%esi, %r8d
	xorl	%r8d, %edi
	roll	$8, %edi
	addl	%edi, %r10d
	movl	%r10d, -152(%rbp)               # 4-byte Spill
	xorl	%r10d, %esi
	roll	$7, %esi
	movl	-68(%rbp), %r10d                # 4-byte Reload
	addl	%eax, %r10d
	xorl	%r10d, %edx
	roll	$16, %edx
	movl	-44(%rbp), %ecx                 # 4-byte Reload
	addl	%edx, %ecx
	xorl	%ecx, %eax
	roll	$12, %eax
	addl	%eax, %r10d
	xorl	%r10d, %edx
	roll	$8, %edx
	addl	%edx, %ecx
	movl	%ecx, -44(%rbp)                 # 4-byte Spill
	xorl	%ecx, %eax
	roll	$7, %eax
	movl	-64(%rbp), %ebx                 # 4-byte Reload
	addl	%r9d, %ebx
	xorl	%ebx, %r11d
	roll	$16, %r11d
	movl	-60(%rbp), %r15d                # 4-byte Reload
	addl	%r11d, %r15d
	xorl	%r15d, %r9d
	roll	$12, %r9d
	addl	%r9d, %ebx
	xorl	%ebx, %r11d
	roll	$8, %r11d
	addl	%r11d, %r15d
	xorl	%r15d, %r9d
	roll	$7, %r9d
	movl	-56(%rbp), %ecx                 # 4-byte Reload
	addl	%r14d, %ecx
	xorl	%ecx, %r12d
	roll	$16, %r12d
	movl	-52(%rbp), %r13d                # 4-byte Reload
	addl	%r12d, %r13d
	xorl	%r13d, %r14d
	roll	$12, %r14d
	addl	%r14d, %ecx
	xorl	%ecx, %r12d
	roll	$8, %r12d
	addl	%r12d, %r13d
	xorl	%r13d, %r14d
	roll	$7, %r14d
	addl	%eax, %r8d
	xorl	%r8d, %r12d
	roll	$16, %r12d
	addl	%r12d, %r15d
	xorl	%r15d, %eax
	roll	$12, %eax
	addl	%eax, %r8d
	movl	%r8d, -48(%rbp)                 # 4-byte Spill
	xorl	%r8d, %r12d
	movl	-148(%rbp), %r8d                # 4-byte Reload
	roll	$8, %r12d
	addl	%r12d, %r15d
	movl	%r15d, -60(%rbp)                # 4-byte Spill
	xorl	%r15d, %eax
	roll	$7, %eax
	addl	%r9d, %r10d
	xorl	%r10d, %edi
	roll	$16, %edi
	addl	%edi, %r13d
	xorl	%r13d, %r9d
	roll	$12, %r9d
	addl	%r9d, %r10d
	movl	%r10d, -68(%rbp)                # 4-byte Spill
	xorl	%r10d, %edi
	movl	-152(%rbp), %r10d               # 4-byte Reload
	roll	$8, %edi
	addl	%edi, %r13d
	movl	%r13d, -52(%rbp)                # 4-byte Spill
	xorl	%r13d, %r9d
	roll	$7, %r9d
	addl	%r14d, %ebx
	xorl	%ebx, %edx
	roll	$16, %edx
	addl	%edx, %r10d
	xorl	%r10d, %r14d
	roll	$12, %r14d
	addl	%r14d, %ebx
	movl	%ebx, -64(%rbp)                 # 4-byte Spill
	xorl	%ebx, %edx
	roll	$8, %edx
	addl	%edx, %r10d
	xorl	%r10d, %r14d
	roll	$7, %r14d
	addl	%esi, %ecx
	xorl	%ecx, %r11d
	roll	$16, %r11d
	movl	-44(%rbp), %ebx                 # 4-byte Reload
	addl	%r11d, %ebx
	xorl	%ebx, %esi
	roll	$12, %esi
	addl	%esi, %ecx
	movl	%ecx, -56(%rbp)                 # 4-byte Spill
	xorl	%ecx, %r11d
	roll	$8, %r11d
	addl	%r11d, %ebx
	movl	%ebx, -44(%rbp)                 # 4-byte Spill
	xorl	%ebx, %esi
	roll	$7, %esi
	decl	%r8d
	jne	.LBB1_3
# %bb.4:
	movl	%esi, -128(%rbp)
	movl	-48(%rbp), %ecx                 # 4-byte Reload
	movl	%ecx, -144(%rbp)
	movl	%edi, -96(%rbp)
	movl	%r10d, -112(%rbp)
	movl	%eax, -124(%rbp)
	movl	-68(%rbp), %eax                 # 4-byte Reload
	movl	%eax, -140(%rbp)
	movl	%edx, -92(%rbp)
	movl	-44(%rbp), %eax                 # 4-byte Reload
	movl	%eax, -108(%rbp)
	movl	%r9d, -120(%rbp)
	movl	-64(%rbp), %eax                 # 4-byte Reload
	movl	%eax, -136(%rbp)
	movl	%r11d, -88(%rbp)
	movl	-60(%rbp), %eax                 # 4-byte Reload
	movl	%eax, -104(%rbp)
	movl	%r14d, -116(%rbp)
	movl	-56(%rbp), %eax                 # 4-byte Reload
	movl	%eax, -132(%rbp)
	movl	%r12d, -84(%rbp)
	movl	-52(%rbp), %eax                 # 4-byte Reload
	movl	%eax, -100(%rbp)
	xorl	%eax, %eax
	movq	-160(%rbp), %rdx                # 8-byte Reload
	.p2align	4
.LBB1_5:                                # =>This Inner Loop Header: Depth=1
	movl	-224(%rbp,%rax,4), %ecx
	addl	-144(%rbp,%rax,4), %ecx
	movl	%ecx, (%rdx,%rax,4)
	incq	%rax
	cmpq	$16, %rax
	jne	.LBB1_5
# %bb.6:
	addq	$56, %rsp
	popq	%rbx
	popq	%r12
	popq	%r13
	popq	%r14
	popq	%r15
	popq	%rbp
	retq
.Lfunc_end1:
	.size	chacha20_block, .Lfunc_end1-chacha20_block
                                        # -- End function
	.ident	"clang version 20.1.8"
	.section	".note.GNU-stack","",@progbits
	.addrsig
