# =============================================================================
# demo.annotated.s — clang -O1 output for asm/demo.c, explained instruction by
#                     instruction. The two star routines are the ELF toolkit's
#                     pure-logic core: sym_by_addr (address->symbol BINARY
#                     SEARCH) and x86_insn_len (the OPCODE-LENGTH DECODER), plus
#                     its helper modrm_bytes and the driver demo_run.
# =============================================================================
#
# HOW TO READ THIS FILE
# ---------------------
# This is the EXACT assembly clang 20 emits for asm/demo.c at -O1 (see demo.s for
# the untouched original), with a comment on essentially every instruction. AT&T
# syntax throughout:  op  src, dst   (destination LAST). `%reg` is a register,
# `$imm` an immediate, `N(%base,%index,scale)` is memory at base+index*scale+N.
# Register widths are the same register: rax(64)/eax(32)/ax(16)/al(8); writing
# eax zero-extends into rax, which is why the compiler prefers `movl`/`leal`.
#
# THE SYSTEM V AMD64 ABI (the contract every function here obeys)
# --------------------------------------------------------------
#   integer/pointer args, in order:  rdi, rsi, rdx, rcx, r8, r9, then the STACK
#   return value:                    rax  (eax for an int)
#   callee-saved (a fn must preserve): rbx, rbp, r12, r13, r14, r15
#   caller-saved (free to clobber):   rax, rcx, rdx, rsi, rdi, r8, r9, r10, r11
#   the "red zone":                  128 bytes below rsp a LEAF may use freely
#   stack alignment:                 rsp % 16 == 0 at the point of any `call`
#
# FOUR OPTIMIZER TRICKS TO WATCH FOR (all appear below)
# -----------------------------------------------------
#   (1) SIGNED /2 via round-toward-zero. `(hi-lo)/2` for a possibly-negative int
#       becomes `x >> 31` (the sign bit) added back before an arithmetic `sar`,
#       so truncation matches C's division. Seen in every midpoint computation.
#   (2) `<= K` rewritten as `< K+1`. demo_run's inlined search compares against
#       0x1091 instead of doing "<= 0x1090", saving an instruction.
#   (3) A SET-MEMBERSHIP TEST becomes a 64-bit BITMAP + `btq`. x86_insn_len's
#       "is this byte a legacy prefix?" is answered by `bt` into a constant
#       `movabsq`-loaded mask instead of a chain of compares. Same trick recurs
#       for the opcode-class groups.
#   (4) BRANCHLESS small arithmetic: `sete`/`cmov` compute "add 4 iff RIP-
#       relative" and "return 0 iff truncated" without jumps.
#
# A NOTE ON x86_insn_len's ANNOTATION DEPTH (honesty first)
# ---------------------------------------------------------
# sym_by_addr, modrm_bytes and demo_run are annotated in FULL below. x86_insn_len
# is a large C if-cascade; at -O1 the optimizer expanded it into ~450 lines that
# include several NEAR-DUPLICATE copies of the "decode ModRM tail + add immediate"
# logic (one per opcode-class that reaches it) and two constant opcode BITMAPS.
# Annotating all 450 near-identical lines would obscure more than it teaches, so
# below we annotate every STRUCTURALLY DISTINCT block in full (prologue, the
# prefix loop with its bitmap, REX, opcode fetch, the 0x0F path, one complete
# ModRM-tail, and the shared return/truncation merge) and mark the duplicated
# blocks as such. For the literal C-statement-to-instruction mapping of the whole
# function, read demo.O0.s, where nothing is duplicated or reordered.
# =============================================================================

	.file	"demo.c"
	.text

