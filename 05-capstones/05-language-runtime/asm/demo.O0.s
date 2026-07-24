	.file	"demo.c"
	.text
	.globl	vm_run                          # -- Begin function vm_run
	.p2align	4
	.type	vm_run,@function
vm_run:                                 # @vm_run
# %bb.0:
	pushq	%rbp
	movq	%rsp, %rbp
	subq	$288, %rsp                      # imm = 0x120
	movq	%rdi, -8(%rbp)
	movl	$0, -340(%rbp)
	movl	$0, -344(%rbp)
	movl	$0, -348(%rbp)
.LBB0_1:                                # =>This Inner Loop Header: Depth=1
	cmpl	$8, -348(%rbp)
	jge	.LBB0_4
# %bb.2:                                #   in Loop: Header=BB0_1 Depth=1
	movslq	-348(%rbp), %rax
	movq	$0, -336(%rbp,%rax,8)
# %bb.3:                                #   in Loop: Header=BB0_1 Depth=1
	movl	-348(%rbp), %eax
	addl	$1, %eax
	movl	%eax, -348(%rbp)
	jmp	.LBB0_1
.LBB0_4:
	movq	-8(%rbp), %rax
	movl	-344(%rbp), %ecx
	movl	%ecx, %edx
	addl	$1, %edx
	movl	%edx, -344(%rbp)
	movslq	%ecx, %rcx
	movzbl	(%rax,%rcx), %eax
	movl	%eax, %ecx
	leaq	vm_run.table(%rip), %rax
	movq	(%rax,%rcx,8), %rax
	movq	%rax, -416(%rbp)                # 8-byte Spill
	jmp	.LBB0_16
.Ltmp0:                                 # Block address taken
.LBB0_5:                                #   in Loop: Header=BB0_16 Depth=1
	movq	-8(%rbp), %rax
	movl	-344(%rbp), %ecx
	movl	%ecx, %edx
	addl	$1, %edx
	movl	%edx, -344(%rbp)
	movslq	%ecx, %rcx
	movzbl	(%rax,%rcx), %eax
	movl	%eax, %ecx
	movl	-340(%rbp), %eax
	movl	%eax, %edx
	addl	$1, %edx
	movl	%edx, -340(%rbp)
	cltq
	movq	%rcx, -272(%rbp,%rax,8)
	movq	-8(%rbp), %rax
	movl	-344(%rbp), %ecx
	movl	%ecx, %edx
	addl	$1, %edx
	movl	%edx, -344(%rbp)
	movslq	%ecx, %rcx
	movzbl	(%rax,%rcx), %eax
	movl	%eax, %ecx
	leaq	vm_run.table(%rip), %rax
	movq	(%rax,%rcx,8), %rax
	movq	%rax, -416(%rbp)                # 8-byte Spill
	jmp	.LBB0_16
.Ltmp1:                                 # Block address taken
.LBB0_6:                                #   in Loop: Header=BB0_16 Depth=1
	movq	-8(%rbp), %rax
	movl	-344(%rbp), %ecx
	movl	%ecx, %edx
	addl	$1, %edx
	movl	%edx, -344(%rbp)
	movslq	%ecx, %rcx
	movzbl	(%rax,%rcx), %eax
                                        # kill: def $rax killed $eax
	movq	-336(%rbp,%rax,8), %rcx
	movl	-340(%rbp), %eax
	movl	%eax, %edx
	addl	$1, %edx
	movl	%edx, -340(%rbp)
	cltq
	movq	%rcx, -272(%rbp,%rax,8)
	movq	-8(%rbp), %rax
	movl	-344(%rbp), %ecx
	movl	%ecx, %edx
	addl	$1, %edx
	movl	%edx, -344(%rbp)
	movslq	%ecx, %rcx
	movzbl	(%rax,%rcx), %eax
	movl	%eax, %ecx
	leaq	vm_run.table(%rip), %rax
	movq	(%rax,%rcx,8), %rax
	movq	%rax, -416(%rbp)                # 8-byte Spill
	jmp	.LBB0_16
.Ltmp2:                                 # Block address taken
.LBB0_7:                                #   in Loop: Header=BB0_16 Depth=1
	movl	-340(%rbp), %eax
	addl	$-1, %eax
	movl	%eax, -340(%rbp)
	cltq
	movq	-272(%rbp,%rax,8), %rcx
	movq	-8(%rbp), %rax
	movl	-344(%rbp), %edx
	movl	%edx, %esi
	addl	$1, %esi
	movl	%esi, -344(%rbp)
	movslq	%edx, %rdx
	movzbl	(%rax,%rdx), %eax
                                        # kill: def $rax killed $eax
	movq	%rcx, -336(%rbp,%rax,8)
	movq	-8(%rbp), %rax
	movl	-344(%rbp), %ecx
	movl	%ecx, %edx
	addl	$1, %edx
	movl	%edx, -344(%rbp)
	movslq	%ecx, %rcx
	movzbl	(%rax,%rcx), %eax
	movl	%eax, %ecx
	leaq	vm_run.table(%rip), %rax
	movq	(%rax,%rcx,8), %rax
	movq	%rax, -416(%rbp)                # 8-byte Spill
	jmp	.LBB0_16
