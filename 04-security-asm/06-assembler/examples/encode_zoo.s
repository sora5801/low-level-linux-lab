# ===========================================================================
# encode_zoo.s — one of (almost) every instruction FORM masm understands.
# ===========================================================================
# This file is a correctness harness, not a runnable program: `make test`
# assembles it with masm AND with GNU `as`, then diffs the two .text byte
# streams. If masm's encoder is right, the bytes are identical. It deliberately
# stresses the ModR/M / SIB / REX corner cases (rsp & r12 force a SIB byte;
# rbp & r13 cannot use the zero-displacement encoding; r8..r15 need REX.B).
#
# It is intentionally BRANCH-FREE so the comparison is exact. masm always emits
# fixed-size rel32 branches, whereas GNU `as` performs "branch relaxation" and
# shrinks a nearby jump to a 2-byte rel8 form — a real difference we document
# rather than hide. Branch encoding and forward/backward label resolution are
# exercised (and round-tripped through objdump) by examples/dots.s instead.
# ===========================================================================
        .text
        .globl _start
_start:
        # --- mov reg,reg (REX.W + 89 /r) ---
        mov  %rax, %rbx
        mov  %r8,  %r15
        mov  %rsp, %rbp

        # --- mov imm,reg (C7 /0 for imm32; B8+ movabs for full imm64) ---
        mov  $1, %rax
        mov  $-1, %rcx
        mov  $0x7fffffff, %rdx
        mov  $0x1122334455667788, %r10

        # --- mov mem,reg  loads (8B /r) — the addressing-mode gauntlet ---
        mov  0(%rax), %rax
        mov  8(%rsp), %rbx           # rsp base => SIB byte required
        mov  -4(%rbp), %rcx          # rbp => disp8 form
        mov  (%rbp), %rdx            # rbp + 0 => must still be disp8=0
        mov  16(%r12), %rsi          # r12 => SIB + REX.B
        mov  (%r13), %rdi            # r13 + 0 => disp8=0 + REX.B

        # --- mov reg,mem  store (89 /r) ---
        mov  %rax, 24(%rdi)

        # --- lea (8D /r): address arithmetic, no memory touched ---
        lea  8(%rsp), %rax

        # --- add / sub / cmp ---
        add  %rax, %rbx
        add  $1, %rax                # imm8 (83 /0)
        add  $0x100, %rbx            # imm32 (81 /0)
        sub  %rcx, %rdx
        sub  $8, %rsp
        cmp  %rax, %rbx
        cmp  $0, %rdi

        # --- stack ---
        push %rax
        push %r8
        pop  %rbp
        pop  %r12

        # --- control flow: call to an EXTERNAL symbol (rel32 + PLT32 reloc in
        #     both assemblers, so the bytes still match), then leaf epilogue ---
        call ext_func
        ret
        syscall
        nop
