# =============================================================================
# demo.annotated.s — clang's -O1 output for demo.c, explained instruction by
#                    instruction. THIS is the payoff of the whole capstone: you
#                    can SEE computed-goto dispatch in the machine code.
# =============================================================================
#
# Source: asm/demo.c — a miniature stack bytecode VM (the dispatch core of
# src/vm.c) that computes sum(1..10) = 55. Regenerate the raw file with
# `make asm` (this annotated copy is never overwritten). AT&T syntax:
#
#     op   src, dst                       # movl $1,%eax  =>  eax = 1
#     %reg / $imm / sym(%rip) / N(%reg,%idx,scale)
#     movzbl = load a byte, ZERO-extend to 32 bits (top of rax cleared too)
#     movslq = load a dword, SIGN-extend to 64 bits (for int->index math)
#     movsbl/movsbq = load a byte, SIGN-extend (used for the signed rel8 operand)
#
# ------------------------- SysV AMD64 ABI (the contract) ---------------------
#   integer/pointer args:  rdi, rsi, rdx, rcx, r8, r9   (vm_run takes one: rdi)
#   return value:          rax
#   callee-saved:          rbx, rbp, r12-r15, rsp  (a function must preserve these)
#   caller-saved/scratch:  rax, rcx, rdx, rsi, rdi, r8-r11
#   the RED ZONE:          128 bytes below rsp a LEAF function may use without
#                          allocating. vm_run makes no calls (all control flow is
#                          computed goto), so it IS a leaf and leans on the red
#                          zone — see the frame note below.
#   16-byte stack alignment at every `call` (not exercised: no calls here).
#
# ------------------------- How this function uses registers ------------------
# The optimizer pinned the interpreter's hot variables into registers for the
# whole function; keeping these four in your head makes the rest read like C:
#
#     %rdi = code   — base pointer to the bytecode array (arg0). Never reused.
#     %eax = sp     — operand-stack index (0-based). Sign-extended when indexing.
#     %edx = ip     — instruction-pointer index into code[]. Carried between
#                     handlers ALREADY advanced past the opcode just dispatched.
#     %rcx = &table — base of the 9-entry address-of-label dispatch table.
#     %rsi,%r8,%r9  — scratch.
#
# ------------------------- Frame layout (red-zone leaf) ----------------------
# Two C arrays live in the frame, addressed at NEGATIVE offsets from rbp:
#     slot[8]  (i64 locals)         at  -64(%rbp)   .. -8(%rbp)
#     stack[32](i64 operand stack)  at -320(%rbp)   .. -72(%rbp)
# Every effective address stays within [rbp-320, rbp); the base displacements
# -328/-336 you see below are just `stack[sp-1]`/`stack[sp-2]` rewritten as
# (-320 - k*8) folded into the addressing mode, and are only used when sp>=1/2,
# so they never actually touch memory below rbp-320. `subq $192,%rsp` + the
# 128-byte red zone exactly covers rbp-320.
#
# ------------------------- The big idea: computed goto -----------------------
# A `switch`-based interpreter has ONE indirect branch that every opcode returns
# to, so the CPU can't predict "what follows op X". Here, every handler ends with
# its OWN copy of `jmpq *(%rcx,%rsi,8)` — the dispatch. N branch sites, one per
# opcode, each independently predicted. That single duplicated instruction is the
# entire performance argument for computed-goto dispatch, and you can count the
# copies below: one per handler block.
#
# Block -> opcode map (via the dispatch table `vm_run.table` at the bottom):
#   .LBB0_1=CONST  .LBB0_2=LOAD  .LBB0_3=STORE  .LBB0_4=ADD  .LBB0_5=SUB
#   .LBB0_6=LE     .LBB0_7=JMPF  .LBB0_10=JMP   .LBB0_11=RET
# =============================================================================

	.file	"demo.c"
	.text
	.globl	vm_run                          # export vm_run (called by main)
	.p2align	4                       # 16-byte align the entry for fetch
	.type	vm_run,@function
vm_run:                                 # i64 vm_run(const u8 *code)   code in %rdi

