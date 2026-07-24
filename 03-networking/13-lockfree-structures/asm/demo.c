/* ===========================================================================
 * demo.c — the beating heart of a Treiber stack: the push/pop CAS loops,
 *          with a tagged (ABA-versioned) pointer, in ~60 lines of pure logic.
 * ===========================================================================
 *
 * WHY THIS FILE EXISTS (and why it has no #includes)
 * --------------------------------------------------
 * The lab's rule: every project ships the assembly the compiler ACTUALLY emits
 * for its most instructive pure-logic routine, hand-annotated. For a lock-free
 * stack that routine is the compare-and-swap (CAS) retry loop, because the
 * whole subject reduces to one machine instruction: `lock cmpxchg`. To make the
 * generated .s show that instruction *inline* (not hidden behind a libatomic
 * call), this file is deliberately self-contained — no system headers, its own
 * integer types — and it uses a 64-bit *packed* tagged pointer so the atomic is
 * a natively-lock-free 8-byte word. An 8-byte atomic CAS compiles to a single
 * `lock cmpxchgq` at every -O level, with NO -mcx16 and NO libatomic call.
 *
 * (The real library, treiber_stack.c, instead uses a full 128-bit tagged
 * pointer via `lock cmpxchg16b` — 64-bit pointer + 64-bit counter, so the tag
 * never wraps in practice. That needs -mcx16. Here we trade the wider counter
 * for a self-contained demo whose asm needs no special flags. Both techniques
 * are explained in the README.)
 *
 * THE ABA PROBLEM, IN ONE SENTENCE
 * --------------------------------
 * A plain pointer CAS can succeed on a stale value: thread T1 reads head==A and
 * plans head:=A->next; before its CAS, T2 pops A, pops B, then pushes A back —
 * head is A again, so T1's CAS(A -> A->next) *succeeds*, but A->next now points
 * at freed/wrong memory. The fix: pack a version counter next to the pointer so
 * "A the first time" and "A after the round trip" are DIFFERENT 64-bit words;
 * the counter was bumped by the intervening pops, so T1's CAS fails and retries.
 * ===========================================================================
 */

/* Own types — no <stdint.h>. On the Linux x86-64 LP64 model `unsigned long` is
 * 64 bits, wide enough to hold a packed (pointer|tag) word. */
typedef unsigned long u64;

/* A stack node. `next` is an ordinary field: in this packed scheme the ONLY
 * atomic object is the head word; a node is thread-private until its address is
 * published into head by a successful CAS, and immutable-enough afterward for
 * the tag to guard against ABA. */
struct node {
    struct node *next;   /* next-lower node, or 0 at the bottom of the stack */
    long         value;  /* the payload we stored                            */
};

/* ---- The packed tagged pointer -------------------------------------------
 * x86-64 user virtual addresses are "canonical": today they fit in 48 bits,
 * so the top 16 bits of every pointer we ever see are zero and free for us to
 * borrow as an ABA counter. We pack [ tag:16 | pointer:48 ] into one u64.
 *
 *   bits 63..48 : 16-bit version tag (incremented on every successful update)
 *   bits 47..0  : the node pointer
 *
 * Because both halves live in one word, a single 8-byte CAS swaps them together
 * atomically — that is the entire trick. */
#define PTR_MASK  0x0000FFFFFFFFFFFFUL   /* low 48 bits: the pointer field    */
#define TAG_SHIFT 48                     /* high 16 bits: the version field   */

static inline u64          pack(struct node *p, u64 tag) {
    /* mask the pointer (defensive: guarantees we never clobber the tag field)
     * and drop the tag into the high bits. */
    return ((u64)tag << TAG_SHIFT) | ((u64)p & PTR_MASK);
}
static inline struct node *ptr_of(u64 w) { return (struct node *)(w & PTR_MASK); }
static inline u64          tag_of(u64 w) { return w >> TAG_SHIFT; }

/* ---------------------------------------------------------------------------
 * demo_push — put node `n` on top of the stack.
 *
 * ABI: head in %rdi, n in %rsi (SysV AMD64). No return value.
 *
 * The loop: read the current head word; point n->next at the current top; build
 * a new word {n, tag+1}; CAS it in. If another thread changed head first, CAS
 * writes the fresh value back into `old` and we retry with the new top.
 *
 * Ordering:
 *   success = RELEASE — everything we wrote into *n (its ->next and ->value)
 *     must be visible to a thread that later ACQUIRE-loads head and pops n.
 *     Release publishes those writes as a package with the pointer.
 *   failure = RELAXED — a failed CAS did no publishing; we only need the newly
 *     observed head value to retry, so no synchronization is required.
 * --------------------------------------------------------------------------- */
void demo_push(u64 *head, struct node *n) {
    /* RELAXED initial read: push never dereferences the old top, so we need no
     * ordering yet — the CAS will re-validate the value anyway. */
    u64 old = __atomic_load_n(head, __ATOMIC_RELAXED);
    u64 neu;
    do {
        n->next = ptr_of(old);              /* link n above the current top   */
        neu     = pack(n, tag_of(old) + 1); /* bump the ABA tag               */
    } while (!__atomic_compare_exchange_n(
                 head, &old, neu,
                 1 /* weak: allow spurious failure, cheaper on some ISAs */,
                 __ATOMIC_RELEASE,   /* success ordering (publish n)          */
                 __ATOMIC_RELAXED)); /* failure ordering (just reload old)    */
}

/* ---------------------------------------------------------------------------
 * demo_pop — remove and return the top node, or 0 if the stack is empty.
 *
 * ABI: head in %rdi; returns the node pointer in %rax.
 *
 * Ordering:
 *   initial load = ACQUIRE — we are about to dereference the top (top->next),
 *     so we must see the pusher's fully-initialized node. Acquire here pairs
 *     with the RELEASE in demo_push.
 *   success = ACQUIRE — after we win, we go on to read top->value; acquire
 *     guarantees that read sees the published node.
 *   failure = ACQUIRE — the CAS reloaded a new head into `old`; we will
 *     dereference *that* node next iteration, so it too must be acquired.
 *
 * The tag is what makes this safe against ABA: even if `top` is the same
 * address as a previously-popped node, the intervening pops bumped the tag, so
 * `old` no longer equals head and the CAS fails instead of corrupting the list.
 * --------------------------------------------------------------------------- */
struct node *demo_pop(u64 *head) {
    u64 old = __atomic_load_n(head, __ATOMIC_ACQUIRE);
    u64 neu;
    struct node *top;
    do {
        top = ptr_of(old);
        if (!top) return 0;                       /* empty stack             */
        neu = pack(top->next, tag_of(old) + 1);   /* new top = top->next     */
    } while (!__atomic_compare_exchange_n(
                 head, &old, neu, 1,
                 __ATOMIC_ACQUIRE,    /* success ordering                    */
                 __ATOMIC_ACQUIRE));  /* failure ordering                    */
    return top;
}
