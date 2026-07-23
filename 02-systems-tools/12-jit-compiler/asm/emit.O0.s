	.file	"emit.c"
	.text
	.globl	emit_u8                         # -- Begin function emit_u8
	.p2align	4
	.type	emit_u8,@function
emit_u8:                                # @emit_u8
# %bb.0:
	pushq	%rbp
	movq	%rsp, %rbp
	movb	%sil, %al
	movq	%rdi, -8(%rbp)
	movb	%al, -9(%rbp)
	movq	-8(%rbp), %rax
	movq	8(%rax), %rax
	movq	-8(%rbp), %rcx
	cmpq	16(%rcx), %rax
	jb	.LBB0_2
# %bb.1:
	movq	-8(%rbp), %rax
	movl	$1, 24(%rax)
	jmp	.LBB0_3
.LBB0_2:
	movb	-9(%rbp), %dl
	movq	-8(%rbp), %rax
	movq	(%rax), %rax
	movq	-8(%rbp), %rsi
	movq	8(%rsi), %rcx
	movq	%rcx, %rdi
	addq	$1, %rdi
	movq	%rdi, 8(%rsi)
	movb	%dl, (%rax,%rcx)
.LBB0_3:
	popq	%rbp
	retq
.Lfunc_end0:
	.size	emit_u8, .Lfunc_end0-emit_u8
                                        # -- End function
	.globl	emit_u32                        # -- Begin function emit_u32
	.p2align	4
	.type	emit_u32,@function
emit_u32:                               # @emit_u32
# %bb.0:
	pushq	%rbp
	movq	%rsp, %rbp
	subq	$16, %rsp
	movq	%rdi, -8(%rbp)
	movl	%esi, -12(%rbp)
	movq	-8(%rbp), %rdi
	movl	-12(%rbp), %eax
	andl	$255, %eax
                                        # kill: def $al killed $al killed $eax
	movzbl	%al, %esi
	callq	emit_u8
	movq	-8(%rbp), %rdi
	movl	-12(%rbp), %eax
	shrl	$8, %eax
	andl	$255, %eax
                                        # kill: def $al killed $al killed $eax
	movzbl	%al, %esi
	callq	emit_u8
	movq	-8(%rbp), %rdi
	movl	-12(%rbp), %eax
	shrl	$16, %eax
	andl	$255, %eax
                                        # kill: def $al killed $al killed $eax
	movzbl	%al, %esi
	callq	emit_u8
	movq	-8(%rbp), %rdi
	movl	-12(%rbp), %eax
	shrl	$24, %eax
	andl	$255, %eax
                                        # kill: def $al killed $al killed $eax
	movzbl	%al, %esi
	callq	emit_u8
	addq	$16, %rsp
	popq	%rbp
	retq
.Lfunc_end1:
	.size	emit_u32, .Lfunc_end1-emit_u32
                                        # -- End function
	.globl	modrm                           # -- Begin function modrm
	.p2align	4
	.type	modrm,@function
modrm:                                  # @modrm
# %bb.0:
	pushq	%rbp
	movq	%rsp, %rbp
	movb	%dl, %al
	movb	%sil, %cl
	movb	%dil, %dl
	movb	%dl, -1(%rbp)
	movb	%cl, -2(%rbp)
	movb	%al, -3(%rbp)
	movzbl	-1(%rbp), %eax
	andl	$3, %eax
	shll	$6, %eax
	movzbl	-2(%rbp), %ecx
	andl	$7, %ecx
	shll	$3, %ecx
	orl	%ecx, %eax
	movzbl	-3(%rbp), %ecx
	andl	$7, %ecx
	orl	%ecx, %eax
                                        # kill: def $al killed $al killed $eax
	popq	%rbp
	retq
.Lfunc_end2:
	.size	modrm, .Lfunc_end2-modrm
                                        # -- End function
	.globl	emit_prologue                   # -- Begin function emit_prologue
	.p2align	4
	.type	emit_prologue,@function
