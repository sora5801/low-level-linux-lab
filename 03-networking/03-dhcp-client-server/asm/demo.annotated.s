# =============================================================================
# demo.annotated.s — clang -O1 output for asm/demo.c, explained instruction by
#                    instruction. Two routines: the DHCP option TLV walker and
#                    the UDP pseudo-header checksum.
# =============================================================================
#
# HOW TO READ THIS FILE
# ---------------------
# This is the EXACT assembly clang 20 emits for asm/demo.c at -O1 (see demo.s
# for the untouched original), with a comment on essentially every instruction.
# AT&T syntax throughout:
#
#     op   src, dst                  # movl $1, %eax   =>  eax = 1
#     %reg                           # a register
#     $imm                           # an immediate constant
#     N(%base,%index,scale)          # memory at [base + index*scale + N]
#     movzbl / movzwl                # zero-extend byte/word into a 32-bit reg
#
# Register widths are the SAME register: rax(64)/eax(32)/ax(16)/al(8). Writing a
# 32-bit reg (eax) ZERO-EXTENDS into the 64-bit reg (rax) for free — that is why
# the compiler keeps using 32-bit ops on values it knows are small.
#
# SYSTEM V AMD64 ABI (what every function here obeys)
# ---------------------------------------------------
#   * integer/pointer arguments, in order:  rdi, rsi, rdx, rcx, r8, r9
#   * return value:                          rax  (ax/al for narrow types)
#   * callee-saved (a function must preserve): rbx, rbp, r12, r13, r14, r15, rsp
#   * caller-saved (scratch): rax, rcx, rdx, rsi, rdi, r8, r9, r10, r11
#   * the "red zone": 128 bytes below rsp a leaf may use without moving rsp
#   * stack must be 16-byte aligned at the point of a `call`
#
# Both functions are LEAF functions (they call nothing), so the only prologue
# cost is saving %rbp for a legible frame — kept because we compiled with
# -fno-omit-frame-pointer. dhcp_opt_find additionally saves %rbx because it uses
# it as a scratch temporary and rbx is callee-saved: it MUST be restored.
# =============================================================================

	.file	"demo.c"
	.text

# =============================================================================
# const u8 *dhcp_opt_find(const u8 *opts, usize opts_len, u8 code, u8 *out_len)
#
#   arg0 opts     -> rdi     (base pointer of the options area)
#   arg1 opts_len -> rsi     (number of bytes available)
#   arg2 code     -> edx/dl  (the option type we are hunting)
#   arg3 out_len  -> rcx     (where to store the matched length; may be NULL)
#   return        -> rax     (&value on match, or 0/NULL)
#
# THE BIG IDEA. There is no single branch back to a loop top here. At -O1 clang
# turned the loop body's four possible outcomes into a small STATE MACHINE in
# %r9d, then a shared dispatch tail acts on the state:
#
#     r9d == 3  -> break out of the loop        (END byte, or a bounds check failed)
#     r9d == 2  -> `continue` (re-test i<len)    (we consumed a PAD byte)
#     r9d == 1  -> return &opts[i+2]             (this option matched `code`)
#     r9d == 0  -> advance i and loop again      (non-matching TLV, skip it)
#
# So as you read, treat r9d as "what to do next". i lives in %r8, t in %r10, l
# in %r11/%ebx. This is a faithful, if roundabout, encoding of the C `while`.
# =============================================================================
	.globl	dhcp_opt_find
	.p2align	4
	.type	dhcp_opt_find,@function
dhcp_opt_find:
# %bb.0: ---- entry: the `while (i < opts_len)` guard for the very first pass ---
	testq	%rsi, %rsi              # opts_len == 0 ?  (test sets ZF if rsi==0)
	je	.LBB0_1                 # empty buffer -> skip everything, return NULL

# ---- PROLOGUE (only reached when there is at least one byte to scan) ---------
	pushq	%rbp                    # save caller's frame pointer
	movq	%rsp, %rbp              # establish our frame
	pushq	%rbx                    # save callee-saved rbx (we use it as scratch)
                                        # implicit-def: $rax = the return value reg,
                                        #   left undefined until a path sets it.
	xorl	%r8d, %r8d              # i = 0  (r8). xor is the 2-byte zero idiom.
	jmp	.LBB0_4                 # jump into the loop body (guard already passed)
	.p2align	4

# ---- dispatch fragment A: reached with a freshly-computed state in r9d -------
.LBB0_15:                               # "state <= 1" landed here (state 0 or 1)
	testl	%r9d, %r9d              # state == 0 ?
	jne	.LBB0_19                # state == 1 -> RETURN (rax already = &value)
