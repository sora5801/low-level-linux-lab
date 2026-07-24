/* ===========================================================================
 * asm/demo.c — the instructive pure-logic core, isolated for annotated asm.
 * ===========================================================================
 *
 * This file is a SELF-CONTAINED extraction of the most asm-worthy routines in
 * the project: the mutex fast path (an atomic compare-exchange, 0 -> 1) and the
 * test-and-test-and-set spinlock with a PAUSE hint. It deliberately declares its
 * own types and includes NO headers, so it cross-compiles to Linux assembly on
 * any host with:
 *
 *   clang --target=x86_64-pc-linux-gnu -S -O1 ... asm/demo.c -o asm/demo.s
 *
 * The kernel/futex parts are intentionally absent — a `syscall` looks the same
 * everywhere and is covered by the nolibc reference project. What is UNIQUE and
 * worth staring at here is the atomic read-modify-write: how a C
 * `__atomic_compare_exchange_n` becomes a single `lock cmpxchg`, how the ZF flag
 * carries the success/fail result, and how the spin loop lowers to `xchg` +
 * `pause`. See asm/demo.annotated.s for the line-by-line walkthrough.
 * ===========================================================================
 */

/* Our own fixed-width type — no <stdint.h> needed. A futex word is exactly 32
 * bits, and `unsigned int` is 32 bits in the LP64 model Linux uses on x86-64. */
typedef unsigned int u32;

/* ---------------------------------------------------------------------------
 * demo_mutex_trylock — the entire uncontended acquire, in one atomic op.
 *
 * Attempt the state transition 0 (unlocked) -> 1 (locked). Returns non-zero if
 * we won the lock, 0 if it was already held. This lowers to a `lock cmpxchg`:
 * the CPU compares [state] with `expected` (0) and, only if equal, stores 1 —
 * all as one indivisible, bus-locked operation. No syscall, no spin.
 *
 * ACQUIRE on success: no memory access inside the critical section may be
 * reordered to before we hold the lock. RELAXED on failure: a failed try tells
 * us nothing we must synchronize with.
 * --------------------------------------------------------------------------- */
int demo_mutex_trylock(u32 *state)
{
    u32 expected = 0;
    return __atomic_compare_exchange_n(state, &expected, 1u,
                                       0 /* strong */,
                                       __ATOMIC_ACQUIRE, __ATOMIC_RELAXED);
}

/* ---------------------------------------------------------------------------
 * demo_mutex_lock_fast — the fast path of a full mutex lock.
 *
 * Returns 0 if we acquired the lock with a single CAS (the common case), or the
 * observed non-zero state (1 or 2) that the caller's slow path would then hand
 * to futex. The point of extracting this is to show that the compiler keeps the
 * value the failing CAS wrote back into `c` and returns it in one register —
 * no reload from memory.
 * --------------------------------------------------------------------------- */
u32 demo_mutex_lock_fast(u32 *state)
{
    u32 c = 0;
    if (__atomic_compare_exchange_n(state, &c, 1u,
                                    0, __ATOMIC_ACQUIRE, __ATOMIC_RELAXED))
        return 0;           /* fast: 0 -> 1 succeeded, we hold the lock        */
    return c;               /* contended: c is the real current state (1 or 2) */
}

/* ---------------------------------------------------------------------------
 * demo_mutex_unlock_fast — decrement-and-decide.
 *
 * Atomically subtract 1 (RELEASE, to publish the critical section) and inspect
 * the OLD value. If it was 1 the lock was uncontended and we are done with no
 * syscall (return 0). If it was 2 there may be a sleeper: fully free the lock
 * and return 1 so the caller issues a FUTEX_WAKE. Shows `lock xadd`/`lock dec`.
 * --------------------------------------------------------------------------- */
int demo_mutex_unlock_fast(u32 *state)
{
    if (__atomic_fetch_sub(state, 1u, __ATOMIC_RELEASE) != 1u) {
        __atomic_store_n(state, 0u, __ATOMIC_RELEASE);
        return 1;           /* was contended (2): caller must wake a waiter    */
    }
    return 0;               /* was 1: clean fast unlock, no wake needed        */
}

/* PAUSE: the spin-wait hint. Emits the `pause` instruction (F3 90). See the
 * annotated asm for why a busy loop that omits this wastes power and eats a
 * pipeline flush when the awaited write finally arrives. */
static inline void cpu_relax(void)
{
    __builtin_ia32_pause();
}

/* ---------------------------------------------------------------------------
 * demo_spin_lock — test-and-test-and-set acquire with backoff.
 *
 * Outer loop: TEST-AND-SET via `xchg` (an implicitly-locked read-modify-write).
 * If we swapped in 1 over a previous 0, we own the lock. Otherwise fall into the
 * inner TEST loop: spin on a plain relaxed LOAD (cheap, keeps the cache line in
 * Shared state) with a PAUSE between reads, until the holder releases; then jump
 * back and re-attempt the expensive locked exchange. This is the canonical
 * spinlock shape — one locked op to acquire, read-only spinning while waiting.
 * --------------------------------------------------------------------------- */
void demo_spin_lock(u32 *locked)
{
    for (;;) {
        if (__atomic_exchange_n(locked, 1u, __ATOMIC_ACQUIRE) == 0)
            return;                                     /* acquired            */
        while (__atomic_load_n(locked, __ATOMIC_RELAXED) != 0)
            cpu_relax();                                /* spin read + PAUSE   */
    }
}

/* demo_spin_unlock — release the spinlock with a single ordered store. On x86
 * this is a plain `mov` of 0; the RELEASE order is a compiler barrier that keeps
 * the critical-section writes from sinking past it. */
void demo_spin_unlock(u32 *locked)
{
    __atomic_store_n(locked, 0u, __ATOMIC_RELEASE);
}
