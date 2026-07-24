	.file	"demo.c"
	.text
	.globl	vm_run                          # -- Begin function vm_run
	.p2align	4
	.type	vm_run,@function
vm_run:                                 # @vm_run
# %bb.0:
	pushq	%rbp
	movq	%rsp, %rbp
	subq	$480, %rsp                      # imm = 0x1E0
	movq	%rdi, -8(%rbp)
	leaq	-528(%rbp), %rax
	movq	%rax, -536(%rbp)
	movq	-8(%rbp), %rax
	movq	%rax, -544(%rbp)
	movq	-544(%rbp), %rax
	movq	%rax, %rcx
	addq	$1, %rcx
	movq	%rcx, -544(%rbp)
	movzbl	(%rax), %eax
	movl	%eax, %ecx
	leaq	vm_run.table(%rip), %rax
	movq	(%rax,%rcx,8), %rax
	movq	%rax, -608(%rbp)                # 8-byte Spill
	jmp	.LBB0_7
.Ltmp0:                                 # Block address taken
.LBB0_1:                                #   in Loop: Header=BB0_7 Depth=1
	movq	-544(%rbp), %rax
	movq	%rax, %rcx
	addq	$1, %rcx
	movq	%rcx, -544(%rbp)
	movsbq	(%rax), %rcx
	movq	-536(%rbp), %rax
	movq	%rax, %rdx
	addq	$8, %rdx
	movq	%rdx, -536(%rbp)
	movq	%rcx, (%rax)
	movq	-544(%rbp), %rax
	movq	%rax, %rcx
	addq	$1, %rcx
	movq	%rcx, -544(%rbp)
	movzbl	(%rax), %eax
	movl	%eax, %ecx
	leaq	vm_run.table(%rip), %rax
	movq	(%rax,%rcx,8), %rax
	movq	%rax, -608(%rbp)                # 8-byte Spill
	jmp	.LBB0_7
.Ltmp1:                                 # Block address taken
.LBB0_2:                                #   in Loop: Header=BB0_7 Depth=1
	movq	-536(%rbp), %rax
	movq	%rax, %rcx
	addq	$-8, %rcx
	movq	%rcx, -536(%rbp)
	movq	-8(%rax), %rax
	movq	%rax, -552(%rbp)
	movq	-536(%rbp), %rax
	movq	%rax, %rcx
	addq	$-8, %rcx
	movq	%rcx, -536(%rbp)
	movq	-8(%rax), %rax
	movq	%rax, -560(%rbp)
	movq	-560(%rbp), %rcx
	addq	-552(%rbp), %rcx
	movq	-536(%rbp), %rax
	movq	%rax, %rdx
	addq	$8, %rdx
	movq	%rdx, -536(%rbp)
	movq	%rcx, (%rax)
	movq	-544(%rbp), %rax
	movq	%rax, %rcx
	addq	$1, %rcx
	movq	%rcx, -544(%rbp)
	movzbl	(%rax), %eax
	movl	%eax, %ecx
	leaq	vm_run.table(%rip), %rax
	movq	(%rax,%rcx,8), %rax
	movq	%rax, -608(%rbp)                # 8-byte Spill
	jmp	.LBB0_7
.Ltmp2:                                 # Block address taken
.LBB0_3:                                #   in Loop: Header=BB0_7 Depth=1
	movq	-536(%rbp), %rax
	movq	%rax, %rcx
	addq	$-8, %rcx
	movq	%rcx, -536(%rbp)
	movq	-8(%rax), %rax
	movq	%rax, -568(%rbp)
	movq	-536(%rbp), %rax
	movq	%rax, %rcx
	addq	$-8, %rcx
	movq	%rcx, -536(%rbp)
	movq	-8(%rax), %rax
	movq	%rax, -576(%rbp)
	movq	-576(%rbp), %rcx
	subq	-568(%rbp), %rcx
	movq	-536(%rbp), %rax
	movq	%rax, %rdx
	addq	$8, %rdx
	movq	%rdx, -536(%rbp)
	movq	%rcx, (%rax)
	movq	-544(%rbp), %rax
	movq	%rax, %rcx
	addq	$1, %rcx
	movq	%rcx, -544(%rbp)
	movzbl	(%rax), %eax
	movl	%eax, %ecx
	leaq	vm_run.table(%rip), %rax
	movq	(%rax,%rcx,8), %rax
	movq	%rax, -608(%rbp)                # 8-byte Spill
	jmp	.LBB0_7
