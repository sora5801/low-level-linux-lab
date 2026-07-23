	.file	"loader.c"
                                        # Start of file scope inline assembly
	.text
	.globl	_start
	.type	_start,@function
_start:
	xorl	%ebp, %ebp
	movq	%rsp, %rdi
	andq	$-16, %rsp
	callq	loader_main
	hlt

                                        # End of file scope inline assembly
	.globl	memset                          # -- Begin function memset
	.p2align	4
	.type	memset,@function
memset:                                 # @memset
# %bb.0:
	pushq	%rbp
	movq	%rsp, %rbp
	movq	%rdi, -8(%rbp)
	movl	%esi, -12(%rbp)
	movq	%rdx, -24(%rbp)
	movq	-8(%rbp), %rax
	movq	%rax, -32(%rbp)
.LBB0_1:                                # =>This Inner Loop Header: Depth=1
	movq	-24(%rbp), %rax
	movq	%rax, %rcx
	addq	$-1, %rcx
	movq	%rcx, -24(%rbp)
	cmpq	$0, %rax
	je	.LBB0_3
# %bb.2:                                #   in Loop: Header=BB0_1 Depth=1
	movl	-12(%rbp), %eax
	movb	%al, %cl
	movq	-32(%rbp), %rax
	movq	%rax, %rdx
	addq	$1, %rdx
	movq	%rdx, -32(%rbp)
	movb	%cl, (%rax)
	jmp	.LBB0_1
.LBB0_3:
	movq	-8(%rbp), %rax
	popq	%rbp
	retq
.Lfunc_end0:
	.size	memset, .Lfunc_end0-memset
                                        # -- End function
	.globl	memcpy                          # -- Begin function memcpy
	.p2align	4
	.type	memcpy,@function
memcpy:                                 # @memcpy
# %bb.0:
	pushq	%rbp
	movq	%rsp, %rbp
	movq	%rdi, -8(%rbp)
	movq	%rsi, -16(%rbp)
	movq	%rdx, -24(%rbp)
	movq	-8(%rbp), %rax
	movq	%rax, -32(%rbp)
	movq	-16(%rbp), %rax
	movq	%rax, -40(%rbp)
.LBB1_1:                                # =>This Inner Loop Header: Depth=1
	movq	-24(%rbp), %rax
	movq	%rax, %rcx
	addq	$-1, %rcx
	movq	%rcx, -24(%rbp)
	cmpq	$0, %rax
	je	.LBB1_3
# %bb.2:                                #   in Loop: Header=BB1_1 Depth=1
	movq	-40(%rbp), %rax
	movq	%rax, %rcx
	addq	$1, %rcx
	movq	%rcx, -40(%rbp)
	movb	(%rax), %cl
	movq	-32(%rbp), %rax
	movq	%rax, %rdx
	addq	$1, %rdx
	movq	%rdx, -32(%rbp)
	movb	%cl, (%rax)
	jmp	.LBB1_1
.LBB1_3:
	movq	-8(%rbp), %rax
	popq	%rbp
	retq
.Lfunc_end1:
	.size	memcpy, .Lfunc_end1-memcpy
                                        # -- End function
	.globl	loader_main                     # -- Begin function loader_main
	.p2align	4
	.type	loader_main,@function
loader_main:                            # @loader_main
# %bb.0:
	pushq	%rbp
	movq	%rsp, %rbp
	subq	$192, %rsp
	movq	%rdi, -8(%rbp)
	movq	-8(%rbp), %rax
	movq	(%rax), %rax
	movq	%rax, -16(%rbp)
	movq	-8(%rbp), %rax
	addq	$8, %rax
	movq	%rax, -24(%rbp)
	movq	-24(%rbp), %rax
	movq	-16(%rbp), %rcx
	shlq	$3, %rcx
	addq	%rcx, %rax
	addq	$8, %rax
	movq	%rax, -32(%rbp)
	movq	-32(%rbp), %rax
	movq	%rax, -40(%rbp)
.LBB2_1:                                # =>This Inner Loop Header: Depth=1
	movq	-40(%rbp), %rax
	cmpq	$0, (%rax)
	je	.LBB2_3
# %bb.2:                                #   in Loop: Header=BB2_1 Depth=1
	movq	-40(%rbp), %rax
	addq	$8, %rax
	movq	%rax, -40(%rbp)
	jmp	.LBB2_1
.LBB2_3:
	movq	-40(%rbp), %rax
	addq	$8, %rax
	movq	%rax, -48(%rbp)
	movq	-32(%rbp), %rax
	movq	%rax, -56(%rbp)
.LBB2_4:                                # =>This Loop Header: Depth=1
                                        #     Child Loop BB2_14 Depth 2
	movq	-56(%rbp), %rax
	cmpq	$0, (%rax)
	je	.LBB2_23
# %bb.5:                                #   in Loop: Header=BB2_4 Depth=1
	movq	-56(%rbp), %rax
	movq	(%rax), %rax
	movq	%rax, -64(%rbp)
	movq	-64(%rbp), %rax
	movsbl	(%rax), %eax
	cmpl	$76, %eax
	jne	.LBB2_21
# %bb.6:                                #   in Loop: Header=BB2_4 Depth=1
	movq	-64(%rbp), %rax
	movsbl	1(%rax), %eax
	cmpl	$68, %eax
	jne	.LBB2_21
# %bb.7:                                #   in Loop: Header=BB2_4 Depth=1
	movq	-64(%rbp), %rax
	movsbl	2(%rax), %eax
	cmpl	$76, %eax
	jne	.LBB2_21
# %bb.8:                                #   in Loop: Header=BB2_4 Depth=1
	movq	-64(%rbp), %rax
	movsbl	3(%rax), %eax
	cmpl	$65, %eax
	jne	.LBB2_21
# %bb.9:                                #   in Loop: Header=BB2_4 Depth=1
	movq	-64(%rbp), %rax
	movsbl	4(%rax), %eax
	cmpl	$66, %eax
	jne	.LBB2_21
# %bb.10:                               #   in Loop: Header=BB2_4 Depth=1
	movq	-64(%rbp), %rax
	movsbl	5(%rax), %eax
	cmpl	$95, %eax
	jne	.LBB2_21
# %bb.11:                               #   in Loop: Header=BB2_4 Depth=1
	movq	-64(%rbp), %rdi
	leaq	.L.str(%rip), %rsi
	callq	kstreq
	cmpl	$0, %eax
	je	.LBB2_13
# %bb.12:                               #   in Loop: Header=BB2_4 Depth=1
	movl	$1, g_debug(%rip)
.LBB2_13:                               #   in Loop: Header=BB2_4 Depth=1
	leaq	.L.str.1(%rip), %rax
	movq	%rax, -72(%rbp)
	movq	-64(%rbp), %rax
	movq	%rax, -80(%rbp)
	movq	-72(%rbp), %rax
	movq	%rax, -88(%rbp)
.LBB2_14:                               #   Parent Loop BB2_4 Depth=1
                                        # =>  This Inner Loop Header: Depth=2
	movq	-88(%rbp), %rax
	movsbl	(%rax), %ecx
	xorl	%eax, %eax
                                        # kill: def $al killed $al killed $eax
	cmpl	$0, %ecx
	movb	%al, -173(%rbp)                 # 1-byte Spill
	je	.LBB2_16
# %bb.15:                               #   in Loop: Header=BB2_14 Depth=2
	movq	-80(%rbp), %rax
	movsbl	(%rax), %eax
	movq	-88(%rbp), %rcx
	movsbl	(%rcx), %ecx
	cmpl	%ecx, %eax
	sete	%al
	movb	%al, -173(%rbp)                 # 1-byte Spill
.LBB2_16:                               #   in Loop: Header=BB2_14 Depth=2
	movb	-173(%rbp), %al                 # 1-byte Reload
	testb	$1, %al
	jne	.LBB2_17
	jmp	.LBB2_18
.LBB2_17:                               #   in Loop: Header=BB2_14 Depth=2
	movq	-80(%rbp), %rax
	addq	$1, %rax
	movq	%rax, -80(%rbp)
	movq	-88(%rbp), %rax
	addq	$1, %rax
	movq	%rax, -88(%rbp)
	jmp	.LBB2_14
.LBB2_18:                               #   in Loop: Header=BB2_4 Depth=1
	movq	-88(%rbp), %rax
	movsbl	(%rax), %eax
	cmpl	$0, %eax
	jne	.LBB2_20
# %bb.19:                               #   in Loop: Header=BB2_4 Depth=1
	movq	-80(%rbp), %rax
	movq	%rax, g_env_libpath(%rip)
.LBB2_20:                               #   in Loop: Header=BB2_4 Depth=1
	jmp	.LBB2_21
.LBB2_21:                               #   in Loop: Header=BB2_4 Depth=1
	jmp	.LBB2_22
.LBB2_22:                               #   in Loop: Header=BB2_4 Depth=1
	movq	-56(%rbp), %rax
	addq	$8, %rax
	movq	%rax, -56(%rbp)
	jmp	.LBB2_4
.LBB2_23:
	cmpq	$2, -16(%rbp)
	jge	.LBB2_25
# %bb.24:
	leaq	.L.str.2(%rip), %rdi
	callq	die
.LBB2_25:
	movq	-24(%rbp), %rax
	movq	8(%rax), %rax
	movq	%rax, -96(%rbp)
	movq	-96(%rbp), %rdi
	callq	klast_slash
	movq	%rax, -104(%rbp)
	cmpq	$0, -104(%rbp)
	jle	.LBB2_34
# %bb.26:
	cmpq	$4095, -104(%rbp)               # imm = 0xFFF
	jae	.LBB2_28
# %bb.27:
	movq	-104(%rbp), %rax
	movq	%rax, -184(%rbp)                # 8-byte Spill
	jmp	.LBB2_29
.LBB2_28:
	movl	$4095, %eax                     # imm = 0xFFF
	movq	%rax, -184(%rbp)                # 8-byte Spill
	jmp	.LBB2_29
.LBB2_29:
	movq	-184(%rbp), %rax                # 8-byte Reload
	movq	%rax, -112(%rbp)
	movq	$0, -120(%rbp)
.LBB2_30:                               # =>This Inner Loop Header: Depth=1
	movq	-120(%rbp), %rax
	cmpq	-112(%rbp), %rax
	jae	.LBB2_33
# %bb.31:                               #   in Loop: Header=BB2_30 Depth=1
	movq	-96(%rbp), %rax
	movq	-120(%rbp), %rcx
	movb	(%rax,%rcx), %dl
	movq	-120(%rbp), %rcx
	leaq	g_prog_dir(%rip), %rax
	movb	%dl, (%rax,%rcx)
# %bb.32:                               #   in Loop: Header=BB2_30 Depth=1
	movq	-120(%rbp), %rax
	addq	$1, %rax
	movq	%rax, -120(%rbp)
	jmp	.LBB2_30
.LBB2_33:
	movq	-112(%rbp), %rcx
	leaq	g_prog_dir(%rip), %rax
	movb	$0, (%rax,%rcx)
.LBB2_34:
	leaq	.L.str.3(%rip), %rdi
	callq	trace
	movq	-96(%rbp), %rdi
	callq	load_path
	movq	%rax, -128(%rbp)
	movl	$0, -132(%rbp)
.LBB2_35:                               # =>This Loop Header: Depth=1
                                        #     Child Loop BB2_37 Depth 2
	movl	-132(%rbp), %eax
	cmpl	g_nobjs(%rip), %eax
	jge	.LBB2_44
# %bb.36:                               #   in Loop: Header=BB2_35 Depth=1
	movslq	-132(%rbp), %rcx
	leaq	g_objs(%rip), %rax
	imulq	$264, %rcx, %rcx                # imm = 0x108
	addq	%rcx, %rax
	movq	%rax, -144(%rbp)
	movl	$0, -148(%rbp)
.LBB2_37:                               #   Parent Loop BB2_35 Depth=1
                                        # =>  This Inner Loop Header: Depth=2
	movl	-148(%rbp), %eax
	movq	-144(%rbp), %rcx
	cmpl	248(%rcx), %eax
	jge	.LBB2_42
# %bb.38:                               #   in Loop: Header=BB2_37 Depth=2
	movq	-144(%rbp), %rax
	movq	56(%rax), %rax
	movq	-144(%rbp), %rcx
	movslq	-148(%rbp), %rdx
	movl	184(%rcx,%rdx,4), %ecx
                                        # kill: def $rcx killed $ecx
	addq	%rcx, %rax
	movq	%rax, -160(%rbp)
	cmpl	$0, g_debug(%rip)
	je	.LBB2_40
# %bb.39:                               #   in Loop: Header=BB2_37 Depth=2
	leaq	.L.str.4(%rip), %rdi
	callq	dstr
	movq	-160(%rbp), %rdi
	callq	dstr
	leaq	.L.str.5(%rip), %rdi
	callq	dstr
.LBB2_40:                               #   in Loop: Header=BB2_37 Depth=2
	movq	-144(%rbp), %rdi
	movq	-160(%rbp), %rsi
	callq	load_needed
# %bb.41:                               #   in Loop: Header=BB2_37 Depth=2
	movl	-148(%rbp), %eax
	addl	$1, %eax
	movl	%eax, -148(%rbp)
	jmp	.LBB2_37
.LBB2_42:                               #   in Loop: Header=BB2_35 Depth=1
	jmp	.LBB2_43
.LBB2_43:                               #   in Loop: Header=BB2_35 Depth=1
	movl	-132(%rbp), %eax
	addl	$1, %eax
	movl	%eax, -132(%rbp)
	jmp	.LBB2_35
.LBB2_44:
	movl	$0, -164(%rbp)
.LBB2_45:                               # =>This Inner Loop Header: Depth=1
	movl	-164(%rbp), %eax
	cmpl	g_nobjs(%rip), %eax
	jge	.LBB2_48
# %bb.46:                               #   in Loop: Header=BB2_45 Depth=1
	movslq	-164(%rbp), %rax
	leaq	g_objs(%rip), %rdi
	imulq	$264, %rax, %rax                # imm = 0x108
	addq	%rax, %rdi
	callq	relocate_object
# %bb.47:                               #   in Loop: Header=BB2_45 Depth=1
	movl	-164(%rbp), %eax
	addl	$1, %eax
	movl	%eax, -164(%rbp)
	jmp	.LBB2_45
.LBB2_48:
	movl	$0, -168(%rbp)
.LBB2_49:                               # =>This Inner Loop Header: Depth=1
	movl	-168(%rbp), %eax
	cmpl	g_nobjs(%rip), %eax
	jge	.LBB2_52
# %bb.50:                               #   in Loop: Header=BB2_49 Depth=1
	movslq	-168(%rbp), %rax
	leaq	g_objs(%rip), %rdi
	imulq	$264, %rax, %rax                # imm = 0x108
	addq	%rax, %rdi
	callq	apply_relro
# %bb.51:                               #   in Loop: Header=BB2_49 Depth=1
	movl	-168(%rbp), %eax
	addl	$1, %eax
	movl	%eax, -168(%rbp)
	jmp	.LBB2_49
.LBB2_52:
	movl	g_nobjs(%rip), %eax
	subl	$1, %eax
	movl	%eax, -172(%rbp)
.LBB2_53:                               # =>This Inner Loop Header: Depth=1
	cmpl	$0, -172(%rbp)
	jl	.LBB2_56
# %bb.54:                               #   in Loop: Header=BB2_53 Depth=1
	movslq	-172(%rbp), %rax
	leaq	g_objs(%rip), %rdi
	imulq	$264, %rax, %rax                # imm = 0x108
	addq	%rax, %rdi
	movq	-16(%rbp), %rax
	subq	$1, %rax
	movl	%eax, %esi
	movq	-24(%rbp), %rdx
	addq	$8, %rdx
	movq	-32(%rbp), %rcx
	callq	run_init
# %bb.55:                               #   in Loop: Header=BB2_53 Depth=1
	movl	-172(%rbp), %eax
	addl	$-1, %eax
	movl	%eax, -172(%rbp)
	jmp	.LBB2_53
.LBB2_56:
	movq	-128(%rbp), %rdi
	movq	-16(%rbp), %rax
	movl	%eax, %esi
	movq	-24(%rbp), %rdx
	movq	-32(%rbp), %rcx
	movq	-48(%rbp), %r8
	callq	handoff
.Lfunc_end2:
	.size	loader_main, .Lfunc_end2-loader_main
                                        # -- End function
	.p2align	4                               # -- Begin function kstreq
	.type	kstreq,@function
kstreq:                                 # @kstreq
# %bb.0:
	pushq	%rbp
	movq	%rsp, %rbp
	movq	%rdi, -8(%rbp)
	movq	%rsi, -16(%rbp)
.LBB3_1:                                # =>This Inner Loop Header: Depth=1
	movq	-8(%rbp), %rax
	movsbl	(%rax), %ecx
	xorl	%eax, %eax
                                        # kill: def $al killed $al killed $eax
	cmpl	$0, %ecx
	movb	%al, -17(%rbp)                  # 1-byte Spill
	je	.LBB3_3
# %bb.2:                                #   in Loop: Header=BB3_1 Depth=1
	movq	-8(%rbp), %rax
	movsbl	(%rax), %eax
	movq	-16(%rbp), %rcx
	movsbl	(%rcx), %ecx
	cmpl	%ecx, %eax
	sete	%al
	movb	%al, -17(%rbp)                  # 1-byte Spill
.LBB3_3:                                #   in Loop: Header=BB3_1 Depth=1
	movb	-17(%rbp), %al                  # 1-byte Reload
	testb	$1, %al
	jne	.LBB3_4
	jmp	.LBB3_5
.LBB3_4:                                #   in Loop: Header=BB3_1 Depth=1
	movq	-8(%rbp), %rax
	addq	$1, %rax
	movq	%rax, -8(%rbp)
	movq	-16(%rbp), %rax
	addq	$1, %rax
	movq	%rax, -16(%rbp)
	jmp	.LBB3_1