emit_prologue:                          # @emit_prologue
# %bb.0:
	pushq	%rbp
	movq	%rsp, %rbp
	subq	$16, %rsp
	movq	%rdi, -8(%rbp)
	movq	-8(%rbp), %rdi
	movl	$85, %esi
	callq	emit_u8
	movq	-8(%rbp), %rdi
	movl	$72, %esi
	callq	emit_u8
	movq	-8(%rbp), %rdi
	movl	$137, %esi
	callq	emit_u8
	movq	-8(%rbp), %rdi
	movl	$229, %esi
	callq	emit_u8
	movq	-8(%rbp), %rdi
	movl	$83, %esi
	callq	emit_u8
	movq	-8(%rbp), %rdi
	movl	$65, %esi
	callq	emit_u8
	movq	-8(%rbp), %rdi
	movl	$86, %esi
	callq	emit_u8
	movq	-8(%rbp), %rdi
	movl	$65, %esi
	callq	emit_u8
	movq	-8(%rbp), %rdi
	movl	$87, %esi
	callq	emit_u8
	movq	-8(%rbp), %rdi
	movl	$72, %esi
	callq	emit_u8
	movq	-8(%rbp), %rdi
	movl	$131, %esi
	callq	emit_u8
	movq	-8(%rbp), %rdi
	movl	$236, %esi
	callq	emit_u8
	movq	-8(%rbp), %rdi
	movl	$8, %esi
	callq	emit_u8
	movq	-8(%rbp), %rdi
	movl	$72, %esi
	callq	emit_u8
	movq	-8(%rbp), %rdi
	movl	$137, %esi
	callq	emit_u8
	movq	-8(%rbp), %rdi
	movl	$251, %esi
	callq	emit_u8
	movq	-8(%rbp), %rdi
	movl	$73, %esi
	callq	emit_u8
	movq	-8(%rbp), %rdi
	movl	$137, %esi
	callq	emit_u8
	movq	-8(%rbp), %rdi
	movl	$246, %esi
	callq	emit_u8
	movq	-8(%rbp), %rdi
	movl	$73, %esi
	callq	emit_u8
	movq	-8(%rbp), %rdi
	movl	$137, %esi
	callq	emit_u8
	movq	-8(%rbp), %rdi
	movl	$215, %esi
	callq	emit_u8
	addq	$16, %rsp
	popq	%rbp
	retq
.Lfunc_end3:
	.size	emit_prologue, .Lfunc_end3-emit_prologue
                                        # -- End function
	.globl	emit_epilogue                   # -- Begin function emit_epilogue
	.p2align	4
	.type	emit_epilogue,@function
emit_epilogue:                          # @emit_epilogue
# %bb.0:
	pushq	%rbp
	movq	%rsp, %rbp
	subq	$16, %rsp
	movq	%rdi, -8(%rbp)
	movq	-8(%rbp), %rdi
	movl	$72, %esi
	callq	emit_u8
	movq	-8(%rbp), %rdi
	movl	$131, %esi
	callq	emit_u8
	movq	-8(%rbp), %rdi
	movl	$196, %esi
	callq	emit_u8
	movq	-8(%rbp), %rdi
	movl	$8, %esi
	callq	emit_u8
	movq	-8(%rbp), %rdi
	movl	$65, %esi
	callq	emit_u8
	movq	-8(%rbp), %rdi
	movl	$95, %esi
	callq	emit_u8
	movq	-8(%rbp), %rdi
	movl	$65, %esi
	callq	emit_u8
	movq	-8(%rbp), %rdi
	movl	$94, %esi
	callq	emit_u8
	movq	-8(%rbp), %rdi
	movl	$91, %esi
	callq	emit_u8
	movq	-8(%rbp), %rdi
	movl	$93, %esi
	callq	emit_u8
	movq	-8(%rbp), %rdi
	movl	$195, %esi
	callq	emit_u8
	addq	$16, %rsp
	popq	%rbp
	retq
.Lfunc_end4:
	.size	emit_epilogue, .Lfunc_end4-emit_epilogue
                                        # -- End function
	.globl	emit_add_ptr                    # -- Begin function emit_add_ptr
	.p2align	4
	.type	emit_add_ptr,@function
emit_add_ptr:                           # @emit_add_ptr
# %bb.0:
	pushq	%rbp
	movq	%rsp, %rbp
	subq	$32, %rsp
	movq	%rdi, -8(%rbp)
	movl	%esi, -12(%rbp)
	movq	-8(%rbp), %rdi
	movl	$72, %esi
	callq	emit_u8
	movq	-8(%rbp), %rdi
	movl	$129, %esi
	callq	emit_u8
	movq	-8(%rbp), %rax
	movq	%rax, -24(%rbp)                 # 8-byte Spill
	movl	$3, %edx
	xorl	%esi, %esi
	movl	%edx, %edi
	callq	modrm
	movq	-24(%rbp), %rdi                 # 8-byte Reload
	movzbl	%al, %esi
	callq	emit_u8
	movq	-8(%rbp), %rdi
	movl	-12(%rbp), %esi
	callq	emit_u32
	addq	$32, %rsp
	popq	%rbp
	retq
.Lfunc_end5:
	.size	emit_add_ptr, .Lfunc_end5-emit_add_ptr
                                        # -- End function
	.globl	emit_add_cell                   # -- Begin function emit_add_cell
	.p2align	4
	.type	emit_add_cell,@function
