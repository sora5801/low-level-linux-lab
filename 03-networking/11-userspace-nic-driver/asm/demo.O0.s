	.file	"demo.c"
	.text
	.globl	ring_next                       # -- Begin function ring_next
	.p2align	4
	.type	ring_next,@function
ring_next:                              # @ring_next
# %bb.0:
	pushq	%rbp
	movq	%rsp, %rbp
	movw	%si, %ax
	movw	%di, %cx
	movw	%cx, -2(%rbp)
	movw	%ax, -4(%rbp)
	movzwl	-2(%rbp), %eax
	addl	$1, %eax
	movzwl	-4(%rbp), %ecx
	subl	$1, %ecx
	andl	%ecx, %eax
                                        # kill: def $ax killed $ax killed $eax
	popq	%rbp
	retq
.Lfunc_end0:
	.size	ring_next, .Lfunc_end0-ring_next
                                        # -- End function
	.globl	rx_poll                         # -- Begin function rx_poll
	.p2align	4
	.type	rx_poll,@function
rx_poll:                                # @rx_poll
# %bb.0:
	pushq	%rbp
	movq	%rsp, %rbp
	subq	$64, %rsp
	movw	%cx, %ax
	movw	%si, %cx
	movq	%rdi, -8(%rbp)
	movw	%cx, -10(%rbp)
	movq	%rdx, -24(%rbp)
	movw	%ax, -26(%rbp)
	movq	%r8, -40(%rbp)
	movq	-24(%rbp), %rax
	movw	(%rax), %ax
	movw	%ax, -42(%rbp)
	movl	$0, -48(%rbp)
	movl	$0, -52(%rbp)
.LBB1_1:                                # =>This Inner Loop Header: Depth=1
	movl	-48(%rbp), %eax
	movzwl	-26(%rbp), %ecx
	cmpl	%ecx, %eax
	jae	.LBB1_5
# %bb.2:                                #   in Loop: Header=BB1_1 Depth=1
	movq	-8(%rbp), %rdi
	movzwl	-42(%rbp), %eax
                                        # kill: def $rax killed $eax
	shlq	$4, %rax
	addq	%rax, %rdi
	callq	read_status
	movl	%eax, -56(%rbp)
	movl	-56(%rbp), %eax
	andl	$1, %eax
	cmpl	$0, %eax
	jne	.LBB1_4
# %bb.3:
	jmp	.LBB1_5
.LBB1_4:                                #   in Loop: Header=BB1_1 Depth=1
	callq	load_load_barrier
	movq	-8(%rbp), %rax
	movzwl	-42(%rbp), %ecx
                                        # kill: def $rcx killed $ecx
	shlq	$4, %rcx
	addq	%rcx, %rax
	movl	12(%rax), %eax
	addl	-52(%rbp), %eax
	movl	%eax, -52(%rbp)
	movw	-42(%rbp), %ax
	movzwl	%ax, %edi
	movzwl	-10(%rbp), %esi
	callq	ring_next
	movw	%ax, -42(%rbp)
	movl	-48(%rbp), %eax
	addl	$1, %eax
	movl	%eax, -48(%rbp)
	jmp	.LBB1_1
.LBB1_5:
	movw	-42(%rbp), %cx
	movq	-24(%rbp), %rax
	movw	%cx, (%rax)
	movl	-52(%rbp), %ecx
	movq	-40(%rbp), %rax
	movl	%ecx, (%rax)
	movl	-48(%rbp), %eax
	addq	$64, %rsp
	popq	%rbp
	retq
.Lfunc_end1:
	.size	rx_poll, .Lfunc_end1-rx_poll
                                        # -- End function
	.p2align	4                               # -- Begin function read_status
	.type	read_status,@function
read_status:                            # @read_status
# %bb.0:
	pushq	%rbp
	movq	%rsp, %rbp
	movq	%rdi, -8(%rbp)
	movq	-8(%rbp), %rax
	movl	8(%rax), %eax
	popq	%rbp
	retq
