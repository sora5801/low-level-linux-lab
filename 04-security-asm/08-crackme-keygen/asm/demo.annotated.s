# =============================================================================
# demo.annotated.s — clang -O1 output for asm/demo.c, explained line by line.
# =============================================================================
#
# HOW TO READ THIS FILE
# ---------------------
# This is the EXACT assembly clang 20.1 emits for asm/demo.c at -O1 (the
# untouched copy is demo.s), with a comment on essentially every instruction.
# AT&T syntax:   op  src, dst      # e.g.  movq %rax, %r8  =>  r8 = rax
#   %reg = register   $imm = literal   sym(%rip) = RIP-relative addr of sym
#   N(%base,%index) = memory at [base + index + N]
# Register widths are the SAME register: rax(64)/eax(32)/ax(16)/al(8). Writing
# eax ZERO-EXTENDS into rax, which is why constants that fit in 32 bits load via
# `movl`, while the 64-bit hash constants need `movabsq`.
#
# WHY THIS IS THE FILE YOU REVERSE
# --------------------------------
# asm/demo.c is a header-free mirror of serial.h — the same transform the
# crackme checks against. So the instructions below are precisely what you would
# see in `objdump -d crackme` for the license check. Three landmarks let you
# FIND this routine in a stripped binary:
#     movabsq $0x100000001b3   the FNV prime  (the hash multiply)
#     rolq    $7               the rotate     (the single most recognizable op)
#     movabsq $0xff51afd7ed558ccd ... the MurmurHash3 fmix64 finalizer constant
# Grep the disassembly for any of those and you have landed on the keygen math.
#
# THE SYSTEM V AMD64 ABI (what every function here obeys)
#     integer/pointer args:  rdi, rsi, rdx, rcx, r8, r9   (then the stack)
#     return value:          rax
#     callee-saved:          rbx, rbp, r12-r15  (must be preserved)
#     caller-saved:          rax, rcx, rdx, rsi, rdi, r8-r11  (free scratch)
#     stack at a `call`:     rsp % 16 == 0
# Both functions below are leaves that make no calls (everything inlined), so
# they use only caller-saved scratch and never touch the callee-saved set except
# rbp, which -O1 keeps purely for a legible backtrace.
# =============================================================================

	.file	"demo.c"
	.text

# =============================================================================
# u64 key_from_name(const char *name)      name in %rdi   ->   key in %rax
# -----------------------------------------------------------------------------
# The transform: for each byte  h = rotl64((h ^ byte) * PRIME, 7) ^ XOR_CONST,
# then a fmix64 avalanche. Register plan the optimizer chose:
#     %r8  = h        (the running 64-bit hash / accumulator)
#     %rdi = cursor   (walks the NUL-terminated name)
#     %sil = byte     (current input byte, low 8 bits of %rsi)
#     %rax = PRIME    (loop-invariant, hoisted out of the loop)
#     %rcx = XOR_CONST(loop-invariant, hoisted)
#     %rdx = scratch  (this iteration's new h, before commit to %r8)
# =============================================================================
	.globl	key_from_name
	.p2align	4
	.type	key_from_name,@function
key_from_name:
# ---- entry: seed h and peek at the first byte -------------------------------
	movabsq	$-3750763034362895579, %r8      # r8 = h = 0xCBF29CE484222325
                                                #   (FNV-1a offset basis). objdump
                                                #   shows it signed; same 64 bits.
	movzbl	(%rdi), %esi                    # sil = (u8)name[0], zero-extended.
                                                #   movzbl, not movb, so the high
                                                #   bits of rsi are cleared — we
                                                #   want the byte's numeric value.
	testb	%sil, %sil                      # first byte == 0 ? (empty string)
	je	.LBB0_1                         # yes -> skip the loop, h stays = basis

# ---- loop preamble (name is non-empty) --------------------------------------
# %bb.3:
	pushq	%rbp                            # save frame ptr (kept at -O1 only for
	movq	%rsp, %rbp                      #   debuggability; this leaf needs no
                                                #   frame otherwise).
	incq	%rdi                            # cursor -> &name[1]; name[0] is already
                                                #   in sil, so we start the loop body
                                                #   with the first byte in hand.
	movabsq	$1099511628211, %rax            # rax = FNV_PRIME = 0x100000001B3.
                                                #   Hoisted: it is loop-invariant, so
                                                #   load it once, not every iteration.
	movabsq	$25214903917, %rcx              # rcx = XOR_CONST = 0x5DEECE66D (also
                                                #   hoisted).
	.p2align	4                       # align the hot loop head for the fetcher

