	.file	"demo.c"
	.text
	.globl	fp_unwind                       # -- Begin function fp_unwind
	.p2align	4
	.type	fp_unwind,@function
fp_unwind:                              # @fp_unwind
# %bb.0:
	pushq	%rbp
	movq	%rsp, %rbp
	movq	%rdi, -8(%rbp)
	movq	%rsi, -16(%rbp)
	movq	%rdx, -24(%rbp)
	movq	%rcx, -32(%rbp)
	movl	%r8d, -36(%rbp)
	movl	$0, -40(%rbp)
.LBB0_1:                                # =>This Inner Loop Header: Depth=1
	movq	-8(%rbp), %rcx
	xorl	%eax, %eax
                                        # kill: def $al killed $al killed $eax
	cmpq	-16(%rbp), %rcx
	movb	%al, -57(%rbp)                  # 1-byte Spill
	jb	.LBB0_5
# %bb.2:                                #   in Loop: Header=BB0_1 Depth=1
	movq	-8(%rbp), %rcx
	xorl	%eax, %eax
                                        # kill: def $al killed $al killed $eax
	cmpq	-24(%rbp), %rcx
	movb	%al, -57(%rbp)                  # 1-byte Spill
	jae	.LBB0_5
# %bb.3:                                #   in Loop: Header=BB0_1 Depth=1
	movq	-8(%rbp), %rcx
	andq	$7, %rcx
	xorl	%eax, %eax
                                        # kill: def $al killed $al killed $eax
	cmpq	$0, %rcx
	movb	%al, -57(%rbp)                  # 1-byte Spill
	jne	.LBB0_5
# %bb.4:                                #   in Loop: Header=BB0_1 Depth=1
	movl	-40(%rbp), %eax
	cmpl	-36(%rbp), %eax
	setl	%al
	movb	%al, -57(%rbp)                  # 1-byte Spill
.LBB0_5:                                #   in Loop: Header=BB0_1 Depth=1
	movb	-57(%rbp), %al                  # 1-byte Reload
	testb	$1, %al
	jne	.LBB0_6
	jmp	.LBB0_9
.LBB0_6:                                #   in Loop: Header=BB0_1 Depth=1
	movq	-8(%rbp), %rax
	movq	(%rax), %rax
	movq	%rax, -48(%rbp)
	movq	-8(%rbp), %rax
	movq	8(%rax), %rax
	movq	%rax, -56(%rbp)
	movq	-56(%rbp), %rdx
	movq	-32(%rbp), %rax
	movl	-40(%rbp), %ecx
	movl	%ecx, %esi
	addl	$1, %esi
	movl	%esi, -40(%rbp)
	movslq	%ecx, %rcx
	movq	%rdx, (%rax,%rcx,8)
	movq	-48(%rbp), %rax
	cmpq	-8(%rbp), %rax
	ja	.LBB0_8
# %bb.7:
	jmp	.LBB0_9
.LBB0_8:                                #   in Loop: Header=BB0_1 Depth=1
	movq	-48(%rbp), %rax
	movq	%rax, -8(%rbp)
	jmp	.LBB0_1
.LBB0_9:
	movl	-40(%rbp), %eax
	popq	%rbp
	retq
.Lfunc_end0:
	.size	fp_unwind, .Lfunc_end0-fp_unwind
                                        # -- End function
	.globl	rb_avail                        # -- Begin function rb_avail
	.p2align	4
	.type	rb_avail,@function
rb_avail:                               # @rb_avail
# %bb.0:
	pushq	%rbp
	movq	%rsp, %rbp
	movq	%rdi, -8(%rbp)
	movq	%rsi, -16(%rbp)
	movq	-8(%rbp), %rax
	subq	-16(%rbp), %rax
	popq	%rbp
	retq
.Lfunc_end1:
	.size	rb_avail, .Lfunc_end1-rb_avail
                                        # -- End function
	.globl	rb_offset                       # -- Begin function rb_offset
	.p2align	4
	.type	rb_offset,@function
rb_offset:                              # @rb_offset
# %bb.0:
	pushq	%rbp
	movq	%rsp, %rbp
	movq	%rdi, -8(%rbp)
	movq	%rsi, -16(%rbp)
	movq	-8(%rbp), %rax
	movq	-16(%rbp), %rcx
	subq	$1, %rcx
	andq	%rcx, %rax
	popq	%rbp
	retq
