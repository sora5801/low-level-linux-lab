# =============================================================================
# demo.annotated.s — the microVM monitor's core math, in asm, explained by line.
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
#     leal N(%reg), %dst                 # dst = reg + N  (ADDRESS math, no load)
#
# Register widths are views of ONE register: rax(64)/eax(32)/ax(16)/al(8).
# Writing eax ZERO-EXTENDS into rax, so clang prefers `movl` (5 bytes) over
# `movq` (7) whenever the top 32 bits should be zero. The "kill: def $ax killed
# $ax killed $eax" lines are LLVM register-allocator bookkeeping, not real
# instructions — they emit no bytes; they just tell you "only the low 16 bits of
# this value matter from here," which is precisely how a u16 return truncates.
#
# THE SysV AMD64 ABI CONTRACT (what every function here obeys)
# -----------------------------------------------------------
#   * integer/pointer ARGS, in order:  rdi, rsi, rdx, rcx, r8, r9  (then stack)
#     - u16/u32 args arrive in the low halves: di/si (16) or edi/esi (32);
#       u64 (a GPA) fills the full rdi/rsi.
#   * RETURN value:                     rax (low 16/32 bits for u16/u32).
#   * CALLEE-SAVED (must preserve): rbx, rbp, r12-r15, rsp.
#   * CALLER-SAVED (scratch):       rax, rcx, rdx, rsi, rdi, r8-r11.
#   * STACK ALIGNMENT: rsp % 16 == 0 at every `call`. These leaves make no call,
#     so they never move rsp past the frame-pointer push.
#
# At -O1 clang keeps a frame pointer (pushq %rbp ; movq %rsp,%rbp ... popq %rbp)
# for debuggability. It is optional for these leaves — compare demo.O2.s, where
# it is gone and the bodies are pure arithmetic.
#
# THE BIG PICTURE
# ---------------
# asm/demo.c is the monitor's pure math: the VIRTQUEUE INDEX/WRAP arithmetic (the
# split-virtqueue's load-bearing one-liners), the MMIO address decode that routes
# a fault to a device+register, and the KVM exit-reason dispatch. The three
# payoffs to read for:
#   1. "mod power-of-two" (ring slot, register offset) becomes ONE `and`; "divide
#      by power-of-two" (device index) becomes ONE `shr`. No division anywhere.
#   2. 16-bit ring-index WRAP is free: compute wide, then the u16 truncation
#      (the "kill ... $ax" narrowing) IS the mod-65536.
#   3. the grouped `switch` cases, even with -fno-jump-tables, become a 32-bit
#      BITMASK tested with one `btl` — the mask constant literally IS the set of
#      case values.
# =============================================================================

	.file	"demo.c"
	.text

# =============================================================================
# vq_ring_slot(u16 idx, u16 qsize) -> u16        idx & (qsize - 1)
#   args: idx in %di, qsize in %si ; ret in %ax
#
# Map a free-running 16-bit ring index to its slot in a power-of-two-sized ring.
# idx mod qsize == idx & (qsize-1) BECAUSE qsize is a power of two — so no divide,
# just a decrement and an AND. This single `and` is why virtqueue sizes must be
# powers of two.
# =============================================================================
	.globl	vq_ring_slot
	.p2align	4                       # 16-byte align the entry (I-fetch friendly)
	.type	vq_ring_slot,@function
vq_ring_slot:
	pushq	%rbp                            # PROLOGUE: save caller's frame pointer
	movq	%rsp, %rbp                      # rbp = base of our frame
	# kill: def $esi killed $esi def $rsi   # (no bytes) qsize's full reg is live now
	leal	-1(%rsi), %eax                  # eax = qsize - 1  (the low-bits MASK).
	                                        #   LEA does the subtract as address math,
	                                        #   leaving flags alone and not needing a
	                                        #   separate `dec`.
	andl	%edi, %eax                      # eax = idx & (qsize-1) = idx mod qsize.
	                                        #   THIS is the whole function: modulo by a
	                                        #   power of two is one AND.
	# kill: def $ax killed $ax killed $eax  # (no bytes) only the low 16 bits (%ax) are
	                                        #   the u16 result; the top bits are ignored.
	popq	%rbp                            # EPILOGUE: restore caller's rbp
	retq                                    # return %ax
.Lfunc_end0:
	.size	vq_ring_slot, .Lfunc_end0-vq_ring_slot

