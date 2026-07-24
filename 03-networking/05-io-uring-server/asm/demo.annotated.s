# =============================================================================
# demo.annotated.s — clang -O1 output for demo.c, explained instruction by
# instruction. This is the pure logic at the heart of an io_uring ring: the
# power-of-two INDEX MASKING and the RELEASE/ACQUIRE handshake on the shared
# head/tail indices that is the sole synchronization between userspace and the
# kernel.
# =============================================================================
#
# HOW TO READ THIS FILE
# ---------------------
# This is the EXACT assembly clang 20.1.8 emits for demo.c at -O1 (see demo.s for
# the untouched original), annotated. AT&T syntax throughout:
#
#     op   src, dst                      # movl %esi,%eax  =>  eax = esi
#     %reg      register   $imm immediate
#     (%reg)    memory at [reg]          N(%reg)   memory at [reg + N]
#     (%r1,%r2) memory at [r1 + r2]      N(%r1,%r2) memory at [r1 + r2 + N]
#
# A register's narrow names are the SAME physical register: rax/eax/ax/al.
# Writing a 32-bit name (eax) ZERO-EXTENDS into the 64-bit register, so the
# compiler prefers `movl` (5 bytes) over `movq` (7) when the top half is zero.
# The ring indices are 32-bit (u32), so almost everything here is `l`-suffixed.
#
# THE SYSTEM V AMD64 ABI (the contract every function below obeys)
# ----------------------------------------------------------------
#   * Integer/pointer arguments, in order:  rdi, rsi, rdx, rcx, r8, r9   (then stack)
#   * Return value:                          rax (eax for a 32-bit/int return)
#   * Caller-saved (scratch, callee may clobber): rax, rcx, rdx, rsi, rdi, r8-r11
#   * Callee-saved (callee MUST preserve):        rbx, rbp, r12-r15, rsp
#   * The RED ZONE: 128 bytes below rsp a leaf may use without moving rsp. Every
#     function here is a leaf (calls nobody) and spills nothing, so none touch it.
#   * Stack alignment: rsp % 16 == 0 at a `call`. Nothing here calls, so the only
#     stack traffic is the frame-pointer push kept by -fno-omit-frame-pointer
#     (debuggability only; the frame is otherwise dead).
#
# THE ONE BIG LESSON THIS FILE TEACHES
# ------------------------------------
# The C uses __atomic_load_n(..., ACQUIRE) and __atomic_store_n(..., RELEASE) —
# the very smp_load_acquire()/smp_store_release() the kernel uses on these same
# indices. Look at what they COMPILE TO on x86-64:
#
#     acquire load  ->  a plain `movl (%reg), %reg`      (NO fence)
#     release store ->  a plain `movl %reg, (%reg)`      (NO fence)
#                       or even a memory-destination `addl %reg, (%reg)`
#
# There is not a single `mfence`, `lock`, `stlr`, or `ldar`. That is NOT a bug:
# the x86-TSO memory model ALREADY forbids the reorderings a release store and an
# acquire load must forbid (store->store and load->store stay in program order;
# loads are not reordered with older loads). So on x86 the ONLY thing these
# builtins buy is a COMPILER barrier — they stop clang from hoisting the SQE-body
# stores below the tail store, or sinking CQE-body loads above the tail load.
# On a weakly-ordered ISA (ARM/RISC-V) the SAME C source emits real barrier
# instructions (`stlr`/`ldar` or `dmb`). Reading this asm is how you SEE that the
# cost of correct io_uring ordering on x86 is *zero instructions* — the model
# does the work — which is a big part of why io_uring is fast.
# =============================================================================

	.file	"demo.c"
	.text

# =============================================================================
# ring_slot(u32 pos /*edi*/, u32 mask /*esi*/) -> u32 /*eax*/
#   return pos & mask;
# The whole ring never computes a modulo: because entries is a power of two,
# mask == entries-1 and (pos & mask) == (pos % entries) in ONE `and`. `pos` is a
# free-running counter allowed to wrap at 2^32; the mask always lands on a slot.
# =============================================================================
	.globl	ring_slot
	.p2align	4                       # 16-byte-align the entry (I-fetch friendly)
	.type	ring_slot,@function