# ---- the per-byte mixing loop -----------------------------------------------
# Invariant on entry: r8 = h so far, sil = the byte to fold in this iteration.
.LBB0_4:
	movzbl	%sil, %edx                      # rdx = (u64)byte  (zero-extend again;
                                                #   sil may have come from memory)
	xorq	%r8, %rdx                       # rdx = h ^ byte            [xor-in]
	imulq	%rax, %rdx                      # rdx = (h ^ byte) * PRIME  [multiply
                                                #   mod 2^64; PRIME is odd, hence an
                                                #   invertible bijection — the reason
                                                #   FNV/Murmur multipliers are odd]
	rolq	$7, %rdx                        # rdx = rotl64(rdx, 7)      [rotate:
                                                #   feeds the high bits the multiply
                                                #   just churned back into the low
                                                #   bits so later bytes affect all]
	xorq	%rcx, %rdx                      # rdx ^= XOR_CONST          [diffuse]
	movzbl	(%rdi), %esi                    # sil = *cursor  (peek NEXT byte)
	incq	%rdi                            # cursor++  (advance past it)
	movq	%rdx, %r8                       # h = rdx  (commit this iteration)
	testb	%sil, %sil                      # next byte == 0 (NUL terminator)?
	jne	.LBB0_4                         # no -> mix the next byte

# ---- fall-through out of the loop; h is in BOTH r8 and rdx ------------------
# %bb.5:
	popq	%rbp                            # restore caller's frame ptr
	jmp	.LBB0_2                         # go to the finalizer, which reads rdx
                                                #   (rdx still equals the final h)

# ---- empty-string path: no bytes mixed, h is the bare offset basis ----------
.LBB0_1:
	movq	%r8, %rdx                       # rdx = h  (so the finalizer, which
                                                #   reads rdx, sees the basis)

# ---- fmix64 avalanche finalizer:  in rdx -> out rax -------------------------
# h ^= h>>33; h *= C1; h ^= h>>29; h *= C2; h ^= h>>33.
# Each step is individually invertible; together they scatter every input bit
# across the whole 64-bit output so the serial has no visible structure.
.LBB0_2:
	movq	%rdx, %rax                      # rax = h
	shrq	$33, %rax                       # rax = h >> 33   (logical: h is unsigned)
	xorq	%rdx, %rax                      # rax = h ^ (h >> 33)
	movabsq	$-49064778989728563, %rcx       # rcx = FMIX_C1 = 0xFF51AFD7ED558CCD
	imulq	%rax, %rcx                      # rcx = (h ^ h>>33) * C1     [= new h]
	movq	%rcx, %rax                      # rax = h
	shrq	$29, %rax                       # rax = h >> 29
	xorq	%rcx, %rax                      # rax = h ^ (h >> 29)
	movabsq	$-4265267296055464877, %rcx     # rcx = FMIX_C2 = 0xC4CEB9FE1A85EC53
	imulq	%rax, %rcx                      # rcx = (h ^ h>>29) * C2     [= new h]
	movq	%rcx, %rax                      # rax = h
	shrq	$33, %rax                       # rax = h >> 33
	xorq	%rcx, %rax                      # rax = h ^ (h >> 33)  -> RETURN VALUE
	retq                                    # return key in rax
.Lfunc_end0:
	.size	key_from_name, .Lfunc_end0-key_from_name

# =============================================================================
# int validate(const char *name, const char *entered)
#     name in %rdi, entered in %rsi   ->   1 (accept) / 0 (reject) in %eax
# -----------------------------------------------------------------------------
# The optimizer INLINED key_from_name, format_serial, slen, and ct_equal into
# one function. So validate is four movements you can spot:
#   (A) the same mixing loop + finalizer as above (computes the expected key);
#   (B) format the key into a 19-char serial on the stack at -32(%rbp);
#   (C) slen(entered) — walk to its NUL, require length == 19;
#   (D) constant-time compare of 19 bytes -> return 1/0.
# The stack buffer `expected[20]` lives in the red zone at -32(%rbp).
# =============================================================================
	.globl	validate
	.p2align	4
	.type	validate,@function
validate:
	pushq	%rbp                            # PROLOGUE: this function DOES use a
	movq	%rsp, %rbp                      #   frame — expected[20] is addressed
                                                #   as -32(%rbp) below (red zone).

# ---- (A) inlined key_from_name(name): identical loop, regs shifted ----------
# Here h lives in %r9 (r8/sil are the byte-load pair). See key_from_name above
# for the full commentary; only the register names differ.
	movabsq	$-3750763034362895579, %r9      # r9 = h = FNV offset basis
	movzbl	(%rdi), %r8d                    # r8b = name[0]
	testb	%r8b, %r8b                      # empty?
	je	.LBB1_1                         # yes -> skip loop
# %bb.2:
	incq	%rdi                            # cursor -> name[1]
	movabsq	$1099511628211, %rax            # rax = PRIME
	movabsq	$25214903917, %rcx              # rcx = XOR_CONST
	.p2align	4