# =============================================================================
# vq_pending(u16 avail_idx, u16 last_seen) -> u16      (u16)(avail_idx - last_seen)
#   args: avail_idx %di, last_seen %si ; ret %ax
#
# The count of buffers the driver has published but the device has not consumed —
# a 16-bit WRAPPING subtraction. The subtract is done in 32 bits; the result is
# then narrowed to %ax (the "kill" line), and THAT truncation to 16 bits is the
# modular arithmetic that makes wrap-around (avail_idx wrapped past 0xffff while
# last_seen has not) come out correct for free.
# =============================================================================
	.globl	vq_pending
	.p2align	4
	.type	vq_pending,@function
vq_pending:
	pushq	%rbp                            # PROLOGUE
	movq	%rsp, %rbp
	movl	%edi, %eax                      # eax = avail_idx
	subl	%esi, %eax                      # eax = avail_idx - last_seen (32-bit sub)
	# kill: def $ax killed $ax killed $eax  # (no bytes) keep only %ax: the low 16 bits.
	                                        #   e.g. 0x0003 - 0xfffe = 0xffff0005; %ax =
	                                        #   0x0005 = the true gap of 5 across a wrap.
	popq	%rbp                            # EPILOGUE
	retq
.Lfunc_end1:
	.size	vq_pending, .Lfunc_end1-vq_pending

# =============================================================================
# vq_next(u16 idx) -> u16                          (u16)(idx + 1)
#   arg: idx %di ; ret %ax
#
# Advance a ring cursor by one, wrapping 0xffff -> 0x0000. Same trick: add wide
# with LEA, narrow to %ax to get the wrap.
# =============================================================================
	.globl	vq_next
	.p2align	4
	.type	vq_next,@function
vq_next:
	pushq	%rbp                            # PROLOGUE
	movq	%rsp, %rbp
	# kill: def $edi killed $edi def $rdi   # (no bytes) idx's full reg is live
	leal	1(%rdi), %eax                   # eax = idx + 1 (LEA = add without touching
	                                        #   flags; one instruction, no `inc`).
	# kill: def $ax killed $ax killed $eax  # (no bytes) %ax = (idx+1) mod 65536.
	popq	%rbp                            # EPILOGUE
	retq
.Lfunc_end2:
	.size	vq_next, .Lfunc_end2-vq_next

# =============================================================================
# desc_is_writable(u16 flags) -> int          (flags & VIRTQ_DESC_F_WRITE) != 0
#   arg: flags %di ; ret %eax (0/1)
# VIRTQ_DESC_F_WRITE = 2 = bit 1. Testing bit 1 and returning 0/1 is: shift bit 1
# down to bit 0, then AND 1. Branchless — no jump to mispredict.
# =============================================================================
	.globl	desc_is_writable
	.p2align	4
	.type	desc_is_writable,@function
desc_is_writable:
	pushq	%rbp                            # PROLOGUE
	movq	%rsp, %rbp
	movl	%edi, %eax                      # eax = flags
	shrl	%eax                            # eax >>= 1  (bit 1 -> bit 0). `shrl` with no
	                                        #   count is shift-by-one.
	andl	$1, %eax                        # eax &= 1  -> isolates the old bit 1 as 0/1.
	popq	%rbp                            # EPILOGUE
	retq
.Lfunc_end3:
	.size	desc_is_writable, .Lfunc_end3-desc_is_writable

# =============================================================================
# desc_has_next(u16 flags) -> int              (flags & VIRTQ_DESC_F_NEXT) != 0
#   arg: flags %di ; ret %eax (0/1)
# VIRTQ_DESC_F_NEXT = 1 = bit 0, so no shift is needed — just AND 1.
# =============================================================================
	.globl	desc_has_next
	.p2align	4
	.type	desc_has_next,@function
desc_has_next:
	pushq	%rbp                            # PROLOGUE
	movq	%rsp, %rbp
	movl	%edi, %eax                      # eax = flags
	andl	$1, %eax                        # eax &= 1 -> old bit 0 (the NEXT flag) as 0/1
	popq	%rbp                            # EPILOGUE
	retq
.Lfunc_end4:
	.size	desc_has_next, .Lfunc_end4-desc_has_next

