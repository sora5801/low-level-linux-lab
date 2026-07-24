# =============================================================================
# demo.annotated.s — clang -O1 output for asm/demo.c, explained instruction by
#                     instruction. This is the ARITHMETIC CORE of a linker.
# =============================================================================
#
# HOW TO READ THIS FILE
# ---------------------
# This is the exact assembly clang 20 emits for asm/demo.c at -O1 (see the
# untouched asm/demo.s), with a comment on essentially every instruction. AT&T
# syntax throughout:
#
#     op   src, dst                 # movl $1, %eax   =>  eax = 1
#     %reg          register        $imm            immediate (literal)
#     N(%r)         memory [r+N]     (%rb,%ri)       memory [rb + ri]
#
# Same register, different widths: rax(64) / eax(32) / cx(16) / cl(8) / ch(bits
# 8..15). Writing eax zero-extends into rax; that is why clang uses `movl` when
# the top 32 bits should be zero.
#
# THE FUNCTION: apply_reloc(loc, type, S, A, P) -> int
# ----------------------------------------------------
# System V AMD64 passes the five arguments in registers:
#     rdi = loc   pointer to the field bytes inside the output image
#     esi = type  relocation type (R_X86_64_*)
#     rdx = S     symbol's final virtual address
#     rcx = A     addend (signed)
#     r8  = P     virtual address of the field itself (used only by PC-relative)
#     eax = return: 0 RL_OK / 1 RL_TRUNC / 2 RL_BADTYPE
#
# It is a LEAF function (calls nothing — put32/put64 were inlined), so it needs
# no stack beyond the frame pointer, and it may freely clobber the caller-saved
# rax/rcx/rdx/r8. The whole routine is: decode `type`, compute S+A (and -P for
# PC-relative), range-check, then scatter the value out little-endian byte by
# byte. Watch two optimizer tricks in particular:
#   (1) the "fits in signed 32" test done as  (i64)(i32)v == v  with movslq;
#   (2) put32's 4th store and put64's 8th store MERGED into one tail via an
#       index in %rax (3 for a 32-bit field, 7 for a 64-bit field).
# =============================================================================

	.file	"demo.c"
	.text
	.globl	apply_reloc                     # export apply_reloc (extern linkage)
	.p2align	4                       # 16-byte align the entry for fetch
	.type	apply_reloc,@function
apply_reloc:                            # int apply_reloc(u8*,u32,u64,i64,u64)

# ---- PROLOGUE ---------------------------------------------------------------
	pushq	%rbp                    # save caller's frame pointer
	movq	%rsp, %rbp              # rbp = frame base (kept for debuggability)

# ---- SWITCH DISPATCH on `type` (esi) ----------------------------------------
# clang split the switch into two halves around 3: values <=3 (1,2,3) and >3
# (4,10,11). The default answer RL_BADTYPE is pre-loaded into eax so any
# unmatched `jne .Lret` returns 2 without extra work.
	movl	$2, %eax                # eax = 2 = RL_BADTYPE (default result)
	cmpl	$3, %esi                # type <= 3 ?
	jle	.Ldispatch_low          #   yes -> handle {1,2,3}
# %bb.4  (type > 3)
	cmpl	$4, %esi                # type == 4  (R_X86_64_PLT32) ?
	je	.Lpcrel                 #   yes -> PC-relative path (== PC32)
	cmpl	$10, %esi               # type == 10 (R_X86_64_32, unsigned) ?
	je	.Lcase32                #   yes -> unsigned-32 path
	cmpl	$11, %esi               # type == 11 (R_X86_64_32S, signed) ?
	jne	.Lret                   #   no  -> unsupported: return RL_BADTYPE
# %bb.7  (type == 11, R_X86_64_32S)
	addq	%rdx, %rcx              # rcx = A + S     (the value S+A; no -P here)
	jmp	.Lcheck_s32             # share the signed-32 range check below

.Ldispatch_low:                         # type in {1,2,3}
	cmpl	$1, %esi                # type == 1 (R_X86_64_64) ?
	je	.Lcase64                #   yes -> 64-bit absolute store
# %bb.2
	cmpl	$2, %esi                # type == 2 (R_X86_64_PC32) ?
	jne	.Lret                   #   no (type 3 or 0) -> RL_BADTYPE
	# fall through: type == 2 uses the same PC-relative arithmetic as PLT32

