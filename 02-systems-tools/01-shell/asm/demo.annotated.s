# =============================================================================
# demo.annotated.s — clang -O1 output for asm/demo.c, explained instruction by
#                     instruction. This is the shell's glob matcher core: the
#                     two-pointer '*' backtracking that makes globbing cheap.
# =============================================================================
#
# HOW TO READ THIS FILE
# ---------------------
# This is the EXACT assembly clang 20 emits for asm/demo.c at -O1 (see demo.s for
# the untouched original — the labels and instruction order here match it exactly
# so you can diff the two), with a comment on essentially every instruction. AT&T
# syntax throughout:  op  src, dst   (destination LAST). `%reg` is a register,
# `$imm` an immediate, `N(%reg)` is memory at [reg+N]. Register widths are the
# SAME register: rax(64) / eax(32) / ax(16) / al(8); writing eax zero-extends
# into rax, which is why the compiler prefers 32-bit `movl`/`movzbl` forms.
#
# THE SYSTEM V AMD64 ABI (the contract glob_star obeys)
# -----------------------------------------------------
#   integer/pointer args, in order:  rdi, rsi, rdx, rcx, r8, r9, then the STACK
#   return value:                    rax
#   callee-saved (must preserve):    rbx, rbp, r12, r13, r14, r15
#   caller-saved (free to clobber):  rax, rcx, rdx, rsi, rdi, r8, r9, r10, r11
#   the "red zone":                  128 bytes below rsp a leaf may use freely
#   stack alignment:                 rsp % 16 == 0 at the point of any `call`
#
# glob_star(const char *pat, const char *str) takes two pointers:
#     rdi = pat      rsi = str      -> returns int in eax (1 = match, 0 = no)
# It is a LEAF (calls nothing), so clang keeps every live value in caller-saved
# registers and never touches the callee-saved set or spills to the stack. The
# frame (push %rbp / mov %rsp,%rbp) is kept only for debuggability at -O1.
#
# THE REGISTER MAP the optimizer chose (memorize this before reading the body)
# ---------------------------------------------------------------------------
#   rdi = pat        (current pattern cursor)
#   rsi = str        (current string  cursor)
#   rax = star_pat   (pattern pos just after the last '*'; 0 == "no star yet")
#   rcx = star_str   (string  pos the last '*' currently starts at)
#   dl  = c          (the current byte *str) AND doubles as a "keep going?" flag
#   r8b = *pat       (the current pattern byte under inspection)
#   r9b = pat[1]     (look-ahead byte, only for the '\\' escape case)
#
# THE ONE CLEVER TRICK: `dl` has two lifetimes per iteration. At the loop head
# (.LBB0_1) it is loaded with the input byte c=*str and used for all the compares.
# Then, just before looping, the code overwrites dl with 1 to mean "continue" (or
# 0 to mean "bail out, return 0"). The merge point .LBB0_15 tests dl: nonzero
# re-enters the loop (which immediately reloads dl from *str), zero returns 0. One
# register, two jobs — the sort of thing you only notice by reading the asm.
#
# LABELS, RENAMED IN COMMENTS (the code keeps clang's names so demo.s diffs clean)
#   .LBB0_1  = LOOP HEADER          while (*str)
#   .LBB0_2  = DISPATCH on *pat
#   .LBB0_7  = '*' CASE             record star_pat/star_str
#   .LBB0_8  = '\\' ESCAPE CASE
#   .LBB0_11 = LITERAL CASE
#   .LBB0_5  = ADVANCE (pat++)      also the '?' target
#   .LBB0_6  = ADVANCE tail (str++)
#   .LBB0_12 = BACKTRACK            pat=star_pat; str=++star_str
#   .LBB0_13 = HARD FAIL
#   .LBB0_15 = CONTINUE-OR-RETURN0 merge (tests dl)
#   .LBB0_17 = LOOP EXIT           skip trailing '*', test *pat=='\0'
#   .LBB0_16 = RETURN 0
# =============================================================================

	.file	"demo.c"
	.text
	.globl	glob_star                       # export glob_star for the linker
	.p2align	4                       # 16-byte align the entry (I-fetch)
	.type	glob_star,@function
