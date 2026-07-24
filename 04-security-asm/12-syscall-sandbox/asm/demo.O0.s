	.file	"demo.c"
	.text
	.globl	bpf_load_u32                    # -- Begin function bpf_load_u32
	.p2align	4
	.type	bpf_load_u32,@function
bpf_load_u32:                           # @bpf_load_u32
# %bb.0:
	pushq	%rbp
	movq	%rsp, %rbp
	movq	%rdi, -8(%rbp)
	movl	%esi, -12(%rbp)
	movq	-8(%rbp), %rax
	movl	-12(%rbp), %ecx
	addl	$0, %ecx
	movl	%ecx, %ecx
                                        # kill: def $rcx killed $ecx
	movzbl	(%rax,%rcx), %eax
	movq	-8(%rbp), %rcx
	movl	-12(%rbp), %edx
	addl	$1, %edx
	movl	%edx, %edx
                                        # kill: def $rdx killed $edx
	movzbl	(%rcx,%rdx), %ecx
	shll	$8, %ecx
	orl	%ecx, %eax
	movq	-8(%rbp), %rcx
	movl	-12(%rbp), %edx
	addl	$2, %edx
	movl	%edx, %edx
                                        # kill: def $rdx killed $edx
	movzbl	(%rcx,%rdx), %ecx
	shll	$16, %ecx
	orl	%ecx, %eax
	movq	-8(%rbp), %rcx
	movl	-12(%rbp), %edx
	addl	$3, %edx
	movl	%edx, %edx
                                        # kill: def $rdx killed $edx
	movzbl	(%rcx,%rdx), %ecx
	shll	$24, %ecx
	orl	%ecx, %eax
	popq	%rbp
	retq
.Lfunc_end0:
	.size	bpf_load_u32, .Lfunc_end0-bpf_load_u32
                                        # -- End function
	.globl	build_allowlist                 # -- Begin function build_allowlist
	.p2align	4
	.type	build_allowlist,@function
build_allowlist:                        # @build_allowlist
# %bb.0:
	pushq	%rbp
	movq	%rsp, %rbp
	movq	%rdi, -16(%rbp)
	movl	%esi, -20(%rbp)
	movq	%rdx, -32(%rbp)
	movl	%ecx, -36(%rbp)
	movl	$0, -40(%rbp)
# %bb.1:
	movl	-40(%rbp), %eax
	cmpl	-20(%rbp), %eax
	jl	.LBB1_3
# %bb.2:
	movl	$-1, -4(%rbp)
	jmp	.LBB1_41
.LBB1_3:
	movq	-16(%rbp), %rax
	movslq	-40(%rbp), %rcx
	movw	$32, (%rax,%rcx,8)
	movq	-16(%rbp), %rax
	movslq	-40(%rbp), %rcx
	movb	$0, 2(%rax,%rcx,8)
	movq	-16(%rbp), %rax
	movslq	-40(%rbp), %rcx
	movb	$0, 3(%rax,%rcx,8)
	movq	-16(%rbp), %rax
	movslq	-40(%rbp), %rcx
	movl	$4, 4(%rax,%rcx,8)
	movl	-40(%rbp), %eax
	addl	$1, %eax
	movl	%eax, -40(%rbp)
# %bb.4:
	jmp	.LBB1_5
.LBB1_5:
	movl	-40(%rbp), %eax
	cmpl	-20(%rbp), %eax
	jl	.LBB1_7
# %bb.6:
	movl	$-1, -4(%rbp)
	jmp	.LBB1_41
.LBB1_7:
	movq	-16(%rbp), %rax
	movslq	-40(%rbp), %rcx
	movw	$21, (%rax,%rcx,8)
	movq	-16(%rbp), %rax
	movslq	-40(%rbp), %rcx
	movb	$1, 2(%rax,%rcx,8)
	movq	-16(%rbp), %rax
	movslq	-40(%rbp), %rcx
	movb	$0, 3(%rax,%rcx,8)
	movq	-16(%rbp), %rax
	movslq	-40(%rbp), %rcx
	movl	$-1073741762, 4(%rax,%rcx,8)    # imm = 0xC000003E
	movl	-40(%rbp), %eax
	addl	$1, %eax
	movl	%eax, -40(%rbp)
# %bb.8:
	jmp	.LBB1_9
