	.file	"syncprim.c"
	.text
	.globl	sp_spin_init                    # -- Begin function sp_spin_init
	.p2align	4
	.type	sp_spin_init,@function
sp_spin_init:                           # @sp_spin_init
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
.Lfunc_end0:
	.size	sp_spin_init, .Lfunc_end0-sp_spin_init
                                        # -- End function
	.globl	sp_spin_trylock                 # -- Begin function sp_spin_trylock
	.p2align	4
	.type	sp_spin_trylock,@function
sp_spin_trylock:                        # @sp_spin_trylock
# %bb.0:
	pushq	%rbp
	movq	%rsp, %rbp
	movq	%rdi, -8(%rbp)
	movq	-8(%rbp), %rcx
	movl	$1, -12(%rbp)
	movl	-12(%rbp), %eax
	xchgl	%eax, (%rcx)
	movl	%eax, -16(%rbp)
	movl	-16(%rbp), %edx
	xorl	%eax, %eax
	movl	$1, %ecx
	cmpl	$0, %edx
	cmovel	%ecx, %eax
	popq	%rbp
	retq
.Lfunc_end1:
	.size	sp_spin_trylock, .Lfunc_end1-sp_spin_trylock
                                        # -- End function
	.globl	sp_spin_lock                    # -- Begin function sp_spin_lock
	.p2align	4
	.type	sp_spin_lock,@function
sp_spin_lock:                           # @sp_spin_lock
# %bb.0:
	pushq	%rbp
	movq	%rsp, %rbp
	subq	$32, %rsp
	movq	%rdi, -8(%rbp)
.LBB2_1:                                # =>This Loop Header: Depth=1
                                        #     Child Loop BB2_4 Depth 2
	movq	-8(%rbp), %rcx
	movl	$1, -12(%rbp)
	movl	-12(%rbp), %eax
	xchgl	%eax, (%rcx)
	movl	%eax, -16(%rbp)
	cmpl	$0, -16(%rbp)
	jne	.LBB2_3
# %bb.2:
	addq	$32, %rsp
	popq	%rbp
	retq
.LBB2_3:                                #   in Loop: Header=BB2_1 Depth=1
	jmp	.LBB2_4
.LBB2_4:                                #   Parent Loop BB2_1 Depth=1
                                        # =>  This Inner Loop Header: Depth=2
	movq	-8(%rbp), %rax
	movl	(%rax), %eax
	movl	%eax, -20(%rbp)
	cmpl	$0, -20(%rbp)
	je	.LBB2_6
# %bb.5:                                #   in Loop: Header=BB2_4 Depth=2
	callq	sp_cpu_relax
	jmp	.LBB2_4
.LBB2_6:                                #   in Loop: Header=BB2_1 Depth=1
	jmp	.LBB2_1
.Lfunc_end2:
	.size	sp_spin_lock, .Lfunc_end2-sp_spin_lock
                                        # -- End function
	.p2align	4                               # -- Begin function sp_cpu_relax
	.type	sp_cpu_relax,@function
sp_cpu_relax:                           # @sp_cpu_relax
# %bb.0:
	pushq	%rbp
	movq	%rsp, %rbp
	pause
	popq	%rbp
	retq
.Lfunc_end3:
	.size	sp_cpu_relax, .Lfunc_end3-sp_cpu_relax
                                        # -- End function
	.globl	sp_spin_unlock                  # -- Begin function sp_spin_unlock
	.p2align	4
	.type	sp_spin_unlock,@function
sp_spin_unlock:                         # @sp_spin_unlock
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
.Lfunc_end4:
	.size	sp_spin_unlock, .Lfunc_end4-sp_spin_unlock
                                        # -- End function
	.globl	sp_mutex_init                   # -- Begin function sp_mutex_init
	.p2align	4
	.type	sp_mutex_init,@function
sp_mutex_init:                          # @sp_mutex_init
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
	.size	sp_mutex_init, .Lfunc_end5-sp_mutex_init
                                        # -- End function
	.globl	sp_mutex_trylock                # -- Begin function sp_mutex_trylock
	.p2align	4
	.type	sp_mutex_trylock,@function
