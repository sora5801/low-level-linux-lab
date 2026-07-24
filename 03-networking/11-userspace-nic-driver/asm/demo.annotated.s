# =============================================================================
# demo.annotated.s — the descriptor-ring poll kernels, explained instruction by
# instruction. Hand-annotated from demo.s (clang 20.1.8, -O1, Linux SysV ABI).
# =============================================================================
#
# HOW TO READ THIS FILE
# ---------------------
# AT&T syntax:  op  src, dst           # movl $1, %eax   =>  eax = 1
#   %reg        a register             $imm  an immediate literal
#   N(%base,%index)  memory at [base + index + N]      (index already scaled=1)
#   Register widths are the SAME register: rax(64)/eax(32)/ax(16)/al(8). Writing
#   eax zero-extends into rax, so clang uses 32-bit ops (movl/andl) whenever the
#   top half should be zero — it is one byte shorter than the 64-bit form.
#
# SYSTEM V AMD64 ABI (the contract every function here obeys)
# -----------------------------------------------------------
#   integer/pointer args, in order:  rdi, rsi, rdx, rcx, r8, r9   (then stack)
#   return value:                    rax  (eax for 32-bit results)
#   callee-saved (we must preserve): rbx, rbp, r12-r15, rsp
#   caller-saved (free scratch):     rax, rcx, rdx, rsi, rdi, r8-r11
#   the "red zone":                  128 bytes below rsp usable by leaf funcs
#   stack alignment:                 rsp % 16 == 0 at the point of a `call`
# These three functions are LEAF functions (they call nothing), so they need no
# stack locals — every value lives in a register. The only stack traffic is the
# frame-pointer prologue (-O1 keeps %rbp for debuggability) and saving %rbx,
# which two of them borrow as an extra scratch register and therefore, being
# callee-saved, must push/pop.
#
# THE THREE KERNELS
# -----------------
#   ring_next  wrap a ring index: (i+1) & (size-1)   -> ONE `and`, no modulo.
#   rx_poll    the RX hot loop: volatile DD-bit poll, barrier, length, advance.
#   tx_clean   reclaim TX descriptors a batch at a time via the same DD poll.
# The lesson lives in rx_poll: watch the descriptor-status load stay INSIDE the
# loop (it is volatile — the NIC writes DD by DMA, a cached copy would hang) and
# watch the ring wrap compile to a single AND because the size is a power of two.
# =============================================================================

	.file	"demo.c"
	.text

# =============================================================================
# u16 ring_next(u16 i, u16 size)   ->  return (i + 1) & (size - 1);
#   args:  rdi = i,  rsi = size          return: ax
# The entire point: with size a power of two, wraparound is a bitmask AND — no
# expensive `div`/modulo, no branch. This is why every descriptor ring is 2^n.
# =============================================================================
	.globl	ring_next
	.p2align	4                       # 16-byte align the entry (I-fetch friendly)
	.type	ring_next,@function
ring_next:
	pushq	%rbp                            # PROLOGUE: save caller's frame pointer
	movq	%rsp, %rbp                      #   rbp = frame base (kept for debug only)
	# (clang notes it is narrowing esi/edi to their 16-bit args here.)
	leal	1(%rdi), %ecx                   # ecx = i + 1   (LEA does the add without
	                                        #   touching flags; i came in edi)
	leal	-1(%rsi), %eax                  # eax = size - 1 = the low-bit wrap MASK
	andl	%ecx, %eax                      # eax = (i+1) & (size-1)  <- the wrap
	                                        #   result now in ax (u16 return)
	popq	%rbp                            # EPILOGUE: restore frame pointer
	retq                                    # return; ax holds the wrapped index
.Lfunc_end0:
	.size	ring_next, .Lfunc_end0-ring_next

# =============================================================================
# u32 rx_poll(struct desc *ring, u16 size, u16 *rx_index, u16 budget,
#             u32 *bytes_out)
#   args:  rdi = ring, rsi = size, rdx = rx_index, rcx = budget, r8 = bytes_out
#   return: eax = number of completed descriptors consumed
#
# struct desc is 16 bytes: addr @0 (8B), status @8 (4B), length @12 (4B).
# The loop mirrors ixgbe_rx_batch: load DD status (VOLATILE), stop if not done,
# barrier, add length, advance the ring index with the power-of-two mask.
# =============================================================================
	.globl	rx_poll
	.p2align	4
	.type	rx_poll,@function