emit_add_cell:                          # @emit_add_cell
# %bb.0:
	pushq	%rbp
	movq	%rsp, %rbp
	subq	$32, %rsp
	movb	%sil, %al
	movq	%rdi, -8(%rbp)
	movb	%al, -9(%rbp)
	movq	-8(%rbp), %rdi
	movl	$128, %esi
	callq	emit_u8
	movq	-8(%rbp), %rax
	movq	%rax, -24(%rbp)                 # 8-byte Spill
	xorl	%esi, %esi
	movl	$3, %edx
	movl	%esi, %edi
	callq	modrm
	movq	-24(%rbp), %rdi                 # 8-byte Reload
	movzbl	%al, %esi
	callq	emit_u8
	movq	-8(%rbp), %rdi
	movzbl	-9(%rbp), %esi
	callq	emit_u8
	addq	$32, %rsp
	popq	%rbp
	retq
.Lfunc_end6:
	.size	emit_add_cell, .Lfunc_end6-emit_add_cell
                                        # -- End function
	.globl	emit_set_cell                   # -- Begin function emit_set_cell
	.p2align	4
	.type	emit_set_cell,@function
emit_set_cell:                          # @emit_set_cell
# %bb.0:
	pushq	%rbp
	movq	%rsp, %rbp
	subq	$32, %rsp
	movb	%sil, %al
	movq	%rdi, -8(%rbp)
	movb	%al, -9(%rbp)
	movq	-8(%rbp), %rdi
	movl	$198, %esi
	callq	emit_u8
	movq	-8(%rbp), %rax
	movq	%rax, -24(%rbp)                 # 8-byte Spill
	xorl	%esi, %esi
	movl	$3, %edx
	movl	%esi, %edi
	callq	modrm
	movq	-24(%rbp), %rdi                 # 8-byte Reload
	movzbl	%al, %esi
	callq	emit_u8
	movq	-8(%rbp), %rdi
	movzbl	-9(%rbp), %esi
	callq	emit_u8
	addq	$32, %rsp
	popq	%rbp
	retq
.Lfunc_end7:
	.size	emit_set_cell, .Lfunc_end7-emit_set_cell
                                        # -- End function
	.globl	emit_output                     # -- Begin function emit_output
	.p2align	4
	.type	emit_output,@function
emit_output:                            # @emit_output
# %bb.0:
	pushq	%rbp
	movq	%rsp, %rbp
	subq	$16, %rsp
	movq	%rdi, -8(%rbp)
	movq	-8(%rbp), %rdi
	movl	$15, %esi
	callq	emit_u8
	movq	-8(%rbp), %rdi
	movl	$182, %esi
	callq	emit_u8
	movq	-8(%rbp), %rdi
	movl	$59, %esi
	callq	emit_u8
	movq	-8(%rbp), %rdi
	movl	$65, %esi
	callq	emit_u8
	movq	-8(%rbp), %rdi
	movl	$255, %esi
	callq	emit_u8
	movq	-8(%rbp), %rdi
	movl	$215, %esi
	callq	emit_u8
	addq	$16, %rsp
	popq	%rbp
	retq
.Lfunc_end8:
	.size	emit_output, .Lfunc_end8-emit_output
                                        # -- End function
	.globl	emit_input                      # -- Begin function emit_input
	.p2align	4
	.type	emit_input,@function
emit_input:                             # @emit_input
# %bb.0:
	pushq	%rbp
	movq	%rsp, %rbp
	subq	$16, %rsp
	movq	%rdi, -8(%rbp)
	movq	-8(%rbp), %rdi
	movl	$65, %esi
	callq	emit_u8
	movq	-8(%rbp), %rdi
	movl	$255, %esi
	callq	emit_u8
	movq	-8(%rbp), %rdi
	movl	$214, %esi
	callq	emit_u8
	movq	-8(%rbp), %rdi
	movl	$136, %esi
	callq	emit_u8
	movq	-8(%rbp), %rdi
	movl	$3, %esi
	callq	emit_u8
	addq	$16, %rsp
	popq	%rbp
	retq
.Lfunc_end9:
	.size	emit_input, .Lfunc_end9-emit_input
                                        # -- End function
	.globl	emit_loop_open                  # -- Begin function emit_loop_open
	.p2align	4
	.type	emit_loop_open,@function
