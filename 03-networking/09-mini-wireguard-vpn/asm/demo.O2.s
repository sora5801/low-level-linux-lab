	.file	"demo.c"
	.text
	.globl	chacha_quarter_round            # -- Begin function chacha_quarter_round
	.p2align	4
	.type	chacha_quarter_round,@function
chacha_quarter_round:                   # @chacha_quarter_round
# %bb.0:
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
	pushq	%r15
	pushq	%r14
	pushq	%r13
	pushq	%r12
	pushq	%rbx
	subq	$40, %rsp
	movq	%rdi, -40(%rsp)                 # 8-byte Spill
	movaps	.LCPI1_0(%rip), %xmm0           # xmm0 = [1634760805,857760878,2036477234,1797285236]
	movaps	%xmm0, -32(%rsp)
	movups	(%rsi), %xmm0
	movups	16(%rsi), %xmm1
	movaps	%xmm0, -16(%rsp)
	movaps	%xmm1, (%rsp)
	movl	%edx, 16(%rsp)
	movq	(%rcx), %rax
	movq	%rax, 20(%rsp)
	movl	8(%rcx), %eax
	movl	%eax, 28(%rsp)
	movaps	%xmm0, -96(%rsp)
	movaps	-32(%rsp), %xmm0
	movaps	%xmm0, -112(%rsp)
	movaps	%xmm1, -80(%rsp)
	movl	16(%rsp), %eax
	movl	%eax, -64(%rsp)
	movq	20(%rsp), %rax
	movq	%rax, -60(%rsp)
	movl	28(%rsp), %eax
	movl	%eax, -52(%rsp)
	movl	-96(%rsp), %esi
	movl	-92(%rsp), %eax
	movl	-112(%rsp), %r12d
	movl	-108(%rsp), %ecx
	movl	%ecx, -116(%rsp)                # 4-byte Spill
	movl	-64(%rsp), %ecx
	movl	-60(%rsp), %edx
	movl	-80(%rsp), %edi
	movl	-76(%rsp), %r8d
	movl	%r8d, -120(%rsp)                # 4-byte Spill
	movl	-88(%rsp), %r9d
	movl	-104(%rsp), %r10d
	movl	-56(%rsp), %r11d
	movl	-72(%rsp), %ebx
	movl	-84(%rsp), %ebp
	movl	-100(%rsp), %r14d
	movl	-52(%rsp), %r15d
	movl	$10, %r8d
	movl	-68(%rsp), %r13d
	.p2align	4
