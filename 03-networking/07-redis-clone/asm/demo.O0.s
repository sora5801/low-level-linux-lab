	.file	"demo.c"
	.text
	.globl	dict_hash                       # -- Begin function dict_hash
	.p2align	4
	.type	dict_hash,@function
dict_hash:                              # @dict_hash
# %bb.0:
	pushq	%rbp
	movq	%rsp, %rbp
	movq	%rdi, -8(%rbp)
	movq	%rsi, -16(%rbp)
	movq	%rdx, -24(%rbp)
	movabsq	$-4132994306676758123, %rax     # imm = 0xC6A4A7935BD1E995
	movq	%rax, -32(%rbp)
	movl	$47, -36(%rbp)
	movq	-24(%rbp), %rax
	movabsq	$-4132994306676758123, %rcx     # imm = 0xC6A4A7935BD1E995
	imulq	-16(%rbp), %rcx
	xorq	%rcx, %rax
	movq	%rax, -48(%rbp)
	movq	-16(%rbp), %rax
	shrq	$3, %rax
	movq	%rax, -56(%rbp)
	movq	$0, -64(%rbp)
.LBB0_1:                                # =>This Inner Loop Header: Depth=1
	movq	-64(%rbp), %rax
	cmpq	-56(%rbp), %rax
	jae	.LBB0_4
# %bb.2:                                #   in Loop: Header=BB0_1 Depth=1
	movq	-8(%rbp), %rax
	movq	-64(%rbp), %rcx
	shlq	$3, %rcx
	addq	%rcx, %rax
	movq	%rax, -72(%rbp)
	movq	-72(%rbp), %rax
	movzbl	(%rax), %eax
                                        # kill: def $rax killed $eax
	movq	-72(%rbp), %rcx
	movzbl	1(%rcx), %ecx
                                        # kill: def $rcx killed $ecx
	shlq	$8, %rcx
	orq	%rcx, %rax
	movq	-72(%rbp), %rcx
	movzbl	2(%rcx), %ecx
                                        # kill: def $rcx killed $ecx
	shlq	$16, %rcx
	orq	%rcx, %rax
	movq	-72(%rbp), %rcx
	movzbl	3(%rcx), %ecx
                                        # kill: def $rcx killed $ecx
	shlq	$24, %rcx
	orq	%rcx, %rax
	movq	-72(%rbp), %rcx
	movzbl	4(%rcx), %ecx
                                        # kill: def $rcx killed $ecx
	shlq	$32, %rcx
	orq	%rcx, %rax
	movq	-72(%rbp), %rcx
	movzbl	5(%rcx), %ecx
                                        # kill: def $rcx killed $ecx
	shlq	$40, %rcx
	orq	%rcx, %rax
	movq	-72(%rbp), %rcx
	movzbl	6(%rcx), %ecx
                                        # kill: def $rcx killed $ecx
	shlq	$48, %rcx
	orq	%rcx, %rax
	movq	-72(%rbp), %rcx
	movzbl	7(%rcx), %ecx
                                        # kill: def $rcx killed $ecx
	shlq	$56, %rcx
	orq	%rcx, %rax
	movq	%rax, -80(%rbp)
	movabsq	$-4132994306676758123, %rax     # imm = 0xC6A4A7935BD1E995
	imulq	-80(%rbp), %rax
	movq	%rax, -80(%rbp)
	movq	-80(%rbp), %rax
	shrq	$47, %rax
	xorq	-80(%rbp), %rax
	movq	%rax, -80(%rbp)
	movabsq	$-4132994306676758123, %rax     # imm = 0xC6A4A7935BD1E995
	imulq	-80(%rbp), %rax
	movq	%rax, -80(%rbp)
	movq	-80(%rbp), %rax
	xorq	-48(%rbp), %rax
	movq	%rax, -48(%rbp)
	movabsq	$-4132994306676758123, %rax     # imm = 0xC6A4A7935BD1E995
	imulq	-48(%rbp), %rax
	movq	%rax, -48(%rbp)
# %bb.3:                                #   in Loop: Header=BB0_1 Depth=1
	movq	-64(%rbp), %rax
	addq	$1, %rax
	movq	%rax, -64(%rbp)
	jmp	.LBB0_1
.LBB0_4:
	movq	-8(%rbp), %rax
	movq	-56(%rbp), %rcx
	leaq	(%rax,%rcx,8), %rax
	movq	%rax, -88(%rbp)
	movq	-16(%rbp), %rax
                                        # kill: def $eax killed $eax killed $rax
	andl	$7, %eax
                                        # kill: def $rax killed $eax
	movq	%rax, -96(%rbp)                 # 8-byte Spill
	subq	$1, %rax
	je	.LBB0_11
	jmp	.LBB0_13
.LBB0_13:
	movq	-96(%rbp), %rax                 # 8-byte Reload
	subq	$2, %rax
	je	.LBB0_10
	jmp	.LBB0_14