.LBB3_5:
	movq	-8(%rbp), %rax
	movsbl	(%rax), %eax
	movq	-16(%rbp), %rcx
	movsbl	(%rcx), %ecx
	cmpl	%ecx, %eax
	sete	%al
	andb	$1, %al
	movzbl	%al, %eax
	popq	%rbp
	retq
.Lfunc_end3:
	.size	kstreq, .Lfunc_end3-kstreq
                                        # -- End function
	.p2align	4                               # -- Begin function die
	.type	die,@function
die:                                    # @die
# %bb.0:
	pushq	%rbp
	movq	%rsp, %rbp
	subq	$16, %rsp
	movq	%rdi, -8(%rbp)
	leaq	.L.str.6(%rip), %rdi
	callq	dstr
	movq	-8(%rbp), %rdi
	callq	dstr
	leaq	.L.str.5(%rip), %rdi
	callq	dstr
	movl	$127, %edi
	callq	sys_exit
.Lfunc_end4:
	.size	die, .Lfunc_end4-die
                                        # -- End function
	.p2align	4                               # -- Begin function klast_slash
	.type	klast_slash,@function
klast_slash:                            # @klast_slash
# %bb.0:
	pushq	%rbp
	movq	%rsp, %rbp
	movq	%rdi, -8(%rbp)
	movq	$-1, -16(%rbp)
	movq	$0, -24(%rbp)
.LBB5_1:                                # =>This Inner Loop Header: Depth=1
	movq	-8(%rbp), %rax
	movq	-24(%rbp), %rcx
	cmpb	$0, (%rax,%rcx)
	je	.LBB5_6
# %bb.2:                                #   in Loop: Header=BB5_1 Depth=1
	movq	-8(%rbp), %rax
	movq	-24(%rbp), %rcx
	movsbl	(%rax,%rcx), %eax
	cmpl	$47, %eax
	jne	.LBB5_4
# %bb.3:                                #   in Loop: Header=BB5_1 Depth=1
	movq	-24(%rbp), %rax
	movq	%rax, -16(%rbp)
.LBB5_4:                                #   in Loop: Header=BB5_1 Depth=1
	jmp	.LBB5_5
.LBB5_5:                                #   in Loop: Header=BB5_1 Depth=1
	movq	-24(%rbp), %rax
	addq	$1, %rax
	movq	%rax, -24(%rbp)
	jmp	.LBB5_1
.LBB5_6:
	movq	-16(%rbp), %rax
	popq	%rbp
	retq
.Lfunc_end5:
	.size	klast_slash, .Lfunc_end5-klast_slash
                                        # -- End function
	.p2align	4                               # -- Begin function trace
	.type	trace,@function
trace:                                  # @trace
# %bb.0:
	pushq	%rbp
	movq	%rsp, %rbp
	subq	$16, %rsp
	movq	%rdi, -8(%rbp)
	cmpl	$0, g_debug(%rip)
	je	.LBB6_2
# %bb.1:
	leaq	.L.str.7(%rip), %rdi
	callq	dstr
	movq	-8(%rbp), %rdi
	callq	dstr
	leaq	.L.str.5(%rip), %rdi
	callq	dstr
.LBB6_2:
	addq	$16, %rsp
	popq	%rbp
	retq
.Lfunc_end6:
	.size	trace, .Lfunc_end6-trace
                                        # -- End function
	.p2align	4                               # -- Begin function load_path
	.type	load_path,@function
load_path:                              # @load_path
# %bb.0:
	pushq	%rbp
	movq	%rsp, %rbp
	subq	$32, %rsp
	movq	%rdi, -8(%rbp)
	movq	-8(%rbp), %rdi
	callq	trace
	movq	-8(%rbp), %rdi
	callq	sys_openat
	movq	%rax, -16(%rbp)
	cmpq	$0, -16(%rbp)
	jge	.LBB7_2
# %bb.1:
	xorl	%eax, %eax
	movl	%eax, %esi
	subq	-16(%rbp), %rsi
	leaq	.L.str.8(%rip), %rdi
	callq	die2
.LBB7_2:
	movq	-16(%rbp), %rax
	movl	%eax, %edi
	movq	-8(%rbp), %rsi
	callq	load_from_fd
	movq	%rax, -24(%rbp)
	movq	-16(%rbp), %rax
	movl	%eax, %edi
	callq	sys_close
	movq	-24(%rbp), %rax
	addq	$32, %rsp
	popq	%rbp
	retq
.Lfunc_end7:
	.size	load_path, .Lfunc_end7-load_path
                                        # -- End function
	.p2align	4                               # -- Begin function dstr
	.type	dstr,@function
dstr:                                   # @dstr
# %bb.0:
	pushq	%rbp
	movq	%rsp, %rbp
	subq	$16, %rsp
	movq	%rdi, -8(%rbp)
	movq	-8(%rbp), %rax
	movq	%rax, -16(%rbp)                 # 8-byte Spill
	movq	-8(%rbp), %rdi
	callq	kstrlen
	movq	-16(%rbp), %rsi                 # 8-byte Reload
	movq	%rax, %rdx
	movl	$2, %edi
	callq	sys_write
	addq	$16, %rsp
	popq	%rbp
	retq
.Lfunc_end8:
	.size	dstr, .Lfunc_end8-dstr
                                        # -- End function
	.p2align	4                               # -- Begin function load_needed
	.type	load_needed,@function
load_needed:                            # @load_needed
# %bb.0:
	pushq	%rbp
	movq	%rsp, %rbp
	subq	$32, %rsp
	movq	%rdi, -16(%rbp)
	movq	%rsi, -24(%rbp)
	movq	-24(%rbp), %rdi
	callq	already_loaded
	movq	%rax, -32(%rbp)
	cmpq	$0, -32(%rbp)
	je	.LBB9_2
# %bb.1:
	movq	-32(%rbp), %rax
	movq	%rax, -8(%rbp)
	jmp	.LBB9_14
.LBB9_2:
	movq	-24(%rbp), %rdi
	callq	klast_slash
	cmpq	$0, %rax
	jl	.LBB9_4
# %bb.3:
	movq	-24(%rbp), %rdi
	callq	load_path
	movq	%rax, -8(%rbp)
	jmp	.LBB9_14
.LBB9_4:
	movq	-16(%rbp), %rax
	movq	176(%rax), %rdi
	movq	-24(%rbp), %rsi
	callq	try_pathlist
	movq	%rax, -32(%rbp)
	cmpq	$0, %rax
	je	.LBB9_6
# %bb.5:
	movq	-32(%rbp), %rax
	movq	%rax, -8(%rbp)
	jmp	.LBB9_14
.LBB9_6:
	movq	g_env_libpath(%rip), %rdi
	movq	-24(%rbp), %rsi
	callq	try_pathlist
	movq	%rax, -32(%rbp)
	cmpq	$0, %rax
	je	.LBB9_8
# %bb.7:
	movq	-32(%rbp), %rax
	movq	%rax, -8(%rbp)
	jmp	.LBB9_14
.LBB9_8:
	movsbl	g_prog_dir(%rip), %eax
	cmpl	$0, %eax
	je	.LBB9_11
# %bb.9:
	leaq	g_prog_dir(%rip), %rdi
	callq	kstrlen
	movq	%rax, %rsi
	movq	-24(%rbp), %rdx
	leaq	g_prog_dir(%rip), %rdi
	callq	try_dir
	movq	%rax, -32(%rbp)
	cmpq	$0, %rax
	je	.LBB9_11
# %bb.10:
	movq	-32(%rbp), %rax
	movq	%rax, -8(%rbp)
	jmp	.LBB9_14
.LBB9_11:
	movq	-24(%rbp), %rdx
	leaq	.L.str.28(%rip), %rdi
	movl	$1, %esi
	callq	try_dir
	movq	%rax, -32(%rbp)
	cmpq	$0, %rax
	je	.LBB9_13
# %bb.12:
	movq	-32(%rbp), %rax
	movq	%rax, -8(%rbp)
	jmp	.LBB9_14
.LBB9_13:
	leaq	.L.str.29(%rip), %rdi
	callq	dstr
	movq	-24(%rbp), %rdi
	callq	dstr
	leaq	.L.str.5(%rip), %rdi
	callq	dstr
	movl	$127, %edi
	callq	sys_exit
.LBB9_14:
	movq	-8(%rbp), %rax
	addq	$32, %rsp
	popq	%rbp
	retq
.Lfunc_end9:
	.size	load_needed, .Lfunc_end9-load_needed
                                        # -- End function
	.p2align	4                               # -- Begin function relocate_object
	.type	relocate_object,@function
relocate_object:                        # @relocate_object
# %bb.0:
	pushq	%rbp
	movq	%rsp, %rbp
	subq	$16, %rsp
	movq	%rdi, -8(%rbp)
	movq	-8(%rbp), %rax
	cmpl	$0, 252(%rax)
	je	.LBB10_2
# %bb.1:
	jmp	.LBB10_6
.LBB10_2:
	movq	-8(%rbp), %rax
	movl	$1, 252(%rax)
	leaq	.L.str.30(%rip), %rdi
	callq	trace
	movq	-8(%rbp), %rdi
	callq	apply_relr
	movq	-8(%rbp), %rax
	cmpq	$0, 72(%rax)
	je	.LBB10_4
# %bb.3:
	movq	-8(%rbp), %rdi
	movq	-8(%rbp), %rax
	movq	72(%rax), %rsi
	movq	-8(%rbp), %rax
	movq	80(%rax), %rax
	movl	$24, %ecx
	xorl	%edx, %edx
                                        # kill: def $rdx killed $edx
	divq	%rcx
	movq	%rax, %rdx
	callq	apply_rela
.LBB10_4:
	movq	-8(%rbp), %rax
	cmpq	$0, 88(%rax)
	je	.LBB10_6
# %bb.5:
	movq	-8(%rbp), %rdi
	movq	-8(%rbp), %rax
	movq	88(%rax), %rsi
	movq	-8(%rbp), %rax
	movq	96(%rax), %rax
	movl	$24, %ecx
	xorl	%edx, %edx
                                        # kill: def $rdx killed $edx
	divq	%rcx
	movq	%rax, %rdx
	callq	apply_rela
.LBB10_6:
	addq	$16, %rsp
	popq	%rbp
	retq
.Lfunc_end10:
	.size	relocate_object, .Lfunc_end10-relocate_object
                                        # -- End function
	.p2align	4                               # -- Begin function apply_relro
	.type	apply_relro,@function
apply_relro:                            # @apply_relro
# %bb.0:
	pushq	%rbp
	movq	%rsp, %rbp
	subq	$32, %rsp
	movq	%rdi, -8(%rbp)
	movq	-8(%rbp), %rax
	cmpq	$0, 168(%rax)
	jne	.LBB11_2
# %bb.1:
	jmp	.LBB11_6
.LBB11_2:
	movq	-8(%rbp), %rax
	movq	16(%rax), %rdi
	movq	-8(%rbp), %rax
	addq	160(%rax), %rdi
	movl	$4096, %esi                     # imm = 0x1000
	callq	align_down
	movq	%rax, -16(%rbp)
	movq	-8(%rbp), %rax
	movq	16(%rax), %rdi
	movq	-8(%rbp), %rax
	addq	160(%rax), %rdi
	movq	-8(%rbp), %rax
	addq	168(%rax), %rdi
	movl	$4096, %esi                     # imm = 0x1000
	callq	align_up
	movq	%rax, -24(%rbp)
	movq	-24(%rbp), %rax
	cmpq	-16(%rbp), %rax
	jbe	.LBB11_6
# %bb.3:
	movq	-16(%rbp), %rdi
	movq	-24(%rbp), %rsi
	subq	-16(%rbp), %rsi
	movl	$1, %edx
	callq	sys_mprotect
	movq	%rax, -32(%rbp)
	movq	-32(%rbp), %rdi
	callq	mmap_failed
	cmpl	$0, %eax
	je	.LBB11_5
# %bb.4:
	xorl	%eax, %eax
	movl	%eax, %esi
	subq	-32(%rbp), %rsi
	leaq	.L.str.35(%rip), %rdi
	callq	die2
.LBB11_5:
	leaq	.L.str.36(%rip), %rdi
	callq	trace
.LBB11_6:
	addq	$32, %rsp
	popq	%rbp
	retq
.Lfunc_end11:
	.size	apply_relro, .Lfunc_end11-apply_relro
                                        # -- End function
	.p2align	4                               # -- Begin function run_init
	.type	run_init,@function
run_init:                               # @run_init
# %bb.0:
	pushq	%rbp
	movq	%rsp, %rbp
	subq	$64, %rsp
	movq	%rdi, -8(%rbp)
	movl	%esi, -12(%rbp)
	movq	%rdx, -24(%rbp)
	movq	%rcx, -32(%rbp)
	movq	-8(%rbp), %rax
	cmpl	$0, 256(%rax)
	je	.LBB12_2
# %bb.1:
	jmp	.LBB12_11
.LBB12_2:
	movq	-8(%rbp), %rax
	movl	$1, 256(%rax)
	movq	-8(%rbp), %rax
	cmpq	$0, 136(%rax)
	je	.LBB12_4
# %bb.3:
	movq	-8(%rbp), %rax
	movq	136(%rax), %rsi
	leaq	.L.str.37(%rip), %rdi
	callq	trace2
	movq	-8(%rbp), %rax
	movq	136(%rax), %rax
	movl	-12(%rbp), %edi
	movq	-24(%rbp), %rsi
	movq	-32(%rbp), %rdx
	callq	*%rax
.LBB12_4:
	movq	-8(%rbp), %rax
	movq	152(%rax), %rax
	shrq	$3, %rax
	movq	%rax, -40(%rbp)
	movq	$0, -48(%rbp)
.LBB12_5:                               # =>This Inner Loop Header: Depth=1
	movq	-48(%rbp), %rax
	cmpq	-40(%rbp), %rax
	jae	.LBB12_11
# %bb.6:                                #   in Loop: Header=BB12_5 Depth=1
	movq	-8(%rbp), %rax
	movq	144(%rax), %rax
	movq	-48(%rbp), %rcx
	movq	(%rax,%rcx,8), %rax
	movq	%rax, -56(%rbp)
	cmpq	$0, -56(%rbp)
	je	.LBB12_9
# %bb.7:                                #   in Loop: Header=BB12_5 Depth=1
	movq	-56(%rbp), %rax
	cmpq	$-1, %rax
	je	.LBB12_9
# %bb.8:                                #   in Loop: Header=BB12_5 Depth=1
	movq	-56(%rbp), %rax
	movl	-12(%rbp), %edi
	movq	-24(%rbp), %rsi
	movq	-32(%rbp), %rdx
	callq	*%rax
.LBB12_9:                               #   in Loop: Header=BB12_5 Depth=1
	jmp	.LBB12_10
.LBB12_10:                              #   in Loop: Header=BB12_5 Depth=1
	movq	-48(%rbp), %rax
	addq	$1, %rax
	movq	%rax, -48(%rbp)
	jmp	.LBB12_5
.LBB12_11:
	addq	$64, %rsp
	popq	%rbp
	retq
.Lfunc_end12:
	.size	run_init, .Lfunc_end12-run_init
                                        # -- End function
	.p2align	4                               # -- Begin function handoff
	.type	handoff,@function
handoff:                                # @handoff
# %bb.0:
	pushq	%rbp
	movq	%rsp, %rbp
	subq	$160, %rsp
	movq	%rdi, -8(%rbp)
	movl	%esi, -12(%rbp)
	movq	%rdx, -24(%rbp)
	movq	%rcx, -32(%rbp)
	movq	%r8, -40(%rbp)
	movl	-12(%rbp), %eax
	subl	$1, %eax
	movl	%eax, -44(%rbp)
	movq	-24(%rbp), %rax
	addq	$8, %rax
	movq	%rax, -56(%rbp)
	movq	-32(%rbp), %rdi
	callq	count_ptrs
	movl	%eax, -60(%rbp)
	movl	$0, -64(%rbp)
.LBB13_1:                               # =>This Inner Loop Header: Depth=1
	movq	-40(%rbp), %rax
	movslq	-64(%rbp), %rcx
	shlq	$4, %rcx
	addq	%rcx, %rax
	cmpq	$0, (%rax)
	je	.LBB13_3
# %bb.2:                                #   in Loop: Header=BB13_1 Depth=1
	movl	-64(%rbp), %eax
	addl	$1, %eax
	movl	%eax, -64(%rbp)
	jmp	.LBB13_1
.LBB13_3:
	movq	$262144, -72(%rbp)              # imm = 0x40000
	movq	-72(%rbp), %rsi
	xorl	%eax, %eax
	movl	%eax, %r9d
	movl	$3, %edx
	movl	$34, %ecx
	movq	$-1, %r8
	movq	%r9, %rdi
	callq	sys_mmap
	movq	%rax, -80(%rbp)
	movq	-80(%rbp), %rdi
	callq	mmap_failed
	cmpl	$0, %eax
	je	.LBB13_5
# %bb.4:
	xorl	%eax, %eax
	movl	%eax, %esi
	subq	-80(%rbp), %rsi
	leaq	.L.str.38(%rip), %rdi
	callq	die2
.LBB13_5:
	movq	-80(%rbp), %rax
	addq	-72(%rbp), %rax
	movq	%rax, -88(%rbp)
	movl	-44(%rbp), %eax
	addl	$1, %eax
	cltq
	addq	$1, %rax
	movl	-60(%rbp), %ecx
	addl	$1, %ecx
	movslq	%ecx, %rcx
	addq	%rcx, %rax
	movl	-64(%rbp), %ecx
	addl	$1, %ecx
	movslq	%ecx, %rcx
	shlq	%rcx
	addq	%rcx, %rax
	movq	%rax, -96(%rbp)
	movq	-88(%rbp), %rdi
	movq	-96(%rbp), %rax
	shlq	$3, %rax
	subq	%rax, %rdi
	movl	$16, %esi
	callq	align_down
	movq	%rax, -104(%rbp)
	movq	-104(%rbp), %rax
	movq	%rax, -112(%rbp)
	movslq	-44(%rbp), %rcx
	movq	-112(%rbp), %rax
	movq	%rax, %rdx
	addq	$8, %rdx
	movq	%rdx, -112(%rbp)
	movq	%rcx, (%rax)
	movl	$0, -116(%rbp)
