# =============================================================================
# demo.annotated.s — the bad-char scanner (demo.c) at -O1, explained.
# =============================================================================
#
# This is the DEFENSIVE core of the shellcode project: contains_badchar() walks
# a byte buffer and returns the offset of the first "bad" byte (per a 256-bit
# set), or -1 if clean. It is exactly the primitive an IDS/memory scanner runs,
# and the same check a channel analysis uses to decide whether a buffer is
# NUL-free. Below is the real clang -O1 output (see demo.s) with a comment on
# essentially every instruction.
#
# AT&T syntax: `op src, dst`.  Registers: rdi/rsi/rdx = args 1/2/3 (SysV AMD64),
# rax = return value.  rcx, r8, r9 are caller-saved scratch (free to clobber).
#
# C being compiled:
#     long contains_badchar(const u8 *buf, usize len, const badset *set) {
#         for (usize i = 0; i < len; i++)
#             if (badset_has(set, buf[i])) return (long)i;
#         return -1;
#     }
#   where badset_has(s,c) == (s->bits[c>>3] >> (c&7)) & 1.
# =============================================================================

	.text
	.globl	contains_badchar
	.p2align	4
	.type	contains_badchar,@function
contains_badchar:                       # long contains_badchar(rdi=buf, rsi=len, rdx=set)
# ---- PROLOGUE ---------------------------------------------------------------
	pushq	%rbp                    # save caller's frame pointer
	movq	%rsp, %rbp              # establish our frame (kept at -O1 for debuggability)

# ---- set up the return-if-empty case ----------------------------------------
	movq	$-1, %rax               # rax = -1  (the "clean / not found" result,
                                        #   pre-loaded so the empty-buffer path just returns it)
	testq	%rsi, %rsi              # len == 0 ?  (test sets ZF if rsi & rsi == 0)
	je	.LBB0_5                 # if len == 0, skip the loop and return -1

# ---- loop init --------------------------------------------------------------
	xorl	%ecx, %ecx              # i = 0  (rcx is the loop index; xor-zero is 2 bytes, no NUL)
	.p2align	4               # align the loop head for the fetch unit
.LBB0_2:                                # for (;;) {   top of loop, i in rcx
# ---- load buf[i] ------------------------------------------------------------
	movzbl	(%rdi,%rcx), %r8d       # r8 = (u8)buf[i]  — zero-extend one byte; c is now in r8
# ---- badset_has: pick the byte of the bitset:  s->bits[c >> 3] --------------
	movl	%r8d, %r9d              # r9 = c
	shrl	$3, %r9d                # r9 = c >> 3   (which byte of the 32-byte bitset)
	movzbl	(%rdx,%r9), %r9d        # r9 = set->bits[c >> 3]  (load that bitset byte)
# ---- badset_has: pick the bit within it:  (byte >> (c & 7)) & 1 -------------
	andl	$7, %r8d                # r8 = c & 7    (which bit within the byte, 0..7)
	btl	%r8d, %r9d              # BIT TEST: copy bit number r8 of r9 into CF.
                                        #   This is the whole membership test in ONE instruction:
                                        #   CF = 1  <=>  byte value c is in the bad set.
	jb	.LBB0_3                 # jb = "jump if CF" : if the byte is bad, go report it
# ---- loop step: i++ ; i != len ? --------------------------------------------
# %bb.4:
	incq	%rcx                    # i++
	cmpq	%rcx, %rsi              # compare len (rsi) with i (rcx)
	jne	.LBB0_2                 # if i != len, loop again
                                        #   (fall through when i == len: buffer was clean)
# ---- clean: return -1 (rax already holds -1) --------------------------------
.LBB0_5:
	popq	%rbp                    # restore caller's frame pointer
	retq                            # return rax (== -1)

# ---- found: return the offset i ---------------------------------------------
.LBB0_3:
	movq	%rcx, %rax              # rax = i  (the offset of the first bad byte)
	popq	%rbp                    # restore frame pointer
	retq                            # return rax (== i)
.Lfunc_end0:
	.size	contains_badchar, .Lfunc_end0-contains_badchar

# =============================================================================
# WHAT TO TAKE AWAY
#   * The 256-bit bad-char set turns "is this byte bad?" into shr + load + and +
#     btl — a constant, branch-light test, which is why it is fast enough to run
#     over every byte of large buffers (as a real scanner must).
#   * `btl %r8d, %r9d` is the star instruction: hardware bit-addressing. Watch
#     for it whenever C does `(word >> n) & 1`.
#   * The function returns the OFFSET, not just a bool — the loop keeps i in rcx
#     precisely so the "found" path (LBB0_3) can hand it back in rax.
#   * is_nul_free() (further down in demo.s) is the special case set = {0x00};
#     at -O2 clang recognizes it as memchr and may call the vectorized libc one.
#
# This is the defensive/analysis primitive the shellcode project ships on
# purpose: the same scan an IDS runs, and the same NUL-free check that explains
# *why* classic injected payloads had to avoid the 0x00 byte in the first place.
# =============================================================================
