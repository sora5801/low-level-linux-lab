# =============================================================================
# demo.annotated.s — clang's -O1 output for asm/demo.c, explained line by line.
# =============================================================================
#
# HOW TO READ THIS FILE
# ---------------------
# asm/demo.s is the VERBATIM compiler output (clang 20, --target=x86_64-pc-linux-gnu
# -O1). This file reproduces the instructive functions from it and comments
# essentially every instruction. AT&T syntax throughout:
#
#     op   src, dst                # movl $1, %eax   =>   eax = 1
#     %reg                         # a register        (rax/eax/ax/al = same reg)
#     $imm                         # an immediate constant
#     N(%base,%idx,scale)          # memory at  base + idx*scale + N
#
# SysV AMD64 calling convention (what every function here obeys):
#     integer/pointer args:  rdi, rsi, rdx, rcx, r8, r9   (then the stack)
#     return value:          rax (eax for 32-bit)
#     callee-saved:          rbx, rbp, r12-r15  (must be preserved)
#     caller-saved:          rax, rcx, rdx, rsi, rdi, r8-r11
#
# THE BIG PICTURE
# ---------------
# demo.c models the seccomp machinery as pure data:
#   * struct bpf_insn is EXACTLY 8 bytes: code(2) jt(1) jf(1) k(4). Watch the
#     compiler treat a whole instruction as one 64-bit store.
#   * build_allowlist() emits the classic-BPF program; -O1 fully UNROLLS the
#     fixed prologue and precomputes each 8-byte instruction word as an immediate.
#   * seccomp_run() is the BPF virtual machine — a one-register accumulator
#     interpreter. This is the function to study; it is annotated in full.
#   * demo_selftest() calls seccomp_run four times; -O1 INLINED the whole VM
#     into each call site and even vectorized the filter-image setup. We annotate
#     one representative copy and summarize, rather than repeat 4 identical loops.
# =============================================================================

	.file	"demo.c"
	.text

# =============================================================================
# bpf_load_u32(const u8 *data /*rdi*/, u32 off /*esi*/) -> u32 /*eax*/
# -----------------------------------------------------------------------------
# Reassemble a 32-bit little-endian word from four bytes:
#     data[off] | data[off+1]<<8 | data[off+2]<<16 | data[off+3]<<24
# This is how classic BPF's BPF_LD|W|ABS pulls the syscall nr / arch out of the
# seccomp_data byte buffer. A leaf function: no calls, so it needs no red-zone
# and touches only caller-saved registers (plus the -O1 frame pointer).
# =============================================================================
	.globl	bpf_load_u32
	.p2align	4
	.type	bpf_load_u32,@function
bpf_load_u32:
	pushq	%rbp                    # PROLOGUE: save caller's frame pointer
	movq	%rsp, %rbp              #   establish our frame (kept at -O1 for gdb)
	# kill: def $esi killed $esi def $rsi   # (compiler note: esi now used as rsi)
	movl	%esi, %eax              # eax = off            (zero-extends into rax)
	movzbl	(%rdi,%rax), %eax       # eax = data[off]      (byte 0, zero-extended)
	leal	1(%rsi), %ecx           # ecx = off + 1        (address math w/o a load)
	movzbl	(%rdi,%rcx), %ecx       # ecx = data[off+1]
	shll	$8, %ecx                # ecx <<= 8            (byte 1 into bits 8..15)
	orl	%eax, %ecx              # ecx = byte1<<8 | byte0
	leal	2(%rsi), %eax           # eax = off + 2
	movzbl	(%rdi,%rax), %edx       # edx = data[off+2]
	shll	$16, %edx               # edx <<= 16           (byte 2 into bits 16..23)
	orl	%ecx, %edx              # edx = byte2<<16 | byte1<<8 | byte0
	addl	$3, %esi                # esi = off + 3
	movzbl	(%rdi,%rsi), %eax       # eax = data[off+3]
	shll	$24, %eax               # eax <<= 24           (byte 3 into the top)
	orl	%edx, %eax              # eax = the full 32-bit LE value -> RETURN
	popq	%rbp                    # EPILOGUE: restore frame pointer
	retq                            #   return; result already in eax
.Lfunc_end0:
	.size	bpf_load_u32, .Lfunc_end0-bpf_load_u32