.LBB0_16:                               # state 0 (advance) or 2 (continue): re-test
	cmpq	%rsi, %r8              # compare i (r8) with opts_len (rsi)
	jae	.LBB0_18                # i >= opts_len -> loop done, return NULL

# ---- LOOP HEADER: t = opts[i] -----------------------------------------------
.LBB0_4:                                # top of `while` body; i in r8
	movzbl	(%rdi,%r8), %r10d       # t = opts[i]  (byte load, zero-extended)
	movl	$3, %r9d               # pre-load state = 3 ("break") as the default
	cmpl	$255, %r10d             # t == 255 (DHCP_OPT_END) ?
	je	.LBB0_14                # END -> go dispatch with state 3 (break)

# %bb.5: ---- t != END; is it PAD? --------------------------------------------
	testl	%r10d, %r10d            # t == 0 (DHCP_OPT_PAD) ?
	jne	.LBB0_7                 # non-zero -> it is a real TLV, handle below

# %bb.6: ---- PAD branch: `i++; continue;` ------------------------------------
	incq	%r8                     # i++  (skip the single PAD byte)
	movl	$2, %r9d               # state = 2 ("continue")
	cmpl	$1, %r9d               # (shared-tail artifact: 2 <= 1 ? no)
	jle	.LBB0_15                #   not taken
	jmp	.LBB0_17                # go to the state-2/3 half of the dispatch
	.p2align	4

# ---- TLV branch: t is a normal option; validate its length ------------------
.LBB0_7:
	leaq	1(%r8), %r11            # r11 = i + 1  (index of the length byte)
	cmpq	%rsi, %r11              # (i+1) >= opts_len ?  [guard (1)]
	jae	.LBB0_14                # truncated (type with no length) -> break

# %bb.8: ---- read length l and bounds-check the value span -------------------
	movzbl	1(%rdi,%r8), %r11d      # l = opts[i+1]  (the length byte)
	movzbl	%r11b, %ebx             # ebx = l  (zero-extended to 64 via rbx)
	addq	%r8, %rbx              # rbx = i + l
	addq	$2, %rbx               # rbx = i + 2 + l  (one past this option's value)
	cmpq	%rsi, %rbx              # (i + 2 + l) > opts_len ?  [guard (2)]
	ja	.LBB0_14                # value would over-read -> break

# %bb.9: ---- does this option's type match the one we want? ------------------
	cmpb	%dl, %r10b              # compare code (dl) with t (r10b)
	jne	.LBB0_13                # mismatch -> advance to the next option

# %bb.10: ---- MATCH. Optionally store the length, then set the return value --
	testq	%rcx, %rcx              # out_len == NULL ?
	je	.LBB0_12                # NULL -> skip the store
# %bb.11:
	movb	%r11b, (%rcx)           # *out_len = l
.LBB0_12:
	leaq	2(%r8), %rax            # rax = i + 2
	addq	%rdi, %rax              # rax = opts + i + 2 = &opts[i+2]  (the value ptr)
	movl	$1, %r9d               # state = 1 ("return this pointer")
	.p2align	4

# ---- dispatch fragment B: act on the state in r9d ---------------------------
.LBB0_14:                               # entered from END / bounds-fail (state 3)
	cmpl	$1, %r9d               # state <= 1 ?
	jle	.LBB0_15                # state 0 or 1 -> fragment A (return or advance)
.LBB0_17:
	cmpl	$2, %r9d               # state == 2 ?
	je	.LBB0_16                # continue (PAD) -> re-test the loop condition
	jmp	.LBB0_18                # else state == 3 -> break, return NULL

# ---- advance branch: `i += 2 + l; ` then loop -------------------------------
.LBB0_13:
	xorl	%r9d, %r9d              # state = 0 ("advance")
	movq	%rbx, %r8              # i = i + 2 + l  (rbx computed above)
	cmpl	$1, %r9d               # (0 <= 1 ? yes — shared-tail artifact)
	jle	.LBB0_15                # -> fragment A, which will re-test the guard
	jmp	.LBB0_17                #   (never reached: the jle above is always taken)

# ---- return NULL ------------------------------------------------------------
.LBB0_18:
	xorl	%eax, %eax              # rax = 0 (NULL): option not found / malformed
.LBB0_19:                               # common return (rax already set on match)
	popq	%rbx                    # restore callee-saved rbx
	popq	%rbp                    # restore caller's frame pointer
	retq

# ---- the opts_len == 0 fast path (no frame was set up) ----------------------
.LBB0_1:
	xorl	%eax, %eax              # rax = NULL
	retq                            # return without touching rbx/rbp (never pushed)
.Lfunc_end0:
	.size	dhcp_opt_find, .Lfunc_end0-dhcp_opt_find

