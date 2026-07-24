# =============================================================================
# demo.annotated.s — clang -O1 output for asm/demo.c, explained instruction by
#                     instruction. This is the COMPUTED-GOTO dispatch core: the
#                     threaded interpreter loop that makes a bytecode VM fast.
# =============================================================================
#
# HOW TO READ THIS FILE
# ---------------------
# This is the EXACT assembly clang 20 emits for asm/demo.c at -O1 (see demo.s
# for the untouched original), with a comment on essentially every instruction.
# AT&T syntax throughout:  op  src, dst   (destination LAST). `%reg` is a
# register, `$imm` an immediate, `N(%base,%index,scale)` is memory at
# base+index*scale+N. Register widths are the same register: rax(64)/eax(32)/
# ax(16)/al(8); writing eax zero-extends into rax.
#
# THE SYSTEM V AMD64 ABI (the contract every function here obeys)
# --------------------------------------------------------------
#   integer/pointer args, in order:  rdi, rsi, rdx, rcx, r8, r9, then the STACK
#   return value:                    rax
#   callee-saved (must preserve):    rbx, rbp, r12, r13, r14, r15
#   caller-saved (free to clobber):  rax, rcx, rdx, rsi, rdi, r8, r9, r10, r11
#   the "red zone":                  128 bytes below rsp a LEAF may use freely
#   stack alignment:                 rsp % 16 == 0 at the point of any `call`
#
# vm_run has ONE parameter, so it arrives in rdi:
#     rdi = code  (const u8 *)      ->  the bytecode to execute
#     rax = return (i64)            ->  the final stack top
#
# HOW COMPUTED-GOTO DISPATCH LOOKS IN ASM (the whole point)
# ---------------------------------------------------------
# clang pins the two hot pointers in registers for the entire loop:
#     %rcx = ip  (the instruction pointer, walking the bytecode)
#     %rdx = sp  (the value-stack pointer: points at the NEXT-FREE slot)
#     %rax = the base address of the dispatch table (hoisted once)
# The dispatch itself is three instructions, REPLICATED at the tail of every
# handler:
#     movzbl (%rcx), %esi          # fetch the next opcode byte, zero-extend
#     incq   %rcx                  # ip++  (past that opcode)
#     jmpq   *(%rax,%rsi,8)        # INDIRECT JUMP through table[opcode]
# Because that indirect `jmp` appears at MANY sites (not one shared `switch`),
# each site keeps its own branch-predictor history — "what usually follows an
# ADD" is predicted independently from "what follows a MUL". That per-site
# prediction is the 15-25% the technique buys.
#
# THE POINTER INVARIANT clang maintains
# -------------------------------------
# At every `jmpq *table`, %rcx (ip) points at the byte IMMEDIATELY AFTER the
# opcode being dispatched. For a zero-operand op that "after" byte is the next
# opcode, so its handler just loads it and bumps rcx. For OP_PUSH the "after"
# byte is the 1-byte operand, so its handler must ALSO step over the operand —
# which is exactly the extra `leaq 1(%rcx),...` / `addq $2,%rcx` you see below.
# =============================================================================

	.file	"demo.c"
	.text
	.globl	vm_run                          # export vm_run for the linker
	.p2align	4                       # 16-byte align the entry (I-fetch)
	.type	vm_run,@function
vm_run:                                         # i64 vm_run(const u8 *code)

# ---- PROLOGUE + LOOP SETUP --------------------------------------------------
# vm_run makes NO calls (every control transfer is a goto or the final ret), so
# it is effectively a leaf and may use the red zone. That is why the frame math
# below looks "short": the value stack is 512 bytes (i64 stack[64]) but clang
# reserves only 384 explicitly and lets the deepest 128 bytes live in the red
# zone under rsp.
	pushq	%rbp                    # save caller's frame pointer (kept at -O1
	                                #   for debuggability, not strictly needed)
	movq	%rsp, %rbp             # rbp = frame base
	subq	$384, %rsp              # reserve 384 bytes; the other 128 bytes of
	                                #   stack[64] sit in the 128-byte red zone.
	leaq	1(%rdi), %rcx           # rcx = code + 1  == ip, PRE-advanced past the
	                                #   first opcode (which we fetch just below).
	leaq	-512(%rbp), %rdx        # rdx = &stack[0] == sp (next-free slot). The
	                                #   stack grows UPWARD (addq $8) toward rbp.
	movzbl	(%rdi), %esi           # esi = code[0], the FIRST opcode, zero-extended
	                                #   from a byte to a 32-bit index.
	leaq	vm_run.table(%rip), %rax# rax = &dispatch table (RIP-relative, so it
	                                #   works at any load address). Hoisted ONCE;
	                                #   reused by every dispatch below.
	jmpq	*(%rax,%rsi,8)          # goto *table[opcode0]. *8 because each table
	                                #   entry is an 8-byte code pointer. This is
	                                #   the loop's first dispatch.