.LBB1_9:
	movl	-40(%rbp), %eax
	cmpl	-20(%rbp), %eax
	jl	.LBB1_11
# %bb.10:
	movl	$-1, -4(%rbp)
	jmp	.LBB1_41
.LBB1_11:
	movq	-16(%rbp), %rax
	movslq	-40(%rbp), %rcx
	movw	$6, (%rax,%rcx,8)
	movq	-16(%rbp), %rax
	movslq	-40(%rbp), %rcx
	movb	$0, 2(%rax,%rcx,8)
	movq	-16(%rbp), %rax
	movslq	-40(%rbp), %rcx
	movb	$0, 3(%rax,%rcx,8)
	movq	-16(%rbp), %rax
	movslq	-40(%rbp), %rcx
	movl	$-2147483648, 4(%rax,%rcx,8)    # imm = 0x80000000
	movl	-40(%rbp), %eax
	addl	$1, %eax
	movl	%eax, -40(%rbp)
# %bb.12:
	jmp	.LBB1_13
.LBB1_13:
	movl	-40(%rbp), %eax
	cmpl	-20(%rbp), %eax
	jl	.LBB1_15
# %bb.14:
	movl	$-1, -4(%rbp)
	jmp	.LBB1_41
.LBB1_15:
	movq	-16(%rbp), %rax
	movslq	-40(%rbp), %rcx
	movw	$32, (%rax,%rcx,8)
	movq	-16(%rbp), %rax
	movslq	-40(%rbp), %rcx
	movb	$0, 2(%rax,%rcx,8)
	movq	-16(%rbp), %rax
	movslq	-40(%rbp), %rcx
	movb	$0, 3(%rax,%rcx,8)
	movq	-16(%rbp), %rax
	movslq	-40(%rbp), %rcx
	movl	$0, 4(%rax,%rcx,8)
	movl	-40(%rbp), %eax
	addl	$1, %eax
	movl	%eax, -40(%rbp)
# %bb.16:
	jmp	.LBB1_17
.LBB1_17:
	movl	-40(%rbp), %eax
	cmpl	-20(%rbp), %eax
	jl	.LBB1_19
# %bb.18:
	movl	$-1, -4(%rbp)
	jmp	.LBB1_41
.LBB1_19:
	movq	-16(%rbp), %rax
	movslq	-40(%rbp), %rcx
	movw	$53, (%rax,%rcx,8)
	movq	-16(%rbp), %rax
	movslq	-40(%rbp), %rcx
	movb	$0, 2(%rax,%rcx,8)
	movq	-16(%rbp), %rax
	movslq	-40(%rbp), %rcx
	movb	$1, 3(%rax,%rcx,8)
	movq	-16(%rbp), %rax
	movslq	-40(%rbp), %rcx
	movl	$1073741824, 4(%rax,%rcx,8)     # imm = 0x40000000
	movl	-40(%rbp), %eax
	addl	$1, %eax
	movl	%eax, -40(%rbp)
# %bb.20:
	jmp	.LBB1_21
.LBB1_21:
	movl	-40(%rbp), %eax
	cmpl	-20(%rbp), %eax
	jl	.LBB1_23
# %bb.22:
	movl	$-1, -4(%rbp)
	jmp	.LBB1_41
.LBB1_23:
	movq	-16(%rbp), %rax
	movslq	-40(%rbp), %rcx
	movw	$6, (%rax,%rcx,8)
	movq	-16(%rbp), %rax
	movslq	-40(%rbp), %rcx
	movb	$0, 2(%rax,%rcx,8)
	movq	-16(%rbp), %rax
	movslq	-40(%rbp), %rcx
	movb	$0, 3(%rax,%rcx,8)
	movq	-16(%rbp), %rax
	movslq	-40(%rbp), %rcx
	movl	$-2147483648, 4(%rax,%rcx,8)    # imm = 0x80000000
	movl	-40(%rbp), %eax
	addl	$1, %eax
	movl	%eax, -40(%rbp)
# %bb.24:
	movl	$0, -44(%rbp)
.LBB1_25:                               # =>This Inner Loop Header: Depth=1
	movl	-44(%rbp), %eax
	cmpl	-36(%rbp), %eax
	jge	.LBB1_36
# %bb.26:                               #   in Loop: Header=BB1_25 Depth=1
	jmp	.LBB1_27
