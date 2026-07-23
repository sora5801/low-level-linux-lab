# =============================================================================
# demo.annotated.s — the ring buffer index math (demo.c) explained instruction
#                    by instruction. This is clang's -O1 output (see demo.s for
#                    the untouched original) with a comment on nearly every line.
# =============================================================================
#
# HOW TO READ THIS FILE
# ---------------------
# AT&T syntax throughout:  op  source, destination.  So `subl %esi, %eax` means
# eax = eax - esi.  Notation:
#     %reg            a register            $imm          a literal constant
#     N(%reg)         memory at [reg + N]   (%r1,%r2)     address r1 + r2
#     leal/leaq       "load effective address" — compute an address expression
#                     but store the NUMBER, not a memory load. The compiler loves
#                     it as a free add/shift that doesn't touch flags.
#
# Register widths are the SAME register: rax(64) / eax(32) / ax(16) / al(8).
# Writing eax ZERO-EXTENDS into rax, so clang uses 32-bit ops on our u32 values
# and gets the upper 32 bits cleared for free.
#
# THE SYSTEM V AMD64 ABI (what every function here obeys)
# -------------------------------------------------------
#   integer/pointer ARGS, in order:  rdi, rsi, rdx, rcx, r8, r9   (then stack)
#     so for f(u32 a, u32 b, u32 c):   a=edi,  b=esi,  c=edx
#   RETURN value:                     rax   (a struct up to 16 bytes comes back
#                                     packed in rdx:rax — see rb_plan_xfer)
#   CALLEE-SAVED (must preserve):     rbx, rbp, r12, r13, r14, r15
#   CALLER-SAVED (scratch, free):     rax, rcx, rdx, rsi, rdi, r8, r9, r10, r11
#   STACK: 16-byte aligned at a `call`; a leaf may use the 128-byte "red zone"
#          below rsp without adjusting rsp at all.
#
# Every function below is a LEAF (calls nothing), so it needs no stack space and
# no red zone. At -O1 clang still emits a frame-pointer prologue/epilogue
# (push %rbp / mov %rsp,%rbp ... pop %rbp) purely for debuggability; it is
# functionally dead weight here, and at -O2 (demo.O2.s) it vanishes entirely,
# leaving just the arithmetic. That disappearance is itself a lesson: the frame
# pointer is a convenience, not a requirement, for a function with no locals.
#
# THE ONE IDEA BEHIND ALL OF IT
# -----------------------------
# capacity is a POWER OF TWO, so `index & (capacity-1)` is the physical offset
# (a mask, no divide), and head/tail are FREE-RUNNING so `head - tail` is the
# live byte count via plain unsigned wraparound. Watch that turn into a single
# `and` and a single `sub`.
# =============================================================================

	.file	"demo.c"
	.text

# =============================================================================
# u32 rb_count(u32 head, u32 tail)   ->   head - tail
# The number of bytes currently buffered. Unsigned subtraction is exact even
# when the free-running indices have wrapped past 2^32.
# =============================================================================
	.globl	rb_count
	.p2align	4                       # 16-byte align the entry (fetch-friendly)
	.type	rb_count,@function
rb_count:
	pushq	%rbp                            # PROLOGUE: save caller's frame pointer
	movq	%rsp, %rbp                      #   rbp = rsp: frame base (debug only here)
	movl	%edi, %eax                      # eax = head            (arg0, edi -> return reg)
	subl	%esi, %eax                      # eax = head - tail     (arg1 is tail, in esi)
	popq	%rbp                            # EPILOGUE: restore caller's rbp
	retq                                    # return; result (count) is in eax
.Lfunc_end0:
	.size	rb_count, .Lfunc_end0-rb_count

# =============================================================================
# u32 rb_space(u32 head, u32 tail, u32 cap)   ->   cap - (head - tail)
# Free bytes = capacity minus current count. clang rewrites it as
# cap + tail - head so it can fuse the final add into one LEA.
# =============================================================================
	.globl	rb_space
	.p2align	4
	.type	rb_space,@function
rb_space:
	pushq	%rbp                            # PROLOGUE
	movq	%rsp, %rbp
	# The two "kill" lines below are not instructions — they are clang telling
	# its own register allocator that writing esi/edx also defines the full
	# rsi/rdx (upper bits become known-zero), which legitimises the 64-bit LEA
	# that follows. They assemble to nothing.
                                        # kill: def $edx killed $edx def $rdx
                                        # kill: def $esi killed $esi def $rsi
	subl	%edi, %esi                      # esi = tail - head     (esi=tail, edi=head)
	leal	(%rsi,%rdx), %eax               # eax = esi + edx = (tail - head) + cap
                                        #   = cap - (head - tail) = free space.
                                        #   LEA is used purely as a 2-input ADD here.
	popq	%rbp                            # EPILOGUE
	retq                                    # return free space in eax
.Lfunc_end1:
	.size	rb_space, .Lfunc_end1-rb_space

# =============================================================================
# u32 rb_first_chunk(u32 index, u32 cap, u32 n)
#     off       = index & (cap - 1)
#     tail_room = cap - off
#     return    min(n, tail_room)
# How many of n bytes are contiguous before the ring wraps. The min() becomes a
# BRANCHLESS compare + conditional-move (no jump, no misprediction).
#   args: index=edi, cap=esi, n=edx
# =============================================================================
	.globl	rb_first_chunk
	.p2align	4
	.type	rb_first_chunk,@function
