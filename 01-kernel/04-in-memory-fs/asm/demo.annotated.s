# =============================================================================
# demo.annotated.s — clang's -O1 output for asm/demo.c, explained line by line.
# =============================================================================
#
# HOW TO READ THIS FILE
# ---------------------
# This is the EXACT assembly clang emits for asm/demo.c at -O1 (see demo.s for
# the untouched original), with a comment on essentially every instruction.
# AT&T syntax throughout:
#
#     op   source, destination        # e.g.  movl $1, %eax   =>  eax = 1
#     %reg                             # a register
#     $imm                             # an immediate (literal) constant
#     N(%base,%index,scale)            # memory at [base + index*scale + N]
#     leaq N(%b,%i,s), %r              # r = b + i*s + N   (ADDRESS math, no load)
#
# Register widths are windows onto the SAME register:
#     rax(64) / eax(32) / ax(16) / al(8).  Writing a 32-bit name (eax) ALWAYS
#     zero-extends into the full 64-bit register — the compiler exploits this
#     constantly below to zero the top half for free.
#
# THE SYSTEM V AMD64 ABI CONTRACT (what every function here obeys)
# ---------------------------------------------------------------
#   integer/pointer ARGUMENTS, in order:  rdi, rsi, rdx, rcx, r8, r9  (then stack)
#   RETURN value:                         rax
#   CALLEE-SAVED (a function must preserve): rbx, rbp, r12, r13, r14, r15, rsp
#   CALLER-SAVED (scratch, may clobber):   rax, rcx, rdx, rsi, rdi, r8, r9, r10, r11
#   STACK: 16-byte aligned at each `call`; grows down; the 128-byte "red zone"
#          below rsp is free scratch for leaf functions.
# Both routines here are LEAF functions (they call nothing), so they never touch
# the stack for locals at all — every value lives in a caller-saved register.
#
# WHAT THESE TWO FUNCTIONS ARE
# ----------------------------
#   tinyfs_name_hash(const char *name /*rdi*/, u32 len /*esi*/) -> u32 /*eax*/
#       The dcache string hash tinyfs relies on when the VFS looks up a name in
#       one of our directories. A tight loop of shift/add/multiply.
#   tinyfs_next_ino(u32 *counter /*rdi*/) -> u32 /*eax*/
#       The reserved-value-skipping inode-number allocator. Its "if (res==0)
#       res++" guard is the star: watch clang turn a branch into a branchless
#       add-with-carry.
#
# TWO OPTIMIZATIONS TO NOTICE (called out in full at their line):
#   * multiply-by-11 with ZERO multiply instructions — two `leaq`s do it.
#   * skip-zero-on-wraparound with ZERO branches — one `cmp` + one `adc` do it.
# =============================================================================

	.file	"demo.c"
	.text                                   # executable code section

# =============================================================================
# tinyfs_name_hash — hash `len` bytes of `name` into a 32-bit dcache hash.
#
#   C:  u64 hash = 0;
#       while (len--) hash = partial_name_hash((unsigned char)*name++, hash);
#       return end_name_hash(hash);
#
#   partial_name_hash(c, h) = (h + (c<<4) + (c>>4)) * 11   [inlined into loop]
#   end_name_hash(h)        = (u32)h                        [just uses eax]
# =============================================================================
	.globl	tinyfs_name_hash
	.p2align	4                       # 16-byte align the entry for fetch
	.type	tinyfs_name_hash,@function
tinyfs_name_hash:                       # rdi = name, esi = len
# %bb.0:  ---- PROLOGUE + zero-length guard ----------------------------------
	pushq	%rbp                    # save caller's frame pointer (callee-saved).
	                                #   A leaf could skip this; -O1 keeps it so a
	                                #   debugger can still unwind. Costs 1 push.
	movq	%rsp, %rbp              # rbp = frame base for this call.
	testl	%esi, %esi              # set flags from len & len; ZF=1 iff len==0.
	je	.LBB0_4                 # if len==0, skip the loop and return 0.

# %bb.1:  ---- loop set-up (registers only; no stack spills) -----------------
	movl	%esi, %ecx              # ecx = len : the loop TRIP COUNT / bound.
	xorl	%edx, %edx              # rdx = 0   : the loop INDEX i (xor zeroes all
	                                #   64 bits — the idiomatic, 2-byte zero).
	xorl	%eax, %eax              # rax = 0   : the hash ACCUMULATOR (return reg).
	.p2align	4               # align the hot loop top to a 16-byte line.

