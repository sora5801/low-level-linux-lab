# =============================================================================
# demo.annotated.s — clang's -O1 output for asm/demo.c, explained line by line.
# =============================================================================
#
# WHAT YOU ARE LOOKING AT
# -----------------------
# This is the exact assembly clang emits for asm/demo.c at -O1 (the untouched
# copy is demo.s), with a comment on essentially every instruction. asm/demo.c
# is the *userspace* syscall wrapper for this project — the kernel half
# (kernel/sys_hello.c) can't be compiled standalone because it needs the Linux
# in-tree headers, so per the lab convention we annotate the standalone core.
# What that core demonstrates is the entire point of "add a syscall": how a call
# number and its arguments are marshalled and handed to the CPU's `syscall`
# instruction.
#
# AT&T SYNTAX REMINDER (used throughout)
#     op   source, destination        # movl $1, %eax   =>  eax = 1
#     %reg                             # a register            $imm  # a literal
#     N(%reg)                          # memory at [reg + N]
# Writing a 32-bit register (%eax, %edx, ...) ZERO-EXTENDS into the 64-bit one,
# so `movl $463, %eax` (5 bytes) also clears the top 32 bits of %rax — cheaper
# than a full 64-bit `movq`.
#
# THE TWO ABIs THIS FILE STRADDLES  (this is the whole lesson)
# -----------------------------------------------------------
# SysV AMD64 *function* call convention (how C passes args to a normal `call`):
#     args:        rdi, rsi, rdx, RCX, r8, r9        return: rax
#     callee-saved (a function must preserve): rbx, rbp, r12, r13, r14, r15
#     caller-saved (scratch): rax, rcx, rdx, rsi, rdi, r8, r9, r10, r11
#
# Linux x86-64 *syscall* convention (how the kernel entry reads args):
#     number:      rax
#     args:        rdi, rsi, rdx, R10, r8, r9        return: rax
#     the `syscall` instruction itself DESTROYS rcx (<- return RIP) and r11
#     (<- saved RFLAGS). That clobber of rcx is the reason arg4 moves to r10:
#     rcx is not available across `syscall`, so the kernel ABI can't use it.
#
# Both functions below are LEAF functions that keep a frame pointer only because
# -O1 does so for debuggability; -O2 drops it (see demo.O2.s). No `call` is made
# from either — the `syscall` is not a `call` — so no stack alignment work or
# spilling to callee-saved registers is needed.
# =============================================================================

	.file	"demo.c"
	.text                           # executable code section
	.globl	invoke_hello            # export invoke_hello (extern linkage)
	.p2align	4               # 16-byte align the function entry (2^4)
	.type	invoke_hello,@function

# -----------------------------------------------------------------------------
# sword invoke_hello(char *buf, uword len)
#     return raw_syscall3(__NR_hello, buf, len, 0);
#
# On entry (SysV function ABI):  rdi = buf,  rsi = len.
# The `hello` syscall's own signature is hello(buf, len), i.e. syscall arg1=buf,
# arg2=len. Those are ALREADY the syscall argument registers (rdi, rsi), so the
# compiler barely has to move anything: set the number, zero the unused arg3,
# and trap. raw_syscall3 was `static inline`, so it vanished — its body is
# spliced right in here. This is the clearest possible picture of a syscall.
# -----------------------------------------------------------------------------
invoke_hello:                           # @invoke_hello
# %bb.0:
	pushq	%rbp                    #  PROLOGUE: save caller's frame pointer.
	movq	%rsp, %rbp              #  rbp = rsp: establish this frame's base.
	movl	$463, %eax              #  rax = 463 = __NR_hello. THIS is "the __NR
                                        #    argument in rax" — the syscall number
                                        #    the kernel will index sys_call_table
                                        #    with. buf (rdi) and len (rsi) are
                                        #    already in place from the C args, so
                                        #    they need no move at all.
	xorl	%edx, %edx              #  rdx = 0 = syscall arg3 (unused by hello).
                                        #    `xor r,r` is the 2-byte idiom to zero
                                        #    a register and breaks the dep chain.
	#APP                            #  <- clang marks the start of our inline asm
	syscall                         #  TRAP to ring 0. CPU jumps to
                                        #    entry_SYSCALL_64, which calls
                                        #    sys_call_table[463] = __x64_sys_hello.
                                        #    On return: rax = kernel's result
                                        #    (byte count, or -errno); rcx and r11
                                        #    are now CLOBBERED by the instruction.
	#NO_APP                         #  <- end of inline asm
	popq	%rbp                    #  EPILOGUE: restore caller's frame pointer.
	retq                            #  return; rax (from syscall) is the C return.
.Lfunc_end0:
	.size	invoke_hello, .Lfunc_end0-invoke_hello

# -----------------------------------------------------------------------------
	.globl	hello_or_errno
	.p2align	4
	.type	hello_or_errno,@function