sp_mutex_trylock:                       # @sp_mutex_trylock
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
	jne	.LBB6_2
# %bb.1:
	movl	-24(%rbp), %eax                 # 4-byte Reload
	movl	%eax, -12(%rbp)
.LBB6_2:
	movb	-25(%rbp), %al                  # 1-byte Reload
	andb	$1, %al
	movb	%al, -17(%rbp)
	movb	-17(%rbp), %dl
	xorl	%eax, %eax
	movl	$1, %ecx
	testb	$1, %dl
	cmovnel	%ecx, %eax
	popq	%rbp
	retq
.Lfunc_end6:
	.size	sp_mutex_trylock, .Lfunc_end6-sp_mutex_trylock
                                        # -- End function
	.globl	sp_mutex_lock                   # -- Begin function sp_mutex_lock
	.p2align	4
	.type	sp_mutex_lock,@function
sp_mutex_lock:                          # @sp_mutex_lock
# %bb.0:
	pushq	%rbp
	movq	%rsp, %rbp
	subq	$48, %rsp
	movq	%rdi, -8(%rbp)
	movl	$0, -12(%rbp)
	movq	-8(%rbp), %rcx
	movl	$1, -16(%rbp)
	movl	-12(%rbp), %eax
	movl	-16(%rbp), %edx
	lock		cmpxchgl	%edx, (%rcx)
	movl	%eax, %ecx
	sete	%al
	movb	%al, -41(%rbp)                  # 1-byte Spill
	movl	%ecx, -40(%rbp)                 # 4-byte Spill
	testb	$1, %al
	jne	.LBB7_2
# %bb.1:
	movl	-40(%rbp), %eax                 # 4-byte Reload
	movl	%eax, -12(%rbp)
.LBB7_2:
	movb	-41(%rbp), %al                  # 1-byte Reload
	andb	$1, %al
	movb	%al, -17(%rbp)
	testb	$1, -17(%rbp)
	je	.LBB7_4
# %bb.3:
	jmp	.LBB7_9
.LBB7_4:
	cmpl	$2, -12(%rbp)
	je	.LBB7_6
# %bb.5:
	movq	-8(%rbp), %rcx
	movl	$2, -24(%rbp)
	movl	-24(%rbp), %eax
	xchgl	%eax, (%rcx)
	movl	%eax, -28(%rbp)
	movl	-28(%rbp), %eax
	movl	%eax, -12(%rbp)
.LBB7_6:
	jmp	.LBB7_7
.LBB7_7:                                # =>This Inner Loop Header: Depth=1
	cmpl	$0, -12(%rbp)
	je	.LBB7_9
# %bb.8:                                #   in Loop: Header=BB7_7 Depth=1
	movq	-8(%rbp), %rdi
	movl	$2, %esi
	callq	sp_futex_wait
	movq	-8(%rbp), %rcx
	movl	$2, -32(%rbp)
	movl	-32(%rbp), %eax
	xchgl	%eax, (%rcx)
	movl	%eax, -36(%rbp)
	movl	-36(%rbp), %eax
	movl	%eax, -12(%rbp)
	jmp	.LBB7_7
.LBB7_9:
	addq	$48, %rsp
	popq	%rbp
	retq
.Lfunc_end7:
	.size	sp_mutex_lock, .Lfunc_end7-sp_mutex_lock
                                        # -- End function
	.p2align	4                               # -- Begin function sp_futex_wait
	.type	sp_futex_wait,@function
sp_futex_wait:                          # @sp_futex_wait
# %bb.0:
	pushq	%rbp
	movq	%rsp, %rbp
	subq	$16, %rsp
	movq	%rdi, -8(%rbp)
	movl	%esi, -12(%rbp)
	movq	-8(%rbp), %rdi
	movl	-12(%rbp), %edx
	movl	$128, %esi
	callq	sp_futex
	addq	$16, %rsp
	popq	%rbp
	retq