# =============================================================================
# build_allowlist(struct bpf_insn *prog /*rdi*/, int cap /*esi*/,
#                 const u32 *allow /*rdx*/, int n /*ecx*/) -> int /*eax*/
# -----------------------------------------------------------------------------
# Emit the classic-BPF program. THE LESSON HERE is instruction ENCODING: each
# `struct bpf_insn` is 8 bytes laid out (little-endian) as
#     bits  0..15 = code,  16..23 = jt,  24..31 = jf,  32..63 = k
# so the compiler folds each fixed instruction into ONE 64-bit immediate and
# writes it with a single movq. It also UNROLLS the 6-instruction fixed prologue
# and turns the `if (k >= cap) return -1;` bounds checks into a ladder of cmpl/je
# specialized to the known instruction indices. `eax` starts at -1 (the error
# return) and only becomes the real count at the very end.
# =============================================================================
	.globl	build_allowlist
	.p2align	4
	.type	build_allowlist,@function
build_allowlist:
	movl	$-1, %eax               # default return = -1 ("did not fit")
	testl	%esi, %esi              # cap <= 0 ?
	jle	.LBB1_15                #   yes -> return -1 immediately

# ---- prologue instruction 0:  A = seccomp_data.arch  (BPF_LD|W|ABS, k=4) ----
	movabsq	$17179869216, %r8       # r8 = 0x0000000400000020
	                                #   = code 0x0020 | jt0 | jf0 | k(4)<<32
	movq	%r8, (%rdi)             # prog[0] = that 8-byte instruction word
	cmpl	$1, %esi                # cap == 1 ? (no room for prog[1])
	je	.LBB1_15                #   -> return -1

# ---- prologue instruction 1:  if A == AUDIT_ARCH_X86_64 skip next (jt=1) ----
	movabsq	$-4611685752139349995, %r8  # 0xC000003E00010015
	                                #   = code 0x0015(JMP|JEQ|K) | jt1 | jf0
	                                #     | k(0xC000003E = x86_64 arch)<<32
	movq	%r8, 8(%rdi)            # prog[1] = it
	cmpl	$3, %esi                # cap < 3 ?  (need prog[2])
	jl	.LBB1_15                #   -> return -1

# ---- prologue instruction 2:  RET KILL_PROCESS  (arch mismatch lands here) --
	movabsq	$-9223372036854775802, %r8  # 0x8000000000000006
	                                #   = code 0x06(RET|K) | k(0x80000000 KILL)<<32
	movq	%r8, 16(%rdi)          # prog[2] = it
	je	.LBB1_15                # (reuses flags: cap == 3 -> return -1)

# ---- prologue instruction 3:  A = seccomp_data.nr  (BPF_LD|W|ABS, k=0) ------
	movq	$32, 24(%rdi)          # prog[3] = 0x0000000000000020
	                                #   = code 0x20(LD|W|ABS) | k(0 = nr offset)
	cmpl	$5, %esi                # cap < 5 ?
	jl	.LBB1_15                #   -> return -1

# ---- prologue instruction 4:  if A >= X32_SYSCALL_BIT goto KILL (jf=1) ------
	movabsq	$4611686018444165173, %r9   # 0x4000000001000035
	                                #   = code 0x0035(JMP|JGE|K) | jt0 | jf1
	                                #     | k(0x40000000 = x32 bit)<<32
	movq	%r9, 32(%rdi)          # prog[4] = it
	je	.LBB1_15                # (cap == 5 -> return -1)

# ---- past the fixed part: set up the allowlist-row loop ----------------------
	pushq	%rbp                    # now we need callee-saved regs; build a frame
	movq	%rsp, %rbp
	pushq	%r15                    # save r15/r14/r12/rbx (callee-saved) — we use
	pushq	%r14                    #   them as loop state below
	pushq	%r12
	pushq	%rbx
	movq	%r8, 40(%rdi)          # prologue instruction 5: RET KILL (x32 -> die)
	                                #   (r8 still holds the 0x80..0006 KILL word)
	testl	%ecx, %ecx              # n <= 0 ? (empty allowlist)
	setg	%r10b                   #   r10b = (n > 0)
	setle	%r11b                   #   r11b = (n <= 0)
	cmpl	$7, %esi                # cap < 7 ?
	setl	%bl                     #   bl = (cap < 7)
	movl	$6, %r9d                # r9d = 6 = index of the FIRST allowlist insn
	orb	%r11b, %bl              # skip the loop if (n<=0) OR (cap<7)...
	jne	.LBB1_13                #   ...-> straight to the default-KILL tail