glob_star:                                      # int glob_star(pat=rdi, str=rsi)
# %bb.0:                                         # PROLOGUE + initialization
	pushq	%rbp                    # save caller's frame pointer
	movq	%rsp, %rbp             # rbp = frame base (kept only for debug at -O1)
	xorl	%eax, %eax             # rax = 0  == star_pat = NULL (no '*' seen yet)
	xorl	%ecx, %ecx             # rcx = 0  == star_str = NULL
	jmp	.LBB0_1                # jump straight to the loop test (*str?)

	.p2align	4
.LBB0_7:                                        # '*' CASE: star_pat=++pat; star_str=str
	incq	%rdi                    # pat++   (star_pat points AFTER the '*')
	movb	$1, %dl                # dl = 1  == "keep going" flag for .LBB0_15
	movq	%rdi, %rax             # star_pat = pat
	movq	%rsi, %rcx             # star_str = str  (the star matches empty first)

.LBB0_15:                                       # CONTINUE-OR-RETURN0 merge point
	testb	%dl, %dl               # keep going?  (dl != 0)
	je	.LBB0_16               #   dl == 0 -> hard failure, return 0

.LBB0_1:                                        # LOOP HEADER:  while (*str != '\0')
	movzbl	(%rsi), %edx           # dl = c = (uchar)*str  (zero-extended load)
	testb	%dl, %dl               # *str == 0 ?  (end of the string)
	je	.LBB0_17               #   yes -> leave loop, handle trailing '*'s

# %bb.2:                                         # DISPATCH on *pat
	movzbl	(%rdi), %r8d           # r8 = (uchar)*pat
	cmpl	$42, %r8d              # *pat == '*' (ASCII 42) ?
	je	.LBB0_7                #   yes -> the '*' case above
# %bb.3:
	cmpl	$92, %r8d              # *pat == '\\' (ASCII 92) ?
	je	.LBB0_8                #   yes -> the escape case below
# %bb.4:
	cmpl	$63, %r8d              # *pat == '?' (ASCII 63) ?
	jne	.LBB0_11               #   no  -> generic literal compare
	jmp	.LBB0_5                #   yes -> '?' matches any char: advance both

	.p2align	4
.LBB0_8:                                        # '\\x' ESCAPE CASE
	movzbl	1(%rdi), %r9d          # r9 = (uchar)pat[1]  (the escaped char)
	testb	%r9b, %r9b             # pat[1] == 0 ?  (a trailing backslash)
	je	.LBB0_11               #   yes -> treat the '\\' itself as a literal
# %bb.9:
	cmpb	%dl, %r9b              # pat[1] == c ?   (dl still holds c)
	jne	.LBB0_12               #   mismatch -> backtrack into a '*'
# %bb.10:
	addq	$2, %rdi               # pat += 2   (skip '\\' and the escaped char)
	jmp	.LBB0_6                # then str++ (shared advance tail)

	.p2align	4
.LBB0_11:                                       # LITERAL CASE:  if (*pat == c) ...
	cmpb	%dl, %r8b              # *pat == c ?   (r8b = *pat, dl = c)
	jne	.LBB0_12               #   mismatch -> backtrack
.LBB0_5:                                        # ADVANCE (also the '?' target)
	incq	%rdi                    # pat++
.LBB0_6:                                        # ADVANCE tail
	incq	%rsi                    # str++
	movb	$1, %dl                # dl = 1  == "keep going"
	jmp	.LBB0_15

	.p2align	4
.LBB0_12:                                       # BACKTRACK: if (star_pat) ... else 0
	testq	%rax, %rax             # star_pat == NULL ?  (nothing to fall back on)
	je	.LBB0_13               #   yes -> unrecoverable mismatch
# %bb.14:
	incq	%rcx                    # ++star_str  (let the star eat one more char)
	movb	$1, %dl                # dl = 1  == "keep going"
	movq	%rax, %rdi             # pat = star_pat  (resume just after the star)
	movq	%rcx, %rsi             # str = star_str  (the advanced position). Note
	jmp	.LBB0_15               #   star_str only ever INCREASES -> O(n*m) bound.

.LBB0_13:                                       # HARD FAIL (mismatch, no star)
	xorl	%eax, %eax             # rax = 0  (tidy the return register)
	xorl	%edx, %edx             # dl = 0  == "bail"; .LBB0_15 will return 0
	jmp	.LBB0_15

	.p2align	4