.Ltmp3:                                 # Block address taken
.LBB0_8:                                #   in Loop: Header=BB0_16 Depth=1
	movl	-340(%rbp), %eax
	addl	$-1, %eax
	movl	%eax, -340(%rbp)
	cltq
	movq	-272(%rbp,%rax,8), %rax
	movq	%rax, -360(%rbp)
	movl	-340(%rbp), %eax
	addl	$-1, %eax
	movl	%eax, -340(%rbp)
	cltq
	movq	-272(%rbp,%rax,8), %rax
	movq	%rax, -368(%rbp)
	movq	-368(%rbp), %rcx
	addq	-360(%rbp), %rcx
	movl	-340(%rbp), %eax
	movl	%eax, %edx
	addl	$1, %edx
	movl	%edx, -340(%rbp)
	cltq
	movq	%rcx, -272(%rbp,%rax,8)
	movq	-8(%rbp), %rax
	movl	-344(%rbp), %ecx
	movl	%ecx, %edx
	addl	$1, %edx
	movl	%edx, -344(%rbp)
	movslq	%ecx, %rcx
	movzbl	(%rax,%rcx), %eax
	movl	%eax, %ecx
	leaq	vm_run.table(%rip), %rax
	movq	(%rax,%rcx,8), %rax
	movq	%rax, -416(%rbp)                # 8-byte Spill
	jmp	.LBB0_16
.Ltmp4:                                 # Block address taken
.LBB0_9:                                #   in Loop: Header=BB0_16 Depth=1
	movl	-340(%rbp), %eax
	addl	$-1, %eax
	movl	%eax, -340(%rbp)
	cltq
	movq	-272(%rbp,%rax,8), %rax
	movq	%rax, -376(%rbp)
	movl	-340(%rbp), %eax
	addl	$-1, %eax
	movl	%eax, -340(%rbp)
	cltq
	movq	-272(%rbp,%rax,8), %rax
	movq	%rax, -384(%rbp)
	movq	-384(%rbp), %rcx
	subq	-376(%rbp), %rcx
	movl	-340(%rbp), %eax
	movl	%eax, %edx
	addl	$1, %edx
	movl	%edx, -340(%rbp)
	cltq
	movq	%rcx, -272(%rbp,%rax,8)
	movq	-8(%rbp), %rax
	movl	-344(%rbp), %ecx
	movl	%ecx, %edx
	addl	$1, %edx
	movl	%edx, -344(%rbp)
	movslq	%ecx, %rcx
	movzbl	(%rax,%rcx), %eax
	movl	%eax, %ecx
	leaq	vm_run.table(%rip), %rax
	movq	(%rax,%rcx,8), %rax
	movq	%rax, -416(%rbp)                # 8-byte Spill
	jmp	.LBB0_16
.Ltmp5:                                 # Block address taken
.LBB0_10:                               #   in Loop: Header=BB0_16 Depth=1
	movl	-340(%rbp), %eax
	addl	$-1, %eax
	movl	%eax, -340(%rbp)
	cltq
	movq	-272(%rbp,%rax,8), %rax
	movq	%rax, -392(%rbp)
	movl	-340(%rbp), %eax
	addl	$-1, %eax
	movl	%eax, -340(%rbp)
	cltq
	movq	-272(%rbp,%rax,8), %rax
	movq	%rax, -400(%rbp)
	movq	-400(%rbp), %rax
	cmpq	-392(%rbp), %rax
	setle	%al
	andb	$1, %al
	movzbl	%al, %eax
	movslq	%eax, %rcx
	movl	-340(%rbp), %eax
	movl	%eax, %edx
	addl	$1, %edx
	movl	%edx, -340(%rbp)
	cltq
	movq	%rcx, -272(%rbp,%rax,8)
	movq	-8(%rbp), %rax
	movl	-344(%rbp), %ecx
	movl	%ecx, %edx
	addl	$1, %edx
	movl	%edx, -344(%rbp)
	movslq	%ecx, %rcx
	movzbl	(%rax,%rcx), %eax
	movl	%eax, %ecx
	leaq	vm_run.table(%rip), %rax
	movq	(%rax,%rcx,8), %rax
	movq	%rax, -416(%rbp)                # 8-byte Spill
	jmp	.LBB0_16