.LBB1_1:                                # =>This Inner Loop Header: Depth=1
	movl	%r10d, -48(%rsp)                # 4-byte Spill
	movl	%r8d, -44(%rsp)                 # 4-byte Spill
	addl	%esi, %r12d
	xorl	%r12d, %ecx
	roll	$16, %ecx
	addl	%ecx, %edi
	xorl	%edi, %esi
	roll	$12, %esi
	addl	%esi, %r12d
	xorl	%r12d, %ecx
	roll	$8, %ecx
	addl	%ecx, %edi
	xorl	%edi, %esi
	roll	$7, %esi
	movl	-116(%rsp), %r8d                # 4-byte Reload
	addl	%eax, %r8d
	xorl	%r8d, %edx
	roll	$16, %edx
	movl	-120(%rsp), %r10d               # 4-byte Reload
	addl	%edx, %r10d
	xorl	%r10d, %eax
	roll	$12, %eax
	addl	%eax, %r8d
	xorl	%r8d, %edx
	roll	$8, %edx
	addl	%edx, %r10d
	movl	%r10d, -120(%rsp)               # 4-byte Spill
	xorl	%r10d, %eax
	movl	-48(%rsp), %r10d                # 4-byte Reload
	roll	$7, %eax
	addl	%r9d, %r10d
	xorl	%r10d, %r11d
	roll	$16, %r11d
	addl	%r11d, %ebx
	xorl	%ebx, %r9d
	roll	$12, %r9d
	addl	%r9d, %r10d
	xorl	%r10d, %r11d
	roll	$8, %r11d
	addl	%r11d, %ebx
	xorl	%ebx, %r9d
	roll	$7, %r9d
	addl	%ebp, %r14d
	xorl	%r14d, %r15d
	roll	$16, %r15d
	addl	%r15d, %r13d
	xorl	%r13d, %ebp
	roll	$12, %ebp
	addl	%ebp, %r14d
	xorl	%r14d, %r15d
	roll	$8, %r15d
	addl	%r15d, %r13d
	xorl	%r13d, %ebp
	roll	$7, %ebp
	addl	%eax, %r12d
	xorl	%r12d, %r15d
	roll	$16, %r15d
	addl	%r15d, %ebx
	xorl	%ebx, %eax
	roll	$12, %eax
	addl	%eax, %r12d
	xorl	%r12d, %r15d
	roll	$8, %r15d
	addl	%r15d, %ebx
	xorl	%ebx, %eax
	roll	$7, %eax
	addl	%r9d, %r8d
	xorl	%r8d, %ecx
	roll	$16, %ecx
	addl	%ecx, %r13d
	xorl	%r13d, %r9d
	roll	$12, %r9d
	addl	%r9d, %r8d
	movl	%r8d, -116(%rsp)                # 4-byte Spill
	xorl	%r8d, %ecx
	roll	$8, %ecx
	addl	%ecx, %r13d
	xorl	%r13d, %r9d
	roll	$7, %r9d
	addl	%ebp, %r10d
	xorl	%r10d, %edx
	roll	$16, %edx
	addl	%edx, %edi
	xorl	%edi, %ebp
	roll	$12, %ebp
	addl	%ebp, %r10d
	xorl	%r10d, %edx
	roll	$8, %edx
	addl	%edx, %edi
	xorl	%edi, %ebp
	roll	$7, %ebp
	addl	%esi, %r14d
	xorl	%r14d, %r11d
	roll	$16, %r11d
	movl	-120(%rsp), %r8d                # 4-byte Reload
	addl	%r11d, %r8d
	xorl	%r8d, %esi
	roll	$12, %esi
	addl	%esi, %r14d
	xorl	%r14d, %r11d
	roll	$8, %r11d
	addl	%r11d, %r8d
	movl	%r8d, -120(%rsp)                # 4-byte Spill
	xorl	%r8d, %esi
	roll	$7, %esi
	movl	-44(%rsp), %r8d                 # 4-byte Reload
	decl	%r8d
	jne	.LBB1_1
# %bb.2:
	movl	%esi, -96(%rsp)
	movl	%r12d, -112(%rsp)
	movl	%ecx, -64(%rsp)
	movl	%edi, -80(%rsp)
	movl	%eax, -92(%rsp)
	movl	-116(%rsp), %eax                # 4-byte Reload
	movl	%eax, -108(%rsp)
	movl	%edx, -60(%rsp)
	movl	-120(%rsp), %eax                # 4-byte Reload
	movl	%eax, -76(%rsp)
	movl	%r9d, -88(%rsp)
	movl	%r10d, -104(%rsp)
	movl	%r11d, -56(%rsp)
	movl	%ebx, -72(%rsp)
	movl	%ebp, -84(%rsp)
	movl	%r14d, -100(%rsp)
	movl	%r15d, -52(%rsp)
	movl	%r13d, -68(%rsp)
	movl	$1, %eax
	movq	-40(%rsp), %rdx                 # 8-byte Reload
	.p2align	4
.LBB1_3:                                # =>This Inner Loop Header: Depth=1
	movl	-36(%rsp,%rax,4), %ecx
	addl	-116(%rsp,%rax,4), %ecx
	movl	%ecx, -4(%rdx,%rax,4)
	movl	-32(%rsp,%rax,4), %ecx
	addl	-112(%rsp,%rax,4), %ecx
	movl	%ecx, (%rdx,%rax,4)
	addq	$2, %rax
	cmpq	$17, %rax
	jne	.LBB1_3
# %bb.4:
	addq	$40, %rsp
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
