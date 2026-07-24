# =============================================================================
# demo.annotated.s — clang's -O1 output for demo.c, explained line by line,
#                     with the saved return address pointed at explicitly.
# =============================================================================
#
# HOW TO READ THIS FILE
# ---------------------
# This is the exact assembly clang emits for asm/demo.c at -O1 (see demo.s for
# the untouched original), annotated on essentially every instruction. AT&T
# syntax, so operands read source-first:
#
#     op   src, dst            # movq %rsp,%rbp   =>  rbp = rsp
#     %reg                     # a register            (rax/eax/ax/al = same reg)
#     $imm                     # a literal constant
#     N(%reg)                  # memory at [reg + N]
#     (%base,%idx)             # memory at [base + idx]
#
# THE POINT OF THIS FILE
# ----------------------
# demo.c copies attacker bytes into a 64-byte stack buffer with NO length check
# (my_strcpy == strcpy, bug and all). Reading the assembly tells you the one
# number that turns "corrupt some memory" into "control the program counter":
# the byte distance from the start of the buffer to the saved return address.
# For THIS build that distance is 88 (not the naive 72), and below you can see
# precisely where the extra 16 bytes of slack come from.
#
# SYSTEM V AMD64 ABI, the rules every function here obeys:
#   - integer/pointer args, in order:  rdi, rsi, rdx, rcx, r8, r9
#   - return value:                    rax
#   - callee-saved (must preserve):    rbx, rbp, r12-r15, rsp
#   - caller-saved (free to clobber):  rax, rcx, rdx, rsi, rdi, r8-r11
#   - stack alignment:                 rsp % 16 == 0 at the point of a `call`
#   - the `call` instruction pushes an 8-byte return address; `ret` pops it
#     back into rip. THAT pushed value is what the overflow overwrites.
# =============================================================================

	.file	"demo.c"
	.text                           # executable code section

# =============================================================================
# my_strcpy(char *dst /*rdi*/, const char *src /*rsi*/) -> char * /*rax*/
# -----------------------------------------------------------------------------
# The unbounded copy. It is a leaf function (calls nothing), so it needs no
# stack locals — clang keeps everything in registers. The loop copies bytes
# from src to dst until (and including) the first NUL, and NEVER consults the
# size of dst. That missing size check is the entire vulnerability.
# =============================================================================
	.globl	my_strcpy
	.p2align	4               # 16-byte-align the entry for the fetch unit
	.type	my_strcpy,@function
my_strcpy:
# ---- PROLOGUE (frame pointer kept because we pass -fno-omit-frame-pointer) ---
	pushq	%rbp                    # save caller's frame pointer
	movq	%rsp, %rbp              # rbp = base of our (empty) frame
	movq	%rdi, %rax              # rax = dst. Two jobs: (1) it is the return
                                        #   value (strcpy returns dst), and (2) it
                                        #   is the base the loop writes through.
	xorl	%ecx, %ecx              # rcx = 0: the byte index i, our loop cursor.
                                        #   `xor r,r` is the 2-byte idiom for zero.

# ---- COPY LOOP:  while ((dst[i] = src[i]) != 0) i++; -------------------------
# This is the classic strcpy inner loop. Notice there is no comparison of i
# against any buffer length: the ONLY thing that stops it is reading a 0 byte
# out of src. An attacker who controls src controls how far this writes.
	.p2align	4
.Lcopy_loop:                            # was .LBB0_1
	movzbl	(%rsi,%rcx), %edx       # dl = src[i]; movzbl zero-extends the byte
                                        #   into a 32-bit reg (clears the top bits).
	movb	%dl, (%rax,%rcx)        # dst[i] = dl  <-- THE out-of-bounds write.
                                        #   When i grows past the size of dst, this
                                        #   store lands on saved rbp, then on the
                                        #   saved return address, then beyond.
	incq	%rcx                    # i++  (advance the cursor)
	testb	%dl, %dl                # set ZF if the byte we just copied was 0
	jne	.Lcopy_loop             # not NUL yet -> keep copying. Loop length is
                                        #   bounded only by where src's first 0 is.
# ---- EPILOGUE ---------------------------------------------------------------
	popq	%rbp                    # restore caller's frame pointer
	retq                            # return dst (already in rax)
.Lfunc_end0:
	.size	my_strcpy, .Lfunc_end0-my_strcpy

# =============================================================================
# vulnerable(const char *attacker /*rdi*/) -> int /*rax*/
# -----------------------------------------------------------------------------
# The victim frame. It owns the 64-byte `buf`, calls my_strcpy to fill it, then
# calls consume(buf). Because it makes calls and takes buf's address, buf must
# live on the stack — and that stack slot sits BELOW the saved return address,
# so overrunning it reaches the return address. This is the frame to study.
# =============================================================================
	.globl	vulnerable
	.p2align	4
	.type	vulnerable,@function