# =============================================================================
# u16 udp_checksum(u32 saddr, u32 daddr, const u8 *udp,
#                  const u8 *payload, usize payload_len)
#
#   arg0 saddr       -> edi   (source IPv4, network order, held as a value)
#   arg1 daddr       -> esi   (dest IPv4, network order)
#   arg2 udp         -> rdx   (pointer to the 8-byte UDP header)
#   arg3 payload     -> rcx   (pointer to the DHCP bytes)
#   arg4 payload_len -> r8    (count of payload bytes)
#   return           -> ax    (network-order checksum; 0 mapped to 0xFFFF)
#
# THE BIG IDEA. The source adds a dozen 16-bit big-endian words into a 32-bit
# accumulator, then folds and complements. -O1 does two nice tricks:
#   * REASSOCIATION: because integer addition is associative, clang reorders the
#     address/header terms freely and interleaves their byte-swaps.
#   * STRENGTH REDUCTION: the term ((udp[4]<<8)|udp[5]) (the UDP length) appears
#     TWICE in the source — once for the pseudo-header, once for the real UDP
#     header — so clang computes it once and multiplies by 2 via `udp[4]<<9` +
#     `udp[5]*2`, folded into a single `lea`.
# A 16-bit big-endian load is realized as a normal little-endian `movzwl` load
# followed by `rolw $8` (rotate the two bytes), i.e. an in-register byte swap.
# =============================================================================
	.globl	udp_checksum
	.p2align	4
	.type	udp_checksum,@function
udp_checksum:
# %bb.0: ---- PROLOGUE (leaf; frame pointer kept for readability) -------------
	pushq	%rbp                    # save caller's frame pointer
	movq	%rsp, %rbp              # establish frame

# ---- pseudo-header address words, computed out of order and reassociated -----
	movl	%edi, %eax              # eax = saddr
	shrl	$24, %eax              # eax = saddr >> 24 = sa[3]  (top wire byte)
	movl	%esi, %r9d              # r9d = daddr
	shrl	$24, %r9d              # r9d = daddr >> 24 = da[3]
	addl	%eax, %r9d              # r9d = sa[3] + da[3]  (two low-order byte terms)
	movl	%edi, %eax              # eax = saddr (again)
	rolw	$8, %ax                # swap low two bytes: ax = (sa[0]<<8)|sa[1]
	movzwl	%ax, %eax              # eax = (sa[0]<<8)|sa[1]   (src, first half-word)
	shrl	$8, %edi               # edi = saddr >> 8
	andl	$65280, %edi            # edi = (saddr>>8)&0xFF00 = sa[2]<<8  (imm 0xFF00)
	movl	%esi, %r10d             # r10d = daddr
	rolw	$8, %r10w              # swap: r10w = (da[0]<<8)|da[1]
	addl	%r9d, %edi              # edi = (sa[2]<<8) + sa[3] + da[3]
	movzwl	%r10w, %r9d             # r9d = (da[0]<<8)|da[1]   (dst, first half-word)
	addl	%eax, %r9d              # r9d = ((da[0]<<8)|da[1]) + ((sa[0]<<8)|sa[1])
	shrl	$8, %esi               # esi = daddr >> 8
	andl	$65280, %esi            # esi = (daddr>>8)&0xFF00 = da[2]<<8
	addl	%edi, %esi              # esi += (sa[2]<<8)+sa[3]+da[3]
	addl	%r9d, %esi              # esi += the two first-half-words
                                        # -> esi now = sum of ALL FOUR address
                                        #    half-words (the pseudo-header addrs).

# ---- load the 8 UDP-header bytes as three big-endian words + the length ------
	movzbl	4(%rdx), %eax           # eax = udp[4]  (length high byte)
	movzbl	5(%rdx), %edi           # edi = udp[5]  (length low byte)
	movzwl	(%rdx), %r9d            # r9d = little-endian load of udp[0..1]
	rolw	$8, %r9w               # swap -> (udp[0]<<8)|udp[1]  = source port
	movzwl	2(%rdx), %r10d          # r10d = load udp[2..3]
	movzwl	%r9w, %r9d              # zero-extend the swapped source-port word
	rolw	$8, %r10w              # swap -> (udp[2]<<8)|udp[3]  = dest port
	movzwl	%r10w, %r10d            # zero-extend
	movzwl	6(%rdx), %edx           # edx = load udp[6..7]
	rolw	$8, %dx                # swap -> (udp[6]<<8)|udp[7]  = checksum field (0)
	movzwl	%dx, %edx              # zero-extend