# =============================================================================
# sym_by_addr — int sym_by_addr(const struct sym *tab, int n, u64 addr)
#   rdi = tab (array of 16-byte {u64 addr; u64 size}), esi = n, rdx = addr.
#   Returns (eax) the index of the greatest tab[i].addr <= addr, or -1.
#
# This is "find the rightmost element <= key" — a binary search whose invariant
# is `ans` = best index seen so far. clang keeps it fully BRANCHY (a compare and
# two conditional jumps per step) even at -O2; there is no cmov here because both
# arms also update the loop bounds, not just a single value.
# =============================================================================
	.globl	sym_by_addr
	.p2align	4
	.type	sym_by_addr,@function
sym_by_addr:
# %bb.0:  PROLOGUE
	pushq	%rbp                    # save caller's frame pointer
	movq	%rsp, %rbp             # establish our frame (kept at -O1 for debug)
	testl	%esi, %esi             # n <= 0 ?  (set flags from n)
	jle	.LBB0_1                #   yes -> the range is empty, return -1
# %bb.2:  initialise lo/hi/ans
	decl	%esi                    # esi = n - 1  == hi  (last valid index)
	movl	$-1, %eax              # eax = ans = -1   (nothing found yet)
	xorl	%ecx, %ecx             # ecx = lo = 0
	jmp	.LBB0_3                # enter the loop at its test

	.p2align	4
.LBB0_5:                                # <candidate>: tab[mid].addr <= addr
	addl	%r8d, %ecx             # ecx = lo + half ...
	incl	%ecx                    #   ... + 1  == mid + 1  == new lo
	movl	%r9d, %eax             # ans = mid            (r9d holds mid)
	cmpl	%esi, %ecx             # lo <= hi ?
	jg	.LBB0_7                #   lo > hi -> loop done, fall to return
	                                # else fall through into the loop header

.LBB0_3:                                # LOOP HEADER: while (lo <= hi)
	movl	%esi, %r9d             # r9d = hi
	subl	%ecx, %r9d             # r9d = hi - lo
	movl	%r9d, %r8d             # r8d = hi - lo (copy for the /2 fixup)
	shrl	$31, %r8d              # r8d = (hi-lo) >> 31 == sign bit (0 or 1)
	addl	%r9d, %r8d             # r8d = (hi-lo) + signbit   [trick (1): round
	sarl	%r8d                    #   toward zero] then >>1 == (hi-lo)/2 == half
	leal	(%r8,%rcx), %r9d       # r9d = half + lo == mid   (LEA = add, no flags)
	movslq	%r9d, %r10             # r10 = (int64)mid  (sign-extend for indexing)
	shlq	$4, %r10               # r10 = mid * 16   (sizeof(struct sym) == 16)
	cmpq	%rdx, (%rdi,%r10)      # compare tab[mid].addr (first 8 bytes) with addr
	jbe	.LBB0_5                #   tab[mid].addr <= addr -> candidate (LBB0_5)
# %bb.4:  else tab[mid].addr > addr : discard mid, hi = mid - 1
	leal	(%r8,%rcx), %esi       # esi = half + lo == mid  (recompute)
	decl	%esi                    # esi = mid - 1 == new hi
	cmpl	%esi, %ecx             # lo <= hi ?
	jle	.LBB0_3                #   yes -> keep searching
.LBB0_7:  EPILOGUE (found path)
	popq	%rbp                    # restore frame pointer
	retq                            # return eax == ans

.LBB0_1:  n <= 0 : return -1
	movl	$-1, %eax             # eax = -1
	popq	%rbp
	retq
.Lfunc_end0:
	.size	sym_by_addr, .Lfunc_end0-sym_by_addr

# =============================================================================
# modrm_bytes — unsigned modrm_bytes(const u8 *p, unsigned avail, int *ok)
#   rdi = p (points at the ModRM byte), esi = avail, rdx = ok.
#   Returns (eax) bytes consumed by ModRM(+SIB+disp); clears *ok on truncation.
#
# This is the crux of x86 length decoding. The two branchless nuggets below —
# `cmpb $64` to test mod==0, and `sete`+`lea` to add 4 iff a disp32 is implied —
# are worth the price of admission.
# =============================================================================
	.globl	modrm_bytes
	.p2align	4
	.type	modrm_bytes,@function
