	.file	"demo.c"
	.text
	.globl	crc32_ieee                      # -- Begin function crc32_ieee
	.p2align	4
	.type	crc32_ieee,@function
crc32_ieee:                             # @crc32_ieee
# %bb.0:
	pushq	%rbp
	movq	%rsp, %rbp
	movq	%rdi, -8(%rbp)
	movq	%rsi, -16(%rbp)
	movl	$-1, -20(%rbp)
	movq	$0, -32(%rbp)
.LBB0_1:                                # =>This Loop Header: Depth=1
                                        #     Child Loop BB0_3 Depth 2
	movq	-32(%rbp), %rax
	cmpq	-16(%rbp), %rax
	jae	.LBB0_8
# %bb.2:                                #   in Loop: Header=BB0_1 Depth=1
	movq	-8(%rbp), %rax
	movq	-32(%rbp), %rcx
	movzbl	(%rax,%rcx), %eax
	xorl	-20(%rbp), %eax
	movl	%eax, -20(%rbp)
	movl	$0, -36(%rbp)
.LBB0_3:                                #   Parent Loop BB0_1 Depth=1
                                        # =>  This Inner Loop Header: Depth=2
	cmpl	$8, -36(%rbp)
	jge	.LBB0_6
# %bb.4:                                #   in Loop: Header=BB0_3 Depth=2
	movl	-20(%rbp), %eax
	shrl	%eax
	movl	-20(%rbp), %ecx
	andl	$1, %ecx
	xorl	$-1, %ecx
	addl	$1, %ecx
	andl	$3988292384, %ecx               # imm = 0xEDB88320
	xorl	%ecx, %eax
	movl	%eax, -20(%rbp)
# %bb.5:                                #   in Loop: Header=BB0_3 Depth=2
	movl	-36(%rbp), %eax
	addl	$1, %eax
	movl	%eax, -36(%rbp)
	jmp	.LBB0_3
.LBB0_6:                                #   in Loop: Header=BB0_1 Depth=1
	jmp	.LBB0_7
.LBB0_7:                                #   in Loop: Header=BB0_1 Depth=1
	movq	-32(%rbp), %rax
	addq	$1, %rax
	movq	%rax, -32(%rbp)
	jmp	.LBB0_1
.LBB0_8:
	movl	-20(%rbp), %eax
	xorl	$-1, %eax
	popq	%rbp
	retq
.Lfunc_end0:
	.size	crc32_ieee, .Lfunc_end0-crc32_ieee
                                        # -- End function
	.globl	wal_frame_record                # -- Begin function wal_frame_record
	.p2align	4
	.type	wal_frame_record,@function
wal_frame_record:                       # @wal_frame_record
# %bb.0:
	pushq	%rbp
	movq	%rsp, %rbp
	subq	$80, %rsp
	movb	%sil, %al
	movq	%rdi, -8(%rbp)
	movb	%al, -9(%rbp)
	movq	%rdx, -24(%rbp)
	movl	%ecx, -28(%rbp)
	movq	%r8, -40(%rbp)
	movl	%r9d, -44(%rbp)
	movzbl	-9(%rbp), %eax
	cmpl	$2, %eax
	jne	.LBB1_2
# %bb.1:
	movl	$0, -44(%rbp)