# ---- first allowlist row (peeled): prog[6]=JEQ allow[0], prog[7]=RET ALLOW --
	movl	$16777237, 48(%rdi)     # prog[6].{code,jt,jf} = 0x01000015
	                                #   = code 0x0015(JEQ|K) | jt0 | jf1
	movl	(%rdx), %r9d            # r9d = allow[0]
	movl	%r9d, 52(%rdi)          # prog[6].k = allow[0]
	movl	$7, %r9d                # next instruction index = 7
	cmpl	$8, %esi                # cap < 8 ? (no room for prog[7])
	jb	.LBB1_13                #   -> default-KILL tail
	movl	%ecx, %ecx              # zero-extend n           (loop bound)
	movl	%esi, %ebx              # ebx = cap
	movl	$7, %r11d               # r11 = instruction index (offset/8) = 7
	movl	$8, %r9d                # r9d = next index after the pair
	movl	$1, %r14d               # r14 = allowlist row counter i = 1
	movabsq	$9223090561878065158, %r15  # r15 = 0x7FFF000000000006
	                                #   = the RET ALLOW instruction word
	                                #     (code 0x06 | k(0x7FFF0000 ALLOW)<<32)
.LBB1_10:                               # ===== emit RET ALLOW, then loop body =====
	movq	%r15, (%rdi,%r11,8)     # prog[r11] = RET ALLOW  (the 2nd of the pair)
	cmpq	%rcx, %r14              # i < n ?  (unsigned)
	setb	%r10b                   #   r10b = (i < n)  -> "did we finish naturally"
	jae	.LBB1_13                # i >= n -> done, go to tail
	cmpl	%r9d, %esi              # cap <= next index ?
	jle	.LBB1_13                #   out of room -> tail
# %bb.9: write the next JEQ row (prog[r11+1] = JEQ allow[i])
	movl	$16777237, 8(%rdi,%r11,8)   # prog[r11+1].{code,jt,jf} = 0x01000015
	movl	-10(%rdx,%r11,2), %r12d      # r12 = allow[i]  (address = allow + (r11-5))
	movl	%r12d, 12(%rdi,%r11,8)       # prog[r11+1].k = allow[i]
	addq	$2, %r11                # index += 2 (advance past this pair)
	addl	$2, %r9d                # next-index += 2
	incq	%r14                    # i++
	cmpq	%rbx, %r11              # index < cap ?
	jb	.LBB1_10                #   loop
# %bb.12:
	movl	%r11d, %r9d             # r9d = final instruction index
.LBB1_13:                               # ---- tail: try to append default RET KILL --
	cmpl	%esi, %r9d              # index >= cap ?  (no room for the default)
	setge	%cl                     #   cl = (index >= cap)
	orb	%r10b, %cl              # bail if that OR the loop's "unfinished" flag
	popq	%rbx                    # restore callee-saved regs (frame teardown)
	popq	%r12
	popq	%r14
	popq	%r15
	popq	%rbp
	jne	.LBB1_15                #   -> return -1 (did not fit)
# %bb.14: append the fail-closed default and return the count
	movslq	%r9d, %rax              # rax = index               (sign-extend)
	movq	%r8, (%rdi,%rax,8)      # prog[index] = RET KILL_PROCESS (r8 still KILL)
	incl	%r9d                    # count = index + 1
	movl	%r9d, %eax              # return value = instruction count
.LBB1_15:
	retq                            # return eax (count, or -1 on any overflow)
.Lfunc_end1:
	.size	build_allowlist, .Lfunc_end1-build_allowlist

# =============================================================================
# seccomp_run(const struct bpf_insn *prog /*rdi*/, int len /*esi*/,
#             const u8 *data /*rdx*/, int datalen /*ecx*/) -> u32 /*eax*/
# -----------------------------------------------------------------------------
# THE STAR OF THE FILE: the classic-BPF virtual machine the KERNEL runs on every
# syscall. It is an ACCUMULATOR interpreter:
#     r8d = pc   (program counter / instruction index)
#     r9d = A    (the one 32-bit accumulator register)
#     eax = the action to return
#     r10b is reused as a "keep looping?" flag (1 = continue, 0 = stop/return)
# The C `switch (ins->code)` became a ladder of compares because we generated the
# asm with -fno-jump-tables. Note the two BRANCHLESS jt/jf selections (sete+xor,
# and adc) — a nice optimizer trick worth understanding. Every anomaly path loads
# 0x80000000 (KILL) into eax: the interpreter FAILS CLOSED.
# =============================================================================
	.globl	seccomp_run
	.p2align	4
	.type	seccomp_run,@function
seccomp_run:
	testl	%esi, %esi              # len <= 0 ?
	jle	.LBB2_19                #   yes -> return KILL (empty program is unsafe)
	pushq	%rbp                    # PROLOGUE: frame + one callee-saved reg (rbx)
	movq	%rsp, %rbp
	pushq	%rbx
	xorl	%r8d, %r8d              # pc = 0
	# implicit-def: $eax             # eax (return) is undefined until a RET/anomaly
	xorl	%r9d, %r9d              # A  = 0   (accumulator starts cleared)
	.p2align	4