vulnerable:
# ---- PROLOGUE + FRAME LAYOUT ------------------------------------------------
# Trace rsp so you can see the 16-byte-alignment rounding that pushes buf down
# to -80(%rbp). At entry rsp % 16 == 8 (the caller's `call` pushed 8 bytes).
	pushq	%rbp                    # save caller rbp. AFTER the next mov, this
                                        #   saved rbp sits at [rbp]; the return
                                        #   address the caller pushed sits at
                                        #   [rbp + 8]. rsp % 16 == 0 now.
	movq	%rsp, %rbp              # establish frame: rbp = frame base.
	pushq	%rbx                    # save callee-saved rbx (we use it to hold
                                        #   &buf across the two calls). Stored at
                                        #   [rbp - 8]. rsp % 16 == 8 now.
	subq	$72, %rsp               # reserve 72 bytes of locals. rsp lands at
                                        #   rbp-80, and rsp % 16 == 0 again — the
                                        #   alignment the two upcoming calls need.
                                        #   64 (buf) + 8 (rbx already pushed) is 72,
                                        #   but the 8-push + 72-sub together leave
                                        #   buf's base at -80(%rbp): 16 bytes of
                                        #   slack below rbp before buf even starts.

#   FRAME MAP (higher address at top):
#        [rbp + 8]  = SAVED RETURN ADDRESS   <=== the overflow's real target
#        [rbp + 0]  = saved caller rbp
#        [rbp - 8]  = saved rbx              (the "16 bytes of slack": rbx + pad)
#        [rbp - 16] = padding
#        [rbp - 80 .. rbp - 17] = buf[0..63] (the 64-byte buffer)
#
#   OFFSET, buf[0] -> saved return address:
#        (rbp+8) - (rbp-80) = 88 bytes.
#        So: 88 filler bytes, then 8 bytes overwrite the return address.
#        (The naive 64+8 = 72 is wrong precisely because of that 16-byte slack.)

# ---- CALL my_strcpy(buf, attacker) — THE BUG -------------------------------
	movq	%rdi, %rsi              # rsi = attacker. It becomes my_strcpy arg2
                                        #   (src). rdi still holds attacker for one
                                        #   more instant; we overwrite it next.
	leaq	-80(%rbp), %rbx         # rbx = &buf. LEA computes the ADDRESS of the
                                        #   buffer (no memory access). Stash it in
                                        #   callee-saved rbx so it survives the call.
	movq	%rbx, %rdi              # rdi = &buf = my_strcpy arg1 (dst).
	callq	my_strcpy               # copy attacker -> buf, unbounded. If attacker
                                        #   is >= 89 bytes with no early NUL, the
                                        #   store `movb %dl,(%rax,%rcx)` inside
                                        #   my_strcpy walks up over [rbp-8], [rbp],
                                        #   and finally writes [rbp+8]: the return
                                        #   address this function will `ret` into.

# ---- CALL consume(buf) — keeps buf live so nothing is optimized away --------
	movq	%rbx, %rdi              # rdi = &buf (rbx preserved across the call)
	callq	consume@PLT             # consume(buf). @PLT: called through the
                                        #   Procedure Linkage Table because consume
                                        #   is an external symbol resolved at link/
                                        #   load time (see the GOT/PLT lesson in the
                                        #   ret2libc and ROP rungs).

# ---- EPILOGUE:  return 0 ----------------------------------------------------
	xorl	%eax, %eax              # rax = 0  (the `return 0;`)
	addq	$72, %rsp               # free the 72 bytes of locals
	popq	%rbx                    # restore callee-saved rbx
	popq	%rbp                    # restore caller's frame pointer
	retq                            # POP [rsp] (== the saved return address at
                                        #   [rbp+8]) INTO rip. If the copy above
                                        #   overwrote that slot, THIS is the moment
                                        #   the CPU jumps wherever the attacker
                                        #   chose. Control-flow hijack complete.
.Lfunc_end1:
	.size	vulnerable, .Lfunc_end1-vulnerable

	.ident	"clang version 20.1.8"
	.section	".note.GNU-stack","",@progbits  # non-exec stack requested here;
                                                        #   Rung 1 REMOVES this default
                                                        #   with `-z execstack` so the
                                                        #   injected shellcode can run.
# =============================================================================
# WHAT TO TAKE AWAY
#   * A stack buffer overflow is a control-flow hijack because the buffer lives
#     BELOW the saved return address, and strcpy writes UPWARD toward it.
#   * The offset is NOT "buffer size + 8". The compiler adds alignment slack and
#     may save extra registers below rbp. Here 64-byte buf -> real offset 88.
#     MEASURE it (De Bruijn cyclic pattern), never assume it.
#   * The single instruction that ends it all is `retq`: it trusts [rsp] to be a
#     legitimate return address. Every mitigation in the README is, at heart, a
#     way to make that trust safe again — a canary that notices the slot was
#     touched, ASLR/PIE that hides where to point it, NX that makes the injected
#     bytes non-executable, CET shadow stacks that keep a second, unwritable
#     copy of the return address to compare against.
# =============================================================================
