# =============================================================================
# demo.annotated.s — apply_relocations() from demo.c, explained instruction by
# instruction. This is the clang -O1 output in asm/demo.s, unchanged, with a
# comment on essentially every instruction plus the ABI header below. It is the
# beating heart of the dynamic linker: "compute S+A (or B+A), store to *P."
# =============================================================================
#
# HOW TO READ THIS FILE (AT&T syntax)
# -----------------------------------
#     op   src, dst                 # movq %rsi,%rax  =>  rax = rsi
#     %reg                          # a register;  $imm is a literal
#     N(%reg)                       # memory at [reg + N]
#     (%b,%i,%s)                    # memory at [b + i*s]   (s in {1,2,4,8})
#     N(%b,%i,%s)                   # memory at [b + i*s + N]
# eax is the low 32 bits of rax; writing eax zero-extends into rax, so clang
# uses 32-bit ops (movl/cmpl) whenever the value fits, to save code bytes.
#
# THE SYSTEM V AMD64 ABI (what this function is handed and must preserve)
# ----------------------------------------------------------------------
#   Integer/pointer arguments, in order:  rdi, rsi, rdx, rcx, r8, r9
#     apply_relocations(u64 base,           <- rdi
#                       const Elf64_Rela *rela,  <- rsi
#                       u64 count,          <- rdx
#                       const Elf64_Sym *symtab) <- rcx
#   Return value:            rax   (the `written` count)
#   Callee-saved (MUST be restored before ret): rbx, rbp, r12, r13, r14, r15.
#     This function uses all of rbx/r12-r15, so it pushes them in the prologue
#     and pops them in the epilogue.
#   Caller-saved (free to clobber): rax, rcx, rdx, rsi, rdi, r8-r11. Note the
#     `callq *%rax` for an IFUNC resolver may destroy every caller-saved reg,
#     which is exactly why symtab (in rcx) is spilled to the stack and reloaded.
#   The red zone: 128 bytes below rsp usable by a leaf without adjusting rsp.
#     This function is NOT a leaf (it can `call` a resolver), so it cannot use
#     the red zone across that call; it makes real stack room with `sub $24,rsp`.
#   Stack alignment: rsp % 16 == 0 at the point of a `call`.
#
# THE DATA LAYOUT the arithmetic below depends on:
#   Elf64_Rela is 24 bytes:  r_offset @0,  r_info @8,  r_addend @16.
#   Elf64_Sym  is 24 bytes:  st_name @0 .. st_shndx @6,  st_value @8, st_size@16.
#   r_info packs (symbol_index << 32) | type; so the LOW 32 bits are the type
#   (read cheaply with a 32-bit `cmpl %esi`), the HIGH 32 bits the symbol index
#   (extracted with `shrq $32`).
# =============================================================================

	.file	"demo.c"
	.text
	.globl	apply_relocations               # export the symbol
	.p2align	4                       # 16-byte align the entry for fetch
	.type	apply_relocations,@function
apply_relocations:                              # u64 apply_relocations(base, rela, count, symtab)

# ---- PROLOGUE: set up a frame and save the callee-saved registers we use ----
	pushq	%rbp                            # save caller's frame pointer
	movq	%rsp, %rbp                      # rbp = frame base (for debuggers)
	pushq	%r15                            # \
	pushq	%r14                            #  | preserve callee-saved regs we
	pushq	%r13                            #  | are about to repurpose as loop
	pushq	%r12                            #  | state (base, rela, count, cnt)
	pushq	%rbx                            # /
	subq	$24, %rsp                       # reserve 24 bytes for spills
	                                        #   (symtab, and a saved `written`)
	testq	%rdx, %rdx                      # count == 0 ?
	je	.LBB0_1                         #   yes -> return 0 (skip the loop)