.LBB1_3:
	movzbl	%r8b, %edx                      # rdx = byte
	xorq	%r9, %rdx                       # h ^ byte
	imulq	%rax, %rdx                      # * PRIME
	rolq	$7, %rdx                        # rotl 7
	xorq	%rcx, %rdx                      # ^ XOR_CONST
	movzbl	(%rdi), %r8d                    # peek next byte
	incq	%rdi                            # advance
	movq	%rdx, %r9                       # commit h
	testb	%r8b, %r8b                      # NUL?
	jne	.LBB1_3                         # loop
	jmp	.LBB1_4                         # done -> finalizer (h in rdx)
.LBB1_1:
	movq	%r9, %rdx                       # empty path: h (basis) into rdx

# ---- finalizer (same fmix64 as key_from_name); result (the key) ends in rax -
.LBB1_4:
	movq	%rdx, %rax
	shrq	$33, %rax
	xorq	%rdx, %rax                      # h ^= h>>33
	movabsq	$-49064778989728563, %rcx       # FMIX_C1
	imulq	%rax, %rcx                      # h *= C1
	movq	%rcx, %rax
	shrq	$29, %rax
	xorq	%rcx, %rax                      # h ^= h>>29
	movabsq	$-4265267296055464877, %rcx     # FMIX_C2
	imulq	%rax, %rcx                      # h *= C2
	movq	%rcx, %rax
	shrq	$33, %rax
	xorq	%rcx, %rax                      # h ^= h>>33  -> rax = expected key

# ---- (B) inlined format_serial(key, expected): key in rax -> -32(%rbp) -------
# The C loop was `for gi in 0..3: group=(key>>(48-16*gi))&0xFFFF; emit 4 hex; dash`.
# The optimizer reshaped it into a countdown on the SHIFT AMOUNT in %cl:
#     cl = 48, 32, 16, 0   (subtract 16 each pass; stop when it would hit -16)
# %edx is the output index `oi`; %rdi now points at the 16-byte HEX table.
	xorl	%edx, %edx                      # oi = 0  (output cursor)
	movl	$48, %ecx                       # cl = 48 = first shift (bits 63..48)
	leaq	format_serial.HEX(%rip), %rdi   # rdi = &"0123456789ABCDEF" (RIP-rel)
	jmp	.LBB1_5                         # enter the group loop

# ---- dash + advance between groups (runs after groups 0,1,2) -----------------
.LBB1_7:
	addl	$5, %edx                        # oi += 5  (4 hex digits + 1 dash)
	movl	%r8d, %r8d                      # zero-extend oi-of-dash into r8 (addr math)
	movb	$45, -32(%rbp,%r8)              # expected[dash_pos] = '-' (ASCII 45)
.LBB1_8:
	addq	$-16, %rcx                      # next group: shift -= 16  (48->32->16->0)
	cmpq	$-16, %rcx                      # went past the last group (0 - 16)?
	je	.LBB1_9                         # yes -> all four groups done
.LBB1_5:                                        # === group loop body: emit one group ===
	movq	%rax, %r8                       # r8 = key
	shrq	%cl, %r8                        # r8 = key >> cl   (this group to the low bits)
	movl	%r8d, %r9d                      # r9 = low 32 bits of the shifted key
	shrl	$12, %r9d                       # nibble 3 = (group >> 12)
	andl	$15, %r9d                       #          & 0xF
	movzbl	(%r9,%rdi), %r9d                # r9b = HEX[nibble3]
	leal	1(%rdx), %r10d                  # r10 = oi+1  (precompute next indices)
	movl	%edx, %r11d                     # r11 = oi
	movb	%r9b, -32(%rbp,%r11)            # expected[oi+0] = HEX[nibble3]
	movl	%r8d, %r9d
	shrl	$8, %r9d                        # nibble 2 = (group >> 8)
	andl	$15, %r9d
	movzbl	(%r9,%rdi), %r9d                # HEX[nibble2]
	leal	2(%rdx), %r11d                  # r11 = oi+2
	movb	%r9b, -32(%rbp,%r10)            # expected[oi+1] = HEX[nibble2]
	movl	%r8d, %r9d
	shrl	$4, %r9d                        # nibble 1 = (group >> 4)
	andl	$15, %r9d
	movzbl	(%r9,%rdi), %r9d                # HEX[nibble1]
	leal	3(%rdx), %r10d                  # r10 = oi+3
	movb	%r9b, -32(%rbp,%r11)            # expected[oi+2] = HEX[nibble1]
	andl	$15, %r8d                       # nibble 0 = group & 0xF
	movzbl	(%r8,%rdi), %r9d                # HEX[nibble0]
	leal	4(%rdx), %r8d                   # r8 = oi+4  (position of the coming dash)
	movb	%r9b, -32(%rbp,%r10)            # expected[oi+3] = HEX[nibble0]
	testq	%rcx, %rcx                      # was this the LAST group (cl == 0)?
	jne	.LBB1_7                         # no  -> write a dash, then next group