.Lfunc_end8:
	.size	sp_futex_wait, .Lfunc_end8-sp_futex_wait
                                        # -- End function
	.globl	sp_mutex_unlock                 # -- Begin function sp_mutex_unlock
	.p2align	4
	.type	sp_mutex_unlock,@function
sp_mutex_unlock:                        # @sp_mutex_unlock
# %bb.0:
	pushq	%rbp
	movq	%rsp, %rbp
	subq	$32, %rsp
	movq	%rdi, -8(%rbp)
	movq	-8(%rbp), %rcx
	movl	$1, -12(%rbp)
	movl	-12(%rbp), %eax
	negl	%eax
	lock		xaddl	%eax, (%rcx)
	movl	%eax, -16(%rbp)
	cmpl	$1, -16(%rbp)
	je	.LBB9_2
# %bb.1:
	movq	-8(%rbp), %rax
	movl	$0, -20(%rbp)
	movl	-20(%rbp), %ecx
	movl	%ecx, (%rax)
	movq	-8(%rbp), %rdi
	movl	$1, %esi
	callq	sp_futex_wake
.LBB9_2:
	addq	$32, %rsp
	popq	%rbp
	retq
.Lfunc_end9:
	.size	sp_mutex_unlock, .Lfunc_end9-sp_mutex_unlock
                                        # -- End function
	.p2align	4                               # -- Begin function sp_futex_wake
	.type	sp_futex_wake,@function
sp_futex_wake:                          # @sp_futex_wake
# %bb.0:
	pushq	%rbp
	movq	%rsp, %rbp
	subq	$16, %rsp
	movq	%rdi, -8(%rbp)
	movl	%esi, -12(%rbp)
	movq	-8(%rbp), %rdi
	movl	-12(%rbp), %edx
	movl	$129, %esi
	callq	sp_futex
	addq	$16, %rsp
	popq	%rbp
	retq
.Lfunc_end10:
	.size	sp_futex_wake, .Lfunc_end10-sp_futex_wake
                                        # -- End function
	.globl	sp_cond_init                    # -- Begin function sp_cond_init
	.p2align	4
	.type	sp_cond_init,@function
sp_cond_init:                           # @sp_cond_init
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
.Lfunc_end11:
	.size	sp_cond_init, .Lfunc_end11-sp_cond_init
                                        # -- End function
	.globl	sp_cond_wait                    # -- Begin function sp_cond_wait
	.p2align	4
	.type	sp_cond_wait,@function
sp_cond_wait:                           # @sp_cond_wait
# %bb.0:
	pushq	%rbp
	movq	%rsp, %rbp
	subq	$32, %rsp
	movq	%rdi, -8(%rbp)
	movq	%rsi, -16(%rbp)
	movq	-8(%rbp), %rax
	movl	(%rax), %eax
	movl	%eax, -24(%rbp)
	movl	-24(%rbp), %eax
	movl	%eax, -20(%rbp)
	movq	-16(%rbp), %rdi
	callq	sp_mutex_unlock
	movq	-8(%rbp), %rdi
	movl	-20(%rbp), %esi
	callq	sp_futex_wait
	movq	-16(%rbp), %rdi
	callq	sp_mutex_lock
	addq	$32, %rsp
	popq	%rbp
	retq
.Lfunc_end12:
	.size	sp_cond_wait, .Lfunc_end12-sp_cond_wait
                                        # -- End function
	.globl	sp_cond_signal                  # -- Begin function sp_cond_signal
	.p2align	4
	.type	sp_cond_signal,@function
sp_cond_signal:                         # @sp_cond_signal
# %bb.0:
	pushq	%rbp
	movq	%rsp, %rbp
	subq	$16, %rsp
	movq	%rdi, -8(%rbp)
	movq	-8(%rbp), %rcx
	movl	$1, -12(%rbp)
	movl	-12(%rbp), %eax
	lock		xaddl	%eax, (%rcx)
	movl	%eax, -16(%rbp)
	movq	-8(%rbp), %rdi
	movl	$1, %esi
	callq	sp_futex_wake
	addq	$16, %rsp
	popq	%rbp
	retq
