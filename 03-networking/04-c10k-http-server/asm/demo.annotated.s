# =============================================================================
# demo.annotated.s — clang's -O1 output for demo.c (hp_parse), explained.
# =============================================================================
#
# HOW TO READ THIS FILE
# ---------------------
# This is the exact assembly clang 20 emits for asm/demo.c at -O1 (see demo.s
# for the untouched original), with a comment on essentially every instruction.
# AT&T syntax: `op src, dst`. `%reg` is a register, `$imm` an immediate,
# `N(%reg)` is memory at [reg+N], `sym(%rip)` a RIP-relative address, and
# `(%base,%index)` is [base+index].
#
# Register widths are the SAME register: rax(64)/eax(32)/ax(16)/al(8). Writing
# eax zero-extends into rax, so clang prefers `movl`/`incl` (shorter encodings)
# whenever the top 32 bits should end up zero.
#
# THE FUNCTION
# ------------
#   int hp_parse(struct req *req, const char *buf, usize len);
#
# It runs the HTTP request-parser state machine over buf[req->parsed .. len),
# one byte per loop iteration, and returns R_DONE(1) / R_MORE(0) / R_ERR(-1).
# It is resumable: call again with more bytes when it returns R_MORE.
#
# SysV AMD64 ABI (the contract this code obeys):
#   integer/pointer args:  rdi, rsi, rdx, rcx, r8, r9   (then the stack)
#   return value:          rax  (here eax: the int result)
#   callee-saved:          rbx, rbp, r12-r15  (we use rbx, so we save/restore it)
#   caller-saved (scratch):rax, rcx, rdx, rsi, rdi, r8-r11
#   "red zone":            128 bytes below rsp usable by a leaf without adjusting
#                          rsp — this function is a leaf (calls nothing) and uses
#                          NO stack locals at all: everything lives in registers.
#   stack alignment:       rsp % 16 == 0 at a `call` (no calls here, so moot).
#
# REGISTER ALLOCATION clang chose for the whole function (worth memorizing as
# you read — the code never reloads these from memory):
#   %rdi = req            (the struct pointer, arg0)          -- persistent
#   %rsi = buf            (arg1)                              -- persistent
#   %rdx = len            (arg2)                              -- persistent
#   %rax = i              (the cursor / loop index)           -- persistent
#   %ecx = req->head_bytes (a running copy kept in-register)  -- persistent
#   %r8  = 0xE00000000000367D  (punctuation bitmap for is_tchar)
#   %r9  = &"HTTP/1."     (the version literal)
#   %r11d= c             (the current byte buf[i])
#   %r10d= scratch: the switch's state value, and the "why we left the switch"
#          selector (0 => continue loop, 2 => slowloris break, 6 => goto out).
#   %rbx = scratch for ver_idx / is_tchar folding (callee-saved => saved below)
#
# STRUCT LAYOUT (clang's offsets; LP64, 8-byte alignment):
#   0:  int      state          32: usize target_off
#   8:  usize    parsed         40: usize target_len
#   16: usize    method_off     48: int   minor
#   24: usize    method_len     52: unsigned num_headers
#                               56: unsigned head_bytes
#                               60: u8    ver_idx        (struct size 64)
#
# THE BIG LESSON
# --------------
# We compiled with -fno-jump-tables, so the `switch (state)` did NOT become a
# single indirect jump through a table. Instead the optimizer built a BINARY
# SEARCH over the state value (compare against 5, then 2, then equality tests) —
# a balanced tree of `cmp`/`jcc`. That is what a state machine looks like as
# machine code, and it is why keeping the hot states low-numbered can matter.
# Also watch: is_tchar was INLINED (no `call`), and the punctuation branch of it
# became a single `btq` bit-test against a 64-bit mask — the whole "is this one
# of ! # $ % & ' * + - . ^ _ `" question answered in one instruction.
# =============================================================================

	.file	"demo.c"
	.text
	.globl	hp_parse                        # export hp_parse for the linker
	.p2align	4                       # 16-byte align the entry (I-fetch)
	.type	hp_parse,@function