modrm_bytes:
# %bb.0:  PROLOGUE + the avail<1 guard
	pushq	%rbp
	movq	%rsp, %rbp
	testl	%esi, %esi             # avail == 0 ?  (avail < 1)
	je	.LBB1_1                #   yes -> *ok = 0; return 0
# %bb.2:  load ModRM, extract mod
	movzbl	(%rdi), %r8d           # r8d = m = p[0]        (zero-extend the byte)
	movl	%r8d, %ecx             # ecx = m
	shrl	$6, %ecx               # ecx = mod = m >> 6    (top two bits)
	movl	$1, %eax               # eax = n = 1           (the ModRM byte itself)
	cmpl	$3, %ecx               # mod == 3 ?  (register-direct)
	jne	.LBB1_3                #   no  -> memory forms
.LBB1_13:                               # mod == 3: no SIB, no disp; n stays 1
	popq	%rbp
	retq
.LBB1_1:  avail < 1 : truncated before the ModRM byte
	movl	$0, (%rdx)             # *ok = 0
	xorl	%eax, %eax             # return 0
	popq	%rbp
	retq

.LBB1_3:  memory form: is there a SIB byte? (rm == 100b)
	movl	%r8d, %r9d             # r9d = m
	andl	$7, %r9d               # r9d = rm = m & 7
	cmpl	$4, %r9d               # rm == 4 ?  (SIB present)
	jne	.LBB1_8                #   no  -> plain [reg] / RIP-relative
# %bb.4:  SIB present: need a second byte
	cmpl	$1, %esi               # avail == 1 ?  (avail < 2)
	jne	.LBB1_6                #   avail >= 2 -> decode the SIB
# %bb.5:  avail < 2 : truncated inside the SIB
	movl	$0, (%rdx)             # *ok = 0
	popq	%rbp                    # return n (eax == 1)
	retq

.LBB1_8:  no SIB (rm != 4): plain base, maybe RIP-relative
	xorl	%eax, %eax             # eax = 0
	cmpl	$5, %r9d               # rm == 5 ?  (mod==0 => RIP-relative)
	sete	%al                     # al = (rm == 5) ? 1 : 0
	cmpb	$64, %r8b              # m < 0x40 ?  <=> mod == 0 (top two bits clear)
	leal	1(,%rax,4), %edx       # edx = 1 + 4*(rm==5)  == 5 if RIP-rel else 1
	movl	$1, %eax               # eax = 1  (default: just the ModRM byte)
	cmovbl	%edx, %eax             # if mod==0 (m<0x40): eax = edx  [trick (4)]
	                                #   => adds the RIP-relative disp32 branchlessly
.LBB1_9:  add the displacement selected by `mod`
	cmpl	$2, %ecx               # mod == 2 ?  (disp32)
	je	.LBB1_12               #   yes -> n += 4
# %bb.10:
	cmpl	$1, %ecx               # mod == 1 ?  (disp8)
	jne	.LBB1_13               #   neither 1 nor 2 -> done, return n
# %bb.11:  mod == 1: 8-bit displacement
	incl	%eax                    # n += 1
	popq	%rbp
	retq
.LBB1_12:  mod == 2: 32-bit displacement
	addl	$4, %eax               # n += 4
	popq	%rbp
	retq

.LBB1_6:  SIB present, avail >= 2: n starts at 2 (ModRM + SIB)
	movl	$2, %eax               # eax = n = 2
	cmpb	$63, %r8b              # m > 0x3F ?  <=> mod != 0
	ja	.LBB1_9                #   mod != 0 -> just add disp by mod (LBB1_9)
