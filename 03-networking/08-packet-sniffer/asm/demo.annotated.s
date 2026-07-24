# =============================================================================
# demo.annotated.s — clang -O1 output for bpf_run(), explained instruction by
# instruction. This is the classic-BPF interpreter: the accept/reject VM the
# kernel runs on every captured packet. See asm/demo.c for the C.
# =============================================================================
#
# HOW TO READ THIS FILE (AT&T syntax: `op src, dst`)
# --------------------------------------------------
#   movl $1, %eax      => eax = 1            $imm = literal      %reg = register
#   N(%base,%idx,scale)=> memory at base + idx*scale + N
#   Writing a 32-bit name (eax) ZERO-EXTENDS into the 64-bit reg (rax).
#
# THE SysV AMD64 ABI CONTRACT bpf_run obeys
# -----------------------------------------
#   Integer/pointer ARGS, in order:  rdi, rsi, rdx, rcx, r8, r9
#     -> rdi = prog (const struct bpf_insn *), esi = len (u32),
#        rdx = pkt  (const u8 *),              ecx = plen (u32).
#   RETURN value: rax (we compute it in r9d and copy to eax at the end).
#   CALLEE-SAVED (a function must preserve these for its caller): rbx, rbp,
#     r12, r13, r14, r15 — so the prologue pushes every one we use and the
#     epilogue pops them. CALLER-SAVED (free to clobber): rax, rcx, rdx, rsi,
#     rdi, r8-r11.
#   Stack must be 16-byte aligned at a `call`; bpf_run makes NO calls (every
#     load helper was INLINED), so it is a leaf — but it still builds a frame
#     and uses callee-saved regs, hence the save/restore.
#
# THE REGISTER FILE THE OPTIMIZER CHOSE (stable for the whole function)
# --------------------------------------------------------------------
#   eax  = A     the 32-bit accumulator (every load lands here)
#   ebx  = X     the index register (callee-saved, so it is preserved)
#   r10d = pc    program counter = index into prog[]
#   r11d = oob   sticky "a packet load ran off the end" flag (0 = ok)
#   r9d  = ret   the pending return value we will hand back
#   r8d  = plen  packet length (moved out of ecx up front so ecx is free)
#   rdi=prog  rsi=len  rdx=pkt                       (untouched inputs)
#   mem[16] scratch: 64 bytes on the stack at -112(%rbp); zeroed in prologue.
#   Per-instruction TEMPORARIES: r15d holds `code` then its extracted bit-
#     fields; ecx holds `k` then the second operand `b`; r12d/r13d/r14d are
#     scratch; r14b doubles as the "continue the loop?" flag (1=loop, 0=return).
#
# WHAT THE OPTIMIZER DID TO MY C (the whole lesson)
# -------------------------------------------------
#   1. It turned the nested switch(class){switch(mode)...} into a BINARY
#      COMPARISON TREE (cmpl/jg/je chains) instead of a jump table — because we
#      passed -fno-jump-tables. Watch it bisect: class vs 3, then vs 1, etc.
#   2. It MERGED COMMON TAILS. Every "A = <computed>; pc++; continue" collapses
#      into ONE shared block (LBB0_79 for ALU, LBB0_99 for loads, LBB0_2 for the
#      generic continue). Every "return 0" funnels through LBB0_83/85/101. This
#      is why one asm block often serves many C cases.
#   3. It replaced control flow with BRANCHLESS tricks: `cmov` for the RET value
#      select and the TAX/TXA move; `set<cc>` + `xor $3` to index jt/jf without
#      a branch; `(byte<<2)&60` to compute 4*(byte&0xf) in the IP-header trick.
# =============================================================================

	.file	"demo.c"
	.text
	.globl	bpf_run                         # export bpf_run for the linker
	.p2align	4                       # 16-byte align the entry (I-fetch)
	.type	bpf_run,@function