hp_parse:                               # int hp_parse(req*, buf, len)
# %bb.0: ---- PROLOGUE ----------------------------------------------------------
	pushq	%rbp                            # save caller's frame pointer
	movq	%rsp, %rbp                      # rbp = frame base (for backtraces)
	pushq	%rbx                            # save callee-saved rbx: we clobber it
	movq	8(%rdi), %rax                   # rax = i = req->parsed  (resume point)
	cmpq	%rdx, %rax                      # compare i, len
	jae	.LBB0_84                        # if i >= len: nothing to do -> finalize
# %bb.1: ---- loop preheader: hoist loop-invariant values into registers --------
	movl	56(%rdi), %ecx                  # ecx = req->head_bytes
	incl	%ecx                            # pre-increment for the first byte
                                                #   (mirrors ++req->head_bytes)
	movabsq	$-2305843009213680003, %r8      # r8 = 0xE00000000000367D:
                                                #   a 64-bit BITMAP of the RFC token
                                                #   punctuation, indexed by (c-33).
                                                #   Bits set: ! # $ % & ' * + - . ^ _ `
                                                #   ('|' and '~' are out of the 0..63
                                                #    window and tested separately.)
	leaq	hp_parse.V(%rip), %r9           # r9 = &"HTTP/1." (version literal),
                                                #   RIP-relative so it is position-
                                                #   independent.
	.p2align	4
# =============================================================================
# .LBB0_2  ==  .Lloop_top : top of the per-byte for-loop. Invariant: rax=i<len.
# =============================================================================
.LBB0_2:                                # for (; i < len; i++)
	movzbl	(%rsi,%rax), %r11d              # r11 = c = (u8)buf[i]  (zero-extended)
	movl	%ecx, 56(%rdi)                  # req->head_bytes = ecx  (store running #)
	cmpl	$8193, %ecx                     # head_bytes > 8192  (i.e. >= 8193)?
	jb	.LBB0_4                         # no  -> dispatch on state
# %bb.3:  ==  slowloris ceiling exceeded: state=S_ERR, break out of the loop -----
	movl	$13, (%rdi)                     # req->state = 13 = S_ERR
	movl	$2, %r10d                       # r10 = 2  (selector: "break", not goto)
	testl	%r10d, %r10d                    # (compiler-emitted; r10 is nonzero)
	je	.LBB0_75                        #   never taken
	jmp	.LBB0_81                        # -> classify_exit (r10==2 => finalize)
	.p2align	4
# =============================================================================
# .LBB0_4  ==  .Ldispatch : switch (req->state) as a BINARY SEARCH (no jump table)
# =============================================================================
.LBB0_4:
	movl	(%rdi), %r10d                   # r10 = req->state
	cmpl	$5, %r10d                       # state vs 5
	jg	.LBB0_12                        # state > 5  -> states 6..11 subtree
# %bb.5:
	cmpl	$2, %r10d                       # state vs 2
	jg	.LBB0_20                        # state in {3,4,5} -> LBB0_20
# %bb.6:
	testl	%r10d, %r10d                    # state == 0 ?
	je	.LBB0_31                        # S_METHOD -> LBB0_31
# %bb.7:
	cmpl	$1, %r10d                       # state == 1 ?
	je	.LBB0_41                        # S_TARGET -> LBB0_41
# %bb.8:
	cmpl	$2, %r10d                       # state == 2 ?
	jne	.LBB0_73                        # (defensive: not 0/1/2 here => error)
# %bb.9: ---- state S_VERSION: match the literal "HTTP/1." one byte at a time ----
	movzbl	60(%rdi), %ebx                  # ebx = req->ver_idx (0..6)
	cmpb	(%rbx,%r9), %r11b               # c == "HTTP/1."[ver_idx] ?
	jne	.LBB0_73                        # mismatch -> S_ERR
# %bb.10:
	incb	%bl                             # ver_idx++
	movb	%bl, 60(%rdi)                   # store req->ver_idx
	xorl	%r10d, %r10d                    # r10 = 0 (selector: "continue loop")
	cmpb	$7, %bl                         # matched all 7 chars of "HTTP/1." ?
	jne	.LBB0_74                        # not yet -> continue with next byte
# %bb.11:
	movl	$3, (%rdi)                      # req->state = 3 = S_MINOR
	testl	%r10d, %r10d                    # r10==0 -> fall through to continue
	je	.LBB0_75                        # -> loop_advance
	jmp	.LBB0_81
	.p2align	4
