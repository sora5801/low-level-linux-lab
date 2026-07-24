	.file	"demo.c"
	.text
	.globl	vm_run                          # -- Begin function vm_run
	.p2align	4
	.type	vm_run,@function
vm_run:                                 # @vm_run
# %bb.0:
	pushq	%rbp
	movq	%rsp, %rbp
	subq	$384, %rsp                      # imm = 0x180
	leaq	1(%rdi), %rcx
	leaq	-512(%rbp), %rdx
	movzbl	(%rdi), %esi
	leaq	vm_run.table(%rip), %rax
	jmpq	*(%rax,%rsi,8)
	.p2align	4
.Ltmp0:                                 # Block address taken
.LBB0_1:                                # =>This Inner Loop Header: Depth=1
	leaq	1(%rcx), %rsi
	movsbq	(%rcx), %rdi
	movq	%rdi, (%rdx)
	addq	$8, %rdx
	addq	$2, %rcx
	movzbl	(%rsi), %esi
	jmpq	*(%rax,%rsi,8)
	.p2align	4
.Ltmp1:                                 # Block address taken
.LBB0_2:                                # =>This Inner Loop Header: Depth=1
	movq	-8(%rdx), %rsi
	addq	%rsi, -16(%rdx)
	addq	$-8, %rdx
	movzbl	(%rcx), %esi
	incq	%rcx
	jmpq	*(%rax,%rsi,8)
	.p2align	4
.Ltmp2:                                 # Block address taken
.LBB0_3:                                # =>This Inner Loop Header: Depth=1
	movq	-8(%rdx), %rsi
	subq	%rsi, -16(%rdx)
	addq	$-8, %rdx
	movzbl	(%rcx), %esi
	incq	%rcx
	jmpq	*(%rax,%rsi,8)
	.p2align	4
.Ltmp3:                                 # Block address taken
.LBB0_4:                                # =>This Inner Loop Header: Depth=1
	movq	-16(%rdx), %rsi
	imulq	-8(%rdx), %rsi
	movq	%rsi, -16(%rdx)
	addq	$-8, %rdx
	movzbl	(%rcx), %esi
	incq	%rcx
	jmpq	*(%rax,%rsi,8)
	.p2align	4
.Ltmp4:                                 # Block address taken
.LBB0_5:                                # =>This Inner Loop Header: Depth=1
	negq	-8(%rdx)
	movzbl	(%rcx), %esi
	incq	%rcx
	jmpq	*(%rax,%rsi,8)
.Ltmp5:                                 # Block address taken
.LBB0_6:
	movq	-8(%rdx), %rax
	addq	$384, %rsp                      # imm = 0x180
	popq	%rbp
	retq
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
	popq	%rbp
	jmp	vm_run                          # TAILCALL
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
	.addrsig_sym demo_run.program
