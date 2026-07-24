/* ===========================================================================
 * simd_asm.S — the four primitives, hand-written. THE STAR ARTIFACT.
 * ===========================================================================
 *
 * This is GNU assembler (GAS), AT&T syntax, System V AMD64 ABI — the same ABI
 * every project in this lab assumes. It is written to be READ: essentially
 * every instruction carries a comment explaining the *why*. If you only read
 * one file in this project, read this one.
 *
 * AT&T SYNTAX REMINDER
 * --------------------
 *     op   src, dst              # operands are source-then-destination
 *     %reg    register           $imm  immediate      (%reg) memory at [reg]
 *     N(%r)   memory at [r+N]     op b/w/l/q suffix = 8/16/32/64-bit operand
 *
 * SYSTEM V AMD64 CALLING CONVENTION (what the C caller guarantees us)
 * ------------------------------------------------------------------
 *   integer/pointer args, in order:  rdi, rsi, rdx, rcx, r8, r9
 *   integer return value:            rax
 *   caller-saved (we may trash):     rax, rcx, rdx, rsi, rdi, r8-r11, all xmm/ymm
 *   callee-saved (we must preserve): rbx, rbp, r12-r15, rsp
 *   the "red zone":                  128 bytes below rsp usable by leaf funcs
 *   DIRECTION FLAG (DF):             the ABI guarantees DF=0 on entry, so string
 *                                    ops (rep movsb) run FORWARD without a `cld`
 *
 * Every routine here is a LEAF (makes no calls), touches only caller-saved
 * registers, and needs no stack frame — so there is no push/pop, no prologue,
 * and rsp is untouched. That is why the code is pure computation start to `ret`.
 *
 * THE CENTRAL SIMD IDIOM (strlen + memchr)
 * ----------------------------------------
 *     (v)pcmpeqb   compare 16/32 bytes at once: matching lane -> 0xFF, else 0x00
 *     (v)pmovmskb  pack the HIGH BIT of each lane into a scalar integer mask
 *     tzcnt / bsf  index of the first set bit = index of the first match
 * One vector compare + one movemask + one bit-scan locates a byte among 16 or
 * 32 in a handful of cycles. That is the whole magic behind fast strlen/memchr.
 * ===========================================================================
 */

	.text

/* ===========================================================================
 * size_t strlen_avx2_asm(const char *s)      [rdi = s]  ->  rax = length
 * ===========================================================================
 * AVX2, 32 bytes per compare. The one hazard for strlen is that we are given no
 * length, so a 32-byte load must never wander into an unmapped page past the
 * terminator. We defuse it by ALIGNING THE POINTER DOWN to 32 and ignoring the
 * bytes before the string: a 32-byte-aligned load cannot cross a 4096-byte page
 * boundary (4096 is a multiple of 32), so it only ever touches mapped memory
 * that the string itself lives in.
 */
	.globl	strlen_avx2_asm
	.p2align	4                       # 16-byte-align the entry for the fetcher
	.type	strlen_avx2_asm,@function
strlen_avx2_asm:
	vpxor	%ymm0, %ymm0, %ymm0             # ymm0 = 0 : the NUL byte we search for
	mov	%edi, %ecx                      # ecx = low bits of the pointer...
	and	$31, %ecx                       # ecx = misalign = s & 31   (0..31)
	mov	%rdi, %rax                      # rax = s
	and	$-32, %rax                      # rax = aligned = s & ~31 (round DOWN)

	# ---- first, possibly partial, aligned block ------------------------------
	vpcmpeqb (%rax), %ymm0, %ymm1           # ymm1[i] = 0xFF where aligned[i]==0
	vpmovmskb %ymm1, %edx                   # edx = 32-bit mask of zero positions
	shr	%cl, %edx                       # discard the `misalign` bytes that sit
                                                #   BEFORE s inside this aligned block
	# NB: SHR by a count of 0 leaves EFLAGS unchanged, so we must not branch on
	# SHR's flags — set them explicitly with TEST, which is always well-defined.
	test	%edx, %edx                      # any zero byte at/after s in block 0?
	jnz	.Lstrlen_first                  #   yes -> the first NUL is right here

	# ---- steady state: full 32-byte aligned loads ---------------------------