# =============================================================================
# HANDLERS. Each ends with the 3-instruction dispatch tail. The labels clang
# generated are .LBB0_n / .Ltmp_n; the table (bottom of file) maps opcode ->
# handler, so the meaningful names are noted in comments.
# =============================================================================

	.p2align	4
.Ltmp0:                                 # Block address taken (table[OP_PUSH])
.LBB0_1:                                # ---- do_push:  *sp++ = (i8)*ip ; ip++
	leaq	1(%rcx), %rsi           # rsi = ip + 1 == address of the NEXT opcode
	                                #   (the byte after this PUSH's operand).
	movsbq	(%rcx), %rdi           # rdi = sign-extend the 1-byte operand at ip.
	                                #   movsbq: move-sign-byte-to-quad, so a small
	                                #   negative immediate stays negative.
	movq	%rdi, (%rdx)           # *sp = operand           (the push: store...)
	addq	$8, %rdx               # sp++   (one 8-byte slot) (...then bump)
	addq	$2, %rcx               # ip += 2: step over BOTH the operand and the
	                                #   next opcode, restoring the invariant that
	                                #   rcx points just past the opcode we are
	                                #   about to dispatch (loaded via rsi next).
	movzbl	(%rsi), %esi           # esi = next opcode (from ip+1 saved in rsi)
	jmpq	*(%rax,%rsi,8)          # DISPATCH: goto *table[next opcode]

	.p2align	4
.Ltmp1:                                 # Block address taken (table[OP_ADD])
.LBB0_2:                                # ---- do_add:  b=*--sp; a=*--sp; *sp++ = a+b
	movq	-8(%rdx), %rsi          # rsi = sp[-1] == b (the current top)
	addq	%rsi, -16(%rdx)         # sp[-2] += b   i.e. a = a + b, WRITTEN IN PLACE
	                                #   into a's slot. clang fuses "pop b, pop a,
	                                #   push a+b" into one add + one pointer step.
	addq	$-8, %rdx              # sp -= 1 slot: two pops + one push == net -1.
	movzbl	(%rcx), %esi           # fetch next opcode ...
	incq	%rcx                    # ip++ ...
	jmpq	*(%rax,%rsi,8)          # ... DISPATCH.

	.p2align	4
.Ltmp2:                                 # Block address taken (table[OP_SUB])
.LBB0_3:                                # ---- do_sub:  b=*--sp; a=*--sp; *sp++ = a-b
	movq	-8(%rdx), %rsi          # rsi = b (top)
	subq	%rsi, -16(%rdx)         # a = a - b, in place at sp[-2]
	addq	$-8, %rdx              # sp -= 1
	movzbl	(%rcx), %esi           # next opcode ...
	incq	%rcx                    # ip++ ...
	jmpq	*(%rax,%rsi,8)          # ... DISPATCH.

	.p2align	4
.Ltmp3:                                 # Block address taken (table[OP_MUL])
.LBB0_4:                                # ---- do_mul:  b=*--sp; a=*--sp; *sp++ = a*b
	movq	-16(%rdx), %rsi         # rsi = a (sp[-2])
	imulq	-8(%rdx), %rsi          # rsi = a * b   (signed multiply; low 64 bits)
	movq	%rsi, -16(%rdx)         # store product into a's slot
	addq	$-8, %rdx              # sp -= 1
	movzbl	(%rcx), %esi           # next opcode ...
	incq	%rcx                    # ip++ ...
	jmpq	*(%rax,%rsi,8)          # ... DISPATCH.

	.p2align	4
.Ltmp4:                                 # Block address taken (table[OP_NEG])
.LBB0_5:                                # ---- do_neg:  a=*--sp; *sp++ = -a
	negq	-8(%rdx)               # negate the top IN PLACE. pop-then-push of the
	                                #   same slot is a no-op on sp, so sp is
	                                #   unchanged — clang elided the pointer math.
	movzbl	(%rcx), %esi           # next opcode ...
	incq	%rcx                    # ip++ ...
	jmpq	*(%rax,%rsi,8)          # ... DISPATCH.

.Ltmp5:                                 # Block address taken (table[OP_HALT])
.LBB0_6:                                # ---- do_halt:  return sp[-1]
	movq	-8(%rdx), %rax          # rax = sp[-1] == the result (ABI return reg)
	addq	$384, %rsp             # EPILOGUE: release the reserved frame
	popq	%rbp                    # restore caller's frame pointer
	retq                            # return to caller (demo_run), result in rax