.LBB13_6:                               # =>This Inner Loop Header: Depth=1
	movl	-116(%rbp), %eax
	cmpl	-44(%rbp), %eax
	jge	.LBB13_9
# %bb.7:                                #   in Loop: Header=BB13_6 Depth=1
	movq	-56(%rbp), %rax
	movslq	-116(%rbp), %rcx
	movq	(%rax,%rcx,8), %rcx
	movq	-112(%rbp), %rax
	movq	%rax, %rdx
	addq	$8, %rdx
	movq	%rdx, -112(%rbp)
	movq	%rcx, (%rax)
# %bb.8:                                #   in Loop: Header=BB13_6 Depth=1
	movl	-116(%rbp), %eax
	addl	$1, %eax
	movl	%eax, -116(%rbp)
	jmp	.LBB13_6
.LBB13_9:
	movq	-112(%rbp), %rax
	movq	%rax, %rcx
	addq	$8, %rcx
	movq	%rcx, -112(%rbp)
	movq	$0, (%rax)
	movl	$0, -120(%rbp)
.LBB13_10:                              # =>This Inner Loop Header: Depth=1
	movl	-120(%rbp), %eax
	cmpl	-60(%rbp), %eax
	jge	.LBB13_13
# %bb.11:                               #   in Loop: Header=BB13_10 Depth=1
	movq	-32(%rbp), %rax
	movslq	-120(%rbp), %rcx
	movq	(%rax,%rcx,8), %rcx
	movq	-112(%rbp), %rax
	movq	%rax, %rdx
	addq	$8, %rdx
	movq	%rdx, -112(%rbp)
	movq	%rcx, (%rax)
# %bb.12:                               #   in Loop: Header=BB13_10 Depth=1
	movl	-120(%rbp), %eax
	addl	$1, %eax
	movl	%eax, -120(%rbp)
	jmp	.LBB13_10
.LBB13_13:
	movq	-112(%rbp), %rax
	movq	%rax, %rcx
	addq	$8, %rcx
	movq	%rcx, -112(%rbp)
	movq	$0, (%rax)
	movl	$0, -124(%rbp)
.LBB13_14:                              # =>This Inner Loop Header: Depth=1
	movl	-124(%rbp), %eax
	cmpl	-64(%rbp), %eax
	jge	.LBB13_26
# %bb.15:                               #   in Loop: Header=BB13_14 Depth=1
	movq	-40(%rbp), %rax
	movslq	-124(%rbp), %rcx
	shlq	$4, %rcx
	movq	(%rax,%rcx), %rax
	movq	%rax, -136(%rbp)
	movq	-40(%rbp), %rax
	movslq	-124(%rbp), %rcx
	shlq	$4, %rcx
	movq	8(%rax,%rcx), %rax
	movq	%rax, -144(%rbp)
	movq	-136(%rbp), %rax
	movq	%rax, -152(%rbp)                # 8-byte Spill
	subq	$3, %rax
	je	.LBB13_16
	jmp	.LBB13_27
.LBB13_27:                              #   in Loop: Header=BB13_14 Depth=1
	movq	-152(%rbp), %rax                # 8-byte Reload
	subq	$4, %rax
	je	.LBB13_17
	jmp	.LBB13_28
.LBB13_28:                              #   in Loop: Header=BB13_14 Depth=1
	movq	-152(%rbp), %rax                # 8-byte Reload
	subq	$5, %rax
	je	.LBB13_18
	jmp	.LBB13_29
.LBB13_29:                              #   in Loop: Header=BB13_14 Depth=1
	movq	-152(%rbp), %rax                # 8-byte Reload
	subq	$6, %rax
	je	.LBB13_22
	jmp	.LBB13_30
.LBB13_30:                              #   in Loop: Header=BB13_14 Depth=1
	movq	-152(%rbp), %rax                # 8-byte Reload
	subq	$7, %rax
	je	.LBB13_20
	jmp	.LBB13_31
.LBB13_31:                              #   in Loop: Header=BB13_14 Depth=1
	movq	-152(%rbp), %rax                # 8-byte Reload
	subq	$9, %rax
	je	.LBB13_19
	jmp	.LBB13_32
.LBB13_32:                              #   in Loop: Header=BB13_14 Depth=1
	movq	-152(%rbp), %rax                # 8-byte Reload
	subq	$31, %rax
	je	.LBB13_21
	jmp	.LBB13_23
.LBB13_16:                              #   in Loop: Header=BB13_14 Depth=1
	movq	-8(%rbp), %rax
	movq	24(%rax), %rax
	movq	%rax, -144(%rbp)
	jmp	.LBB13_24
.LBB13_17:                              #   in Loop: Header=BB13_14 Depth=1
	movq	$56, -144(%rbp)
	jmp	.LBB13_24
.LBB13_18:                              #   in Loop: Header=BB13_14 Depth=1
	movq	-8(%rbp), %rax
	movq	32(%rax), %rax
	movq	%rax, -144(%rbp)
	jmp	.LBB13_24
.LBB13_19:                              #   in Loop: Header=BB13_14 Depth=1
	movq	-8(%rbp), %rax
	movq	40(%rax), %rax
	movq	%rax, -144(%rbp)
	jmp	.LBB13_24
.LBB13_20:                              #   in Loop: Header=BB13_14 Depth=1
	movq	$0, -144(%rbp)
	jmp	.LBB13_24
.LBB13_21:                              #   in Loop: Header=BB13_14 Depth=1
	movq	-56(%rbp), %rax
	movq	(%rax), %rax
	movq	%rax, -144(%rbp)
	jmp	.LBB13_24
.LBB13_22:                              #   in Loop: Header=BB13_14 Depth=1
	movq	$4096, -144(%rbp)               # imm = 0x1000
	jmp	.LBB13_24
.LBB13_23:                              #   in Loop: Header=BB13_14 Depth=1
	jmp	.LBB13_24
.LBB13_24:                              #   in Loop: Header=BB13_14 Depth=1
	movq	-136(%rbp), %rcx
	movq	-112(%rbp), %rax
	movq	%rax, %rdx
	addq	$8, %rdx
	movq	%rdx, -112(%rbp)
	movq	%rcx, (%rax)
	movq	-144(%rbp), %rcx
	movq	-112(%rbp), %rax
	movq	%rax, %rdx
	addq	$8, %rdx
	movq	%rdx, -112(%rbp)
	movq	%rcx, (%rax)
# %bb.25:                               #   in Loop: Header=BB13_14 Depth=1
	movl	-124(%rbp), %eax
	addl	$1, %eax
	movl	%eax, -124(%rbp)
	jmp	.LBB13_14
.LBB13_26:
	movq	-112(%rbp), %rax
	movq	%rax, %rcx
	addq	$8, %rcx
	movq	%rcx, -112(%rbp)
	movq	$0, (%rax)
	movq	-112(%rbp), %rax
	movq	%rax, %rcx
	addq	$8, %rcx
	movq	%rcx, -112(%rbp)
	movq	$0, (%rax)
	movq	-8(%rbp), %rax
	movq	40(%rax), %rsi
	leaq	.L.str.39(%rip), %rdi
	callq	trace2
	movq	-104(%rbp), %rsi
	leaq	.L.str.40(%rip), %rdi
	callq	trace2
	movq	-8(%rbp), %rax
	movq	40(%rax), %rdi
	movq	-104(%rbp), %rsi
	callq	enter_program
.Lfunc_end13:
	.size	handoff, .Lfunc_end13-handoff
                                        # -- End function
	.p2align	4                               # -- Begin function sys_exit
	.type	sys_exit,@function
sys_exit:                               # @sys_exit
# %bb.0:
	pushq	%rbp
	movq	%rsp, %rbp
	subq	$16, %rsp
	movl	%edi, -4(%rbp)
	movslq	-4(%rbp), %rsi
	movl	$231, %edi
	callq	syscall1
.Lfunc_end14:
	.size	sys_exit, .Lfunc_end14-sys_exit
                                        # -- End function
	.p2align	4                               # -- Begin function syscall1
	.type	syscall1,@function
syscall1:                               # @syscall1
# %bb.0:
	pushq	%rbp
	movq	%rsp, %rbp
	subq	$32, %rsp
	movq	%rdi, -8(%rbp)
	movq	%rsi, -16(%rbp)
	movq	-8(%rbp), %rdi
	movq	-16(%rbp), %rsi
	xorl	%eax, %eax
	movl	%eax, %r9d
	movq	%r9, %rdx
	movq	%r9, %rcx
	movq	%r9, %r8
	movq	$0, (%rsp)
	callq	syscall6
	addq	$32, %rsp
	popq	%rbp
	retq
.Lfunc_end15:
	.size	syscall1, .Lfunc_end15-syscall1
                                        # -- End function
	.p2align	4                               # -- Begin function syscall6
	.type	syscall6,@function
syscall6:                               # @syscall6
# %bb.0:
	pushq	%rbp
	movq	%rsp, %rbp
	movq	16(%rbp), %rax
	movq	%rdi, -8(%rbp)
	movq	%rsi, -16(%rbp)
	movq	%rdx, -24(%rbp)
	movq	%rcx, -32(%rbp)
	movq	%r8, -40(%rbp)
	movq	%r9, -48(%rbp)
	movq	-40(%rbp), %rax
	movq	%rax, -64(%rbp)
	movq	-48(%rbp), %rax
	movq	%rax, -72(%rbp)
	movq	16(%rbp), %rax
	movq	%rax, -80(%rbp)
	movq	-8(%rbp), %rax
	movq	-16(%rbp), %rdi
	movq	-24(%rbp), %rsi
	movq	-32(%rbp), %rdx
	movq	-64(%rbp), %r10
	movq	-72(%rbp), %r8
	movq	-80(%rbp), %r9
	#APP
	syscall
	#NO_APP
	movq	%rax, -56(%rbp)
	movq	-56(%rbp), %rax
	popq	%rbp
	retq
.Lfunc_end16:
	.size	syscall6, .Lfunc_end16-syscall6
                                        # -- End function
	.p2align	4                               # -- Begin function sys_openat
	.type	sys_openat,@function
sys_openat:                             # @sys_openat
# %bb.0:
	pushq	%rbp
	movq	%rsp, %rbp
	subq	$16, %rsp
	movq	%rdi, -8(%rbp)
	movq	-8(%rbp), %rdx
	movl	$257, %edi                      # imm = 0x101
	movq	$-100, %rsi
	xorl	%eax, %eax
	movl	%eax, %r8d
	movq	%r8, %rcx
	callq	syscall4
	addq	$16, %rsp
	popq	%rbp
	retq
.Lfunc_end17:
	.size	sys_openat, .Lfunc_end17-sys_openat
                                        # -- End function
	.p2align	4                               # -- Begin function die2
	.type	die2,@function
die2:                                   # @die2
# %bb.0:
	pushq	%rbp
	movq	%rsp, %rbp
	subq	$16, %rsp
	movq	%rdi, -8(%rbp)
	movq	%rsi, -16(%rbp)
	leaq	.L.str.6(%rip), %rdi
	callq	dstr
	movq	-8(%rbp), %rdi
	callq	dstr
	leaq	.L.str.9(%rip), %rdi
	callq	dstr
	movq	-16(%rbp), %rdi
	callq	dhex
	leaq	.L.str.5(%rip), %rdi
	callq	dstr
	movl	$127, %edi
	callq	sys_exit
.Lfunc_end18:
	.size	die2, .Lfunc_end18-die2
                                        # -- End function
	.p2align	4                               # -- Begin function load_from_fd
	.type	load_from_fd,@function
load_from_fd:                           # @load_from_fd
# %bb.0:
	pushq	%rbp
	movq	%rsp, %rbp
	subq	$128, %rsp
	movl	%edi, -4(%rbp)
	movq	%rsi, -16(%rbp)
	cmpl	$32, g_nobjs(%rip)
	jl	.LBB19_2
# %bb.1:
	leaq	.L.str.10(%rip), %rdi
	callq	die
.LBB19_2:
	movl	g_nobjs(%rip), %eax
	movl	%eax, %ecx
	addl	$1, %ecx
	movl	%ecx, g_nobjs(%rip)
	movslq	%eax, %rcx
	leaq	g_objs(%rip), %rax
	imulq	$264, %rcx, %rcx                # imm = 0x108
	addq	%rcx, %rax
	movq	%rax, -24(%rbp)
	movq	-24(%rbp), %rdi
	xorl	%esi, %esi
	movl	$264, %edx                      # imm = 0x108
	callq	memset@PLT
	movq	-16(%rbp), %rcx
	movq	-24(%rbp), %rax
	movq	%rcx, (%rax)
	movl	-4(%rbp), %edi
	leaq	-88(%rbp), %rsi
	movl	$64, %edx
	xorl	%eax, %eax
	movl	%eax, %ecx
	callq	sys_pread
	movq	%rax, -96(%rbp)
	cmpq	$64, -96(%rbp)
	je	.LBB19_4
# %bb.3:
	leaq	.L.str.11(%rip), %rdi
	callq	die
.LBB19_4:
	movzbl	-88(%rbp), %eax
	cmpl	$127, %eax
	jne	.LBB19_8
# %bb.5:
	movzbl	-87(%rbp), %eax
	cmpl	$69, %eax
	jne	.LBB19_8
# %bb.6:
	movzbl	-86(%rbp), %eax
	cmpl	$76, %eax
	jne	.LBB19_8
# %bb.7:
	movzbl	-85(%rbp), %eax
	cmpl	$70, %eax
	je	.LBB19_9
.LBB19_8:
	leaq	.L.str.12(%rip), %rdi
	callq	die
.LBB19_9:
	movzbl	-84(%rbp), %eax
	cmpl	$2, %eax
	je	.LBB19_11
# %bb.10:
	leaq	.L.str.13(%rip), %rdi
	callq	die
.LBB19_11:
	movzbl	-83(%rbp), %eax
	cmpl	$1, %eax
	je	.LBB19_13
# %bb.12:
	leaq	.L.str.14(%rip), %rdi
	callq	die
.LBB19_13:
	movzwl	-70(%rbp), %eax
	cmpl	$62, %eax
	je	.LBB19_15
# %bb.14:
	leaq	.L.str.15(%rip), %rdi
	callq	die
.LBB19_15:
	movzwl	-72(%rbp), %eax
	cmpl	$3, %eax
	je	.LBB19_18
# %bb.16:
	movzwl	-72(%rbp), %eax
	cmpl	$2, %eax
	je	.LBB19_18
# %bb.17:
	leaq	.L.str.16(%rip), %rdi
	callq	die
.LBB19_18:
	movzwl	-34(%rbp), %eax
                                        # kill: def $rax killed $eax
	cmpq	$56, %rax
	je	.LBB19_20
# %bb.19:
	leaq	.L.str.17(%rip), %rdi
	callq	die
.LBB19_20:
	movzwl	-32(%rbp), %eax
	cmpl	$0, %eax
	je	.LBB19_22
# %bb.21:
	movzwl	-32(%rbp), %eax
	cmpl	$256, %eax                      # imm = 0x100
	jle	.LBB19_23
.LBB19_22:
	leaq	.L.str.18(%rip), %rdi
	callq	die
.LBB19_23:
	movzwl	-32(%rbp), %eax
                                        # kill: def $rax killed $eax
	imulq	$56, %rax, %rax
	movq	%rax, -104(%rbp)
	movl	-4(%rbp), %edi
	movq	-104(%rbp), %rdx
	movq	-56(%rbp), %rcx
	leaq	load_from_fd.ph(%rip), %rsi
	callq	sys_pread
	movq	%rax, -96(%rbp)
	movq	-96(%rbp), %rax
	cmpq	-104(%rbp), %rax
	je	.LBB19_25
# %bb.24:
	leaq	.L.str.19(%rip), %rdi
	callq	die
.LBB19_25:
	movl	-4(%rbp), %edi
	movq	-24(%rbp), %rcx
	leaq	-88(%rbp), %rsi
	leaq	load_from_fd.ph(%rip), %rdx
	callq	map_object
	movq	%rax, %rcx
	movq	-24(%rbp), %rax
	movq	%rcx, 16(%rax)
	movq	-24(%rbp), %rax
	movq	16(%rax), %rcx
	addq	-64(%rbp), %rcx
	movq	-24(%rbp), %rax
	movq	%rcx, 40(%rax)
	movq	$0, -112(%rbp)
	movq	$0, -120(%rbp)
.LBB19_26:                              # =>This Inner Loop Header: Depth=1
	movq	-120(%rbp), %rax
	movzwl	-32(%rbp), %ecx
                                        # kill: def $rcx killed $ecx
	cmpq	%rcx, %rax
	jae	.LBB19_38
# %bb.27:                               #   in Loop: Header=BB19_26 Depth=1
	leaq	load_from_fd.ph(%rip), %rax
	imulq	$56, -120(%rbp), %rcx
	addq	%rcx, %rax
	cmpl	$2, (%rax)
	jne	.LBB19_29
# %bb.28:                               #   in Loop: Header=BB19_26 Depth=1
	movq	-24(%rbp), %rax
	movq	16(%rax), %rax
	leaq	load_from_fd.ph(%rip), %rcx
	imulq	$56, -120(%rbp), %rdx
	addq	%rdx, %rcx
	addq	16(%rcx), %rax
	movq	%rax, -112(%rbp)
	jmp	.LBB19_36
.LBB19_29:                              #   in Loop: Header=BB19_26 Depth=1
	leaq	load_from_fd.ph(%rip), %rax
	imulq	$56, -120(%rbp), %rcx
	addq	%rcx, %rax
	cmpl	$1685382482, (%rax)             # imm = 0x6474E552
	jne	.LBB19_31