# =============================================================================
# .LBB0_12 : subtree for state > 5  (states 6..11)
# =============================================================================
.LBB0_12:
	cmpl	$8, %r10d                       # state vs 8
	jg	.LBB0_23                        # state > 8 -> {9,10,11} subtree
# %bb.13:
	cmpl	$6, %r10d                       # state == 6 ?
	je	.LBB0_34                        # S_H_START -> LBB0_34
# %bb.14:
	cmpl	$7, %r10d                       # state == 7 ?
	je	.LBB0_44                        # S_H_NAME -> LBB0_44
# %bb.15:
	cmpl	$8, %r10d                       # state == 8 ?
	jne	.LBB0_73                        # else error
# %bb.16: ---- state S_H_OWS: skip optional whitespace after the header ':' ------
	xorl	%r10d, %r10d                    # r10 = 0 ("continue") — provisional
	cmpl	$12, %r11d                       # c <= 12 ? (catch \t=9 and \n=10, \r=13
                                                #   is 13 so NOT <=12) — group test
	jle	.LBB0_71                        # c <= 12 -> handle \t / \n specially
# %bb.17:
	cmpl	$127, %r11d                      # c == 0x7f (DEL) ?
	je	.LBB0_73                        #   -> S_ERR (control byte in value)
# %bb.18:
	cmpl	$32, %r11d                       # c == ' ' (space) ?
	je	.LBB0_74                        #   -> skip OWS, continue (r10 still 0)
# %bb.19:
	cmpl	$13, %r11d                       # c == '\r' ?
	je	.LBB0_40                        #   -> empty value: count header, go S_H_CR
	jmp	.LBB0_76                        # else: a real value byte -> S_H_VALUE
.LBB0_20:                               # subtree for state in {3,4,5}
	cmpl	$3, %r10d                       # state == 3 ?
	je	.LBB0_36                        # S_MINOR -> LBB0_36
# %bb.21:
	cmpl	$4, %r10d                       # state == 4 ?
	je	.LBB0_46                        # S_CR -> LBB0_46
# %bb.22:
	cmpl	$5, %r10d                       # state == 5 ?
	je	.LBB0_28                        # S_LF -> LBB0_28 (shared LF check)
	jmp	.LBB0_73                        # else error
.LBB0_23:                               # subtree for state in {9,10,11}
	cmpl	$9, %r10d                       # state == 9 ?
	je	.LBB0_38                        # S_H_VALUE -> LBB0_38
# %bb.24:
	cmpl	$10, %r10d                      # state == 10 ?
	je	.LBB0_28                        # S_H_CR -> LBB0_28 (SAME LF check as S_LF)
# %bb.25:
	cmpl	$11, %r10d                      # state == 11 ?
	jne	.LBB0_73                        # else error
# %bb.26: ---- state S_END_LF: the final LF of the terminating blank line --------
	cmpl	$10, %r11d                      # c == '\n' ?
	jne	.LBB0_73                        # no -> S_ERR
# %bb.27:
	movl	$12, (%rdi)                     # req->state = 12 = S_DONE  (head complete)
	incq	%rax                            # i++  (the "i++; goto out;" — consume LF)
	movl	$6, %r10d                       # r10 = 6 (selector: "goto out")
	testl	%r10d, %r10d
	je	.LBB0_75
	jmp	.LBB0_81                        # -> classify_exit -> finalize (DONE)
# =============================================================================
# .LBB0_28  ==  .Llf_check : shared handler for S_LF (5) and S_H_CR (10).
#   Both mean "expect '\n', then go to S_H_START". The optimizer merged them.
# =============================================================================
.LBB0_28:
	cmpl	$10, %r11d                      # c == '\n' ?
	jne	.LBB0_73                        # no -> S_ERR
# %bb.29:
	movl	$6, (%rdi)                      # req->state = 6 = S_H_START
	jmp	.LBB0_30                        # -> continue loop
# =============================================================================
# .LBB0_31  ==  S_METHOD : read the method token; SP ends it.
# =============================================================================
.LBB0_31:
	cmpl	$32, %r11d                       # c == ' ' (space) ?
	jne	.LBB0_48                        # not space -> it must be a token char
# %bb.32:
	cmpq	$0, 24(%rdi)                    # method_len == 0 ? (SP with empty method)
	je	.LBB0_73                        #   -> S_ERR ("... " with no method)