.Ltmp6:                                 # Block address taken
.LBB0_11:                               #   in Loop: Header=BB0_16 Depth=1
	movq	-8(%rbp), %rax
	movl	-344(%rbp), %ecx
	movl	%ecx, %edx
	addl	$1, %edx
	movl	%edx, -344(%rbp)
	movslq	%ecx, %rcx
	movsbl	(%rax,%rcx), %eax
	movl	%eax, -404(%rbp)
	movl	-340(%rbp), %eax
	addl	$-1, %eax
	movl	%eax, -340(%rbp)
	cltq
	cmpq	$0, -272(%rbp,%rax,8)
	jne	.LBB0_13
# %bb.12:                               #   in Loop: Header=BB0_16 Depth=1
	movl	-404(%rbp), %eax
	addl	-344(%rbp), %eax
	movl	%eax, -344(%rbp)
.LBB0_13:                               #   in Loop: Header=BB0_16 Depth=1
	movq	-8(%rbp), %rax
	movl	-344(%rbp), %ecx
	movl	%ecx, %edx
	addl	$1, %edx
	movl	%edx, -344(%rbp)
	movslq	%ecx, %rcx
	movzbl	(%rax,%rcx), %eax
	movl	%eax, %ecx
	leaq	vm_run.table(%rip), %rax
	movq	(%rax,%rcx,8), %rax
	movq	%rax, -416(%rbp)                # 8-byte Spill
	jmp	.LBB0_16
.Ltmp7:                                 # Block address taken
.LBB0_14:                               #   in Loop: Header=BB0_16 Depth=1
	movq	-8(%rbp), %rax
	movl	-344(%rbp), %ecx
	movl	%ecx, %edx
	addl	$1, %edx
	movl	%edx, -344(%rbp)
	movslq	%ecx, %rcx
	movsbl	(%rax,%rcx), %eax
	movl	%eax, -408(%rbp)
	movl	-408(%rbp), %eax
	addl	-344(%rbp), %eax
	movl	%eax, -344(%rbp)
	movq	-8(%rbp), %rax
	movl	-344(%rbp), %ecx
	movl	%ecx, %edx
	addl	$1, %edx
	movl	%edx, -344(%rbp)
	movslq	%ecx, %rcx
	movzbl	(%rax,%rcx), %eax
	movl	%eax, %ecx
	leaq	vm_run.table(%rip), %rax
	movq	(%rax,%rcx,8), %rax
	movq	%rax, -416(%rbp)                # 8-byte Spill
	jmp	.LBB0_16
.Ltmp8:                                 # Block address taken
.LBB0_15:
	movl	-340(%rbp), %eax
	addl	$-1, %eax
	movl	%eax, -340(%rbp)
	cltq
	movq	-272(%rbp,%rax,8), %rax
	addq	$288, %rsp                      # imm = 0x120
	popq	%rbp
	retq
.LBB0_16:                               # =>This Inner Loop Header: Depth=1
	movq	-416(%rbp), %rax                # 8-byte Reload
	jmpq	*%rax
.Lfunc_end0:
	.size	vm_run, .Lfunc_end0-vm_run
                                        # -- End function
	.globl	main                            # -- Begin function main
	.p2align	4
	.type	main,@function
main:                                   # @main
# %bb.0:
	pushq	%rbp
	movq	%rsp, %rbp
	subq	$16, %rsp
	movl	$0, -4(%rbp)
	leaq	main.prog(%rip), %rdi
	callq	vm_run
                                        # kill: def $eax killed $eax killed $rax
	addq	$16, %rsp
	popq	%rbp
	retq
.Lfunc_end1:
	.size	main, .Lfunc_end1-main
                                        # -- End function
	.type	vm_run.table,@object            # @vm_run.table
	.data
	.p2align	4, 0x0
vm_run.table:
	.quad	.Ltmp0
	.quad	.Ltmp1
	.quad	.Ltmp2
	.quad	.Ltmp3
	.quad	.Ltmp4
	.quad	.Ltmp5
	.quad	.Ltmp6
	.quad	.Ltmp7
	.quad	.Ltmp8
	.size	vm_run.table, 72

	.type	main.prog,@object               # @main.prog
	.section	.rodata,"a",@progbits
	.p2align	4, 0x0
main.prog:
	.ascii	"\000\000\002\000\000\001\002\001\001\001\000\n\005\006\020\001\000\001\001\003\002\000\001\001\000\001\003\002\001\007\351\001\000\b"
	.size	main.prog, 34

	.ident	"clang version 20.1.8"
	.section	".note.GNU-stack","",@progbits
	.addrsig
	.addrsig_sym vm_run
	.addrsig_sym vm_run.table
	.addrsig_sym main.prog