# ---- PROLOGUE ---------------------------------------------------------------
	pushq	%rbp                            # save caller's frame pointer
	movq	%rsp, %rbp                      # rbp = frame base
	subq	$192, %rsp                      # reserve locals (red zone covers the rest)

# ---- Zero slot[8] (the C: `for (k=0;k<8;k++) slot[k]=0;`) -------------------
# The optimizer unrolled the clear into four 16-byte SSE stores of zero.
	xorps	%xmm0, %xmm0                    # xmm0 = 0 (128 bits)
	movaps	%xmm0, -16(%rbp)                # slot[6..7] = 0
	movaps	%xmm0, -32(%rbp)                # slot[4..5] = 0
	movaps	%xmm0, -48(%rbp)                # slot[2..3] = 0
	movaps	%xmm0, -64(%rbp)                # slot[0..1] = 0   (slot base = -64)

# ---- Enter the dispatch loop: the FIRST NEXT(), specialized for ip==0 -------
# C: sp=0; ip=0; then NEXT() == goto *table[code[ip++]].
	xorl	%eax, %eax                      # sp = 0
	movl	$1, %edx                        # ip = 1  (post-increment of the ip=0 read)
	movzbl	(%rdi), %esi                    # esi = code[0]  (first opcode, zero-extended)
	leaq	vm_run.table(%rip), %rcx        # rcx = &dispatch table (RIP-relative)
	jmpq	*(%rcx,%rsi,8)                  # goto *table[code[0]]  — THE dispatch
	.p2align	4

# =========================== OP_CONST ========================================
# C: L_CONST: { stack[sp++] = (i64)code[ip++]; NEXT(); }
# On entry %edx=ip points at the imm8 operand.
.Ltmp0:                                 # Block address taken (table entry 0)
.LBB0_1:
	movslq	%edx, %rsi                      # rsi = ip
	movzbl	(%rdi,%rsi), %edx               # edx = code[ip] = the immediate value
	movslq	%eax, %r8                       # r8  = sp
	incl	%eax                            # sp++            (eax = sp+1)
	movq	%rdx, -320(%rbp,%r8,8)          # stack[sp] = value   (stack base = -320)
	leal	2(%rsi), %edx                   # ip = ip + 2  (skip operand, and the byte NEXT reads)
	addq	%rdi, %rsi                      # rsi = code + ip
	incq	%rsi                            # rsi = &code[ip+1]  (next opcode)
	movzbl	(%rsi), %esi                    # esi = code[ip+1]   (next opcode)
	jmpq	*(%rcx,%rsi,8)                  # NEXT(): dispatch (copy #1)
	.p2align	4

# =========================== OP_LOAD =========================================
# C: L_LOAD: { stack[sp++] = slot[code[ip++]]; NEXT(); }
.Ltmp1:                                 # table entry 1
.LBB0_2:
	movslq	%edx, %rsi                      # rsi = ip
	movzbl	(%rdi,%rsi), %edx               # edx = code[ip] = slot#
	movq	-64(%rbp,%rdx,8), %rdx          # rdx = slot[slot#]   (slot base = -64)
	movslq	%eax, %r8                       # r8  = sp
	incl	%eax                            # sp++
	movq	%rdx, -320(%rbp,%r8,8)          # stack[sp] = slot value
	leal	2(%rsi), %edx                   # ip += 2
	addq	%rdi, %rsi                      # rsi = code + ip
	incq	%rsi                            # &code[ip+1]
	movzbl	(%rsi), %esi                    # next opcode
	jmpq	*(%rcx,%rsi,8)                  # NEXT(): dispatch (copy #2)
	.p2align	4

# =========================== OP_STORE ========================================
# C: L_STORE: { slot[code[ip++]] = stack[--sp]; NEXT(); }
.Ltmp2:                                 # table entry 2
.LBB0_3:
	movslq	%eax, %rsi                      # rsi = sp
	decl	%eax                            # sp--            (eax = sp-1)
	movq	-328(%rbp,%rsi,8), %rsi         # rsi = stack[sp-1]  (value = old top)
	movslq	%edx, %r8                       # r8  = ip
	movzbl	(%rdi,%r8), %edx                # edx = code[ip] = slot#
	movq	%rsi, -64(%rbp,%rdx,8)          # slot[slot#] = value
	leal	2(%r8), %edx                    # ip += 2
	leaq	(%rdi,%r8), %rsi                # rsi = code + ip
	incq	%rsi                            # &code[ip+1]
	movzbl	(%rsi), %esi                    # next opcode
	jmpq	*(%rcx,%rsi,8)                  # NEXT(): dispatch (copy #3)
	.p2align	4

