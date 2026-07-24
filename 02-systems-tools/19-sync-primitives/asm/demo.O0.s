	.file	"demo.c"
	.text
	.globl	demo_mutex_trylock              # -- Begin function demo_mutex_trylock
	.p2align	4
	.type	demo_mutex_trylock,@function
demo_mutex_trylock:                     # @demo_mutex_trylock
# %bb.0:
	pushq	%rbp
	movq	%rsp, %rbp
	movq	%rdi, -8(%rbp)
	movl	$0, -12(%rbp)
	movq	-8(%rbp), %rcx
	movl	$1, -16(%rbp)
	movl	-12(%rbp), %eax
	movl	-16(%rbp), %edx
	lock		cmpxchgl	%edx, (%rcx)
	movl	%eax, %ecx
	sete	%al
	movb	%al, -25(%rbp)                  # 1-byte Spill
	movl	%ecx, -24(%rbp)                 # 4-byte Spill
	testb	$1, %al
	jne	.LBB0_2
# %bb.1:
	movl	-24(%rbp), %eax                 # 4-byte Reload
	movl	%eax, -12(%rbp)
.LBB0_2:
	movb	-25(%rbp), %al                  # 1-byte Reload
	andb	$1, %al
	movb	%al, -17(%rbp)
	movb	-17(%rbp), %al
	andb	$1, %al
	movzbl	%al, %eax
	popq	%rbp
	retq
.Lfunc_end0:
	.size	demo_mutex_trylock, .Lfunc_end0-demo_mutex_trylock
                                        # -- End function
	.globl	demo_mutex_lock_fast            # -- Begin function demo_mutex_lock_fast
	.p2align	4
	.type	demo_mutex_lock_fast,@function
demo_mutex_lock_fast:                   # @demo_mutex_lock_fast
# %bb.0:
	pushq	%rbp
	movq	%rsp, %rbp
	movq	%rdi, -16(%rbp)
	movl	$0, -20(%rbp)
	movq	-16(%rbp), %rcx
	movl	$1, -24(%rbp)
	movl	-20(%rbp), %eax
	movl	-24(%rbp), %edx
	lock		cmpxchgl	%edx, (%rcx)
	movl	%eax, %ecx
	sete	%al
	movb	%al, -33(%rbp)                  # 1-byte Spill
	movl	%ecx, -32(%rbp)                 # 4-byte Spill
	testb	$1, %al
	jne	.LBB1_2
# %bb.1:
	movl	-32(%rbp), %eax                 # 4-byte Reload
	movl	%eax, -20(%rbp)
.LBB1_2:
	movb	-33(%rbp), %al                  # 1-byte Reload
	andb	$1, %al
	movb	%al, -25(%rbp)
	testb	$1, -25(%rbp)
	je	.LBB1_4
# %bb.3:
	movl	$0, -4(%rbp)
	jmp	.LBB1_5
.LBB1_4:
	movl	-20(%rbp), %eax
	movl	%eax, -4(%rbp)
.LBB1_5:
	movl	-4(%rbp), %eax
	popq	%rbp
	retq
.Lfunc_end1:
	.size	demo_mutex_lock_fast, .Lfunc_end1-demo_mutex_lock_fast
                                        # -- End function
	.globl	demo_mutex_unlock_fast          # -- Begin function demo_mutex_unlock_fast
	.p2align	4
	.type	demo_mutex_unlock_fast,@function
demo_mutex_unlock_fast:                 # @demo_mutex_unlock_fast
# %bb.0:
	pushq	%rbp
	movq	%rsp, %rbp
	movq	%rdi, -16(%rbp)
	movq	-16(%rbp), %rcx
	movl	$1, -20(%rbp)
	movl	-20(%rbp), %eax
	negl	%eax
	lock		xaddl	%eax, (%rcx)
	movl	%eax, -24(%rbp)
	cmpl	$1, -24(%rbp)
	je	.LBB2_2
# %bb.1:
	movq	-16(%rbp), %rax
	movl	$0, -28(%rbp)
	movl	-28(%rbp), %ecx
	movl	%ecx, (%rax)
	movl	$1, -4(%rbp)
	jmp	.LBB2_3
.LBB2_2:
	movl	$0, -4(%rbp)
.LBB2_3:
	movl	-4(%rbp), %eax
	popq	%rbp
	retq
.Lfunc_end2:
	.size	demo_mutex_unlock_fast, .Lfunc_end2-demo_mutex_unlock_fast
                                        # -- End function
	.globl	demo_spin_lock                  # -- Begin function demo_spin_lock
	.p2align	4
	.type	demo_spin_lock,@function
demo_spin_lock:                         # @demo_spin_lock
# %bb.0:
	pushq	%rbp
	movq	%rsp, %rbp
	subq	$32, %rsp
	movq	%rdi, -8(%rbp)
.LBB3_1:                                # =>This Loop Header: Depth=1
                                        #     Child Loop BB3_4 Depth 2
	movq	-8(%rbp), %rcx
	movl	$1, -12(%rbp)
	movl	-12(%rbp), %eax
	xchgl	%eax, (%rcx)
	movl	%eax, -16(%rbp)
	cmpl	$0, -16(%rbp)
	jne	.LBB3_3
# %bb.2:
	addq	$32, %rsp
	popq	%rbp
	retq
.LBB3_3:                                #   in Loop: Header=BB3_1 Depth=1
	jmp	.LBB3_4
.LBB3_4:                                #   Parent Loop BB3_1 Depth=1
                                        # =>  This Inner Loop Header: Depth=2
	movq	-8(%rbp), %rax
	movl	(%rax), %eax
	movl	%eax, -20(%rbp)
	cmpl	$0, -20(%rbp)
	je	.LBB3_6
# %bb.5:                                #   in Loop: Header=BB3_4 Depth=2
	callq	cpu_relax
	jmp	.LBB3_4
.LBB3_6:                                #   in Loop: Header=BB3_1 Depth=1
	jmp	.LBB3_1
.Lfunc_end3:
	.size	demo_spin_lock, .Lfunc_end3-demo_spin_lock
                                        # -- End function
	.p2align	4                               # -- Begin function cpu_relax
	.type	cpu_relax,@function
cpu_relax:                              # @cpu_relax
# %bb.0:
	pushq	%rbp
	movq	%rsp, %rbp
	pause
	popq	%rbp
	retq
.Lfunc_end4:
	.size	cpu_relax, .Lfunc_end4-cpu_relax
                                        # -- End function
	.globl	demo_spin_unlock                # -- Begin function demo_spin_unlock
	.p2align	4
	.type	demo_spin_unlock,@function
demo_spin_unlock:                       # @demo_spin_unlock
# %bb.0:
	pushq	%rbp
	movq	%rsp, %rbp
	movq	%rdi, -8(%rbp)
	movq	-8(%rbp), %rax
	movl	$0, -12(%rbp)
	movl	-12(%rbp), %ecx
	movl	%ecx, (%rax)
	popq	%rbp
	retq
.Lfunc_end5:
	.size	demo_spin_unlock, .Lfunc_end5-demo_spin_unlock
                                        # -- End function
	.ident	"clang version 20.1.8"
	.section	".note.GNU-stack","",@progbits
	.addrsig
	.addrsig_sym cpu_relax
