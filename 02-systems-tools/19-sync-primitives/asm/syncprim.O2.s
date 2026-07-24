	.file	"syncprim.c"
	.text
	.globl	sp_spin_init                    # -- Begin function sp_spin_init
	.p2align	4
	.type	sp_spin_init,@function
sp_spin_init:                           # @sp_spin_init
# %bb.0:
	movl	$0, (%rdi)
	retq
.Lfunc_end0:
	.size	sp_spin_init, .Lfunc_end0-sp_spin_init
                                        # -- End function
	.globl	sp_spin_trylock                 # -- Begin function sp_spin_trylock
	.p2align	4
	.type	sp_spin_trylock,@function
sp_spin_trylock:                        # @sp_spin_trylock
# %bb.0:
	movl	$1, %ecx
	xchgl	%ecx, (%rdi)
	xorl	%eax, %eax
	testl	%ecx, %ecx
	sete	%al
	retq
.Lfunc_end1:
	.size	sp_spin_trylock, .Lfunc_end1-sp_spin_trylock
                                        # -- End function
	.globl	sp_spin_lock                    # -- Begin function sp_spin_lock
	.p2align	4
	.type	sp_spin_lock,@function
sp_spin_lock:                           # @sp_spin_lock
# %bb.0:
	.p2align	4
.LBB2_1:                                # =>This Loop Header: Depth=1
                                        #     Child Loop BB2_2 Depth 2
	movl	$1, %eax
	xchgl	%eax, (%rdi)
	testl	%eax, %eax
	jne	.LBB2_2
	jmp	.LBB2_4
	.p2align	4
.LBB2_3:                                #   in Loop: Header=BB2_2 Depth=2
	pause
.LBB2_2:                                #   Parent Loop BB2_1 Depth=1
                                        # =>  This Inner Loop Header: Depth=2
	movl	(%rdi), %eax
	testl	%eax, %eax
	jne	.LBB2_3
	jmp	.LBB2_1
.LBB2_4:
	retq
.Lfunc_end2:
	.size	sp_spin_lock, .Lfunc_end2-sp_spin_lock
                                        # -- End function
	.globl	sp_spin_unlock                  # -- Begin function sp_spin_unlock
	.p2align	4
	.type	sp_spin_unlock,@function
sp_spin_unlock:                         # @sp_spin_unlock
# %bb.0:
	movl	$0, (%rdi)
	retq
.Lfunc_end3:
	.size	sp_spin_unlock, .Lfunc_end3-sp_spin_unlock
                                        # -- End function
	.globl	sp_mutex_init                   # -- Begin function sp_mutex_init
	.p2align	4
	.type	sp_mutex_init,@function
sp_mutex_init:                          # @sp_mutex_init
# %bb.0:
	movl	$0, (%rdi)
	retq
.Lfunc_end4:
	.size	sp_mutex_init, .Lfunc_end4-sp_mutex_init
                                        # -- End function
	.globl	sp_mutex_trylock                # -- Begin function sp_mutex_trylock
	.p2align	4
	.type	sp_mutex_trylock,@function
sp_mutex_trylock:                       # @sp_mutex_trylock
# %bb.0:
	movl	$1, %edx
	xorl	%ecx, %ecx
	xorl	%eax, %eax
	lock		cmpxchgl	%edx, (%rdi)
	sete	%cl
	movl	%ecx, %eax
	retq
.Lfunc_end5:
	.size	sp_mutex_trylock, .Lfunc_end5-sp_mutex_trylock
                                        # -- End function
	.globl	sp_mutex_lock                   # -- Begin function sp_mutex_lock
	.p2align	4
	.type	sp_mutex_lock,@function
sp_mutex_lock:                          # @sp_mutex_lock
# %bb.0:
	movl	$1, %ecx
	xorl	%eax, %eax
	lock		cmpxchgl	%ecx, (%rdi)
	je	.LBB6_5
# %bb.1:
	cmpl	$2, %eax
	je	.LBB6_3
# %bb.2:
	movl	$2, %eax
	xchgl	%eax, (%rdi)
	testl	%eax, %eax
	je	.LBB6_5
.LBB6_3:
	movl	$128, %esi
	movl	$2, %edx
	.p2align	4
.LBB6_4:                                # =>This Inner Loop Header: Depth=1
	movl	$202, %eax
	xorl	%r10d, %r10d
	xorl	%r8d, %r8d
	xorl	%r9d, %r9d
	#APP
	syscall
	#NO_APP
	movl	$2, %eax
	xchgl	%eax, (%rdi)
	testl	%eax, %eax
	jne	.LBB6_4
.LBB6_5:
	retq
.Lfunc_end6:
	.size	sp_mutex_lock, .Lfunc_end6-sp_mutex_lock
                                        # -- End function
	.globl	sp_mutex_unlock                 # -- Begin function sp_mutex_unlock
	.p2align	4
	.type	sp_mutex_unlock,@function