# %bb.7:  mod == 0: a SIB base==101b means "no base, disp32 instead"
	movzbl	1(%rdi), %eax          # eax = sib = p[1]
	andb	$7, %al                # al = sib & 7 == base field
	xorl	%edx, %edx             # edx = 0
	cmpb	$5, %al                # base == 5 ?
	sete	%dl                     # dl = (base == 5)
	leal	2(,%rdx,4), %eax       # eax = 2 + 4*(base==5)  == 6 if disp32 else 2
	jmp	.LBB1_9                # mod==0 so LBB1_9 adds nothing more
.Lfunc_end1:
	.size	modrm_bytes, .Lfunc_end1-modrm_bytes

# =============================================================================
# x86_insn_len — unsigned x86_insn_len(const u8 *p, unsigned avail)
#   rdi = p, esi = avail.  Returns the total instruction length, or 0.
#
# Register roles the optimizer settled on for the whole function:
#   ebx = i (running length / cursor)   r15 = p66 (saw a 0x66 prefix)
#   r14 = 1+i after the opcode          rcx = the legacy-prefix BITMAP constant
#   rdi = p (the input pointer)         esi = avail
# The stack slot -44(%rbp) is the `int ok` local passed by address to modrm_bytes.
# =============================================================================
	.globl	x86_insn_len
	.p2align	4
	.type	x86_insn_len,@function
x86_insn_len:
# %bb.0:  PROLOGUE — claim the callee-saved regs the long body needs
	pushq	%rbp
	movq	%rsp, %rbp
	pushq	%r15                    # p66 lives here (survives the modrm_bytes call)
	pushq	%r14
	pushq	%r13
	pushq	%r12
	pushq	%rbx                    # i (length) lives here — callee-saved on purpose
	pushq	%rax                    # reserve 8 bytes: the `int ok` local at -44(%rbp)
	xorl	%eax, %eax
	movabsq	$198158383604367617, %rcx  # rcx = 0x02C0000000010101  [trick (3)]:
	                                #   the LEGACY-PREFIX bitmap. Bit (b-0x2E) is set
	                                #   for b in {2E,36,3E,64,65,67}. 0x66 and 0x67
	                                #   share the range but 0x66 is handled apart
	                                #   (it must set p66), so its bit is left 0.
	xorl	%r15d, %r15d            # p66 = 0
	xorl	%ebx, %ebx             # i = 0
	jmp	.LBB2_1

.LBB2_10:                               # (a non-prefix byte fell through here)
	xorl	%edx, %edx             # edx = 0
	testb	%dl, %dl               # 0 == 0 -> always
	je	.LBB2_11               #   leave the prefix loop
	.p2align	4
.LBB2_1:                                # PREFIX LOOP HEADER
	cmpl	%esi, %ebx             # i >= avail ?
	jae	.LBB2_34               #   ran out of bytes -> return 0 (via the tail)
# %bb.2:  read p[i]
	movl	%ebx, %edx             # edx = i
	movzbl	(%rdi,%rdx), %edx      # edx = b = p[i]
	leal	-46(%rdx), %r9d        # r9d = b - 0x2E  (bitmap index base)
	cmpl	$57, %r9d              # (b-0x2E) > 57 ?  i.e. b outside 0x2E..0x67
	ja	.LBB2_5                #   yes -> check the 0xF0.. / 0x26 prefixes
# %bb.3:
	btq	%r9, %rcx              # test bitmap bit (b-0x2E)  [trick (3)]
	jb	.LBB2_8                #   set -> b is a seg/addr-size prefix: consume it
# %bb.4:  not in the bitmap: the only remaining in-range prefix is 0x66
	movl	$1, %r8d               # candidate new p66 = 1
	cmpq	$56, %r9              # (b-0x2E) == 56 ?  i.e. b == 0x66
	je	.LBB2_9                #   yes -> consume and set p66 = 1
	.p2align	4
.LBB2_5:  out-of-range byte: is it 0xF0/0xF2/0xF3, or 0x26?
	leal	-240(%rdx), %r8d       # r8d = b - 0xF0
	cmpl	$3, %r8d               # b in 0xF0..0xF3 ?
	ja	.LBB2_7                #   no -> check 0x26
