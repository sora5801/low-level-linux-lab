# =============================================================================
# demo.annotated.s — the VM-exit dispatch, in assembly, explained line by line.
# =============================================================================
#
# HOW TO READ THIS FILE
# ---------------------
# This is the exact assembly clang emits for asm/demo.c at -O1 (see demo.s for
# the untouched original), with a comment on essentially every instruction. AT&T
# syntax throughout:
#
#     op   source, destination          # e.g.  movl $1, %eax   =>  eax = 1
#     %reg                               # a register
#     $imm                               # an immediate (literal) constant
#     N(%reg)                            # memory at [reg + N]
#
# Register widths are views of ONE register: rax(64) / eax(32) / ax(16) / al(8).
# Writing eax ZERO-EXTENDS into rax, so clang prefers `movl $x, %eax` (5 bytes)
# over `movq` (7 bytes) whenever the top 32 bits should be zero.
#
# THE SysV AMD64 ABI CONTRACT (what every function here obeys)
# -----------------------------------------------------------
#   * integer/pointer ARGS, in order:  rdi, rsi, rdx, rcx, r8, r9   (then stack)
#     - our args are 32-bit, so they arrive in the low halves: edi, esi, edx.
#   * RETURN value:                     rax   (a struct <= 16 bytes: rdx:rax;
#                                              here 8 bytes fit in rax alone).
#   * CALLEE-SAVED (a function must preserve): rbx, rbp, r12-r15, rsp.
#   * CALLER-SAVED (free scratch):             rax, rcx, rdx, rsi, rdi, r8-r11.
#   * STACK ALIGNMENT: rsp % 16 == 0 at every `call`. These leaf functions make
#     no call, so they never touch rsp beyond the frame-pointer push.
#
# PROLOGUE / EPILOGUE. At -O1 clang keeps a frame pointer for debuggability:
#     pushq %rbp ; movq %rsp,%rbp   ... body ...   popq %rbp ; retq
# It is strictly optional for these leaves (compare demo.O2.s, where it is gone
# and functions are pure arithmetic), but it makes a backtrace possible and each
# function start/end obvious. rbp is callee-saved, so we must restore it.
#
# THE BIG PICTURE
# ---------------
# asm/demo.c is the VMM's decision core: given a KVM_EXIT_* reason, return the
# policy action the run loop should take. The star is `kvm_exit_action`. We built
# with -fno-jump-tables expecting a compare chain, and the SINGLE-value cases are
# exactly that. But for the two GROUPS of cases that share an action
# ({8,9,17}->STOP_ERR and {7,10}->REENTER), the optimizer did something slicker
# than either a table or a chain: it packed each group into a 32-bit BITMASK and
# tests membership with one `btl` (bit test). That trick, and the way `decode_exit`
# returns a two-field struct packed into %rax, are the payoffs to read for here.
# =============================================================================

	.file	"demo.c"
	.text

# =============================================================================
# kvm_exit_action(u32 reason) -> enum vmm_action
#   arg:  reason in %edi
#   ret:  the action code in %eax
#
# The C was a `switch (reason)`. Read the blocks in this order: (1) range guard,
# (2) the two bitmask membership tests for the grouped cases, (3) explicit
# compares for the singleton cases, (4) the default. The action codes are:
#   ACT_UNHANDLED=0  ACT_EMULATE_IO=1  ACT_EMULATE_MMIO=2
#   ACT_STOP_OK=3    ACT_STOP_ERR=4    ACT_REENTER=5
# =============================================================================
	.globl	kvm_exit_action
	.p2align	4                       # 16-byte align the entry (I-fetch friendly)
	.type	kvm_exit_action,@function
kvm_exit_action:
# ---- PROLOGUE ----
	pushq	%rbp                            # save caller's frame pointer
	movq	%rsp, %rbp                      # rbp = frame base for this call