# %bb.30:                               #   in Loop: Header=BB19_26 Depth=1
	leaq	load_from_fd.ph(%rip), %rax
	imulq	$56, -120(%rbp), %rcx
	addq	%rcx, %rax
	movq	16(%rax), %rcx
	movq	-24(%rbp), %rax
	movq	%rcx, 160(%rax)
	leaq	load_from_fd.ph(%rip), %rax
	imulq	$56, -120(%rbp), %rcx
	addq	%rcx, %rax
	movq	40(%rax), %rcx
	movq	-24(%rbp), %rax
	movq	%rcx, 168(%rax)
	jmp	.LBB19_35
.LBB19_31:                              #   in Loop: Header=BB19_26 Depth=1
	leaq	load_from_fd.ph(%rip), %rax
	imulq	$56, -120(%rbp), %rcx
	addq	%rcx, %rax
	cmpl	$7, (%rax)
	jne	.LBB19_34
# %bb.32:                               #   in Loop: Header=BB19_26 Depth=1
	leaq	load_from_fd.ph(%rip), %rax
	imulq	$56, -120(%rbp), %rcx
	addq	%rcx, %rax
	cmpq	$0, 40(%rax)
	je	.LBB19_34
# %bb.33:                               #   in Loop: Header=BB19_26 Depth=1
	leaq	.L.str.20(%rip), %rdi
	callq	trace
.LBB19_34:                              #   in Loop: Header=BB19_26 Depth=1
	jmp	.LBB19_35
.LBB19_35:                              #   in Loop: Header=BB19_26 Depth=1
	jmp	.LBB19_36
.LBB19_36:                              #   in Loop: Header=BB19_26 Depth=1
	jmp	.LBB19_37
.LBB19_37:                              #   in Loop: Header=BB19_26 Depth=1
	movq	-120(%rbp), %rax
	addq	$1, %rax
	movq	%rax, -120(%rbp)
	jmp	.LBB19_26
.LBB19_38:
	cmpq	$0, -112(%rbp)
	je	.LBB19_40
# %bb.39:
	movq	-24(%rbp), %rdi
	movq	-112(%rbp), %rsi
	callq	parse_dynamic
.LBB19_40:
	movq	-24(%rbp), %rax
	addq	$128, %rsp
	popq	%rbp
	retq
.Lfunc_end19:
	.size	load_from_fd, .Lfunc_end19-load_from_fd
                                        # -- End function
	.p2align	4                               # -- Begin function sys_close
	.type	sys_close,@function
sys_close:                              # @sys_close
# %bb.0:
	pushq	%rbp
	movq	%rsp, %rbp
	subq	$16, %rsp
	movl	%edi, -4(%rbp)
	movslq	-4(%rbp), %rsi
	movl	$3, %edi
	callq	syscall1
	addq	$16, %rsp
	popq	%rbp
	retq
.Lfunc_end20:
	.size	sys_close, .Lfunc_end20-sys_close
                                        # -- End function
	.p2align	4                               # -- Begin function syscall4
	.type	syscall4,@function
syscall4:                               # @syscall4
# %bb.0:
	pushq	%rbp
	movq	%rsp, %rbp
	subq	$48, %rsp
	movq	%rdi, -8(%rbp)
	movq	%rsi, -16(%rbp)
	movq	%rdx, -24(%rbp)
	movq	%rcx, -32(%rbp)
	movq	%r8, -40(%rbp)
	movq	-8(%rbp), %rdi
	movq	-16(%rbp), %rsi
	movq	-24(%rbp), %rdx
	movq	-32(%rbp), %rcx
	movq	-40(%rbp), %r8
	xorl	%eax, %eax
	movl	%eax, %r9d
	movq	$0, (%rsp)
	callq	syscall6
	addq	$48, %rsp
	popq	%rbp
	retq
.Lfunc_end21:
	.size	syscall4, .Lfunc_end21-syscall4
                                        # -- End function
	.p2align	4                               # -- Begin function dhex
	.type	dhex,@function
dhex:                                   # @dhex
# %bb.0:
	pushq	%rbp
	movq	%rsp, %rbp
	subq	$48, %rsp
	movq	%rdi, -8(%rbp)
	movb	$48, -32(%rbp)
	movb	$120, -31(%rbp)
	movl	$0, -36(%rbp)
.LBB22_1:                               # =>This Inner Loop Header: Depth=1
	cmpl	$16, -36(%rbp)
	jge	.LBB22_4
# %bb.2:                                #   in Loop: Header=BB22_1 Depth=1
	movq	-8(%rbp), %rax
	movl	$15, %ecx
	subl	-36(%rbp), %ecx
	shll	$2, %ecx
	movl	%ecx, %ecx
                                        # kill: def $rcx killed $ecx
                                        # kill: def $cl killed $rcx
	shrq	%cl, %rax
	movq	%rax, %rcx
	andq	$15, %rcx
	leaq	dhex.H(%rip), %rax
	movb	(%rax,%rcx), %cl
	movl	-36(%rbp), %eax
	addl	$2, %eax
	cltq
	movb	%cl, -32(%rbp,%rax)
# %bb.3:                                #   in Loop: Header=BB22_1 Depth=1
	movl	-36(%rbp), %eax
	addl	$1, %eax
	movl	%eax, -36(%rbp)
	jmp	.LBB22_1
.LBB22_4:
	movb	$0, -14(%rbp)
	leaq	-32(%rbp), %rdi
	callq	dstr
	addq	$48, %rsp
	popq	%rbp
	retq
.Lfunc_end22:
	.size	dhex, .Lfunc_end22-dhex
                                        # -- End function
	.p2align	4                               # -- Begin function sys_pread
	.type	sys_pread,@function
sys_pread:                              # @sys_pread
# %bb.0:
	pushq	%rbp
	movq	%rsp, %rbp
	subq	$32, %rsp
	movl	%edi, -4(%rbp)
	movq	%rsi, -16(%rbp)
	movq	%rdx, -24(%rbp)
	movq	%rcx, -32(%rbp)
	movslq	-4(%rbp), %rsi
	movq	-16(%rbp), %rdx
	movq	-24(%rbp), %rcx
	movq	-32(%rbp), %r8
	movl	$17, %edi
	callq	syscall4
	addq	$32, %rsp
	popq	%rbp
	retq
.Lfunc_end23:
	.size	sys_pread, .Lfunc_end23-sys_pread
                                        # -- End function
	.p2align	4                               # -- Begin function map_object
	.type	map_object,@function
map_object:                             # @map_object
# %bb.0:
	pushq	%rbp
	movq	%rsp, %rbp
	subq	$192, %rsp
	movl	%edi, -4(%rbp)
	movq	%rsi, -16(%rbp)
	movq	%rdx, -24(%rbp)
	movq	%rcx, -32(%rbp)
	movq	$-1, -40(%rbp)
	movq	$0, -48(%rbp)
	movq	$0, -56(%rbp)
.LBB24_1:                               # =>This Inner Loop Header: Depth=1
	movq	-56(%rbp), %rax
	movq	-16(%rbp), %rcx
	movzwl	56(%rcx), %ecx
                                        # kill: def $rcx killed $ecx
	cmpq	%rcx, %rax
	jae	.LBB24_10
# %bb.2:                                #   in Loop: Header=BB24_1 Depth=1
	movq	-24(%rbp), %rax
	imulq	$56, -56(%rbp), %rcx
	addq	%rcx, %rax
	cmpl	$1, (%rax)
	je	.LBB24_4
# %bb.3:                                #   in Loop: Header=BB24_1 Depth=1
	jmp	.LBB24_9
.LBB24_4:                               #   in Loop: Header=BB24_1 Depth=1
	movq	-24(%rbp), %rax
	imulq	$56, -56(%rbp), %rcx
	addq	%rcx, %rax
	movq	16(%rax), %rdi
	movl	$4096, %esi                     # imm = 0x1000
	callq	align_down
	movq	%rax, -64(%rbp)
	movq	-24(%rbp), %rax
	imulq	$56, -56(%rbp), %rcx
	addq	%rcx, %rax
	movq	16(%rax), %rdi
	movq	-24(%rbp), %rax
	imulq	$56, -56(%rbp), %rcx
	addq	%rcx, %rax
	addq	40(%rax), %rdi
	movl	$4096, %esi                     # imm = 0x1000
	callq	align_up
	movq	%rax, -72(%rbp)
	movq	-64(%rbp), %rax
	cmpq	-40(%rbp), %rax
	jae	.LBB24_6
# %bb.5:                                #   in Loop: Header=BB24_1 Depth=1
	movq	-64(%rbp), %rax
	movq	%rax, -40(%rbp)
.LBB24_6:                               #   in Loop: Header=BB24_1 Depth=1
	movq	-72(%rbp), %rax
	cmpq	-48(%rbp), %rax
	jbe	.LBB24_8
# %bb.7:                                #   in Loop: Header=BB24_1 Depth=1
	movq	-72(%rbp), %rax
	movq	%rax, -48(%rbp)
.LBB24_8:                               #   in Loop: Header=BB24_1 Depth=1
	jmp	.LBB24_9
.LBB24_9:                               #   in Loop: Header=BB24_1 Depth=1
	movq	-56(%rbp), %rax
	addq	$1, %rax
	movq	%rax, -56(%rbp)
	jmp	.LBB24_1
.LBB24_10:
	cmpq	$-1, -40(%rbp)
	jne	.LBB24_12
# %bb.11:
	leaq	.L.str.21(%rip), %rdi
	callq	die
.LBB24_12:
	movq	-48(%rbp), %rax
	subq	-40(%rbp), %rax
	movq	%rax, -80(%rbp)
	movq	-16(%rbp), %rax
	movzwl	16(%rax), %edx
	xorl	%eax, %eax
	movl	$16, %ecx
	cmpl	$2, %edx
	cmovel	%ecx, %eax
	cltq
	movq	%rax, -88(%rbp)
	movq	-16(%rbp), %rax
	movzwl	16(%rax), %eax
	cmpl	$2, %eax
	jne	.LBB24_14
# %bb.13:
	movq	-40(%rbp), %rax
	movq	%rax, -176(%rbp)                # 8-byte Spill
	jmp	.LBB24_15
.LBB24_14:
	xorl	%eax, %eax
                                        # kill: def $rax killed $eax
	movq	%rax, -176(%rbp)                # 8-byte Spill
	jmp	.LBB24_15
.LBB24_15:
	movq	-176(%rbp), %rdi                # 8-byte Reload
	movq	-80(%rbp), %rsi
	movq	-88(%rbp), %rcx
	orq	$34, %rcx
	xorl	%eax, %eax
	movl	%eax, %r9d
	movq	$-1, %r8
	movq	%r9, %rdx
	callq	sys_mmap
	movq	%rax, -96(%rbp)
	movq	-96(%rbp), %rdi
	callq	mmap_failed
	cmpl	$0, %eax
	je	.LBB24_17
# %bb.16:
	xorl	%eax, %eax
	movl	%eax, %esi
	subq	-96(%rbp), %rsi
	leaq	.L.str.22(%rip), %rdi
	callq	die2
.LBB24_17:
	movq	-16(%rbp), %rax
	movzwl	16(%rax), %eax
	cmpl	$2, %eax
	jne	.LBB24_19
# %bb.18:
	xorl	%eax, %eax
                                        # kill: def $rax killed $eax
	movq	%rax, -184(%rbp)                # 8-byte Spill
	jmp	.LBB24_20
.LBB24_19:
	movq	-96(%rbp), %rax
	subq	-40(%rbp), %rax
	movq	%rax, -184(%rbp)                # 8-byte Spill
.LBB24_20:
	movq	-184(%rbp), %rax                # 8-byte Reload
	movq	%rax, -104(%rbp)
	movq	-104(%rbp), %rsi
	leaq	.L.str.23(%rip), %rdi
	callq	trace2
	movq	$0, -112(%rbp)
.LBB24_21:                              # =>This Inner Loop Header: Depth=1
	movq	-112(%rbp), %rax
	movq	-16(%rbp), %rcx
	movzwl	56(%rcx), %ecx
                                        # kill: def $rcx killed $ecx
	cmpq	%rcx, %rax
	jae	.LBB24_41
# %bb.22:                               #   in Loop: Header=BB24_21 Depth=1
	movq	-24(%rbp), %rax
	imulq	$56, -112(%rbp), %rcx
	addq	%rcx, %rax
	movq	%rax, -120(%rbp)
	movq	-120(%rbp), %rax
	cmpl	$1, (%rax)
	je	.LBB24_24
# %bb.23:                               #   in Loop: Header=BB24_21 Depth=1
	jmp	.LBB24_40
.LBB24_24:                              #   in Loop: Header=BB24_21 Depth=1
	movl	$0, -124(%rbp)
	movq	-120(%rbp), %rax
	movl	4(%rax), %eax
	andl	$4, %eax
	cmpl	$0, %eax
	je	.LBB24_26
# %bb.25:                               #   in Loop: Header=BB24_21 Depth=1
	movl	-124(%rbp), %eax
	orl	$1, %eax
	movl	%eax, -124(%rbp)
.LBB24_26:                              #   in Loop: Header=BB24_21 Depth=1
	movq	-120(%rbp), %rax
	movl	4(%rax), %eax
	andl	$2, %eax
	cmpl	$0, %eax
	je	.LBB24_28
# %bb.27:                               #   in Loop: Header=BB24_21 Depth=1
	movl	-124(%rbp), %eax
	orl	$2, %eax
	movl	%eax, -124(%rbp)
.LBB24_28:                              #   in Loop: Header=BB24_21 Depth=1
	movq	-120(%rbp), %rax
	movl	4(%rax), %eax
	andl	$1, %eax
	cmpl	$0, %eax
	je	.LBB24_30
# %bb.29:                               #   in Loop: Header=BB24_21 Depth=1
	movl	-124(%rbp), %eax
	orl	$4, %eax
	movl	%eax, -124(%rbp)
.LBB24_30:                              #   in Loop: Header=BB24_21 Depth=1
	movq	-104(%rbp), %rdi
	movq	-120(%rbp), %rax
	addq	16(%rax), %rdi
	movl	$4096, %esi                     # imm = 0x1000
	callq	align_down
	movq	%rax, -136(%rbp)
	movq	-120(%rbp), %rax
	movq	8(%rax), %rdi
	movl	$4096, %esi                     # imm = 0x1000
	callq	align_down
	movq	%rax, -144(%rbp)
	movq	-104(%rbp), %rdi
	movq	-120(%rbp), %rax
	addq	16(%rax), %rdi
	movq	-120(%rbp), %rax
	addq	32(%rax), %rdi
	movl	$4096, %esi                     # imm = 0x1000
	callq	align_up
	movq	%rax, -152(%rbp)
	movq	-152(%rbp), %rax
	cmpq	-136(%rbp), %rax
	jbe	.LBB24_34
# %bb.31:                               #   in Loop: Header=BB24_21 Depth=1
	movq	-136(%rbp), %rdi
	movq	-152(%rbp), %rsi
	subq	-136(%rbp), %rsi
	movslq	-124(%rbp), %rdx
	movslq	-4(%rbp), %r8
	movq	-144(%rbp), %r9
	movl	$18, %ecx
	callq	sys_mmap
	movq	%rax, -96(%rbp)
	movq	-96(%rbp), %rdi
	callq	mmap_failed
	cmpl	$0, %eax
	je	.LBB24_33
# %bb.32:
	xorl	%eax, %eax
	movl	%eax, %esi
	subq	-96(%rbp), %rsi
	leaq	.L.str.24(%rip), %rdi
	callq	die2
.LBB24_33:                              #   in Loop: Header=BB24_21 Depth=1
	jmp	.LBB24_34
.LBB24_34:                              #   in Loop: Header=BB24_21 Depth=1
	movq	-104(%rbp), %rdi
	movq	-120(%rbp), %rax
	addq	16(%rax), %rdi
	movq	-120(%rbp), %rax
	addq	40(%rax), %rdi
	movl	$4096, %esi                     # imm = 0x1000
	callq	align_up
	movq	%rax, -160(%rbp)
	movq	-160(%rbp), %rax
	cmpq	-152(%rbp), %rax
	jbe	.LBB24_39
# %bb.35:                               #   in Loop: Header=BB24_21 Depth=1
	movq	-120(%rbp), %rax
	movl	4(%rax), %eax
	andl	$2, %eax
	cmpl	$0, %eax
	je	.LBB24_39
# %bb.36:                               #   in Loop: Header=BB24_21 Depth=1
	movq	-152(%rbp), %rdi
	movq	-160(%rbp), %rsi
	subq	-152(%rbp), %rsi
	movslq	-124(%rbp), %rdx
	movl	$50, %ecx
	movq	$-1, %r8
	xorl	%eax, %eax
	movl	%eax, %r9d
	callq	sys_mmap
	movq	%rax, -96(%rbp)
	movq	-96(%rbp), %rdi
	callq	mmap_failed
	cmpl	$0, %eax
	je	.LBB24_38
# %bb.37:
	xorl	%eax, %eax
	movl	%eax, %esi
	subq	-96(%rbp), %rsi
	leaq	.L.str.25(%rip), %rdi
	callq	die2
.LBB24_38:                              #   in Loop: Header=BB24_21 Depth=1
	jmp	.LBB24_39
.LBB24_39:                              #   in Loop: Header=BB24_21 Depth=1
	jmp	.LBB24_40
.LBB24_40:                              #   in Loop: Header=BB24_21 Depth=1
	movq	-112(%rbp), %rax
	addq	$1, %rax
	movq	%rax, -112(%rbp)
	jmp	.LBB24_21
.LBB24_41:
	movq	-32(%rbp), %rax
	movq	$0, 24(%rax)
	movq	$0, -168(%rbp)
.LBB24_42:                              # =>This Inner Loop Header: Depth=1
	movq	-168(%rbp), %rax
	movq	-16(%rbp), %rcx
	movzwl	56(%rcx), %ecx
                                        # kill: def $rcx killed $ecx
	cmpq	%rcx, %rax
	jae	.LBB24_49
# %bb.43:                               #   in Loop: Header=BB24_42 Depth=1
	movq	-24(%rbp), %rax
	imulq	$56, -168(%rbp), %rcx
	addq	%rcx, %rax
	cmpl	$1, (%rax)
	jne	.LBB24_47