# %bb.6:                                        # yes -> last group, no trailing dash
	movl	%r8d, %edx                      # oi += 4 (no dash); oi is now 19
	jmp	.LBB1_8                         # rejoin the loop tail (will exit)

# ---- NUL-terminate the formatted serial -------------------------------------
.LBB1_9:
	movl	%edx, %eax                      # eax = oi (== 19)
	movb	$0, -32(%rbp,%rax)              # expected[19] = '\0'

# ---- (C) inlined slen(entered): find the terminator, need length 19 ---------
# rsi = entered. Walk with rcx starting at -1 so that after the loop rcx equals
# the string length (it counts bytes until the NUL, off-by-one via the pre-incr).
	movq	$-1, %rcx                       # rcx = -1 (length accumulator - 1)
	.p2align	4
.LBB1_10:
	cmpb	$0, 1(%rsi,%rcx)                # is entered[rcx+1] == 0 ?
	leaq	1(%rcx), %rcx                   # rcx++  (advance regardless — no early
                                                #   data-dependent timing here anyway)
	jne	.LBB1_10                        # not NUL -> keep scanning
# %bb.11:
	xorl	%eax, %eax                      # default return = 0 (reject)
	cmpl	$19, %ecx                       # strlen(entered) == 19 ?
	jne	.LBB1_15                        # no -> return 0 (length gate failed)

# ---- (D) inlined ct_equal(entered, expected, 19): constant-time compare -----
# The C was `diff |= a[i]^b[i]` for all 19 bytes, then a branch-free zero test.
# KEY OPTIMIZER INSIGHT: clang PROVED that `1u ^ ((diff|(0-diff))>>31)` equals
# "diff == 0" and lowered the whole idiom to a single `sete` (setcc) at the end.
# `sete` is itself branchless, so the constant-time property SURVIVES the
# optimization — the loop still touches all 19 bytes with no early-out.
# %bb.12:
	xorl	%eax, %eax                      # i = 0            (loop counter, rax)
	xorl	%ecx, %ecx                      # diff = 0         (accumulator, ecx)
	.p2align	4
.LBB1_13:
	movzbl	-32(%rbp,%rax), %edx            # dl = expected[i]
	xorb	(%rsi,%rax), %dl                # dl = expected[i] ^ entered[i]
	movzbl	%dl, %edx                       # zero-extend the byte diff
	orl	%edx, %ecx                      # diff |= (that byte's difference)
	incq	%rax                            # i++
	cmpq	$19, %rax                       # i == 19 ? (fixed count -> data-
	jne	.LBB1_13                        #   independent timing; no early break)
# %bb.14:
	xorl	%eax, %eax                      # eax = 0
	testl	%ecx, %ecx                      # diff == 0 ?  (all bytes matched?)
	sete	%al                             # al = (diff == 0) ? 1 : 0  [branchless]
.LBB1_15:
	popq	%rbp                            # EPILOGUE
	retq                                    # return 1 (accept) / 0 (reject) in eax
.Lfunc_end1:
	.size	validate, .Lfunc_end1-validate

# ---- read-only data: the hex digit table ------------------------------------
	.type	format_serial.HEX,@object
	.section	.rodata.str1.16,"aMS",@progbits,1
	.p2align	4, 0x0
format_serial.HEX:
	.asciz	"0123456789ABCDEF"              # 16 digits + NUL; indexed by nibble
	.size	format_serial.HEX, 17

	.ident	"clang version 20.1.8"
	.section	".note.GNU-stack","",@progbits  # non-executable stack (default)
	.addrsig
# =============================================================================
# WHAT TO TAKE AWAY
#   * The keygen math is three landmarks in the asm: movabsq of the FNV prime,
#     `rolq $7`, and the fmix64 movabsq constants. Find them and you have the
#     algorithm — that is the entire static-RE step of this project.
#   * The optimizer inlined FOUR functions into `validate` and reshaped the
#     format loop into a shift-amount countdown. Reading asm is how you SEE that;
#     compare with demo.O0.s (naive, one function-call per routine) and
#     demo.O2.s (even more aggressive).
#   * A hand-rolled constant-time compare collapsed to a single `sete`, and the
#     timing-safety held. The defense lesson: constant-time code must be VERIFIED
#     in the emitted asm, because the optimizer is free to reintroduce a branch —
#     here it did not, but you only know that by looking.
# =============================================================================