emit_loop_open:                         # @emit_loop_open
# %bb.0:
	pushq	%rbp
	movq	%rsp, %rbp
	subq	$16, %rsp
	movq	%rdi, -8(%rbp)
	movq	-8(%rbp), %rdi
	movl	$128, %esi
	callq	emit_u8
	movq	-8(%rbp), %rdi
	movl	$59, %esi
	callq	emit_u8
	movq	-8(%rbp), %rdi
	xorl	%esi, %esi
	callq	emit_u8
	movq	-8(%rbp), %rdi
	movl	$15, %esi
	callq	emit_u8
	movq	-8(%rbp), %rdi
	movl	$132, %esi
	callq	emit_u8
	movq	-8(%rbp), %rax
	movq	8(%rax), %rax
	movq	%rax, -16(%rbp)
	movq	-8(%rbp), %rdi
	xorl	%esi, %esi
	callq	emit_u32
	movq	-16(%rbp), %rax
	addq	$16, %rsp
	popq	%rbp
	retq
.Lfunc_end10:
	.size	emit_loop_open, .Lfunc_end10-emit_loop_open
                                        # -- End function
	.globl	emit_loop_close                 # -- Begin function emit_loop_close
	.p2align	4
	.type	emit_loop_close,@function
emit_loop_close:                        # @emit_loop_close
# %bb.0:
	pushq	%rbp
	movq	%rsp, %rbp
	subq	$16, %rsp
	movq	%rdi, -8(%rbp)
	movq	-8(%rbp), %rdi
	movl	$128, %esi
	callq	emit_u8
	movq	-8(%rbp), %rdi
	movl	$59, %esi
	callq	emit_u8
	movq	-8(%rbp), %rdi
	xorl	%esi, %esi
	callq	emit_u8
	movq	-8(%rbp), %rdi
	movl	$15, %esi
	callq	emit_u8
	movq	-8(%rbp), %rdi
	movl	$133, %esi
	callq	emit_u8
	movq	-8(%rbp), %rax
	movq	8(%rax), %rax
	movq	%rax, -16(%rbp)
	movq	-8(%rbp), %rdi
	xorl	%esi, %esi
	callq	emit_u32
	movq	-16(%rbp), %rax
	addq	$16, %rsp
	popq	%rbp
	retq
.Lfunc_end11:
	.size	emit_loop_close, .Lfunc_end11-emit_loop_close
                                        # -- End function
	.globl	patch_rel32                     # -- Begin function patch_rel32
	.p2align	4
	.type	patch_rel32,@function
patch_rel32:                            # @patch_rel32
# %bb.0:
	pushq	%rbp
	movq	%rsp, %rbp
	movq	%rdi, -8(%rbp)
	movq	%rsi, -16(%rbp)
	movq	%rdx, -24(%rbp)
	movq	-16(%rbp), %rax
	addq	$4, %rax
	movq	-8(%rbp), %rcx
	cmpq	16(%rcx), %rax
	jbe	.LBB12_2
# %bb.1:
	movq	-8(%rbp), %rax
	movl	$1, 24(%rax)
	jmp	.LBB12_3
.LBB12_2:
	movq	-24(%rbp), %rax
                                        # kill: def $eax killed $eax killed $rax
	movq	-16(%rbp), %rcx
	addq	$4, %rcx
                                        # kill: def $ecx killed $ecx killed $rcx
	subl	%ecx, %eax
	movl	%eax, -28(%rbp)
	movl	-28(%rbp), %eax
	movl	%eax, -32(%rbp)
	movl	-32(%rbp), %eax
	andl	$255, %eax
	movb	%al, %dl
	movq	-8(%rbp), %rax
	movq	(%rax), %rax
	movq	-16(%rbp), %rcx
	movb	%dl, (%rax,%rcx)
	movl	-32(%rbp), %eax
	shrl	$8, %eax
	andl	$255, %eax
	movb	%al, %dl
	movq	-8(%rbp), %rax
	movq	(%rax), %rax
	movq	-16(%rbp), %rcx
	movb	%dl, 1(%rax,%rcx)
	movl	-32(%rbp), %eax
	shrl	$16, %eax
	andl	$255, %eax
	movb	%al, %dl
	movq	-8(%rbp), %rax
	movq	(%rax), %rax
	movq	-16(%rbp), %rcx
	movb	%dl, 2(%rax,%rcx)
	movl	-32(%rbp), %eax
	shrl	$24, %eax
	andl	$255, %eax
	movb	%al, %dl
	movq	-8(%rbp), %rax
	movq	(%rax), %rax
	movq	-16(%rbp), %rcx
	movb	%dl, 3(%rax,%rcx)
.LBB12_3:
	popq	%rbp
	retq
.Lfunc_end12:
	.size	patch_rel32, .Lfunc_end12-patch_rel32
                                        # -- End function
	.ident	"clang version 20.1.8"
	.section	".note.GNU-stack","",@progbits
	.addrsig
	.addrsig_sym emit_u8
	.addrsig_sym emit_u32
	.addrsig_sym modrm