# =========================== OP_ADD ==========================================
# C: L_ADD: { b=stack[--sp]; a=stack[--sp]; stack[sp++]=a+b; NEXT(); }
# Net effect: pop 2, push 1 => sp decreases by 1, result lands at old stack[sp-2].
# No operand byte, so ip advances by only 1 across the NEXT().
.Ltmp3:                                 # table entry 3
.LBB0_4:
	movslq	%eax, %rsi                      # rsi = sp
	decl	%eax                            # sp-- (net for the whole op)
	movq	-328(%rbp,%rsi,8), %r8          # r8 = stack[sp-1] = b
	addq	%r8, -336(%rbp,%rsi,8)          # stack[sp-2] += b   (a = a + b, in place)
	movslq	%edx, %rsi                      # rsi = ip (points at next opcode already)
	incl	%edx                            # ip++    (carried ip for the NEXT read)
	addq	%rdi, %rsi                      # rsi = code + ip
	movzbl	(%rsi), %esi                    # next opcode = code[ip]
	jmpq	*(%rcx,%rsi,8)                  # NEXT(): dispatch (copy #4)
	.p2align	4

# =========================== OP_SUB ==========================================
# C: L_SUB: { b=stack[--sp]; a=stack[--sp]; stack[sp++]=a-b; NEXT(); }
# Identical shape to ADD but `subq` instead of `addq`.
.Ltmp4:                                 # table entry 4
.LBB0_5:
	movslq	%eax, %rsi                      # rsi = sp
	decl	%eax                            # sp--
	movq	-328(%rbp,%rsi,8), %r8          # r8 = stack[sp-1] = b
	subq	%r8, -336(%rbp,%rsi,8)          # stack[sp-2] -= b   (a = a - b)
	movslq	%edx, %rsi                      # rsi = ip
	incl	%edx                            # ip++
	addq	%rdi, %rsi                      # code + ip
	movzbl	(%rsi), %esi                    # next opcode
	jmpq	*(%rcx,%rsi,8)                  # NEXT(): dispatch (copy #5)
	.p2align	4

# =========================== OP_LE ===========================================
# C: L_LE: { b=stack[--sp]; a=stack[--sp]; stack[sp++]=(a<=b); NEXT(); }
# The comparison result is materialized as 0/1 with `setle` (no branch).
.Ltmp5:                                 # table entry 5
.LBB0_6:
	movslq	%eax, %rsi                      # rsi = sp
	decl	%eax                            # sp-- (pop 2 / push 1 => net -1)
	movq	-336(%rbp,%rsi,8), %r8          # r8 = stack[sp-2] = a
	xorl	%r9d, %r9d                      # r9 = 0  (result accumulator; also breaks dep)
	cmpq	-328(%rbp,%rsi,8), %r8          # flags = a - stack[sp-1]  (a - b)
	setle	%r9b                            # r9 = (a <= b) ? 1 : 0   (signed: ZF | SF!=OF)
	movq	%r9, -336(%rbp,%rsi,8)          # stack[sp-2] = result
	movslq	%edx, %rsi                      # rsi = ip
	incl	%edx                            # ip++  (no operand)
	addq	%rdi, %rsi                      # code + ip
	movzbl	(%rsi), %esi                    # next opcode
	jmpq	*(%rcx,%rsi,8)                  # NEXT(): dispatch (copy #6)
	.p2align	4

