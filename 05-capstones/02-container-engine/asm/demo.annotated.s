# =============================================================================
# demo.annotated.s — clang's -O1 output for demo.c, explained instruction by
# instruction. The clone-flags + capability bitmask composition, as real
# x86-64 machine code.
# =============================================================================
#
# SOURCE OF TRUTH: this is a hand-annotated copy of asm/demo.s (clang 20, -O1,
# --target=x86_64-pc-linux-gnu, -fno-jump-tables -fno-omit-frame-pointer). Every
# instruction below appears in demo.s unchanged; only the comments are added.
#
# HOW TO READ AT&T SYNTAX
# -----------------------
#     op   src, dst              # dst = dst OP src   (destination is LAST)
#     movl $1, %eax              # eax = 1            ($ = immediate literal)
#     %reg                       # a register;  %eax is the low 32 bits of %rax
#     N(%base,%index,scale)      # memory at [base + index*scale + N]
#     symbol(%rip)               # RIP-relative address of `symbol` (PIC-friendly)
#   Writing a 32-bit register (eax) ZERO-EXTENDS into the 64-bit one (rax); that
#   is why `xorl %eax,%eax` is the idiom for "rax = 0" and why 32-bit ops
#   dominate this listing whenever the high 32 bits are known to be zero.
#
# THE SYSTEM V AMD64 ABI (what these functions obey)
# --------------------------------------------------
#   * integer/pointer ARGUMENTS, in order:  rdi, rsi, rdx, rcx, r8, r9, then stack
#   * RETURN value:                          rax (eax for a 32-bit value)
#   * CALLEE-SAVED (a function must preserve): rbx, rbp, r12-r15, rsp
#   * CALLER-SAVED (a call may clobber):       rax, rcx, rdx, rsi, rdi, r8-r11
#   * STACK ALIGNMENT: at the instant a `call` executes, rsp must be 16-byte
#     aligned. `call` pushes the 8-byte return address, so on entry rsp ≡ 8
#     (mod 16); the `pushq %rbp` in each prologue brings it back to 16.
#
# THE FOUR FUNCTIONS
# ------------------
#   compose_clone_flags — table-driven OR of the requested CLONE_NEW* bits (star).
#   cap_keep_mask       — build the 64-bit keep mask (constant-folded away!).
#   cap_drop_mask       — the masked complement over caps 0..40 (a 64-bit lesson).
#   main                — compose both, probe three bits, pack the exit code (7).
# We forced the three composers to stay real functions (noinline in the C) so the
# GENERAL logic is visible; watch main call them and fold the results.
# =============================================================================

	.file	"demo.c"
	.text                                   # executable code section

# =============================================================================
# u32 compose_clone_flags(u32 want)
#   want -> edi   (bitmap of WANT_* request bits)
#   ret  -> eax   (the OR of the matching CLONE_NEW* kernel bits)
#
# The C is a loop over kNsMap[]: for each { want_bit, clone_flag } pair, if
# (want & want_bit) then flags |= clone_flag. Each struct ns_map is 8 bytes:
# want_bit at offset +0, clone_flag at offset +4 — hence the scale-8 addressing
# and the +4 displacement below.
#
# Register roles the optimizer chose:
#   rcx = i (loop index)   rdx = &kNsMap (base)   eax = flags (accumulator)
# =============================================================================
	.globl	compose_clone_flags
	.p2align	4                       # 16-byte-align the entry for the fetcher
	.type	compose_clone_flags,@function
compose_clone_flags:
# %bb.0:  -- PROLOGUE + loop setup -------------------------------------------
	pushq	%rbp                    # save caller's frame pointer (callee-saved)
	movq	%rsp, %rbp              # establish our frame; rsp now 16-byte aligned
	xorl	%ecx, %ecx              # i = 0
	leaq	kNsMap(%rip), %rdx      # rdx = &kNsMap[0] (RIP-relative: position-indep.)
	xorl	%eax, %eax              # flags = 0  (the return accumulator)
	jmp	.LBB0_1                 # jump into the loop at its header (test-first)
	.p2align	4
# ---- loop tail: advance i and check the bound -------------------------------
.LBB0_3:                                # continue: (also the "skip" landing pad)
	incq	%rcx                    # i++
	cmpq	$7, %rcx                # i == NS_MAP_LEN (7)?
	je	.LBB0_4                 #   yes -> exit the loop
# ---- loop header: test `want & kNsMap[i].want_bit` --------------------------
.LBB0_1:                                # =>inner loop header
	testl	%edi, (%rdx,%rcx,8)     # compute want & *(u32*)(kNsMap + i*8 + 0)
	                                #   i.e. want & kNsMap[i].want_bit; sets ZF
	je	.LBB0_3                 # bit not requested -> skip the OR, continue