sp_mutex_unlock:                        # @sp_mutex_unlock
# %bb.0:
	lock		decl	(%rdi)
	je	.LBB7_2
# %bb.1:
	movl	$0, (%rdi)
	movl	$202, %eax
	movl	$129, %esi
	movl	$1, %edx
	xorl	%r10d, %r10d
	xorl	%r8d, %r8d
	xorl	%r9d, %r9d
	#APP
	syscall
	#NO_APP
.LBB7_2:
	retq
.Lfunc_end7:
	.size	sp_mutex_unlock, .Lfunc_end7-sp_mutex_unlock
                                        # -- End function
	.globl	sp_cond_init                    # -- Begin function sp_cond_init
	.p2align	4
	.type	sp_cond_init,@function
sp_cond_init:                           # @sp_cond_init
# %bb.0:
	movl	$0, (%rdi)
	retq
.Lfunc_end8:
	.size	sp_cond_init, .Lfunc_end8-sp_cond_init
                                        # -- End function
	.globl	sp_cond_wait                    # -- Begin function sp_cond_wait
	.p2align	4
	.type	sp_cond_wait,@function
sp_cond_wait:                           # @sp_cond_wait
# %bb.0:
	pushq	%r15
	pushq	%r14
	pushq	%rbx
	movq	%rsi, %rbx
	movl	(%rdi), %r14d
	lock		decl	(%rsi)
	je	.LBB9_2
# %bb.1:
	movl	$0, (%rbx)
	movl	$202, %eax
	movl	$129, %esi
	movl	$1, %edx
	movq	%rdi, %r15
	movq	%rbx, %rdi
	xorl	%r10d, %r10d
	xorl	%r8d, %r8d
	xorl	%r9d, %r9d
	#APP
	syscall
	#NO_APP
	movq	%r15, %rdi
.LBB9_2:
	movl	$202, %eax
	movl	$128, %esi
	movq	%r14, %rdx
	xorl	%r10d, %r10d
	xorl	%r8d, %r8d
	xorl	%r9d, %r9d
	#APP
	syscall
	#NO_APP
	movl	$1, %ecx
	xorl	%eax, %eax
	lock		cmpxchgl	%ecx, (%rbx)
	je	.LBB9_7
# %bb.3:
	cmpl	$2, %eax
	je	.LBB9_5
# %bb.4:
	movl	$2, %eax
	xchgl	%eax, (%rbx)
	testl	%eax, %eax
	je	.LBB9_7
.LBB9_5:
	movl	$2, %edx
	.p2align	4
.LBB9_6:                                # =>This Inner Loop Header: Depth=1
	movl	$202, %eax
	movq	%rbx, %rdi
	xorl	%r10d, %r10d
	xorl	%r8d, %r8d
	xorl	%r9d, %r9d
	#APP
	syscall
	#NO_APP
	movl	$2, %eax
	xchgl	%eax, (%rbx)
	testl	%eax, %eax
	jne	.LBB9_6
.LBB9_7:
	popq	%rbx
	popq	%r14
	popq	%r15
	retq
.Lfunc_end9:
	.size	sp_cond_wait, .Lfunc_end9-sp_cond_wait
                                        # -- End function
	.globl	sp_cond_signal                  # -- Begin function sp_cond_signal
	.p2align	4
	.type	sp_cond_signal,@function
sp_cond_signal:                         # @sp_cond_signal
# %bb.0:
	lock		incl	(%rdi)
	movl	$202, %eax
	movl	$129, %esi
	movl	$1, %edx
	xorl	%r10d, %r10d
	xorl	%r8d, %r8d
	xorl	%r9d, %r9d
	#APP
	syscall
	#NO_APP
	retq
.Lfunc_end10:
	.size	sp_cond_signal, .Lfunc_end10-sp_cond_signal
                                        # -- End function
	.globl	sp_cond_broadcast               # -- Begin function sp_cond_broadcast
	.p2align	4
	.type	sp_cond_broadcast,@function
sp_cond_broadcast:                      # @sp_cond_broadcast
# %bb.0:
	lock		incl	(%rdi)
	movl	$202, %eax
	movl	$129, %esi
	movl	$2147483647, %edx               # imm = 0x7FFFFFFF
	xorl	%r10d, %r10d
	xorl	%r8d, %r8d
	xorl	%r9d, %r9d
	#APP
	syscall
	#NO_APP
	retq
.Lfunc_end11:
	.size	sp_cond_broadcast, .Lfunc_end11-sp_cond_broadcast
                                        # -- End function
	.globl	sp_sem_init                     # -- Begin function sp_sem_init
	.p2align	4
	.type	sp_sem_init,@function