.Lstrlen_loop:
	add	$32, %rax                       # advance to the next aligned 32B block
	vpcmpeqb (%rax), %ymm0, %ymm1           # compare 32 bytes against zero
	vpmovmskb %ymm1, %edx                   # gather their zero-mask
	test	%edx, %edx                      # NUL anywhere in this block?
	jz	.Lstrlen_loop                   #   no -> keep scanning

	sub	%rdi, %rax                      # rax = byte offset from s to this block
	tzcnt	%edx, %edx                      # edx = index of first zero within block
                                                #   (tzcnt is BMI1; every AVX2 CPU has it)
	add	%rdx, %rax                      # rax = offset-to-block + in-block index
	vzeroupper                              # drop YMM upper halves: avoids the
                                                #   AVX->legacy-SSE transition stall for
                                                #   whatever SSE code runs after us
	ret                                     # return length in rax

.Lstrlen_first:
	tzcnt	%edx, %eax                      # length = index of the first zero,
                                                #   measured from s (mask already shifted)
	vzeroupper                              # same transition-stall hygiene
	ret
.Lstrlen_end:
	.size	strlen_avx2_asm, .Lstrlen_end-strlen_avx2_asm


/* ===========================================================================
 * void *memchr_sse2_asm(const void *s, int c, size_t n)
 *                                    [rdi=s, esi=c, rdx=n]  ->  rax = ptr|NULL
 * ===========================================================================
 * SSE2, 16 bytes per compare. Here the length n IS known, so there is no page
 * dance: we vectorize the full 16-byte chunks and finish the <16 tail one byte
 * at a time, which by construction never reads past s+n.
 */
	.globl	memchr_sse2_asm
	.p2align	4
	.type	memchr_sse2_asm,@function
memchr_sse2_asm:
	test	%rdx, %rdx                      # n == 0 ?
	je	.Lmemchr_null                   #   -> nothing to search, return NULL

	# ---- broadcast the search byte c into all 16 lanes of xmm0 (SSE2 only) --
	movzbl	%sil, %esi                      # esi = (unsigned char)c  (memchr uses
                                                #   only the low byte; zero-extend it)
	movd	%esi, %xmm0                     # xmm0 byte0 = c, bytes 1..15 = 0
	punpcklbw %xmm0, %xmm0                  # interleave low bytes: bytes 0,1 = c,c
	punpcklwd %xmm0, %xmm0                  # interleave low words: bytes 0..3 = c
	pshufd	$0, %xmm0, %xmm0                # broadcast dword0 to all 4 -> 16x c

.Lmemchr_chunk:
	cmp	$16, %rdx                       # at least one full 16-byte block left?
	jb	.Lmemchr_tail                   #   no -> handle the remainder scalar-ly
	movdqu	(%rdi), %xmm1                   # load 16 bytes (unaligned: caller's ptr
                                                #   has no alignment guarantee)
	pcmpeqb	%xmm0, %xmm1                    # xmm1[i] = 0xFF where byte[i] == c
	pmovmskb %xmm1, %eax                    # eax = 16-bit mask of matching lanes
	test	%eax, %eax                      # any match in this block?
	jnz	.Lmemchr_hit                    #   yes -> pinpoint it
	add	$16, %rdi                       # advance data pointer by one block
	sub	$16, %rdx                       # and shrink the remaining count
	jmp	.Lmemchr_chunk

.Lmemchr_hit:
	bsf	%eax, %eax                      # index of first set bit = first match.
                                                #   (bsf is well-defined here since the
                                                #   mask is nonzero; tzcnt would need SSE4)
	add	%rdi, %rax                      # pointer = current block base + index
	ret

.Lmemchr_tail:
	test	%rdx, %rdx                      # 0 leftover bytes?
	je	.Lmemchr_null                   #   -> not found
.Lmemchr_tail_loop:
	movzbl	(%rdi), %eax                    # load one candidate byte
	cmp	%sil, %al                       # equal to (unsigned char)c ?
	je	.Lmemchr_found
	inc	%rdi                            # next byte
	dec	%rdx                            # one fewer to check
	jnz	.Lmemchr_tail_loop              # loop while bytes remain