bpf_run:                                # u32 bpf_run(prog, len, pkt, plen)
# %bb.0:  ---- PROLOGUE: build a frame and save callee-saved registers --------
	pushq	%rbp                    # save caller's frame pointer
	movq	%rsp, %rbp              # rbp = base of our frame
	pushq	%r15                    # \  save the callee-saved registers we
	pushq	%r14                    #  \ are about to use as scratch/state.
	pushq	%r13                    #  / The ABI promises the caller these
	pushq	%r12                    # /  survive the call.
	pushq	%rbx                    # (rbx = X — MUST be preserved)
	movl	%ecx, %r8d              # r8d = plen. Free ecx for per-insn use.
	xorps	%xmm0, %xmm0            # xmm0 = 0 (16 zero bytes)
	movaps	%xmm0, -64(%rbp)        # \ zero the 64-byte mem[16] scratch with
	movaps	%xmm0, -80(%rbp)        #  \ four 16-byte SSE stores: this is the
	movaps	%xmm0, -96(%rbp)        #  / C initializer `u32 mem[16] = {0}`.
	movaps	%xmm0, -112(%rbp)       # / (mem[0] is at the lowest addr, -112)
	xorl	%r11d, %r11d            # oob = 0
	xorl	%r10d, %r10d            # pc  = 0
	xorl	%ebx, %ebx              # X   = 0
	xorl	%eax, %eax              # A   = 0
                                        # r9d (ret) is left undefined until set
	jmp	.LBB0_4                 # enter the dispatch loop at its test

# ---- MISC class (BPF_MISC=7): register move TAX/TXA -------------------------
# Reached when class==7. `code & 0x80` distinguishes TXA (A=X) from TAX (X=A).
.LBB0_1:                                #   in Loop
	testb	%r15b, %r15b            # test bit 7 of code (sign bit of the byte)
	cmovnsl	%eax, %ebx              # if bit7 CLEAR (TAX): X = A  (branchless)
	incl	%r10d                   # pc++
	movl	%ebx, %eax              # A = X. For TAX this is A=A (X just became
                                        #   A); for TXA (bit7 set, cmov skipped)
                                        #   this is the real A=X. One move serves
                                        #   both cases — a neat optimizer fold.
	.p2align	4
# ---- SHARED TAIL: "instruction handled, keep looping" ----------------------
.LBB0_2:                                #   in Loop
	movb	$1, %r14b               # continue-flag = 1 (loop, don't return)
.LBB0_3:                                #   in Loop
	testb	%r14b, %r14b            # continue?
	je	.LBB0_106               #   0 => leave the loop and return r9d
# ---- DISPATCH LOOP TOP: `while (pc < len)` ---------------------------------
.LBB0_4:                                # =>This Inner Loop Header
	cmpl	%esi, %r10d             # compare pc (r10d) with len (esi)
	jae	.LBB0_105               # pc >= len => fell off the end, return 0
# %bb.5:  ---- fetch the current instruction prog[pc] -------------------------
	movl	%r10d, %r14d            # r14 = pc (zero-extended index)
	movzwl	(%rdi,%r14,8), %r15d    # r15d = code. Stride 8 = sizeof(bpf_insn);
                                        #   code is the u16 at offset 0.
	movl	4(%rdi,%r14,8), %ecx    # ecx = k (the u32 at offset 4)
	movl	%r15d, %r12d            # r12d = code
	andl	$7, %r12d               # r12d = code & 7 = CLASS
	cmpl	$3, %r12d               # bisect the 8 classes: is CLASS > 3?
	jg	.LBB0_13                #   yes (4..7: ALU/JMP/RET/MISC)
# %bb.6:                                #   CLASS in 0..3
	cmpl	$1, %r12d
	jg	.LBB0_20                #   CLASS 2 or 3 => ST / STX
# %bb.7:
	testl	%r12d, %r12d
	jne	.LBB0_24                #   CLASS == 1 => LDX
# %bb.8:  ---- CLASS==0: LD (load into A) — switch on addressing MODE ---------
	movl	%r15d, %r12d
	shrl	$5, %r12d               # \ r12d = (code >> 5) & 7 = MODE index:
	andl	$7, %r12d               # / IMM=0 ABS=1 IND=2 MEM=3 LEN=4 MSH=5
	xorl	%r14d, %r14d            # pre-clear the continue-flag byte
	cmpl	$1, %r12d
	jle	.LBB0_41                #   MODE <= 1 => IMM or ABS
