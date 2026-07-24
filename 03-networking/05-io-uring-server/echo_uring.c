/* ===========================================================================
 * echo_uring.c — a fully asynchronous TCP echo server built on io_uring.
 * ===========================================================================
 *
 * This is the flagship of the project. There is exactly ONE thread and ONE
 * blocking syscall in the steady state — io_uring_enter(2), hidden inside
 * liburing's io_uring_submit_and_wait(). Everything else — accept, recv, send,
 * close — is handed to the kernel as a Submission Queue Entry (SQE) and reaped
 * later as a Completion Queue Entry (CQE). We never call accept(), recv(),
 * send(), or close() directly.
 *
 * Compare this with epoll (epoll_echo.c): epoll only tells you a fd is *ready*;
 * you still make the read()/write() syscalls yourself, one per operation. With
 * io_uring the kernel performs the operation AND tells you the result, so a
 * single io_uring_enter can carry dozens of submissions and reap dozens of
 * completions. Add the "multishot" variants (one SQE that fires many CQEs) and
 * the per-message syscall count on the hot path drops toward zero. That is the
 * whole point of the exercise; the README walks the numbers.
 *
 * ---------------------------------------------------------------------------
 * THE FOUR ADVANCED FEATURES THIS SERVER USES (and why)
 * ---------------------------------------------------------------------------
 *   1. MULTISHOT ACCEPT   — one ACCEPT SQE that stays armed and emits a CQE per
 *                           incoming connection. No re-arming syscall per accept.
 *   2. MULTISHOT RECV     — one RECV SQE per connection that stays armed and
 *                           emits a CQE per arriving chunk. No re-arm per read.
 *   3. PROVIDED BUFFER RING — a pool of receive buffers registered with the
 *                           kernel. The kernel PICKS a buffer for each recv and
 *                           tells us which one via the CQE. We never pre-post a
 *                           buffer per read, and we never guess a size. This is
 *                           the registered-buffer variant that pairs with
 *                           multishot recv (see the note on FIXED buffers below).
 *   4. REGISTERED FILES   — the listening socket is registered in the ring's
 *                           fixed-file table (index 0) and the accept SQE refers
 *                           to it by index with IOSQE_FIXED_FILE, saving the
 *                           kernel an fd -> struct file lookup + refcount per op.
 *
 * A NOTE ON "FIXED BUFFERS" vs "PROVIDED BUFFERS" (both are "registered"):
 *   - FIXED buffers (io_uring_register_buffers + READ_FIXED/WRITE_FIXED): YOU
 *     pin a fixed set of iovecs; each I/O names one by index. Great when each
 *     connection owns one buffer for its lifetime. The kernel skips get_user_
 *     pages() on every I/O because the pages are already pinned.
 *   - PROVIDED buffers (this file): a RING of buffers the kernel draws from on
 *     demand. This is what multishot recv needs, because a multishot op cannot
 *     know in advance how many buffers it will consume. We use provided buffers
 *     for exactly that reason; the README's "Going further" shows the READ_FIXED
 *     path. Both avoid re-pinning user memory on the hot path.
 *
 * ---------------------------------------------------------------------------
 * PLATFORM: Linux >= 5.19 for provided buffer *rings* + multishot recv; the
 * code degrades in features, not correctness, on newer kernels. Needs liburing
 * >= 2.3 (io_uring_setup_buf_ring, io_uring_prep_recv_multishot). No special
 * privilege is required for a plain TCP server on a high port.
 * ===========================================================================
 */
#include "common.h"
#include <liburing.h>    /* the whole io_uring userspace API                   */
#include <stdint.h>      /* uint64_t for the user_data cookie                  */

/* ---- tunables -------------------------------------------------------------
 * SQ/CQ depth: how many SQEs we can stage before submitting. The CQ is sized to
 * 2x by default so completions cannot overflow while we batch submissions. */
#define QUEUE_DEPTH    4096

/* Provided-buffer ring geometry. NBUFS MUST be a power of two — the ring is
 * indexed by (tail & (NBUFS-1)), exactly the masking trick asm/demo.c distills.
 * BUF_SIZE is the most bytes one recv can deliver into a single buffer. */
#define NBUFS          2048          /* power of two: buffers in the pool       */
#define BUF_SIZE       2048          /* bytes per buffer                        */
#define BUF_GROUP_ID   1             /* our chosen id for this buffer group     */