# ---- PC-RELATIVE: v = S + A - P  (R_X86_64_PC32 / R_X86_64_PLT32) -----------
.Lpcrel:
	addq	%rdx, %rcx              # rcx = A + S
	subq	%r8,  %rcx              # rcx = A + S - P   => the disp32 value v

# ---- SIGNED-32 RANGE CHECK (shared by PC32/PLT32 and 32S) -------------------
# "Does v fit in a signed 32-bit field?" is answered by sign-extending the low
# 32 bits and asking whether that reproduces the full 64-bit value:
#     (i64)(i32)v == v   <=>   v is in [-2^31, 2^31-1].
.Lcheck_s32:
	movslq	%ecx, %rdx              # rdx = sign_extend32->64( low32(v) )
	movl	$1, %eax                # eax = 1 = RL_TRUNC (tentative)
	cmpq	%rcx, %rdx              # sign-extended low32  ==  full v ?
	je	.Lput32                 #   yes: value fits -> store 4 bytes
	# no: fall through to .Lret with eax = RL_TRUNC (relocation truncated)

# ---- RETURN (eax already holds the status code) -----------------------------
.Lret:
	popq	%rbp                    # restore caller's frame pointer
	retq                            # return eax (0/1/2)

# ---- R_X86_64_32: unsigned-32 range check ----------------------------------
# Fits in unsigned 32 iff the high 32 bits are all zero.
.Lcase32:
	addq	%rdx, %rcx              # rcx = S + A
	movq	%rcx, %rax              # rax = v
	shrq	$32, %rax               # rax = high32(v); sets ZF if it is zero
	movl	$1, %eax                # eax = 1 = RL_TRUNC (does NOT touch flags)
	jne	.Lret                   # high bits set -> too big -> RL_TRUNC
	# else fall through: value fits, store the low 4 bytes

# ---- put32(loc, v): scatter the low 4 bytes little-endian ------------------
# Reached by PC32/PLT32/32S (via .Lcheck_s32) and by R_32 (above). rcx = v.
.Lput32:
	movb	%cl, (%rdi)             # loc[0] = bits  0.. 7   (cl)
	movb	%ch, 1(%rdi)            # loc[1] = bits  8..15   (ch)
	movl	%ecx, %eax              # eax = v
	shrl	$16, %eax               # eax = v >> 16
	movb	%al, 2(%rdi)            # loc[2] = bits 16..23
	shrq	$24, %rcx               # rcx = v >> 24, so cl = bits 24..31
	movl	$3, %eax                # eax = 3 = index of the LAST byte to store
	jmp	.Lstore_tail            # shared tail: loc[3] = cl, then return OK

# ---- put64(loc, v): scatter all 8 bytes little-endian (R_X86_64_64) --------
# No range check: 64 bits holds any address. rcx = S + A.
.Lcase64:
	addq	%rdx, %rcx              # rcx = S + A   (the absolute 64-bit value)
	movb	%cl, (%rdi)             # loc[0] = bits  0.. 7
	movb	%ch, 1(%rdi)            # loc[1] = bits  8..15
	movl	%ecx, %eax
	shrl	$16, %eax
	movb	%al, 2(%rdi)            # loc[2] = bits 16..23
	movl	%ecx, %eax
	shrl	$24, %eax
	movb	%al, 3(%rdi)            # loc[3] = bits 24..31
	movq	%rcx, %rax
	shrq	$32, %rax
	movb	%al, 4(%rdi)            # loc[4] = bits 32..39
	movq	%rcx, %rax
	shrq	$40, %rax
	movb	%al, 5(%rdi)            # loc[5] = bits 40..47
	movq	%rcx, %rax
	shrq	$48, %rax
	movb	%al, 6(%rdi)            # loc[6] = bits 48..55
	shrq	$56, %rcx               # cl = bits 56..63
	movl	$7, %eax                # eax = 7 = index of the LAST byte (loc[7])
	# fall through into the shared tail