# %bb.6:
	cmpl	$1, %r8d               # b == 0xF1 ?  (the one non-prefix in that run)
	jne	.LBB2_8                #   b is 0xF0/0xF2/0xF3 -> consume as prefix
.LBB2_7:
	cmpl	$38, %edx              # b == 0x26 ?  (es segment override)
	jne	.LBB2_10               #   not a prefix -> exit the loop
	.p2align	4
.LBB2_8:  consume a prefix that does NOT change p66
	movl	%r15d, %r8d            # keep p66 unchanged
.LBB2_9:  advance past the consumed prefix byte
	incl	%ebx                    # i++
	movb	$1, %dl
	movl	%r8d, %r15d            # p66 = r8d (1 only on the 0x66 path)
	testb	%dl, %dl               # always true -> loop again
	jne	.LBB2_1

.LBB2_11:  ---- REX byte? (0x40..0x4F, the last prefix before the opcode) ----
	movb	$1, %cl
	cmpl	%esi, %ebx             # i >= avail ?
	jae	.LBB2_14               #   out of bytes -> skip REX handling
# %bb.12:
	movl	%ebx, %eax
	movzbl	(%rdi,%rax), %eax      # eax = b = p[i]
	movl	%eax, %edx
	andb	$-16, %dl              # dl = b & 0xF0
	cmpb	$64, %dl               # (b & 0xF0) == 0x40 ?  i.e. b in 0x40..0x4F
	jne	.LBB2_14               #   not a REX -> leave i as is
# %bb.13:  it is a REX prefix: capture REX.W and consume it
	incl	%ebx                    # i++
	testb	$8, %al                # REX.W bit (0x08) set ?
	sete	%cl                     # cl = !(REX.W)   (used far below for imm size)
.LBB2_14:  ---- OPCODE FETCH ----
	xorl	%eax, %eax             # default return value = 0 (truncated)
	cmpl	%esi, %ebx             # i >= avail ?
	jae	.LBB2_34               #   no opcode byte -> return 0
# %bb.15:
	leal	1(%rbx), %r14d         # r14 = i + 1  (length once the opcode is eaten)
	movl	%ebx, %edx
	movzbl	(%rdi,%rdx), %r12d     # r12 = op = p[i]
	cmpb	$15, %r12b             # op == 0x0F ?  (two-byte escape)
	jne	.LBB2_21               #   no -> one-byte opcode dispatch
# %bb.16:  ---- TWO-BYTE OPCODE (0x0F xx) ----
	cmpl	%r14d, %esi            # i+1 >= avail ?  (room for op2 ?)
	jbe	.LBB2_34               #   no -> return 0
# %bb.17:
	movl	%r14d, %eax
	cmpb	$-113, (%rdi,%rax)     # op2 vs 0x8F (-113): op2 > 0x8F ?
	jg	.LBB2_26               #   op2 in 0x90.. -> not a jcc rel32; modrm tail
# %bb.18:  op2 <= 0x8F.  jcc rel32 (0x80..0x8F) => length = (i+2) + 4
	addl	$6, %ebx               # i += 6  (0F + op2 + rel32; i was at the 0F)
.LBB2_19:
	xorl	%eax, %eax
	cmpl	%esi, %ebx             # i+6 <= avail ?
.LBB2_20:
	cmovbel	%ebx, %eax             # eax = (fits) ? length : 0   [trick (4)]
	jmp	.LBB2_34
.LBB2_21:  ---- ONE-BYTE OPCODE DISPATCH (structurally-distinct head) ----
	movl	%r12d, %edx
	andb	$-16, %dl              # dl = op & 0xF0
	cmpb	$80, %dl               # (op & 0xF0) == 0x50 ?  push/pop r (0x50..0x5F)
	je	.LBB2_22               #   yes -> no operands: length = i+1 (r14)