# ---- (1) RANGE GUARD: is `reason` even in the switch's dense window [0,17]? ----
	cmpl	$17, %edi                       # compare reason with 17 (the max case)
	ja	.LBB0_1                         # UNSIGNED above 17 -> can't be a grouped
                                                #   case; skip to the singleton/default
                                                #   compares. `ja` (not `jg`) because
                                                #   reason is unsigned: a huge value must
                                                #   not look "negative/less".

# ---- (2a) BITMASK TEST for the STOP_ERR group {8, 9, 17} ----
# 0x20300 has exactly bits 8, 9, and 17 set:  (1<<8)|(1<<9)|(1<<17) = 131840.
	movl	$131840, %eax                   # eax = 0x20300 = the STOP_ERR case set
	btl	%edi, %eax                      # CF = bit number `reason` of eax. i.e.
                                                #   CF=1 iff reason is 8, 9, or 17.
	jb	.LBB0_8                         # `jb` = jump if CF==1 -> it's a STOP_ERR

# ---- (2b) BITMASK TEST for the REENTER group {7, 10} ----
# 0x480 has bits 7 and 10 set:  (1<<7)|(1<<10) = 1152.
	movl	$1152, %eax                     # eax = 0x480 = the REENTER case set
	btl	%edi, %eax                      # CF=1 iff reason is 7 or 10
	jae	.LBB0_5                         # `jae` = jump if CF==0 -> NOT a REENTER
                                                #   case; go handle the singletons.
# %bb.9: fall-through means reason WAS 7 or 10 -> ACT_REENTER
	movl	$5, %eax                        # eax = 5 = ACT_REENTER
	popq	%rbp                            # EPILOGUE: restore caller's rbp
	retq                                    # return eax

# ---- STOP_ERR return target (from 2a) ----
.LBB0_8:
	movl	$4, %eax                        # eax = 4 = ACT_STOP_ERR
	popq	%rbp
	retq

# ---- (3a) singleton: KVM_EXIT_MMIO (6) -> ACT_EMULATE_MMIO ----
.LBB0_5:
	cmpl	$6, %edi                        # reason == 6 (MMIO) ?
	jne	.LBB0_1                         # no -> try the last singletons/default
	movl	$2, %eax                        # eax = 2 = ACT_EMULATE_MMIO
	popq	%rbp
	retq

# ---- (3b/3c) singletons: KVM_EXIT_IO (2) and KVM_EXIT_HLT (5); else default ----
.LBB0_1:
	cmpl	$2, %edi                        # reason == 2 (IO) ?
	je	.LBB0_2                         # yes -> ACT_EMULATE_IO
	cmpl	$5, %edi                        # reason == 5 (HLT) ?
	jne	.LBB0_11                        # no -> default (ACT_UNHANDLED)
	movl	$3, %eax                        # eax = 3 = ACT_STOP_OK  (HLT)
	popq	%rbp
	retq
.LBB0_2:
	movl	$1, %eax                        # eax = 1 = ACT_EMULATE_IO
	popq	%rbp
	retq

# ---- (4) DEFAULT: any reason we do not model ----
.LBB0_11:
	xorl	%eax, %eax                      # eax = 0 = ACT_UNHANDLED. `xor r,r` is the
                                                #   idiomatic 2-byte zero and a dependency
                                                #   breaker the CPU special-cases.
	popq	%rbp
	retq
.Lfunc_end0:
	.size	kvm_exit_action, .Lfunc_end0-kvm_exit_action

# =============================================================================
# io_batch_bytes(u32 size, u32 count) -> u32     size * count
#   args: size in %edi, count in %esi ; ret in %eax
# The total number of bytes a rep-string port I/O moves in one exit. Both operands
# are runtime values, so it is a genuine multiply (no strength reduction possible).
# =============================================================================
	.globl	io_batch_bytes
	.p2align	4
	.type	io_batch_bytes,@function
io_batch_bytes:
	pushq	%rbp                            # PROLOGUE
	movq	%rsp, %rbp
	movl	%edi, %eax                      # eax = size          (move arg0 to the
                                                #   return register to multiply in place)
	imull	%esi, %eax                      # eax = eax * count. `imul` here because
                                                #   the low 32 bits of a signed or unsigned
                                                #   product are identical; one instruction.
	popq	%rbp                            # EPILOGUE
	retq