# =========================== OP_JMPF (conditional back/forward jump) ==========
# C: L_JMPF: { int rel=(signed char)code[ip++]; if (stack[--sp]==0) ip+=rel; NEXT(); }
# %edx=ip points at the signed rel8 operand on entry.
.Ltmp6:                                 # table entry 6
.LBB0_7:
	leal	1(%rdx), %esi                   # esi = ip+1  (the NOT-taken next ip)
	movslq	%eax, %r8                       # r8 = sp
	decl	%eax                            # sp-- (pop the condition)
	cmpq	$0, -328(%rbp,%r8,8)            # compare popped condition (stack[sp-1]) with 0
	jne	.LBB0_9                         # if condition != 0: do NOT jump (fall through)
# %bb.8:  (condition == 0: take the branch, add the signed displacement)
	movslq	%edx, %rdx                      # rdx = ip (index of rel operand)
	movsbl	(%rdi,%rdx), %edx               # edx = (signed char)code[ip] = rel  (SIGN-extended)
	addl	%edx, %esi                      # esi = (ip+1) + rel   = taken target ip
.LBB0_9:
	movslq	%esi, %rdx                      # rdx = new ip
	incl	%esi                            # esi = new ip + 1  (carried ip after NEXT read)
	addq	%rdi, %rdx                      # rdx = code + new ip
	movzbl	(%rdx), %r8d                    # r8d = code[new ip]  (next opcode)
	movl	%esi, %edx                      # edx = carried ip
	jmpq	*(%rcx,%r8,8)                  # NEXT(): dispatch (copy #7)
	.p2align	4

# =========================== OP_JMP (unconditional) ==========================
# C: L_JMP: { int rel=(signed char)code[ip++]; ip+=rel; NEXT(); }
# This is the loop back-edge (program byte 30 holds rel = -23, i.e. 0xE9).
.Ltmp7:                                 # table entry 7
.LBB0_10:
	movslq	%edx, %rdx                      # rdx = ip (index of rel operand)
	movsbq	(%rdi,%rdx), %rsi               # rsi = (signed char)code[ip] = rel (sign-extend to 64)
	leaq	(%rdx,%rsi), %r8                # r8  = ip + rel   (target ip, saved for the fetch)
	addq	%rsi, %rdx                      # rdx = ip + rel
	incq	%rdx                            # rdx = ip + rel + 1  (= new ip after `ip+=rel`,`ip++`)
	incl	%edx                            # edx = ip + rel + 2  (carried ip after NEXT read)
	leaq	(%rdi,%r8), %rsi                # rsi = code + (ip+rel)
	incq	%rsi                            # rsi = &code[ip+rel+1]  (next opcode)
	movzbl	(%rsi), %esi                    # esi = next opcode
	jmpq	*(%rcx,%rsi,8)                  # NEXT(): dispatch (copy #8)

# =========================== OP_RET =========================================
# C: L_RET: { return stack[--sp]; }  — leave the loop, return the top of stack.
.Ltmp8:                                 # table entry 8
.LBB0_11:
	cltq                                    # sign-extend eax(sp) into rax
	movq	-328(%rbp,%rax,8), %rax         # rax = stack[sp-1] = result (55)  -> return value
# ---- EPILOGUE ---------------------------------------------------------------
	addq	$192, %rsp                      # release the frame
	popq	%rbp                            # restore caller's frame pointer
	retq                                    # return to main with rax = 55
.Lfunc_end0:
	.size	vm_run, .Lfunc_end0-vm_run

# =============================================================================
# main — hands vm_run the program and forwards its result as the process's exit
# status. A textbook TAIL CALL: main has nothing to do after vm_run returns, so
# instead of `call vm_run; ret`, it restores rbp and `jmp`s — vm_run's `ret`
# returns straight to main's caller. (Its result 55 becomes `echo $?`.)
# =============================================================================
	.globl	main
	.p2align	4
	.type	main,@function
main:
	pushq	%rbp                            # (trivial frame; kept at -O1 for debuggability)
	movq	%rsp, %rbp
	leaq	main.prog(%rip), %rdi           # rdi = &prog  (arg0 to vm_run)
	popq	%rbp                            # tear the frame back down first...
	jmp	vm_run                          # ...then TAILCALL vm_run (its ret returns for us)
.Lfunc_end1:
	.size	main, .Lfunc_end1-main