# %bb.23:  test the "no-operand" opcode set via a second BITMAP
	movzbl	%r12b, %edx
	leal	-144(%rdx), %r8d       # r8d = op - 0x90
	cmpl	$60, %r8d              # op in 0x90..0xCC window ?
	ja	.LBB2_28               #   out of window -> other groups
# %bb.24:
	movabsq	$1299288492496388865, %r9  # r9 = 0x1208000000000301  [trick (3)]:
	                                #   bit (op-0x90) set for the no-operand opcodes
	                                #   nop(90)/cwde(98)/cdq(99)/ret(C3)/leave(C9)/
	                                #   int3(CC) that map into this window.
	btq	%r8, %r9              # is op one of them ?
	jae	.LBB2_28               #   no -> fall to the group decoders
.LBB2_22:  no-operand opcode: length is just prefixes+REX+opcode
	movl	%r14d, %eax            # eax = i + 1
	jmp	.LBB2_34               # return it
#
# ----------------------------------------------------------------------------
# BELOW: the one-byte opcode GROUP decoders (ALU r/m forms, group1/2/3/11,
# rel8/rel32 branches, mov-imm). The optimizer emitted these as a web of compares
# and TWO more constant bitmaps (0x4800004, 0x4060000000, 0x1000001, ...), each
# selecting a class, followed by a ModRM tail. The ModRM tail appears in SEVERAL
# near-identical copies (LBB2_30..LBB2_34, LBB2_40..LBB2_58, LBB2_55..LBB2_70):
# each inlines the same "shr $6 => mod; and $7 => rm; add SIB/disp" logic you
# already saw fully annotated in modrm_bytes, then adds the opcode's immediate
# and finally reaches the shared return-merge. ONE representative copy and the
# shared merge are annotated here; the others are the same pattern for a
# different opcode class and are elided for readability (see demo.O0.s for the
# straight-line version).
# ----------------------------------------------------------------------------
#
# ---- representative ModRM tail (LBB2_40): decode ModRM, no/short immediate ----
.LBB2_40:
	movl	%r10d, %r11d           # r11 = ModRM byte
	andl	$7, %r11d              # r11 = rm = ModRM & 7
	cmpl	$4, %r11d              # rm == 4 ?  (SIB present)  -- same test as
	jne	.LBB2_51               #   modrm_bytes; branches to the sub-cases
# %bb.41:
	cmpl	$1, %esi               # avail == 1 ?  (truncation guard)
	je	.LBB2_32               #   yes -> fold into the return with ok==0
# %bb.42:
	movl	$2, %eax               # n = 2 (ModRM + SIB)
	cmpb	$63, %r10b             # mod != 0 ?
	ja	.LBB2_52               #   -> add disp by mod
# %bb.43:  SIB, mod==0: base==5 => extra disp32 (sete + lea, as in modrm_bytes)
	movzbl	1(%rdi,%r9), %eax      # eax = SIB byte
	andb	$7, %al                # al = base
	xorl	%esi, %esi
	cmpb	$5, %al                # base == 5 ?
	sete	%sil                    # sil = (base==5)
	leal	2(,%rsi,4), %eax       # n = 2 + 4*(base==5)
	jmp	.LBB2_52
# ... (LBB2_44..LBB2_97: further opcode-class tests and their ModRM tails —
#      same shapes as above for group3/5, rel8/rel32, mov-imm; elided) ...
#
# ---- the SHARED RETURN MERGE (every path lands here) ----
.LBB2_31:
	xorl	%r8d, %r8d             # r8 = "ok" flag = 0 (not truncated) on this path
.LBB2_32:
	addl	%ebx, %eax             # eax = i + tail_length  == candidate length
.LBB2_33:
	testb	%r8b, %r8b             # was a truncation detected (ok != 0) ?
	cmovnel	%ecx, %eax             # if so, eax = ecx(0)  [trick (4)]: return 0
