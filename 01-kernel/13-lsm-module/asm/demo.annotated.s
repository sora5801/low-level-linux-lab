# =============================================================================
# demo.annotated.s — clang's -O1 output for demo.c, explained instruction by
#                    instruction. This is the security-critical core of the
#                    PathGuard LSM: the code that turns a path string into an
#                    ALLOW/DENY verdict.
# =============================================================================
#
# HOW TO READ THIS FILE
# ---------------------
# This is the exact assembly clang emits for asm/demo.c at -O1 (see demo.s for
# the untouched original), with a comment on essentially every instruction.
# AT&T syntax throughout:
#
#     op   source, destination          # movl $1, %eax   =>  eax = 1
#     %reg                               # a register
#     $imm                               # an immediate (literal) constant
#     N(%base,%index,scale)              # memory at [base + index*scale + N]
#     (%rdi,%rcx)                        # memory at [rdi + rcx*1]  (scale 1)
#
# Register widths are the SAME register truncated: rax(64) / eax(32) / ax(16) /
# al(8). Writing a 32-bit reg (e.g. `movzbl`) ZERO-EXTENDS into the full 64-bit
# register, which is why byte loads become `movzbl ..., %edx` — the top bits of
# rdx are cleared for free.
#
# THE SysV AMD64 ABI CONTRACT (the rules both functions obey)
# -----------------------------------------------------------
#   * Integer/pointer ARGUMENTS, in order:   rdi, rsi, rdx, rcx, r8, r9
#   * RETURN value:                          rax  (eax for an `int`)
#   * CALLEE-SAVED (a function must preserve): rbx, rbp, r12, r13, r14, r15, rsp
#   * CALLER-SAVED (scratch, freely clobbered): rax, rcx, rdx, rsi, rdi,
#                                               r8, r9, r10, r11
#   Both functions here are LEAF functions (they call nothing), and at -O1 they
#   use only scratch registers, so they never have to save/restore a callee-
#   saved register. The `push %rbp / mov %rsp,%rbp` you see is just a frame
#   pointer kept for debuggability (-fno-omit-frame-pointer), not ABI-required.
#
# THE STRUCT LAYOUT THE ASM REVEALS
# ---------------------------------
#   struct pg_rule { const char *prefix;  int verdict; };
#   On x86-64 the pointer is 8 bytes at offset 0; `int verdict` is 4 bytes at
#   offset 8; then 4 bytes of TAIL PADDING so the next array element's pointer
#   is 8-byte aligned. Total sizeof == 16. You can literally read that off the
#   code below: indexing rules[i] is `i << 4` (multiply by 16) and reading
#   .verdict is `8(...)`. Memory layout is not a diagram in a book here — it is
#   an addressing mode.
# =============================================================================

	.file	"demo.c"
	.text

# =============================================================================
# int pg_path_has_prefix(const char *path, const char *prefix)
#
#   path   -> rdi     (arg0)
#   prefix -> rsi     (arg1)
#   return -> eax     (1 = path is at/under prefix, 0 = not)
#
# C structure being compiled:
#     i = 0;
#     while (prefix[i]) { if (path[i] != prefix[i]) return 0; i++; }
#     return (path[i] == '\0' || path[i] == '/');
#
# The optimizer keeps the loop index in rcx, keeps the "current prefix byte"
# live in dl across iterations (loading prefix[i+1] at the bottom of the loop),
# and pre-loads eax=0 so the mismatch path can `jne` straight to the return.
# =============================================================================
	.globl	pg_path_has_prefix
	.p2align	4
	.type	pg_path_has_prefix,@function
pg_path_has_prefix:
# %bb.0: — entry
	pushq	%rbp                    # save caller's frame pointer (debug frame)
	movq	%rsp, %rbp             # rbp = frame base for this call

	movzbl	(%rsi), %edx           # dl = prefix[0], zero-extended into edx.
                                        #   movzbl = "move zero-extend byte->long".
	testb	%dl, %dl               # set ZF if prefix[0] == 0 (empty prefix?)
	je	.LBB0_1                 # empty prefix -> skip loop, go match-at-i=0