# %bb.9:
	cmpl	$2, %r12d
	je	.LBB0_64                #   MODE == 2 => IND (A = P[X+k])
# %bb.10:
	cmpl	$3, %r12d
	je	.LBB0_69                #   MODE == 3 => MEM (A = M[k])
# %bb.11:
	movl	%r8d, %ecx              # ecx = plen (operand for the LEN case)
	cmpl	$4, %r12d
	je	.LBB0_99                #   MODE == 4 => LEN (A = plen), via A=ecx
	jmp	.LBB0_83                #   MODE == 5 (MSH invalid for LD) => ret 0
	.p2align	4
# ---- CLASS 2/3: ST / STX (store A or X into scratch M[k]) -------------------
.LBB0_20:                               #   in Loop
	andl	$15, %ecx               # ecx = k & 0xf (mem[] has 16 slots)
	cmpl	$2, %r12d
	jne	.LBB0_33                #   CLASS 3 => STX
# %bb.21:                               #   CLASS 2 => ST: M[k] = A
	movl	%eax, -112(%rbp,%rcx,4) # store A into mem[k&0xf]
	incl	%r10d                   # pc++
	jmp	.LBB0_2                 # continue
	.p2align	4
# ---- CLASS 6/7: RET / MISC -------------------------------------------------
.LBB0_13:                               #   in Loop
	cmpl	$5, %r12d
	jg	.LBB0_22                #   CLASS 6 or 7 => RET / MISC
# %bb.14:
	cmpl	$4, %r12d
	jne	.LBB0_28                #   CLASS == 5 => JMP
# %bb.15:  ---- CLASS==4: ALU — switch on operator, pick operand b -----------
	testb	$8, %r15b               # source bit: code & BPF_X(0x08)?
	cmovnel	%ebx, %ecx              # if X-source: b = X (else b stays = k)
	shrl	$4, %r15d               # \ r15d = (code >> 4) & 0xf = ALU OP:
	andl	$15, %r15d              # / ADD0 SUB1 MUL2 DIV3 OR4 AND5 LSH6 RSH7
	xorl	%r14d, %r14d
	cmpl	$3, %r15d
	jg	.LBB0_34                #   OP > 3 => OR/AND/LSH/RSH
# %bb.16:
	cmpl	$1, %r15d
	jg	.LBB0_51                #   OP 2 or 3 => MUL / DIV
# %bb.17:
	testl	%r15d, %r15d
	je	.LBB0_75                #   OP == 0 => ADD
# %bb.18:
	cmpl	$1, %r15d
	jne	.LBB0_101               #   not 1 => unknown op => return 0
# %bb.19:                               #   OP == 1 => SUB
	subl	%ecx, %eax              # A = A - b
	jmp	.LBB0_79                # ALU write-back tail (pc++, continue)
	.p2align	4
# ---- CLASS==6: RET is the ONLY exit; also the MISC fork --------------------
.LBB0_22:                               #   in Loop
	cmpl	$6, %r12d
	jne	.LBB0_1                 #   CLASS == 7 => MISC (TAX/TXA above)
# %bb.23:                               #   CLASS == 6 => RET
	andl	$24, %r15d              # r15d = code & 0x18 = RVAL field
	cmpl	$16, %r15d              # RVAL == BPF_A (0x10 = 16)?
	cmovel	%eax, %ecx              # if so, return value = A; else it stays = k
	xorl	%r14d, %r14d            # continue-flag = 0 => we will RETURN
	movl	%ecx, %r9d              # ret = (A or k)
	jmp	.LBB0_3                 # -> flag is 0 -> fall to the epilogue
# ---- CLASS==1: LDX (load into X) — switch on MODE --------------------------
.LBB0_24:                               #   in Loop
	shrl	$5, %r15d               # \ r15d = (code>>5)&7 = MODE index
	andl	$7, %r15d               # /
	xorl	%r14d, %r14d
	cmpl	$3, %r15d
	jg	.LBB0_38                #   MODE > 3 => LEN or MSH
# %bb.25:
	testl	%r15d, %r15d
	je	.LBB0_58                #   MODE == 0 => IMM (X = k)