.LBB1_27:                               #   in Loop: Header=BB1_25 Depth=1
	movl	-40(%rbp), %eax
	cmpl	-20(%rbp), %eax
	jl	.LBB1_29
# %bb.28:
	movl	$-1, -4(%rbp)
	jmp	.LBB1_41
.LBB1_29:                               #   in Loop: Header=BB1_25 Depth=1
	movq	-16(%rbp), %rax
	movslq	-40(%rbp), %rcx
	movw	$21, (%rax,%rcx,8)
	movq	-16(%rbp), %rax
	movslq	-40(%rbp), %rcx
	movb	$0, 2(%rax,%rcx,8)
	movq	-16(%rbp), %rax
	movslq	-40(%rbp), %rcx
	movb	$1, 3(%rax,%rcx,8)
	movq	-32(%rbp), %rax
	movslq	-44(%rbp), %rcx
	movl	(%rax,%rcx,4), %edx
	movq	-16(%rbp), %rax
	movslq	-40(%rbp), %rcx
	movl	%edx, 4(%rax,%rcx,8)
	movl	-40(%rbp), %eax
	addl	$1, %eax
	movl	%eax, -40(%rbp)
# %bb.30:                               #   in Loop: Header=BB1_25 Depth=1
	jmp	.LBB1_31
.LBB1_31:                               #   in Loop: Header=BB1_25 Depth=1
	movl	-40(%rbp), %eax
	cmpl	-20(%rbp), %eax
	jl	.LBB1_33
# %bb.32:
	movl	$-1, -4(%rbp)
	jmp	.LBB1_41
.LBB1_33:                               #   in Loop: Header=BB1_25 Depth=1
	movq	-16(%rbp), %rax
	movslq	-40(%rbp), %rcx
	movw	$6, (%rax,%rcx,8)
	movq	-16(%rbp), %rax
	movslq	-40(%rbp), %rcx
	movb	$0, 2(%rax,%rcx,8)
	movq	-16(%rbp), %rax
	movslq	-40(%rbp), %rcx
	movb	$0, 3(%rax,%rcx,8)
	movq	-16(%rbp), %rax
	movslq	-40(%rbp), %rcx
	movl	$2147418112, 4(%rax,%rcx,8)     # imm = 0x7FFF0000
	movl	-40(%rbp), %eax
	addl	$1, %eax
	movl	%eax, -40(%rbp)
# %bb.34:                               #   in Loop: Header=BB1_25 Depth=1
	jmp	.LBB1_35
.LBB1_35:                               #   in Loop: Header=BB1_25 Depth=1
	movl	-44(%rbp), %eax
	addl	$1, %eax
	movl	%eax, -44(%rbp)
	jmp	.LBB1_25
.LBB1_36:
	jmp	.LBB1_37
.LBB1_37:
	movl	-40(%rbp), %eax
	cmpl	-20(%rbp), %eax
	jl	.LBB1_39
# %bb.38:
	movl	$-1, -4(%rbp)
	jmp	.LBB1_41
.LBB1_39:
	movq	-16(%rbp), %rax
	movslq	-40(%rbp), %rcx
	movw	$6, (%rax,%rcx,8)
	movq	-16(%rbp), %rax
	movslq	-40(%rbp), %rcx
	movb	$0, 2(%rax,%rcx,8)
	movq	-16(%rbp), %rax
	movslq	-40(%rbp), %rcx
	movb	$0, 3(%rax,%rcx,8)
	movq	-16(%rbp), %rax
	movslq	-40(%rbp), %rcx
	movl	$-2147483648, 4(%rax,%rcx,8)    # imm = 0x80000000
	movl	-40(%rbp), %eax
	addl	$1, %eax
	movl	%eax, -40(%rbp)
# %bb.40:
	movl	-40(%rbp), %eax
	movl	%eax, -4(%rbp)
.LBB1_41:
	movl	-4(%rbp), %eax
	popq	%rbp
	retq
.Lfunc_end1:
	.size	build_allowlist, .Lfunc_end1-build_allowlist
                                        # -- End function
	.globl	seccomp_run                     # -- Begin function seccomp_run
	.p2align	4
	.type	seccomp_run,@function