# %bb.33:
	leaq	1(%rax), %r10                   # r10 = i + 1 (target starts after the SP)
	movq	%r10, 32(%rdi)                  # req->target_off = i + 1
	movl	$1, (%rdi)                      # req->state = 1 = S_TARGET
	jmp	.LBB0_30                        # -> continue loop
# =============================================================================
# .LBB0_34  ==  S_H_START : a header line begins, OR a blank line ends the head.
# =============================================================================
.LBB0_34:
	cmpl	$13, %r11d                       # c == '\r' ? (a bare CRLF => end of head)
	jne	.LBB0_55                        # not CR -> must be a token (header name)
# %bb.35:
	movl	$11, (%rdi)                     # req->state = 11 = S_END_LF (await final LF)
	jmp	.LBB0_30                        # -> continue loop
# =============================================================================
# .LBB0_36  ==  S_MINOR : the single HTTP minor-version digit.
# =============================================================================
.LBB0_36:
	leal	-58(%r11), %r10d                # r10 = c - 58  (58 == '9'+1)
	cmpb	$-11, %r10b                     # unsigned: (c-58) >= 0xF5  <=>  c in '0'..'9'
	jbe	.LBB0_73                        # else (not a digit) -> S_ERR
# %bb.37:
	addl	$-48, %r11d                     # r11 = c - '0'  (the numeric minor version)
	movl	%r11d, 48(%rdi)                 # req->minor = c - '0'
	movl	$4, (%rdi)                      # req->state = 4 = S_CR
	jmp	.LBB0_30                        # -> continue loop
# =============================================================================
# .LBB0_38  ==  S_H_VALUE : read the header value until CR.
# =============================================================================
.LBB0_38:
	cmpl	$10, %r11d                       # c == '\n' ? (a bare LF inside a value)
	je	.LBB0_73                        #   -> S_ERR
# %bb.39:
	xorl	%r10d, %r10d                    # r10 = 0 ("continue")
	cmpl	$13, %r11d                       # c == '\r' ? (end of the value)
	jne	.LBB0_74                        # no -> ordinary value byte, keep reading
	                                        # yes -> fall into the CR handler below
# =============================================================================
# .LBB0_40  ==  .Lvalue_cr : CR ended a header value (from S_H_VALUE or empty OWS).
# =============================================================================
.LBB0_40:
	incl	52(%rdi)                        # req->num_headers++  (one more header)
	movl	$10, (%rdi)                     # req->state = 10 = S_H_CR (await LF)
	testl	%r10d, %r10d                    # r10 is 0 here -> continue
	je	.LBB0_75                        # -> loop_advance
	jmp	.LBB0_81
# =============================================================================
# .LBB0_41  ==  S_TARGET : read the request-target until SP.
# =============================================================================
.LBB0_41:
	cmpl	$32, %r11d                       # c == ' ' ?
	jne	.LBB0_61                        # not SP -> validate as a target byte
# %bb.42:
	cmpq	$0, 40(%rdi)                    # target_len == 0 ? (SP with empty target)
	je	.LBB0_73                        #   -> S_ERR
# %bb.43:
	movb	$0, 60(%rdi)                    # req->ver_idx = 0 (start matching version)
	movl	$2, (%rdi)                      # req->state = 2 = S_VERSION
	jmp	.LBB0_30                        # -> continue loop
# =============================================================================
# .LBB0_44  ==  S_H_NAME : read the header field-name until ':'.
# =============================================================================
.LBB0_44:
	cmpl	$58, %r11d                       # c == ':' (58) ?
	jne	.LBB0_65                        # not ':' -> must be a token char
# %bb.45:
	movl	$8, (%rdi)                      # req->state = 8 = S_H_OWS
	jmp	.LBB0_30                        # -> continue loop
# =============================================================================
# .LBB0_46  ==  S_CR : expect the CR that ends the request line.
# =============================================================================
.LBB0_46:
	cmpl	$13, %r11d                       # c == '\r' ?
	jne	.LBB0_73                        # no -> S_ERR
# %bb.47:
	movl	$5, (%rdi)                      # req->state = 5 = S_LF
	jmp	.LBB0_30                        # -> continue loop
