	.file	"demo.c"
	.text
	.globl	demo_mutex_trylock              # -- Begin function demo_mutex_trylock
	.p2align	4
	.type	demo_mutex_trylock,@function
demo_mutex_trylock:                     # @demo_mutex_trylock
# %bb.0:
	pushq	%rbp
	movq	%rsp, %rbp
	movl	$1, %edx
	xorl	%ecx, %ecx
	xorl	%eax, %eax
	lock		cmpxchgl	%edx, (%rdi)
	sete	%cl
	movl	%ecx, %eax
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
	movl	$1, %ecx
	xorl	%eax, %eax
	lock		cmpxchgl	%ecx, (%rdi)
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
	xorl	%eax, %eax
	lock		decl	(%rdi)
	je	.LBB2_2
# %bb.1:
	movl	$0, (%rdi)
	movl	$1, %eax
.LBB2_2:
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
	movl	$1, %eax
	xchgl	%eax, (%rdi)
	testl	%eax, %eax
	je	.LBB3_6
# %bb.1:
	pushq	%rbp
	movq	%rsp, %rbp
	jmp	.LBB3_3
	.p2align	4
.LBB3_4:                                #   in Loop: Header=BB3_3 Depth=1
	pause
.LBB3_3:                                # =>This Inner Loop Header: Depth=1
	movl	(%rdi), %eax
	testl	%eax, %eax
	jne	.LBB3_4
# %bb.2:                                #   in Loop: Header=BB3_3 Depth=1
	movl	$1, %eax
	xchgl	%eax, (%rdi)
	testl	%eax, %eax
	jne	.LBB3_3
# %bb.5:
	popq	%rbp
.LBB3_6:
	retq
.Lfunc_end3:
	.size	demo_spin_lock, .Lfunc_end3-demo_spin_lock
                                        # -- End function
	.globl	demo_spin_unlock                # -- Begin function demo_spin_unlock
	.p2align	4
	.type	demo_spin_unlock,@function
demo_spin_unlock:                       # @demo_spin_unlock
# %bb.0:
	pushq	%rbp
	movq	%rsp, %rbp
	movl	$0, (%rdi)
	popq	%rbp
	retq
.Lfunc_end4:
	.size	demo_spin_unlock, .Lfunc_end4-demo_spin_unlock
                                        # -- End function
	.ident	"clang version 20.1.8"
	.section	".note.GNU-stack","",@progbits
	.addrsig