# %bb.2: — loop setup (prefix is non-empty)
	xorl	%eax, %eax             # eax = 0. This PRE-SEEDS the return value so
                                        #   the mismatch exit (.LBB0_6) can just
                                        #   return whatever is in eax = 0 = "no".
	xorl	%ecx, %ecx             # rcx = 0 = i (the loop index)
	.p2align	4               # align the hot loop head to 16 bytes

.LBB0_3:                                # ---- LOOP TOP: dl already holds prefix[i]
	cmpb	%dl, (%rdi,%rcx)       # compare path[i] (mem [rdi+i]) against prefix[i]
	jne	.LBB0_6                 # differ -> return 0 (eax is already 0)
# %bb.4: — this character matched
	movzbl	1(%rsi,%rcx), %edx     # dl = prefix[i+1]  (peek the NEXT prefix byte)
	incq	%rcx                    # i++
	testb	%dl, %dl               # was prefix[i+1] the terminating NUL?
	jne	.LBB0_3                 # no -> keep matching
	jmp	.LBB0_5                 # yes -> whole prefix matched, do boundary test

.LBB0_1:                                # reached only when prefix was empty
	xorl	%ecx, %ecx             # i = 0 (nothing matched, boundary tested at 0)

.LBB0_5:                                # ---- BOUNDARY CHECK; rcx = i = matched length
	movzbl	(%rdi,%rcx), %eax      # al = path[i] = first byte PAST the prefix
	testb	%al, %al               # is it '\0' (path == the directory itself)?
	sete	%cl                     # cl = (path[i] == 0) ? 1 : 0
	cmpb	$47, %al               # 47 = '/'. is it a component separator?
	sete	%al                     # al = (path[i] == '/') ? 1 : 0
	orb	%cl, %al               # al = (path[i]==0) || (path[i]=='/')  <- the rule
	movzbl	%al, %eax              # zero-extend the 0/1 result to the full eax
                                        #   This is the KEY security line: a match
                                        #   counts only at a component boundary, so
                                        #   "/etc/secret" does NOT match "/etc/secretX".

.LBB0_6:                                # common return point
                                        # (mismatch fell in here with eax == 0)
	popq	%rbp                    # restore caller's frame pointer
	retq                            # return eax to the caller

.Lfunc_end0:
	.size	pg_path_has_prefix, .Lfunc_end0-pg_path_has_prefix

# =============================================================================
# int pg_policy_lookup(const char *path, const struct pg_rule *rules,
#                      int n, int default_verdict)
#
#   path            -> rdi   (arg0)
#   rules           -> rsi   (arg1)
#   n               -> edx   (arg2)
#   default_verdict -> ecx   (arg3)
#   return          -> eax
#
# clang INLINED pg_path_has_prefix into this function, producing a nested loop:
#   outer over rules[i], inner walking the prefix. Watch the same boundary test
#   from above reappear at .LBB1_6. This is why the extracted core is worth
#   reading: one routine, but it is the innermost thing the whole LSM does per
#   file_open and per exec.
# =============================================================================
	.globl	pg_policy_lookup
	.p2align	4
	.type	pg_policy_lookup,@function
pg_policy_lookup:
# %bb.0: — entry / fast reject
	movl	%ecx, %eax             # eax = default_verdict. Pre-load it so the
                                        #   "no rule matched" and "n<=0" exits are free.
	testl	%edx, %edx             # n <= 0 ?  (compare signed against 0)
	jle	.LBB1_11                # empty table -> return default immediately
                                        #   Note: no rbp was pushed yet, so this
                                        #   early exit needs no epilogue.