.LBB2_34:  EPILOGUE — restore the frame and callee-saved regs (reverse order)
	addq	$8, %rsp               # release the `int ok` slot
	popq	%rbx
	popq	%r12
	popq	%r13
	popq	%r14
	popq	%r15
	popq	%rbp
	retq                            # return eax (the instruction length, or 0)
#
# (LBB2_26, LBB2_44..LBB2_97, and the second/third ModRM-tail copies live between
#  the blocks above in demo.s. They are the elided duplicates described in the
#  header; each is a clone of the annotated tail for a different opcode class.)
.Lfunc_end2:
	.size	x86_insn_len, .Lfunc_end2-x86_insn_len

# =============================================================================
# demo_run — the self-test. clang INLINED sym_by_addr (with n=4, addr=0x1090)
# and the sweep loop, but kept the x86_insn_len calls (its body is too large to
# fold). Watch trick (2): the inlined search compares against 0x1091 to realise
# "tab[mid].addr <= 0x1090".
# =============================================================================
	.globl	demo_run
	.p2align	4
	.type	demo_run,@function
demo_run:
# %bb.0:  PROLOGUE — save the callee-saved regs that must survive the calls
	pushq	%rbp
	movq	%rsp, %rbp
	pushq	%r15                    # off (result piece) — survives x86_insn_len
	pushq	%r14                    # p   (sweep cursor)  — survives the call
	pushq	%r12                    # &code[0]            — survives the call
	pushq	%rbx                    # total (length sum)  — survives the call
	movl	$3, %edx               # hi = n-1 = 3        (inlined sym_by_addr)
	movl	$-1, %ecx              # ans = -1
	xorl	%esi, %esi             # lo = 0
	leaq	demo_run.tab(%rip), %rax# rax = &tab[0]  (RIP-relative: PIC-safe)
	jmp	.LBB3_1
	.p2align	4
.LBB3_3:                                # candidate: tab[mid].addr <= 0x1090
	addl	%edi, %esi             # lo += half ...
	incl	%esi                    #   ... +1 == mid+1 == new lo
	movl	%r8d, %ecx             # ans = mid
	cmpl	%edx, %esi             # lo <= hi ?
	jg	.LBB3_5                #   lo > hi -> search done
.LBB3_1:                                # LOOP HEADER (inlined binary search)
	movl	%edx, %r8d             # r8 = hi
	subl	%esi, %r8d             # r8 = hi - lo
	movl	%r8d, %edi             # edi = hi-lo
	shrl	$31, %edi              # edi = sign bit          [trick (1)]
	addl	%r8d, %edi             # edi = (hi-lo)+signbit
	sarl	%edi                    # edi = (hi-lo)/2 == half
	leal	(%rdi,%rsi), %r8d      # r8 = half + lo == mid
	movslq	%r8d, %r9              # r9 = (int64)mid
	shlq	$4, %r9               # r9 = mid * 16   (struct sym stride)
	cmpq	$4241, (%r9,%rax)      # tab[mid].addr <? 0x1091  [trick (2): "<=0x1090"]
	jb	.LBB3_3                #   below -> candidate branch
# %bb.2:  else hi = mid - 1
	leal	(%rdi,%rsi), %edx      # edx = mid
	decl	%edx                    # edx = mid - 1 == new hi
	cmpl	%edx, %esi             # lo <= hi ?
	jle	.LBB3_1                #   yes -> keep searching
.LBB3_5:  search finished: ecx = idx.  Compute off = 0x1090 - tab[idx].addr
	xorl	%r14d, %r14d           # p = 0  (sweep cursor)
	movl	$0, %r15d              # off = 0
	testl	%ecx, %ecx             # idx < 0 ?
	js	.LBB3_7                #   yes -> leave off = 0
# %bb.6:
	movl	%ecx, %ecx             # zero-extend idx
	shlq	$4, %rcx               # rcx = idx * 16
	movl	$4240, %r15d           # r15 = 0x1090
	subl	(%rcx,%rax), %r15d     # off = 0x1090 - tab[idx].addr   (expect 0x10)