.Lfunc_end2:
	.size	read_status, .Lfunc_end2-read_status
                                        # -- End function
	.p2align	4                               # -- Begin function load_load_barrier
	.type	load_load_barrier,@function
load_load_barrier:                      # @load_load_barrier
# %bb.0:
	pushq	%rbp
	movq	%rsp, %rbp
	#APP
	#NO_APP
	popq	%rbp
	retq
.Lfunc_end3:
	.size	load_load_barrier, .Lfunc_end3-load_load_barrier
                                        # -- End function
	.globl	tx_clean                        # -- Begin function tx_clean
	.p2align	4
	.type	tx_clean,@function
tx_clean:                               # @tx_clean
# %bb.0:
	pushq	%rbp
	movq	%rsp, %rbp
	subq	$48, %rsp
	movw	%r8w, %ax
                                        # kill: def $cx killed $cx killed $ecx
                                        # kill: def $si killed $si killed $esi
	movq	%rdi, -8(%rbp)
	movw	%si, -10(%rbp)
	movq	%rdx, -24(%rbp)
	movw	%cx, -26(%rbp)
	movw	%ax, -28(%rbp)
	movq	-24(%rbp), %rax
	movw	(%rax), %ax
	movw	%ax, -30(%rbp)
	movzwl	-10(%rbp), %eax
	subl	$1, %eax
                                        # kill: def $ax killed $ax killed $eax
	movw	%ax, -32(%rbp)
	movl	$0, -36(%rbp)
.LBB4_1:                                # =>This Inner Loop Header: Depth=1
	movzwl	-26(%rbp), %eax
	movzwl	-30(%rbp), %ecx
	subl	%ecx, %eax
	movzwl	-32(%rbp), %ecx
	andl	%ecx, %eax
                                        # kill: def $ax killed $ax killed $eax
	movw	%ax, -38(%rbp)
	movzwl	-38(%rbp), %eax
	movzwl	-28(%rbp), %ecx
	cmpl	%ecx, %eax
	jge	.LBB4_3
# %bb.2:
	jmp	.LBB4_6
.LBB4_3:                                #   in Loop: Header=BB4_1 Depth=1
	movzwl	-30(%rbp), %eax
	movzwl	-28(%rbp), %ecx
	addl	%ecx, %eax
	subl	$1, %eax
	movzwl	-32(%rbp), %ecx
	andl	%ecx, %eax
                                        # kill: def $ax killed $ax killed $eax
	movw	%ax, -40(%rbp)
	movq	-8(%rbp), %rdi
	movzwl	-40(%rbp), %eax
                                        # kill: def $rax killed $eax
	shlq	$4, %rax
	addq	%rax, %rdi
	callq	read_status
	movl	%eax, -44(%rbp)
	movl	-44(%rbp), %eax
	andl	$1, %eax
	cmpl	$0, %eax
	jne	.LBB4_5
# %bb.4:
	jmp	.LBB4_6
.LBB4_5:                                #   in Loop: Header=BB4_1 Depth=1
	movzwl	-30(%rbp), %eax
	movzwl	-28(%rbp), %ecx
	addl	%ecx, %eax
	movzwl	-32(%rbp), %ecx
	andl	%ecx, %eax
                                        # kill: def $ax killed $ax killed $eax
	movw	%ax, -30(%rbp)
	movzwl	-28(%rbp), %eax
	addl	-36(%rbp), %eax
	movl	%eax, -36(%rbp)
	jmp	.LBB4_1
.LBB4_6:
	movw	-30(%rbp), %cx
	movq	-24(%rbp), %rax
	movw	%cx, (%rax)
	movl	-36(%rbp), %eax
	addq	$48, %rsp
	popq	%rbp
	retq
.Lfunc_end4:
	.size	tx_clean, .Lfunc_end4-tx_clean
                                        # -- End function
	.ident	"clang version 20.1.8"
	.section	".note.GNU-stack","",@progbits
	.addrsig
	.addrsig_sym ring_next
	.addrsig_sym read_status
	.addrsig_sym load_load_barrier
