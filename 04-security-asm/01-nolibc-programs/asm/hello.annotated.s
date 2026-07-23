# =============================================================================
# hello.annotated.s — the compiler's output for hello.c, explained line by line.
# =============================================================================
#
# HOW TO READ THIS FILE
# ---------------------
# This is the exact assembly clang emits for hello.c at -O1 (see hello.s for the
# untouched original), with a comment on essentially every instruction. AT&T
# syntax is used throughout, which means:
#
#     op   source, destination          # e.g.  movl $1, %eax   =>  eax = 1
#     %reg                               # a register
#     $imm                               # an immediate (literal) constant
#     symbol(%rip)                       # RIP-relative address of `symbol`
#     N(%reg)                            # memory at [reg + N]
#
# Register-name sizes are the SAME register at different widths:
#     rax (64-bit) / eax (32-bit) / ax (16) / al (8).  Writing eax ZERO-EXTENDS
#     into rax, which is why the compiler uses `movl $1, %eax` (5 bytes) instead
#     of `movq $1, %rax` (7 bytes) whenever the top 32 bits should be zero.
#
# THE BIG PICTURE
# ---------------
# hello.c had five functions (syscall3, kstrlen, sys_write, sys_exit, _start).
# At -O1 the optimizer INLINED all of them into _start and even evaluated
# kstrlen("Hello...\n") at compile time to the constant 35. So the entire
# program collapses to: "set up a write syscall, set up an exit_group syscall."
# That collapse is the lesson: nothing in a nolibc program is magic.
# =============================================================================

	.file	"hello.c"
	.text                           # code goes in the .text section (executable)
	.globl	_start                  # export `_start` so the linker can find the
                                        #   ELF entry point (the process front door)
	.p2align	4               # align _start to a 16-byte boundary (2^4),
                                        #   which helps the CPU's instruction fetch
	.type	_start,@function        # mark the symbol as a function (for tools)
_start:                                 # <-- execution begins HERE after execve

# ---- PROLOGUE ---------------------------------------------------------------
# Establish a stack frame. Strictly unnecessary for this leaf that makes no
# nested `call`, but -O1 keeps the frame pointer for debuggability. It costs two
# instructions and makes a backtrace possible.
	pushq	%rbp                    # save the caller's frame pointer... except
                                        #   there is no caller: the kernel entered
                                        #   us. This pushes whatever rbp held (0)
                                        #   and moves rsp down 8 bytes.
	movq	%rsp, %rbp              # rbp = rsp: rbp now marks the base of our
                                        #   frame; locals would live below it.

# ---- SET UP THE write(2) SYSCALL: write(1, msg, 35) -------------------------
# Recall the syscall ABI: rax=number, then args in rdi, rsi, rdx, r10, r8, r9.
	leaq	_start.msg(%rip), %rsi  # rsi = &msg. LEA computes the ADDRESS of the
                                        #   string (RIP-relative, so it works at
                                        #   any load address) WITHOUT dereferencing
                                        #   it. This is syscall arg2 = buf.
	movl	$1, %eax                # rax = 1  = SYS_write (the syscall number)
	movl	$1, %edi                # rdi = 1  = fd 1 = stdout (syscall arg1)
	movl	$35, %edx               # rdx = 35 = length (syscall arg3). Note: the
                                        #   compiler folded kstrlen(msg) to 35 at
                                        #   COMPILE time — the loop vanished.
	#APP                             # <- clang marks the boundary of our inline asm
	syscall                         # trap into the kernel. The CPU switches to
                                        #   ring 0, runs sys_write, and on return
                                        #   rax holds the bytes written (35), while
                                        #   rcx and r11 are now CLOBBERED (the
                                        #   instruction stored return-RIP in rcx and
                                        #   RFLAGS in r11). We declared that clobber
                                        #   in C, so the compiler didn't rely on them.
	#NO_APP                          # <- end of inline asm

# ---- SET UP THE exit_group(2) SYSCALL: exit_group(7) ------------------------
	movl	$231, %eax              # rax = 231 = SYS_exit_group
	movl	$7, %edi                # rdi = 7   = the exit status ($? in the shell)
	xorl	%esi, %esi              # rsi = 0. `xor r,r` is the idiomatic, 2-byte
                                        #   way to zero a register (arg2 = 0). It is
                                        #   faster than `mov $0` and the CPU special-
                                        #   cases it as a dependency breaker.
	xorl	%edx, %edx              # rdx = 0 (arg3 = 0). exit_group ignores these,
                                        #   but our syscall3() wrapper always passes
                                        #   three args, so the compiler zeroes them.
	#APP
	syscall                         # trap again: the kernel tears down the process.
                                        #   This NEVER returns — there is deliberately
                                        #   no `ret`, no epilogue, no `pop %rbp`. The
                                        #   `noreturn` in C told the compiler the code
                                        #   below is unreachable, so it emitted none.
	#NO_APP
.Lfunc_end0:
	.size	_start, .Lfunc_end0-_start   # record the byte length of _start (for
                                             #   readelf/objdump symbol sizes)

# ---- READ-ONLY DATA: the string literal -------------------------------------
	.type	_start.msg,@object
	.section	.rodata,"a",@progbits   # .rodata = read-only data; "a" = allocated
                                                #   (loaded into memory), not writable.
	.p2align	4, 0x0                  # 16-byte align, pad with zero bytes
_start.msg:
	.asciz	"Hello from a program with no libc!\n"   # .asciz = bytes + a trailing
                                                         #   NUL (0x00). That NUL is what
                                                         #   kstrlen would have stopped on,
                                                         #   though here the length was
                                                         #   constant-folded to 35.
	.size	_start.msg, 36                  # 35 visible chars + 1 NUL = 36 bytes

	.ident	"clang version 20.1.8"          # toolchain stamp (harmless metadata)
	.section	".note.GNU-stack","",@progbits  # declares we do NOT need an
                                                        #   executable stack — a security
                                                        #   default the linker records.
# =============================================================================
# WHAT TO TAKE AWAY
#   * A Linux process starts at `_start`, not `main`. `main` is a libc idea.
#   * A syscall is just: load rax + args, execute `syscall`, read rax. No magic.
#   * The optimizer inlined and constant-folded five functions into ~12
#     instructions. Reading asm is how you SEE that happen — always keep it open.
#   * Compare this file with hello.O0.s to watch the same program written the
#     naive way (every function a real call, every value spilled to the stack).
# =============================================================================