.LBB0_14:
	movq	-96(%rbp), %rax                 # 8-byte Reload
	subq	$3, %rax
	je	.LBB0_9
	jmp	.LBB0_15
.LBB0_15:
	movq	-96(%rbp), %rax                 # 8-byte Reload
	subq	$4, %rax
	je	.LBB0_8
	jmp	.LBB0_16
.LBB0_16:
	movq	-96(%rbp), %rax                 # 8-byte Reload
	subq	$5, %rax
	je	.LBB0_7
	jmp	.LBB0_17
.LBB0_17:
	movq	-96(%rbp), %rax                 # 8-byte Reload
	subq	$6, %rax
	je	.LBB0_6
	jmp	.LBB0_18
.LBB0_18:
	movq	-96(%rbp), %rax                 # 8-byte Reload
	subq	$7, %rax
	jne	.LBB0_12
	jmp	.LBB0_5
.LBB0_5:
	movq	-88(%rbp), %rax
	movzbl	6(%rax), %eax
                                        # kill: def $rax killed $eax
	shlq	$48, %rax
	xorq	-48(%rbp), %rax
	movq	%rax, -48(%rbp)
.LBB0_6:
	movq	-88(%rbp), %rax
	movzbl	5(%rax), %eax
                                        # kill: def $rax killed $eax
	shlq	$40, %rax
	xorq	-48(%rbp), %rax
	movq	%rax, -48(%rbp)
.LBB0_7:
	movq	-88(%rbp), %rax
	movzbl	4(%rax), %eax
                                        # kill: def $rax killed $eax
	shlq	$32, %rax
	xorq	-48(%rbp), %rax
	movq	%rax, -48(%rbp)
.LBB0_8:
	movq	-88(%rbp), %rax
	movzbl	3(%rax), %eax
                                        # kill: def $rax killed $eax
	shlq	$24, %rax
	xorq	-48(%rbp), %rax
	movq	%rax, -48(%rbp)
.LBB0_9:
	movq	-88(%rbp), %rax
	movzbl	2(%rax), %eax
                                        # kill: def $rax killed $eax
	shlq	$16, %rax
	xorq	-48(%rbp), %rax
	movq	%rax, -48(%rbp)
.LBB0_10:
	movq	-88(%rbp), %rax
	movzbl	1(%rax), %eax
                                        # kill: def $rax killed $eax
	shlq	$8, %rax
	xorq	-48(%rbp), %rax
	movq	%rax, -48(%rbp)
.LBB0_11:
	movq	-88(%rbp), %rax
	movzbl	(%rax), %eax
                                        # kill: def $rax killed $eax
	xorq	-48(%rbp), %rax
	movq	%rax, -48(%rbp)
	movabsq	$-4132994306676758123, %rax     # imm = 0xC6A4A7935BD1E995
	imulq	-48(%rbp), %rax
	movq	%rax, -48(%rbp)
.LBB0_12:
	movq	-48(%rbp), %rax
	shrq	$47, %rax
	xorq	-48(%rbp), %rax
	movq	%rax, -48(%rbp)
	movabsq	$-4132994306676758123, %rax     # imm = 0xC6A4A7935BD1E995
	imulq	-48(%rbp), %rax
	movq	%rax, -48(%rbp)
	movq	-48(%rbp), %rax
	shrq	$47, %rax
	xorq	-48(%rbp), %rax
	movq	%rax, -48(%rbp)
	movq	-48(%rbp), %rax
	popq	%rbp
	retq
.Lfunc_end0:
	.size	dict_hash, .Lfunc_end0-dict_hash
                                        # -- End function
	.globl	rehash_target_index             # -- Begin function rehash_target_index
	.p2align	4
	.type	rehash_target_index,@function
rehash_target_index:                    # @rehash_target_index
# %bb.0:
	pushq	%rbp
	movq	%rsp, %rbp
	movq	%rdi, -8(%rbp)
	movq	%rsi, -16(%rbp)
	movq	%rdx, -24(%rbp)
	movq	%rcx, -32(%rbp)
	cmpq	$0, -32(%rbp)
	jl	.LBB1_2
# %bb.1:
	movq	-24(%rbp), %rax
	movq	%rax, -48(%rbp)                 # 8-byte Spill
	jmp	.LBB1_3
.LBB1_2:
	movq	-16(%rbp), %rax
	movq	%rax, -48(%rbp)                 # 8-byte Spill
.LBB1_3:
	movq	-48(%rbp), %rax                 # 8-byte Reload
	movq	%rax, -40(%rbp)
	movq	-8(%rbp), %rax
	andq	-40(%rbp), %rax
	popq	%rbp
	retq
.Lfunc_end1:
	.size	rehash_target_index, .Lfunc_end1-rehash_target_index
                                        # -- End function
	.ident	"clang version 20.1.8"
	.section	".note.GNU-stack","",@progbits
	.addrsig
