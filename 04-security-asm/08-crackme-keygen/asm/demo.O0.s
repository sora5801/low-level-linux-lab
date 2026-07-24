	.file	"demo.c"
	.text
	.globl	key_from_name                   # -- Begin function key_from_name
	.p2align	4
	.type	key_from_name,@function
key_from_name:                          # @key_from_name
# %bb.0:
	pushq	%rbp
	movq	%rsp, %rbp
	subq	$32, %rsp
	movq	%rdi, -8(%rbp)
	movabsq	$-3750763034362895579, %rax     # imm = 0xCBF29CE484222325
	movq	%rax, -16(%rbp)
	movq	-8(%rbp), %rax
	movq	%rax, -24(%rbp)
.LBB0_1:                                # =>This Inner Loop Header: Depth=1
	movq	-24(%rbp), %rax
	cmpb	$0, (%rax)
	je	.LBB0_4
# %bb.2:                                #   in Loop: Header=BB0_1 Depth=1
	movq	-24(%rbp), %rax
	movzbl	(%rax), %eax
                                        # kill: def $rax killed $eax
	xorq	-16(%rbp), %rax
	movq	%rax, -16(%rbp)
	movabsq	$1099511628211, %rax            # imm = 0x100000001B3
	imulq	-16(%rbp), %rax
	movq	%rax, -16(%rbp)
	movq	-16(%rbp), %rdi
	movl	$7, %esi
	callq	rotl64
	movq	%rax, -16(%rbp)
	movabsq	$25214903917, %rax              # imm = 0x5DEECE66D
	xorq	-16(%rbp), %rax
	movq	%rax, -16(%rbp)
# %bb.3:                                #   in Loop: Header=BB0_1 Depth=1
	movq	-24(%rbp), %rax
	addq	$1, %rax
	movq	%rax, -24(%rbp)
	jmp	.LBB0_1
.LBB0_4:
	movq	-16(%rbp), %rax
	shrq	$33, %rax
	xorq	-16(%rbp), %rax
	movq	%rax, -16(%rbp)
	movabsq	$-49064778989728563, %rax       # imm = 0xFF51AFD7ED558CCD
	imulq	-16(%rbp), %rax
	movq	%rax, -16(%rbp)
	movq	-16(%rbp), %rax
	shrq	$29, %rax
	xorq	-16(%rbp), %rax
	movq	%rax, -16(%rbp)
	movabsq	$-4265267296055464877, %rax     # imm = 0xC4CEB9FE1A85EC53
	imulq	-16(%rbp), %rax
	movq	%rax, -16(%rbp)
	movq	-16(%rbp), %rax
	shrq	$33, %rax
	xorq	-16(%rbp), %rax
	movq	%rax, -16(%rbp)
	movq	-16(%rbp), %rax
	addq	$32, %rsp
	popq	%rbp
	retq
.Lfunc_end0:
	.size	key_from_name, .Lfunc_end0-key_from_name
                                        # -- End function
	.p2align	4                               # -- Begin function rotl64
	.type	rotl64,@function
rotl64:                                 # @rotl64
# %bb.0:
	pushq	%rbp
	movq	%rsp, %rbp
	movq	%rdi, -8(%rbp)
	movl	%esi, -12(%rbp)
	movq	-8(%rbp), %rax
	movl	-12(%rbp), %ecx
                                        # kill: def $rcx killed $ecx
                                        # kill: def $cl killed $rcx
	shlq	%cl, %rax
	movq	-8(%rbp), %rdx
	movl	$64, %ecx
	subl	-12(%rbp), %ecx
	movl	%ecx, %ecx
                                        # kill: def $rcx killed $ecx
                                        # kill: def $cl killed $rcx
	shrq	%cl, %rdx
	movq	%rdx, %rcx
	orq	%rcx, %rax
	popq	%rbp
	retq
.Lfunc_end1:
	.size	rotl64, .Lfunc_end1-rotl64
                                        # -- End function
	.globl	validate                        # -- Begin function validate
	.p2align	4
	.type	validate,@function
validate:                               # @validate
# %bb.0:
	pushq	%rbp
	movq	%rsp, %rbp
	subq	$64, %rsp
	movq	%rdi, -16(%rbp)
	movq	%rsi, -24(%rbp)
	movq	-16(%rbp), %rdi
	callq	key_from_name
	movq	%rax, -56(%rbp)
	movq	-56(%rbp), %rdi
	leaq	-48(%rbp), %rsi
	callq	format_serial
	movq	-24(%rbp), %rdi
	callq	slen
	cmpl	$19, %eax
	je	.LBB2_2
