# ===========================================================================
# dots.s — a tiny, REAL Linux program masm can assemble end to end.
# ===========================================================================
# It prints ten '.' characters and exits 0. Assemble it with masm, link the
# resulting object with `ld`, and run it:
#
#     ../masm dots.s -o dots.o
#     ld dots.o -o dots            # ld supplies no libc; we only use syscalls
#     ./dots ; echo " exit=$?"     # -> .......... exit=0
#
# What it exercises (and why it is a good teaching sample):
#   * .text / .data / .globl and a global entry symbol `_start`
#   * mov $imm,%reg           -> C7 /0 encoding (imm fits in 32 bits)
#   * lea sym(%rip),%reg       -> forces a RELOCATION (R_X86_64_PC32 to `dot`),
#                                 because .text cannot know .data's final address
#   * sub $imm,%reg / cmp ...  -> group-1 imm8 encodings (83 /5, 83 /7)
#   * a FORWARD branch (je done) and a BACKWARD branch (jne loop), both to
#     LOCAL labels, so the assembler resolves them itself — no relocation.
#   * syscall                  -> the raw kernel entry (write, then exit)
# ===========================================================================

        .text
        .globl _start
_start:
        mov  $10, %rbx           # rbx = loop counter (10 dots to print)
        cmp  $0, %rbx            # already zero?
        je   done                # FORWARD reference -> needs the two passes

loop:
        # write(1, dot, 1)  — syscall number 1, args in rdi, rsi, rdx
        mov  $1, %rax            # SYS_write
        mov  $1, %rdi            # fd 1 = stdout
        lea  dot(%rip), %rsi     # buf = &dot  (RELOCATION: R_X86_64_PC32 -> dot)
        mov  $1, %rdx            # count = 1 byte
        syscall                  # trap into the kernel

        sub  $1, %rbx            # counter--
        cmp  $0, %rbx            # reached zero?
        jne  loop                # BACKWARD reference -> resolved locally

done:
        # exit(0)  — syscall number 60
        mov  $60, %rax           # SYS_exit
        mov  $0,  %rdi           # status 0
        syscall                  # process gone; never returns

        .data
dot:
        .byte 46                 # ASCII '.'  (0x2e)