# ---- LOOP SET-UP (count != 0): park the arguments in callee-saved regs -------
# %bb.2:
	movq	%rdx, %r14                      # r14 = count            (loop trip)
	movq	%rsi, %r12                      # r12 = rela             (cursor)
	movq	%rdi, %r13                      # r13 = base  (B)        (constant)
	addq	$16, %r12                       # r12 -> &rela[0].r_addend, so the
	                                        #   three fields are addressed as
	                                        #   -16(%r12)=r_offset, -8=r_info,
	                                        #   0=r_addend. One pointer, three
	                                        #   fields — a neat clang trick.
	xorl	%r15d, %r15d                    # r15 = written = 0
	                                        # implicit-def / kill %rax (no-op
	                                        #   bookkeeping the compiler emits)
	movq	%rcx, -56(%rbp)                 # SPILL symtab to the stack, so an
	                                        #   IFUNC `call` can't clobber it
	.p2align	4                       # align the hot loop top

# ============================ TOP OF THE LOOP ================================
# for (i = 0; i < count; i++)   — one relocation per iteration.
.LBB0_3:                                        # loop_top:
	movq	-16(%r12), %rbx                 # rbx = r_offset  (P = base + this)
	movq	-8(%r12), %rsi                  # rsi = r_info    (type | idx<<32)
	movq	(%r12), %rax                    # rax = r_addend  (A)
	cmpl	$5, %esi                        # compare TYPE (low 32 bits) with 5
	jle	.LBB0_4                         #   type <= 5  -> handle NONE/64/etc.

# ---- TYPE > 5: distinguish GLOB_DAT(6)/JUMP_SLOT(7)/RELATIVE(8)/IRELATIVE(37) -
# %bb.6:
	leal	-6(%rsi), %edx                  # edx = type - 6
	cmpl	$2, %edx                        # (type-6) < 2  <=>  type in {6,7}
	jb	.LBB0_13                        #   GLOB_DAT/JUMP_SLOT -> S+A path
# %bb.7:
	cmpl	$37, %esi                       # type == 37 (IRELATIVE) ?
	je	.LBB0_10                        #   -> call the resolver
# %bb.8:
	cmpl	$8, %esi                        # type == 8 (RELATIVE) ?
	jne	.LBB0_12                        #   no  -> unknown type: stop & return
# %bb.9:  ---- R_X86_64_RELATIVE:  *P = B + A ----
	addq	%r13, %rax                      # rax = base + A          (B + A)
	jmp	.LBB0_14                        #   -> store to *P

# ---- TYPE <= 5: only NONE(0) and R_X86_64_64(1) are meaningful here ----------
.LBB0_4:                                        # type_le5:
	testl	%esi, %esi                      # type == 0 (NONE) ?
	je	.LBB0_5                         #   -> skip (nothing to write)
# %bb.11:
	cmpl	$1, %esi                        # type == 1 (R_X86_64_64) ?
	jne	.LBB0_12                        #   no  -> unknown type: stop & return
	                                        #   yes -> fall into the S+A path

# ---- SYMBOLIC:  R_X86_64_64 / GLOB_DAT / JUMP_SLOT:  *P = S + A --------------
# S = resolve(base, symtab, idx) = base + symtab[idx].st_value.
.LBB0_13:                                       # do_symbolic:
	shrq	$32, %rsi                       # rsi = r_info >> 32 = symbol index
	leaq	(%rsi,%rsi,2), %rdx             # rdx = idx*3   (Elf64_Sym is 3 qwords)
	addq	%r13, %rax                      # rax = base + A
	addq	8(%rcx,%rdx,8), %rax            # rax += *(symtab + idx*24 + 8)
	                                        #      = symtab[idx].st_value
	                                        #  => rax = (base + st_value) + A
	                                        #        =  S + A          ✔
	jmp	.LBB0_14                        #   -> store to *P
	                                        # (rcx still holds symtab here: this
	                                        #  path made no call, so no reload.)

# ---- UNKNOWN TYPE: return the count accumulated so far -----------------------
.LBB0_12:                                       # unknown_type:
	movl	$1, %eax                        # action code 1 = "return" (bit0 set)
	movq	%r15, -48(%rbp)                 # save `written` for the epilogue
	jmp	.LBB0_15                        #   -> dispatch (will branch to return)

