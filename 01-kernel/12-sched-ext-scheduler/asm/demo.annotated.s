# =============================================================================
# demo.annotated.s — clang's -O1 output for demo.c, explained instruction by
# instruction. demo.c is the pure-arithmetic core lifted out of the scx_fifo
# BPF scheduler (the virtual-time / weight math), so this is the machine code
# the CPU would run for the scheduler's hottest lines. The untouched compiler
# output is in demo.s; this file is that, annotated.
# =============================================================================
#
# HOW TO READ THIS FILE
# ---------------------
# AT&T syntax, as clang emits: `op  source, destination`.
#     movq %rdi, %rax      # rax = rdi   (source first, destination last)
#     $imm                 # an immediate literal, e.g. $100
#     N(%reg)              # memory/address at [reg + N]
#     leaq N(%r), %d       # d = r + N   (address arithmetic; sets NO flags)
# Register widths are views of ONE register: rax(64) / eax(32) / ax(16) / al(8).
# Writing eax ZERO-EXTENDS into rax, so `movl $X, %eax` (5 bytes) is the compact
# way to load a value whose top 32 bits are zero — you will see it constantly.
#
# THE SYSTEM V AMD64 ABI CONTRACT (what every function below obeys)
# ----------------------------------------------------------------
#   integer/pointer arguments, in order:  rdi, rsi, rdx, rcx, r8, r9  (then stack)
#   return value:                         rax   (rdx:rax for 128-bit)
#   callee-saved (a function MUST preserve): rbx, rbp, r12, r13, r14, r15, rsp
#   caller-saved (a function may clobber):   rax, rcx, rdx, rsi, rdi, r8-r11
#   stack alignment:                      rsp % 16 == 0 at the point of a `call`
#   the "red zone":                       128 bytes below rsp a leaf may scribble
#
# Our four functions map their C parameters onto the arg registers like so:
#   vtime_before(u64 a,  u64 b)                        a=rdi  b=rsi
#   vtime_charge(u64 vtime, u64 slice_rem, u32 weight) vtime=rdi slice=rsi wt=edx
#   vtime_clamp (u64 vtime, u64 vtime_now)             vtime=rdi now=rsi
#   vtime_on_stop(u64 vtime,u64 slice,u32 wt,u64 now)  rdi, rsi, edx, rcx
#
# These are all LEAF functions (they call nothing), so strictly they need no
# stack frame at all. We compiled at -O1 with -fno-omit-frame-pointer, so clang
# still emits the two-instruction push/mov rbp prologue purely so a debugger can
# unwind — it costs a little and buys a readable backtrace. -O2 drops it.
# =============================================================================

	.file	"demo.c"
	.text                           # executable code section

# =============================================================================
# vtime_before(u64 a, u64 b) -> int   :  return (s64)(a - b) < 0;
#
# The wrap-safe timestamp compare. The lesson here is the total absence of a
# comparison-and-branch: the "< 0" test becomes a single logical shift that
# lifts the sign bit down into bit 0. Branchless, four instructions of body.
# =============================================================================
	.globl	vtime_before
	.p2align	4                       # 16-byte align the entry (fetch-friendly)
	.type	vtime_before,@function
vtime_before:
# ---- PROLOGUE ----
	pushq	%rbp                    # save caller's frame pointer
	movq	%rsp, %rbp              # rbp = base of our frame (for the debugger)
# ---- BODY: (s64)(a - b) < 0 ----
	movq	%rdi, %rax              # rax = a          (arg0)
	subq	%rsi, %rax              # rax = a - b      (modular 64-bit subtract;
	                                #   wraps consistently, which is the whole
	                                #   point — magnitudes are never compared)
	shrq	$63, %rax              # rax = (a-b) >> 63, LOGICAL shift: this
	                                #   isolates the SIGN BIT. Result is 1 when
	                                #   (s64)(a-b) is negative, else 0 — exactly
	                                #   the boolean we want, with no cmp/setcc.
	                                # kill: def $eax killed $eax killed $rax
	                                #   (clang liveness note: the return is an
	                                #   int, so only eax matters; rax's top half
	                                #   is already 0 from the shift.)
# ---- EPILOGUE ----
	popq	%rbp                    # restore caller's frame pointer
	retq                            # return; boolean is in eax
