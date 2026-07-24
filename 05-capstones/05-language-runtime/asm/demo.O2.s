	.file	"demo.c"
	.text
	.globl	vm_run                          # -- Begin function vm_run
	.p2align	4
	.type	vm_run,@function
vm_run:                                 # @vm_run
# %bb.0:
	subq	$200, %rsp
	xorps	%xmm0, %xmm0
	movaps	%xmm0, -80(%rsp)
	movaps	%xmm0, -96(%rsp)
	movaps	%xmm0, -112(%rsp)
	movaps	%xmm0, -128(%rsp)
	xorl	%eax, %eax
	movl	$1, %edx
	movzbl	(%rdi), %esi
	leaq	vm_run.table(%rip), %rcx
	jmpq	*(%rcx,%rsi,8)
	.p2align	4
.Ltmp0:                                 # Block address taken
.LBB0_1:                                # =>This Inner Loop Header: Depth=1
	movslq	%edx, %rsi
	movzbl	(%rdi,%rsi), %edx
	movslq	%eax, %r8
	incl	%eax
	movq	%rdx, -64(%rsp,%r8,8)
	leal	2(%rsi), %edx
	addq	%rdi, %rsi
	incq	%rsi
	movzbl	(%rsi), %esi
	jmpq	*(%rcx,%rsi,8)
	.p2align	4
.Ltmp1:                                 # Block address taken
.LBB0_2:                                # =>This Inner Loop Header: Depth=1
	movslq	%edx, %rsi
	movzbl	(%rdi,%rsi), %edx
	movq	-128(%rsp,%rdx,8), %rdx
	movslq	%eax, %r8
	incl	%eax
	movq	%rdx, -64(%rsp,%r8,8)
	leal	2(%rsi), %edx
	addq	%rdi, %rsi
	incq	%rsi
	movzbl	(%rsi), %esi
	jmpq	*(%rcx,%rsi,8)
	.p2align	4
.Ltmp2:                                 # Block address taken
.LBB0_3:                                # =>This Inner Loop Header: Depth=1
	movslq	%eax, %rsi
	decl	%eax
	movq	-72(%rsp,%rsi,8), %rsi
	movslq	%edx, %r8
	movzbl	(%rdi,%r8), %edx
	movq	%rsi, -128(%rsp,%rdx,8)
	leal	2(%r8), %edx
	leaq	(%rdi,%r8), %rsi
	incq	%rsi
	movzbl	(%rsi), %esi
	jmpq	*(%rcx,%rsi,8)
	.p2align	4
.Ltmp3:                                 # Block address taken
.LBB0_4:                                # =>This Inner Loop Header: Depth=1
	movslq	%eax, %rsi
	decl	%eax
	movq	-72(%rsp,%rsi,8), %r8
	addq	%r8, -80(%rsp,%rsi,8)
	movslq	%edx, %rsi
	incl	%edx
	addq	%rdi, %rsi
	movzbl	(%rsi), %esi
	jmpq	*(%rcx,%rsi,8)
	.p2align	4
.Ltmp4:                                 # Block address taken
.LBB0_5:                                # =>This Inner Loop Header: Depth=1
	movslq	%eax, %rsi
	decl	%eax
	movq	-72(%rsp,%rsi,8), %r8
	subq	%r8, -80(%rsp,%rsi,8)
	movslq	%edx, %rsi
	incl	%edx
	addq	%rdi, %rsi
	movzbl	(%rsi), %esi
	jmpq	*(%rcx,%rsi,8)
	.p2align	4
.Ltmp5:                                 # Block address taken
.LBB0_6:                                # =>This Inner Loop Header: Depth=1
	movslq	%eax, %rsi
	decl	%eax
	movq	-80(%rsp,%rsi,8), %r8
	xorl	%r9d, %r9d
	cmpq	-72(%rsp,%rsi,8), %r8
	setle	%r9b
	movq	%r9, -80(%rsp,%rsi,8)
	movslq	%edx, %rsi
	incl	%edx
	addq	%rdi, %rsi
	movzbl	(%rsi), %esi
	jmpq	*(%rcx,%rsi,8)
	.p2align	4
.Ltmp6:                                 # Block address taken
.LBB0_7:                                # =>This Inner Loop Header: Depth=1
	leal	1(%rdx), %esi
	movslq	%eax, %r8
	decl	%eax
	cmpq	$0, -72(%rsp,%r8,8)
	jne	.LBB0_9
# %bb.8:                                #   in Loop: Header=BB0_7 Depth=1
	movslq	%edx, %rdx
	movsbl	(%rdi,%rdx), %edx
	addl	%edx, %esi
.LBB0_9:                                #   in Loop: Header=BB0_7 Depth=1
	movslq	%esi, %rdx
	incl	%esi
	addq	%rdi, %rdx
	movzbl	(%rdx), %r8d
	movl	%esi, %edx
	jmpq	*(%rcx,%r8,8)
	.p2align	4
.Ltmp7:                                 # Block address taken
.LBB0_10:                               # =>This Inner Loop Header: Depth=1
	movslq	%edx, %rdx
	movsbq	(%rdi,%rdx), %rsi
	leaq	(%rdx,%rsi), %r8
	addq	%rsi, %rdx
	incq	%rdx
	incl	%edx
	leaq	(%rdi,%r8), %rsi
	incq	%rsi
	movzbl	(%rsi), %esi
	jmpq	*(%rcx,%rsi,8)
.Ltmp8:                                 # Block address taken
.LBB0_11:
	cltq
	movq	-72(%rsp,%rax,8), %rax
	addq	$200, %rsp
	retq
.Lfunc_end0:
	.size	vm_run, .Lfunc_end0-vm_run
                                        # -- End function
	.globl	main                            # -- Begin function main
	.p2align	4
	.type	main,@function
main:                                   # @main
# %bb.0:
	leaq	main.prog(%rip), %rdi
	jmp	vm_run                          # TAILCALL
.Lfunc_end1:
	.size	main, .Lfunc_end1-main
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
	.addrsig_sym main.prog
