/* ===========================================================================
 * demo.c — the beating heart of an io_uring ring, distilled to pure logic.
 * ===========================================================================
 *
 * DELIBERATELY self-contained: no headers, our own fixed-width types, so
 *     clang --target=x86_64-pc-linux-gnu -S
 * turns it into clean Linux/SysV assembly with nothing from libc in the way.
 * The real server (../echo_uring.c) leans on liburing, which cannot be compiled
 * to standalone asm — so we lift out the two operations the whole ring machinery
 * reduces to, which are ALSO the two the project spec calls out:
 *
 *   1. RING INDEX MASKING — a slot is (position & (entries-1)). The ring buffer
 *      never wraps a modulo; because `entries` is a power of two, the mask keeps
 *      a monotonically-increasing 32-bit head/tail pointing at the right slot,
 *      and the difference (tail - head) — computed in UNSIGNED arithmetic so it
 *      stays correct across the 2^32 wrap — is the number of occupied slots.
 *
 *   2. THE smp_store_release / smp_load_acquire HANDSHAKE on the head/tail
 *      indices. This is the ONLY synchronization between a userspace program and
 *      the kernel across the shared ring memory, and getting its memory ordering
 *      right is the entire correctness argument of io_uring. We model the kernel-
 *      facing indices with C11 atomics and the exact release/acquire the kernel
 *      uses (it is literally smp_store_release()/smp_load_acquire() in
 *      fs/io_uring.c). Watch, in the annotated asm, how on x86 a release store is
 *      just a plain `mov` — because x86-TSO already forbids the store-store and
 *      load-store reorderings a release must forbid — while the COMPILER barrier
 *      still does real work, and how the same source would emit `stlr`/`ldar` on
 *      ARM. That gap (same C, different machine barriers) is the lesson.
 *
 * The functions below mirror what liburing's io_uring_get_sqe / io_uring_submit
 * / io_uring_peek_cqe / io_uring_cq_advance / io_uring_buf_ring_advance do,
 * minus the mmap plumbing.
 * ===========================================================================
 */

/* Our own fixed-width types (LP64: int is 32-bit, long/pointer 64-bit on x86-64).
 * The ring indices are 32-bit, matching struct io_uring_sq/cq in the kernel. */
typedef unsigned int   u32;
typedef unsigned long  u64;
typedef int            i32;

/* C11 memory orders spelled numerically the way the compiler predefines them, so
 * we need no <stdatomic.h>. These map 1:1 onto __atomic_* builtin arguments:
 *   RELAXED = no ordering, just atomicity     (value 0)
 *   ACQUIRE = no later load/store floats above this load   (value 2)
 *   RELEASE = no earlier load/store sinks below this store  (value 3)
 * The kernel's smp_store_release == __ATOMIC_RELEASE store; smp_load_acquire ==
 * __ATOMIC_ACQUIRE load. We use exactly those. */
#define MO_RELAXED  __ATOMIC_RELAXED
#define MO_ACQUIRE  __ATOMIC_ACQUIRE
#define MO_RELEASE  __ATOMIC_RELEASE

/* A minimal Submission Queue Entry. The real struct io_uring_sqe is 64 bytes;
 * we keep only enough fields to make the stores in submit_one() meaningful. */
struct sqe {
    u32 opcode;      /* IORING_OP_* — what to do (accept/recv/send/...)         */
    i32 fd;          /* the fd (or fixed-file index) to act on                  */
    u64 user_data;   /* the cookie handed back verbatim in the matching CQE     */
};

/* A minimal Completion Queue Entry (the real one is 16 bytes: this IS it). */
struct cqe {
    u64 user_data;   /* echoes the SQE's cookie                                 */
    i32 res;         /* syscall-style result: bytes, new fd, or -errno          */
    u32 flags;       /* IORING_CQE_F_* (MORE, BUFFER, ...)                      */
};

/* The submission ring as userspace sees it after io_uring_setup + mmap.
 *   khead / ktail point INTO the shared mapping: the kernel is the CONSUMER of
 *   the SQ, so it owns `khead` (we acquire-load it to see how far it has drained)
 *   and we are the PRODUCER, so we own `ktail` (we release-store it to publish
 *   new SQEs). `entries` is a power of two; `mask` == entries-1.
 *   sqe_tail is our PRIVATE cached copy of the tail we bump as we fill SQEs, and
 *   only mirror into *ktail when we submit — so a burst of get_sqe()s touches
 *   shared memory (and the kernel) exactly once, at submit. */
struct sq {
    u32 *khead;        /* shared: kernel-updated consumer head (acquire-load)   */
    u32 *ktail;        /* shared: our producer tail (release-store)             */
    u32  mask;         /* entries - 1                                           */
    u32  entries;      /* ring capacity (power of two)                          */
    struct sqe *sqes;  /* the SQE array (also shared with the kernel)           */
    u32  sqe_tail;     /* private: next slot we will fill                       */
};

/* The completion ring. Here the roles flip: the kernel is the PRODUCER (it owns
 * `ktail`, which we acquire-load), and WE are the consumer (we own `khead`,
 * release-storing it once we have read a batch of CQEs so the kernel may reuse
 * the slots). */
struct cq {
    u32 *khead;        /* shared: our consumer head (release-store)             */
    u32 *ktail;        /* shared: kernel-updated producer tail (acquire-load)   */
    u32  mask;         /* entries - 1                                           */
    struct cqe *cqes;  /* the CQE array (shared)                                */
};