.Lfunc_end0:
	.size	vtime_before, .Lfunc_end0-vtime_before

# =============================================================================
# vtime_charge(u64 vtime, u64 slice_remaining, u32 weight) -> u64
#     used = SCX_SLICE_DFL - slice_remaining;
#     return vtime + used * 100 / weight;
#
# The fairness charge. Shows a 64-bit multiply and a 64-bit unsigned divide, and
# WHY the C multiplies before dividing. `divq` is one of the most expensive
# integer ops on x86 (tens of cycles) — keep that in mind when you see it in a
# scheduler hot path.
# =============================================================================
	.globl	vtime_charge
	.p2align	4
	.type	vtime_charge,@function
vtime_charge:
# ---- PROLOGUE ----
	pushq	%rbp                    # save caller's frame pointer
	movq	%rsp, %rbp              # establish frame
# ---- used = SCX_SLICE_DFL - slice_remaining ----
	movl	$20000000, %eax         # rax = 20000000 (SCX_SLICE_DFL, 20ms in ns);
	                                #   movl zero-extends, so the top 32 bits of
	                                #   rax are cleared for free. imm = 0x1312D00
	subq	%rsi, %rax              # rax = SLICE - slice_remaining = `used` (ns)
# ---- used * 100 ----
	imulq	$100, %rax, %rax        # rax = used * 100. Multiply BEFORE divide so
	                                #   small slices don't round to zero — the
	                                #   order the C carefully specifies. Signed
	                                #   imul is fine: the value is far below 2^63.
# ---- ... / weight  (64-bit unsigned divide) ----
	movl	%edx, %ecx              # rcx = weight. arg2 arrived in edx (a u32);
	                                #   movl zero-extends it into the full rcx.
	xorl	%edx, %edx              # rdx = 0. `divq` divides the 128-bit value
	                                #   rdx:rax by its operand, so the high half
	                                #   rdx MUST be zeroed first or we'd divide a
	                                #   garbage 128-bit dividend. xor is the 2-byte
	                                #   idiom for "= 0" and breaks the dep chain.
	divq	%rcx                    # unsigned divide rdx:rax by rcx (weight):
	                                #   quotient -> rax, remainder -> rdx. This is
	                                #   `used * 100 / weight`.
# ---- vtime + (...) ----
	addq	%rdi, %rax              # rax = vtime + quotient   (arg0 + result)
# ---- EPILOGUE ----
	popq	%rbp
	retq                            # 64-bit result in rax
.Lfunc_end1:
	.size	vtime_charge, .Lfunc_end1-vtime_charge

# =============================================================================
# vtime_clamp(u64 vtime, u64 vtime_now) -> u64
#     floor = vtime_now - SCX_SLICE_DFL;
#     if (vtime_before(vtime, floor)) vtime = floor;
#     return vtime;
#
# The anti-hoarding clamp — and a gorgeous branchless idiom. The if/assign is
# realised with a CONDITIONAL MOVE (cmov): no branch, so no branch misprediction
# on a line that runs on every enqueue. vtime_before was INLINED away into the
# flags of one cmp.
# =============================================================================
	.globl	vtime_clamp
	.p2align	4
	.type	vtime_clamp,@function
vtime_clamp:
# ---- PROLOGUE ----
	pushq	%rbp
	movq	%rsp, %rbp
# ---- floor = vtime_now - SCX_SLICE_DFL ----
	leaq	-20000000(%rsi), %rax   # rax = vtime_now - 20000000 = floor. LEA does
	                                #   the subtract as address math and, crucially,
	                                #   sets NO flags — the flags are saved for the
	                                #   compare below.
# ---- if (vtime is before floor) result = floor; else result = vtime ----
	cmpq	%rax, %rdi              # compute rdi - rax = vtime - floor, set flags.
	                                #   SF (sign) = 1 exactly when vtime is "before"
	                                #   floor, i.e. (s64)(vtime-floor) < 0 — the
	                                #   inlined vtime_before test.
	cmovnsq	%rdi, %rax              # cmov-if-Not-Sign: if vtime is NOT before floor
	                                #   (vtime >= floor), overwrite rax with rdi
	                                #   (vtime). Otherwise rax keeps `floor`. Net:
	                                #   rax = max(vtime, floor) in wrap-safe order —
	                                #   the whole if/assign, with zero branches.