.Lfunc_end13:
	.size	sp_cond_signal, .Lfunc_end13-sp_cond_signal
                                        # -- End function
	.globl	sp_cond_broadcast               # -- Begin function sp_cond_broadcast
	.p2align	4
	.type	sp_cond_broadcast,@function
sp_cond_broadcast:                      # @sp_cond_broadcast
# %bb.0:
	pushq	%rbp
	movq	%rsp, %rbp
	subq	$16, %rsp
	movq	%rdi, -8(%rbp)
	movq	-8(%rbp), %rcx
	movl	$1, -12(%rbp)
	movl	-12(%rbp), %eax
	lock		xaddl	%eax, (%rcx)
	movl	%eax, -16(%rbp)
	movq	-8(%rbp), %rdi
	movl	$2147483647, %esi               # imm = 0x7FFFFFFF
	callq	sp_futex_wake
	addq	$16, %rsp
	popq	%rbp
	retq
.Lfunc_end14:
	.size	sp_cond_broadcast, .Lfunc_end14-sp_cond_broadcast
                                        # -- End function
	.globl	sp_sem_init                     # -- Begin function sp_sem_init
	.p2align	4
	.type	sp_sem_init,@function
sp_sem_init:                            # @sp_sem_init
# %bb.0:
	pushq	%rbp
	movq	%rsp, %rbp
	movq	%rdi, -8(%rbp)
	movl	%esi, -12(%rbp)
	movq	-8(%rbp), %rax
	movl	-12(%rbp), %ecx
	movl	%ecx, -16(%rbp)
	movl	-16(%rbp), %ecx
	movl	%ecx, (%rax)
	popq	%rbp
	retq
.Lfunc_end15:
	.size	sp_sem_init, .Lfunc_end15-sp_sem_init
                                        # -- End function
	.globl	sp_sem_trywait                  # -- Begin function sp_sem_trywait
	.p2align	4
	.type	sp_sem_trywait,@function
sp_sem_trywait:                         # @sp_sem_trywait
# %bb.0:
	pushq	%rbp
	movq	%rsp, %rbp
	movq	%rdi, -16(%rbp)
	movq	-16(%rbp), %rax
	movl	(%rax), %eax
	movl	%eax, -24(%rbp)
	movl	-24(%rbp), %eax
	movl	%eax, -20(%rbp)
.LBB16_1:                               # =>This Inner Loop Header: Depth=1
	cmpl	$0, -20(%rbp)
	jbe	.LBB16_7
# %bb.2:                                #   in Loop: Header=BB16_1 Depth=1
	movq	-16(%rbp), %rcx
	movl	-20(%rbp), %eax
	decl	%eax
	movl	%eax, -28(%rbp)
	movl	-20(%rbp), %eax
	movl	-28(%rbp), %edx
	lock		cmpxchgl	%edx, (%rcx)
	movl	%eax, %ecx
	sete	%al
	movb	%al, -37(%rbp)                  # 1-byte Spill
	movl	%ecx, -36(%rbp)                 # 4-byte Spill
	testb	$1, %al
	jne	.LBB16_4
# %bb.3:                                #   in Loop: Header=BB16_1 Depth=1
	movl	-36(%rbp), %eax                 # 4-byte Reload
	movl	%eax, -20(%rbp)
.LBB16_4:                               #   in Loop: Header=BB16_1 Depth=1
	movb	-37(%rbp), %al                  # 1-byte Reload
	andb	$1, %al
	movb	%al, -29(%rbp)
	testb	$1, -29(%rbp)
	je	.LBB16_6
# %bb.5:
	movl	$1, -4(%rbp)
	jmp	.LBB16_8
.LBB16_6:                               #   in Loop: Header=BB16_1 Depth=1
	jmp	.LBB16_1
.LBB16_7:
	movl	$0, -4(%rbp)
.LBB16_8:
	movl	-4(%rbp), %eax
	popq	%rbp
	retq
.Lfunc_end16:
	.size	sp_sem_trywait, .Lfunc_end16-sp_sem_trywait
                                        # -- End function
	.globl	sp_sem_wait                     # -- Begin function sp_sem_wait
	.p2align	4
	.type	sp_sem_wait,@function
