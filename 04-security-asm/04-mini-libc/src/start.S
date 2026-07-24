# =============================================================================
# start.S — the process entry point `_start` for mini-libc.  ★ STAR ARTIFACT ★
# =============================================================================
#
# This is the very first instruction the CPU executes in our process. Not
# `main`: the kernel, after execve(2), knows nothing about `main` — it reads
# the ELF header's e_entry field and jumps there, and the linker fills e_entry
# from the symbol named `_start`. WE are that front door. Everything a hosted
# program assumes has already happened (the runtime is set up) is *our* job now.
#
# It is written in hand assembly, not C, for one unavoidable reason: at this
# instant there is NO valid C environment yet. In particular `rsp` points
# straight at raw kernel-supplied data (argc/argv/envp/auxv), and a C compiler
# would spill locals and set up a frame *before* we could read it, corrupting
# the very layout we need. Assembly lets us read the stack untouched, then hand
# clean C-ABI arguments to the first real C function (__libc_start_main).
#
# -----------------------------------------------------------------------------
# THE INITIAL PROCESS STACK (System V AMD64 ABI, Fig. 3.9). Higher addresses at
# the bottom. At the moment control reaches `_start`, `rsp` points at `argc`:
#
#     position        content                     size
#     ---------------------------------------------------------------
#     rsp + 0      -> argc                         8 bytes   (a long)
#     rsp + 8      -> argv[0]   (char *)           8         first program arg
#     rsp + 16     -> argv[1]                      8
#         ...
#     rsp + 8+8*argc-> argv[argc-1]                8
#                     NULL                         8         <- terminates argv
#     next         -> envp[0]   (char *)           8         first "KEY=value"
#         ...
#                     NULL                         8         <- terminates envp
#     next         -> auxv[0].a_type               8   \  each auxv entry is an
#                     auxv[0].a_val                8   /   Elf64_auxv_t (16 bytes)
#         ...
#                     AT_NULL (0)                  8         <- terminates auxv
#                     ...strings argv/envp point into, then the top of the stack
#
# So:  argc   is at   [rsp]
#      argv   is  &[rsp + 8]           (an array of argc pointers, then NULL)
#      envp   is  &[rsp + 8 + 8*(argc+1)]     i.e. argv + argc + 1
#      auxv   is  right after envp's NULL terminator (walked in C, see crt.c)
#
# TWO GUARANTEES the kernel makes on entry, both of which we rely on:
#   1. rsp is 16-byte aligned. The SysV ABI requires (rsp % 16 == 0) at the
#      point a `call` executes; we exploit that below.
#   2. rbp is not meaningful. The ABI *recommends* zeroing it to mark the
#      outermost frame so stack unwinders/backtraces terminate cleanly.
# =============================================================================

	.text
	.globl	_start                  # export _start so the linker can set e_entry
	.type	_start,@function
_start:
# ---- ABI hygiene ------------------------------------------------------------
	xor	%ebp, %ebp              # rbp = 0. Writing the 32-bit half zero-extends
	                                #   to clear all 64 bits (1 byte shorter than
	                                #   `xor %rbp,%rbp`). Marks the outermost stack
	                                #   frame: a backtrace that walks saved-rbp
	                                #   links stops when it hits this 0.

# ---- Marshal main()'s arguments straight off the initial stack --------------
# We are about to call:
#     void __libc_start_main(int (*main)(int,char**,char**), long argc, char **argv)
# whose SysV arg registers are rdi, rsi, rdx. Fill them from the stack IMAGE
# before we perturb rsp in any way.
	mov	(%rsp), %rsi            # rsi = *[rsp] = argc         (arg #2, a long).
	                                #   A plain 8-byte load; the kernel pushed argc
	                                #   as a full word.
	lea	8(%rsp), %rdx           # rdx = rsp + 8 = &argv[0]    (arg #3).
	                                #   LEA computes the ADDRESS; it does not read
	                                #   memory. argv is this pointer-to-pointer.
	lea	main(%rip), %rdi        # rdi = &main                 (arg #1).
	                                #   RIP-relative so it resolves correctly whether
	                                #   the image is PIE or fixed-load. `main` is an
	                                #   external symbol supplied by the linked program;
	                                #   the linker patches this displacement.

# ---- Realign the stack for the ABI ------------------------------------------
	and	$-16, %rsp              # force rsp down to a 16-byte boundary.
	                                #   $-16 = 0xFFFF_FFFF_FFFF_FFF0, so this clears
	                                #   the low 4 bits. The kernel already handed us a
	                                #   16-aligned rsp, so on a compliant loader this
	                                #   is a no-op — but doing it is free insurance: a
	                                #   misaligned rsp at a `call` crashes any callee
	                                #   that uses aligned SSE moves (movaps). We saved
	                                #   argc/argv into registers first, so discarding
	                                #   the low bits of rsp loses nothing.

# ---- Enter the C world ------------------------------------------------------
	call	__libc_start_main       # hand off to C. This function publishes
	                                #   `environ`, locates the auxv, calls main(),
	                                #   and finally invokes exit_group — so it NEVER
	                                #   returns. At the moment this `call` executes
	                                #   rsp % 16 == 0 (we just aligned it), satisfying
	                                #   the ABI; the `call` pushes the 8-byte return
	                                #   address, so inside the callee rsp % 16 == 8,
	                                #   exactly what a compiled prologue expects.

# ---- Unreachable safety net -------------------------------------------------
	ud2	                        # If __libc_start_main ever returned (it must not),
	                                #   there is no valid return address on this stack
	                                #   and "returning" would execute garbage. ud2 is
	                                #   the guaranteed-invalid opcode: it raises SIGILL
	                                #   and dies loudly instead of silently corrupting.
.Lstart_end:
	.size	_start, .Lstart_end - _start

# -----------------------------------------------------------------------------
# Mark the stack NON-EXECUTABLE (the NX/W^X security default).
#
# The linker unions the ".note.GNU-stack" flags of every input object. If ANY
# object omits this note or requests an executable stack ("x"), the linker makes
# the WHOLE program's stack executable — re-enabling classic "inject shellcode
# onto the stack and jump to it" attacks. Every hand-written .S must therefore
# emit this note with EMPTY flags ("") to vote for a non-exec stack. Cost to an
# attacker: with NX on, a stack buffer overflow can no longer run injected bytes;
# they must pivot to return-oriented programming (ROP) against existing code.
# -----------------------------------------------------------------------------
	.section	.note.GNU-stack,"",@progbits