# ---- R_X86_64_NONE: skip this entry WITHOUT counting it ---------------------
.LBB0_5:                                        # type_none:
	movl	$4, %eax                        # action code 4 = "continue" (bit0=0),
	                                        #   and note we did NOT `inc %r15`
	jmp	.LBB0_15                        #   -> dispatch (advances the loop)

# ---- R_X86_64_IRELATIVE:  *P = ((u64(*)(void))(B + A))() --------------------
.LBB0_10:                                       # do_irelative:
	addq	%r13, %rax                      # rax = base + A = resolver address
	callq	*%rax                           # call it; ABI puts the result in rax.
	                                        #   This may clobber ALL caller-saved
	                                        #   regs, hence the reload next:
	movq	-56(%rbp), %rcx                 # RELOAD symtab (spilled earlier)
	.p2align	4

# ---- STORE:  *P = value  (the actual GOT/PLT slot write) --------------------
.LBB0_14:                                       # store_slot:
	movq	%rax, (%rbx,%r13)               # *(base + r_offset) = rax
	                                        #   i.e. *P = S+A / B+A / resolver().
	                                        #   THIS is "wire up the GOT/PLT".
	incq	%r15                            # written++
	xorl	%eax, %eax                      # action code 0 = "continue"

# ---- DISPATCH: decide continue vs. early-return, from the action code -------
.LBB0_15:                                       # dispatch:
	testb	$3, %al                         # low 2 bits set?  (only code 1 sets)
	jne	.LBB0_16                        #   set -> early return (unknown type)
# %bb.17:  ---- advance to the next relocation ----
	addq	$24, %r12                       # r12 += sizeof(Elf64_Rela) = 24
	decq	%r14                            # count--
	jne	.LBB0_3                         #   more? -> loop_top
	jmp	.LBB0_18                        #   done  -> return written

# ---- count == 0 entry: nothing to do ----------------------------------------
.LBB0_1:                                        # empty:
	xorl	%r15d, %r15d                    # written = 0
	jmp	.LBB0_18

# ---- restore `written` saved on the unknown-type path -----------------------
.LBB0_16:                                       # early_return:
	movq	-48(%rbp), %r15                 # written = saved value

# ---- EPILOGUE: return written, restore callee-saved regs, unwind frame -------
.LBB0_18:                                       # ret:
	movq	%r15, %rax                      # rax = written (the return value)
	addq	$24, %rsp                       # drop the spill area
	popq	%rbx                            # \
	popq	%r12                            #  | restore callee-saved registers
	popq	%r13                            #  | in reverse push order
	popq	%r14                            #  |
	popq	%r15                            # /
	popq	%rbp                            # restore caller's frame pointer
	retq                                    # return to caller; rax = written
.Lfunc_end0:
	.size	apply_relocations, .Lfunc_end0-apply_relocations

	.ident	"clang version 20.1.8"           # toolchain stamp (metadata)
	.section	".note.GNU-stack","",@progbits   # non-executable stack: a
	                                                 #   security default
# =============================================================================
# WHAT TO TAKE AWAY
#   * Every dynamic relocation reduces to one store: `*(base + r_offset) = V`,
#     where V is B+A (RELATIVE), S+A (64/GLOB_DAT/JUMP_SLOT), or a resolver's
#     return (IRELATIVE). Everything else a loader does is finding S.
#   * clang folded three type cases (64/GLOB_DAT/JUMP_SLOT) into ONE code path
#     because their arithmetic is identical — read the C and you'll see why.
#   * The `st_value` load uses `leaq (%rsi,%rsi,2)` to multiply the symbol index
#     by 3 (qwords) — strength reduction of `idx * sizeof(Elf64_Sym)`.
#   * symtab is spilled/reloaded around `callq *%rax` because an IFUNC resolver
#     is caller code that may trample rcx — an ABI-correctness detail you can
#     SEE here but would never notice in the C.
#   * Compare with demo.O0.s (every value spilled to the stack, one C statement
#     at a time) and demo.O2.s (the same logic, scheduled even tighter).
# =============================================================================