#define LISTEN_FIXED_IDX 0           /* the listener's slot in the fixed-file table */

/* ---------------------------------------------------------------------------
 * user_data: io_uring hands back, unchanged, the 64-bit cookie we stamped on
 * each SQE. It is the ONLY thing tying a CQE back to what we asked for, so we
 * pack everything the completion handler needs into it — no per-connection
 * allocation, no lookup table. Layout (56 bits used of 64):
 *
 *     bits 63..56 : op type  (ACCEPT / RECV / SEND / CLOSE)
 *     bits 47..32 : buffer id (only meaningful for SEND: which pool buffer to
 *                              recycle once the echo has been transmitted)
 *     bits 31..0  : the connection fd (a normal fd; fits in 32 bits)
 *
 * This is a common io_uring idiom: user_data is a free 8-byte scratch word, so
 * use it as a tagged union instead of chasing pointers in the hot path.
 * --------------------------------------------------------------------------- */
enum op_type { OP_ACCEPT = 0, OP_RECV = 1, OP_SEND = 2, OP_CLOSE = 3 };

static inline uint64_t ud_make(enum op_type op, unsigned bid, int fd)
{
    return ((uint64_t)op  << 56)
         | ((uint64_t)(bid & 0xffff) << 32)
         | (uint64_t)(uint32_t)fd;
}
static inline enum op_type ud_op(uint64_t ud)  { return (enum op_type)(ud >> 56); }
static inline unsigned     ud_bid(uint64_t ud) { return (unsigned)((ud >> 32) & 0xffff); }
static inline int          ud_fd(uint64_t ud)  { return (int)(uint32_t)ud; }

/* ---------------------------------------------------------------------------
 * Global server state. A single-threaded server has no data races, so none of
 * this is atomic or locked. (The ONLY cross-thread ordering in an io_uring
 * program lives between US and the KERNEL, on the ring head/tail indices — and
 * liburing's store-release/acquire-load handle that for us. See the memory-
 * ordering discussion in the README and the distilled version in asm/demo.c.)
 * --------------------------------------------------------------------------- */
static struct io_uring        ring;      /* the ring pair (SQ + CQ)             */
static struct io_uring_buf_ring *buf_ring;/* the provided-buffer ring           */
static unsigned char         *buf_base;  /* backing store: NBUFS * BUF_SIZE     */

/* Address of provided buffer number `bid` in the backing store. The kernel told
 * us `bid`; we recompute the pointer the same way we registered it. */
static inline unsigned char *buf_addr(unsigned bid)
{
    return buf_base + (size_t)bid * BUF_SIZE;
}

/* get_sqe_or_submit — obtain a free SQE, flushing the SQ if it is momentarily
 * full. io_uring_get_sqe returns NULL when every SQE between the kernel's
 * consumer head and our producer tail is in use; the cure is to submit what we
 * have (advancing the tail so the kernel drains it) and ask again. In practice
 * QUEUE_DEPTH is large enough that the retry almost never triggers, but a server
 * must never assume an SQE is available. */
static struct io_uring_sqe *get_sqe_or_submit(void)
{
    struct io_uring_sqe *sqe = io_uring_get_sqe(&ring);
    if (sqe)
        return sqe;
    io_uring_submit(&ring);              /* push the tail; kernel starts draining */
    sqe = io_uring_get_sqe(&ring);
    if (!sqe) {
        /* Truly wedged (should be impossible with a 4096-deep SQ). Better to die
         * loudly than to silently drop a connection's I/O. */
        die("io_uring_get_sqe: submission queue exhausted");
    }
    return sqe;
}

/* ---- arm the ONE multishot accept ----------------------------------------
 * A multishot ACCEPT stays armed in the kernel: every completed TCP handshake
 * produces its own CQE (res = the new connection fd) while IORING_CQE_F_MORE
 * stays set, meaning "still armed, more CQEs coming from this same SQE." We only
 * ever re-arm if the kernel drops F_MORE (a terminal error on the listener).
 *
 * The listener is referenced by its FIXED-FILE index, not its raw fd:
 * IOSQE_FIXED_FILE tells the kernel "the fd field is an index into the
 * registered file table," letting it skip the fd -> file translation each accept.*/