ring_slot:
	pushq	%rbp                        # PROLOGUE: save caller's frame pointer
	movq	%rsp, %rbp                  #   establish our frame (debug only here)
	movl	%edi, %eax                  # eax = pos
	andl	%esi, %eax                  # eax = pos & mask  = the physical slot index
	popq	%rbp                        # EPILOGUE: restore caller's frame pointer
	retq                                # return slot in eax
.Lfunc_end0:
	.size	ring_slot, .Lfunc_end0-ring_slot

# =============================================================================
# sq_space_left(const struct sq *r /*rdi*/) -> u32 /*eax*/
#   head = acquire_load(*r->khead);  return r->entries - (r->sqe_tail - head);
# struct sq offsets:  khead=0  ktail=8  mask=16  entries=20  sqes=24  sqe_tail=32
# The compiler reassociated  entries-(sqe_tail-head)  into  head+entries-sqe_tail
# so it can stream three memory operands into one register with add/sub.
# =============================================================================
	.globl	sq_space_left
	.p2align	4
	.type	sq_space_left,@function
sq_space_left:
	pushq	%rbp                        # PROLOGUE
	movq	%rsp, %rbp
	movq	(%rdi), %rax                # rax = r->khead  (the POINTER at offset 0)
	movl	(%rax), %eax                # eax = *khead = head.  <== ACQUIRE-LOAD of the
	                                    #   kernel's consumer head. On x86 an acquire
	                                    #   load is just this plain `movl`; the ordering
	                                    #   it guarantees (nothing below floats above it)
	                                    #   is enforced by the compiler + x86-TSO, at
	                                    #   zero instruction cost.
	addl	20(%rdi), %eax              # eax = head + r->entries   (entries at +20)
	subl	32(%rdi), %eax              # eax = head + entries - r->sqe_tail (tail at +32)
	                                    #   = entries - (sqe_tail - head), the free count.
	                                    #   All 32-bit UNSIGNED, so it stays correct even
	                                    #   after the counters wrap past 2^32.
	popq	%rbp                        # EPILOGUE
	retq                                # return free-slot count in eax
.Lfunc_end1:
	.size	sq_space_left, .Lfunc_end1-sq_space_left

# =============================================================================
# get_sqe(struct sq *r /*rdi*/, u32 opcode /*esi*/, i32 fd /*edx*/, u64 ud /*rcx*/)
#        -> i32 /*eax*/
#   if (sq_space_left(r)==0) return -1;
#   idx = sqe_tail & mask;  s=&sqes[idx];  s->opcode=..; s->fd=..; s->user_data=..;
#   sqe_tail++;  return idx;
# sq_space_left() was INLINED. Note space==0  <=>  head+entries == sqe_tail, which
# is exactly the compare below — no subtraction needed for the emptiness test.
# =============================================================================
	.globl	get_sqe
	.p2align	4
	.type	get_sqe,@function
get_sqe:
	pushq	%rbp                        # PROLOGUE
	movq	%rsp, %rbp
	movq	(%rdi), %rax                # rax = r->khead pointer
	movl	(%rax), %r9d                # r9d = *khead = head   (inlined ACQUIRE-load)
	movl	32(%rdi), %r8d              # r8d = r->sqe_tail
	addl	20(%rdi), %r9d              # r9d = head + r->entries
	movl	$-1, %eax                   # eax = -1  (the "SQ full" return, set eagerly)
	cmpl	%r8d, %r9d                  # compare (head+entries) vs sqe_tail
	je	.LBB2_2                         # if equal -> space_left==0 -> return -1
# ---- there is room: fill one SQE -------------------------------------------
	movl	16(%rdi), %eax              # eax = r->mask
	andl	%r8d, %eax                  # eax = sqe_tail & mask = idx  (RING MASKING)
	movq	24(%rdi), %r9               # r9  = r->sqes  (base of the SQE array)
	movq	%rax, %r10                  # r10 = idx
	shlq	$4, %r10                    # r10 = idx * 16  (sizeof(struct sqe)=16 bytes),
	                                    #   the BYTE offset of sqes[idx]
	movl	%esi, (%r9,%r10)            # sqes[idx].opcode = opcode      (field @ +0)
	movl	%edx, 4(%r9,%r10)           # sqes[idx].fd = fd              (field @ +4)
	movq	%rcx, 8(%r9,%r10)           # sqes[idx].user_data = ud       (field @ +8)
	                                    #   These three stores are the SQE BODY. They
	                                    #   MUST become visible to the kernel before the
	                                    #   tail is released in submit_one() — see there.
	incl	%r8d                        # sqe_tail + 1
	movl	%r8d, 32(%rdi)              # r->sqe_tail = sqe_tail+1  (claim the slot,
	                                    #   PRIVATE tail only; kernel can't see it yet)
	                                    # eax already holds idx from the `andl` above.
