/* ===========================================================================
 * demo.c — the PURE-LOGIC core of the virtual NIC's software RX ring, extracted
 *          so it can be compiled to standalone assembly.
 * ===========================================================================
 *
 * WHY THIS FILE EXISTS
 * --------------------
 * The driver itself (../vnetdev.c) is a Linux *kernel module*. It #includes
 * <linux/netdevice.h>, <linux/skbuff.h>, and friends — headers that only exist
 * inside a configured kernel source tree. You therefore CANNOT run
 *
 *     clang -S vnetdev.c
 *
 * on a normal host: the compile fails at the first kernel #include. So, per the
 * lab convention, we lift the single most instructive piece of *pure logic* out
 * of the driver — the ring-buffer index arithmetic that decides where the next
 * frame goes — into this self-contained file. It has no kernel headers, no libc,
 * and declares its own integer types. That makes it a real translation unit we
 * can turn into real, inspectable assembly (see demo.s / demo.O0.s / demo.O2.s
 * and the hand-annotated demo.annotated.s).
 *
 * THE CONCEPT: A FREE-RUNNING INDEX RING (the kfifo trick)
 * -------------------------------------------------------
 * The driver keeps a fixed array of `struct sk_buff *` slots. `head` is the
 * producer cursor (ndo_start_xmit pushes here) and `tail` is the consumer cursor
 * (the NAPI poll loop pops here). The trick that makes this fast and correct is:
 *
 *   1. head and tail are *free-running* unsigned counters — they only ever
 *      increment and are allowed to wrap around 2^32. They are NOT masked.
 *   2. The physical array slot is `index & (SIZE - 1)`, which is a single AND
 *      because SIZE is a power of two. `& (SIZE-1)` == `% SIZE` for powers of 2,
 *      but the AND has no division unit latency.
 *   3. The number of occupied slots is simply `head - tail`, computed in
 *      unsigned arithmetic. Unsigned subtraction is modular (mod 2^32), so this
 *      stays correct even at the instant head wraps past 0 while tail has not —
 *      as long as the ring never holds more than 2^32 entries (ours holds 64).
 *
 * This is exactly how the kernel's own `struct kfifo` works, and why kfifo
 * *requires* a power-of-two size. Reading the assembly below, you will see the
 * `%` never appears — every "modulo" is one `and` instruction.
 * ===========================================================================
 */

/* We are freestanding: no <stdint.h>. Spell out the widths we rely on. On the
 * x86-64 SysV target these are exact: unsigned int is 32-bit, int is 32-bit. */
typedef unsigned int u32;   /* 32-bit unsigned — matches the driver's counters */
typedef int          i32;   /* 32-bit signed   — used for the "-1 = full" sentinel */

/* The ring geometry. SIZE must be a power of two for the AND-mask trick to be
 * equivalent to a modulo. MASK is SIZE-1, i.e. the low ORDER bits all set. */
#define RING_ORDER 6u                 /* 2^6 = 64 slots                        */
#define RING_SIZE  (1u << RING_ORDER) /* 64                                    */
#define RING_MASK  (RING_SIZE - 1u)   /* 63 == 0b0011_1111                      */

/* ---------------------------------------------------------------------------
 * rb_count — how many entries are currently queued.
 *
 * `head - tail` in unsigned 32-bit is the occupancy, valid across wrap. If head
 * has incremented 1000 times and tail 990 times, the ring holds 10 — regardless
 * of any wrap of the raw counters. This compiles to a single subtraction.
 * --------------------------------------------------------------------------- */
u32 rb_count(u32 head, u32 tail)
{
    return head - tail;                 /* modular subtraction = live count      */
}

/* ---------------------------------------------------------------------------
 * rb_is_full — is there no room for one more frame?
 *
 * Full means occupancy has reached the capacity. Because occupancy is
 * `head - tail`, "full" is `(head - tail) >= RING_SIZE`. We deliberately keep
 * capacity == RING_SIZE (not SIZE-1); the free-running counters let us tell
 * "full" (count==SIZE) apart from "empty" (count==0) without wasting a slot,
 * which the naive `head==tail means empty, (head+1)==tail means full` scheme
 * cannot do.
 * --------------------------------------------------------------------------- */
i32 rb_is_full(u32 head, u32 tail)
{
    return (head - tail) >= RING_SIZE;  /* compare occupancy against capacity    */
}

/* ---------------------------------------------------------------------------
 * rb_is_empty — nothing to consume?
 *
 * Empty is exactly head == tail. The NAPI poll loop tests this to know when to
 * stop and call napi_complete_done().
 * --------------------------------------------------------------------------- */
i32 rb_is_empty(u32 head, u32 tail)
{
    return head == tail;
}

/* ---------------------------------------------------------------------------
 * rb_slot — map a free-running index to a physical array slot.
 *
 * The whole point of the power-of-two size: this is ONE `and`, not a `div`.
 * Watch for it in the assembly (`andl $63, ...`).
 * --------------------------------------------------------------------------- */
u32 rb_slot(u32 index)
{
    return index & RING_MASK;           /* == index % RING_SIZE, but branch/div-free */
}

/* ---------------------------------------------------------------------------
 * rb_reserve — the producer's decision, and the star of the annotated asm.
 *
 * Given a pointer to the producer cursor `*head` and a snapshot of the consumer
 * cursor `tail`, either:
 *   - reserve the next slot: compute it, advance *head, and return the slot; or
 *   - report the ring is full by returning -1.
 *
 * This is precisely what ndo_start_xmit does under the ring spinlock before it
 * stores the sk_buff pointer. Returning the slot as a signed i32 lets a single
 * value carry both the success index (0..63) and the failure sentinel (-1),
 * which is why the return type is i32, not u32. The assembly shows the compiler
 * turning the whole thing into a compare, a conditional branch, an AND, and an
 * increment — no multiply, no divide.
 * --------------------------------------------------------------------------- */
i32 rb_reserve(u32 *head, u32 tail)
{
    u32 h = *head;                      /* read the producer cursor once         */
    if ((h - tail) >= RING_SIZE)        /* occupancy check: is the ring full?    */
        return -1;                      /* caller must drop the frame            */
    *head = h + 1u;                     /* publish the advance (commit the slot)  */
    return (i32)(h & RING_MASK);        /* physical slot for this frame          */
}

/* ---------------------------------------------------------------------------
 * skb_has_headroom — a second, sk_buff-flavored bit of pure arithmetic.
 *
 * A struct sk_buff owns a linear buffer with four cursors laid out as:
 *
 *     head  <=  data  <=  tail  <=  end
 *     |----------|---------|--------|
 *      headroom    payload  tailroom
 *
 * "Headroom" is the free space in FRONT of the payload: `data - head`. Protocol
 * layers push headers by moving `data` backwards (skb_push), which requires
 * enough headroom or the kernel must reallocate. This helper answers "can I
 * prepend `need` bytes without reallocating?" using the same modular-safe
 * unsigned subtraction. (In the real kernel these are pointers; here they are
 * u32 offsets so the file stays header-free and portable to assembly.)
 * --------------------------------------------------------------------------- */
i32 skb_has_headroom(u32 head, u32 data, u32 need)
{
    return (data - head) >= need;       /* headroom = data-head; enough for a push? */
}