seccomp_run:                            # @seccomp_run
# %bb.0:
	pushq	%rbp
	movq	%rsp, %rbp
	subq	$80, %rsp
	movq	%rdi, -16(%rbp)
	movl	%esi, -20(%rbp)
	movq	%rdx, -32(%rbp)
	movl	%ecx, -36(%rbp)
	movl	$0, -40(%rbp)
	movl	$0, -44(%rbp)
.LBB2_1:                                # =>This Inner Loop Header: Depth=1
	movl	-44(%rbp), %eax
	cmpl	-20(%rbp), %eax
	jge	.LBB2_18
# %bb.2:                                #   in Loop: Header=BB2_1 Depth=1
	movq	-16(%rbp), %rax
	movslq	-44(%rbp), %rcx
	leaq	(%rax,%rcx,8), %rax
	movq	%rax, -56(%rbp)
	movq	-56(%rbp), %rax
	movzwl	(%rax), %eax
	movl	%eax, -60(%rbp)                 # 4-byte Spill
	subl	$6, %eax
	je	.LBB2_14
	jmp	.LBB2_20
.LBB2_20:                               #   in Loop: Header=BB2_1 Depth=1
	movl	-60(%rbp), %eax                 # 4-byte Reload
	subl	$21, %eax
	je	.LBB2_6
	jmp	.LBB2_21
.LBB2_21:                               #   in Loop: Header=BB2_1 Depth=1
	movl	-60(%rbp), %eax                 # 4-byte Reload
	subl	$32, %eax
	je	.LBB2_3
	jmp	.LBB2_22
.LBB2_22:                               #   in Loop: Header=BB2_1 Depth=1
	movl	-60(%rbp), %eax                 # 4-byte Reload
	subl	$53, %eax
	je	.LBB2_10
	jmp	.LBB2_15
.LBB2_3:                                #   in Loop: Header=BB2_1 Depth=1
	movq	-56(%rbp), %rax
	movl	4(%rax), %eax
	addl	$4, %eax
	cmpl	-36(%rbp), %eax
	jbe	.LBB2_5
# %bb.4:
	movl	$-2147483648, -4(%rbp)          # imm = 0x80000000
	jmp	.LBB2_19
.LBB2_5:                                #   in Loop: Header=BB2_1 Depth=1
	movq	-32(%rbp), %rdi
	movq	-56(%rbp), %rax
	movl	4(%rax), %esi
	callq	bpf_load_u32
	movl	%eax, -40(%rbp)
	jmp	.LBB2_16
.LBB2_6:                                #   in Loop: Header=BB2_1 Depth=1
	movl	-40(%rbp), %eax
	movq	-56(%rbp), %rcx
	cmpl	4(%rcx), %eax
	jne	.LBB2_8
# %bb.7:                                #   in Loop: Header=BB2_1 Depth=1
	movq	-56(%rbp), %rax
	movzbl	2(%rax), %eax
	movl	%eax, -64(%rbp)                 # 4-byte Spill
	jmp	.LBB2_9
.LBB2_8:                                #   in Loop: Header=BB2_1 Depth=1
	movq	-56(%rbp), %rax
	movzbl	3(%rax), %eax
	movl	%eax, -64(%rbp)                 # 4-byte Spill
.LBB2_9:                                #   in Loop: Header=BB2_1 Depth=1
	movl	-64(%rbp), %eax                 # 4-byte Reload
	addl	-44(%rbp), %eax
	movl	%eax, -44(%rbp)
	jmp	.LBB2_16
.LBB2_10:                               #   in Loop: Header=BB2_1 Depth=1
	movl	-40(%rbp), %eax
	movq	-56(%rbp), %rcx
	cmpl	4(%rcx), %eax
	jb	.LBB2_12
# %bb.11:                               #   in Loop: Header=BB2_1 Depth=1
	movq	-56(%rbp), %rax
	movzbl	2(%rax), %eax
	movl	%eax, -68(%rbp)                 # 4-byte Spill
	jmp	.LBB2_13
.LBB2_12:                               #   in Loop: Header=BB2_1 Depth=1
	movq	-56(%rbp), %rax
	movzbl	3(%rax), %eax
	movl	%eax, -68(%rbp)                 # 4-byte Spill