.Lfunc_end1:
	.size	io_batch_bytes, .Lfunc_end1-io_batch_bytes

# =============================================================================
# serial_is_console_write(u32 direction, u32 port, u32 console_port) -> u32 (0/1)
#   args: direction %edi, port %esi, console_port %edx ; ret %eax
# C:  (direction == OUT) & (port == console_port), where OUT == 1.
# The compiler turns two equality tests + a bitwise AND into a BRANCHLESS sequence
# of xor/or/sete — no jump to mispredict, which is ideal on a per-exit hot path.
# =============================================================================
	.globl	serial_is_console_write
	.p2align	4
	.type	serial_is_console_write,@function
serial_is_console_write:
	pushq	%rbp                            # PROLOGUE
	movq	%rsp, %rbp
	xorl	$1, %edi                        # edi = direction ^ 1. This is 0 EXACTLY
                                                #   when direction == 1 (OUT). xor-with-a-
                                                #   constant is how you test "== const"
                                                #   without touching flags you need later.
	xorl	%edx, %esi                      # esi = port ^ console_port. 0 iff equal.
	xorl	%eax, %eax                      # eax = 0 (clear the result; also clears
                                                #   flags' dependence — see sete below).
	orl	%edi, %esi                      # esi = (direction^1) | (port^cport). This
                                                #   is 0 iff BOTH halves were 0, i.e. iff
                                                #   direction==OUT AND port==cport. Sets ZF.
	sete	%al                             # al = (ZF==1) ? 1 : 0. The boolean result,
                                                #   materialized from the flag with no branch.
	popq	%rbp                            # EPILOGUE (eax already holds 0/1)
	retq
.Lfunc_end2:
	.size	serial_is_console_write, .Lfunc_end2-serial_is_console_write

# =============================================================================
# decode_exit(u32 reason) -> struct exit_decision { u32 action; u32 is_error; }
#   arg: reason %edi ; ret: the 8-byte struct PACKED IN %rax
#
# Same dispatch as kvm_exit_action, but it must ALSO return is_error (1 iff the
# action is ACT_STOP_ERR). The struct is 8 bytes (two u32), which the SysV ABI
# returns in a single register: `action` in the low 32 bits of rax, `is_error` in
# the high 32 bits. Watch how each arm sets ecx=action and rax=(is_error<<32),
# then the shared tail ORs them together. Only the STOP_ERR arm makes is_error=1,
# so only it loads the 1<<32 constant; every other arm leaves rax's top half 0.
# =============================================================================
	.globl	decode_exit
	.p2align	4
	.type	decode_exit,@function
decode_exit:
	pushq	%rbp                            # PROLOGUE
	movq	%rsp, %rbp

# ---- identical range guard + bitmask tests as kvm_exit_action ----
	cmpl	$17, %edi                       # reason within [0,17]?
	ja	.LBB3_1                         # no -> singletons/default
	movl	$131840, %eax                   # STOP_ERR set = {8,9,17}
	btl	%edi, %eax
	jb	.LBB3_8                         # reason in {8,9,17} -> STOP_ERR arm
	movl	$1152, %eax                     # REENTER set = {7,10}
	btl	%edi, %eax
	jae	.LBB3_5                         # not {7,10} -> singletons
# %bb.9: REENTER arm (reason 7 or 10)
	movl	$5, %ecx                        # ecx = action = ACT_REENTER (5)
	xorl	%eax, %eax                      # rax = 0 -> is_error = 0 (high half clear)
	jmp	.LBB3_12                        # go pack {ecx, rax}

# ---- STOP_ERR arm: the only place is_error becomes 1 ----
.LBB3_8:
	movl	$4, %ecx                        # ecx = action = ACT_STOP_ERR (4)
	movabsq	$4294967296, %rax               # rax = 0x1_0000_0000 = (1 << 32):
                                                #   this sets is_error=1 IN THE HIGH 32 BITS
                                                #   of the return register directly. A neat
                                                #   trick: no shift instruction needed.
	jmp	.LBB3_12