static void arm_accept(void)
{
    struct io_uring_sqe *sqe = get_sqe_or_submit();
    /* prep_multishot_accept(sqe, fd, addr, addrlen, flags). We pass NULL for the
     * peer address (an echo server does not need it) and 0 flags. `fd` here is
     * the fixed-file INDEX because we set IOSQE_FIXED_FILE next. */
    io_uring_prep_multishot_accept(sqe, LISTEN_FIXED_IDX, NULL, NULL, 0);
    io_uring_sqe_set_flags(sqe, IOSQE_FIXED_FILE);
    io_uring_sqe_set_data64(sqe, ud_make(OP_ACCEPT, 0, LISTEN_FIXED_IDX));
}

/* ---- arm a multishot recv on a freshly-accepted connection ----------------
 * One RECV SQE that stays armed and fires a CQE per arriving chunk. Because it
 * is multishot it does NOT carry its own buffer — it draws one from our provided
 * buffer group on each fire. IOSQE_BUFFER_SELECT says "pick a buffer from group
 * buf_group for me"; the CQE will report which buffer via IORING_CQE_F_BUFFER.
 *
 * The connection fd is a NORMAL fd (multishot accept returned a real fd, not a
 * direct descriptor), so we do NOT set IOSQE_FIXED_FILE here. */
static void arm_recv(int conn_fd)
{
    struct io_uring_sqe *sqe = get_sqe_or_submit();
    /* prep_recv_multishot(sqe, sockfd, buf=NULL, len=0, flags=0): buf/len are
     * zero precisely because the buffer comes from the provided group. */
    io_uring_prep_recv_multishot(sqe, conn_fd, NULL, 0, 0);
    io_uring_sqe_set_flags(sqe, IOSQE_BUFFER_SELECT);
    sqe->buf_group = BUF_GROUP_ID;       /* which provided-buffer pool to draw from */
    io_uring_sqe_set_data64(sqe, ud_make(OP_RECV, 0, conn_fd));
}

/* ---- echo: send the bytes we just received, straight from the pool buffer ---
 * We transmit directly out of the provided buffer the recv landed in — zero
 * copy. CRITICAL OWNERSHIP RULE: that buffer now belongs to this in-flight send
 * and must NOT be recycled back to the pool until the SEND completes, or the
 * kernel could hand the same buffer to another recv and overwrite our data mid-
 * flight. We therefore stash the buffer id in the send's user_data and only
 * recycle it when the SEND CQE arrives.
 *
 * ORDERING CAVEAT (honest scope): io_uring does NOT guarantee that two unlinked
 * SEND SQEs on the same socket transmit in submission order — the kernel may run
 * them on different workers. For a byte-stream echo that would matter if one
 * message spanned multiple recv buffers with multiple in-flight sends. Our
 * closed-loop client keeps exactly one message in flight per connection, so this
 * is never exercised; a PIPELINING client would need per-connection send
 * serialization (queue the next send until the previous completes) or IOSQE_IO_LINK
 * chaining. See the README "Going further". */
static void arm_send(int conn_fd, unsigned bid, unsigned len)
{
    struct io_uring_sqe *sqe = get_sqe_or_submit();
    /* prep_send(sqe, sockfd, buf, len, flags). flags=0: a plain send. */
    io_uring_prep_send(sqe, conn_fd, buf_addr(bid), len, 0);
    io_uring_sqe_set_data64(sqe, ud_make(OP_SEND, bid, conn_fd));
}

/* ---- async close ----------------------------------------------------------
 * IORING_OP_CLOSE closes the fd from inside the kernel worker, so even teardown
 * never costs us a synchronous close() syscall. */
static void arm_close(int conn_fd)
{
    struct io_uring_sqe *sqe = get_sqe_or_submit();
    io_uring_prep_close(sqe, conn_fd);
    io_uring_sqe_set_data64(sqe, ud_make(OP_CLOSE, 0, conn_fd));
}