.Ltmp3:                                 # Block address taken
.LBB0_4:                                #   in Loop: Header=BB0_7 Depth=1
	movq	-536(%rbp), %rax
	movq	%rax, %rcx
	addq	$-8, %rcx
	movq	%rcx, -536(%rbp)
	movq	-8(%rax), %rax
	movq	%rax, -584(%rbp)
	movq	-536(%rbp), %rax
	movq	%rax, %rcx
	addq	$-8, %rcx
	movq	%rcx, -536(%rbp)
	movq	-8(%rax), %rax
	movq	%rax, -592(%rbp)
	movq	-592(%rbp), %rcx
	imulq	-584(%rbp), %rcx
	movq	-536(%rbp), %rax
	movq	%rax, %rdx
	addq	$8, %rdx
	movq	%rdx, -536(%rbp)
	movq	%rcx, (%rax)
	movq	-544(%rbp), %rax
	movq	%rax, %rcx
	addq	$1, %rcx
	movq	%rcx, -544(%rbp)
	movzbl	(%rax), %eax
	movl	%eax, %ecx
	leaq	vm_run.table(%rip), %rax
	movq	(%rax,%rcx,8), %rax
	movq	%rax, -608(%rbp)                # 8-byte Spill
	jmp	.LBB0_7
.Ltmp4:                                 # Block address taken
.LBB0_5:                                #   in Loop: Header=BB0_7 Depth=1
	movq	-536(%rbp), %rax
	movq	%rax, %rcx
	addq	$-8, %rcx
	movq	%rcx, -536(%rbp)
	movq	-8(%rax), %rax
	movq	%rax, -600(%rbp)
	xorl	%eax, %eax
	movl	%eax, %ecx
	subq	-600(%rbp), %rcx
	movq	-536(%rbp), %rax
	movq	%rax, %rdx
	addq	$8, %rdx
	movq	%rdx, -536(%rbp)
	movq	%rcx, (%rax)
	movq	-544(%rbp), %rax
	movq	%rax, %rcx
	addq	$1, %rcx
	movq	%rcx, -544(%rbp)
	movzbl	(%rax), %eax
	movl	%eax, %ecx
	leaq	vm_run.table(%rip), %rax
	movq	(%rax,%rcx,8), %rax
	movq	%rax, -608(%rbp)                # 8-byte Spill
	jmp	.LBB0_7
.Ltmp5:                                 # Block address taken
.LBB0_6:
	movq	-536(%rbp), %rax
	movq	-8(%rax), %rax
	addq	$480, %rsp                      # imm = 0x1E0
	popq	%rbp
	retq
.LBB0_7:                                # =>This Inner Loop Header: Depth=1
	movq	-608(%rbp), %rax                # 8-byte Reload
	jmpq	*%rax
.Lfunc_end0:
	.size	vm_run, .Lfunc_end0-vm_run
                                        # -- End function
	.globl	demo_run                        # -- Begin function demo_run
	.p2align	4
	.type	demo_run,@function
demo_run:                               # @demo_run
# %bb.0:
	pushq	%rbp
	movq	%rsp, %rbp
	leaq	demo_run.program(%rip), %rdi
	callq	vm_run
                                        # kill: def $eax killed $eax killed $rax
	popq	%rbp
	retq
.Lfunc_end1:
	.size	demo_run, .Lfunc_end1-demo_run
                                        # -- End function
	.type	vm_run.table,@object            # @vm_run.table
	.section	.data.rel.ro,"aw",@progbits
	.p2align	4, 0x0
vm_run.table:
	.quad	.Ltmp0
	.quad	.Ltmp1
	.quad	.Ltmp2
	.quad	.Ltmp3
	.quad	.Ltmp4
	.quad	.Ltmp5
	.size	vm_run.table, 48

	.type	demo_run.program,@object        # @demo_run.program
	.section	.rodata,"a",@progbits
demo_run.program:
	.ascii	"\000\002\000\003\001\000\004\003\000\001\002\005"
	.size	demo_run.program, 12

	.ident	"clang version 20.1.8"
	.section	".note.GNU-stack","",@progbits
	.addrsig
	.addrsig_sym vm_run
	.addrsig_sym vm_run.table
	.addrsig_sym demo_run.program