rx_poll:
	pushq	%rbp                            # PROLOGUE: frame pointer
	movq	%rsp, %rbp
	pushq	%rbx                            # save callee-saved rbx (used as the
	                                        #   scratch address register below)
	movzwl	(%rdx), %r9d                    # r9 = *rx_index = idx (zero-extend u16)
	decl	%esi                            # esi = size - 1 = the wrap MASK
	xorl	%eax, %eax                      # eax = 0 = done (packets consumed)
	xorl	%r10d, %r10d                    # r10 = 0 = bytes accumulator
	jmp	.LBB1_1                         # enter the loop at the budget test

	.p2align	4
# ---- loop_latch: reached after processing a good descriptor, AND the shared
#      exit test for "descriptor not done". r11 holds the last status word. -----
.LBB1_4:                                    #  (label .LBB1_4)
	testb	$1, %r11b                       # test DD (bit 0) of the last status:
	je	.LBB1_5                         #   DD==0 -> leave loop (nothing new)
# ---- loop_header: check the packet budget -----------------------------------
.LBB1_1:                                    #  (label .LBB1_1)
	cmpl	%ecx, %eax                      # compare done (eax) vs budget (ecx)
	jae	.LBB1_5                         #   done >= budget -> stop the batch
# ---- body: load descriptor status (THE volatile poll load) ------------------
# %bb.2:
	movzwl	%r9w, %ebx                      # ebx = idx (zero-extended)
	shll	$4, %ebx                        # ebx = idx * 16  (sizeof(struct desc))
	movl	8(%rdi,%rbx), %r11d             # r11 = ring[idx].status  <-- VOLATILE
	                                        #   load: re-read from memory EVERY
	                                        #   iteration. The NIC sets DD here via
	                                        #   DMA; a hoisted/cached load would spin
	                                        #   forever on a stale "not done" value.
	testb	$1, %r11b                       # test the DD bit (status & RXD_STAT_DD)
	je	.LBB1_4                         #   not done -> exit path (re-tests, leaves)
# ---- bb.3: descriptor is done — barrier, consume length, advance ------------
# %bb.3:
	addq	%rdi, %rbx                      # rbx = &ring[idx]  (base + idx*16)
	#APP
	#NO_APP                                 # load_load_barrier(): the empty inline
	                                        #   asm. It emits NO instruction but is a
	                                        #   COMPILER fence — it forces the DD load
	                                        #   above to stay ordered before the
	                                        #   length load below. On x86-64 TSO the
	                                        #   hardware never reorders load-after-
	                                        #   load, so no fence opcode is needed;
	                                        #   on ARM/POWER this would be an acquire.
	addl	12(%rbx), %r10d                 # r10 += ring[idx].length (offset 12).
	                                        #   Read ONLY after the DD check — the NIC
	                                        #   wrote status+length in one writeback.
	incl	%r9d                            # idx++  (r9)
	andl	%esi, %r9d                      # idx &= (size-1)  <- power-of-two wrap
	incl	%eax                            # done++
	jmp	.LBB1_4                         # back to the shared latch (DD still set
	                                        #   in r11 -> falls through to loop_header)
# ---- loop_exit: write back the ring position and byte count -----------------
.LBB1_5:                                    #  (label .LBB1_5)
	movw	%r9w, (%rdx)                    # *rx_index = idx (persist SW head)
	movl	%r10d, (%r8)                    # *bytes_out = bytes
	popq	%rbx                            # EPILOGUE: restore callee-saved rbx
	popq	%rbp                            #   restore frame pointer
	retq                                    # return done (in eax)
.Lfunc_end1:
	.size	rx_poll, .Lfunc_end1-rx_poll

# =============================================================================
# u32 tx_clean(struct desc *ring, u16 size, u16 *clean_index, u16 tx_index,
#              u16 batch)
#   args:  rdi = ring, rsi = size, rdx = clean_index, rcx = tx_index, r8 = batch
#   return: eax = number of descriptors reclaimed
#
# Reclaim finished TX descriptors in fixed-size groups: only every batch-th
# descriptor carries Report-Status, so we test DD on the LAST descriptor of the
# next group; if set, the whole group is done. Note the ring-distance idiom
# (tx_index - clean) & mask to count outstanding entries. clang merged the two
# `break`s and the loop-continue into one flags-driven tail (the negl/and trick).
# =============================================================================
	.globl	tx_clean
	.p2align	4
	.type	tx_clean,@function