.LBB1_2:
	movq	-8(%rbp), %rax
	addq	$8, %rax
	movq	%rax, -56(%rbp)
	movq	$0, -64(%rbp)
	movb	-9(%rbp), %dl
	movq	-56(%rbp), %rax
	movq	-64(%rbp), %rcx
	movq	%rcx, %rsi
	addq	$1, %rsi
	movq	%rsi, -64(%rbp)
	movb	%dl, (%rax,%rcx)
	movq	-56(%rbp), %rdi
	addq	-64(%rbp), %rdi
	movl	-28(%rbp), %esi
	callq	put_u32
	movq	-64(%rbp), %rax
	addq	$4, %rax
	movq	%rax, -64(%rbp)
	movq	-56(%rbp), %rdi
	addq	-64(%rbp), %rdi
	movq	-24(%rbp), %rsi
	movl	-28(%rbp), %edx
	callq	copy_bytes
	movl	-28(%rbp), %eax
                                        # kill: def $rax killed $eax
	addq	-64(%rbp), %rax
	movq	%rax, -64(%rbp)
	movq	-56(%rbp), %rdi
	addq	-64(%rbp), %rdi
	movl	-44(%rbp), %esi
	callq	put_u32
	movq	-64(%rbp), %rax
	addq	$4, %rax
	movq	%rax, -64(%rbp)
	movq	-56(%rbp), %rdi
	addq	-64(%rbp), %rdi
	movq	-40(%rbp), %rsi
	movl	-44(%rbp), %edx
	callq	copy_bytes
	movl	-44(%rbp), %eax
                                        # kill: def $rax killed $eax
	addq	-64(%rbp), %rax
	movq	%rax, -64(%rbp)
	movq	-8(%rbp), %rdi
	movq	-64(%rbp), %rax
	movl	%eax, %esi
	callq	put_u32
	movq	-8(%rbp), %rax
	addq	$4, %rax
	movq	%rax, -72(%rbp)                 # 8-byte Spill
	movq	-56(%rbp), %rdi
	movq	-64(%rbp), %rsi
	callq	crc32_ieee
	movq	-72(%rbp), %rdi                 # 8-byte Reload
	movl	%eax, %esi
	callq	put_u32
	movq	-64(%rbp), %rax
	addq	$8, %rax
	addq	$80, %rsp
	popq	%rbp
	retq
.Lfunc_end1:
	.size	wal_frame_record, .Lfunc_end1-wal_frame_record
                                        # -- End function
	.p2align	4                               # -- Begin function put_u32
	.type	put_u32,@function
put_u32:                                # @put_u32
# %bb.0:
	pushq	%rbp
	movq	%rsp, %rbp
	movq	%rdi, -8(%rbp)
	movl	%esi, -12(%rbp)
	movl	-12(%rbp), %eax
	movb	%al, %cl
	movq	-8(%rbp), %rax
	movb	%cl, (%rax)
	movl	-12(%rbp), %eax
	shrl	$8, %eax
	movb	%al, %cl
	movq	-8(%rbp), %rax
	movb	%cl, 1(%rax)
	movl	-12(%rbp), %eax
	shrl	$16, %eax
	movb	%al, %cl
	movq	-8(%rbp), %rax
	movb	%cl, 2(%rax)
	movl	-12(%rbp), %eax
	shrl	$24, %eax
	movb	%al, %cl
	movq	-8(%rbp), %rax
	movb	%cl, 3(%rax)
	popq	%rbp
	retq
.Lfunc_end2:
	.size	put_u32, .Lfunc_end2-put_u32
                                        # -- End function
	.p2align	4                               # -- Begin function copy_bytes
	.type	copy_bytes,@function
copy_bytes:                             # @copy_bytes
# %bb.0:
	pushq	%rbp
	movq	%rsp, %rbp
	movq	%rdi, -8(%rbp)
	movq	%rsi, -16(%rbp)
	movl	%edx, -20(%rbp)
	movl	$0, -24(%rbp)
.LBB3_1:                                # =>This Inner Loop Header: Depth=1
	movl	-24(%rbp), %eax
	cmpl	-20(%rbp), %eax
	jae	.LBB3_4
# %bb.2:                                #   in Loop: Header=BB3_1 Depth=1
	movq	-16(%rbp), %rax
	movl	-24(%rbp), %ecx
                                        # kill: def $rcx killed $ecx
	movb	(%rax,%rcx), %dl
	movq	-8(%rbp), %rax
	movl	-24(%rbp), %ecx
                                        # kill: def $rcx killed $ecx
	movb	%dl, (%rax,%rcx)
# %bb.3:                                #   in Loop: Header=BB3_1 Depth=1
	movl	-24(%rbp), %eax
	addl	$1, %eax
	movl	%eax, -24(%rbp)
	jmp	.LBB3_1
.LBB3_4:
	popq	%rbp
	retq
.Lfunc_end3:
	.size	copy_bytes, .Lfunc_end3-copy_bytes
                                        # -- End function
	.ident	"clang version 20.1.8"
	.section	".note.GNU-stack","",@progbits
	.addrsig
	.addrsig_sym crc32_ieee
	.addrsig_sym put_u32
	.addrsig_sym copy_bytes