# %bb.1: — set up the outer loop
	pushq	%rbp                    # NOW establish the frame (we will loop)
	movq	%rsp, %rbp
	movl	%edx, %ecx             # ecx = n (loop bound; used as rcx below)
	xorl	%edx, %edx             # rdx = 0 = i (rule index)
	jmp	.LBB1_2

	.p2align	4
.LBB1_9:                                # ---- OUTER "continue": rules[i] did not match
	incq	%rdx                    # i++
	cmpq	%rcx, %rdx             # i == n ?
	je	.LBB1_10                # exhausted the table -> return default

.LBB1_2:                                # ---- OUTER LOOP TOP; rdx = i
	movq	%rdx, %r8              # r8 = i
	shlq	$4, %r8                # r8 = i * 16  <- sizeof(struct pg_rule) == 16
	movq	(%rsi,%r8), %r9        # r9 = rules[i].prefix   (pointer at offset 0)
	movzbl	(%r9), %r11d           # r11b = prefix[0]
	xorl	%r10d, %r10d           # r10 = 0 = j (inner index)  [inlined matcher]
	testb	%r11b, %r11b           # empty prefix?
	je	.LBB1_6                 # yes -> jump straight to the boundary test

	.p2align	4
.LBB1_4:                                # ---- INNER LOOP TOP; r11b = prefix[j]
	cmpb	%r11b, (%rdi,%r10)     # path[j] == prefix[j] ?
	jne	.LBB1_9                 # mismatch -> abandon this rule, try next
# %bb.5: — this char matched
	movzbl	1(%r9,%r10), %r11d     # r11b = prefix[j+1]  (peek next prefix byte)
	incq	%r10                    # j++
	testb	%r11b, %r11b           # prefix exhausted?
	jne	.LBB1_4                 # no -> keep matching this prefix

.LBB1_6:                                # ---- BOUNDARY CHECK; r10 = j = matched length
	movzbl	(%rdi,%r10), %r9d      # r9b = path[j] = first byte past the prefix
	cmpl	$47, %r9d              # is it '/' ?
	je	.LBB1_8                 # yes -> component boundary -> this rule matches
# %bb.7:
	testl	%r9d, %r9d             # is it '\0' ?
	jne	.LBB1_9                 # neither '/' nor NUL -> false match -> next rule

.LBB1_8:                                # ---- rules[i] MATCHES: return its verdict
	addq	%r8, %rsi             # rsi = rules + i*16 = &rules[i]
	movl	8(%rsi), %eax         # eax = rules[i].verdict  <- .verdict at offset 8

.LBB1_10:                               # fell through here after a match, or from
                                        #   .LBB1_9 exhausting the table (eax=default)
	popq	%rbp                    # tear down the frame we built at %bb.1
.LBB1_11:                               # the n<=0 fast path lands here (no frame)
	retq                            # return eax (a verdict, or the default)

.Lfunc_end1:
	.size	pg_policy_lookup, .Lfunc_end1-pg_policy_lookup

	.ident	"clang version 20.1.8"
	.section	".note.GNU-stack","",@progbits  # stack need not be executable
	.addrsig                                        # address-significance table
                                                        #   (LTO metadata; harmless)
# =============================================================================
# WHAT TO TAKE AWAY
#   * The security rule is one `orb %cl,%al`: a prefix only matches a path at a
#     component boundary ('\0' or '/'). Delete that test and the LSM's protected
#     set silently swells to every sibling name that shares the prefix.
#   * sizeof(struct pg_rule)==16 and .verdict-at-offset-8 are not documentation;
#     they are the `shlq $4` and `movl 8(...)` the compiler had to emit. Struct
#     layout IS the addressing mode.
#   * Both routines are leaf functions using only caller-saved scratch, so they
#     never touch rbx/r12-r15 — a good reminder of which registers are "free".
#   * Compare with demo.O0.s to see the same logic written naively — every
#     variable spilled to the stack (-16(%rbp), -32(%rbp)) and reloaded each
#     use — and with demo.O2.s to see the optimizer's most aggressive form.
# =============================================================================