sp_sem_wait:                            # @sp_sem_wait
# %bb.0:
	pushq	%rbp
	movq	%rsp, %rbp
	subq	$32, %rsp
	movq	%rdi, -8(%rbp)
.LBB17_1:                               # =>This Loop Header: Depth=1
                                        #     Child Loop BB17_2 Depth 2
	movq	-8(%rbp), %rax
	movl	(%rax), %eax
	movl	%eax, -16(%rbp)
	movl	-16(%rbp), %eax
	movl	%eax, -12(%rbp)
.LBB17_2:                               #   Parent Loop BB17_1 Depth=1
                                        # =>  This Inner Loop Header: Depth=2
	cmpl	$0, -12(%rbp)
	jbe	.LBB17_8
# %bb.3:                                #   in Loop: Header=BB17_2 Depth=2
	movq	-8(%rbp), %rcx
	movl	-12(%rbp), %eax
	decl	%eax
	movl	%eax, -20(%rbp)
	movl	-12(%rbp), %eax
	movl	-20(%rbp), %edx
	lock		cmpxchgl	%edx, (%rcx)
	movl	%eax, %ecx
	sete	%al
	movb	%al, -29(%rbp)                  # 1-byte Spill
	movl	%ecx, -28(%rbp)                 # 4-byte Spill
	testb	$1, %al
	jne	.LBB17_5
# %bb.4:                                #   in Loop: Header=BB17_2 Depth=2
	movl	-28(%rbp), %eax                 # 4-byte Reload
	movl	%eax, -12(%rbp)
.LBB17_5:                               #   in Loop: Header=BB17_2 Depth=2
	movb	-29(%rbp), %al                  # 1-byte Reload
	andb	$1, %al
	movb	%al, -21(%rbp)
	testb	$1, -21(%rbp)
	je	.LBB17_7
# %bb.6:
	addq	$32, %rsp
	popq	%rbp
	retq
.LBB17_7:                               #   in Loop: Header=BB17_2 Depth=2
	jmp	.LBB17_2
.LBB17_8:                               #   in Loop: Header=BB17_1 Depth=1
	movq	-8(%rbp), %rdi
	xorl	%esi, %esi
	callq	sp_futex_wait
	jmp	.LBB17_1
.Lfunc_end17:
	.size	sp_sem_wait, .Lfunc_end17-sp_sem_wait
                                        # -- End function
	.globl	sp_sem_post                     # -- Begin function sp_sem_post
	.p2align	4
	.type	sp_sem_post,@function
sp_sem_post:                            # @sp_sem_post
# %bb.0:
	pushq	%rbp
	movq	%rsp, %rbp
	subq	$16, %rsp
	movq	%rdi, -8(%rbp)
	movq	-8(%rbp), %rcx
	movl	$1, -12(%rbp)
	movl	-12(%rbp), %eax
	lock		xaddl	%eax, (%rcx)
	movl	%eax, -16(%rbp)
	movq	-8(%rbp), %rdi
	movl	$1, %esi
	callq	sp_futex_wake
	addq	$16, %rsp
	popq	%rbp
	retq
.Lfunc_end18:
	.size	sp_sem_post, .Lfunc_end18-sp_sem_post
                                        # -- End function
	.globl	sp_rwlock_init                  # -- Begin function sp_rwlock_init
	.p2align	4
	.type	sp_rwlock_init,@function
sp_rwlock_init:                         # @sp_rwlock_init
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
.Lfunc_end19:
	.size	sp_rwlock_init, .Lfunc_end19-sp_rwlock_init
                                        # -- End function
	.globl	sp_rwlock_tryrdlock             # -- Begin function sp_rwlock_tryrdlock
	.p2align	4
	.type	sp_rwlock_tryrdlock,@function
sp_rwlock_tryrdlock:                    # @sp_rwlock_tryrdlock
# %bb.0:
	pushq	%rbp
	movq	%rsp, %rbp
	movq	%rdi, -16(%rbp)
	movq	-16(%rbp), %rax
	movl	(%rax), %eax
	movl	%eax, -24(%rbp)
	movl	-24(%rbp), %eax
	movl	%eax, -20(%rbp)