.LBB0_17:                                       # LOOP EXIT: skip trailing '*'s
	movzbl	(%rdi), %ecx           # cl = *pat
	incq	%rdi                    # pat++  (advance past the byte we just read)
	cmpb	$42, %cl               # was it '*' ?
	je	.LBB0_17               #   yes -> keep skipping stars
# %bb.18:                                        # return (*pat == '\0')
	xorl	%eax, %eax             # eax = 0 (prepare the boolean result)
	testb	%cl, %cl               # first non-star byte == '\0' ?
	sete	%al                    # al = (cl == 0) -> 1 iff pattern fully consumed
	popq	%rbp                    # EPILOGUE: restore frame pointer
	retq                            # return al (1 = match, 0 = no match)

.LBB0_16:                                       # RETURN 0 (from the dl==0 path)
	xorl	%eax, %eax             # return value 0
	popq	%rbp                    # EPILOGUE
	retq
.Lfunc_end0:
	.size	glob_star, .Lfunc_end0-glob_star

# =============================================================================
# demo_run — the self-contained driver (see demo.s for the full instruction text).
# =============================================================================
# demo_run calls glob_star four times with COMPILE-TIME-CONSTANT strings and ORs
# the results into bits 0..3. At -O1 the optimizer makes a decision worth seeing:
# it INLINES glob_star four separate times (once per call site) instead of
# emitting four `call`s. The result in demo.s is four back-to-back copies of the
# exact loop annotated above — same shape, just renumbered labels (.LBB1_* rather
# than .LBB0_*) and different scratch registers per copy, wired so each copy's
# boolean result feeds the next `orl`. Because it is the identical routine four
# times, annotating each copy line-by-line would only repeat the above, so we
# describe the wiring instead:
#
#   copy 1:  glob_star("*.c",  "main.c")  -> bit 0  (<<0)
#   copy 2:  glob_star("a?c",  "abc")     -> bit 1  (<<1)
#   copy 3:  glob_star("a*d",  "abc")     -> bit 2  (<<2)
#   copy 4:  glob_star("*",    "")        -> bit 3  (<<3)
#
# The final tail of demo_run in demo.s is:
#     orl   %eax, %ecx           # combine copy-1 and copy-2 results
#     orl   %edx, %ecx           # ... OR in copy-3
#     leal  (%rcx,%rsi,8), %eax  # ... + copy-4 result * 8 (its <<3), via LEA
# i.e. the compiler turned `b3 << 3` into an `lea` with scale 8. Expected result:
# 0b1011 == 11.
#
# WHY -O1 DID NOT FOLD IT TO `mov $11,%eax`: all inputs are compile-time
# constants, so in principle demo_run is a constant. -O1 inlines but stops short
# of running the whole matcher at compile time; -O2 (see demo.O2.s) is more
# aggressive. Reading the asm is how you SEE inlining and folding decisions that
# are invisible in the C — the same lesson as the reference nolibc project.
# =============================================================================
	.ident	"clang version 20.1.8"
	.section	".note.GNU-stack","",@progbits  # non-executable stack (security)
	.addrsig                                # address-significance table (LTO aid)

# =============================================================================
# WHAT TO TAKE AWAY
#   * The '*' backtracking needs NO recursion and NO stack: star_pat/star_str
#     live in two registers (rax/rcx), and a mismatch just rewrites the pat/str
#     cursors at .LBB0_12. star_str only advances, so the loop is O(|pat|*|str|),
#     never exponential — the same discipline that keeps a regex engine linear.
#   * The optimizer packed the input byte c and the "continue?" boolean into the
#     single register dl, and fused the two-line trailing-star loop into one
#     read-advance-test loop (.LBB0_17). Register pressure is why it does this.
#   * Compare with demo.O0.s (every variable spilled to the stack, star_pat/
#     star_str reloaded from memory each iteration) and demo.O2.s (tighter
#     scheduling) to watch the optimizer work.
#   * asm/match.s is the REAL matcher (../match.c) INCLUDING [a-z] bracket
#     classes; read it next to see how the class scan compiles.
# =============================================================================