# %bb.1:
	movl	$0, -4(%rbp)
	jmp	.LBB2_3
.LBB2_2:
	movq	-24(%rbp), %rdi
	leaq	-48(%rbp), %rsi
	movl	$19, %edx
	callq	ct_equal
	movl	%eax, -4(%rbp)
.LBB2_3:
	movl	-4(%rbp), %eax
	addq	$64, %rsp
	popq	%rbp
	retq
.Lfunc_end2:
	.size	validate, .Lfunc_end2-validate
                                        # -- End function
	.p2align	4                               # -- Begin function format_serial
	.type	format_serial,@function
format_serial:                          # @format_serial
# %bb.0:
	pushq	%rbp
	movq	%rsp, %rbp
	movq	%rdi, -8(%rbp)
	movq	%rsi, -16(%rbp)
	movl	$0, -20(%rbp)
	movl	$0, -24(%rbp)
.LBB3_1:                                # =>This Inner Loop Header: Depth=1
	cmpl	$4, -24(%rbp)
	jae	.LBB3_6
# %bb.2:                                #   in Loop: Header=BB3_1 Depth=1
	movq	-8(%rbp), %rax
	movl	-24(%rbp), %edx
	shll	$4, %edx
	movl	$48, %ecx
	subl	%edx, %ecx
	movl	%ecx, %ecx
                                        # kill: def $rcx killed $ecx
                                        # kill: def $cl killed $rcx
	shrq	%cl, %rax
	andq	$65535, %rax                    # imm = 0xFFFF
                                        # kill: def $eax killed $eax killed $rax
	movl	%eax, -28(%rbp)
	movl	-28(%rbp), %eax
	shrl	$12, %eax
	andl	$15, %eax
	movl	%eax, %eax
	movl	%eax, %ecx
	leaq	format_serial.HEX(%rip), %rax
	movb	(%rax,%rcx), %dl
	movq	-16(%rbp), %rax
	movl	-20(%rbp), %ecx
	movl	%ecx, %esi
	addl	$1, %esi
	movl	%esi, -20(%rbp)
	movl	%ecx, %ecx
                                        # kill: def $rcx killed $ecx
	movb	%dl, (%rax,%rcx)
	movl	-28(%rbp), %eax
	shrl	$8, %eax
	andl	$15, %eax
	movl	%eax, %eax
	movl	%eax, %ecx
	leaq	format_serial.HEX(%rip), %rax
	movb	(%rax,%rcx), %dl
	movq	-16(%rbp), %rax
	movl	-20(%rbp), %ecx
	movl	%ecx, %esi
	addl	$1, %esi
	movl	%esi, -20(%rbp)
	movl	%ecx, %ecx
                                        # kill: def $rcx killed $ecx
	movb	%dl, (%rax,%rcx)
	movl	-28(%rbp), %eax
	shrl	$4, %eax
	andl	$15, %eax
	movl	%eax, %eax
	movl	%eax, %ecx
	leaq	format_serial.HEX(%rip), %rax
	movb	(%rax,%rcx), %dl
	movq	-16(%rbp), %rax
	movl	-20(%rbp), %ecx
	movl	%ecx, %esi
	addl	$1, %esi
	movl	%esi, -20(%rbp)
	movl	%ecx, %ecx
                                        # kill: def $rcx killed $ecx
	movb	%dl, (%rax,%rcx)
	movl	-28(%rbp), %eax
	andl	$15, %eax
	movl	%eax, %eax
	movl	%eax, %ecx
	leaq	format_serial.HEX(%rip), %rax
	movb	(%rax,%rcx), %dl
	movq	-16(%rbp), %rax
	movl	-20(%rbp), %ecx
	movl	%ecx, %esi
	addl	$1, %esi
	movl	%esi, -20(%rbp)
	movl	%ecx, %ecx
                                        # kill: def $rcx killed $ecx
	movb	%dl, (%rax,%rcx)
	cmpl	$3, -24(%rbp)
	je	.LBB3_4