.LBB20_1:                               # =>This Inner Loop Header: Depth=1
	movl	-20(%rbp), %eax
	andl	$-2147483648, %eax              # imm = 0x80000000
	cmpl	$0, %eax
	setne	%al
	xorb	$-1, %al
	testb	$1, %al
	jne	.LBB20_2
	jmp	.LBB20_7
.LBB20_2:                               #   in Loop: Header=BB20_1 Depth=1
	movq	-16(%rbp), %rcx
	movl	-20(%rbp), %eax
	incl	%eax
	movl	%eax, -28(%rbp)
	movl	-20(%rbp), %eax
	movl	-28(%rbp), %edx
	lock		cmpxchgl	%edx, (%rcx)
	movl	%eax, %ecx
	sete	%al
	movb	%al, -37(%rbp)                  # 1-byte Spill
	movl	%ecx, -36(%rbp)                 # 4-byte Spill
	testb	$1, %al
	jne	.LBB20_4
# %bb.3:                                #   in Loop: Header=BB20_1 Depth=1
	movl	-36(%rbp), %eax                 # 4-byte Reload
	movl	%eax, -20(%rbp)
.LBB20_4:                               #   in Loop: Header=BB20_1 Depth=1
	movb	-37(%rbp), %al                  # 1-byte Reload
	andb	$1, %al
	movb	%al, -29(%rbp)
	testb	$1, -29(%rbp)
	je	.LBB20_6
# %bb.5:
	movl	$1, -4(%rbp)
	jmp	.LBB20_8
.LBB20_6:                               #   in Loop: Header=BB20_1 Depth=1
	jmp	.LBB20_1
.LBB20_7:
	movl	$0, -4(%rbp)
.LBB20_8:
	movl	-4(%rbp), %eax
	popq	%rbp
	retq
.Lfunc_end20:
	.size	sp_rwlock_tryrdlock, .Lfunc_end20-sp_rwlock_tryrdlock
                                        # -- End function
	.globl	sp_rwlock_rdlock                # -- Begin function sp_rwlock_rdlock
	.p2align	4
	.type	sp_rwlock_rdlock,@function
sp_rwlock_rdlock:                       # @sp_rwlock_rdlock
# %bb.0:
	pushq	%rbp
	movq	%rsp, %rbp
	subq	$32, %rsp
	movq	%rdi, -8(%rbp)
.LBB21_1:                               # =>This Inner Loop Header: Depth=1
	movq	-8(%rbp), %rax
	movl	(%rax), %eax
	movl	%eax, -16(%rbp)
	movl	-16(%rbp), %eax
	movl	%eax, -12(%rbp)
	movl	-12(%rbp), %eax
	andl	$-2147483648, %eax              # imm = 0x80000000
	cmpl	$0, %eax
	je	.LBB21_3
# %bb.2:                                #   in Loop: Header=BB21_1 Depth=1
	movq	-8(%rbp), %rdi
	movl	-12(%rbp), %esi
	callq	sp_futex_wait
	jmp	.LBB21_1
.LBB21_3:                               #   in Loop: Header=BB21_1 Depth=1
	movq	-8(%rbp), %rcx
	movl	-12(%rbp), %eax
	incl	%eax
	movl	%eax, -20(%rbp)
	movl	-12(%rbp), %eax
	movl	-20(%rbp), %edx
	lock		cmpxchgl	%edx, (%rcx)
	movl	%eax, %ecx
	sete	%al
	movb	%al, -29(%rbp)                  # 1-byte Spill
	movl	%ecx, -28(%rbp)                 # 4-byte Spill
	testb	$1, %al
	jne	.LBB21_5
# %bb.4:                                #   in Loop: Header=BB21_1 Depth=1
	movl	-28(%rbp), %eax                 # 4-byte Reload
	movl	%eax, -12(%rbp)