# %bb.26:
	cmpl	$3, %r15d
	jne	.LBB0_83                #   not MEM => return 0 (ABS/IND invalid→X)
# %bb.27:                               #   MODE == 3 => MEM (X = M[k])
	andl	$15, %ecx               # k & 0xf
	movl	-112(%rbp,%rcx,4), %r12d # r12 = mem[k&0xf]
	movl	%r11d, %r13d            # r13 = oob (carry it through the shared tail)
	jmp	.LBB0_63                # LDX write-back tail (X = r12, pc++)
# ---- CLASS==5: JMP — pick operand, switch on the branch op -----------------
.LBB0_28:                               #   in Loop
	movl	%r15d, %r12d
	andl	$240, %r12d             # r12d = code & 0xf0 = JMP OP
	je	.LBB0_47                #   OP == 0 => JA (unconditional)
# %bb.29:
	testb	$8, %r15b               # source bit: X or K?
	cmovnel	%ebx, %ecx              # if X-source: b = X (else b = k)
	addl	$-16, %r12d             # \ normalize: subtract JEQ's 0x10 then /16,
	shrl	$4, %r12d               # / giving JEQ=0 JGT=1 JGE=2 JSET=3
	xorl	%r15d, %r15d            # r15b = 0 = "did we resolve a branch?" flag
	cmpl	$1, %r12d
	jg	.LBB0_48                #   op 2 or 3 => JGE / JSET
# %bb.30:
	testl	%r12d, %r12d
	je	.LBB0_70                #   op == 0 => JEQ
# %bb.31:
	cmpl	$1, %r12d
	jne	.LBB0_85                #   not JGT => unknown => return 0
# %bb.32:                               #   op == 1 => JGT
	cmpl	%ecx, %eax              # compare A (eax) with b (ecx)
	seta	%cl                     # cl = (A > b) unsigned  (branchless test)
	jmp	.LBB0_72                # shared jt/jf resolver
# ---- CLASS==3: STX (M[k] = X) ----------------------------------------------
.LBB0_33:                               #   in Loop
	movl	%ebx, -112(%rbp,%rcx,4) # store X into mem[k&0xf]
	incl	%r10d                   # pc++
	jmp	.LBB0_2                 # continue
# ---- ALU op >= 4 sub-tree: OR / AND / LSH / RSH ----------------------------
.LBB0_34:                               #   in Loop
	cmpl	$5, %r15d
	jg	.LBB0_55                #   OP 6 or 7 => LSH / RSH
# %bb.35:
	cmpl	$4, %r15d
	je	.LBB0_76                #   OP == 4 => OR
# %bb.36:
	cmpl	$5, %r15d
	jne	.LBB0_101               #   not AND => unknown => return 0
# %bb.37:                               #   OP == 5 => AND
	andl	%eax, %ecx              # ecx = A & b
	movl	%ecx, %eax              # A = A & b
	jmp	.LBB0_79                # ALU write-back tail
# ---- LDX MODE >= 4 sub-tree: LEN / MSH -------------------------------------
.LBB0_38:                               #   in Loop
	cmpl	$5, %r15d
	je	.LBB0_59                #   MODE == 5 => MSH (the IP-hdr-len trick)
# %bb.39:
	movl	%r11d, %r13d            # carry oob into the tail
	movl	%r8d, %r12d             # r12 = plen (the LEN value)
	cmpl	$4, %r15d
	je	.LBB0_63                #   MODE == 4 => LEN (X = plen)
	jmp	.LBB0_83                #   else invalid => return 0
# ---- LD IMM / ABS sub-tree -------------------------------------------------
.LBB0_41:                               #   in Loop
	testl	%r12d, %r12d
	je	.LBB0_99                #   MODE == 0 => IMM (A = k), via A=ecx
# %bb.42:
	cmpl	$1, %r12d
	jne	.LBB0_83                #   not ABS => return 0
# %bb.43:                               #   MODE == 1 => ABS: switch on SIZE
	andl	$24, %r15d              # r15d = code & 0x18 = SIZE (W=0 H=8 B=16)
	cmpl	$8, %r15d
	je	.LBB0_88                #   SIZE == H => 2-byte big-endian load