.Lmemchr_null:
	xor	%eax, %eax                      # return NULL (0)
	ret
.Lmemchr_found:
	mov	%rdi, %rax                      # return the address of the match
	ret
.Lmemchr_end:
	.size	memchr_sse2_asm, .Lmemchr_end-memchr_sse2_asm


/* ===========================================================================
 * void *memcpy_erms_asm(void *dst, const void *src, size_t n)
 *                                  [rdi=dst, rsi=src, rdx=n]  ->  rax = dst
 * ===========================================================================
 * The ENTIRE fast memcpy on a modern Intel CPU, in three instructions. With
 * ERMS ("Enhanced REP MOVSB", CPUID.07H:EBX.ERMS[bit 9]), microcode turns
 * `rep movsb` into wide, cache-line-sized internal transfers that saturate
 * memory bandwidth — so glibc's memcpy literally jumps to `rep movsb` for large
 * copies. No SIMD register, no unrolling: the hardware does it.
 */
	.globl	memcpy_erms_asm
	.p2align	4
	.type	memcpy_erms_asm,@function
memcpy_erms_asm:
	mov	%rdi, %rax                      # memcpy returns dst; stash it now,
                                                #   because rep movsb will mutate rdi
	mov	%rdx, %rcx                      # rcx = n = the REP element count
	rep movsb                               # copy rcx bytes [rsi]->[rdi], both
                                                #   auto-increment; DF=0 per ABI so it
                                                #   runs forward. Clobbers rcx,rsi,rdi
                                                #   (all caller-saved) — rax survives.
	ret                                     # return dst (untouched in rax)
.Lmemcpy_erms_end:
	.size	memcpy_erms_asm, .Lmemcpy_erms_end-memcpy_erms_asm


/* ===========================================================================
 * void *memcpy_sse_asm(void *dst, const void *src, size_t n)
 *                                 [rdi=dst, rsi=src, rdx=n]  ->  rax = dst
 * ===========================================================================
 * The "before ERMS" way, and what you'd use on a CPU without it: copy 16 bytes
 * per iteration with unaligned SSE load/store, then a byte tail. Shown so you
 * can compare the explicit-vector approach against the microcoded rep movsb
 * above and understand *why* rep movsb wins on ERMS hardware.
 */
	.globl	memcpy_sse_asm
	.p2align	4
	.type	memcpy_sse_asm,@function
memcpy_sse_asm:
	mov	%rdi, %rax                      # save dst for the return value
.Lmemcpy_sse_16:
	cmp	$16, %rdx                       # at least 16 bytes to move?
	jb	.Lmemcpy_sse_tail
	movdqu	(%rsi), %xmm0                   # load 16 source bytes (unaligned)
	movdqu	%xmm0, (%rdi)                   # store 16 dest bytes  (unaligned)
	add	$16, %rsi                       # advance source
	add	$16, %rdi                       # advance dest
	sub	$16, %rdx                       # 16 fewer bytes remaining
	jmp	.Lmemcpy_sse_16
.Lmemcpy_sse_tail:
	test	%rdx, %rdx                      # any 0..15-byte remainder?
	jz	.Lmemcpy_sse_done
.Lmemcpy_sse_byte:
	movzbl	(%rsi), %ecx                    # load one source byte
	mov	%cl, (%rdi)                     # store it to dest
	inc	%rsi
	inc	%rdi
	dec	%rdx
	jnz	.Lmemcpy_sse_byte               # until the tail is drained
.Lmemcpy_sse_done:
	ret                                     # rax still holds dst
.Lmemcpy_sse_end:
	.size	memcpy_sse_asm, .Lmemcpy_sse_end-memcpy_sse_asm


/* ---------------------------------------------------------------------------
 * Mark the stack as NON-executable. This is a security default the linker
 * records in a PT_GNU_STACK program header: it tells the kernel this object
 * does not need an executable stack (W^X). Omitting it on even one linked .o
 * can force the WHOLE process to an executable stack — a gift to exploit
 * writers. Every hand-written .S in this lab ends with this line.
 * --------------------------------------------------------------------------- */
	.section	.note.GNU-stack,"",@progbits