# %bb.3:                                #   in Loop: Header=BB3_1 Depth=1
	movq	-16(%rbp), %rax
	movl	-20(%rbp), %ecx
	movl	%ecx, %edx
	addl	$1, %edx
	movl	%edx, -20(%rbp)
	movl	%ecx, %ecx
                                        # kill: def $rcx killed $ecx
	movb	$45, (%rax,%rcx)
.LBB3_4:                                #   in Loop: Header=BB3_1 Depth=1
	jmp	.LBB3_5
.LBB3_5:                                #   in Loop: Header=BB3_1 Depth=1
	movl	-24(%rbp), %eax
	addl	$1, %eax
	movl	%eax, -24(%rbp)
	jmp	.LBB3_1
.LBB3_6:
	movq	-16(%rbp), %rax
	movl	-20(%rbp), %ecx
                                        # kill: def $rcx killed $ecx
	movb	$0, (%rax,%rcx)
	popq	%rbp
	retq
.Lfunc_end3:
	.size	format_serial, .Lfunc_end3-format_serial
                                        # -- End function
	.p2align	4                               # -- Begin function slen
	.type	slen,@function
slen:                                   # @slen
# %bb.0:
	pushq	%rbp
	movq	%rsp, %rbp
	movq	%rdi, -8(%rbp)
	movq	-8(%rbp), %rax
	movq	%rax, -16(%rbp)
.LBB4_1:                                # =>This Inner Loop Header: Depth=1
	movq	-16(%rbp), %rax
	cmpb	$0, (%rax)
	je	.LBB4_3
# %bb.2:                                #   in Loop: Header=BB4_1 Depth=1
	movq	-16(%rbp), %rax
	addq	$1, %rax
	movq	%rax, -16(%rbp)
	jmp	.LBB4_1
.LBB4_3:
	movq	-16(%rbp), %rax
	movq	-8(%rbp), %rcx
	subq	%rcx, %rax
                                        # kill: def $eax killed $eax killed $rax
	popq	%rbp
	retq
.Lfunc_end4:
	.size	slen, .Lfunc_end4-slen
                                        # -- End function
	.p2align	4                               # -- Begin function ct_equal
	.type	ct_equal,@function
ct_equal:                               # @ct_equal
# %bb.0:
	pushq	%rbp
	movq	%rsp, %rbp
	movq	%rdi, -8(%rbp)
	movq	%rsi, -16(%rbp)
	movl	%edx, -20(%rbp)
	movl	$0, -24(%rbp)
	movl	$0, -28(%rbp)
.LBB5_1:                                # =>This Inner Loop Header: Depth=1
	movl	-28(%rbp), %eax
	cmpl	-20(%rbp), %eax
	jae	.LBB5_4
# %bb.2:                                #   in Loop: Header=BB5_1 Depth=1
	movq	-8(%rbp), %rax
	movl	-28(%rbp), %ecx
                                        # kill: def $rcx killed $ecx
	movzbl	(%rax,%rcx), %eax
	movq	-16(%rbp), %rcx
	movl	-28(%rbp), %edx
                                        # kill: def $rdx killed $edx
	movzbl	(%rcx,%rdx), %ecx
	xorl	%ecx, %eax
	orl	-24(%rbp), %eax
	movl	%eax, -24(%rbp)
# %bb.3:                                #   in Loop: Header=BB5_1 Depth=1
	movl	-28(%rbp), %eax
	addl	$1, %eax
	movl	%eax, -28(%rbp)
	jmp	.LBB5_1
.LBB5_4:
	movl	-24(%rbp), %eax
	xorl	%ecx, %ecx
	subl	-24(%rbp), %ecx
	orl	%ecx, %eax
	shrl	$31, %eax
	xorl	$1, %eax
	popq	%rbp
	retq
.Lfunc_end5:
	.size	ct_equal, .Lfunc_end5-ct_equal
                                        # -- End function
	.type	format_serial.HEX,@object       # @format_serial.HEX
	.section	.rodata,"a",@progbits
	.p2align	4, 0x0
format_serial.HEX:
	.asciz	"0123456789ABCDEF"
	.size	format_serial.HEX, 17

	.ident	"clang version 20.1.8"
	.section	".note.GNU-stack","",@progbits
	.addrsig
	.addrsig_sym key_from_name
	.addrsig_sym rotl64
	.addrsig_sym format_serial
	.addrsig_sym slen
	.addrsig_sym ct_equal
	.addrsig_sym format_serial.HEX