# ---- singleton MMIO (6) -> EMULATE_MMIO, is_error 0 ----
.LBB3_5:
	cmpl	$6, %edi
	jne	.LBB3_1
	movl	$2, %ecx                        # ecx = ACT_EMULATE_MMIO (2)
	xorl	%eax, %eax                      # is_error = 0
	jmp	.LBB3_12

# ---- singletons IO (2) / HLT (5); else default ----
.LBB3_1:
	cmpl	$2, %edi
	je	.LBB3_2
	cmpl	$5, %edi
	jne	.LBB3_11
	movl	$3, %ecx                        # ecx = ACT_STOP_OK (3)  (HLT)
	xorl	%eax, %eax                      # is_error = 0
	jmp	.LBB3_12
.LBB3_2:
	movl	$1, %ecx                        # ecx = ACT_EMULATE_IO (1)
	xorl	%eax, %eax                      # is_error = 0
	jmp	.LBB3_12

# ---- default -> UNHANDLED, is_error 0 ----
.LBB3_11:
	xorl	%eax, %eax                      # rax = 0 (is_error 0)
	xorl	%ecx, %ecx                      # ecx = 0 = ACT_UNHANDLED

# ---- shared tail: fold action (ecx) into the low half of rax ----
.LBB3_12:
	orq	%rcx, %rax                      # rax = (is_error << 32) | action. Because
                                                #   ecx's write zero-extended into rcx, the
                                                #   OR drops `action` cleanly into bits 0-31
                                                #   while bits 32-63 keep is_error. One
                                                #   register now carries the whole struct.
	popq	%rbp                            # EPILOGUE
	retq
.Lfunc_end3:
	.size	decode_exit, .Lfunc_end3-decode_exit

# =============================================================================
# demo_selftest(void) -> int
# A dozen assertions over CONSTANT inputs. The optimizer evaluated every one at
# compile time, proved they all pass, and deleted the entire body — leaving just
# "return 0". Seeing a function you wrote vanish to `xor %eax,%eax` is the whole
# point of reading the assembly: constant propagation is real and ruthless.
# =============================================================================
	.globl	demo_selftest
	.p2align	4
	.type	demo_selftest,@function
demo_selftest:
	pushq	%rbp                            # PROLOGUE (kept at -O1 for the frame)
	movq	%rsp, %rbp
	xorl	%eax, %eax                      # eax = 0: the folded result of all checks
	popq	%rbp                            # EPILOGUE
	retq
.Lfunc_end4:
	.size	demo_selftest, .Lfunc_end4-demo_selftest

	.ident	"clang version 20.1.8"
	.section	".note.GNU-stack","",@progbits  # non-executable stack: a
                                                        #   security default the linker records.
# =============================================================================
# WHAT TO TAKE AWAY
#   * -fno-jump-tables killed the jump table, but the optimizer still beat a naive
#     compare chain: grouped switch cases became a 32-bit BITMASK + one `btl`.
#     The mask constants 0x20300 and 0x480 literally ARE the sets {8,9,17} and
#     {7,10}. That is the single most surprising thing in this file — go stare at
#     it until `btl %edi, %eax` reads as "is `reason` in this set?".
#   * A struct of two u32 (8 bytes) returns in ONE register: low half = first
#     field, high half = second. `movabsq $1<<32` writes the second field in
#     place with no shift; `orq` merges the two. No hidden pointer, no memory.
#   * Booleans go branchless: xor/or/sete instead of compare-and-jump.
#   * Constant inputs make whole functions evaporate (demo_selftest -> return 0).
#   * Compare with demo.O0.s (every value spilled to the stack, the switch a plain
#     compare ladder — easiest to trace) and demo.O2.s (same logic, frame pointer
#     gone). Reading all three side by side is how you learn what -O really does.
# =============================================================================