# =============================================================================
# .LBB0_48  ==  is_tchar() inlined for the S_METHOD path. Accept => extend method.
# =============================================================================
.LBB0_48:
	leal	-48(%r11), %r10d                # r10 = c - '0'
	cmpb	$10, %r10b                       # (c-'0') < 10  ->  c is a DIGIT
	jb	.LBB0_52                        #   accept
# %bb.49:
	movl	%r11d, %r10d                    # r10 = c
	andb	$-33, %r10b                     # r10 &= 0xDF  (clear bit5: fold to UPPER)
	addb	$-65, %r10b                     # r10 = folded - 'A'
	cmpb	$26, %r10b                       # < 26  ->  c is a LETTER (A-Z or a-z)
	jb	.LBB0_52                        #   accept
# %bb.50:
	movl	%r11d, %r10d                    # r10 = c
	addl	$-33, %r10d                     # r10 = c - 33  (index into punctuation map)
	cmpl	$63, %r10d                       # index > 63 -> outside the 64-bit bitmap
	ja	.LBB0_77                        #   -> test '|' and '~' explicitly
# %bb.51:
	btq	%r10, %r8                       # BIT TEST: is bit (c-33) set in the mask?
	jae	.LBB0_77                        # CF=0 (bit clear) -> maybe '|'/'~', else err
.LBB0_52:                               # ---- ACCEPT: c is a token char (method) ----
	movq	24(%rdi), %r11                  # r11 = req->method_len
	testq	%r11, %r11                      # method_len == 0 ? (first char of method)
	jne	.LBB0_54                        # no -> offset already recorded
# %bb.53:
	movq	%rax, 16(%rdi)                  # req->method_off = i (record start)
.LBB0_54:
	incq	%r11                            # method_len++
	movq	%r11, 24(%rdi)                  # store req->method_len
	xorl	%r10d, %r10d                    # r10 = 0 ("continue")
	cmpq	$17, %r11                       # method_len >= 17 (> MAX_METHOD_LEN 16)?
	jae	.LBB0_73                        #   too long -> S_ERR
	jmp	.LBB0_74                        # else continue loop
# =============================================================================
# .LBB0_55  ==  is_tchar() inlined for the S_H_START path (header name's 1st char).
# =============================================================================
.LBB0_55:
	leal	-48(%r11), %r10d                # r10 = c - '0'
	cmpb	$10, %r10b                       # digit?
	jb	.LBB0_59                        #   accept -> begin header name
# %bb.56:
	movl	%r11d, %r10d
	andb	$-33, %r10b                     # fold to upper
	addb	$-65, %r10b                     # - 'A'
	cmpb	$26, %r10b                       # letter?
	jb	.LBB0_59                        #   accept
# %bb.57:
	leal	-33(%r11), %r10d                # r10 = c - 33
	cmpl	$63, %r10d                       # outside bitmap window?
	ja	.LBB0_79                        #   -> test '|' / '~'
# %bb.58:
	btq	%r10, %r8                       # punctuation bitmap test
	jae	.LBB0_79                        #   clear -> maybe '|'/'~'
.LBB0_59:                               # ---- ACCEPT: header name starts here -------
	cmpl	$64, 52(%rdi)                   # num_headers >= 64 (MAX_HEADERS) ?
	jae	.LBB0_73                        #   too many headers -> S_ERR
# %bb.60:
	movl	$7, (%rdi)                      # req->state = 7 = S_H_NAME
	jmp	.LBB0_30                        # -> continue loop
# =============================================================================
# .LBB0_61  ==  S_TARGET byte validation: reject controls/space/DEL in the target.
# =============================================================================
.LBB0_61:
	cmpb	$33, %r11b                       # c >= 33 (0x21)? (i.e. above SP/controls)
	setae	%r10b                            # r10b = (c >= 33)
	cmpl	$127, %r11d                      # c != 127 (DEL)?
	setne	%r11b                            # r11b = (c != 127)
	testb	%r11b, %r10b                    # both true => a legal visible target byte
	je	.LBB0_73                        # otherwise (control/space/DEL) -> S_ERR
# %bb.62:
	movq	40(%rdi), %r10                  # r10 = req->target_len
	testq	%r10, %r10                      # target_len == 0 ? (first target byte)
	jne	.LBB0_64                        # no -> offset recorded already
# %bb.63:
	movq	%rax, 32(%rdi)                  # req->target_off = i (record start)