# %bb.44:                               #   in Loop: Header=BB24_42 Depth=1
	movq	-24(%rbp), %rax
	imulq	$56, -168(%rbp), %rcx
	addq	%rcx, %rax
	movq	8(%rax), %rax
	movq	-16(%rbp), %rcx
	cmpq	32(%rcx), %rax
	ja	.LBB24_47
# %bb.45:                               #   in Loop: Header=BB24_42 Depth=1
	movq	-16(%rbp), %rax
	movq	32(%rax), %rax
	movq	-24(%rbp), %rcx
	imulq	$56, -168(%rbp), %rdx
	addq	%rdx, %rcx
	movq	8(%rcx), %rcx
	movq	-24(%rbp), %rdx
	imulq	$56, -168(%rbp), %rsi
	addq	%rsi, %rdx
	addq	32(%rdx), %rcx
	cmpq	%rcx, %rax
	jae	.LBB24_47
# %bb.46:
	movq	-104(%rbp), %rcx
	movq	-24(%rbp), %rax
	imulq	$56, -168(%rbp), %rdx
	addq	%rdx, %rax
	addq	16(%rax), %rcx
	movq	-16(%rbp), %rax
	movq	32(%rax), %rax
	movq	-24(%rbp), %rdx
	imulq	$56, -168(%rbp), %rsi
	addq	%rsi, %rdx
	subq	8(%rdx), %rax
	addq	%rax, %rcx
	movq	-32(%rbp), %rax
	movq	%rcx, 24(%rax)
	jmp	.LBB24_49
.LBB24_47:                              #   in Loop: Header=BB24_42 Depth=1
	jmp	.LBB24_48
.LBB24_48:                              #   in Loop: Header=BB24_42 Depth=1
	movq	-168(%rbp), %rax
	addq	$1, %rax
	movq	%rax, -168(%rbp)
	jmp	.LBB24_42
.LBB24_49:
	movq	-16(%rbp), %rax
	movzwl	56(%rax), %eax
	movl	%eax, %ecx
	movq	-32(%rbp), %rax
	movq	%rcx, 32(%rax)
	movq	-104(%rbp), %rax
	addq	$192, %rsp
	popq	%rbp
	retq
.Lfunc_end24:
	.size	map_object, .Lfunc_end24-map_object
                                        # -- End function
	.p2align	4                               # -- Begin function parse_dynamic
	.type	parse_dynamic,@function
parse_dynamic:                          # @parse_dynamic
# %bb.0:
	pushq	%rbp
	movq	%rsp, %rbp
	subq	$96, %rsp
	movq	%rdi, -8(%rbp)
	movq	%rsi, -16(%rbp)
	movq	$7, -24(%rbp)
	movq	-16(%rbp), %rax
	movq	%rax, -32(%rbp)
.LBB25_1:                               # =>This Inner Loop Header: Depth=1
	movq	-32(%rbp), %rax
	cmpq	$0, (%rax)
	je	.LBB25_26
# %bb.2:                                #   in Loop: Header=BB25_1 Depth=1
	movq	-32(%rbp), %rax
	movq	8(%rax), %rax
	movq	%rax, -40(%rbp)
	movq	-32(%rbp), %rax
	movq	(%rax), %rax
	movq	%rax, -88(%rbp)                 # 8-byte Spill
	subq	$1, %rax
	je	.LBB25_19
	jmp	.LBB25_53
.LBB25_53:                              #   in Loop: Header=BB25_1 Depth=1
	movq	-88(%rbp), %rax                 # 8-byte Reload
	subq	$2, %rax
	je	.LBB25_8
	jmp	.LBB25_54
.LBB25_54:                              #   in Loop: Header=BB25_1 Depth=1
	movq	-88(%rbp), %rax                 # 8-byte Reload
	subq	$4, %rax
	je	.LBB25_12
	jmp	.LBB25_55
.LBB25_55:                              #   in Loop: Header=BB25_1 Depth=1
	movq	-88(%rbp), %rax                 # 8-byte Reload
	subq	$5, %rax
	je	.LBB25_4
	jmp	.LBB25_56
.LBB25_56:                              #   in Loop: Header=BB25_1 Depth=1
	movq	-88(%rbp), %rax                 # 8-byte Reload
	subq	$6, %rax
	je	.LBB25_3
	jmp	.LBB25_57
.LBB25_57:                              #   in Loop: Header=BB25_1 Depth=1
	movq	-88(%rbp), %rax                 # 8-byte Reload
	subq	$7, %rax
	je	.LBB25_5
	jmp	.LBB25_58
.LBB25_58:                              #   in Loop: Header=BB25_1 Depth=1
	movq	-88(%rbp), %rax                 # 8-byte Reload
	subq	$8, %rax
	je	.LBB25_6
	jmp	.LBB25_59
.LBB25_59:                              #   in Loop: Header=BB25_1 Depth=1
	movq	-88(%rbp), %rax                 # 8-byte Reload
	subq	$12, %rax
	je	.LBB25_14
	jmp	.LBB25_60
.LBB25_60:                              #   in Loop: Header=BB25_1 Depth=1
	movq	-88(%rbp), %rax                 # 8-byte Reload
	subq	$14, %rax
	je	.LBB25_17
	jmp	.LBB25_61
.LBB25_61:                              #   in Loop: Header=BB25_1 Depth=1
	movq	-88(%rbp), %rax                 # 8-byte Reload
	subq	$15, %rax
	je	.LBB25_18
	jmp	.LBB25_62
.LBB25_62:                              #   in Loop: Header=BB25_1 Depth=1
	movq	-88(%rbp), %rax                 # 8-byte Reload
	subq	$20, %rax
	je	.LBB25_9
	jmp	.LBB25_63
.LBB25_63:                              #   in Loop: Header=BB25_1 Depth=1
	movq	-88(%rbp), %rax                 # 8-byte Reload
	subq	$23, %rax
	je	.LBB25_7
	jmp	.LBB25_64
.LBB25_64:                              #   in Loop: Header=BB25_1 Depth=1
	movq	-88(%rbp), %rax                 # 8-byte Reload
	subq	$25, %rax
	je	.LBB25_15
	jmp	.LBB25_65
.LBB25_65:                              #   in Loop: Header=BB25_1 Depth=1
	movq	-88(%rbp), %rax                 # 8-byte Reload
	subq	$27, %rax
	je	.LBB25_16
	jmp	.LBB25_66
.LBB25_66:                              #   in Loop: Header=BB25_1 Depth=1
	movq	-88(%rbp), %rax                 # 8-byte Reload
	subq	$29, %rax
	je	.LBB25_18
	jmp	.LBB25_67
.LBB25_67:                              #   in Loop: Header=BB25_1 Depth=1
	movq	-88(%rbp), %rax                 # 8-byte Reload
	subq	$1879047925, %rax               # imm = 0x6FFFFEF5
	je	.LBB25_13
	jmp	.LBB25_68
.LBB25_68:                              #   in Loop: Header=BB25_1 Depth=1
	movq	-88(%rbp), %rax                 # 8-byte Reload
	subq	$1879048174, %rax               # imm = 0x6FFFFFEE
	je	.LBB25_11
	jmp	.LBB25_69
.LBB25_69:                              #   in Loop: Header=BB25_1 Depth=1
	movq	-88(%rbp), %rax                 # 8-byte Reload
	subq	$1879048175, %rax               # imm = 0x6FFFFFEF
	je	.LBB25_10
	jmp	.LBB25_23
.LBB25_3:                               #   in Loop: Header=BB25_1 Depth=1
	movq	-8(%rbp), %rax
	movq	16(%rax), %rcx
	addq	-40(%rbp), %rcx
	movq	-8(%rbp), %rax
	movq	%rcx, 48(%rax)
	jmp	.LBB25_24
.LBB25_4:                               #   in Loop: Header=BB25_1 Depth=1
	movq	-8(%rbp), %rax
	movq	16(%rax), %rcx
	addq	-40(%rbp), %rcx
	movq	-8(%rbp), %rax
	movq	%rcx, 56(%rax)
	jmp	.LBB25_24
.LBB25_5:                               #   in Loop: Header=BB25_1 Depth=1
	movq	-8(%rbp), %rax
	movq	16(%rax), %rcx
	addq	-40(%rbp), %rcx
	movq	-8(%rbp), %rax
	movq	%rcx, 72(%rax)
	jmp	.LBB25_24
.LBB25_6:                               #   in Loop: Header=BB25_1 Depth=1
	movq	-40(%rbp), %rcx
	movq	-8(%rbp), %rax
	movq	%rcx, 80(%rax)
	jmp	.LBB25_24
.LBB25_7:                               #   in Loop: Header=BB25_1 Depth=1
	movq	-8(%rbp), %rax
	movq	16(%rax), %rcx
	addq	-40(%rbp), %rcx
	movq	-8(%rbp), %rax
	movq	%rcx, 88(%rax)
	jmp	.LBB25_24
.LBB25_8:                               #   in Loop: Header=BB25_1 Depth=1
	movq	-40(%rbp), %rcx
	movq	-8(%rbp), %rax
	movq	%rcx, 96(%rax)
	jmp	.LBB25_24
.LBB25_9:                               #   in Loop: Header=BB25_1 Depth=1
	movq	-40(%rbp), %rax
	movq	%rax, -24(%rbp)
	jmp	.LBB25_24
.LBB25_10:                              #   in Loop: Header=BB25_1 Depth=1
	movq	-8(%rbp), %rax
	movq	16(%rax), %rcx
	addq	-40(%rbp), %rcx
	movq	-8(%rbp), %rax
	movq	%rcx, 104(%rax)
	jmp	.LBB25_24
.LBB25_11:                              #   in Loop: Header=BB25_1 Depth=1
	movq	-40(%rbp), %rcx
	movq	-8(%rbp), %rax
	movq	%rcx, 112(%rax)
	jmp	.LBB25_24
.LBB25_12:                              #   in Loop: Header=BB25_1 Depth=1
	movq	-8(%rbp), %rax
	movq	16(%rax), %rcx
	addq	-40(%rbp), %rcx
	movq	-8(%rbp), %rax
	movq	%rcx, 120(%rax)
	jmp	.LBB25_24
.LBB25_13:                              #   in Loop: Header=BB25_1 Depth=1
	movq	-8(%rbp), %rax
	movq	16(%rax), %rcx
	addq	-40(%rbp), %rcx
	movq	-8(%rbp), %rax
	movq	%rcx, 128(%rax)
	jmp	.LBB25_24
.LBB25_14:                              #   in Loop: Header=BB25_1 Depth=1
	movq	-8(%rbp), %rax
	movq	16(%rax), %rcx
	addq	-40(%rbp), %rcx
	movq	-8(%rbp), %rax
	movq	%rcx, 136(%rax)
	jmp	.LBB25_24
.LBB25_15:                              #   in Loop: Header=BB25_1 Depth=1
	movq	-8(%rbp), %rax
	movq	16(%rax), %rcx
	addq	-40(%rbp), %rcx
	movq	-8(%rbp), %rax
	movq	%rcx, 144(%rax)
	jmp	.LBB25_24
.LBB25_16:                              #   in Loop: Header=BB25_1 Depth=1
	movq	-40(%rbp), %rcx
	movq	-8(%rbp), %rax
	movq	%rcx, 152(%rax)
	jmp	.LBB25_24
.LBB25_17:                              #   in Loop: Header=BB25_1 Depth=1
	movq	-40(%rbp), %rcx
	movq	-8(%rbp), %rax
	movq	%rcx, 8(%rax)
	jmp	.LBB25_24
.LBB25_18:                              #   in Loop: Header=BB25_1 Depth=1
	movq	-40(%rbp), %rcx
	movq	-8(%rbp), %rax
	movq	%rcx, 176(%rax)
	jmp	.LBB25_24
.LBB25_19:                              #   in Loop: Header=BB25_1 Depth=1
	movq	-8(%rbp), %rax
	cmpl	$16, 248(%rax)
	jge	.LBB25_21
# %bb.20:                               #   in Loop: Header=BB25_1 Depth=1
	movq	-40(%rbp), %rax
	movl	%eax, %edx
	movq	-8(%rbp), %rax
	movq	-8(%rbp), %rsi
	movl	248(%rsi), %ecx
	movl	%ecx, %edi
	addl	$1, %edi
	movl	%edi, 248(%rsi)
	movslq	%ecx, %rcx
	movl	%edx, 184(%rax,%rcx,4)
	jmp	.LBB25_22
.LBB25_21:
	leaq	.L.str.26(%rip), %rdi
	callq	die
.LBB25_22:                              #   in Loop: Header=BB25_1 Depth=1
	jmp	.LBB25_24
.LBB25_23:                              #   in Loop: Header=BB25_1 Depth=1
	jmp	.LBB25_24
.LBB25_24:                              #   in Loop: Header=BB25_1 Depth=1
	jmp	.LBB25_25
.LBB25_25:                              #   in Loop: Header=BB25_1 Depth=1
	movq	-32(%rbp), %rax
	addq	$16, %rax
	movq	%rax, -32(%rbp)
	jmp	.LBB25_1
.LBB25_26:
	movq	-8(%rbp), %rax
	cmpq	$0, 88(%rax)
	je	.LBB25_29
# %bb.27:
	cmpq	$7, -24(%rbp)
	je	.LBB25_29
# %bb.28:
	leaq	.L.str.27(%rip), %rdi
	callq	die
.LBB25_29:
	movq	-8(%rbp), %rax
	cmpq	$0, 56(%rax)
	je	.LBB25_35
# %bb.30:
	movq	-8(%rbp), %rax
	cmpq	$0, 8(%rax)
	je	.LBB25_32
# %bb.31:
	movq	-8(%rbp), %rax
	movq	56(%rax), %rcx
	movq	-8(%rbp), %rax
	movq	8(%rax), %rax
	addq	%rax, %rcx
	movq	-8(%rbp), %rax
	movq	%rcx, 8(%rax)
.LBB25_32:
	movq	-8(%rbp), %rax
	cmpq	$0, 176(%rax)
	je	.LBB25_34
# %bb.33:
	movq	-8(%rbp), %rax
	movq	56(%rax), %rcx
	movq	-8(%rbp), %rax
	movq	176(%rax), %rax
	addq	%rax, %rcx
	movq	-8(%rbp), %rax
	movq	%rcx, 176(%rax)
.LBB25_34:
	jmp	.LBB25_35
.LBB25_35:
	movq	-8(%rbp), %rax
	cmpq	$0, 120(%rax)
	je	.LBB25_37
# %bb.36:
	movq	-8(%rbp), %rax
	movq	120(%rax), %rax
	movl	4(%rax), %eax
	movl	%eax, %ecx
	movq	-8(%rbp), %rax
	movq	%rcx, 64(%rax)
	jmp	.LBB25_52
.LBB25_37:
	movq	-8(%rbp), %rax
	cmpq	$0, 128(%rax)
	je	.LBB25_51
# %bb.38:
	movq	-8(%rbp), %rax
	movq	128(%rax), %rax
	movl	(%rax), %eax
	movl	%eax, -44(%rbp)
	movq	-8(%rbp), %rax
	movq	128(%rax), %rax
	movl	4(%rax), %eax
	movl	%eax, -48(%rbp)
	movq	-8(%rbp), %rax
	movq	128(%rax), %rax
	movl	8(%rax), %eax
	movl	%eax, -52(%rbp)
	movq	-8(%rbp), %rax
	movq	128(%rax), %rax
	addq	$16, %rax
	movl	-52(%rbp), %ecx
                                        # kill: def $rcx killed $ecx
	shlq	$3, %rcx
	addq	%rcx, %rax
	movq	%rax, -64(%rbp)
	movq	-64(%rbp), %rax
	movl	-44(%rbp), %ecx
                                        # kill: def $rcx killed $ecx
	shlq	$2, %rcx
	addq	%rcx, %rax
	movq	%rax, -72(%rbp)
	movl	$0, -76(%rbp)
	movl	$0, -80(%rbp)
.LBB25_39:                              # =>This Inner Loop Header: Depth=1
	movl	-80(%rbp), %eax
	cmpl	-44(%rbp), %eax
	jae	.LBB25_44
# %bb.40:                               #   in Loop: Header=BB25_39 Depth=1
	movq	-64(%rbp), %rax
	movl	-80(%rbp), %ecx
                                        # kill: def $rcx killed $ecx
	movl	(%rax,%rcx,4), %eax
	cmpl	-76(%rbp), %eax
	jbe	.LBB25_42
# %bb.41:                               #   in Loop: Header=BB25_39 Depth=1
	movq	-64(%rbp), %rax
	movl	-80(%rbp), %ecx
                                        # kill: def $rcx killed $ecx
	movl	(%rax,%rcx,4), %eax
	movl	%eax, -76(%rbp)
.LBB25_42:                              #   in Loop: Header=BB25_39 Depth=1
	jmp	.LBB25_43
.LBB25_43:                              #   in Loop: Header=BB25_39 Depth=1
	movl	-80(%rbp), %eax
	addl	$1, %eax
	movl	%eax, -80(%rbp)
	jmp	.LBB25_39
.LBB25_44:
	movl	-76(%rbp), %eax
	cmpl	-48(%rbp), %eax
	jae	.LBB25_46
# %bb.45:
	movl	-48(%rbp), %eax
	movl	%eax, %ecx
	movq	-8(%rbp), %rax
	movq	%rcx, 64(%rax)
	jmp	.LBB25_50
.LBB25_46:
	jmp	.LBB25_47
.LBB25_47:                              # =>This Inner Loop Header: Depth=1
	movq	-72(%rbp), %rax
	movl	-76(%rbp), %ecx
	subl	-48(%rbp), %ecx
	movl	%ecx, %ecx
                                        # kill: def $rcx killed $ecx
	movl	(%rax,%rcx,4), %eax
	andl	$1, %eax
	cmpl	$0, %eax
	setne	%al
	xorb	$-1, %al
	testb	$1, %al
	jne	.LBB25_48
	jmp	.LBB25_49