.LBB2_2:
	                                    # (clang note: eax holds idx, or -1 if we jumped)
	popq	%rbp                        # EPILOGUE
	retq                                # return idx (or -1) in eax
.Lfunc_end2:
	.size	get_sqe, .Lfunc_end2-get_sqe

# =============================================================================
# submit_one(struct sq *r /*rdi*/)   -- io_uring_submit()'s core
#   release_store(*r->ktail, r->sqe_tail);
# THE headline instruction of the whole file. Publishing our staged SQEs to the
# kernel is a single store of the private tail into the SHARED ktail. Because the
# builtin is __ATOMIC_RELEASE, clang guarantees the SQE-body stores in get_sqe()
# are not reordered after it; on x86-TSO that guarantee is FREE, so the release
# is literally a plain `movl` with no fence.
# =============================================================================
	.globl	submit_one
	.p2align	4
	.type	submit_one,@function
submit_one:
	pushq	%rbp                        # PROLOGUE
	movq	%rsp, %rbp
	movq	8(%rdi), %rax               # rax = r->ktail  (the SHARED tail POINTER, +8)
	movl	32(%rdi), %ecx              # ecx = r->sqe_tail  (our private producer tail)
	movl	%ecx, (%rax)                # *ktail = sqe_tail.  <== smp_store_release.
	                                    #   Just a `movl`: no mfence/lock. The kernel
	                                    #   acquire-LOADS this word before reading the
	                                    #   SQEs; the release/acquire pair makes every
	                                    #   SQE-body store above HAPPEN-BEFORE the kernel
	                                    #   sees the new tail. Publish data, THEN index.
	popq	%rbp                        # EPILOGUE
	retq
.Lfunc_end3:
	.size	submit_one, .Lfunc_end3-submit_one

# =============================================================================
# peek_cqe(const struct cq *r /*rdi*/, u32 *head_out /*rsi*/) -> const cqe* /*rax*/
#   head = relaxed_load(*r->khead);  tail = acquire_load(*r->ktail);
#   if (head==tail) return 0;
#   *head_out = head;  return &r->cqes[head & r->mask];
# struct cq offsets:  khead=0  ktail=8  mask=16  cqes=24
# The roles are FLIPPED vs the SQ: on the CQ the kernel is the producer (owns
# ktail, we acquire-load it) and we are the consumer (own khead, relaxed-load).
# =============================================================================
	.globl	peek_cqe
	.p2align	4
	.type	peek_cqe,@function
peek_cqe:
	pushq	%rbp                        # PROLOGUE
	movq	%rsp, %rbp
	movq	(%rdi), %rax                # rax = r->khead pointer
	movl	(%rax), %eax                # eax = *khead = head.  RELAXED load: WE are the
	                                    #   only writer of khead, so no ordering needed.
	movq	8(%rdi), %rcx               # rcx = r->ktail pointer
	movl	(%rcx), %ecx                # ecx = *ktail = tail.  <== ACQUIRE-load of the
	                                    #   kernel's producer tail. Pairs with the
	                                    #   kernel's release store of tail, so once we
	                                    #   see tail>head, the CQE body at cqes[head]
	                                    #   (res/flags/user_data) is guaranteed visible.
	cmpl	%ecx, %eax                  # compare head vs tail
	jne	.LBB4_2                         # if head != tail -> a CQE is ready
# ---- empty ring: return NULL -----------------------------------------------
	xorl	%eax, %eax                  # eax = 0  (NULL). `xor r,r` is the 2-byte idiom
	                                    #   for zeroing and is a CPU dependency-breaker.
	popq	%rbp
	retq
.LBB4_2:                                # ---- non-empty: hand back &cqes[head&mask] ----
	movl	%eax, (%rsi)                # *head_out = head  (caller passes it to cq_advance)
	andl	16(%rdi), %eax              # eax = head & r->mask = slot   (RING MASKING)
	shlq	$4, %rax                    # rax = slot * 16  (sizeof(struct cqe)=16 bytes)
	addq	24(%rdi), %rax              # rax = r->cqes + slot*16 = &cqes[slot]  (return)
	popq	%rbp                        # EPILOGUE
	retq                                # return the CQE pointer in rax