# %bb.2:  requested: OR in this namespace's kernel flag
	orl	4(%rdx,%rcx,8), %eax    # flags |= *(u32*)(kNsMap + i*8 + 4)
	                                #   i.e. flags |= kNsMap[i].clone_flag
	jmp	.LBB0_3                 # continue to the loop tail
.LBB0_4:  -- EPILOGUE --------------------------------------------------------
	popq	%rbp                    # restore caller's frame pointer
	retq                            # return; eax holds the composed flag word
.Lfunc_end0:
	.size	compose_clone_flags, .Lfunc_end0-compose_clone_flags

# =============================================================================
# u64 cap_keep_mask(void)   — build the 64-bit keep mask.
#   ret -> rax
#
# THE LESSON: the C is a loop over kKeep[] OR-ing (1ULL << kKeep[i]). But kKeep
# is all compile-time constants, so the optimizer evaluated the ENTIRE loop at
# COMPILE time and the function collapses to "return the constant 0xA80425FB".
# Because every kept cap number is <= 31, the mask fits in 32 bits, so clang
# loads it with a 5-byte `movl` (which zero-extends into rax) instead of a
# 10-byte `movabsq`. Compare with cap_drop_mask, which genuinely needs 64 bits.
# =============================================================================
	.globl	cap_keep_mask
	.p2align	4
	.type	cap_keep_mask,@function
cap_keep_mask:
# %bb.0:
	pushq	%rbp                    # PROLOGUE: save frame pointer
	movq	%rsp, %rbp              # set up frame
	movl	$2818844155, %eax       # rax = 0xA80425FB — the whole loop, folded.
	                                #   bits 0,1,3,4,5,6,7,8,10,13,18,27,29,31 set
	                                #   (the 14 Docker-default kept capabilities)
	popq	%rbp                    # EPILOGUE
	retq                            # return the mask in rax
.Lfunc_end1:
	.size	cap_keep_mask, .Lfunc_end1-cap_keep_mask

# =============================================================================
# u64 cap_drop_mask(u64 keep)   — every DEFINED cap (0..40) that is NOT kept.
#   keep -> rdi        ret -> rax
#
# C:  all_defined = (1<<41) - 1 = 0x1FFFFFFFFFF   (bits 0..40 set)
#     return all_defined & ~keep
# This is where the u64 earns its keep: 0x1FFFFFFFFFF does NOT fit in 32 bits, so
# every operation here is 64-bit (notq / movabsq / andq on the R-registers).
# =============================================================================
	.globl	cap_drop_mask
	.p2align	4
	.type	cap_drop_mask,@function
cap_drop_mask:
# %bb.0:
	pushq	%rbp                    # PROLOGUE
	movq	%rsp, %rbp              # set up frame
	notq	%rdi                    # rdi = ~keep  (64-bit complement)
	movabsq	$2199023255551, %rax    # rax = 0x1FFFFFFFFFF (bits 0..40) — a 64-bit
	                                #   immediate needs the 10-byte movabsq form
	andq	%rdi, %rax              # rax = all_defined & ~keep  = the drop set
	popq	%rbp                    # EPILOGUE
	retq                            # return the drop mask in rax
.Lfunc_end2:
	.size	cap_drop_mask, .Lfunc_end2-cap_drop_mask

# =============================================================================
# int main(void) — compose both masks, probe three bits, pack a 3-bit exit code.
#
#   bit0: the flag word contains BOTH CLONE_NEWUSER and CLONE_NEWNET
#   bit1: CAP_NET_RAW (13) is KEPT
#   bit2: CAP_SYS_ADMIN (21) is in the DROP mask
#   => a correct build returns 1 + 2 + 4 = 7.
#
# rbx is a callee-saved cache of the keep mask across the second call; r14 is a
# callee-saved cache of the bit0/bit1 partial result. Both are saved/restored.
# =============================================================================
	.globl	main
	.p2align	4
	.type	main,@function
main:
# %bb.0:  -- PROLOGUE --
	pushq	%rbp                    # save frame pointer
	movq	%rsp, %rbp              # set up frame
	pushq	%r14                    # save r14 (callee-saved; caches a partial result)
	pushq	%rbx                    # save rbx (callee-saved; caches the keep mask)

# ---- check 1: compose_clone_flags(0x3F) has NEWUSER and NEWNET --------------
	movl	$63, %edi               # arg0 want = 0x3F = WANT_USER|MNT|PID|NET|UTS|IPC
	callq	compose_clone_flags     # eax = the composed clone() flag word
	notl	%eax                    # eax = ~flags  (so a SET wanted bit becomes 0)
	xorl	%r14d, %r14d            # partial result = 0
	testl	$1342177280, %eax       # test ~flags & 0x50000000, where 0x50000000 =
	                                #   CLONE_NEWUSER(0x10000000)|CLONE_NEWNET(0x40000000).
	                                #   If BOTH bits are set in flags, ~flags clears
	                                #   them, the AND is 0, and ZF=1.
	sete	%r14b                   # r14b = (both present) ? 1 : 0   -> the "1" bit