.LBB25_48:                              #   in Loop: Header=BB25_47 Depth=1
	movl	-76(%rbp), %eax
	addl	$1, %eax
	movl	%eax, -76(%rbp)
	jmp	.LBB25_47
.LBB25_49:
	movl	-76(%rbp), %eax
	addl	$1, %eax
	movl	%eax, %eax
	movl	%eax, %ecx
	movq	-8(%rbp), %rax
	movq	%rcx, 64(%rax)
.LBB25_50:
	jmp	.LBB25_51
.LBB25_51:
	jmp	.LBB25_52
.LBB25_52:
	addq	$96, %rsp
	popq	%rbp
	retq
.Lfunc_end25:
	.size	parse_dynamic, .Lfunc_end25-parse_dynamic
                                        # -- End function
	.p2align	4                               # -- Begin function align_down
	.type	align_down,@function
align_down:                             # @align_down
# %bb.0:
	pushq	%rbp
	movq	%rsp, %rbp
	movq	%rdi, -8(%rbp)
	movq	%rsi, -16(%rbp)
	movq	-8(%rbp), %rax
	movq	-16(%rbp), %rcx
	subq	$1, %rcx
	xorq	$-1, %rcx
	andq	%rcx, %rax
	popq	%rbp
	retq
.Lfunc_end26:
	.size	align_down, .Lfunc_end26-align_down
                                        # -- End function
	.p2align	4                               # -- Begin function align_up
	.type	align_up,@function
align_up:                               # @align_up
# %bb.0:
	pushq	%rbp
	movq	%rsp, %rbp
	movq	%rdi, -8(%rbp)
	movq	%rsi, -16(%rbp)
	movq	-8(%rbp), %rax
	addq	-16(%rbp), %rax
	subq	$1, %rax
	movq	-16(%rbp), %rcx
	subq	$1, %rcx
	xorq	$-1, %rcx
	andq	%rcx, %rax
	popq	%rbp
	retq
.Lfunc_end27:
	.size	align_up, .Lfunc_end27-align_up
                                        # -- End function
	.p2align	4                               # -- Begin function sys_mmap
	.type	sys_mmap,@function
sys_mmap:                               # @sys_mmap
# %bb.0:
	pushq	%rbp
	movq	%rsp, %rbp
	subq	$64, %rsp
	movq	%rdi, -8(%rbp)
	movq	%rsi, -16(%rbp)
	movq	%rdx, -24(%rbp)
	movq	%rcx, -32(%rbp)
	movq	%r8, -40(%rbp)
	movq	%r9, -48(%rbp)
	movq	-8(%rbp), %rsi
	movq	-16(%rbp), %rdx
	movq	-24(%rbp), %rcx
	movq	-32(%rbp), %r8
	movq	-40(%rbp), %r9
	movq	-48(%rbp), %rax
	movl	$9, %edi
	movq	%rax, (%rsp)
	callq	syscall6
	addq	$64, %rsp
	popq	%rbp
	retq
.Lfunc_end28:
	.size	sys_mmap, .Lfunc_end28-sys_mmap
                                        # -- End function
	.p2align	4                               # -- Begin function mmap_failed
	.type	mmap_failed,@function
mmap_failed:                            # @mmap_failed
# %bb.0:
	pushq	%rbp
	movq	%rsp, %rbp
	movq	%rdi, -8(%rbp)
	cmpq	$-4096, -8(%rbp)                # imm = 0xF000
	seta	%al
	andb	$1, %al
	movzbl	%al, %eax
	popq	%rbp
	retq
.Lfunc_end29:
	.size	mmap_failed, .Lfunc_end29-mmap_failed
                                        # -- End function
	.p2align	4                               # -- Begin function trace2
	.type	trace2,@function
trace2:                                 # @trace2
# %bb.0:
	pushq	%rbp
	movq	%rsp, %rbp
	subq	$16, %rsp
	movq	%rdi, -8(%rbp)
	movq	%rsi, -16(%rbp)
	cmpl	$0, g_debug(%rip)
	je	.LBB30_2
# %bb.1:
	leaq	.L.str.7(%rip), %rdi
	callq	dstr
	movq	-8(%rbp), %rdi
	callq	dstr
	leaq	.L.str.9(%rip), %rdi
	callq	dstr
	movq	-16(%rbp), %rdi
	callq	dhex
	leaq	.L.str.5(%rip), %rdi
	callq	dstr
.LBB30_2:
	addq	$16, %rsp
	popq	%rbp
	retq
.Lfunc_end30:
	.size	trace2, .Lfunc_end30-trace2
                                        # -- End function
	.p2align	4                               # -- Begin function sys_write
	.type	sys_write,@function
sys_write:                              # @sys_write
# %bb.0:
	pushq	%rbp
	movq	%rsp, %rbp
	subq	$32, %rsp
	movl	%edi, -4(%rbp)
	movq	%rsi, -16(%rbp)
	movq	%rdx, -24(%rbp)
	movslq	-4(%rbp), %rsi
	movq	-16(%rbp), %rdx
	movq	-24(%rbp), %rcx
	movl	$1, %edi
	callq	syscall3
	addq	$32, %rsp
	popq	%rbp
	retq
.Lfunc_end31:
	.size	sys_write, .Lfunc_end31-sys_write
                                        # -- End function
	.p2align	4                               # -- Begin function kstrlen
	.type	kstrlen,@function
kstrlen:                                # @kstrlen
# %bb.0:
	pushq	%rbp
	movq	%rsp, %rbp
	movq	%rdi, -8(%rbp)
	movq	-8(%rbp), %rax
	movq	%rax, -16(%rbp)
.LBB32_1:                               # =>This Inner Loop Header: Depth=1
	movq	-16(%rbp), %rax
	cmpb	$0, (%rax)
	je	.LBB32_3
# %bb.2:                                #   in Loop: Header=BB32_1 Depth=1
	movq	-16(%rbp), %rax
	addq	$1, %rax
	movq	%rax, -16(%rbp)
	jmp	.LBB32_1
.LBB32_3:
	movq	-16(%rbp), %rax
	movq	-8(%rbp), %rcx
	subq	%rcx, %rax
	popq	%rbp
	retq
.Lfunc_end32:
	.size	kstrlen, .Lfunc_end32-kstrlen
                                        # -- End function
	.p2align	4                               # -- Begin function syscall3
	.type	syscall3,@function
syscall3:                               # @syscall3
# %bb.0:
	pushq	%rbp
	movq	%rsp, %rbp
	subq	$48, %rsp
	movq	%rdi, -8(%rbp)
	movq	%rsi, -16(%rbp)
	movq	%rdx, -24(%rbp)
	movq	%rcx, -32(%rbp)
	movq	-8(%rbp), %rdi
	movq	-16(%rbp), %rsi
	movq	-24(%rbp), %rdx
	movq	-32(%rbp), %rcx
	xorl	%eax, %eax
	movl	%eax, %r9d
	movq	%r9, %r8
	movq	$0, (%rsp)
	callq	syscall6
	addq	$48, %rsp
	popq	%rbp
	retq
.Lfunc_end33:
	.size	syscall3, .Lfunc_end33-syscall3
                                        # -- End function
	.p2align	4                               # -- Begin function already_loaded
	.type	already_loaded,@function
already_loaded:                         # @already_loaded
# %bb.0:
	pushq	%rbp
	movq	%rsp, %rbp
	subq	$32, %rsp
	movq	%rdi, -16(%rbp)
	movl	$0, -20(%rbp)
.LBB34_1:                               # =>This Inner Loop Header: Depth=1
	movl	-20(%rbp), %eax
	cmpl	g_nobjs(%rip), %eax
	jge	.LBB34_10
# %bb.2:                                #   in Loop: Header=BB34_1 Depth=1
	movslq	-20(%rbp), %rcx
	leaq	g_objs(%rip), %rax
	imulq	$264, %rcx, %rcx                # imm = 0x108
	addq	%rcx, %rax
	cmpq	$0, 8(%rax)
	je	.LBB34_5
# %bb.3:                                #   in Loop: Header=BB34_1 Depth=1
	movslq	-20(%rbp), %rcx
	leaq	g_objs(%rip), %rax
	imulq	$264, %rcx, %rcx                # imm = 0x108
	addq	%rcx, %rax
	movq	8(%rax), %rdi
	movq	-16(%rbp), %rsi
	callq	kstreq
	cmpl	$0, %eax
	je	.LBB34_5
# %bb.4:
	movslq	-20(%rbp), %rcx
	leaq	g_objs(%rip), %rax
	imulq	$264, %rcx, %rcx                # imm = 0x108
	addq	%rcx, %rax
	movq	%rax, -8(%rbp)
	jmp	.LBB34_11
.LBB34_5:                               #   in Loop: Header=BB34_1 Depth=1
	movslq	-20(%rbp), %rcx
	leaq	g_objs(%rip), %rax
	imulq	$264, %rcx, %rcx                # imm = 0x108
	addq	%rcx, %rax
	cmpq	$0, (%rax)
	je	.LBB34_8
# %bb.6:                                #   in Loop: Header=BB34_1 Depth=1
	movslq	-20(%rbp), %rcx
	leaq	g_objs(%rip), %rax
	imulq	$264, %rcx, %rcx                # imm = 0x108
	addq	%rcx, %rax
	movq	(%rax), %rdi
	movq	-16(%rbp), %rsi
	callq	kstreq
	cmpl	$0, %eax
	je	.LBB34_8
# %bb.7:
	movslq	-20(%rbp), %rcx
	leaq	g_objs(%rip), %rax
	imulq	$264, %rcx, %rcx                # imm = 0x108
	addq	%rcx, %rax
	movq	%rax, -8(%rbp)
	jmp	.LBB34_11
.LBB34_8:                               #   in Loop: Header=BB34_1 Depth=1
	jmp	.LBB34_9
.LBB34_9:                               #   in Loop: Header=BB34_1 Depth=1
	movl	-20(%rbp), %eax
	addl	$1, %eax
	movl	%eax, -20(%rbp)
	jmp	.LBB34_1
.LBB34_10:
	movq	$0, -8(%rbp)
.LBB34_11:
	movq	-8(%rbp), %rax
	addq	$32, %rsp
	popq	%rbp
	retq
.Lfunc_end34:
	.size	already_loaded, .Lfunc_end34-already_loaded
                                        # -- End function
	.p2align	4                               # -- Begin function try_pathlist
	.type	try_pathlist,@function
try_pathlist:                           # @try_pathlist
# %bb.0:
	pushq	%rbp
	movq	%rsp, %rbp
	subq	$64, %rsp
	movq	%rdi, -16(%rbp)
	movq	%rsi, -24(%rbp)
	cmpq	$0, -16(%rbp)
	jne	.LBB35_2
# %bb.1:
	movq	$0, -8(%rbp)
	jmp	.LBB35_15
.LBB35_2:
	movq	-16(%rbp), %rax
	movq	%rax, -32(%rbp)
.LBB35_3:                               # =>This Loop Header: Depth=1
                                        #     Child Loop BB35_5 Depth 2
	movq	-32(%rbp), %rax
	cmpb	$0, (%rax)
	je	.LBB35_14
# %bb.4:                                #   in Loop: Header=BB35_3 Depth=1
	movq	-32(%rbp), %rax
	movq	%rax, -40(%rbp)
.LBB35_5:                               #   Parent Loop BB35_3 Depth=1
                                        # =>  This Inner Loop Header: Depth=2
	movq	-32(%rbp), %rax
	movsbl	(%rax), %ecx
	xorl	%eax, %eax
                                        # kill: def $al killed $al killed $eax
	cmpl	$0, %ecx
	movb	%al, -49(%rbp)                  # 1-byte Spill
	je	.LBB35_7
# %bb.6:                                #   in Loop: Header=BB35_5 Depth=2
	movq	-32(%rbp), %rax
	movsbl	(%rax), %eax
	cmpl	$58, %eax
	setne	%al
	movb	%al, -49(%rbp)                  # 1-byte Spill
.LBB35_7:                               #   in Loop: Header=BB35_5 Depth=2
	movb	-49(%rbp), %al                  # 1-byte Reload
	testb	$1, %al
	jne	.LBB35_8
	jmp	.LBB35_9
.LBB35_8:                               #   in Loop: Header=BB35_5 Depth=2
	movq	-32(%rbp), %rax
	addq	$1, %rax
	movq	%rax, -32(%rbp)
	jmp	.LBB35_5
.LBB35_9:                               #   in Loop: Header=BB35_3 Depth=1
	movq	-40(%rbp), %rdi
	movq	-32(%rbp), %rsi
	movq	-40(%rbp), %rax
	subq	%rax, %rsi
	movq	-24(%rbp), %rdx
	callq	try_dir
	movq	%rax, -48(%rbp)
	cmpq	$0, -48(%rbp)
	je	.LBB35_11
# %bb.10:
	movq	-48(%rbp), %rax
	movq	%rax, -8(%rbp)
	jmp	.LBB35_15
.LBB35_11:                              #   in Loop: Header=BB35_3 Depth=1
	movq	-32(%rbp), %rax
	movsbl	(%rax), %eax
	cmpl	$58, %eax
	jne	.LBB35_13
# %bb.12:                               #   in Loop: Header=BB35_3 Depth=1
	movq	-32(%rbp), %rax
	addq	$1, %rax
	movq	%rax, -32(%rbp)
.LBB35_13:                              #   in Loop: Header=BB35_3 Depth=1
	jmp	.LBB35_3
.LBB35_14:
	movq	$0, -8(%rbp)
.LBB35_15:
	movq	-8(%rbp), %rax
	addq	$64, %rsp
	popq	%rbp
	retq
.Lfunc_end35:
	.size	try_pathlist, .Lfunc_end35-try_pathlist
                                        # -- End function
	.p2align	4                               # -- Begin function try_dir
	.type	try_dir,@function
try_dir:                                # @try_dir
# %bb.0:
	pushq	%rbp
	movq	%rsp, %rbp
	subq	$4176, %rsp                     # imm = 0x1050
	movq	%rdi, -16(%rbp)
	movq	%rsi, -24(%rbp)
	movq	%rdx, -32(%rbp)
	movq	$0, -4136(%rbp)
	movq	$0, -4144(%rbp)
.LBB36_1:                               # =>This Inner Loop Header: Depth=1
	movq	-4144(%rbp), %rcx
	xorl	%eax, %eax
                                        # kill: def $al killed $al killed $eax
	cmpq	-24(%rbp), %rcx
	movb	%al, -4169(%rbp)                # 1-byte Spill
	jae	.LBB36_3
# %bb.2:                                #   in Loop: Header=BB36_1 Depth=1
	movq	-4136(%rbp), %rax
	addq	$1, %rax
	cmpq	$4096, %rax                     # imm = 0x1000
	setb	%al
	movb	%al, -4169(%rbp)                # 1-byte Spill
.LBB36_3:                               #   in Loop: Header=BB36_1 Depth=1
	movb	-4169(%rbp), %al                # 1-byte Reload
	testb	$1, %al
	jne	.LBB36_4
	jmp	.LBB36_6
.LBB36_4:                               #   in Loop: Header=BB36_1 Depth=1
	movq	-16(%rbp), %rax
	movq	-4144(%rbp), %rcx
	movb	(%rax,%rcx), %cl
	movq	-4136(%rbp), %rax
	movq	%rax, %rdx
	addq	$1, %rdx
	movq	%rdx, -4136(%rbp)
	movb	%cl, -4128(%rbp,%rax)
# %bb.5:                                #   in Loop: Header=BB36_1 Depth=1
	movq	-4144(%rbp), %rax
	addq	$1, %rax
	movq	%rax, -4144(%rbp)
	jmp	.LBB36_1
.LBB36_6:
	cmpq	$0, -4136(%rbp)
	je	.LBB36_10
# %bb.7:
	movq	-4136(%rbp), %rax
	subq	$1, %rax
	movsbl	-4128(%rbp,%rax), %eax
	cmpl	$47, %eax
	je	.LBB36_10
# %bb.8:
	movq	-4136(%rbp), %rax
	addq	$1, %rax
	cmpq	$4096, %rax                     # imm = 0x1000
	jae	.LBB36_10
# %bb.9:
	movq	-4136(%rbp), %rax
	movq	%rax, %rcx
	addq	$1, %rcx
	movq	%rcx, -4136(%rbp)
	movb	$47, -4128(%rbp,%rax)
.LBB36_10:
	leaq	-4128(%rbp), %rdi
	addq	-4136(%rbp), %rdi
	movl	$4096, %esi                     # imm = 0x1000
	subq	-4136(%rbp), %rsi
	movq	-32(%rbp), %rdx
	callq	kstrcpy
	addq	-4136(%rbp), %rax
	movq	%rax, -4136(%rbp)
	movq	-4136(%rbp), %rax
	movb	$0, -4128(%rbp,%rax)
	leaq	-4128(%rbp), %rdi
	callq	sys_openat
	movq	%rax, -4152(%rbp)
	cmpq	$0, -4152(%rbp)
	jge	.LBB36_12
# %bb.11:
	movq	$0, -8(%rbp)
	jmp	.LBB36_15
.LBB36_12:
	movq	-32(%rbp), %rax
	movq	%rax, -4160(%rbp)
	cmpl	$32, try_dir.nnames(%rip)
	jge	.LBB36_14
# %bb.13:
	movslq	try_dir.nnames(%rip), %rax
	leaq	try_dir.names(%rip), %rdi
	shlq	$12, %rax
	addq	%rax, %rdi
	leaq	-4128(%rbp), %rdx
	movl	$4096, %esi                     # imm = 0x1000
	callq	kstrcpy
	movl	try_dir.nnames(%rip), %eax
	movl	%eax, %ecx
	addl	$1, %ecx
	movl	%ecx, try_dir.nnames(%rip)
	movslq	%eax, %rcx
	leaq	try_dir.names(%rip), %rax
	shlq	$12, %rcx
	addq	%rcx, %rax
	movq	%rax, -4160(%rbp)
.LBB36_14:
	movq	-4152(%rbp), %rax
	movl	%eax, %edi
	movq	-4160(%rbp), %rsi
	callq	load_from_fd
	movq	%rax, -4168(%rbp)
	movq	-4152(%rbp), %rax
	movl	%eax, %edi
	callq	sys_close
	movq	-4168(%rbp), %rax
	movq	%rax, -8(%rbp)