.LBB0_64:
	incq	%r10                            # target_len++
	movq	%r10, 40(%rdi)                  # store req->target_len
# =============================================================================
# .LBB0_30  ==  .Lcontinue : states that always "continue the loop" land here.
# =============================================================================
.LBB0_30:
	xorl	%r10d, %r10d                    # r10 = 0 (selector: "continue")
	testl	%r10d, %r10d
	je	.LBB0_75                        # -> loop_advance (always taken)
	jmp	.LBB0_81
# =============================================================================
# .LBB0_65  ==  is_tchar() inlined for the S_H_NAME path (subsequent name chars).
#   Same three tests, but "not a token char" here is a hard error (it was not a
#   ':' either), so the fall-through goes to set_error.
# =============================================================================
.LBB0_65:
	leal	-48(%r11), %ebx                 # ebx = c - '0'
	xorl	%r10d, %r10d                    # r10 = 0 ("continue" if it IS a tchar)
	cmpb	$10, %bl                         # digit?
	jb	.LBB0_74                        #   yes -> continue (still in S_H_NAME)
# %bb.66:
	movl	%r11d, %ebx
	andb	$-33, %bl                       # fold upper
	addb	$-65, %bl                       # - 'A'
	cmpb	$26, %bl                         # letter?
	jb	.LBB0_74                        #   yes -> continue
# %bb.67:
	leal	-33(%r11), %ebx                 # ebx = c - 33
	cmpl	$63, %ebx                        # outside bitmap window?
	ja	.LBB0_69                        #   -> test '|' / '~'
# %bb.68:
	btq	%rbx, %r8                       # punctuation bitmap test
	jb	.LBB0_74                        #   set -> it is a tchar, continue
.LBB0_69:
	cmpl	$124, %r11d                      # c == '|' (124) ?  (out-of-window tchar)
	je	.LBB0_74                        #   yes -> continue
# %bb.70:
	cmpl	$126, %r11d                      # c == '~' (126) ?  (out-of-window tchar)
	jne	.LBB0_73                        #   no  -> not a token char -> S_ERR
	jmp	.LBB0_74                        #   yes -> continue
# =============================================================================
# .LBB0_71  ==  S_H_OWS low-byte cases: c <= 12, i.e. TAB(9) or LF(10).
# =============================================================================
.LBB0_71:
	cmpl	$9, %r11d                        # c == '\t' (TAB) ?
	je	.LBB0_74                        #   yes -> skip OWS, continue
# %bb.72:
	cmpl	$10, %r11d                       # c == '\n' (bare LF in OWS) ?
	jne	.LBB0_76                        #   no (some other ctrl) -> treat as value
	                                        #   yes -> fall into set_error
	.p2align	4
# =============================================================================
# .LBB0_73  ==  .Lset_error : the single "reject this request" sink.
# =============================================================================
.LBB0_73:
	movl	$13, (%rdi)                     # req->state = 13 = S_ERR
	movl	$6, %r10d                       # r10 = 6 (selector: "goto out")
.LBB0_74:                               # .Ldispatch_exit: r10 decides break vs cont.
	testl	%r10d, %r10d                    # r10 == 0 ?
	jne	.LBB0_81                        #   nonzero -> leave loop (classify_exit)
.LBB0_75:                               # .Lloop_advance: the for-loop's i++ step
	incq	%rax                            # i++
	incl	%ecx                            # head_bytes++ (kept in a register)
	cmpq	%rdx, %rax                      # i < len ?
	jb	.LBB0_2                         #   yes -> next byte (top of loop)
	jmp	.LBB0_84                        #   no  -> ran out of input -> finalize
# =============================================================================
# .LBB0_76  ==  S_H_OWS: first non-space byte begins the value -> S_H_VALUE.
# =============================================================================
.LBB0_76:
	movl	$9, (%rdi)                      # req->state = 9 = S_H_VALUE
	testl	%r10d, %r10d                    # r10==0 -> continue
	je	.LBB0_75
	jmp	.LBB0_81
# =============================================================================
# .LBB0_77 / .LBB0_79  ==  out-of-window token punctuation: '|' (124), '~' (126).
#   Reached when (c-33) > 63 so the bitmap could not be consulted.
# =============================================================================
.LBB0_77:                               # (S_METHOD path)
	cmpl	$124, %r11d                      # c == '|' ?
	je	.LBB0_52                        #   yes -> accept as method char