.LBB2_2:                                # ================= main fetch loop =========
	movslq	%r8d, %r10              # r10 = (long)pc
	leaq	(%rdi,%r10,8), %r11     # r11 = &prog[pc]   (8 bytes per instruction)
	movzwl	(%rdi,%r10,8), %ebx     # ebx = prog[pc].code   (the 16-bit opcode)
	xorl	%r10d, %r10d            # loop flag = 0 (assume we stop unless told else)
	cmpl	$31, %ebx               # opcode > 31 ?  (split the switch in two halves)
	jg	.LBB2_6                 #   high group: JGE(53) and LD(32)

# ---- low group: RET(6) and JEQ(21) ----
	cmpl	$6, %ebx                # opcode == 6  (BPF_RET|BPF_K) ?
	je	.LBB2_10
	cmpl	$21, %ebx               # opcode == 21 (0x15 = BPF_JMP|BPF_JEQ|BPF_K) ?
	jne	.LBB2_17                #   neither -> default: KILL

# ---- case BPF_JMP|BPF_JEQ|BPF_K:  pc += (A==k) ? jt : jf --------------------
	xorl	%r10d, %r10d            # clear r10
	cmpl	4(%r11), %r9d           # compare A (r9d) with ins->k (at insn offset 4)
	sete	%r10b                   # r10b = (A == k) ? 1 : 0
	xorq	$3, %r10                # BRANCHLESS select: 1^3=2, 0^3=3.
	                                #   insn layout: byte 2 = jt, byte 3 = jf, so
	                                #   this picks the OFFSET of jt (eq) or jf (ne).
	movzbl	(%r11,%r10), %r10d      # r10d = ins->jt (if A==k) else ins->jf
	jmp	.LBB2_12                # apply the skip
	.p2align	4
.LBB2_6:                                # ---- high group ----
	cmpl	$53, %ebx               # opcode == 53 (0x35 = BPF_JMP|BPF_JGE|BPF_K) ?
	je	.LBB2_11
	cmpl	$32, %ebx               # opcode == 32 (0x20 = BPF_LD|BPF_W|BPF_ABS) ?
	jne	.LBB2_17                #   neither -> default: KILL

# ---- case BPF_LD|BPF_W|BPF_ABS:  bounds-check, then A = load32(data + k) ----
	movl	4(%r11), %r10d          # r10d = ins->k   (the absolute byte offset)
	leal	4(%r10), %r11d          # r11d = k + 4    (last byte the load touches)
	cmpl	%ecx, %r11d             # (k+4) vs datalen ?
	jbe	.LBB2_18                # if (k+4) <= datalen (unsigned) -> safe load
	xorl	%r10d, %r10d            # else OOB: loop flag = 0 (stop)
	movl	$-2147483648, %eax      # eax = 0x80000000 = KILL   (fail closed)
	jmp	.LBB2_14
.LBB2_17:                               # ---- default: unknown opcode ----
	movl	$-2147483648, %eax      # KILL: a filter we cannot interpret is untrusted
	jmp	.LBB2_14
.LBB2_10:                               # ---- case BPF_RET|BPF_K:  return ins->k ----
	movl	4(%r11), %eax           # eax = ins->k = the SECCOMP_RET action word
	xorl	%r10d, %r10d            # loop flag = 0 -> fall through and return eax
	jmp	.LBB2_14
.LBB2_11:                               # ---- case BPF_JMP|BPF_JGE|BPF_K ------------
	cmpl	4(%r11), %r9d           # compute A - k: sets CF = 1 iff A < k (unsigned)
	adcq	$2, %r11                # r11 += 2 + CF.  A>=k (CF0)->+2=jt offset;
	                                #   A<k (CF1)->+3=jf offset. BRANCHLESS again.
	movzbl	(%r11), %r10d           # r10d = the selected jt/jf skip count
.LBB2_12:                               # ---- apply a jump's skip to pc ----
	addl	%r10d, %r8d             # pc += skip   (loop's pc++ below adds the +1,
	                                #   giving classic-BPF's "skip N after this one")
.LBB2_13:                               # ---- reached after LD or a taken jump ----
	movb	$1, %r10b               # loop flag = 1 (keep interpreting)
.LBB2_14:                               # ---- common continue/stop test ----
	testb	%r10b, %r10b            # loop flag set ?
	je	.LBB2_21                #   0 -> stop and return eax
	incl	%r8d                    # pc++
	cmpl	%esi, %r8d              # pc < len ?
	jl	.LBB2_2                 #   yes -> next instruction
	jmp	.LBB2_20                #   ran off the end without a RET -> KILL