.LBB2_13:                               #   in Loop: Header=BB2_1 Depth=1
	movl	-68(%rbp), %eax                 # 4-byte Reload
	addl	-44(%rbp), %eax
	movl	%eax, -44(%rbp)
	jmp	.LBB2_16
.LBB2_14:
	movq	-56(%rbp), %rax
	movl	4(%rax), %eax
	movl	%eax, -4(%rbp)
	jmp	.LBB2_19
.LBB2_15:
	movl	$-2147483648, -4(%rbp)          # imm = 0x80000000
	jmp	.LBB2_19
.LBB2_16:                               #   in Loop: Header=BB2_1 Depth=1
	jmp	.LBB2_17
.LBB2_17:                               #   in Loop: Header=BB2_1 Depth=1
	movl	-44(%rbp), %eax
	addl	$1, %eax
	movl	%eax, -44(%rbp)
	jmp	.LBB2_1
.LBB2_18:
	movl	$-2147483648, -4(%rbp)          # imm = 0x80000000
.LBB2_19:
	movl	-4(%rbp), %eax
	addq	$80, %rsp
	popq	%rbp
	retq
.Lfunc_end2:
	.size	seccomp_run, .Lfunc_end2-seccomp_run
                                        # -- End function
	.globl	demo_selftest                   # -- Begin function demo_selftest
	.p2align	4
	.type	demo_selftest,@function
demo_selftest:                          # @demo_selftest
# %bb.0:
	pushq	%rbp
	movq	%rsp, %rbp
	subq	$352, %rsp                      # imm = 0x160
	movq	.L__const.demo_selftest.allow(%rip), %rax
	movq	%rax, -284(%rbp)
	movl	.L__const.demo_selftest.allow+8(%rip), %eax
	movl	%eax, -276(%rbp)
	leaq	-272(%rbp), %rdi
	leaq	-284(%rbp), %rdx
	movl	$32, %esi
	movl	$3, %ecx
	callq	build_allowlist
	movl	%eax, -288(%rbp)
	cmpl	$0, -288(%rbp)
	jge	.LBB3_2
# %bb.1:
	movl	$1, -4(%rbp)
	jmp	.LBB3_11
.LBB3_2:
	leaq	-352(%rbp), %rdi
	movl	$1, %esi
	movl	$3221225534, %edx               # imm = 0xC000003E
	callq	make_data
	leaq	-272(%rbp), %rdi
	movl	-288(%rbp), %esi
	leaq	-352(%rbp), %rdx
	movl	$64, %ecx
	callq	seccomp_run
	cmpl	$2147418112, %eax               # imm = 0x7FFF0000
	je	.LBB3_4
# %bb.3:
	movl	$2, -4(%rbp)
	jmp	.LBB3_11
.LBB3_4:
	leaq	-352(%rbp), %rdi
	movl	$41, %esi
	movl	$3221225534, %edx               # imm = 0xC000003E
	callq	make_data
	leaq	-272(%rbp), %rdi
	movl	-288(%rbp), %esi
	leaq	-352(%rbp), %rdx
	movl	$64, %ecx
	callq	seccomp_run
	cmpl	$-2147483648, %eax              # imm = 0x80000000
	je	.LBB3_6
# %bb.5:
	movl	$3, -4(%rbp)
	jmp	.LBB3_11
.LBB3_6:
	leaq	-352(%rbp), %rdi
	movl	$1, %esi
	movl	$3221225535, %edx               # imm = 0xC000003F
	callq	make_data
	leaq	-272(%rbp), %rdi
	movl	-288(%rbp), %esi
	leaq	-352(%rbp), %rdx
	movl	$64, %ecx
	callq	seccomp_run
	cmpl	$-2147483648, %eax              # imm = 0x80000000
	je	.LBB3_8
# %bb.7:
	movl	$4, -4(%rbp)
	jmp	.LBB3_11
.LBB3_8:
	leaq	-352(%rbp), %rdi
	movl	$1073741825, %esi               # imm = 0x40000001
	movl	$3221225534, %edx               # imm = 0xC000003E
	callq	make_data
	leaq	-272(%rbp), %rdi
	movl	-288(%rbp), %esi
	leaq	-352(%rbp), %rdx
	movl	$64, %ecx
	callq	seccomp_run
	cmpl	$-2147483648, %eax              # imm = 0x80000000
	je	.LBB3_10
# %bb.9:
	movl	$5, -4(%rbp)
	jmp	.LBB3_11