# %bb.78:
	cmpl	$126, %r11d                      # c == '~' ?
	je	.LBB0_52                        #   yes -> accept
	jmp	.LBB0_73                        #   else -> S_ERR
.LBB0_79:                               # (S_H_START path)
	cmpl	$124, %r11d                      # c == '|' ?
	je	.LBB0_59                        #   yes -> begin header name
# %bb.80:
	cmpl	$126, %r11d                      # c == '~' ?
	je	.LBB0_59                        #   yes -> begin header name
	jmp	.LBB0_73                        #   else -> S_ERR
# =============================================================================
# .LBB0_81  ==  .Lclassify_exit : we left the switch NOT via "continue".
#   r10 selector: 2 => slowloris break; 6 => goto out; (4 => an unreachable
#   compiler-synthesized path). All roads lead to finalize here.
# =============================================================================
.LBB0_81:
	cmpl	$2, %r10d                       # r10 == 2 (the break)?
	je	.LBB0_84                        #   -> finalize
# %bb.82:
	cmpl	$4, %r10d                       # r10 == 4 ? (never set in practice)
	jne	.LBB0_84                        #   r10==6 (goto out) -> finalize
# %bb.83:
                                        # implicit-def: $eax   (dead path)
	jmp	.LBB0_85                        # unreachable; would return with eax undef
# =============================================================================
# .LBB0_84  ==  .Lfinalize (the C label `out:`): store i, compute return code.
# =============================================================================
.LBB0_84:
	movq	%rax, 8(%rdi)                   # req->parsed = i  (persist resume point)
	movl	(%rdi), %eax                    # eax = req->state
	xorl	%ecx, %ecx                      # ecx = 0
	cmpl	$12, %eax                       # state == 12 (S_DONE) ?
	sete	%cl                             # cl = (state == S_DONE) ? 1 : 0
	cmpl	$13, %eax                       # state == 13 (S_ERR) ?
	movl	$-1, %eax                       # eax = -1 = R_ERR  (assume error)
	cmovnel	%ecx, %eax                      # if state != S_ERR: eax = cl
                                                #   -> S_DONE ? 1 (R_DONE) : 0 (R_MORE)
                                                #   This branchless trio IS the three
                                                #   `return` statements at `out:`.
.LBB0_85:                               # ---- EPILOGUE ----
	popq	%rbx                            # restore callee-saved rbx
	popq	%rbp                            # restore caller's frame pointer
	retq                                    # return eax to the caller
.Lfunc_end0:
	.size	hp_parse, .Lfunc_end0-hp_parse

# ---- READ-ONLY DATA ---------------------------------------------------------
	.type	hp_parse.V,@object
	.section	.rodata.str1.1,"aMS",@progbits,1
hp_parse.V:
	.asciz	"HTTP/1."                       # the version literal, NUL-terminated;
                                                #   8 bytes, indexed by ver_idx 0..6.
	.size	hp_parse.V, 8

	.ident	"clang version 20.1.8"          # toolchain stamp (harmless)
	.section	".note.GNU-stack","",@progbits  # non-executable stack (security)
	.addrsig
# =============================================================================
# WHAT TO TAKE AWAY
#   * A `switch` over an enum, compiled with -fno-jump-tables, becomes a BINARY
#     SEARCH of cmp/jcc — see .Ldispatch. With jump tables it would be one
#     indirect jump; the tradeoff is branch predictability vs. table lookup.
#   * is_tchar() was INLINED three times (once per calling state) and its
#     punctuation test collapsed to a single `btq` against a 64-bit bitmap —
#     a beautiful example of turning a 14-way `case` into one instruction.
#   * The whole parser needs NO stack locals: `i`, `c`, `head_bytes`, and the
#     two loop-invariant constants all live in registers for the entire loop.
#     That is why a byte-at-a-time state machine is cheap enough to run on every
#     octet of every request at C10k scale.
#   * The three `return`s at `out:` became a branchless sete/cmov sequence.
#   * Compare with demo.O0.s (every variable spilled to the stack, one block per
#     C statement) and demo.O2.s (even more aggressive) to see the optimizer work.
# =============================================================================