tx_clean:
	pushq	%rbp                            # PROLOGUE
	movq	%rsp, %rbp
	pushq	%rbx                            # save callee-saved rbx (scratch below)
	# (clang widens r8 (batch) to 64-bit here for addressing use.)
	movzwl	(%rdx), %r9d                    # r9 = *clean_index = clean
	decl	%esi                            # esi = size - 1 = the wrap MASK
	leal	-1(%r8), %r10d                  # r10 = batch - 1 (index of the RS desc)
	xorl	%eax, %eax                      # eax = 0 = freed
	jmp	.LBB2_1                         # enter loop at the outstanding test

	.p2align	4
# ---- break_underfull: taken when outstanding < batch. Zeroes r11 so the shared
#      exit test below falls through to loop_exit. (A rotated unconditional jmp.)
.LBB2_2:                                    #  (label .LBB2_2)
	xorl	%r11d, %r11d                    # r11 = 0
	testb	%r11b, %r11b                    # ZF = 1 (0 == 0)
	je	.LBB2_7                         #   always taken -> loop_exit
# ---- loop_header: compute outstanding = (tx_index - clean) & mask -----------
.LBB2_1:                                    #  (label .LBB2_1)
	movl	%ecx, %r11d                     # r11 = tx_index
	subl	%r9d, %r11d                     # r11 = tx_index - clean
	andl	%esi, %r11d                     # r11 = (tx_index - clean) & mask
	                                        #   = number of outstanding descriptors
	cmpw	%r8w, %r11w                     # compare outstanding vs batch (unsigned)
	jb	.LBB2_2                         #   outstanding < batch -> break_underfull
# ---- bb.3: probe the RS-marked descriptor's DD bit (VOLATILE load) ----------
# %bb.3:
	leal	(%r9,%r10), %r11d               # r11 = clean + (batch - 1)
	andl	%esi, %r11d                     # r11 = (clean + batch - 1) & mask = check
	movzwl	%r11w, %r11d                    # zero-extend the index
	shll	$4, %r11d                       # check * 16 (sizeof(struct desc))
	movl	8(%rdi,%r11), %r11d             # r11 = ring[check].status  <-- VOLATILE
	                                        #   load (NIC DMAs DD here; re-read live)
	andl	$1, %r11d                       # r11 = status & ADVTXD_STAT_DD (DD bit)
	je	.LBB2_5                         #   DD==0 -> group not sent; skip advance
# ---- bb.4: whole group is done — advance clean past the batch ---------------
# %bb.4:
	addl	%r8d, %r9d                      # clean += batch
	andl	%esi, %r9d                      # clean &= mask  (power-of-two wrap)
# ---- shared tail: freed += (DD ? batch : 0), then continue iff DD ------------
.LBB2_5:                                    #  (label .LBB2_5)
	movl	%r11d, %ebx                     # ebx = DD (1 if done, 0 if not)
	negl	%ebx                            # ebx = -DD  -> 0xFFFFFFFF if DD, else 0
	andl	%r8d, %ebx                      # ebx = batch & mask_of_DD = DD?batch:0
	addl	%ebx, %eax                      # freed += that  (branchless accumulate)
	testb	%r11b, %r11b                    # was DD set?
	jne	.LBB2_1                         #   yes -> loop again for the next group
	                                        #   no  -> fall through to loop_exit
# ---- loop_exit --------------------------------------------------------------
.LBB2_7:                                    #  (label .LBB2_7)
	movw	%r9w, (%rdx)                    # *clean_index = clean (persist progress)
	popq	%rbx                            # EPILOGUE: restore rbx
	popq	%rbp                            #   restore frame pointer
	retq                                    # return freed (in eax)
.Lfunc_end2:
	.size	tx_clean, .Lfunc_end2-tx_clean

	.ident	"clang version 20.1.8"
	.section	".note.GNU-stack","",@progbits  # non-executable stack (security default)
	.addrsig
# =============================================================================
# WHAT TO TAKE AWAY
#   * The volatile descriptor-status load (`movl 8(%rdi,%rbx), %r11d`) sits
#     INSIDE both hot loops and is re-issued every iteration. That is the DMA-
#     polling contract made visible: the NIC's DD write must be re-read from the
#     coherent cache line, never cached in a register.
#   * `load_load_barrier()` produced the bare `#APP/#NO_APP` marker and ZERO
#     opcodes — proof that on x86-64 a compiler fence is all the DD-before-length
#     ordering costs. A weakly-ordered ISA would show a real fence instruction.
#   * Ring wraparound is a single `andl %esi, %reg` in every function, never a
#     `div` — the dividend-free payoff of sizing rings to a power of two.
#   * At -O1 clang already merges branches (the negl/and conditional accumulate
#     in tx_clean) and rotates loops so the exit test reuses a live register.
#     Compare demo.O0.s (every value spilled to the stack) and demo.O2.s.
# =============================================================================