# ---- EPILOGUE ----
	popq	%rbp
	retq                            # result in rax
.Lfunc_end2:
	.size	vtime_clamp, .Lfunc_end2-vtime_clamp

# =============================================================================
# vtime_on_stop(u64 vtime, u64 slice_remaining, u32 weight, u64 vtime_now) -> u64
#     vtime = vtime_charge(vtime, slice_remaining, weight);
#     vtime = vtime_clamp(vtime, vtime_now);
#     return vtime;
#
# The complete .stopping() update. The payoff of reading asm: both helper calls
# are GONE — the optimizer inlined vtime_charge and vtime_clamp into one flat,
# call-free body. There is no `call`, no argument re-marshalling: charge's divide
# feeds straight into clamp's compare. arg3 (vtime_now) rode in rcx untouched
# through the whole charge computation.
# =============================================================================
	.globl	vtime_on_stop
	.p2align	4
	.type	vtime_on_stop,@function
vtime_on_stop:
# ---- PROLOGUE ----
	pushq	%rbp
	movq	%rsp, %rbp
# ---- inlined vtime_charge: vtime + (SLICE - slice)*100 / weight ----
	movl	$20000000, %eax         # rax = SCX_SLICE_DFL (20ms ns)
	subq	%rsi, %rax              # rax = SLICE - slice_remaining = used
	imulq	$100, %rax, %rax        # rax = used * 100
	movl	%edx, %esi              # rsi = weight (arg2 from edx, zero-extended).
	                                #   rsi is free now — slice_remaining is spent.
	xorl	%edx, %edx              # rdx = 0: clear the high half of the dividend
	divq	%rsi                    # rdx:rax / weight -> quotient in rax
	addq	%rdi, %rax              # rax = vtime + quotient  (charge complete)
# ---- inlined vtime_clamp against vtime_now (still in rcx) ----
	addq	$-20000000, %rcx        # rcx = vtime_now - 20000000 = floor. imm shown
	                                #   as 0xFECED300 = (-20000000) in two's comp.
	cmpq	%rcx, %rax              # rax - floor, set flags (SF = charged vtime is
	                                #   before floor)
	cmovsq	%rcx, %rax              # cmov-if-Sign: if the charged vtime is before
	                                #   floor, snap rax up to floor. Else keep it.
	                                #   (Same clamp as vtime_clamp, opposite cmov
	                                #   sense because the operands are swapped.)
# ---- EPILOGUE ----
	popq	%rbp
	retq                            # final clamped vtime in rax
.Lfunc_end3:
	.size	vtime_on_stop, .Lfunc_end3-vtime_on_stop

	.ident	"clang version 20.1.8"
	.section	".note.GNU-stack","",@progbits  # non-executable stack: a
	                                                #   security default the linker
	                                                #   records for this object.
	.addrsig                                # address-significance table (LTO icf
	                                        #   metadata); harmless here.

# =============================================================================
# WHAT TO TAKE AWAY
#   * "< 0" on a subtraction is just the sign bit: `shrq $63` beats cmp+setcc.
#     The wrap-safe compare the scheduler depends on is nearly free.
#   * 64-bit division (`divq`) is expensive and needs rdx pre-zeroed because it
#     divides the 128-bit rdx:rax. A scheduler doing this per context-switch
#     cares — which is why the multiply-by-100 is kept small and constant.
#   * `cmov` turns `if (x) y = z;` into a branchless move: no misprediction on
#     the hot enqueue path. This is the single most common asm idiom to learn to
#     recognise.
#   * -O1 already INLINED vtime_charge + vtime_clamp into vtime_on_stop, erasing
#     two `call`s. Now compare the other two committed files:
#       - demo.O0.s : the naive mapping — the helpers stay real `call`s
#                     (vtime_on_stop calls charge then clamp; clamp calls
#                     vtime_before) and every value is spilled to the stack.
#                     Easiest to trace line-by-line.
#       - demo.O2.s : the optimizer off the leash — it adds a `shrq $32; je`
#                     check before the divide so it can use the cheaper 32-bit
#                     `divl` when the dividend fits in 32 bits, and drops the
#                     frame pointer entirely. That divide-width trick is the
#                     "why is it doing THAT?" reward for reading -O2 output.
# =============================================================================