.LBB3_7:  linear-sweep the 9 code bytes, summing x86_insn_len
	leaq	demo_run.code(%rip), %r12  # r12 = &code[0]
	xorl	%ebx, %ebx             # total = 0
	.p2align	4
.LBB3_8:                                # while (p < 9)
	cmpl	$8, %r14d              # p > 8 ?  (p reached the end)
	ja	.LBB3_10               #   yes -> done
# %bb.9:
	movl	%r14d, %edi            # arg0 = p ...
	addq	%r12, %rdi             #   ... rdi = code + p
	movl	$9, %esi               # arg1 = 9 ...
	subl	%r14d, %esi            #   ... esi = 9 - p == avail
	callq	x86_insn_len           # eax = len (r14/rbx/r12/r15 preserved across it)
	addl	%eax, %ebx             # total += len
	addl	%eax, %r14d            # p += len
	testl	%eax, %eax             # len == 0 ?  (undecodable byte)
	jne	.LBB3_8                #   no -> continue the sweep
.LBB3_10:  return off + total  (expect 0x10 + 9 == 25)
	addl	%r15d, %ebx            # ebx = total + off
	movl	%ebx, %eax             # return value
	popq	%rbx                    # restore callee-saved regs (reverse of prologue)
	popq	%r12
	popq	%r14
	popq	%r15
	popq	%rbp
	retq
.Lfunc_end3:
	.size	demo_run, .Lfunc_end3-demo_run

# =============================================================================
# READ-ONLY DATA
# =============================================================================
	.type	demo_run.tab,@object
	.section	.rodata,"a",@progbits   # "a" = allocated, read-only
	.p2align	4, 0x0
demo_run.tab:                                   # the 4-entry sorted symbol table
	.quad	4096                            # tab[0].addr = 0x1000
	.quad	64                              # tab[0].size = 0x40
	.quad	4160                            # tab[1].addr = 0x1040
	.quad	64                              #            .size
	.quad	4224                            # tab[2].addr = 0x1080  <- the winner
	.quad	64                              #            .size
	.quad	4352                            # tab[3].addr = 0x1100
	.quad	32                              #            .size
	.size	demo_run.tab, 64

	.type	demo_run.code,@object
demo_run.code:                                  # the 9 prologue bytes we sweep
	.ascii	"UH\211\345H\203\354\020\303"   # 55 48 89 e5 48 83 ec 10 c3
	                                        #   = push rbp; mov rbp,rsp; sub rsp,0x10; ret
	.size	demo_run.code, 9

	.ident	"clang version 20.1.8"
	.section	".note.GNU-stack","",@progbits  # non-executable stack (security)
	.addrsig                                # address-significance table (LTO aid)
	.addrsig_sym demo_run.code

# =============================================================================
# WHAT TO TAKE AWAY
#   * A "find rightmost <= key" binary search is a small variant of the classic:
#     it records `ans = mid` and keeps going right instead of returning on a hit.
#     clang leaves it branchy because each step also moves lo/hi, not just ans.
#   * x86 length decoding is all in the ModRM byte: mod picks disp width, rm==100b
#     pulls in a SIB, and the mod==0 special cases (rm==101b RIP-relative, SIB
#     base==101b) each add a disp32. modrm_bytes is that rule in ~20 instructions.
#   * The optimizer turns SET MEMBERSHIP ("is this byte a prefix / a no-operand
#     opcode?") into a 64-bit constant BITMAP tested with `btq` — far cheaper than
#     a compare chain. It also computes small conditionals (add 4 iff RIP-rel,
#     return 0 iff truncated) with `sete`/`cmov`, avoiding branches entirely.
#   * Compare with demo.O0.s (every statement its own instructions, every local
#     spilled to the stack, no bitmaps, no duplication) to see the same logic
#     before the optimizer reshaped it.
# =============================================================================
