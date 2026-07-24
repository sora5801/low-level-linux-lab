# =============================================================================
# demo.annotated.s — clang's -O1 output for demo.c, explained line by line.
# =============================================================================
#
# HOW TO READ THIS FILE
# ---------------------
# This is the exact assembly clang 20 emits for asm/demo.c at -O1 (see demo.s for
# the untouched original), with a comment on essentially every instruction. AT&T
# syntax: `op src, dst`. `%reg` is a register, `$imm` an immediate, `N(%reg)` is
# memory at [reg+N], and `(%base,%index)` is [base+index].
#
# Register widths are the SAME register: rax(64)/eax(32)/ax(16)/al(8), and ch is
# bits 8..15 of ecx. Writing a 32-bit name (eax) zero-extends into the 64-bit
# register, so clang prefers `movl`/`incl` whenever the top 32 bits should be 0.
#
# THE TWO FUNCTIONS
# -----------------
#   u32 crc32_ieee(const u8 *data, usize len);
#   u64 wal_frame_record(u8 *out, u8 op, const u8 *key, u32 klen,
#                        const u8 *val, u32 vlen);
#
# SysV AMD64 ABI (how args arrive; see ../../../tools/gen_asm.sh):
#   integer/pointer args, in order:  rdi, rsi, rdx, rcx, r8, r9
#   return value:                    rax (eax for 32-bit)
#   callee-saved (must preserve):    rbx, rbp, r12-r15
#   caller-saved (scratch):          rax, rcx, rdx, rsi, rdi, r8-r11
# Both functions are leaves (they call nothing — copy_bytes/put_u32/crc32 were all
# INLINED), so they need no stack locals; only rbp is pushed, and that only
# because -O1 keeps a frame pointer for debuggability. Everything else stays in
# registers, which is the point: framing a log record is pure register math.
#
# THE BIG PICTURE
# ---------------
# crc32_ieee is a textbook reflected CRC: init all-ones, per byte do 8 rounds of
# "shift right one; if the bit shifted out was 1, xor the reflected polynomial
# 0xEDB88320", then invert. wal_frame_record lays out [reclen][crc][body], writing
# every multi-byte length little-endian, then calls the (inlined) CRC over the
# body. Watch two things: (1) the branchless mask idiom the optimizer uses for
# "xor the polynomial only if the low bit is set", and (2) how put_u32 — written
# as four byte stores in C — is sometimes kept as bytes and sometimes fused back
# into one 32-bit store, because x86 is little-endian and unaligned stores are
# legal. The source spelled out the bytes for PORTABILITY; the optimizer is free
# to recombine them for THIS target.
# =============================================================================

	.file	"demo.c"
	.text

# =============================================================================
# u32 crc32_ieee(const u8 *data /*rdi*/, usize len /*rsi*/)  ->  u32 in eax
# CRC-32/IEEE, bit-at-a-time. Returns the checksum of data[0..len).
# =============================================================================
	.globl	crc32_ieee
	.p2align	4                       # 16-byte align the entry (I-fetch friendly)
	.type	crc32_ieee,@function
crc32_ieee:
# ---- PROLOGUE ---------------------------------------------------------------
	pushq	%rbp                    # save caller's frame pointer
	movq	%rsp, %rbp              # rbp = frame base (kept only for debug at -O1)

# ---- guard: empty input -----------------------------------------------------
	testq	%rsi, %rsi              # len == 0 ?  (test sets ZF if rsi is zero)
	je	.LBB0_1                 # yes -> return 0 (the empty-message CRC)

# ---- init the register (crc = 0xFFFFFFFF) and the byte index ----------------
# %bb.4:                                # fall-through: len > 0
	movl	$-1, %eax               # crc = 0xFFFFFFFF  (-1 as u32 == all ones)
	xorl	%ecx, %ecx              # i = 0  (xor r,r is the 2-byte zero idiom)
	.p2align	4

# ---- OUTER LOOP: for each byte data[i] --------------------------------------
.LBB0_5:                                # do { ... } while (++i != len)
	movzbl	(%rdi,%rcx), %edx       # edx = data[i], zero-extended to 32 bits
	xorl	%edx, %eax              # crc ^= data[i]   (fold the message byte in)
	movl	$8, %edx                # k = 8  (process the byte's 8 bits)
	.p2align	4