.Lfunc_end0:
	.size	vm_run, .Lfunc_end0-vm_run

# =============================================================================
# demo_run — builds the program for (2+3)*4-1 and runs it. At -O1 clang does NOT
# fold this to `mov $19` (contrast the regex-engine demo, which folded fully):
# the answer only exists AFTER threading data through vm_run's INDIRECT jumps,
# and an indirect jump whose target is a runtime table load is opaque to the
# constant folder. So demo_run stays a real (tail) call. That opacity is the
# same property that makes the branch predictor, not the compiler, responsible
# for dispatch speed — the lesson of this whole file.
# =============================================================================
	.globl	demo_run
	.p2align	4
	.type	demo_run,@function
demo_run:                                       # int demo_run(void)
	pushq	%rbp                    # PROLOGUE (frame kept at -O1)
	movq	%rsp, %rbp
	leaq	demo_run.program(%rip), %rdi   # rdi = &program == arg0 for vm_run
	popq	%rbp                    # EPILOGUE done BEFORE the call so the call
	                                #   can be a tail jump (no return here).
	jmp	vm_run                          # TAIL CALL: reuse our return address;
	                                        #   vm_run's `ret` returns to OUR caller.
.Lfunc_end1:
	.size	demo_run, .Lfunc_end1-demo_run

# ---- THE DISPATCH TABLE -----------------------------------------------------
# Six 8-byte code pointers, one per opcode, indexed by the opcode value. It goes
# in .data.rel.ro ("relocatable read-only"): the addresses need load-time
# relocation (they are absolute label addresses), but are read-only thereafter.
	.type	vm_run.table,@object
	.section	.data.rel.ro,"aw",@progbits
	.p2align	4, 0x0
vm_run.table:
	.quad	.Ltmp0                  # table[0] = OP_PUSH -> do_push
	.quad	.Ltmp1                  # table[1] = OP_ADD  -> do_add
	.quad	.Ltmp2                  # table[2] = OP_SUB  -> do_sub
	.quad	.Ltmp3                  # table[3] = OP_MUL  -> do_mul
	.quad	.Ltmp4                  # table[4] = OP_NEG  -> do_neg
	.quad	.Ltmp5                  # table[5] = OP_HALT -> do_halt
	.size	vm_run.table, 48        # 6 entries * 8 bytes

# ---- THE PROGRAM BYTES ------------------------------------------------------
# The hand-assembled bytecode, in .rodata. Read it as opcode/operand pairs:
#   00 02  PUSH 2   00 03  PUSH 3   01  ADD   00 04  PUSH 4   03  MUL
#   00 01  PUSH 1   02  SUB   05  HALT      => (2+3)*4-1 = 19
	.type	demo_run.program,@object
	.section	.rodata,"a",@progbits
demo_run.program:
	.ascii	"\000\002\000\003\001\000\004\003\000\001\002\005"
	.size	demo_run.program, 12

	.ident	"clang version 20.1.8"
	.section	".note.GNU-stack","",@progbits  # non-executable stack (security)
	.addrsig                                # address-significance table (LTO aid)
	.addrsig_sym vm_run
	.addrsig_sym demo_run.program

# =============================================================================
# WHAT TO TAKE AWAY
#   * DISPATCH IS THE PRODUCT. The three-instruction "movzbl / inc / jmp *table"
#     tail, replicated at every handler, IS threaded dispatch. Each copy of the
#     indirect jump gets its own branch-predictor entry — that is the speedup.
#   * THE STACK MACHINE VANISHES INTO POINTER MATH. push == "store, add 8 to sp";
#     pop == "sub 8 from sp, load". clang even fuses pop+pop+push of a binary op
#     into one in-place `add`/`sub`/`imul` plus a single `addq $-8` on sp.
#   * TWO REGISTERS RUN THE WHOLE VM. ip in %rcx, sp in %rdx, live across every
#     handler; the table base in %rax is hoisted once. No memory-resident
#     interpreter state in the hot path.
#   * LEAF => RED ZONE. Because vm_run calls nothing, 128 bytes of its 512-byte
#     value stack live in the red zone, shrinking the explicit `sub $384,%rsp`.
#   * COMPARE THE THREE LEVELS:
#       - demo.O0.s  spills ip and sp to memory and reloads them every step; the
#         dispatch is still a `jmp *table`, but wrapped in loads/stores — the
#         clearest line-by-line mapping of the C.
#       - demo.s (this file, -O1) pins ip/sp in registers: the tight version.
#       - demo.O2.s drops the frame pointer entirely (rbp is a general register)
#         and threads TWO stack pointers (sp and sp-8) to shave an address
#         calc; demo_run STILL is not constant-folded, because the indirect jump
#         hides the program from the optimizer.
# =============================================================================