# ---- SHARED TAIL: store the final byte and return RL_OK ---------------------
# %rax is 3 (came from put32) or 7 (came from put64); %cl holds that top byte.
# One store serves both widths — a neat size win the optimizer found.
.Lstore_tail:
	movb	%cl, (%rdi,%rax)        # loc[eax] = cl  (loc[3] or loc[7])
	xorl	%eax, %eax              # eax = 0 = RL_OK
	popq	%rbp
	retq
.Lfunc_end0:
	.size	apply_reloc, .Lfunc_end0-apply_reloc

# =============================================================================
# demo_selfcheck() -> u64
# -----------------------
# The lesson here is the OPTIMIZER, not the ISA. demo_selfcheck() called
# apply_reloc twice with compile-time-constant arguments, so clang evaluated
# BOTH relocations at compile time and simply wrote the resulting bytes into the
# stack image. Nothing of apply_reloc survives in this function — only the final
# checksum loop over the 16 already-patched bytes remains. Seeing the linker's
# arithmetic vanish into constants is the whole point of keeping asm open.
# =============================================================================
	.globl	demo_selfcheck
	.p2align	4
	.type	demo_selfcheck,@function
demo_selfcheck:
	pushq	%rbp                    # PROLOGUE
	movq	%rsp, %rbp

# ---- build the 16-byte image on the stack (-16(%rbp) .. -1(%rbp)) -----------
	xorps	%xmm0, %xmm0            # xmm0 = 0 (16 zero bytes)
	movaps	%xmm0, -16(%rbp)        # img[0..15] = 0   (the initial zero fill)

# ---- Scenario 1 result, PRECOMPUTED: a `call` at 0x401000 to S=0x401234.
# apply_reloc(&img[1], PC32, S=0x401234, A=-4, P=0x401001) folds to disp=0x22F.
# Together with the 0xE8 opcode in img[0], the first 4 bytes are E8 2F 02 00.
	movl	$143336, -16(%rbp)      # img[0..3] = 0x00022FE8  (E8,2F,02,00 LE)
                                        #   0xE8 = call opcode; 0x0000022F = disp
	movb	$0, -12(%rbp)           # img[4] = 0 (top byte of the disp32)

# ---- Scenario 2 result, PRECOMPUTED: an absolute 8-byte pointer = 0x401234
# apply_reloc(&img[8], R_64, S=0x401234, A=0, P=0) folds to the raw address.
	movw	$4660, -8(%rbp)         # img[8..9]  = 0x1234        (34,12 LE)
	movb	$64, -6(%rbp)           # img[10]    = 0x40
	movl	$0, -5(%rbp)            # img[11..14]= 0
	movb	$0, -1(%rbp)            # img[15]    = 0
                                        #   => img[8..15] = 0x0000000000401234

# ---- the surviving runtime work: sum = sum*131 + img[i], for i in 0..15 -----
	xorl	%ecx, %ecx              # rcx = i = 0   (loop counter / index)
	xorl	%eax, %eax              # rax = sum = 0
	.p2align	4
.Lsum_loop:
	imulq	$131, %rax, %rdx        # rdx = sum * 131
	movzbl	-16(%rbp,%rcx), %eax    # eax = img[i]  (zero-extended byte)
	addq	%rdx, %rax              # sum = sum*131 + img[i]
	incq	%rcx                    # i++
	cmpq	$16, %rcx               # i == 16 ?
	jne	.Lsum_loop              #   no -> keep folding
# %bb.2
	popq	%rbp                    # EPILOGUE
	retq                            # return sum in rax
.Lfunc_end1:
	.size	demo_selfcheck, .Lfunc_end1-demo_selfcheck

	.ident	"clang version 20.1.8"
	.section	".note.GNU-stack","",@progbits  # non-executable stack: a
                                        #   linker RECORDS this flag; see README's
                                        #   defense notes on W^X and PT_GNU_STACK.
	.addrsig
# =============================================================================
# WHAT TO TAKE AWAY
#   * apply_reloc is the linker in miniature: pick a formula by relocation type,
#     compute S+A (minus P for PC-relative), range-check, patch the bytes.
#   * "Relocation truncated to fit" is the signed/unsigned 32-bit check failing
#     — code and target ended up more than 2 GiB apart.
#   * The optimizer merged put32/put64's final store and constant-folded the
#     whole of demo_selfcheck's relocations. Reading asm is how you SEE that.
# =============================================================================