# =============================================================================
# The dispatch table: 9 code addresses, one per opcode, indexed by the opcode
# byte. `.quad .LtmpN` emits the 64-bit address of the label taken with `&&` in
# C. It lives in .data.rel.ro (read-only after the dynamic loader applies
# relocations), because label addresses aren't known until link/load time.
# =============================================================================
	.type	vm_run.table,@object
	.section	.data.rel.ro,"aw",@progbits
	.p2align	4, 0x0
vm_run.table:
	.quad	.Ltmp0                          # table[0] = OP_CONST  handler
	.quad	.Ltmp1                          # table[1] = OP_LOAD
	.quad	.Ltmp2                          # table[2] = OP_STORE
	.quad	.Ltmp3                          # table[3] = OP_ADD
	.quad	.Ltmp4                          # table[4] = OP_SUB
	.quad	.Ltmp5                          # table[5] = OP_LE
	.quad	.Ltmp6                          # table[6] = OP_JMPF
	.quad	.Ltmp7                          # table[7] = OP_JMP
	.quad	.Ltmp8                          # table[8] = OP_RET
	.size	vm_run.table, 72

# =============================================================================
# The program bytes (34 of them). Decoded, this IS the source loop:
#   acc = 0; i = 1; while (i <= 10) { acc += i; i += 1; } return acc;   // = 55
#
#   off  bytes            meaning
#   0    00 00            CONST 0
#   2    02 00            STORE 0        ; slot0 (acc) = 0
#   4    00 01            CONST 1
#   6    02 01            STORE 1        ; slot1 (i)   = 1
#   8    01 01            LOAD 1         ; loop: push i          <-- back-edge target
#   10   00 0A            CONST 10       ; push 10
#   12   05               LE             ; push (i <= 10)
#   13   06 10            JMPF +16        ; if false -> off 31 (end)
#   15   01 00            LOAD 0         ; push acc
#   17   01 01            LOAD 1         ; push i
#   19   03               ADD            ; acc + i
#   20   02 00            STORE 0        ; acc = acc + i
#   22   01 01            LOAD 1         ; push i
#   24   00 01            CONST 1        ; push 1
#   26   03               ADD            ; i + 1
#   27   02 01            STORE 1        ; i = i + 1
#   29   07 E9            JMP -23         ; -> off 8 (loop)   (0xE9 = -23 signed)
#   31   01 00            LOAD 0         ; end: push acc
#   33   08               RET            ; return acc = 55
#
# The two displacements match the interpreter's semantics exactly: JMPF's +16 is
# measured from the byte AFTER its operand (15 + 16 = 31), and JMP's -23 likewise
# (31 - 23 = 8). This is the same backpatched-jump math the real compiler
# (src/compiler.c, emitJump/patchJump/emitLoop) performs for `while`/`for`.
# =============================================================================
	.type	main.prog,@object
	.section	.rodata,"a",@progbits
	.p2align	4, 0x0
main.prog:
	.ascii	"\000\000\002\000\000\001\002\001\001\001\000\n\005\006\020\001\000\001\001\003\002\000\001\001\000\001\003\002\001\007\351\001\000\b"
	.size	main.prog, 34

	.ident	"clang version 20.1.8"
	.section	".note.GNU-stack","",@progbits    # non-executable stack (security default)
	.addrsig
	.addrsig_sym vm_run
	.addrsig_sym main.prog
# =============================================================================
# WHAT TO TAKE AWAY
#   * Computed-goto dispatch = one `jmpq *table(...)` at the tail of EVERY opcode
#     handler (count them: 8 copies + the entry copy). That replication is the
#     whole speed argument — each site gets its own branch prediction history.
#   * The interpreter's hot state (ip, sp, the code pointer, the table base) all
#     lives in registers across the entire loop; memory is touched only for the
#     operand stack and locals. That register residency is why a good bytecode VM
#     is fast without a JIT — and the JIT (src/jit.c) removes even the dispatch.
#   * Signed rel8 branch operands (movsbl/movsbq) implement the language's
#     `while`/`for` back-edges — the exact bytes src/compiler.c backpatches.
#   * Compare with demo.O0.s (every C statement spilled to the stack, dispatch
#     through memory) and demo.O2.s to watch the optimizer tighten the loop.
# =============================================================================