/* ---- return a provided buffer to the pool ---------------------------------
 * The mirror image of the initial fill. io_uring_buf_ring_add writes the buffer
 * back into the ring's next slot (indexed by our cached tail & mask); then
 * io_uring_buf_ring_advance publishes it with a STORE-RELEASE of the ring tail.
 *
 * WHY STORE-RELEASE: the kernel does an ACQUIRE-load of this tail before reading
 * a buffer descriptor out of the ring. The release/acquire pair guarantees that
 * every field we wrote into the buffer-ring slot (addr, len, bid) is visible to
 * the kernel BEFORE it observes the advanced tail. Publish the data, THEN the
 * index. Reverse the order and the kernel could read a slot we have not finished
 * filling. On x86-TSO the release is a plain store (the ISA already orders
 * store->store), but the compiler barrier still matters and on ARM this lowers
 * to a real `stlr`. asm/demo.c distills exactly this. */
static void recycle_buf(unsigned bid)
{
    io_uring_buf_ring_add(buf_ring,
                          buf_addr(bid),                 /* addr of the buffer   */
                          BUF_SIZE,                      /* its full capacity    */
                          (unsigned short)bid,           /* its id               */
                          io_uring_buf_ring_mask(NBUFS), /* tail & mask indexing */
                          0);                            /* buffer offset in add */
    io_uring_buf_ring_advance(buf_ring, 1);              /* store-release the tail */
}

/* ===========================================================================
 * Completion handlers — one per op type. Each is handed the CQE the kernel
 * produced. cqe->res is the syscall-style result (bytes, new fd, or -errno);
 * cqe->flags carries io_uring metadata (F_MORE = op still armed, F_BUFFER =
 * a provided buffer was consumed and its id is in the high bits).
 * ===========================================================================
 */

static void on_accept(struct io_uring_cqe *cqe)
{
    int res = cqe->res;

    if (res >= 0) {
        /* res is a brand-new connection fd. Turn off Nagle so tiny echoes are
         * not delayed, then arm its multishot recv. Best-effort NODELAY. */
        int conn_fd = res;
        (void)set_nodelay(conn_fd);
        arm_recv(conn_fd);
    } else {
        /* A negative res on accept is usually transient (-EMFILE/-ENFILE: out of
         * fds; -ECONNABORTED: client bailed mid-handshake). We just log; there is
         * no connection to clean up. -ECANCELED means the op was cancelled. */
        if (res != -ECANCELED)
            fprintf(stderr, "accept: %s\n", strerror(-res));
    }

    /* If the kernel dropped F_MORE, the multishot accept is no longer armed
     * (terminal listener error) — re-arm a fresh one so we keep accepting.
     * On the normal path F_MORE stays set and we do nothing. */
    if (!(cqe->flags & IORING_CQE_F_MORE))
        arm_accept();
}

static void on_recv(struct io_uring_cqe *cqe)
{
    int conn_fd = ud_fd(io_uring_cqe_get_data64(cqe));
    int res     = cqe->res;

    /* Did this completion consume a provided buffer? (A 0-byte or errored recv
     * usually does not.) If so, extract its id from the CQE flags: the buffer id
     * lives in the top bits, shifted by IORING_CQE_BUFFER_SHIFT. */
    int have_buf = (cqe->flags & IORING_CQE_F_BUFFER) != 0;
    unsigned bid = have_buf ? (cqe->flags >> IORING_CQE_BUFFER_SHIFT) : 0;

    if (res > 0) {
        /* Defensive: a buffer-select recv with data ALWAYS reports its buffer via
         * IORING_CQE_F_BUFFER — but if the flag were ever absent we would not know
         * which buffer holds the bytes, so we must not fabricate bid=0 and echo
         * from a buffer that may be in use. Treat it as "nothing to echo" and just
         * keep the recv armed. In practice this branch never runs. */
        if (!have_buf) {
            if (!(cqe->flags & IORING_CQE_F_MORE))
                arm_recv(conn_fd);
            return;
        }

        /* Normal data path. Echo it straight back out of the pool buffer. The
         * buffer is now owned by that send and will be recycled on its CQE. */
        arm_send(conn_fd, bid, (unsigned)res);

        /* Multishot recv keeps itself armed as long as F_MORE is set. If the
         * kernel cleared it (e.g. it momentarily ran out of internal room), we
         * must re-arm a new multishot recv or this connection goes deaf. */
        if (!(cqe->flags & IORING_CQE_F_MORE))
            arm_recv(conn_fd);
        return;
    }

    /* res <= 0 from here: the connection is going away or stalling.
     * If it still handed us a buffer, give it back before we forget the id. */
    if (have_buf)
        recycle_buf(bid);

    if (res == 0) {
        /* res == 0 is a clean EOF: the peer sent FIN. Multishot recv terminates
         * (F_MORE is clear). Close our end asynchronously. */
        arm_close(conn_fd);
        return;
    }

    /* res < 0: an error code. */
    if (res == -ENOBUFS) {
        /* The provided pool was momentarily empty (all buffers out on in-flight
         * sends). The multishot recv self-terminated (F_MORE clear). Buffers
         * free up as those sends complete and recycle, so simply re-arm the recv
         * — the retry will find buffers available. Not fatal, do not close. */
        arm_recv(conn_fd);
        return;
    }
    if (res == -ECANCELED) {
        /* Op cancelled (e.g. we closed the fd). Nothing to do. */
        return;
    }
    /* -ECONNRESET, -ETIMEDOUT, etc: peer misbehaved. Log once and close. */
    fprintf(stderr, "recv(fd=%d): %s\n", conn_fd, strerror(-res));
    arm_close(conn_fd);
}