# sword hello_or_errno(char *buf, uword len, int *err_out)
#     r = raw_syscall3(__NR_hello, buf, len, 0);
#     if (r < 0 && r >= -4095) { *err_out = -r; return -1; }
#     *err_out = 0; return r;
#
# On entry:  rdi = buf,  rsi = len,  rdx = err_out.
# There is a register COLLISION here worth savouring: err_out arrives in rdx,
# but the syscall needs rdx = arg3 = 0. So the compiler must relocate err_out
# out of the way FIRST. Then, after the syscall, it computes the errno decode
# BRANCHLESSLY: clang recognised my two-sided signed test `r<0 && r>=-4095` as
# the classic glibc idiom "(unsigned long)r >= (unsigned long)-4095" and emitted
# a single unsigned compare plus two `cmov`s — no conditional jumps at all.
# -----------------------------------------------------------------------------
hello_or_errno:                         # @hello_or_errno
# %bb.0:
	pushq	%rbp                    #  PROLOGUE: save caller's frame pointer.
	movq	%rsp, %rbp              #  establish frame base.
	movq	%rdx, %r8               #  r8 = err_out. MOVE the pointer out of rdx,
                                        #    because rdx is about to become syscall
                                        #    arg3. r8 is caller-saved scratch and
                                        #    is untouched by `syscall`, so err_out
                                        #    survives the trap here.
	xorl	%r9d, %r9d              #  r9 = 0. Pre-stage the "no error" errno
                                        #    value (0) so the cmov below can select
                                        #    it without a branch.
	movl	$463, %eax              #  rax = 463 = __NR_hello (the call number).
	xorl	%edx, %edx              #  rdx = 0 = syscall arg3 (unused). buf (rdi)
                                        #    and len (rsi) already sit in place.
	#APP
	syscall                         #  TRAP. Returns rax = r (byte count or
                                        #    -errno). rcx/r11 clobbered as always.
	#NO_APP

# ---- branchless decode of the kernel's [-4095,-1] error band ----------------
	movl	%eax, %ecx              #  ecx = (int)r. Take the low 32 bits as a
                                        #    candidate errno source (errnos are
                                        #    small, so 32 bits suffice).
	negl	%ecx                    #  ecx = -r  -> the POSITIVE errno we would
                                        #    report IF this turns out to be an error.
	cmpq	$-4095, %rax            #  compare r against -4095, setting flags for
                                        #    an UNSIGNED test. As unsigned, r in
                                        #    [-4095,-1] is >= (unsigned)-4095, while
                                        #    any r >= 0 (or r < -4095) is < it.
                                        #    This single compare replaces the whole
                                        #    `r < 0 && r >= -4095`.
	cmovbl	%r9d, %ecx              #  if BELOW (unsigned r < -4095, i.e. NOT an
                                        #    error): ecx = r9d = 0. So *err_out
                                        #    becomes 0 on the success path.
	movq	$-1, %rdx               #  rdx = -1: pre-stage the error return value.
	cmovaeq	%rdx, %rax              #  if ABOVE-OR-EQUAL (error band): rax = -1.
                                        #    Otherwise rax keeps r (the byte count).
                                        #    Two cmovs, zero branches — the flags
                                        #    from the one `cmpq` drive both.
	movl	%ecx, (%r8)             #  *err_out = ecx  (the errno, or 0). Stored
                                        #    through the pointer we parked in r8.
	popq	%rbp                    #  EPILOGUE: restore frame pointer.
	retq                            #  return rax: -1 on error, else the count.
.Lfunc_end1:
	.size	hello_or_errno, .Lfunc_end1-hello_or_errno

	.ident	"clang version 20.1.8"          # toolchain stamp (harmless metadata)
	.section	".note.GNU-stack","",@progbits  # mark stack non-executable
	.addrsig                                # address-significance table (LTO hint)
# =============================================================================
# WHAT TO TAKE AWAY
#   * A syscall is not a function call: number in %rax, args in rdi/rsi/rdx/R10/
#     r8/r9, then the `syscall` instruction — which destroys %rcx and %r11.
#   * invoke_hello shows the number 463 (__NR_hello) landing in %rax literally.
#     Because hello's args (buf,len) already match rdi,rsi, the wrapper is almost
#     nothing — the raw instruction is the whole story.
#   * hello_or_errno shows two real lessons: (1) err_out had to be moved out of
#     rdx before rdx became syscall arg3 (register pressure at the ABI seam), and
#     (2) clang turned the [-4095,-1] errno test into ONE unsigned `cmpq` plus
#     two `cmov`s — the exact branchless idiom glibc's syscall() wrapper uses.
#   * Compare with demo.O0.s (raw_syscall3 is a real `call`, everything spilled
#     to the stack) and demo.O2.s (frame pointer dropped: invoke_hello becomes
#     just `mov $463,%eax; xor %edx,%edx; syscall; ret`).
# =============================================================================