.LBB0_2:                                # ---- LOOP TOP: one character per pass ----
	movzbl	(%rdi,%rdx), %esi       # esi = (unsigned char)name[i].  MOVZBL loads
	                                #   ONE byte from [name + i] and ZERO-extends
	                                #   it to 32 bits — this is the C cast to
	                                #   `unsigned char` made explicit, so 0x80..0xFF
	                                #   are 128..255, never sign-extended negatives.
	movl	%esi, %r8d              # r8d = c  (working copy for the << 4 term).
	shll	$4, %r8d                # r8d = c << 4.  32-bit shift is safe: c<=255 so
	                                #   c<<4 <= 4080, the top bits stay zero.
	addq	%rax, %r8               # r8  = (c<<4) + hash   (64-bit add of the accum).
	shrl	$4, %esi                # esi = c >> 4  (the low-nibble spread term).
	addq	%r8, %rsi               # rsi = hash + (c<<4) + (c>>4)  = the pre-multiply
	                                #   value. This is the (...) in partial_name_hash.
	# ---- multiply by 11 using TWO leaqs and NO `imul` ----------------------
	# The compiler factors 11 = 1 + 2*5.  leaq does base+index*scale in one uop,
	# so it is (ab)used here purely as arithmetic, not to compute an address:
	leaq	(%rsi,%rsi,4), %rax     # rax = rsi + rsi*4      = rsi * 5.
	leaq	(%rsi,%rax,2), %rax     # rax = rsi + (rsi*5)*2  = rsi * 11  = new hash.
	incq	%rdx                    # i++  (advance the index).
	cmpl	%edx, %ecx              # compare len, i  (sets flags from len - i).
	jne	.LBB0_2                 # loop while i != len.  `name++` is folded into
	                                #   the (%rdi,%rdx) addressing, so no pointer
	                                #   register is bumped separately.

# %bb.3:  ---- normal return: hash is already in eax --------------------------
	                                # end_name_hash((u32)hash) is free: the low 32
	                                #   bits we want are exactly eax. clang notes
	                                #   "kill: def $eax killed $rax" — the truncation.
	popq	%rbp                    # restore caller's frame pointer.
	retq                            # return; eax = hash.

.LBB0_4:                                # ---- len==0 fast path -----------------
	xorl	%eax, %eax              # return value = 0 (empty name hashes to 0).
	popq	%rbp                    # restore frame pointer.
	retq                            # return.
.Lfunc_end0:
	.size	tinyfs_name_hash, .Lfunc_end0-tinyfs_name_hash

# =============================================================================
# tinyfs_next_ino — hand out the next inode number, skipping the reserved 0.
#
#   C:  u32 res = *counter;
#       res++;
#       if (res == 0) res++;      // wrapped past UINT_MAX; 0 means "no inode"
#       *counter = res;
#       return res;
#
# The whole function is BRANCHLESS. clang recognizes "add 1, then add 1 more iff
# the result was 0" as add-with-carry and emits it with one cmp + one adc.
# =============================================================================
	.globl	tinyfs_next_ino
	.p2align	4
	.type	tinyfs_next_ino,@function
tinyfs_next_ino:                        # rdi = counter (pointer to the u32 cell)
# %bb.0:
	pushq	%rbp                    # prologue: save frame pointer (callee-saved).
	movq	%rsp, %rbp              # establish frame base.
	movl	(%rdi), %eax            # eax = *counter  = old value (res before ++).
	leal	1(%rax), %ecx           # ecx = old + 1   = the candidate result. leal is
	                                #   used as a 3-operand add that does NOT touch
	                                #   flags — we want the flags from the cmp below,
	                                #   not from this increment.
	cmpl	$1, %ecx                # compute (candidate - 1), setting CF (borrow).
	                                #   Unsigned: CF=1 iff candidate < 1 iff
	                                #   candidate == 0 — i.e. old was 0xFFFFFFFF and
	                                #   the ++ wrapped to 0. This is the "res==0" test.
	adcl	$1, %eax                # eax = old + 1 + CF.  ADD-WITH-CARRY folds BOTH
	                                #   increments into one op:
	                                #     normal case (CF=0): eax = old + 1
	                                #     wrap  case (CF=1):  eax = old + 1 + 1 = 1
	                                #   exactly reproducing "res++; if(!res) res++;".
	movl	%eax, (%rdi)            # *counter = res  (persist for the next call).
	                                #   clang notes the eax->rax kill: the u32 return
	                                #   lives in the low 32 bits, top half irrelevant.
	popq	%rbp                    # restore frame pointer.
	retq                            # return; eax = the freshly allocated inode number.
.Lfunc_end1:
	.size	tinyfs_next_ino, .Lfunc_end1-tinyfs_next_ino

	.ident	"clang version 20.1.8"          # toolchain stamp (harmless metadata)
	.section	".note.GNU-stack","",@progbits  # mark the stack NON-executable
	                                                #   (a security default the linker
	                                                #   records; no exec stack needed).
	.addrsig                                # address-significance table (LTO hint)
# =============================================================================
# WHAT TO TAKE AWAY
#   * Both routines are leaves: no stack locals, everything in registers, args
#     arrive in rdi/rsi per the SysV ABI and the result leaves in rax/eax.
#   * "* 11" compiled to ZERO multiply instructions — two `leaq`s (11 = 1+2*5).
#     Constant multiplies almost always become lea/shift/add chains; recognizing
#     that pattern in a disassembly is a core reverse-engineering skill.
#   * "if (res == 0) res++" compiled to ZERO branches — `cmp $1` sets the carry
#     exactly on wraparound and `adc $1` consumes it. The optimizer turned a
#     control-flow decision into straight-line data flow.
#   * Compare with demo.O0.s (every value spilled to the stack, partial_name_hash
#     a real `call`) to see the same C written the naive way, and demo.O2.s to
#     see clang UNROLL the hash loop two characters per iteration.
# =============================================================================