static void on_send(struct io_uring_cqe *cqe)
{
    uint64_t ud      = io_uring_cqe_get_data64(cqe);
    int      conn_fd = ud_fd(ud);
    unsigned bid     = ud_bid(ud);
    int      res     = cqe->res;

    /* The send is done (success or failure): the buffer is ours again. Recycle
     * it back to the provided pool so a future recv can use it. This is the
     * release half of the ownership rule established in arm_send(). */
    recycle_buf(bid);

    if (res < 0) {
        /* -EPIPE / -ECONNRESET: the client vanished mid-echo. Close our side.
         * (-ECANCELED just means the fd was already being torn down.) */
        if (res != -ECANCELED)
            arm_close(conn_fd);
        return;
    }

    /* SCOPE / HONESTY: we treat a successful send as fully transmitted. For a
     * small echo payload into a socket with room in its send buffer that is
     * essentially always true, but a robust server MUST handle a SHORT send
     * (res < len) by re-issuing the untransmitted tail. Doing that here would
     * require tracking each send's original length (there is no room left in the
     * 56-bit user_data), so the teaching core documents the gap instead of
     * hiding it. See the README "Going further". */
}

/* ===========================================================================
 * main — set everything up, then loop forever: submit staged SQEs + wait for at
 * least one completion, drain the whole CQ batch, repeat.
 * ===========================================================================
 */