sp_sem_init:                            # @sp_sem_init
# %bb.0:
	movl	%esi, (%rdi)
	retq
.Lfunc_end12:
	.size	sp_sem_init, .Lfunc_end12-sp_sem_init
                                        # -- End function
	.globl	sp_sem_trywait                  # -- Begin function sp_sem_trywait
	.p2align	4
	.type	sp_sem_trywait,@function
sp_sem_trywait:                         # @sp_sem_trywait
# %bb.0:
	movl	(%rdi), %eax
	.p2align	4
.LBB13_1:                               # =>This Inner Loop Header: Depth=1
	testl	%eax, %eax
	je	.LBB13_2
# %bb.3:                                #   in Loop: Header=BB13_1 Depth=1
	leal	-1(%rax), %ecx
                                        # kill: def $eax killed $eax killed $rax
	lock		cmpxchgl	%ecx, (%rdi)
                                        # kill: def $eax killed $eax def $rax
	jne	.LBB13_1
# %bb.4:
	movl	$1, %eax
	retq
.LBB13_2:
	xorl	%eax, %eax
	retq
.Lfunc_end13:
	.size	sp_sem_trywait, .Lfunc_end13-sp_sem_trywait
                                        # -- End function
	.globl	sp_sem_wait                     # -- Begin function sp_sem_wait
	.p2align	4
	.type	sp_sem_wait,@function
sp_sem_wait:                            # @sp_sem_wait
# %bb.0:
	movl	$128, %esi
	jmp	.LBB14_1
	.p2align	4
.LBB14_5:                               #   in Loop: Header=BB14_1 Depth=1
	movl	$202, %eax
	xorl	%edx, %edx
	xorl	%r10d, %r10d
	xorl	%r8d, %r8d
	xorl	%r9d, %r9d
	#APP
	syscall
	#NO_APP
.LBB14_1:                               # =>This Loop Header: Depth=1
                                        #     Child Loop BB14_2 Depth 2
	movl	(%rdi), %eax
	.p2align	4
.LBB14_2:                               #   Parent Loop BB14_1 Depth=1
                                        # =>  This Inner Loop Header: Depth=2
	testl	%eax, %eax
	je	.LBB14_5
# %bb.3:                                #   in Loop: Header=BB14_2 Depth=2
	leal	-1(%rax), %ecx
                                        # kill: def $eax killed $eax killed $rax
	lock		cmpxchgl	%ecx, (%rdi)
                                        # kill: def $eax killed $eax def $rax
	jne	.LBB14_2
# %bb.4:
	retq
.Lfunc_end14:
	.size	sp_sem_wait, .Lfunc_end14-sp_sem_wait
                                        # -- End function
	.globl	sp_sem_post                     # -- Begin function sp_sem_post
	.p2align	4
	.type	sp_sem_post,@function
sp_sem_post:                            # @sp_sem_post
# %bb.0:
	lock		incl	(%rdi)
	movl	$202, %eax
	movl	$129, %esi
	movl	$1, %edx
	xorl	%r10d, %r10d
	xorl	%r8d, %r8d
	xorl	%r9d, %r9d
	#APP
	syscall
	#NO_APP
	retq
.Lfunc_end15:
	.size	sp_sem_post, .Lfunc_end15-sp_sem_post
                                        # -- End function
	.globl	sp_rwlock_init                  # -- Begin function sp_rwlock_init
	.p2align	4
	.type	sp_rwlock_init,@function
sp_rwlock_init:                         # @sp_rwlock_init
# %bb.0:
	movl	$0, (%rdi)
	retq
.Lfunc_end16:
	.size	sp_rwlock_init, .Lfunc_end16-sp_rwlock_init
                                        # -- End function
	.globl	sp_rwlock_tryrdlock             # -- Begin function sp_rwlock_tryrdlock
	.p2align	4
	.type	sp_rwlock_tryrdlock,@function
sp_rwlock_tryrdlock:                    # @sp_rwlock_tryrdlock
# %bb.0:
	movl	(%rdi), %eax
	.p2align	4
.LBB17_1:                               # =>This Inner Loop Header: Depth=1
	testl	%eax, %eax
	js	.LBB17_2
# %bb.3:                                #   in Loop: Header=BB17_1 Depth=1
	leal	1(%rax), %ecx
                                        # kill: def $eax killed $eax killed $rax
	lock		cmpxchgl	%ecx, (%rdi)
                                        # kill: def $eax killed $eax def $rax
	jne	.LBB17_1
# %bb.4:
	movl	$1, %eax
	retq
.LBB17_2:
	xorl	%eax, %eax
	retq
.Lfunc_end17:
	.size	sp_rwlock_tryrdlock, .Lfunc_end17-sp_rwlock_tryrdlock
                                        # -- End function
	.globl	sp_rwlock_rdlock                # -- Begin function sp_rwlock_rdlock
	.p2align	4
	.type	sp_rwlock_rdlock,@function