.LBB36_15:
	movq	-8(%rbp), %rax
	addq	$4176, %rsp                     # imm = 0x1050
	popq	%rbp
	retq
.Lfunc_end36:
	.size	try_dir, .Lfunc_end36-try_dir
                                        # -- End function
	.p2align	4                               # -- Begin function kstrcpy
	.type	kstrcpy,@function
kstrcpy:                                # @kstrcpy
# %bb.0:
	pushq	%rbp
	movq	%rsp, %rbp
	movq	%rdi, -8(%rbp)
	movq	%rsi, -16(%rbp)
	movq	%rdx, -24(%rbp)
	movq	$0, -32(%rbp)
.LBB37_1:                               # =>This Inner Loop Header: Depth=1
	movq	-24(%rbp), %rax
	movq	-32(%rbp), %rcx
	movsbl	(%rax,%rcx), %ecx
	xorl	%eax, %eax
                                        # kill: def $al killed $al killed $eax
	cmpl	$0, %ecx
	movb	%al, -33(%rbp)                  # 1-byte Spill
	je	.LBB37_3
# %bb.2:                                #   in Loop: Header=BB37_1 Depth=1
	movq	-32(%rbp), %rax
	addq	$1, %rax
	cmpq	-16(%rbp), %rax
	setb	%al
	movb	%al, -33(%rbp)                  # 1-byte Spill
.LBB37_3:                               #   in Loop: Header=BB37_1 Depth=1
	movb	-33(%rbp), %al                  # 1-byte Reload
	testb	$1, %al
	jne	.LBB37_4
	jmp	.LBB37_5
.LBB37_4:                               #   in Loop: Header=BB37_1 Depth=1
	movq	-24(%rbp), %rax
	movq	-32(%rbp), %rcx
	movb	(%rax,%rcx), %dl
	movq	-8(%rbp), %rax
	movq	-32(%rbp), %rcx
	movb	%dl, (%rax,%rcx)
	movq	-32(%rbp), %rax
	addq	$1, %rax
	movq	%rax, -32(%rbp)
	jmp	.LBB37_1
.LBB37_5:
	movq	-8(%rbp), %rax
	movq	-32(%rbp), %rcx
	movb	$0, (%rax,%rcx)
	movq	-32(%rbp), %rax
	popq	%rbp
	retq
.Lfunc_end37:
	.size	kstrcpy, .Lfunc_end37-kstrcpy
                                        # -- End function
	.p2align	4                               # -- Begin function apply_relr
	.type	apply_relr,@function
apply_relr:                             # @apply_relr
# %bb.0:
	pushq	%rbp
	movq	%rsp, %rbp
	movq	%rdi, -8(%rbp)
	movq	-8(%rbp), %rax
	cmpq	$0, 104(%rax)
	jne	.LBB38_2
# %bb.1:
	jmp	.LBB38_15
.LBB38_2:
	movq	-8(%rbp), %rax
	movq	112(%rax), %rax
	shrq	$3, %rax
	movq	%rax, -16(%rbp)
	movq	$0, -24(%rbp)
	movq	$0, -32(%rbp)
.LBB38_3:                               # =>This Loop Header: Depth=1
                                        #     Child Loop BB38_7 Depth 2
	movq	-32(%rbp), %rax
	cmpq	-16(%rbp), %rax
	jae	.LBB38_15
# %bb.4:                                #   in Loop: Header=BB38_3 Depth=1
	movq	-8(%rbp), %rax
	movq	104(%rax), %rax
	movq	-32(%rbp), %rcx
	movq	(%rax,%rcx,8), %rax
	movq	%rax, -40(%rbp)
	movq	-40(%rbp), %rax
	andq	$1, %rax
	cmpq	$0, %rax
	jne	.LBB38_6
# %bb.5:                                #   in Loop: Header=BB38_3 Depth=1
	movq	-8(%rbp), %rax
	movq	16(%rax), %rax
	addq	-40(%rbp), %rax
	movq	%rax, -24(%rbp)
	movq	-8(%rbp), %rax
	movq	16(%rax), %rcx
	movq	-24(%rbp), %rax
	addq	(%rax), %rcx
	movq	%rcx, (%rax)
	movq	-24(%rbp), %rax
	addq	$8, %rax
	movq	%rax, -24(%rbp)
	jmp	.LBB38_13
.LBB38_6:                               #   in Loop: Header=BB38_3 Depth=1
	movq	-24(%rbp), %rax
	movq	%rax, -48(%rbp)
	movq	-40(%rbp), %rax
	shrq	%rax
	movq	%rax, -56(%rbp)
.LBB38_7:                               #   Parent Loop BB38_3 Depth=1
                                        # =>  This Inner Loop Header: Depth=2
	cmpq	$0, -56(%rbp)
	je	.LBB38_12
# %bb.8:                                #   in Loop: Header=BB38_7 Depth=2
	movq	-56(%rbp), %rax
	andq	$1, %rax
	cmpq	$0, %rax
	je	.LBB38_10
# %bb.9:                                #   in Loop: Header=BB38_7 Depth=2
	movq	-8(%rbp), %rax
	movq	16(%rax), %rcx
	movq	-48(%rbp), %rax
	addq	(%rax), %rcx
	movq	%rcx, (%rax)
.LBB38_10:                              #   in Loop: Header=BB38_7 Depth=2
	jmp	.LBB38_11
.LBB38_11:                              #   in Loop: Header=BB38_7 Depth=2
	movq	-56(%rbp), %rax
	shrq	%rax
	movq	%rax, -56(%rbp)
	movq	-48(%rbp), %rax
	addq	$8, %rax
	movq	%rax, -48(%rbp)
	jmp	.LBB38_7
.LBB38_12:                              #   in Loop: Header=BB38_3 Depth=1
	movq	-24(%rbp), %rax
	addq	$504, %rax                      # imm = 0x1F8
	movq	%rax, -24(%rbp)
.LBB38_13:                              #   in Loop: Header=BB38_3 Depth=1
	jmp	.LBB38_14
.LBB38_14:                              #   in Loop: Header=BB38_3 Depth=1
	movq	-32(%rbp), %rax
	addq	$1, %rax
	movq	%rax, -32(%rbp)
	jmp	.LBB38_3
.LBB38_15:
	popq	%rbp
	retq
.Lfunc_end38:
	.size	apply_relr, .Lfunc_end38-apply_relr
                                        # -- End function
	.p2align	4                               # -- Begin function apply_rela
	.type	apply_rela,@function
apply_rela:                             # @apply_rela
# %bb.0:
	pushq	%rbp
	movq	%rsp, %rbp
	subq	$112, %rsp
	movq	%rdi, -8(%rbp)
	movq	%rsi, -16(%rbp)
	movq	%rdx, -24(%rbp)
	movq	$0, -32(%rbp)
.LBB39_1:                               # =>This Inner Loop Header: Depth=1
	movq	-32(%rbp), %rax
	cmpq	-24(%rbp), %rax
	jae	.LBB39_16
# %bb.2:                                #   in Loop: Header=BB39_1 Depth=1
	movq	-16(%rbp), %rax
	movq	-32(%rbp), %rcx
	leaq	(%rcx,%rcx,2), %rcx
	leaq	(%rax,%rcx,8), %rax
	movq	%rax, -40(%rbp)
	movq	-40(%rbp), %rax
	movl	8(%rax), %eax
                                        # kill: def $rax killed $eax
	movq	%rax, -48(%rbp)
	movq	-8(%rbp), %rax
	movq	16(%rax), %rax
	movq	-40(%rbp), %rcx
	movq	(%rcx), %rcx
	addq	%rcx, %rax
	movq	%rax, -56(%rbp)
	movq	-40(%rbp), %rax
	movq	16(%rax), %rax
	movq	%rax, -64(%rbp)
	movq	-48(%rbp), %rax
	movq	%rax, -104(%rbp)                # 8-byte Spill
	testq	%rax, %rax
	je	.LBB39_3
	jmp	.LBB39_17
.LBB39_17:                              #   in Loop: Header=BB39_1 Depth=1
	movq	-104(%rbp), %rax                # 8-byte Reload
	subq	$1, %rax
	je	.LBB39_5
	jmp	.LBB39_18
.LBB39_18:                              #   in Loop: Header=BB39_1 Depth=1
	movq	-104(%rbp), %rax                # 8-byte Reload
	subq	$5, %rax
	je	.LBB39_9
	jmp	.LBB39_19
.LBB39_19:                              #   in Loop: Header=BB39_1 Depth=1
	movq	-104(%rbp), %rax                # 8-byte Reload
	subq	$6, %rax
	je	.LBB39_6
	jmp	.LBB39_20
.LBB39_20:                              #   in Loop: Header=BB39_1 Depth=1
	movq	-104(%rbp), %rax                # 8-byte Reload
	subq	$7, %rax
	je	.LBB39_7
	jmp	.LBB39_21
.LBB39_21:                              #   in Loop: Header=BB39_1 Depth=1
	movq	-104(%rbp), %rax                # 8-byte Reload
	subq	$8, %rax
	je	.LBB39_4
	jmp	.LBB39_22
.LBB39_22:                              #   in Loop: Header=BB39_1 Depth=1
	movq	-104(%rbp), %rax                # 8-byte Reload
	addq	$-16, %rax
	subq	$3, %rax
	jb	.LBB39_12
	jmp	.LBB39_23
.LBB39_23:                              #   in Loop: Header=BB39_1 Depth=1
	movq	-104(%rbp), %rax                # 8-byte Reload
	subq	$37, %rax
	je	.LBB39_8
	jmp	.LBB39_13
.LBB39_3:                               #   in Loop: Header=BB39_1 Depth=1
	jmp	.LBB39_14
.LBB39_4:                               #   in Loop: Header=BB39_1 Depth=1
	movq	-8(%rbp), %rax
	movq	16(%rax), %rcx
	addq	-64(%rbp), %rcx
	movq	-56(%rbp), %rax
	movq	%rcx, (%rax)
	jmp	.LBB39_14
.LBB39_5:                               #   in Loop: Header=BB39_1 Depth=1
	movq	-8(%rbp), %rdi
	movq	-40(%rbp), %rsi
	callq	resolve_reloc_symbol
	movq	%rax, %rcx
	addq	-64(%rbp), %rcx
	movq	-56(%rbp), %rax
	movq	%rcx, (%rax)
	jmp	.LBB39_14
.LBB39_6:                               #   in Loop: Header=BB39_1 Depth=1
	movq	-8(%rbp), %rdi
	movq	-40(%rbp), %rsi
	callq	resolve_reloc_symbol
	movq	%rax, %rcx
	addq	-64(%rbp), %rcx
	movq	-56(%rbp), %rax
	movq	%rcx, (%rax)
	jmp	.LBB39_14
.LBB39_7:                               #   in Loop: Header=BB39_1 Depth=1
	movq	-8(%rbp), %rdi
	movq	-40(%rbp), %rsi
	callq	resolve_reloc_symbol
	movq	%rax, %rcx
	addq	-64(%rbp), %rcx
	movq	-56(%rbp), %rax
	movq	%rcx, (%rax)
	jmp	.LBB39_14
.LBB39_8:                               #   in Loop: Header=BB39_1 Depth=1
	movq	-8(%rbp), %rax
	movq	16(%rax), %rax
	addq	-64(%rbp), %rax
	callq	*%rax
	movq	%rax, %rcx
	movq	-56(%rbp), %rax
	movq	%rcx, (%rax)
	jmp	.LBB39_14
.LBB39_9:                               #   in Loop: Header=BB39_1 Depth=1
	movq	-40(%rbp), %rax
	movq	8(%rax), %rax
	shrq	$32, %rax
	movq	%rax, -72(%rbp)
	movq	-8(%rbp), %rax
	movq	56(%rax), %rax
	movq	-8(%rbp), %rcx
	movq	48(%rcx), %rcx
	imulq	$24, -72(%rbp), %rdx
	addq	%rdx, %rcx
	movl	(%rcx), %ecx
                                        # kill: def $rcx killed $ecx
	addq	%rcx, %rax
	movq	%rax, -80(%rbp)
	movq	-80(%rbp), %rdi
	leaq	-96(%rbp), %rsi
	callq	global_lookup
	cmpl	$0, %eax
	jne	.LBB39_11
# %bb.10:
	leaq	.L.str.31(%rip), %rdi
	callq	die
.LBB39_11:                              #   in Loop: Header=BB39_1 Depth=1
	movq	-56(%rbp), %rdi
	movq	-96(%rbp), %rax
	movq	16(%rax), %rsi
	movq	-88(%rbp), %rax
	addq	8(%rax), %rsi
	movq	-88(%rbp), %rax
	movq	16(%rax), %rdx
	callq	memcpy@PLT
	jmp	.LBB39_14
.LBB39_12:
	leaq	.L.str.32(%rip), %rdi
	callq	die
.LBB39_13:
	movq	-48(%rbp), %rsi
	leaq	.L.str.33(%rip), %rdi
	callq	die2
.LBB39_14:                              #   in Loop: Header=BB39_1 Depth=1
	jmp	.LBB39_15
.LBB39_15:                              #   in Loop: Header=BB39_1 Depth=1
	movq	-32(%rbp), %rax
	addq	$1, %rax
	movq	%rax, -32(%rbp)
	jmp	.LBB39_1
.LBB39_16:
	addq	$112, %rsp
	popq	%rbp
	retq
.Lfunc_end39:
	.size	apply_rela, .Lfunc_end39-apply_rela
                                        # -- End function
	.p2align	4                               # -- Begin function resolve_reloc_symbol
	.type	resolve_reloc_symbol,@function
resolve_reloc_symbol:                   # @resolve_reloc_symbol
# %bb.0:
	pushq	%rbp
	movq	%rsp, %rbp
	subq	$64, %rsp
	movq	%rdi, -16(%rbp)
	movq	%rsi, -24(%rbp)
	movq	-24(%rbp), %rax
	movq	8(%rax), %rax
	shrq	$32, %rax
	movq	%rax, -32(%rbp)
	movq	-16(%rbp), %rax
	movq	48(%rax), %rax
	imulq	$24, -32(%rbp), %rcx
	addq	%rcx, %rax
	movq	%rax, -40(%rbp)
	movq	-16(%rbp), %rax
	movq	56(%rax), %rax
	movq	-40(%rbp), %rcx
	movl	(%rcx), %ecx
                                        # kill: def $rcx killed $ecx
	addq	%rcx, %rax
	movq	%rax, -48(%rbp)
	movq	-40(%rbp), %rax
	movzbl	4(%rax), %eax
	sarl	$4, %eax
	cmpl	$0, %eax
	jne	.LBB40_3
# %bb.1:
	movq	-40(%rbp), %rax
	movzwl	6(%rax), %eax
	cmpl	$0, %eax
	je	.LBB40_3
# %bb.2:
	movq	-16(%rbp), %rax
	movq	16(%rax), %rax
	movq	-40(%rbp), %rcx
	addq	8(%rcx), %rax
	movq	%rax, -8(%rbp)
	jmp	.LBB40_8
.LBB40_3:
	movq	-48(%rbp), %rdi
	leaq	-64(%rbp), %rsi
	callq	global_lookup
	cmpl	$0, %eax
	je	.LBB40_5
# %bb.4:
	leaq	-64(%rbp), %rdi
	callq	symdef_addr
	movq	%rax, -8(%rbp)
	jmp	.LBB40_8
.LBB40_5:
	movq	-40(%rbp), %rax
	movzbl	4(%rax), %eax
	sarl	$4, %eax
	cmpl	$2, %eax
	jne	.LBB40_7
# %bb.6:
	movq	$0, -8(%rbp)
	jmp	.LBB40_8
.LBB40_7:
	leaq	.L.str.34(%rip), %rdi
	callq	dstr
	movq	-48(%rbp), %rdi
	callq	dstr
	leaq	.L.str.5(%rip), %rdi
	callq	dstr
	movl	$127, %edi
	callq	sys_exit
.LBB40_8:
	movq	-8(%rbp), %rax
	addq	$64, %rsp
	popq	%rbp
	retq
.Lfunc_end40:
	.size	resolve_reloc_symbol, .Lfunc_end40-resolve_reloc_symbol
                                        # -- End function
	.p2align	4                               # -- Begin function global_lookup
	.type	global_lookup,@function
global_lookup:                          # @global_lookup
# %bb.0:
	pushq	%rbp
	movq	%rsp, %rbp
	subq	$64, %rsp
	movq	%rdi, -16(%rbp)
	movq	%rsi, -24(%rbp)
	leaq	-40(%rbp), %rdi
	xorl	%esi, %esi
	movl	$16, %edx
	callq	memset@PLT
	movl	$0, -44(%rbp)
.LBB41_1:                               # =>This Inner Loop Header: Depth=1
	movl	-44(%rbp), %eax
	cmpl	g_nobjs(%rip), %eax
	jge	.LBB41_10
# %bb.2:                                #   in Loop: Header=BB41_1 Depth=1
	movslq	-44(%rbp), %rax
	leaq	g_objs(%rip), %rdi
	imulq	$264, %rax, %rax                # imm = 0x108
	addq	%rax, %rdi
	movq	-16(%rbp), %rsi
	leaq	-64(%rbp), %rdx
	callq	lookup_in
	cmpl	$0, %eax
	je	.LBB41_8
# %bb.3:                                #   in Loop: Header=BB41_1 Depth=1
	movq	-56(%rbp), %rax
	movzbl	4(%rax), %eax
	sarl	$4, %eax
	cmpl	$1, %eax
	jne	.LBB41_5
# %bb.4:
	movq	-24(%rbp), %rax
	movq	-64(%rbp), %rcx
	movq	%rcx, (%rax)
	movq	-56(%rbp), %rcx
	movq	%rcx, 8(%rax)
	movl	$1, -4(%rbp)
	jmp	.LBB41_13