# =============================================================================
# mmio_device_index(u64 gpa, u64 base) -> u32       (gpa - base) / 0x200
#   args: gpa %rdi, base %rsi ; ret %eax
# Which virtio-mmio device window did the fault land in? The stride is 0x200, a
# power of two, so the divide is a right shift by 9 (2^9 = 512 = 0x200). Full
# 64-bit math because a GPA is 64-bit.
# =============================================================================
	.globl	mmio_device_index
	.p2align	4
	.type	mmio_device_index,@function
mmio_device_index:
	pushq	%rbp                            # PROLOGUE
	movq	%rsp, %rbp
	movq	%rdi, %rax                      # rax = gpa
	subq	%rsi, %rax                      # rax = gpa - base  (offset from bus base)
	shrq	$9, %rax                        # rax >>= 9  == /512 == /0x200: the device #.
	                                        #   division-by-power-of-two is one shift.
	# kill: def $eax killed $eax killed $rax# (no bytes) return the low 32 bits (u32).
	popq	%rbp                            # EPILOGUE
	retq
.Lfunc_end5:
	.size	mmio_device_index, .Lfunc_end5-mmio_device_index

# =============================================================================
# mmio_reg_offset(u64 gpa, u64 base) -> u32          (gpa - base) % 0x200
#   args: gpa %rdi, base %rsi ; ret %eax
# Which register within that device? modulo a power-of-two stride == AND with
# (stride-1) = 0x1ff. Note clang subtracts in 32 bits (`subl`) here: the result is
# masked to the low 9 bits anyway, so the high 32 bits can't matter — a free
# narrowing.
# =============================================================================
	.globl	mmio_reg_offset
	.p2align	4
	.type	mmio_reg_offset,@function
mmio_reg_offset:
	pushq	%rbp                            # PROLOGUE
	movq	%rsp, %rbp
	movq	%rdi, %rax                      # rax = gpa
	subl	%esi, %eax                      # eax = (gpa - base) low 32 bits. `subl` (not
	                                        #   subq): only the low 9 bits survive the AND
	                                        #   below, so 32-bit math is sufficient.
	andl	$511, %eax                      # eax &= 0x1ff  == mod 0x200: the register
	                                        #   offset within the device window.
	# kill: def $eax killed $eax killed $rax# (no bytes) u32 result in %eax.
	popq	%rbp                            # EPILOGUE
	retq
.Lfunc_end6:
	.size	mmio_reg_offset, .Lfunc_end6-mmio_reg_offset

# =============================================================================
# kvm_exit_action(u32 reason) -> enum vmm_action
#   arg: reason %edi ; ret action in %eax
#
# The C was a `switch (reason)`. Action codes:
#   ACT_UNHANDLED=0  ACT_MMIO=1  ACT_PIO=2  ACT_STOP_OK=3  ACT_STOP_ERR=4
#   ACT_REENTER=5
# We built with -fno-jump-tables expecting a compare chain; the singleton cases
# ARE compares, but the two case GROUPS were folded into 32-bit BITMASKS tested
# with a single `btl`. Read the blocks as: (1) range guard, (2) the two bitmask
# membership tests, (3) explicit compares for the singletons, (4) default.
# =============================================================================
	.globl	kvm_exit_action
	.p2align	4
	.type	kvm_exit_action,@function
kvm_exit_action:
	pushq	%rbp                            # PROLOGUE
	movq	%rsp, %rbp

# ---- (1) RANGE GUARD: is `reason` inside the dense window [0,17]? ----
	cmpl	$17, %edi                       # compare reason with 17 (the max case value)
	ja	.LBB7_1                         # UNSIGNED above 17 -> can't be a grouped case;
	                                        #   skip to the singleton compares. `ja` (not
	                                        #   `jg`) because reason is unsigned.

# ---- (2a) BITMASK TEST for the STOP_ERR group {8, 9, 17} ----
# 0x20300 has exactly bits 8, 9, 17 set:  (1<<8)|(1<<9)|(1<<17) = 131840.
	movl	$131840, %eax                   # eax = 0x20300 = the STOP_ERR case set
	btl	%edi, %eax                      # CF = bit number `reason` of eax; i.e. CF=1
	                                        #   iff reason is 8, 9, or 17.
	jb	.LBB7_8                         # `jb` = jump if CF==1 -> a STOP_ERR reason.