.Lfunc_end2:
	.size	rb_offset, .Lfunc_end2-rb_offset
                                        # -- End function
	.globl	rb_first_chunk                  # -- Begin function rb_first_chunk
	.p2align	4
	.type	rb_first_chunk,@function
rb_first_chunk:                         # @rb_first_chunk
# %bb.0:
	pushq	%rbp
	movq	%rsp, %rbp
	movq	%rdi, -8(%rbp)
	movq	%rsi, -16(%rbp)
	movq	%rdx, -24(%rbp)
	movq	-24(%rbp), %rax
	subq	-8(%rbp), %rax
	movq	%rax, -32(%rbp)
	movq	-16(%rbp), %rax
	cmpq	-32(%rbp), %rax
	ja	.LBB3_2
# %bb.1:
	movq	-16(%rbp), %rax
	movq	%rax, -40(%rbp)                 # 8-byte Spill
	jmp	.LBB3_3
.LBB3_2:
	movq	-32(%rbp), %rax
	movq	%rax, -40(%rbp)                 # 8-byte Spill
.LBB3_3:
	movq	-40(%rbp), %rax                 # 8-byte Reload
	popq	%rbp
	retq
.Lfunc_end3:
	.size	rb_first_chunk, .Lfunc_end3-rb_first_chunk
                                        # -- End function
	.globl	fnv1a                           # -- Begin function fnv1a
	.p2align	4
	.type	fnv1a,@function
fnv1a:                                  # @fnv1a
# %bb.0:
	pushq	%rbp
	movq	%rsp, %rbp
	movq	%rdi, -8(%rbp)
	movq	%rsi, -16(%rbp)
	movabsq	$-3750763034362895579, %rax     # imm = 0xCBF29CE484222325
	movq	%rax, -24(%rbp)
	movq	$0, -32(%rbp)
.LBB4_1:                                # =>This Inner Loop Header: Depth=1
	movq	-32(%rbp), %rax
	cmpq	-16(%rbp), %rax
	jae	.LBB4_4
# %bb.2:                                #   in Loop: Header=BB4_1 Depth=1
	movq	-8(%rbp), %rax
	movq	-32(%rbp), %rcx
	movzbl	(%rax,%rcx), %eax
                                        # kill: def $rax killed $eax
	xorq	-24(%rbp), %rax
	movq	%rax, -24(%rbp)
	movabsq	$1099511628211, %rax            # imm = 0x100000001B3
	imulq	-24(%rbp), %rax
	movq	%rax, -24(%rbp)
# %bb.3:                                #   in Loop: Header=BB4_1 Depth=1
	movq	-32(%rbp), %rax
	addq	$1, %rax
	movq	%rax, -32(%rbp)
	jmp	.LBB4_1
.LBB4_4:
	movq	-24(%rbp), %rax
	popq	%rbp
	retq
.Lfunc_end4:
	.size	fnv1a, .Lfunc_end4-fnv1a
                                        # -- End function
	.globl	demo_selftest                   # -- Begin function demo_selftest
	.p2align	4
	.type	demo_selftest,@function
demo_selftest:                          # @demo_selftest
# %bb.0:
	pushq	%rbp
	movq	%rsp, %rbp
	subq	$608, %rsp                      # imm = 0x260
	leaq	-64(%rbp), %rax
	movq	%rax, -72(%rbp)
	movq	-72(%rbp), %rax
	addq	$16, %rax
	movq	%rax, -64(%rbp)
	movq	$43690, -56(%rbp)               # imm = 0xAAAA
	movq	-72(%rbp), %rax
	addq	$32, %rax
	movq	%rax, -48(%rbp)
	movq	$48059, -40(%rbp)               # imm = 0xBBBB
	movq	$0, -32(%rbp)
	movq	$52428, -24(%rbp)               # imm = 0xCCCC
	movq	-72(%rbp), %rdi
	movq	-72(%rbp), %rsi
	movq	-72(%rbp), %rdx
	addq	$48, %rdx
	leaq	-592(%rbp), %rcx
	movl	$64, %r8d
	callq	fp_unwind
	movl	%eax, -596(%rbp)
	cmpl	$3, -596(%rbp)
	je	.LBB5_2