# %bb.44:
	testl	%r15d, %r15d
	jne	.LBB0_94                #   SIZE == B (16) => 1-byte load
# %bb.45:  ---- LD ABS word: A = P[k], 32-bit big-endian, bounds-checked -----
	leal	4(%rcx), %eax           # eax = k + 4  (one past the last byte read)
	cmpl	%r8d, %eax
	seta	%al                     # al = (k+4 > plen)  -> would read past end
	cmpl	$-4, %ecx
	setae	%r14b                   # r14b = (k >= -4) -> the k+4 add overflowed
	orb	%al, %r14b              # OOB if either: matches load_word's check
	jne	.LBB0_96                #   OOB => set A=0, oob=1, then drop
# %bb.46:
	movzbl	(%rdx,%rcx), %eax       # eax = P[k] (first/most-significant byte)
	jmp	.LBB0_68                # assemble the remaining 3 bytes
# ---- JMP JA (unconditional): pc += 1 + k -----------------------------------
.LBB0_47:                               #   in Loop  (reached from the JMP op==0 test)
	addl	%ecx, %r10d             # pc += k
	incl	%r10d                   # pc += 1
	jmp	.LBB0_2                 # continue
# ---- SHARED "return 0" (unknown class / invalid LD/LDX mode) ---------------
.LBB0_83:                               #   in Loop
	xorl	%r9d, %r9d              # ret = 0 (drop)
	jmp	.LBB0_3                 # continue-flag already 0 => return
# ---- JMP op 2/3 sub-tree: JGE / JSET ---------------------------------------
.LBB0_48:                               #   in Loop
	cmpl	$2, %r12d
	je	.LBB0_71                #   op == 2 => JGE
# %bb.49:
	cmpl	$3, %r12d
	jne	.LBB0_85                #   not JSET => unknown => return 0
# %bb.50:                               #   op == 3 => JSET (bit test)
	testl	%eax, %ecx              # A & b
	setne	%cl                     # cl = (A & b) != 0
	jmp	.LBB0_72                # shared jt/jf resolver
# ---- ALU op 2/3 sub-tree: MUL / DIV ----------------------------------------
.LBB0_51:                               #   in Loop
	cmpl	$2, %r15d
	je	.LBB0_77                #   OP == 2 => MUL
# %bb.52:
	cmpl	$3, %r15d
	jne	.LBB0_101               #   not DIV => unknown => return 0
# %bb.53:                               #   OP == 3 => DIV (guard divide-by-0)
	testl	%ecx, %ecx
	je	.LBB0_100               #   b == 0 => return 0 (the kernel does too)
# %bb.54:
	movq	%rdx, %r14              # save pkt: DIV clobbers edx (the remainder)
	xorl	%edx, %edx              # edx = 0: zero-extend the dividend to edx:eax
	divl	%ecx                    # A = (edx:eax) / b ; edx = remainder
	movq	%r14, %rdx              # restore pkt pointer
	jmp	.LBB0_79                # ALU write-back tail
# ---- ALU op 6/7 sub-tree: LSH / RSH ----------------------------------------
.LBB0_55:                               #   in Loop
	cmpl	$6, %r15d
	je	.LBB0_78                #   OP == 6 => LSH
# %bb.56:
	cmpl	$7, %r15d
	jne	.LBB0_101               #   not RSH => unknown => return 0
# %bb.57:                               #   OP == 7 => RSH
	                                #   (cl already holds b's low byte)
	shrl	%cl, %eax               # A >>= (b & 31): x86 shifts mask cl to 5
                                        #   bits, exactly the C `b & 31`.
	jmp	.LBB0_79
# ---- LDX IMM: X = k --------------------------------------------------------
.LBB0_58:                               #   in Loop
	movl	%r11d, %r13d            # carry oob
	movl	%ecx, %r12d             # r12 = k
	jmp	.LBB0_63                # X = r12, pc++, continue