.LBB21_5:                               #   in Loop: Header=BB21_1 Depth=1
	movb	-29(%rbp), %al                  # 1-byte Reload
	andb	$1, %al
	movb	%al, -21(%rbp)
	testb	$1, -21(%rbp)
	je	.LBB21_7
# %bb.6:
	addq	$32, %rsp
	popq	%rbp
	retq
.LBB21_7:                               #   in Loop: Header=BB21_1 Depth=1
	jmp	.LBB21_1
.Lfunc_end21:
	.size	sp_rwlock_rdlock, .Lfunc_end21-sp_rwlock_rdlock
                                        # -- End function
	.globl	sp_rwlock_rdunlock              # -- Begin function sp_rwlock_rdunlock
	.p2align	4
	.type	sp_rwlock_rdunlock,@function
sp_rwlock_rdunlock:                     # @sp_rwlock_rdunlock
# %bb.0:
	pushq	%rbp
	movq	%rsp, %rbp
	subq	$32, %rsp
	movq	%rdi, -8(%rbp)
	movq	-8(%rbp), %rcx
	movl	$1, -16(%rbp)
	movl	-16(%rbp), %eax
	negl	%eax
	lock		xaddl	%eax, (%rcx)
	movl	%eax, -20(%rbp)
	movl	-20(%rbp), %eax
	movl	%eax, -12(%rbp)
	cmpl	$1, -12(%rbp)
	jne	.LBB22_2
# %bb.1:
	movq	-8(%rbp), %rdi
	movl	$1, %esi
	callq	sp_futex_wake
.LBB22_2:
	addq	$32, %rsp
	popq	%rbp
	retq
.Lfunc_end22:
	.size	sp_rwlock_rdunlock, .Lfunc_end22-sp_rwlock_rdunlock
                                        # -- End function
	.globl	sp_rwlock_trywrlock             # -- Begin function sp_rwlock_trywrlock
	.p2align	4
	.type	sp_rwlock_trywrlock,@function
sp_rwlock_trywrlock:                    # @sp_rwlock_trywrlock
# %bb.0:
	pushq	%rbp
	movq	%rsp, %rbp
	movq	%rdi, -8(%rbp)
	movl	$0, -12(%rbp)
	movq	-8(%rbp), %rcx
	movl	$-2147483648, -16(%rbp)         # imm = 0x80000000
	movl	-12(%rbp), %eax
	movl	-16(%rbp), %edx
	lock		cmpxchgl	%edx, (%rcx)
	movl	%eax, %ecx
	sete	%al
	movb	%al, -25(%rbp)                  # 1-byte Spill
	movl	%ecx, -24(%rbp)                 # 4-byte Spill
	testb	$1, %al
	jne	.LBB23_2
# %bb.1:
	movl	-24(%rbp), %eax                 # 4-byte Reload
	movl	%eax, -12(%rbp)
.LBB23_2:
	movb	-25(%rbp), %al                  # 1-byte Reload
	andb	$1, %al
	movb	%al, -17(%rbp)
	movb	-17(%rbp), %dl
	xorl	%eax, %eax
	movl	$1, %ecx
	testb	$1, %dl
	cmovnel	%ecx, %eax
	popq	%rbp
	retq
.Lfunc_end23:
	.size	sp_rwlock_trywrlock, .Lfunc_end23-sp_rwlock_trywrlock
                                        # -- End function
	.globl	sp_rwlock_wrlock                # -- Begin function sp_rwlock_wrlock
	.p2align	4
	.type	sp_rwlock_wrlock,@function
sp_rwlock_wrlock:                       # @sp_rwlock_wrlock
# %bb.0:
	pushq	%rbp
	movq	%rsp, %rbp
	subq	$32, %rsp
	movq	%rdi, -8(%rbp)
.LBB24_1:                               # =>This Inner Loop Header: Depth=1
	movq	-8(%rbp), %rax
	movl	(%rax), %eax
	movl	%eax, -16(%rbp)
	movl	-16(%rbp), %eax
	movl	%eax, -12(%rbp)
	cmpl	$0, -12(%rbp)
	jne	.LBB24_7