.LBB2_18:                               # ---- inlined bpf_load_u32(data, k) ----
	movzbl	(%rdx,%r10), %r9d       # A  = data[k]            (byte 0)
	leal	1(%r10), %r11d          # r11 = k+1
	movzbl	(%rdx,%r11), %r11d      # data[k+1]
	shll	$8, %r11d               #   << 8
	orl	%r9d, %r11d
	leal	2(%r10), %r9d           # k+2
	movzbl	(%rdx,%r9), %ebx        # data[k+2]
	shll	$16, %ebx               #   << 16
	orl	%r11d, %ebx
	leal	3(%r10), %r9d           # k+3
	movzbl	(%rdx,%r9), %r9d        # data[k+3]
	shll	$24, %r9d               #   << 24
	orl	%ebx, %r9d              # A = full 32-bit LE word (nr or arch)
	jmp	.LBB2_13                # continue looping
.LBB2_19:
	movl	$-2147483648, %eax      # KILL: len <= 0 entry path
	retq
.LBB2_20:
	movl	$-2147483648, %eax      # KILL: fell off the end of the program
.LBB2_21:                               # ---- return path ----
	popq	%rbx                    # restore callee-saved rbx
	popq	%rbp                    # tear down frame
	retq                            # return eax (the action word)
.Lfunc_end2:
	.size	seccomp_run, .Lfunc_end2-seccomp_run

# =============================================================================
# demo_selftest() -> int   (annotated in summary form)
# -----------------------------------------------------------------------------
# The source builds one filter, then runs it against four crafted seccomp_data
# images and checks the verdict each time. At -O1 the optimizer did two striking
# things; see asm/demo.s lines 256-665 for the full, un-elided output:
#
#   1) CONSTANT-FOLDED THE FILTER. build_allowlist({0,1,231}) has no runtime
#      inputs the optimizer cannot see, so the whole 11-instruction program is
#      precomputed and splatted into the stack with 16-byte SSE stores:
#
#         movaps .LCPI3_0(%rip), %xmm0     # xmm0 = [0x20, 4, 0x10015, 0xc000003e]
#         movaps %xmm0, -320(%rbp)         #   = prog[0..1] written 16 bytes at once
#         ... three more movaps for prog[2..7] ...
#
#      Those constants are exactly the packed bpf_insn words we decoded by hand
#      in build_allowlist above (0x1000015 = JEQ|jf1, 0x7fff0000 = ALLOW, etc.).
#
#   2) INLINED seccomp_run FOUR TIMES — once per test case (labels LBB3_3,
#      LBB3_23, LBB3_43, LBB3_63). Each copy is byte-for-byte the interpreter
#      annotated above (fetch loop, sete/xor jt-select, adc jgE-select, inlined
#      load). Between copies it rebuilds only the seccomp_data image (a single
#      movabsq packs nr+arch, e.g. 0xC000003E00000029 = arch x86_64, nr 41 for
#      the "socket is denied" case) and compares the returned action against the
#      expected constant:
#
#         cmpl  $2147418112, %ecx          # == 0x7FFF0000 (ALLOW) ? case 1
#         negl  %ecx ; jno                 # == 0x80000000 (KILL)  ? cases 3,4
#                                          #   (neg overflows iff ecx==0x80000000)
#
# We do not re-list all four identical loops; reading one (seccomp_run, above)
# teaches the whole thing, and duplicating it would hide rather than reveal the
# lesson. The takeaway: an allowlist filter is small enough that the compiler can
# evaluate it entirely at build time — there is nothing magic in seccomp, just a
# few bytes of accumulator-machine code the kernel runs before every syscall.
# =============================================================================

	.ident	"clang version 20.1.8"
	.section	".note.GNU-stack","",@progbits
# =============================================================================
# WHAT TO TAKE AWAY
#   * A struct bpf_insn is 8 bytes; the compiler happily treats one as a single
#     64-bit store. Reading the packed immediates (0x..0015, 0x..0006, ...) IS
#     reading the seccomp program's machine code.
#   * seccomp_run is a tiny accumulator VM: load a word from seccomp_data, a few
#     compares, return an action. Every error path returns KILL — fail closed.
#   * The two branchless jt/jf selects (sete+xorq, and adcq) are worth stealing:
#     they turn "which byte, jt or jf?" into arithmetic with no extra branch.
#   * -O1 constant-folded a whole filter and inlined the VM four times. Keep the
#     asm open: it is how you SEE the optimizer erase your abstractions.
# =============================================================================