# %bb.1:
	movl	$1, -4(%rbp)
	jmp	.LBB5_25
.LBB5_2:
	cmpq	$43690, -592(%rbp)              # imm = 0xAAAA
	je	.LBB5_4
# %bb.3:
	movl	$2, -4(%rbp)
	jmp	.LBB5_25
.LBB5_4:
	cmpq	$48059, -584(%rbp)              # imm = 0xBBBB
	je	.LBB5_6
# %bb.5:
	movl	$3, -4(%rbp)
	jmp	.LBB5_25
.LBB5_6:
	cmpq	$52428, -576(%rbp)              # imm = 0xCCCC
	je	.LBB5_8
# %bb.7:
	movl	$4, -4(%rbp)
	jmp	.LBB5_25
.LBB5_8:
	movq	-72(%rbp), %rdi
	addq	$1, %rdi
	movq	-72(%rbp), %rsi
	movq	-72(%rbp), %rdx
	addq	$48, %rdx
	leaq	-592(%rbp), %rcx
	movl	$64, %r8d
	callq	fp_unwind
	cmpl	$0, %eax
	je	.LBB5_10
# %bb.9:
	movl	$5, -4(%rbp)
	jmp	.LBB5_25
.LBB5_10:
	movl	$1000, %edi                     # imm = 0x3E8
	movl	$40, %esi
	callq	rb_avail
	cmpq	$960, %rax                      # imm = 0x3C0
	je	.LBB5_12
# %bb.11:
	movl	$6, -4(%rbp)
	jmp	.LBB5_25
.LBB5_12:
	movl	$5, %edi
	movq	$-16, %rsi
	callq	rb_avail
	cmpq	$21, %rax
	je	.LBB5_14
# %bb.13:
	movl	$7, -4(%rbp)
	jmp	.LBB5_25
.LBB5_14:
	movl	$4103, %edi                     # imm = 0x1007
	movl	$4096, %esi                     # imm = 0x1000
	callq	rb_offset
	cmpq	$7, %rax
	je	.LBB5_16
# %bb.15:
	movl	$8, -4(%rbp)
	jmp	.LBB5_25
.LBB5_16:
	movl	$233492771, %edi                # imm = 0xDEAD123
	movl	$4096, %esi                     # imm = 0x1000
	callq	rb_offset
	cmpq	$291, %rax                      # imm = 0x123
	je	.LBB5_18
# %bb.17:
	movl	$9, -4(%rbp)
	jmp	.LBB5_25
.LBB5_18:
	movl	$100, %edi
	movl	$12, %esi
	movl	$4096, %edx                     # imm = 0x1000
	callq	rb_first_chunk
	cmpq	$12, %rax
	je	.LBB5_20
# %bb.19:
	movl	$10, -4(%rbp)
	jmp	.LBB5_25
.LBB5_20:
	movl	$4090, %edi                     # imm = 0xFFA
	movl	$12, %esi
	movl	$4096, %edx                     # imm = 0x1000
	callq	rb_first_chunk
	cmpq	$6, %rax
	je	.LBB5_22
# %bb.21:
	movl	$11, -4(%rbp)
	jmp	.LBB5_25
.LBB5_22:
	leaq	.L.str(%rip), %rdi
	movl	$2, %esi
	callq	fnv1a
	movabsq	$620445648566982762, %rcx       # imm = 0x89C4407B545986A
	cmpq	%rcx, %rax
	je	.LBB5_24
# %bb.23:
	movl	$12, -4(%rbp)
	jmp	.LBB5_25
.LBB5_24:
	movl	$0, -4(%rbp)
.LBB5_25:
	movl	-4(%rbp), %eax
	addq	$608, %rsp                      # imm = 0x260
	popq	%rbp
	retq
.Lfunc_end5:
	.size	demo_selftest, .Lfunc_end5-demo_selftest
                                        # -- End function
	.type	.L.str,@object                  # @.str
	.section	.rodata.str1.1,"aMS",@progbits,1
.L.str:
	.asciz	"ab"
	.size	.L.str, 3

	.ident	"clang version 20.1.8"
	.section	".note.GNU-stack","",@progbits
	.addrsig
	.addrsig_sym fp_unwind
	.addrsig_sym rb_avail
	.addrsig_sym rb_offset
	.addrsig_sym rb_first_chunk
	.addrsig_sym fnv1a