# %bb.2:                                #   in Loop: Header=BB24_1 Depth=1
	movq	-8(%rbp), %rcx
	movl	$-2147483648, -20(%rbp)         # imm = 0x80000000
	movl	-12(%rbp), %eax
	movl	-20(%rbp), %edx
	lock		cmpxchgl	%edx, (%rcx)
	movl	%eax, %ecx
	sete	%al
	movb	%al, -29(%rbp)                  # 1-byte Spill
	movl	%ecx, -28(%rbp)                 # 4-byte Spill
	testb	$1, %al
	jne	.LBB24_4
# %bb.3:                                #   in Loop: Header=BB24_1 Depth=1
	movl	-28(%rbp), %eax                 # 4-byte Reload
	movl	%eax, -12(%rbp)
.LBB24_4:                               #   in Loop: Header=BB24_1 Depth=1
	movb	-29(%rbp), %al                  # 1-byte Reload
	andb	$1, %al
	movb	%al, -21(%rbp)
	testb	$1, -21(%rbp)
	je	.LBB24_6
# %bb.5:
	addq	$32, %rsp
	popq	%rbp
	retq
.LBB24_6:                               #   in Loop: Header=BB24_1 Depth=1
	jmp	.LBB24_8
.LBB24_7:                               #   in Loop: Header=BB24_1 Depth=1
	movq	-8(%rbp), %rdi
	movl	-12(%rbp), %esi
	callq	sp_futex_wait
.LBB24_8:                               #   in Loop: Header=BB24_1 Depth=1
	jmp	.LBB24_1
.Lfunc_end24:
	.size	sp_rwlock_wrlock, .Lfunc_end24-sp_rwlock_wrlock
                                        # -- End function
	.globl	sp_rwlock_wrunlock              # -- Begin function sp_rwlock_wrunlock
	.p2align	4
	.type	sp_rwlock_wrunlock,@function
sp_rwlock_wrunlock:                     # @sp_rwlock_wrunlock
# %bb.0:
	pushq	%rbp
	movq	%rsp, %rbp
	subq	$16, %rsp
	movq	%rdi, -8(%rbp)
	movq	-8(%rbp), %rax
	movl	$0, -12(%rbp)
	movl	-12(%rbp), %ecx
	movl	%ecx, (%rax)
	movq	-8(%rbp), %rdi
	movl	$2147483647, %esi               # imm = 0x7FFFFFFF
	callq	sp_futex_wake
	addq	$16, %rsp
	popq	%rbp
	retq
.Lfunc_end25:
	.size	sp_rwlock_wrunlock, .Lfunc_end25-sp_rwlock_wrunlock
                                        # -- End function
	.p2align	4                               # -- Begin function sp_futex
	.type	sp_futex,@function
sp_futex:                               # @sp_futex
# %bb.0:
	pushq	%rbp
	movq	%rsp, %rbp
	movq	%rdi, -8(%rbp)
	movl	%esi, -12(%rbp)
	movl	%edx, -16(%rbp)
	movq	$0, -32(%rbp)
	movq	$0, -40(%rbp)
	movq	$0, -48(%rbp)
	movq	-8(%rbp), %rdi
	movslq	-12(%rbp), %rsi
	movl	-16(%rbp), %eax
	movl	%eax, %edx
	movq	-32(%rbp), %r10
	movq	-40(%rbp), %r8
	movq	-48(%rbp), %r9
	movl	$202, %eax
	#APP
	syscall
	#NO_APP
	movq	%rax, -24(%rbp)
	movq	-24(%rbp), %rax
	popq	%rbp
	retq
.Lfunc_end26:
	.size	sp_futex, .Lfunc_end26-sp_futex
                                        # -- End function
	.ident	"clang version 20.1.8"
	.section	".note.GNU-stack","",@progbits
	.addrsig
	.addrsig_sym sp_cpu_relax
	.addrsig_sym sp_mutex_lock
	.addrsig_sym sp_futex_wait
	.addrsig_sym sp_mutex_unlock
	.addrsig_sym sp_futex_wake
	.addrsig_sym sp_futex