# ---- LDX MSH: X = 4 * (P[k] & 0xf) — reach past variable IP options --------
.LBB0_59:                               #   in Loop
	xorl	%r14d, %r14d
	movl	$1, %r13d               # assume OOB (r13 = local oob = 1)
	movl	$0, %r12d               # X value defaults to 0
	cmpl	%r8d, %ecx              # k < plen ?
	jae	.LBB0_61                #   k >= plen => leave OOB, skip the load
# %bb.60:
	movzbl	(%rdx,%rcx), %r12d      # r12 = P[k] (the version/IHL byte)
	shll	$2, %r12d               # \ (P[k] << 2) & 60  ==  4 * (P[k] & 0xf):
	andl	$60, %r12d              # / low nibble * 4 = IP header length in bytes
	movl	%r11d, %r13d            # r13 = oob(0): the load succeeded
.LBB0_61:                               #   in Loop
	testl	%r13d, %r13d            # was it OOB?
	je	.LBB0_63                #   no => commit X = r12, continue
# %bb.62:                               #   yes => propagate oob and drop
	movl	%r13d, %r11d            # oob = 1
	movl	%r12d, %ebx             # X = 0 (harmless; we're about to return)
	xorl	%r9d, %r9d              # ret = 0
	jmp	.LBB0_3                 # continue-flag is 0 => return 0
# ---- SHARED LDX write-back: commit X, pc++, continue -----------------------
.LBB0_63:                               #   in Loop
	incl	%r10d                   # pc++
	movl	%r13d, %r11d            # restore oob flag
	movl	%r12d, %ebx             # X = computed value (k / plen / mem / MSH)
	jmp	.LBB0_2                 # continue
# ---- LD IND: A = P[X + k]; compute the effective offset, then load ---------
.LBB0_64:                               #   in Loop
	andl	$24, %r15d              # SIZE = code & 0x18
	addl	%ebx, %ecx              # ecx = k + X  (the indirect byte offset)
	cmpl	$8, %r15d
	je	.LBB0_86                #   H => 2-byte big-endian load
# %bb.65:
	testl	%r15d, %r15d
	jne	.LBB0_92                #   B => 1-byte load
# %bb.66:                               #   W => 4-byte load (bounds check first)
	leal	4(%rcx), %eax           # eax = off + 4
	cmpl	%r8d, %eax
	seta	%al                     # off+4 > plen ?
	cmpl	$-4, %ecx
	setae	%r14b                   # off+4 overflowed ?
	orb	%al, %r14b
	jne	.LBB0_96                #   OOB => drop
# %bb.67:
	movl	%ecx, %eax
	movzbl	(%rdx,%rax), %eax       # eax = P[off] (top byte)
# ---- SHARED word assembler: fold P[off..off+3] big-endian into eax ---------
.LBB0_68:                               #   in Loop  (eax already = P[off])
	shll	$24, %eax               # P[off] << 24  (most significant)
	leal	1(%rcx), %r14d
	movzbl	(%rdx,%r14), %r14d      # P[off+1]
	shll	$16, %r14d
	orl	%eax, %r14d
	leal	2(%rcx), %eax
	movzbl	(%rdx,%rax), %eax       # P[off+2]
	shll	$8, %eax
	orl	%r14d, %eax             # eax = P[off]<<24 | P[off+1]<<16 | P[off+2]<<8
	addl	$3, %ecx                # advance offset to the last byte
	jmp	.LBB0_91                # OR in P[off+3], then the load tail
# ---- LD MEM: A = M[k] ------------------------------------------------------
.LBB0_69:                               #   in Loop
	andl	$15, %ecx               # k & 0xf
	movl	-112(%rbp,%rcx,4), %ecx # ecx = mem[k&0xf]
	jmp	.LBB0_99                # A = ecx, pc++, continue
# ---- SHARED "return 0" for unknown JMP op ----------------------------------
.LBB0_85:                               #   in Loop
	xorl	%r9d, %r9d              # ret = 0
	jmp	.LBB0_73                # r15b(resolved?)=0 => fall through to return
# ---- JEQ ------------------------------------------------------------------
.LBB0_70:                               #   in Loop
	cmpl	%ecx, %eax
	sete	%cl                     # cl = (A == b)
	jmp	.LBB0_72