# ---- (2b) BITMASK TEST for the REENTER group {7, 10} ----
# 0x480 has bits 7 and 10 set:  (1<<7)|(1<<10) = 1152.
	movl	$1152, %eax                     # eax = 0x480 = the REENTER case set
	btl	%edi, %eax                      # CF=1 iff reason is 7 or 10
	jae	.LBB7_6                         # `jae` = jump if CF==0 -> NOT a REENTER case;
	                                        #   go handle the MMIO/IO/HLT singletons.
# fall-through: reason WAS 7 or 10 -> ACT_REENTER
	movl	$5, %eax                        # eax = 5 = ACT_REENTER
	popq	%rbp
	retq

# ---- STOP_ERR return target (from 2a) ----
.LBB7_8:
	movl	$4, %eax                        # eax = 4 = ACT_STOP_ERR
	popq	%rbp
	retq

# ---- (3a) singleton: KVM_EXIT_MMIO (6) -> ACT_MMIO, the hot case ----
# Note the optimizer PRE-LOADS the answer (1) before confirming the reason: since
# ACT_MMIO=1 is wanted only when reason==6, it sets eax=1 then checks, and if the
# check fails it falls through to the compare ladder (eax will be overwritten).
.LBB7_6:
	movl	$1, %eax                        # eax = 1 = ACT_MMIO (speculatively)
	cmpl	$6, %edi                        # reason == 6 (MMIO) ?
	jne	.LBB7_1                         # no -> singletons/default (eax gets replaced)
# %bb.11: reason == 6 -> return the 1 we already loaded
	popq	%rbp
	retq

# ---- (3b/3c) singletons: KVM_EXIT_HLT (5) and KVM_EXIT_IO (2); else default ----
.LBB7_1:
	cmpl	$5, %edi                        # reason == 5 (HLT) ?
	je	.LBB7_7                         # yes -> ACT_STOP_OK
# %bb.2:
	cmpl	$2, %edi                        # reason == 2 (IO) ?
	jne	.LBB7_10                        # no -> default (ACT_UNHANDLED)
# %bb.3: reason == 2 -> ACT_PIO
	movl	$2, %eax                        # eax = 2 = ACT_PIO (the debug/exit port)
	popq	%rbp
	retq
.LBB7_7:
	movl	$3, %eax                        # eax = 3 = ACT_STOP_OK  (HLT)
	popq	%rbp
	retq

# ---- (4) DEFAULT: any reason we do not model ----
.LBB7_10:
	xorl	%eax, %eax                      # eax = 0 = ACT_UNHANDLED. `xor r,r` is the
	                                        #   idiomatic 2-byte zero and a dependency
	                                        #   breaker the CPU special-cases.
	popq	%rbp
	retq
.Lfunc_end7:
	.size	kvm_exit_action, .Lfunc_end7-kvm_exit_action

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
	xorl	%eax, %eax                      # eax = 0: the folded result of every check
	popq	%rbp                            # EPILOGUE
	retq
.Lfunc_end8:
	.size	demo_selftest, .Lfunc_end8-demo_selftest

	.ident	"clang version 20.1.8"
	.section	".note.GNU-stack","",@progbits  # non-executable stack: a security
	                                                #   default the linker records.
# =============================================================================
# WHAT TO TAKE AWAY
#   * A virtqueue's "mod queue-size" is a single `and` and its "divide by stride"
#     a single `shr`, because both are powers of two. That is not an accident:
#     virtio REQUIRES power-of-two queue sizes precisely so ring math is this
#     cheap on the per-buffer hot path.
#   * 16-bit ring-index WRAP costs nothing: compute in 32 bits, then the u16
#     truncation (the "kill ... $ax" narrowing) performs the mod-65536. The
#     wrap you were worried about is just "keep the low 16 bits."
#   * -fno-jump-tables removed the jump table, but grouped switch cases still beat
#     a naive compare chain: each group is a 32-bit BITMASK and membership is one
#     `btl`. The masks 0x20300 and 0x480 literally ARE the sets {8,9,17} and
#     {7,10}. Stare until `btl %edi,%eax` reads as "is reason in this set?".
#   * Compare with demo.O0.s (every value spilled to the stack, the switch a plain
#     compare ladder — easiest to trace) and demo.O2.s (same logic, frame pointer
#     gone, selftest still evaporated). Reading all three side by side is how you
#     learn what -O really does.
# =============================================================================