/* ---- 1. RING INDEX MASKING -------------------------------------------------
 * Turn a free-running 32-bit position into a physical slot index. Because the
 * ring size is a power of two, `& mask` is the same as `% entries` but with no
 * division — a single `and`. The position keeps counting up forever and is
 * allowed to wrap around 2^32; masking always lands on a valid slot. */
u32 ring_slot(u32 pos, u32 mask)
{
    return pos & mask;
}

/* ---- 2. how many SQ slots are still free -----------------------------------
 * occupied = tail - head, evaluated in UNSIGNED 32-bit arithmetic so that even
 * after the counters wrap past 2^32 the difference is still the true in-flight
 * count (modular subtraction). free = entries - occupied.
 *
 * We ACQUIRE-load the kernel's consumer head: everything the kernel did to those
 * SQ slots (marking them consumed) must be visible before we treat the slots as
 * reusable. On x86 the acquire load is a plain `mov`; the ordering it buys is
 * against OUR later accesses, enforced by the compiler barrier. */
u32 sq_space_left(const struct sq *r)
{
    u32 head = __atomic_load_n(r->khead, MO_ACQUIRE);   /* kernel's consumer head */
    return r->entries - (r->sqe_tail - head);           /* unsigned wrap-safe     */
}

/* ---- 3. reserve + fill one SQE ---------------------------------------------
 * The userspace half of io_uring_get_sqe(): if there is room, compute the slot
 * with the mask, write our request into the SHARED sqe array, bump our PRIVATE
 * tail, and return the slot index. Nothing is published yet — the kernel cannot
 * see this SQE until submit_one() releases the tail. Returns -1 if the SQ is
 * full (caller must submit and drain first). */
i32 get_sqe(struct sq *r, u32 opcode, i32 fd, u64 user_data)
{
    if (sq_space_left(r) == 0)
        return -1;                                       /* no room right now     */
    u32 idx = ring_slot(r->sqe_tail, r->mask);           /* physical slot         */
    struct sqe *s = &r->sqes[idx];
    s->opcode    = opcode;                               /* fill the entry...     */
    s->fd        = fd;
    s->user_data = user_data;
    r->sqe_tail++;                                        /* claim the slot (local)*/
    return (i32)idx;
}

/* ---- 4. THE STORE-RELEASE THAT PUBLISHES SUBMISSIONS -----------------------
 * io_uring_submit()'s core: make every SQE we staged visible to the kernel by
 * advancing the SHARED tail to our private tail with a RELEASE store.
 *
 * WHY RELEASE, precisely: the kernel does `tail = smp_load_acquire(sq->ktail)`
 * before it reads sqes[old_tail .. new_tail). The release here pairs with that
 * acquire: it guarantees that all our stores into the SQE bodies (opcode, fd,
 * user_data) in get_sqe() HAPPEN-BEFORE the kernel observes the new tail, so the
 * kernel can never read a half-written SQE. Publish the data, THEN the index —
 * and the release is what nails down that "then". Emit a relaxed store instead
 * and the kernel could, on a weakly-ordered CPU, see the new tail while the SQE
 * body is still stale garbage. */
void submit_one(struct sq *r)
{
    __atomic_store_n(r->ktail, r->sqe_tail, MO_RELEASE); /* smp_store_release     */
}

/* ---- 5. peek at one completion (the acquire side) --------------------------
 * io_uring_peek_cqe()'s core. We own the CQ head, so we read it with a plain
 * (relaxed) load — no other agent writes it. We ACQUIRE-load the kernel's
 * producer tail: this pairs with the kernel's release store of that tail, so
 * once we see tail > head, every field the kernel wrote into cqes[head] (res,
 * flags, user_data) is guaranteed visible to us. Skip the acquire and we could
 * read a CQE the kernel has "announced" via the tail but not finished filling.
 * Returns a pointer to the head CQE, or 0 (NULL) if the ring is empty. */
const struct cqe *peek_cqe(const struct cq *r, u32 *head_out)
{
    u32 head = __atomic_load_n(r->khead, MO_RELAXED);    /* we own head: relaxed  */
    u32 tail = __atomic_load_n(r->ktail, MO_ACQUIRE);    /* kernel's tail: acquire*/
    if (head == tail)
        return 0;                                        /* empty                 */
    *head_out = head;
    return &r->cqes[ring_slot(head, r->mask)];           /* masked slot           */
}

/* ---- 6. release consumed completions ---------------------------------------
 * io_uring_cq_advance()'s core. After reading `count` CQEs, advance the SHARED
 * head with a RELEASE store so the kernel may reuse those slots. The release
 * ensures our reads of the CQE bodies HAPPEN-BEFORE the kernel sees the advanced
 * head and overwrites them: consume the data, THEN free the slot. */
void cq_advance(struct cq *r, u32 count)
{
    u32 head = __atomic_load_n(r->khead, MO_RELAXED);    /* current head (ours)   */
    __atomic_store_n(r->khead, head + count, MO_RELEASE);/* publish new head      */
}

/* ---- 7. provided-buffer-ring tail advance ----------------------------------
 * The exact operation recycle_buf() performs in echo_uring.c via
 * io_uring_buf_ring_advance(): after writing `count` buffer descriptors into the
 * ring, publish them to the kernel with a RELEASE store of the buffer ring tail,
 * so the kernel (acquire-loading this tail before it picks a buffer for a recv)
 * never sees a descriptor slot we have not finished filling. Same release
 * discipline as submit_one(), on a different ring. */
void buf_ring_advance(u32 *btail, u32 count)
{
    u32 nt = __atomic_load_n(btail, MO_RELAXED) + count; /* we own our cached tail*/
    __atomic_store_n(btail, nt, MO_RELEASE);             /* store-release publish */
}
