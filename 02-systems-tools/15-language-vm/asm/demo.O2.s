	.file	"demo.c"
	.text
	.globl	vm_run                          # -- Begin function vm_run
	.p2align	4
	.type	vm_run,@function
vm_run:                                 # @vm_run
# %bb.0:
	subq	$392, %rsp                      # imm = 0x188
	leaq	1(%rdi), %rax
	leaq	-128(%rsp), %rsi
	leaq	-8(%rsi), %rdx
	movzbl	(%rdi), %edi
	leaq	vm_run.table(%rip), %rcx
	jmpq	*(%rcx,%rdi,8)
	.p2align	4
.Ltmp0:                                 # Block address taken
.LBB0_1:                                # =>This Inner Loop Header: Depth=1
	leaq	1(%rax), %rdi
	movsbq	(%rax), %rdx
	movq	%rdx, (%rsi)
	addq	$8, %rsi
	addq	$2, %rax
	leaq	-8(%rsi), %rdx
	movzbl	(%rdi), %edi
	jmpq	*(%rcx,%rdi,8)
	.p2align	4
.Ltmp1:                                 # Block address taken
.LBB0_2:                                # =>This Inner Loop Header: Depth=1
	movq	-8(%rsi), %rdi
	addq	%rdi, -16(%rsi)
	movq	%rdx, %rsi
	addq	$-8, %rdx
	movzbl	(%rax), %edi
	addq	$1, %rax
	jmpq	*(%rcx,%rdi,8)
	.p2align	4
.Ltmp2:                                 # Block address taken
.LBB0_3:                                # =>This Inner Loop Header: Depth=1
	movq	-8(%rsi), %rdi
	subq	%rdi, -16(%rsi)
	movq	%rdx, %rsi
	addq	$-8, %rdx
	movzbl	(%rax), %edi
	addq	$1, %rax
	jmpq	*(%rcx,%rdi,8)
	.p2align	4
.Ltmp3:                                 # Block address taken
.LBB0_4:                                # =>This Inner Loop Header: Depth=1
	movq	-16(%rsi), %rdi
	imulq	-8(%rsi), %rdi
	movq	%rdi, -16(%rsi)
	movq	%rdx, %rsi
	addq	$-8, %rdx
	movzbl	(%rax), %edi
	addq	$1, %rax
	jmpq	*(%rcx,%rdi,8)
	.p2align	4
.Ltmp4:                                 # Block address taken
.LBB0_5:                                # =>This Inner Loop Header: Depth=1
	negq	(%rdx)
	movzbl	(%rax), %edi
	incq	%rax
	jmpq	*(%rcx,%rdi,8)
.Ltmp5:                                 # Block address taken
.LBB0_6:
	movq	(%rdx), %rax
	addq	$392, %rsp                      # imm = 0x188
	retq
.Lfunc_end0:
	.size	vm_run, .Lfunc_end0-vm_run
                                        # -- End function
	.globl	demo_run                        # -- Begin function demo_run
	.p2align	4
	.type	demo_run,@function
demo_run:                               # @demo_run
# %bb.0:
	leaq	demo_run.program(%rip), %rdi
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