# ---- INNER LOOP: 8 rounds of polynomial long division -----------------------
# The reflected CRC step:  out = (crc >> 1) ^ (POLY if (crc & 1) else 0).
# The optimizer renders "if (crc&1) xor POLY" branchlessly with a negate-to-mask.
.LBB0_6:                                # for (k = 8; k; k--)
	movl	%eax, %r8d              # r8d = crc
	shrl	%r8d                    # r8d = crc >> 1   (shift the register down one)
	andl	$1, %eax                # eax = crc & 1    (the bit we just shifted out)
	negl	%eax                    # eax = -(crc&1) = 0x00000000 or 0xFFFFFFFF (mask)
	andl	$-306674912, %eax       # eax = mask & 0xEDB88320  (poly, or 0 if bit clear)
	xorl	%r8d, %eax              # crc = (crc>>1) ^ (poly-or-0)
	decl	%edx                    # k--
	jne	.LBB0_6                 # repeat for all 8 bits

# ---- advance to the next byte ----------------------------------------------
# %bb.7:                                #   in Loop: Header=BB0_5
	incq	%rcx                    # i++
	cmpq	%rsi, %rcx              # i == len ?
	jne	.LBB0_5                 # no -> next byte

# ---- EPILOGUE: final invert and return --------------------------------------
# %bb.2:
	notl	%eax                    # crc = ~crc  (final xor with 0xFFFFFFFF)
	popq	%rbp                    # restore frame pointer
	retq                            # return crc in eax

# ---- len == 0 path ----------------------------------------------------------
.LBB0_1:
	xorl	%eax, %eax              # return 0. Correct: 0xFFFFFFFF ^ 0xFFFFFFFF == 0,
	                                #   so the compiler folded the whole init+invert
	                                #   of the empty message straight to the constant 0.
	popq	%rbp
	retq
.Lfunc_end0:
	.size	crc32_ieee, .Lfunc_end0-crc32_ieee

# =============================================================================
# u64 wal_frame_record(u8 *out /*rdi*/, u8 op /*sil*/, const u8 *key /*rdx*/,
#                      u32 klen /*ecx*/, const u8 *val /*r8*/, u32 vlen /*r9d*/)
#   -> u64 record length in rax
#
# Writes  [u32 reclen][u32 crc][ op | klen | key | vlen | val ]  into `out`.
# body starts at out+8; reclen and crc are backfilled once body length is known.
# =============================================================================
	.globl	wal_frame_record
	.p2align	4
	.type	wal_frame_record,@function
wal_frame_record:
# ---- PROLOGUE ---------------------------------------------------------------
	pushq	%rbp
	movq	%rsp, %rbp

# ---- "if (op == OP_DEL) vlen = 0;"  as a branchless conditional-move ---------
	xorl	%r10d, %r10d            # r10d = 0  (candidate effective vlen)
	cmpl	$2, %esi                # op == OP_DEL(2) ?
	cmovnel	%r9d, %r10d             # if op != DEL, r10d = vlen; else keep 0
	                                #   => r10d = effective vlen (0 for a delete)

# ---- body = out + 8;  body[0] = op ------------------------------------------
	leaq	8(%rdi), %r9            # r9 = out + 8 = body pointer (LEA = address calc)
	movb	%sil, 8(%rdi)           # body[0] = op   (store op's low byte at out+8)

# ---- put_u32(body+1, klen): four little-endian byte stores ------------------
# Here the optimizer KEPT put_u32 as byte stores (the destination out+9 is
# misaligned and split across a nice register-byte pattern).
	movb	%cl, 9(%rdi)            # body[1] = klen bits  0..7   (cl)
	movb	%ch, 10(%rdi)           # body[2] = klen bits  8..15  (ch = ecx>>8)
	movl	%ecx, %eax              # eax = klen
	shrl	$16, %eax               # eax = klen >> 16
	movb	%al, 11(%rdi)           # body[3] = klen bits 16..23
	movl	%ecx, %eax              # eax = klen (reload; eax was clobbered)
	shrl	$24, %eax               # eax = klen >> 24
	movb	%al, 12(%rdi)           # body[4] = klen bits 24..31

# ---- copy_bytes(body+5, key, klen) ------------------------------------------
	movl	%ecx, %esi              # esi = klen (reuse rsi as the byte count/offset)
	testl	%ecx, %ecx              # klen == 0 ?
	je	.LBB1_3                 # skip the copy entirely if the key is empty
# %bb.1:
	xorl	%eax, %eax              # i = 0
	.p2align	4
.LBB1_2:                                # for (i = 0; i != klen; i++)
	movzbl	(%rdx,%rax), %ecx       # ecx = key[i]
	movb	%cl, 13(%rdi,%rax)      # (out+13)[i] = key[i]   (out+13 == body+5)
	incq	%rax                    # i++
	cmpq	%rax, %rsi              # i == klen ?
	jne	.LBB1_2
.LBB1_3:

# ---- put_u32(body+5+klen, vlen): ONE 32-bit store this time -----------------
# Same put_u32 source, but here clang fused the four bytes into a single movl.
# Legal because x86 stores are little-endian and may be unaligned — so the
# portable byte-spelling in C costs nothing on this target.
	movl	%r10d, 5(%r9,%rsi)      # *(u32*)(body + 5 + klen) = vlen   (r9=body, rsi=klen)

# ---- rax = running body length so far = 9 + klen ----------------------------
	leaq	9(%rsi), %rax           # n = klen + 9  (op1 + klen4 + key(klen) + vlen4)
	movl	%r10d, %ecx             # ecx = effective vlen (loop bound below)
	testl	%r10d, %r10d            # vlen == 0 ?  (true for DEL or empty value)
	je	.LBB1_6                 # skip the value copy

# ---- copy_bytes(body+9+klen, val, vlen) -------------------------------------
# %bb.4:
	leaq	(%r9,%rsi), %rdx        # rdx = body + klen
	addq	$9, %rdx                # rdx = body + klen + 9 = value destination
	xorl	%esi, %esi              # j = 0
	.p2align	4
.LBB1_5:                                # for (j = 0; j != vlen; j++)
	movzbl	(%r8,%rsi), %r10d       # r10d = val[j]
	movb	%r10b, (%rdx,%rsi)      # dest[j] = val[j]
	incq	%rsi                    # j++
	cmpq	%rsi, %rcx              # j == vlen ?
	jne	.LBB1_5
.LBB1_6:

# ---- finalize body length: n = (9 + klen) + vlen ----------------------------
	addq	%rcx, %rax              # rax = 9 + klen + vlen = total body length (n)

# ---- put_u32(out, reclen = n): the length frame -----------------------------
	movl	%eax, (%rdi)            # *(u32*)out = reclen   (single LE store)

# ---- crc32_ieee(body, n)  INLINED -------------------------------------------
# Identical inner math to crc32_ieee above; r9 = body, rax = n (byte count).
	movl	$-1, %ecx               # crc = 0xFFFFFFFF
	xorl	%edx, %edx              # i = 0
	.p2align	4
.LBB1_7:                                # for each body byte
	movzbl	(%r9,%rdx), %esi        # esi = body[i]
	xorl	%esi, %ecx              # crc ^= body[i]
	movl	$8, %esi                # k = 8
	.p2align	4
.LBB1_8:                                # 8 bit rounds (see crc32_ieee for the idiom)
	movl	%ecx, %r8d              # r8d = crc
	shrl	%r8d                    # r8d = crc >> 1
	andl	$1, %ecx                # ecx = crc & 1
	negl	%ecx                    # ecx = 0x0 or 0xFFFFFFFF (mask)
	andl	$-306674912, %ecx       # & 0xEDB88320  (reflected polynomial)
	xorl	%r8d, %ecx              # crc = (crc>>1) ^ (poly-or-0)
	decl	%esi                    # k--
	jne	.LBB1_8
# %bb.9:
	incq	%rdx                    # i++
	cmpq	%rax, %rdx              # i == n ?
	jne	.LBB1_7

# ---- put_u32(out+4, crc) and return 8 + n -----------------------------------
# %bb.10:
	notl	%ecx                    # crc = ~crc  (final invert)
	movl	%ecx, 4(%rdi)           # *(u32*)(out+4) = crc   (the record's checksum)
	addq	$8, %rax                # rax = 8 (header) + n (body) = total record bytes
	popq	%rbp
	retq                            # return total length in rax
.Lfunc_end1:
	.size	wal_frame_record, .Lfunc_end1-wal_frame_record

	.ident	"clang version 20.1.8"
	.section	".note.GNU-stack","",@progbits  # non-executable stack (security default)
	.addrsig
# =============================================================================
# WHAT TO TAKE AWAY
#   * A CRC is polynomial long division: "shift out a bit; conditionally xor the
#     polynomial." The optimizer expresses the condition with negate-to-mask
#     (neg + and), so there is no branch in the 8-round inner loop — branchless
#     code the CPU pipelines well.
#   * put_u32 is written as explicit little-endian byte stores for PORTABILITY.
#     On this little-endian target the optimizer may keep the bytes (misaligned
#     klen) or fuse them into one `movl` (vlen, reclen) — both are correct; the C
#     never assumed the host's byte order, which is the whole point.
#   * Framing is register-cheap: length + checksum + copies, no heap, no locals.
#     That is why every write can afford to be logged this way.
#   * Compare with demo.O0.s (each C statement its own spill-heavy block) and
#     demo.O2.s (the byte-copy loops get vectorized) to see the same record
#     builder at three optimization levels.
# =============================================================================