rb_first_chunk:
	pushq	%rbp                            # PROLOGUE
	movq	%rsp, %rbp
	movl	%esi, %eax                      # eax = cap
	leal	-1(%rax), %ecx                  # ecx = cap - 1 = the wrap MASK
                                        #   (LEA as "eax + (-1)", flags untouched)
	andl	%edi, %ecx                      # ecx = index & (cap-1) = off (physical offset)
	subl	%ecx, %eax                      # eax = cap - off = tail_room (bytes to wrap)
	cmpl	%eax, %edx                      # compare n (edx) against tail_room (eax);
                                        #   sets CF if n < tail_room (unsigned)
	cmovbl	%edx, %eax                      # if n < tail_room (CF=1), eax = n.
                                        #   CMOVB = "conditional move if below".
                                        #   Net: eax = min(n, tail_room), no branch.
                                        # kill: def $eax killed $eax killed $rax
	popq	%rbp                            # EPILOGUE
	retq                                    # return the contiguous chunk length in eax
.Lfunc_end2:
	.size	rb_first_chunk, .Lfunc_end2-rb_first_chunk

# =============================================================================
# struct rb_plan rb_plan_xfer(u32 index, u32 cap, u32 n)
#     first  = min(n, cap - (index & (cap-1)))     bytes before the wrap
#     second = n - first                           wrapped remainder at offset 0
#     returns { first, second }
#
# THE ABI HIGHLIGHT: struct rb_plan is 8 bytes (two u32). The SysV ABI returns a
# struct that small PACKED INTO ONE REGISTER (%rax), NOT via a hidden pointer:
# `first` in the low 32 bits, `second` in the high 32 bits. The shl/lea pair at
# the end is the compiler assembling that packed return value.
#   args: index=edi, cap=esi, n=edx
# =============================================================================
	.globl	rb_plan_xfer
	.p2align	4
	.type	rb_plan_xfer,@function
rb_plan_xfer:
	pushq	%rbp                            # PROLOGUE
	movq	%rsp, %rbp
                                        # kill: def $edx killed $edx def $rdx
                                        # kill: def $esi killed $esi def $rsi
	leal	-1(%rsi), %eax                  # eax = cap - 1 = mask
	andl	%edi, %eax                      # eax = index & mask = off
	subl	%eax, %esi                      # esi = cap - off = tail_room
	cmpl	%esi, %edx                      # compare n (edx) vs tail_room (esi)
	cmovbl	%edx, %esi                      # esi = min(n, tail_room) = FIRST (contiguous)
	subl	%esi, %edx                      # edx = n - first = SECOND (wrapped remainder)
	shlq	$32, %rdx                       # rdx <<= 32: move `second` into the HIGH half
	leaq	(%rsi,%rdx), %rax               # rax = esi | (rdx) = first(low32) | second(high32)
                                        #   (rsi's top 32 bits are zero, so add == OR).
                                        #   This IS the packed { first, second } struct.
	popq	%rbp                            # EPILOGUE
	retq                                    # return the 8-byte struct in rax
.Lfunc_end3:
	.size	rb_plan_xfer, .Lfunc_end3-rb_plan_xfer

# =============================================================================
# int rb_selftest(void)   ->   the optimizer's victory lap.
# rb_selftest() runs a dozen assertions over compile-time-CONSTANT inputs and
# returns 0 on success. clang evaluated EVERY branch at compile time, proved the
# result is always 0, and deleted the entire body. What remains is the idiomatic
# "return 0": `xor eax,eax`. Reading assembly is how you SEE constant folding
# erase code you wrote — the whole point of keeping the .s open.
# =============================================================================
	.globl	rb_selftest
	.p2align	4
	.type	rb_selftest,@function
rb_selftest:
	pushq	%rbp                            # PROLOGUE
	movq	%rsp, %rbp
	xorl	%eax, %eax                      # eax = 0. `xor r,r` is the 2-byte, dependency-
                                        #   breaking idiom for "return 0"; the whole
                                        #   test body was constant-folded away.
	popq	%rbp                            # EPILOGUE
	retq                                    # return 0
.Lfunc_end4:
	.size	rb_selftest, .Lfunc_end4-rb_selftest

	.ident	"clang version 20.1.8"          # toolchain stamp (metadata)
	.section	".note.GNU-stack","",@progbits  # mark the stack NON-executable (W^X)
	.addrsig                                # address-significance table (LTO icf hint)
# =============================================================================
# WHAT TO TAKE AWAY
#   * `index & (cap-1)` is a power-of-two modulo: ONE `and`, no division. This is
#     why every serious ring buffer (kfifo, io_uring, DPDK) sizes to a power of 2.
#   * `head - tail` on free-running unsigned counters is the exact byte count,
#     wraparound and all — ONE `sub`. No separate "full vs empty" flag needed.
#   * min() with no data-dependent branch = cmp + cmov. Branchless, mispredict-free.
#   * A <=16-byte struct returns in rdx:rax, no memory round-trip (see the shl/lea).
#   * At -O2 the frame-pointer prologue/epilogue disappears (demo.O2.s); compare
#     the three files to watch the same math shed every non-essential instruction.
# =============================================================================