.Lfunc_end4:
	.size	peek_cqe, .Lfunc_end4-peek_cqe

# =============================================================================
# cq_advance(struct cq *r /*rdi*/, u32 count /*esi*/)  -- io_uring_cq_advance()'s core
#   head = relaxed_load(*r->khead);  release_store(*r->khead, head + count);
# After reading `count` CQEs, free their slots by advancing the SHARED head with
# a RELEASE store, so the kernel (which acquire-loads head before REUSING a slot)
# never overwrites a CQE we have not read. Consume data, THEN free the slot.
# =============================================================================
	.globl	cq_advance
	.p2align	4
	.type	cq_advance,@function
cq_advance:
	pushq	%rbp                        # PROLOGUE
	movq	%rsp, %rbp
	movq	(%rdi), %rax                # rax = r->khead pointer
	addl	%esi, (%rax)                # *khead += count.  <== the load(head)+store(head+
	                                    #   count) RELEASE fused into ONE memory-
	                                    #   destination add. It is NOT lock-prefixed and
	                                    #   need not be: WE are khead's only writer, so a
	                                    #   non-atomic read-modify-write is safe, and the
	                                    #   aligned 32-bit store it performs is atomic and
	                                    #   (x86-TSO) release-ordered w.r.t. our prior
	                                    #   CQE reads. On ARM this would be ldxr/stlxr or
	                                    #   a load + `stlr`.
	popq	%rbp                        # EPILOGUE
	retq
.Lfunc_end5:
	.size	cq_advance, .Lfunc_end5-cq_advance

# =============================================================================
# buf_ring_advance(u32 *btail /*rdi*/, u32 count /*esi*/)
#        -- io_uring_buf_ring_advance()'s core, used by recycle_buf() in the server
#   btail is passed BY POINTER (not inside a struct), so the release store is a
#   memory-destination add straight through rdi. Same discipline as submit_one():
#   after writing buffer descriptors into the provided-buffer ring, publish them
#   to the kernel with a release store of the ring tail, so the kernel never
#   picks a buffer slot we have not finished filling.
# =============================================================================
	.globl	buf_ring_advance
	.p2align	4
	.type	buf_ring_advance,@function
buf_ring_advance:
	pushq	%rbp                        # PROLOGUE
	movq	%rsp, %rbp
	addl	%esi, (%rdi)                # *btail += count.  <== smp_store_release, again a
	                                    #   plain (unlocked) memory add: publish the
	                                    #   buffers by bumping the tail the kernel
	                                    #   acquire-loads. No fence on x86.
	popq	%rbp                        # EPILOGUE
	retq
.Lfunc_end6:
	.size	buf_ring_advance, .Lfunc_end6-buf_ring_advance

	.ident	"clang version 20.1.8"
	.section	".note.GNU-stack","",@progbits   # non-executable stack (security default)
	.addrsig                                     # address-significance table (LTO metadata)
# =============================================================================
# WHAT TO TAKE AWAY
#   * A ring slot is `position & (entries-1)` — one `and`, no modulo, because the
#     ring size is a power of two and the position counter is allowed to wrap.
#     (ring_slot, and the `andl` inside get_sqe / peek_cqe.)
#   * Occupancy is `tail - head` in UNSIGNED 32-bit math, wrap-safe by
#     construction; free space is `entries - (tail-head)`. (sq_space_left.)
#   * The producer publishes with a RELEASE store of the tail AFTER filling the
#     entry; the consumer reads with an ACQUIRE load of the tail BEFORE reading
#     the entry. That single pair is the whole userspace<->kernel contract.
#     (submit_one/buf_ring_advance release; sq_space_left/peek_cqe acquire.)
#   * On x86-64 those releases and acquires cost ZERO extra instructions — plain
#     `movl`/`addl`, no `mfence`, no `lock` — because x86-TSO already orders them;
#     the builtins' only job here is to fence the COMPILER. The identical C emits
#     `stlr`/`ldar` on ARM. Seeing that is the point of reading the asm.
#   * Compare with demo.O0.s (every value spilled to the stack, the release store
#     still a plain mov) and demo.O2.s (same code, tighter register allocation).
# =============================================================================