int main(int argc, char **argv)
{
    int port = (argc > 1) ? atoi(argv[1]) : 8080;

    /* A blocking listener is fine: WE never call accept(); the kernel does it
     * for our multishot ACCEPT SQE. (Pass nonblock=0.) */
    int listen_fd = make_listener(port, 0);

    /* ---- bring up the ring ------------------------------------------------
     * io_uring_queue_init(entries, ring, flags): asks the kernel (via
     * io_uring_setup(2)) for SQ/CQ rings with room for `entries` SQEs, mmaps the
     * three shared regions (SQ ring, CQ ring, SQE array) into our address space,
     * and fills in `ring`. flags=0 keeps it simple; production servers often add
     * IORING_SETUP_SINGLE_ISSUER | IORING_SETUP_DEFER_TASKRUN for throughput. */
    int r = io_uring_queue_init(QUEUE_DEPTH, &ring, 0);
    if (r < 0) {
        errno = -r;
        die("io_uring_queue_init (is your kernel >= 5.19?)");
    }

    /* ---- REGISTERED FILES: pin the listener in the fixed-file table --------
     * io_uring_register_files(ring, fds, n) publishes an array of fds to the
     * kernel once; ops then reference a fd by its index with IOSQE_FIXED_FILE,
     * skipping the per-op fdget()/fput() refcount dance. We register just the
     * listener at index 0 (LISTEN_FIXED_IDX). Accepted connections come back as
     * ordinary fds — registering all of them would need a resizable direct table
     * (see "Going further"), out of scope for the teaching core. */
    r = io_uring_register_files(&ring, &listen_fd, 1);
    if (r < 0) {
        errno = -r;
        die("io_uring_register_files");
    }

    /* ---- PROVIDED BUFFER RING: register a pool of recv buffers -------------
     * io_uring_setup_buf_ring(ring, nentries, bgid, flags, &err) allocates the
     * ring structure, mmaps/registers it with the kernel under group id `bgid`,
     * and returns a pointer we fill with buffer descriptors. nentries MUST be a
     * power of two (the ring is masked, not divided). */
    int bret = 0;
    buf_ring = io_uring_setup_buf_ring(&ring, NBUFS, BUF_GROUP_ID, 0, &bret);
    if (!buf_ring) {
        errno = -bret;
        die("io_uring_setup_buf_ring");
    }

    /* Backing store for the buffers themselves (the ring holds descriptors, not
     * bytes). One flat allocation of NBUFS * BUF_SIZE; freed implicitly at exit.
     * ALLOCATION NOTE: ~4 MiB via malloc -> almost certainly one mmap arena from
     * glibc; it lives for the whole process, so we never free it explicitly. */
    buf_base = malloc((size_t)NBUFS * BUF_SIZE);
    if (!buf_base)
        die("malloc(buffer pool)");

    /* Fill the ring: hand every buffer to the kernel. We add all NBUFS then do a
     * SINGLE advance of NBUFS — one store-release publishes the whole initial
     * pool, rather than a release per buffer. `mask` = NBUFS-1 selects the slot
     * (tail+i)&mask; `i` is both the buffer id and its offset within this add
     * batch. */
    unsigned mask = io_uring_buf_ring_mask(NBUFS);
    for (unsigned i = 0; i < NBUFS; i++) {
        io_uring_buf_ring_add(buf_ring,
                              buf_addr(i),           /* buffer address           */
                              BUF_SIZE,              /* buffer length            */
                              (unsigned short)i,     /* buffer id (== index)     */
                              mask,                  /* ring mask                */
                              i);                    /* offset within this batch */
    }
    io_uring_buf_ring_advance(buf_ring, NBUFS);      /* publish the whole pool   */

    /* Stage the single multishot accept and push it to the kernel. */
    arm_accept();
    io_uring_submit(&ring);

    fprintf(stderr,
            "echo_uring: listening on :%d  (io_uring, multishot accept+recv, "
            "%d provided buffers x %d B)\n", port, NBUFS, BUF_SIZE);

    /* ---- the event loop ---------------------------------------------------
     * io_uring_submit_and_wait(ring, 1): a SINGLE io_uring_enter(2) that both
     * flushes every SQE we staged during the last batch AND blocks until at
     * least one CQE is ready. This is the one syscall on the steady-state path.
     * We then drain ALL ready completions before returning to the kernel, so one
     * enter amortises over a whole batch of accepts/recvs/sends/closes. */
    for (;;) {
        r = io_uring_submit_and_wait(&ring, 1);
        if (r < 0) {
            if (r == -EINTR)
                continue;            /* a signal interrupted the wait; retry     */
            errno = -r;
            die("io_uring_submit_and_wait");
        }

        /* io_uring_for_each_cqe walks the CQ from the kernel's producer tail
         * down to our cached head WITHOUT advancing head — so we can look at
         * every CQE, then release them all at once. `head` is the loop cursor;
         * `cqe` points at the shared CQ memory. */
        struct io_uring_cqe *cqe;
        unsigned head;
        unsigned count = 0;
        io_uring_for_each_cqe(&ring, head, cqe) {
            count++;
            switch (ud_op(io_uring_cqe_get_data64(cqe))) {
            case OP_ACCEPT: on_accept(cqe); break;
            case OP_RECV:   on_recv(cqe);   break;
            case OP_SEND:   on_send(cqe);   break;
            case OP_CLOSE:  /* nothing to reap; fd is gone. */ break;
            }
        }

        /* io_uring_cq_advance(ring, count): publish "we consumed `count` CQEs"
         * by advancing the CQ head with a STORE-RELEASE. The kernel ACQUIRE-loads
         * this head before reusing those slots, so the release guarantees it does
         * not overwrite a CQE we have not yet read. Batching the advance to once
         * per loop (not once per CQE) is the mirror of batching submissions. */
        io_uring_cq_advance(&ring, count);
    }

    /* Not reached in normal operation; a real deployment would trap SIGINT and
     * fall through to here. io_uring_queue_exit unmaps the rings; the OS reclaims
     * the buffer pool and sockets on exit. */
    io_uring_queue_exit(&ring);
    free(buf_base);
    return 0;
}