.LBB3_10:
	movl	$0, -4(%rbp)
.LBB3_11:
	movl	-4(%rbp), %eax
	addq	$352, %rsp                      # imm = 0x160
	popq	%rbp
	retq
.Lfunc_end3:
	.size	demo_selftest, .Lfunc_end3-demo_selftest
                                        # -- End function
	.p2align	4                               # -- Begin function make_data
	.type	make_data,@function
make_data:                              # @make_data
# %bb.0:
	pushq	%rbp
	movq	%rsp, %rbp
	subq	$32, %rsp
	movq	%rdi, -8(%rbp)
	movl	%esi, -12(%rbp)
	movl	%edx, -16(%rbp)
	movl	$0, -20(%rbp)
.LBB4_1:                                # =>This Inner Loop Header: Depth=1
	cmpl	$64, -20(%rbp)
	jge	.LBB4_4
# %bb.2:                                #   in Loop: Header=BB4_1 Depth=1
	movq	-8(%rbp), %rax
	movslq	-20(%rbp), %rcx
	movb	$0, (%rax,%rcx)
# %bb.3:                                #   in Loop: Header=BB4_1 Depth=1
	movl	-20(%rbp), %eax
	addl	$1, %eax
	movl	%eax, -20(%rbp)
	jmp	.LBB4_1
.LBB4_4:
	movq	-8(%rbp), %rdi
	movl	-12(%rbp), %edx
	xorl	%esi, %esi
	callq	store_u32
	movq	-8(%rbp), %rdi
	movl	-16(%rbp), %edx
	movl	$4, %esi
	callq	store_u32
	addq	$32, %rsp
	popq	%rbp
	retq
.Lfunc_end4:
	.size	make_data, .Lfunc_end4-make_data
                                        # -- End function
	.p2align	4                               # -- Begin function store_u32
	.type	store_u32,@function
store_u32:                              # @store_u32
# %bb.0:
	pushq	%rbp
	movq	%rsp, %rbp
	movq	%rdi, -8(%rbp)
	movl	%esi, -12(%rbp)
	movl	%edx, -16(%rbp)
	movl	-16(%rbp), %eax
	movb	%al, %dl
	movq	-8(%rbp), %rax
	movl	-12(%rbp), %ecx
	addl	$0, %ecx
	movl	%ecx, %ecx
                                        # kill: def $rcx killed $ecx
	movb	%dl, (%rax,%rcx)
	movl	-16(%rbp), %eax
	shrl	$8, %eax
	movb	%al, %dl
	movq	-8(%rbp), %rax
	movl	-12(%rbp), %ecx
	addl	$1, %ecx
	movl	%ecx, %ecx
                                        # kill: def $rcx killed $ecx
	movb	%dl, (%rax,%rcx)
	movl	-16(%rbp), %eax
	shrl	$16, %eax
	movb	%al, %dl
	movq	-8(%rbp), %rax
	movl	-12(%rbp), %ecx
	addl	$2, %ecx
	movl	%ecx, %ecx
                                        # kill: def $rcx killed $ecx
	movb	%dl, (%rax,%rcx)
	movl	-16(%rbp), %eax
	shrl	$24, %eax
	movb	%al, %dl
	movq	-8(%rbp), %rax
	movl	-12(%rbp), %ecx
	addl	$3, %ecx
	movl	%ecx, %ecx
                                        # kill: def $rcx killed $ecx
	movb	%dl, (%rax,%rcx)
	popq	%rbp
	retq
.Lfunc_end5:
	.size	store_u32, .Lfunc_end5-store_u32
                                        # -- End function
	.type	.L__const.demo_selftest.allow,@object # @__const.demo_selftest.allow
	.section	.rodata,"a",@progbits
	.p2align	2, 0x0
.L__const.demo_selftest.allow:
	.long	0                               # 0x0
	.long	1                               # 0x1
	.long	231                             # 0xe7
	.size	.L__const.demo_selftest.allow, 12

	.ident	"clang version 20.1.8"
	.section	".note.GNU-stack","",@progbits
	.addrsig
	.addrsig_sym bpf_load_u32
	.addrsig_sym build_allowlist
	.addrsig_sym seccomp_run
	.addrsig_sym make_data
	.addrsig_sym store_u32