# ---- check 2 & 3 need the capability masks ---------------------------------
	callq	cap_keep_mask           # rax = keep mask (0xA80425FB)
	movq	%rax, %rbx              # rbx = keep  (cache across the next call)
	movq	%rax, %rdi              # arg0 = keep
	callq	cap_drop_mask           # rax = drop mask = defined & ~keep

# ---- check 2: is CAP_NET_RAW (bit 13) kept? --------------------------------
	shrl	$12, %ebx               # ebx = keep >> 12  (bit 13 lands in bit 1)
	andl	$2, %ebx                # keep only bit 1 -> 2 if cap 13 kept, else 0.
	                                #   (32-bit ops are safe: bit 13 is in the low word)
	orl	%r14d, %ebx             # fold in check 1 -> ebx = bit0 | bit1

# ---- check 3: is CAP_SYS_ADMIN (bit 21) in the drop mask? -------------------
	shrl	$19, %eax               # eax = drop >> 19  (bit 21 lands in bit 2)
	andl	$4, %eax                # keep only bit 2 -> 4 if cap 21 dropped, else 0
	orl	%ebx, %eax              # eax = bit0 | bit1 | bit2  => 7 on a correct build
	                                # kill: def $eax — eax is the 32-bit return/exit code
# ---- EPILOGUE: unwind the saved registers in REVERSE push order -------------
	popq	%rbx                    # restore rbx
	popq	%r14                    # restore r14
	popq	%rbp                    # restore frame pointer
	retq                            # exit status = eax (7)
.Lfunc_end3:
	.size	main, .Lfunc_end3-main

# =============================================================================
# READ-ONLY DATA: the request->flag translation table, laid out as data.
#
# Each row is one `struct ns_map` = 8 bytes: .long want_bit | .long clone_flag.
# These are the exact bytes compose_clone_flags indexes with (%rdx,%rcx,8): the
# want_bit at offset +0 feeds the `testl`, the clone_flag at +4 feeds the `orl`.
# =============================================================================
	.type	kNsMap,@object
	.section	.rodata,"a",@progbits   # "a" = allocatable, read-only
	.p2align	4, 0x0                  # 16-byte align the table
kNsMap:
	.long	1                               # [0] want_bit  = WANT_USER   (1<<0)
	.long	268435456                       #     clone_flag= CLONE_NEWUSER(0x10000000)
	.long	2                               # [1] want_bit  = WANT_MNT    (1<<1)
	.long	131072                          #     clone_flag= CLONE_NEWNS  (0x00020000)
	.long	4                               # [2] want_bit  = WANT_PID    (1<<2)
	.long	536870912                       #     clone_flag= CLONE_NEWPID (0x20000000)
	.long	8                               # [3] want_bit  = WANT_NET    (1<<3)
	.long	1073741824                      #     clone_flag= CLONE_NEWNET (0x40000000)
	.long	16                              # [4] want_bit  = WANT_UTS    (1<<4)
	.long	67108864                        #     clone_flag= CLONE_NEWUTS (0x04000000)
	.long	32                              # [5] want_bit  = WANT_IPC    (1<<5)
	.long	134217728                       #     clone_flag= CLONE_NEWIPC (0x08000000)
	.long	64                              # [6] want_bit  = WANT_CGROUP (1<<6)
	.long	33554432                        #     clone_flag= CLONE_NEWCGROUP(0x2000000)
	.size	kNsMap, 56                      # 7 rows * 8 bytes = 56

	.ident	"clang version 20.1.8"          # toolchain stamp (metadata)
	.section	".note.GNU-stack","",@progbits  # non-executable stack: a security
	                                                #   default the linker records
	.addrsig                                # address-significance table (LTO hint)
# =============================================================================
# WHAT TO TAKE AWAY
#   * Composing clone() flags is just a table walk: test a request bit, OR in a
#     kernel bit. The sparse CLONE_NEW* values (0x20000, 0x2000000 ... 0x40000000)
#     are why we translate from a dense WANT_* bitmap rather than shifting.
#   * cap_keep_mask shows the optimizer evaluating a whole constant loop at
#     COMPILE time — the function became a single `movl` of 0xA80425FB.
#   * cap_drop_mask shows WHY the masks are u64: the "all defined caps" value
#     0x1FFFFFFFFFF needs 64-bit registers and a movabsq immediate.
#   * main shows the classic trick of probing a single bit with shift+and, and
#     caching values in callee-saved rbx/r14 across calls.
#   * Compare with demo.O0.s (every value spilled to the stack, the loops written
#     naively) and demo.O2.s (tighter still) to watch the optimizer work.
# =============================================================================