# ---- JGE ------------------------------------------------------------------
.LBB0_71:                               #   in Loop
	cmpl	%ecx, %eax
	setae	%cl                     # cl = (A >= b) unsigned
# ---- SHARED conditional-branch resolver: pc += 1 + (taken ? jt : jf) -------
.LBB0_72:                               #   in Loop  (cl = 1 if the test is true)
	leaq	(%rdi,%r14,8), %r14     # r14 = &prog[pc] (r14 still = pc index)
	movzbl	%cl, %ecx               # ecx = taken (0 or 1)
	xorq	$3, %rcx                # taken?1^3=2 : 0^3=3. In the 8-byte insn,
                                        #   offset 2 = jt, offset 3 = jf — so this
                                        #   picks the right branch byte, no branch.
	movzbl	(%r14,%rcx), %ecx       # ecx = taken ? jt : jf
	addl	%ecx, %r10d             # pc += offset
	incl	%r10d                   # pc += 1 (offsets are from the NEXT insn)
	movb	$1, %r15b               # mark "branch resolved"
.LBB0_73:                               #   in Loop
	testb	%r15b, %r15b
	jne	.LBB0_2                 #   resolved => continue the loop
	jmp	.LBB0_81                #   else (unknown op) => return 0
# ---- ADD ------------------------------------------------------------------
.LBB0_75:                               #   in Loop
	addl	%eax, %ecx
	movl	%ecx, %eax              # A = A + b
	jmp	.LBB0_79
# ---- OR --------------------------------------------------------------------
.LBB0_76:                               #   in Loop
	orl	%eax, %ecx
	movl	%ecx, %eax              # A = A | b
	jmp	.LBB0_79
# ---- MUL -------------------------------------------------------------------
.LBB0_77:                               #   in Loop
	imull	%eax, %ecx
	movl	%ecx, %eax              # A = A * b
	jmp	.LBB0_79
# ---- LSH -------------------------------------------------------------------
.LBB0_78:                               #   in Loop
	shll	%cl, %eax               # A <<= (b & 31)
# ---- SHARED ALU write-back: pc++, continue ---------------------------------
.LBB0_79:                               #   in Loop  (A already holds the result)
	incl	%r10d                   # pc++
	movb	$1, %r14b               # continue-flag = 1
	testb	%r14b, %r14b
	jne	.LBB0_2                 # continue
.LBB0_81:                               #   in Loop
	xorl	%r14d, %r14d            # continue-flag = 0
	jmp	.LBB0_3                 # => return r9d (0)
# ---- LD ABS half: A = P[k], 16-bit big-endian ------------------------------
.LBB0_86:                               #   in Loop  (offset in ecx: here = k+X for IND)
	leal	2(%rcx), %eax           # off + 2
	cmpl	%r8d, %eax
	seta	%al                     # off+2 > plen ?
	cmpl	$-2, %ecx
	setae	%r14b                   # off+2 overflowed ?
	orb	%al, %r14b
	jne	.LBB0_96                #   OOB => drop
# %bb.87:
	movl	%ecx, %eax
	movzbl	(%rdx,%rax), %eax       # eax = P[off] (high byte)
	jmp	.LBB0_90
.LBB0_88:                               #   in Loop  (LD ABS half; offset = k)
	leal	2(%rcx), %eax
	cmpl	%r8d, %eax
	seta	%al
	cmpl	$-2, %ecx
	setae	%r14b
	orb	%al, %r14b
	jne	.LBB0_96                #   OOB => drop
# %bb.89:
	movzbl	(%rdx,%rcx), %eax       # eax = P[k] (high byte)
.LBB0_90:                               #   in Loop
	shll	$8, %eax                # high byte << 8
	incl	%ecx                    # point at the low byte
.LBB0_91:                               #   in Loop  (fall-in from the word path too)
	movzbl	(%rdx,%rcx), %ecx       # ecx = P[off+1] (low byte)
	orl	%eax, %ecx              # ecx = big-endian value assembled
	jmp	.LBB0_97                # load tail (checks oob, commits A)
