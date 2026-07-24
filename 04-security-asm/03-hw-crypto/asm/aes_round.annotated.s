# =============================================================================
# aes_round.annotated.s — clang -O1 output for asm/aes_round.c, annotated, with
#                         the genuine machine encodings of the AES-NI round.
# =============================================================================
#
# This is the file the project brief asks for: "annotate the aesenc round in
# asm." It is the exact assembly clang emits for the one-block AES-128 encrypt in
# aes_round.c (see aes_round.s for the untouched original), plus the real opcode
# bytes taken from `objdump -d` of the assembled object, so you can see how an
# AES round is encoded, not just named.
#
# SysV AMD64 ABI for aes128_encrypt_block(const u8 *in, u8 *out, const u8 *rk):
#     in  = rdi   (pointer to 16 plaintext bytes)
#     out = rsi   (pointer to 16 output bytes)
#     rk  = rdx   (pointer to the 11 expanded round keys, 176 bytes)
#   XMM0 is the single working register; the whole cipher lives in it. XMM regs
#   are caller-saved in SysV, so we may clobber XMM0 freely with no save/restore.
#
# THE ONE BIG IDEA
# ----------------
# A software AES round is dozens of instructions and a secret-indexed table
# lookup. AES-NI does the ENTIRE round — ShiftRows, SubBytes (the S-box, in
# hardware), MixColumns, and AddRoundKey — in a SINGLE `aesenc` instruction,
# with fixed latency and NO data-dependent memory access. That is why AES-NI is
# both fast AND immune to the cache-timing attacks that plague table-driven
# software AES (see aes_ct.c for the constant-time software alternative).
#
# HOW THE OPERATION DECODES (Intel SDM):
#   aesenc     xmm1, xmm2/m128 :  state <- MixColumns(SubBytes(ShiftRows(xmm1)))
#                                  then  state <- state XOR (xmm2/m128)   [round key]
#   aesenclast xmm1, xmm2/m128 :  same, but WITHOUT MixColumns (the final round)
# Here the round-key operand is a MEMORY operand N(%rdx): the CPU loads the round
# key straight from the schedule and XORs it as part of the instruction.
# =============================================================================

	.file	"aes_round.c"
	.text
	.globl	aes128_encrypt_block
	.p2align	4                       # align the entry to 16 bytes
	.type	aes128_encrypt_block,@function
aes128_encrypt_block:

# ---- PROLOGUE ---------------------------------------------------------------
	pushq	%rbp                    # save caller's frame pointer (kept at -O1)
	movq	%rsp, %rbp              # establish our frame

# ---- load plaintext and do the initial AddRoundKey (round 0 whitening) ------
	movdqu	(%rdi), %xmm0           # xmm0 = in[0..15].  encoding: f3 0f 6f 07
	                                #   movdqu = UNALIGNED 128-bit load (the F3
	                                #   prefix picks the unaligned form); we must
	                                #   not assume the caller's buffer is aligned.
	pxor	(%rdx), %xmm0           # xmm0 ^= rk[0].  encoding: 66 0f ef 02
	                                #   the pre-round key XOR (AddRoundKey #0).
	                                #   The 66 prefix makes this the 128-bit SSE2
	                                #   form; the memory operand is [rdx+0].

# ---- the nine FULL rounds ---------------------------------------------------
# Each `aesenc N(%rdx), %xmm0` is one complete AES round against round key N.
# Opcode is 66 0f 38 dc /r (aesenc). The last byte(s) are the ModR/M + disp that
# select the round-key memory operand; note how the displacement encoding grows:
	aesenc	16(%rdx), %xmm0         # round 1.  66 0f 38 dc 42 10
	                                #   ModR/M 42 = mod=01,reg=000(xmm0),r/m=010(rdx)
	                                #   -> [rdx+disp8]; disp8 = 0x10.
	aesenc	32(%rdx), %xmm0         # round 2.  66 0f 38 dc 42 20   (disp8 0x20)
	aesenc	48(%rdx), %xmm0         # round 3.  66 0f 38 dc 42 30   (disp8 0x30)
	aesenc	64(%rdx), %xmm0         # round 4.  66 0f 38 dc 42 40   (disp8 0x40)
	aesenc	80(%rdx), %xmm0         # round 5.  66 0f 38 dc 42 50   (disp8 0x50)
	aesenc	96(%rdx), %xmm0         # round 6.  66 0f 38 dc 42 60   (disp8 0x60)
	aesenc	112(%rdx), %xmm0        # round 7.  66 0f 38 dc 42 70   (disp8 0x70)
	aesenc	128(%rdx), %xmm0        # round 8.  66 0f 38 dc 82 80 00 00 00
	                                #   0x80 = 128 is OUTSIDE the signed disp8
	                                #   range (-128..127), so the encoder switches
	                                #   to ModR/M 82 = mod=10 -> [rdx+disp32], and
	                                #   the displacement is now four bytes.
	aesenc	144(%rdx), %xmm0        # round 9.  66 0f 38 dc 82 90 00 00 00 (disp32)

# ---- the final round: aesenclast (SubBytes+ShiftRows+AddRoundKey, NO Mix) ---
	aesenclast	160(%rdx), %xmm0 # round 10.  66 0f 38 dd 82 a0 00 00 00
	                                #   opcode byte dd (vs dc) = aesenclast. AES
	                                #   omits MixColumns on the last round so that
	                                #   encryption and decryption stay symmetric;
	                                #   this instruction encodes exactly that.

# ---- store ciphertext -------------------------------------------------------
	movdqu	%xmm0, (%rsi)           # out[0..15] = xmm0.  encoding: f3 0f 7f 06
	                                #   unaligned 128-bit store to the out buffer.

# ---- EPILOGUE ---------------------------------------------------------------
	popq	%rbp
	retq                            # encoding: c3
.Lfunc_end0:
	.size	aes128_encrypt_block, .Lfunc_end0-aes128_encrypt_block

	.ident	"clang version 20.1.8"
	.section	".note.GNU-stack","",@progbits
# =============================================================================
# WHAT TO TAKE AWAY
#   * One `aesenc` == one entire AES round (ShiftRows+SubBytes+MixColumns+XOR key)
#     in hardware, ~4-cycle latency, with NO memory lookup indexed by secrets.
#   * The whole AES-128 block is: pxor key0; aesenc x9; aesenclast; two movdqu.
#     Twelve instructions, all in xmm0. AES-256 is the same shape with 13 aesenc
#     before aesenclast (see aes_ni.c).
#   * The S-box that a software AES stores in a 256-byte table (and leaks through
#     the cache) is baked into the silicon here — that is the side-channel win.
#   * Encoding detail worth internalizing: `aesenc`/`aesenclast` use the 3-byte
#     opcode map 66 0F 38 DC/DD, and the round-key operand's displacement is a
#     disp8 for offsets that fit in a signed byte, a disp32 once they don't.
# =============================================================================