# ---- the length term, counted twice, via strength reduction -----------------
	shll	$9, %eax               # eax = udp[4] << 9  = 2*(udp[4]<<8)
	leal	(%rax,%rdi,2), %eax     # eax = udp[4]<<9 + udp[5]*2
                                        #     = 2 * ((udp[4]<<8)|udp[5])
                                        #  (UDP length appears in BOTH the pseudo-
                                        #   header and the UDP header, hence x2)

# ---- accumulate everything that is not the payload --------------------------
	addl	%esi, %eax              # += the four address half-words
	addl	%r9d, %eax              # += source port
	addl	%r10d, %eax             # += dest port
	addl	%edx, %eax              # += checksum field (0, but summed for symmetry)
	addl	$17, %eax              # += (zero byte)+protocol 17 (IPPROTO_UDP)
                                        # eax = full sum of pseudo-hdr + UDP header.

# ---- payload loop: add 16-bit big-endian words while >= 2 bytes remain -------
	cmpq	$2, %r8                # payload_len < 2 ?
	jb	.LBB1_2                 # too short for even one word -> skip loop
	.p2align	4
.LBB1_1:                                # do { ... } while (n > 1)
	movzwl	(%rcx), %edx            # load payload[0..1] (little-endian)
	rolw	$8, %dx                # swap -> (p[0]<<8)|p[1]  (network-order word)
	movzwl	%dx, %edx              # zero-extend
	addl	%edx, %eax              # sum += word
	addq	$2, %rcx               # p += 2
	addq	$-2, %r8               # n -= 2
	cmpq	$1, %r8                # n > 1 ?
	ja	.LBB1_1                 # keep going while at least 2 bytes remain

# ---- odd trailing byte: pad with an implicit zero low byte ------------------
.LBB1_2:
	testq	%r8, %r8               # n == 0 ?
	je	.LBB1_4                 # even length -> nothing left to add
# %bb.3:
	movzbl	(%rcx), %ecx            # ecx = last byte p[0]
	shll	$8, %ecx               # ecx = p[0] << 8  (it is the HIGH byte of a word)
	addl	%ecx, %eax              # sum += p[0]<<8

# ---- fold the carries out of the top 16 bits, twice -------------------------
.LBB1_4:
	movzwl	%ax, %ecx              # ecx = sum & 0xFFFF   (low half)
	shrl	$16, %eax              # eax = sum >> 16       (carry half)
	addl	%ecx, %eax              # eax = low + carry  (first fold; may carry once)
	movl	%eax, %ecx              # copy the folded value
	shrl	$16, %ecx              # ecx = (that) >> 16
	addl	%eax, %ecx              # ecx = value + its own carry (second fold)

# ---- ones-complement, with the RFC 768 "0 -> 0xFFFF" special case ------------
	cmpw	$-1, %cx               # is the folded low-16 == 0xFFFF ?  (=> ~sum == 0)
	notl	%ecx                    # ecx = ~sum  (NOT does NOT disturb EFLAGS, so the
                                        #   cmpw result above is still live below)
	movl	$65535, %eax            # eax = 0xFFFF  (the value to use when ~sum == 0)
	cmovnel	%ecx, %eax             # if cx != 0xFFFF: eax = ~sum;  else keep 0xFFFF.
                                        #   Branchless encoding of `c==0 ? 0xFFFF : c`.
                                        # kill: def $ax — only ax (16 bits) is returned.
	popq	%rbp                    # restore caller's frame pointer
	retq                            # return the checksum in ax
.Lfunc_end1:
	.size	udp_checksum, .Lfunc_end1-udp_checksum

# =============================================================================
# WHAT TO TAKE AWAY
#   * A `while` with several early-exits can compile to a STATE MACHINE (r9d
#     here) plus a shared dispatch tail rather than one clean back-edge. Reading
#     the asm is how you learn to recognize that pattern.
#   * Every one of the walker's bounds checks (guards 1 and 2) is a real compare
#     against opts_len — the compiler did not optimize them away, because they
#     guard memory safety on attacker-controlled length bytes.
#   * A network-order 16-bit load is `movzwl` + `rolw $8`: a little-endian load
#     plus an in-register byte swap. You will see this everywhere in packet code.
#   * The optimizer reassociates the checksum's additions and strength-reduces
#     the doubly-counted UDP length into one `lea`. The math is identical; the
#     instruction count is not.
#   * `not` leaves the flags alone, which lets clang test the "== 0xFFFF" case
#     BEFORE complementing and still use that flag in a following `cmovne`.
# Compare this with demo.O0.s (every value spilled to the stack, one C statement
# at a time) and demo.O2.s (the same tricks, plus tighter scheduling).
# =============================================================================
	.ident	"clang version 20.1.8"
	.section	".note.GNU-stack","",@progbits