# ---- LD/LDX byte loads (IND then ABS) --------------------------------------
.LBB0_92:                               #   in Loop  (IND byte: offset = k+X in ecx)
	cmpl	%r8d, %ecx
	jae	.LBB0_96                #   off >= plen => OOB => drop
# %bb.93:
	movl	%ecx, %eax
	movzbl	(%rdx,%rax), %ecx       # ecx = P[off]
	jmp	.LBB0_97
.LBB0_94:                               #   in Loop  (ABS byte: offset = k in ecx)
	cmpl	%r8d, %ecx
	jae	.LBB0_96                #   off >= plen => OOB => drop
# %bb.95:
	movzbl	(%rdx,%rcx), %ecx       # ecx = P[k]
	jmp	.LBB0_97
# ---- OOB handler: a packet load ran past the captured length ---------------
.LBB0_96:                               #   in Loop
	xorl	%ecx, %ecx              # value = 0
	movl	$1, %r11d               # oob = 1  (sticky)
# ---- SHARED load tail: if oob -> drop (ret 0); else A = value, pc++ --------
.LBB0_97:                               #   in Loop  (ecx = loaded value)
	xorl	%r14d, %r14d
	testl	%r11d, %r11d            # oob?
	je	.LBB0_99                #   no => commit A and continue
# %bb.98:                               #   yes => `if (oob) return 0`
	movl	%ecx, %eax              # A = 0
	xorl	%r9d, %r9d              # ret = 0
	jmp	.LBB0_3                 # continue-flag 0 => return 0
# ---- SHARED "A = ecx; pc++; continue" (IMM, LEN, MEM, and every good load) -
.LBB0_99:                               #   in Loop
	incl	%r10d                   # pc++
	movl	%ecx, %eax              # A = ecx (k / plen / mem / packet value)
	jmp	.LBB0_2                 # continue
# ---- DIV-by-zero funnels here ----------------------------------------------
.LBB0_100:                              #   in Loop
	xorl	%r14d, %r14d            # continue-flag = 0
.LBB0_101:                              #   in Loop  (shared unknown-sub-op => 0)
	xorl	%r9d, %r9d              # ret = 0
	testb	%r14b, %r14b
	jne	.LBB0_2                 #   (defensive) continue if flagged
	jmp	.LBB0_81                # else return 0
# ---- pc >= len: ran off the end without a RET => drop ----------------------
.LBB0_105:
	xorl	%r9d, %r9d              # ret = 0
# ---- EPILOGUE: return r9d in eax, restore callee-saved regs ----------------
.LBB0_106:
	movl	%r9d, %eax              # rax = return value (SysV: result in rax)
	popq	%rbx                    # \ restore the callee-saved registers in
	popq	%r12                    #  \ REVERSE push order, so each caller sees
	popq	%r13                    #  / its value intact (LIFO on the stack).
	popq	%r14                    # /
	popq	%r15
	popq	%rbp                    # restore caller's frame pointer
	retq                            # pop return address, jump back to caller
.Lfunc_end0:
	.size	bpf_run, .Lfunc_end0-bpf_run
                                        # -- End function
	.ident	"clang version 20.1.8"
	.section	".note.GNU-stack","",@progbits  # non-executable stack
	.addrsig
# =============================================================================
# WHAT TO TAKE AWAY
#   * A cBPF program is just loads + compares + forward jumps + one return. The
#     kernel runs exactly this loop per packet; SO_ATTACH_FILTER hands it in.
#   * Every packet load is bounds-checked (the LBB0_96 funnel). That is what
#     lets the kernel run an untrusted filter on hostile packets safely.
#   * -O1 dissolved a two-level switch into a comparison tree and merged every
#     duplicate tail. One asm block (LBB0_79, LBB0_99, LBB0_2, LBB0_72) now
#     serves many C cases — read the labels as "the shared tail for ...".
#   * Branchless idioms to notice: cmov (RET-value select, TAX/TXA), set<cc>
#     + xor $3 (index jt vs jf), (byte<<2)&60 (compute 4*(byte&0xf)).
#   Compare with demo.O0.s to see the same logic written the naive way — every
#   switch a chain of compares, every variable spilled to the stack.
# =============================================================================