.LBB41_5:                               #   in Loop: Header=BB41_1 Depth=1
	cmpq	$0, -32(%rbp)
	jne	.LBB41_7
# %bb.6:                                #   in Loop: Header=BB41_1 Depth=1
	movq	-64(%rbp), %rax
	movq	%rax, -40(%rbp)
	movq	-56(%rbp), %rax
	movq	%rax, -32(%rbp)
.LBB41_7:                               #   in Loop: Header=BB41_1 Depth=1
	jmp	.LBB41_8
.LBB41_8:                               #   in Loop: Header=BB41_1 Depth=1
	jmp	.LBB41_9
.LBB41_9:                               #   in Loop: Header=BB41_1 Depth=1
	movl	-44(%rbp), %eax
	addl	$1, %eax
	movl	%eax, -44(%rbp)
	jmp	.LBB41_1
.LBB41_10:
	cmpq	$0, -32(%rbp)
	je	.LBB41_12
# %bb.11:
	movq	-24(%rbp), %rax
	movq	-40(%rbp), %rcx
	movq	%rcx, (%rax)
	movq	-32(%rbp), %rcx
	movq	%rcx, 8(%rax)
	movl	$1, -4(%rbp)
	jmp	.LBB41_13
.LBB41_12:
	movl	$0, -4(%rbp)
.LBB41_13:
	movl	-4(%rbp), %eax
	addq	$64, %rsp
	popq	%rbp
	retq
.Lfunc_end41:
	.size	global_lookup, .Lfunc_end41-global_lookup
                                        # -- End function
	.p2align	4                               # -- Begin function symdef_addr
	.type	symdef_addr,@function
symdef_addr:                            # @symdef_addr
# %bb.0:
	pushq	%rbp
	movq	%rsp, %rbp
	subq	$16, %rsp
	movq	%rdi, -8(%rbp)
	movq	-8(%rbp), %rax
	movq	(%rax), %rax
	movq	16(%rax), %rax
	movq	-8(%rbp), %rcx
	movq	8(%rcx), %rcx
	addq	8(%rcx), %rax
	movq	%rax, -16(%rbp)
	movq	-8(%rbp), %rax
	movq	8(%rax), %rax
	movzbl	4(%rax), %eax
	andl	$15, %eax
	cmpl	$10, %eax
	jne	.LBB42_2
# %bb.1:
	callq	*-16(%rbp)
	movq	%rax, -16(%rbp)
.LBB42_2:
	movq	-16(%rbp), %rax
	addq	$16, %rsp
	popq	%rbp
	retq
.Lfunc_end42:
	.size	symdef_addr, .Lfunc_end42-symdef_addr
                                        # -- End function
	.p2align	4                               # -- Begin function lookup_in
	.type	lookup_in,@function
lookup_in:                              # @lookup_in
# %bb.0:
	pushq	%rbp
	movq	%rsp, %rbp
	subq	$64, %rsp
	movq	%rdi, -16(%rbp)
	movq	%rsi, -24(%rbp)
	movq	%rdx, -32(%rbp)
	movq	-16(%rbp), %rax
	cmpq	$0, 48(%rax)
	je	.LBB43_3
# %bb.1:
	movq	-16(%rbp), %rax
	cmpq	$0, 56(%rax)
	je	.LBB43_3
# %bb.2:
	movq	-16(%rbp), %rax
	cmpq	$0, 64(%rax)
	jne	.LBB43_4
.LBB43_3:
	movl	$0, -4(%rbp)
	jmp	.LBB43_16
.LBB43_4:
	movq	$0, -40(%rbp)
.LBB43_5:                               # =>This Inner Loop Header: Depth=1
	movq	-40(%rbp), %rax
	movq	-16(%rbp), %rcx
	cmpq	64(%rcx), %rax
	jae	.LBB43_15
# %bb.6:                                #   in Loop: Header=BB43_5 Depth=1
	movq	-16(%rbp), %rax
	movq	48(%rax), %rax
	imulq	$24, -40(%rbp), %rcx
	addq	%rcx, %rax
	movq	%rax, -48(%rbp)
	movq	-48(%rbp), %rax
	movzwl	6(%rax), %eax
	cmpl	$0, %eax
	jne	.LBB43_8
# %bb.7:                                #   in Loop: Header=BB43_5 Depth=1
	jmp	.LBB43_14
.LBB43_8:                               #   in Loop: Header=BB43_5 Depth=1
	movq	-48(%rbp), %rax
	movzbl	4(%rax), %eax
	sarl	$4, %eax
	cltq
	movq	%rax, -56(%rbp)
	cmpq	$1, -56(%rbp)
	je	.LBB43_11
# %bb.9:                                #   in Loop: Header=BB43_5 Depth=1
	cmpq	$2, -56(%rbp)
	je	.LBB43_11
# %bb.10:                               #   in Loop: Header=BB43_5 Depth=1
	jmp	.LBB43_14
.LBB43_11:                              #   in Loop: Header=BB43_5 Depth=1
	movq	-16(%rbp), %rax
	movq	56(%rax), %rdi
	movq	-48(%rbp), %rax
	movl	(%rax), %eax
                                        # kill: def $rax killed $eax
	addq	%rax, %rdi
	movq	-24(%rbp), %rsi
	callq	kstreq
	cmpl	$0, %eax
	jne	.LBB43_13
# %bb.12:                               #   in Loop: Header=BB43_5 Depth=1
	jmp	.LBB43_14
.LBB43_13:
	movq	-16(%rbp), %rcx
	movq	-32(%rbp), %rax
	movq	%rcx, (%rax)
	movq	-48(%rbp), %rcx
	movq	-32(%rbp), %rax
	movq	%rcx, 8(%rax)
	movl	$1, -4(%rbp)
	jmp	.LBB43_16
.LBB43_14:                              #   in Loop: Header=BB43_5 Depth=1
	movq	-40(%rbp), %rax
	addq	$1, %rax
	movq	%rax, -40(%rbp)
	jmp	.LBB43_5
.LBB43_15:
	movl	$0, -4(%rbp)
.LBB43_16:
	movl	-4(%rbp), %eax
	addq	$64, %rsp
	popq	%rbp
	retq
.Lfunc_end43:
	.size	lookup_in, .Lfunc_end43-lookup_in
                                        # -- End function
	.p2align	4                               # -- Begin function sys_mprotect
	.type	sys_mprotect,@function
sys_mprotect:                           # @sys_mprotect
# %bb.0:
	pushq	%rbp
	movq	%rsp, %rbp
	subq	$32, %rsp
	movq	%rdi, -8(%rbp)
	movq	%rsi, -16(%rbp)
	movq	%rdx, -24(%rbp)
	movq	-8(%rbp), %rsi
	movq	-16(%rbp), %rdx
	movq	-24(%rbp), %rcx
	movl	$10, %edi
	callq	syscall3
	addq	$32, %rsp
	popq	%rbp
	retq
.Lfunc_end44:
	.size	sys_mprotect, .Lfunc_end44-sys_mprotect
                                        # -- End function
	.p2align	4                               # -- Begin function count_ptrs
	.type	count_ptrs,@function
count_ptrs:                             # @count_ptrs
# %bb.0:
	pushq	%rbp
	movq	%rsp, %rbp
	movq	%rdi, -8(%rbp)
	movl	$0, -12(%rbp)
.LBB45_1:                               # =>This Inner Loop Header: Depth=1
	movq	-8(%rbp), %rax
	movslq	-12(%rbp), %rcx
	cmpq	$0, (%rax,%rcx,8)
	je	.LBB45_3
# %bb.2:                                #   in Loop: Header=BB45_1 Depth=1
	movl	-12(%rbp), %eax
	addl	$1, %eax
	movl	%eax, -12(%rbp)
	jmp	.LBB45_1
.LBB45_3:
	movl	-12(%rbp), %eax
	popq	%rbp
	retq
.Lfunc_end45:
	.size	count_ptrs, .Lfunc_end45-count_ptrs
                                        # -- End function
	.p2align	4                               # -- Begin function enter_program
	.type	enter_program,@function
enter_program:                          # @enter_program
# %bb.0:
	pushq	%rbp
	movq	%rsp, %rbp
	movq	%rdi, -8(%rbp)
	movq	%rsi, -16(%rbp)
	movq	-8(%rbp), %rdi
	movq	-16(%rbp), %rsi
	#APP
	movq	%rsi, %rsp
	xorl	%edx, %edx
	xorl	%ebp, %ebp
	jmpq	*%rdi

	#NO_APP
.Lfunc_end46:
	.size	enter_program, .Lfunc_end46-enter_program
                                        # -- End function
	.type	.L.str,@object                  # @.str
	.section	.rodata.str1.1,"aMS",@progbits,1
.L.str:
	.asciz	"LDLAB_DEBUG=1"
	.size	.L.str, 14

	.type	g_debug,@object                 # @g_debug
	.local	g_debug
	.comm	g_debug,4,4
	.type	.L.str.1,@object                # @.str.1
.L.str.1:
	.asciz	"LDLAB_LIBRARY_PATH="
	.size	.L.str.1, 20

	.type	g_env_libpath,@object           # @g_env_libpath
	.local	g_env_libpath
	.comm	g_env_libpath,8,8
	.type	.L.str.2,@object                # @.str.2
.L.str.2:
	.asciz	"usage: loader ./program [args...]   (set LDLAB_DEBUG=1 to trace)"
	.size	.L.str.2, 65

	.type	g_prog_dir,@object              # @g_prog_dir
	.local	g_prog_dir
	.comm	g_prog_dir,4096,16
	.type	.L.str.3,@object                # @.str.3
.L.str.3:
	.asciz	"loading main program"
	.size	.L.str.3, 21

	.type	g_nobjs,@object                 # @g_nobjs
	.local	g_nobjs
	.comm	g_nobjs,4,4
	.type	g_objs,@object                  # @g_objs
	.local	g_objs
	.comm	g_objs,8448,16
	.type	.L.str.4,@object                # @.str.4
.L.str.4:
	.asciz	"[ld] DT_NEEDED "
	.size	.L.str.4, 16

	.type	.L.str.5,@object                # @.str.5
.L.str.5:
	.asciz	"\n"
	.size	.L.str.5, 2

	.type	.L.str.6,@object                # @.str.6
.L.str.6:
	.asciz	"loader: "
	.size	.L.str.6, 9

	.type	.L.str.7,@object                # @.str.7
.L.str.7:
	.asciz	"[ld] "
	.size	.L.str.7, 6

	.type	.L.str.8,@object                # @.str.8
.L.str.8:
	.asciz	"cannot open object"
	.size	.L.str.8, 19

	.type	.L.str.9,@object                # @.str.9
.L.str.9:
	.asciz	" "
	.size	.L.str.9, 2

	.type	dhex.H,@object                  # @dhex.H
	.section	.rodata,"a",@progbits
	.p2align	4, 0x0
dhex.H:
	.asciz	"0123456789abcdef"
	.size	dhex.H, 17

	.type	.L.str.10,@object               # @.str.10
	.section	.rodata.str1.1,"aMS",@progbits,1
.L.str.10:
	.asciz	"too many shared objects"
	.size	.L.str.10, 24

	.type	.L.str.11,@object               # @.str.11
.L.str.11:
	.asciz	"short read of ELF header"
	.size	.L.str.11, 25

	.type	.L.str.12,@object               # @.str.12
.L.str.12:
	.asciz	"not an ELF file (bad magic)"
	.size	.L.str.12, 28

	.type	.L.str.13,@object               # @.str.13
.L.str.13:
	.asciz	"not ELFCLASS64"
	.size	.L.str.13, 15

	.type	.L.str.14,@object               # @.str.14
.L.str.14:
	.asciz	"not little-endian"
	.size	.L.str.14, 18

	.type	.L.str.15,@object               # @.str.15
.L.str.15:
	.asciz	"not x86-64"
	.size	.L.str.15, 11

	.type	.L.str.16,@object               # @.str.16
.L.str.16:
	.asciz	"not ET_DYN/ET_EXEC"
	.size	.L.str.16, 19

	.type	.L.str.17,@object               # @.str.17
.L.str.17:
	.asciz	"unexpected e_phentsize"
	.size	.L.str.17, 23

	.type	.L.str.18,@object               # @.str.18
.L.str.18:
	.asciz	"implausible e_phnum"
	.size	.L.str.18, 20

	.type	load_from_fd.ph,@object         # @load_from_fd.ph
	.local	load_from_fd.ph
	.comm	load_from_fd.ph,14336,16
	.type	.L.str.19,@object               # @.str.19
.L.str.19:
	.asciz	"short read of program headers"
	.size	.L.str.19, 30

	.type	.L.str.20,@object               # @.str.20
.L.str.20:
	.asciz	"note: PT_TLS present (TLS is not set up by this loader)"
	.size	.L.str.20, 56

	.type	.L.str.21,@object               # @.str.21
.L.str.21:
	.asciz	"no PT_LOAD segments"
	.size	.L.str.21, 20

	.type	.L.str.22,@object               # @.str.22
.L.str.22:
	.asciz	"reserve mmap failed"
	.size	.L.str.22, 20

	.type	.L.str.23,@object               # @.str.23
.L.str.23:
	.asciz	"load bias"
	.size	.L.str.23, 10

	.type	.L.str.24,@object               # @.str.24
.L.str.24:
	.asciz	"segment mmap failed"
	.size	.L.str.24, 20

	.type	.L.str.25,@object               # @.str.25
.L.str.25:
	.asciz	"bss mmap failed"
	.size	.L.str.25, 16

	.type	.L.str.26,@object               # @.str.26
.L.str.26:
	.asciz	"too many DT_NEEDED entries"
	.size	.L.str.26, 27

	.type	.L.str.27,@object               # @.str.27
.L.str.27:
	.asciz	"PLT relocations are DT_REL, not DT_RELA (unsupported)"
	.size	.L.str.27, 54

	.type	.L.str.28,@object               # @.str.28
.L.str.28:
	.asciz	"."
	.size	.L.str.28, 2

	.type	.L.str.29,@object               # @.str.29
.L.str.29:
	.asciz	"loader: cannot find shared object: "
	.size	.L.str.29, 36

	.type	try_dir.names,@object           # @try_dir.names
	.local	try_dir.names
	.comm	try_dir.names,131072,16
	.type	try_dir.nnames,@object          # @try_dir.nnames
	.local	try_dir.nnames
	.comm	try_dir.nnames,4,4
	.type	.L.str.30,@object               # @.str.30
.L.str.30:
	.asciz	"relocating"
	.size	.L.str.30, 11

	.type	.L.str.31,@object               # @.str.31
.L.str.31:
	.asciz	"R_X86_64_COPY: undefined symbol"
	.size	.L.str.31, 32

	.type	.L.str.32,@object               # @.str.32
.L.str.32:
	.asciz	"TLS relocation encountered (unsupported in this teaching core)"
	.size	.L.str.32, 63

	.type	.L.str.33,@object               # @.str.33
.L.str.33:
	.asciz	"unhandled relocation type"
	.size	.L.str.33, 26

	.type	.L.str.34,@object               # @.str.34
.L.str.34:
	.asciz	"loader: undefined symbol: "
	.size	.L.str.34, 27

	.type	.L.str.35,@object               # @.str.35
.L.str.35:
	.asciz	"relro mprotect failed"
	.size	.L.str.35, 22

	.type	.L.str.36,@object               # @.str.36
.L.str.36:
	.asciz	"applied RELRO (GOT now read-only)"
	.size	.L.str.36, 34

	.type	.L.str.37,@object               # @.str.37
.L.str.37:
	.asciz	"DT_INIT"
	.size	.L.str.37, 8

	.type	.L.str.38,@object               # @.str.38
.L.str.38:
	.asciz	"stack mmap failed"
	.size	.L.str.38, 18

	.type	.L.str.39,@object               # @.str.39
.L.str.39:
	.asciz	"entry"
	.size	.L.str.39, 6

	.type	.L.str.40,@object               # @.str.40
.L.str.40:
	.asciz	"new sp"
	.size	.L.str.40, 7

	.ident	"clang version 20.1.8"
	.section	".note.GNU-stack","",@progbits
	.addrsig
	.addrsig_sym memset
	.addrsig_sym memcpy
	.addrsig_sym kstreq
	.addrsig_sym die
	.addrsig_sym klast_slash
	.addrsig_sym trace
	.addrsig_sym load_path
	.addrsig_sym dstr
	.addrsig_sym load_needed
	.addrsig_sym relocate_object
	.addrsig_sym apply_relro
	.addrsig_sym run_init
	.addrsig_sym handoff
	.addrsig_sym sys_exit
	.addrsig_sym syscall1
	.addrsig_sym syscall6
	.addrsig_sym sys_openat
	.addrsig_sym die2
	.addrsig_sym load_from_fd
	.addrsig_sym sys_close
	.addrsig_sym syscall4
	.addrsig_sym dhex
	.addrsig_sym sys_pread
	.addrsig_sym map_object
	.addrsig_sym parse_dynamic
	.addrsig_sym align_down
	.addrsig_sym align_up
	.addrsig_sym sys_mmap
	.addrsig_sym mmap_failed
	.addrsig_sym trace2
	.addrsig_sym sys_write
	.addrsig_sym kstrlen
	.addrsig_sym syscall3
	.addrsig_sym already_loaded
	.addrsig_sym try_pathlist
	.addrsig_sym try_dir
	.addrsig_sym kstrcpy
	.addrsig_sym apply_relr
	.addrsig_sym apply_rela
	.addrsig_sym resolve_reloc_symbol
	.addrsig_sym global_lookup
	.addrsig_sym symdef_addr
	.addrsig_sym lookup_in
	.addrsig_sym sys_mprotect
	.addrsig_sym count_ptrs
	.addrsig_sym enter_program
	.addrsig_sym g_debug
	.addrsig_sym g_env_libpath
	.addrsig_sym g_prog_dir
	.addrsig_sym g_nobjs
	.addrsig_sym g_objs
	.addrsig_sym dhex.H
	.addrsig_sym load_from_fd.ph
	.addrsig_sym try_dir.names
	.addrsig_sym try_dir.nnames