sp_rwlock_rdlock:                       # @sp_rwlock_rdlock
# %bb.0:
	movl	$128, %esi
	jmp	.LBB18_1
	.p2align	4
.LBB18_2:                               #   in Loop: Header=BB18_1 Depth=1
	movl	$202, %eax
	xorl	%r10d, %r10d
	xorl	%r8d, %r8d
	xorl	%r9d, %r9d
	#APP
	syscall
	#NO_APP
.LBB18_1:                               # =>This Inner Loop Header: Depth=1
	movl	(%rdi), %edx
	testl	%edx, %edx
	js	.LBB18_2
# %bb.3:                                #   in Loop: Header=BB18_1 Depth=1
	leal	1(%rdx), %ecx
	movl	%edx, %eax
	lock		cmpxchgl	%ecx, (%rdi)
	jne	.LBB18_1
# %bb.4:
	retq
.Lfunc_end18:
	.size	sp_rwlock_rdlock, .Lfunc_end18-sp_rwlock_rdlock
                                        # -- End function
	.globl	sp_rwlock_rdunlock              # -- Begin function sp_rwlock_rdunlock
	.p2align	4
	.type	sp_rwlock_rdunlock,@function
sp_rwlock_rdunlock:                     # @sp_rwlock_rdunlock
# %bb.0:
	lock		decl	(%rdi)
	jne	.LBB19_2
# %bb.1:
	movl	$202, %eax
	movl	$129, %esi
	movl	$1, %edx
	xorl	%r10d, %r10d
	xorl	%r8d, %r8d
	xorl	%r9d, %r9d
	#APP
	syscall
	#NO_APP
.LBB19_2:
	retq
.Lfunc_end19:
	.size	sp_rwlock_rdunlock, .Lfunc_end19-sp_rwlock_rdunlock
                                        # -- End function
	.globl	sp_rwlock_trywrlock             # -- Begin function sp_rwlock_trywrlock
	.p2align	4
	.type	sp_rwlock_trywrlock,@function
sp_rwlock_trywrlock:                    # @sp_rwlock_trywrlock
# %bb.0:
	movl	$-2147483648, %edx              # imm = 0x80000000
	xorl	%ecx, %ecx
	xorl	%eax, %eax
	lock		cmpxchgl	%edx, (%rdi)
	sete	%cl
	movl	%ecx, %eax
	retq
.Lfunc_end20:
	.size	sp_rwlock_trywrlock, .Lfunc_end20-sp_rwlock_trywrlock
                                        # -- End function
	.globl	sp_rwlock_wrlock                # -- Begin function sp_rwlock_wrlock
	.p2align	4
	.type	sp_rwlock_wrlock,@function
sp_rwlock_wrlock:                       # @sp_rwlock_wrlock
# %bb.0:
	pushq	%rbx
	movl	$-2147483648, %ebx              # imm = 0x80000000
	movl	$128, %esi
	jmp	.LBB21_1
	.p2align	4
.LBB21_4:                               #   in Loop: Header=BB21_1 Depth=1
	movl	$202, %eax
	xorl	%r10d, %r10d
	xorl	%r8d, %r8d
	xorl	%r9d, %r9d
	#APP
	syscall
	#NO_APP
.LBB21_1:                               # =>This Inner Loop Header: Depth=1
	movl	(%rdi), %edx
	testl	%edx, %edx
	jne	.LBB21_4
# %bb.2:                                #   in Loop: Header=BB21_1 Depth=1
	xorl	%eax, %eax
	lock		cmpxchgl	%ebx, (%rdi)
	jne	.LBB21_1
# %bb.3:
	popq	%rbx
	retq
.Lfunc_end21:
	.size	sp_rwlock_wrlock, .Lfunc_end21-sp_rwlock_wrlock
                                        # -- End function
	.globl	sp_rwlock_wrunlock              # -- Begin function sp_rwlock_wrunlock
	.p2align	4
	.type	sp_rwlock_wrunlock,@function
sp_rwlock_wrunlock:                     # @sp_rwlock_wrunlock
# %bb.0:
	movl	$0, (%rdi)
	movl	$202, %eax
	movl	$129, %esi
	movl	$2147483647, %edx               # imm = 0x7FFFFFFF
	xorl	%r10d, %r10d
	xorl	%r8d, %r8d
	xorl	%r9d, %r9d
	#APP
	syscall
	#NO_APP
	retq
.Lfunc_end22:
	.size	sp_rwlock_wrunlock, .Lfunc_end22-sp_rwlock_wrunlock
                                        # -- End function
	.ident	"clang version 20.1.8"
	.section	".note.GNU-stack","",@progbits
	.addrsig
