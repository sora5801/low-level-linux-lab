/* ===========================================================================
 * syncprim.c — the implementations. Freestanding: no libc, no system headers.
 * ===========================================================================
 *
 * The two primitives everything is built from:
 *
 *   (1) __atomic_* builtins — single-instruction (or lock-prefixed) operations
 *       on a memory word, each carrying a *memory order* that tells BOTH the
 *       compiler and the CPU which reorderings are forbidden. On x86-64 (a
 *       Total-Store-Order machine) acquire/release loads and stores are just
 *       plain MOVs — the ordering is enforced by forbidding *compiler*
 *       reordering. A read-modify-write (CAS, XCHG, XADD) becomes a LOCK-
 *       prefixed instruction, which is a full barrier in hardware. The ordering
 *       annotations still matter: they are the contract, and they are what makes
 *       the same code correct on a weakly-ordered CPU such as AArch64.
 *
 *   (2) futex(2) — Linux syscall 202. Signature:
 *          long futex(uint32_t *uaddr, int op, uint32_t val,
 *                     const struct timespec *timeout,
 *                     uint32_t *uaddr2, uint32_t val3);
 *       We use two ops:
 *          FUTEX_WAIT(0): the kernel ATOMICALLY checks *uaddr == val and, only
 *              if equal, puts the calling thread to sleep on a wait queue keyed
 *              by the word's address. If *uaddr != val it returns -EAGAIN at
 *              once. That atomic "compare then sleep" is the anti-lost-wakeup
 *              guarantee: a concurrent updater that changes the word after our
 *              check-in-userspace but before we sleep is caught by the kernel's
 *              re-check under the bucket lock.
 *          FUTEX_WAKE(1): wake up to `val` threads sleeping on the address.
 *       Both are OR'd with FUTEX_PRIVATE_FLAG(128): these locks live in a single
 *       process's address space, so the kernel can skip the shared-mapping
 *       lookup and use a cheaper per-mm hash bucket.
 * ===========================================================================
 */

#include "syncprim.h"

/* ===========================================================================
 * The raw futex syscall — no libc wrapper, so the whole file is freestanding.
 * =========================================================================== */

/* Linux x86-64 numbers/flags. On a real box these come from <sys/syscall.h> and
 * <linux/futex.h>; we spell them out so this file needs no system headers. They
 * are stable kernel ABI. */
#define SP_SYS_futex           202
#define SP_FUTEX_WAIT            0
#define SP_FUTEX_WAKE            1
#define SP_FUTEX_PRIVATE_FLAG  128
#define SP_FUTEX_WAIT_PRIVATE  (SP_FUTEX_WAIT | SP_FUTEX_PRIVATE_FLAG)
#define SP_FUTEX_WAKE_PRIVATE  (SP_FUTEX_WAKE | SP_FUTEX_PRIVATE_FLAG)

/* INT_MAX without <limits.h>: "wake everyone" for broadcast. */
#define SP_INT_MAX 0x7fffffff

/* ---------------------------------------------------------------------------
 * sp_futex — issue the syscall directly.
 *
 * The Linux syscall ABI (distinct from the C-call ABI!) is:
 *     rax = syscall number
 *     args in rdi, rsi, rdx, r10, r8, r9   (note: r10, NOT rcx, for arg4)
 *     return value in rax
 *     the `syscall` instruction itself CLOBBERS rcx and r11 (it parks the
 *     return RIP in rcx and RFLAGS in r11), so we list them as clobbered.
 * "memory" in the clobber list forbids the compiler from caching our futex word
 * across the call — essential, since the kernel reads it.
 *
 * A raw syscall returns -errno on failure (e.g. -EAGAIN == -11), NOT the
 * libc convention of -1 with errno set. Callers below treat all failures the
 * same way — re-loop and re-check the userspace word — so we simply return the
 * raw value and let the loops absorb EAGAIN/EINTR/ETIMEDOUT.
 * --------------------------------------------------------------------------- */
static long sp_futex(uint32_t *uaddr, int op, uint32_t val)
{
    long ret;
    /* args 4/5/6 must sit in r10/r8/r9; a "local register variable" pins them.
     * timeout(r10)=NULL, uaddr2(r8)=NULL, val3(r9)=0 — unused for WAIT/WAKE. */
    register long r10 __asm__("r10") = 0;   /* timeout = NULL                  */
    register long r8  __asm__("r8")  = 0;   /* uaddr2  = NULL                  */
    register long r9  __asm__("r9")  = 0;   /* val3    = 0                     */
    __asm__ volatile(
        "syscall"
        : "=a"(ret)                                     /* out: rax -> ret     */
        : "a"((long)SP_SYS_futex),                      /* rax = 202           */
          "D"(uaddr),                                   /* rdi = uaddr         */
          "S"((long)op),                                /* rsi = op            */
          "d"((long)val),                               /* rdx = val           */
          "r"(r10), "r"(r8), "r"(r9)                    /* r10/r8/r9 = args4-6 */
        : "rcx", "r11", "memory"                        /* syscall clobbers    */
    );
    return ret;
}

/* Block until *uaddr is observed to differ from `expected` (someone woke us or
 * changed it). Spurious returns (EAGAIN/EINTR) are FINE: every caller re-checks
 * the userspace condition in a loop, so a premature wake just costs one extra
 * turn around the loop. */
static void sp_futex_wait(uint32_t *uaddr, uint32_t expected)
{
    (void)sp_futex(uaddr, SP_FUTEX_WAIT_PRIVATE, expected);
}

/* Wake up to `n` threads parked on this address. Returns (ignored) count woken;
 * waking with zero sleepers is a cheap kernel no-op, which is why the semaphore
 * and condvar can afford to wake unconditionally. */
static void sp_futex_wake(uint32_t *uaddr, int n)
{
    (void)sp_futex(uaddr, SP_FUTEX_WAKE_PRIVATE, (uint32_t)n);
}

/* The PAUSE instruction: a hint to the CPU that this is a spin-wait loop. It
 * (a) throttles the pipeline so a tight retry loop does not saturate execution
 * ports or overheat, (b) yields resources to the sibling hyperthread, and (c)
 * avoids a costly memory-order-violation flush when the awaited store finally
 * lands. It is NOT a sleep — just a "back off a few cycles" nudge. */
static inline void sp_cpu_relax(void)
{
    __builtin_ia32_pause();
}

/* ===========================================================================
 * SPINLOCK — test-and-test-and-set with exponential-ish backoff via PAUSE.
 * =========================================================================== */

void sp_spin_init(sp_spinlock *s)
{
    /* No concurrency during init, so a relaxed store is enough: there is no
     * happens-before to establish yet. */
    __atomic_store_n(&s->locked, 0, __ATOMIC_RELAXED);
}

sp_bool sp_spin_trylock(sp_spinlock *s)
{
    /* One shot: atomically swap 1 in and read the old value. If it was 0 we won.
     * ACQUIRE: nothing in the critical section may be hoisted above this point,
     * and we must observe every write the previous holder released. */
    return __atomic_exchange_n(&s->locked, 1, __ATOMIC_ACQUIRE) == 0
               ? SP_TRUE : SP_FALSE;
}

void sp_spin_lock(sp_spinlock *s)
{
    for (;;) {
        /* TEST-and-set: try to grab the lock with a single atomic exchange.
         * ACQUIRE so the critical section cannot start until we hold the lock. */
        if (__atomic_exchange_n(&s->locked, 1, __ATOMIC_ACQUIRE) == 0)
            return;                         /* uncontended: got it, no spinning */

        /* TEST: now spin on a *plain load* (RELAXED), not the atomic exchange.
         * This is the "test-and-test-and-set" refinement: an exchange writes
         * every iteration, which keeps the cache line bouncing in Modified
         * state between cores (a storm of coherency traffic). A read-only spin
         * lets every waiter hold the line Shared and watch quietly; only when
         * the holder releases (line goes Invalid) does one waiter attempt the
         * expensive write again. PAUSE spaces out the reads. */
        while (__atomic_load_n(&s->locked, __ATOMIC_RELAXED) != 0)
            sp_cpu_relax();
    }
}

void sp_spin_unlock(sp_spinlock *s)
{
    /* RELEASE: every write inside the critical section must be globally visible
     * BEFORE the lock reads 0, so the next acquirer (whose ACQUIRE pairs with
     * this RELEASE) sees a consistent world. On x86 this is a plain MOV; the
     * release semantics are enforced against the *compiler*, and x86's store
     * ordering does the rest in hardware. */
    __atomic_store_n(&s->locked, 0, __ATOMIC_RELEASE);
}

/* ===========================================================================
 * MUTEX — Ulrich Drepper's three-state futex mutex ("Futexes Are Tricky").
 * =========================================================================== */

void sp_mutex_init(sp_mutex *m)
{
    __atomic_store_n(&m->state, 0, __ATOMIC_RELAXED);   /* 0 = unlocked        */
}

sp_bool sp_mutex_trylock(sp_mutex *m)
{
    uint32_t expected = 0;
    /* CAS 0 -> 1: succeed only if currently unlocked. Never blocks, never
     * syscalls. ACQUIRE on success (we now own the critical section); RELAXED on
     * failure (we learned nothing we must synchronize with — we just give up). */
    return __atomic_compare_exchange_n(
               &m->state, &expected, 1,
               0 /* strong */, __ATOMIC_ACQUIRE, __ATOMIC_RELAXED)
               ? SP_TRUE : SP_FALSE;
}

void sp_mutex_lock(sp_mutex *m)
{
    /* ---- FAST PATH (no syscall) ----------------------------------------- */
    uint32_t c = 0;
    /* Try the uncontended transition 0 -> 1. On the common path this single
     * `lock cmpxchg` is the ENTIRE cost of locking: no kernel, no scheduler.
     * On success `c` is unchanged (0) and we return. On failure the builtin
     * writes the *actual* current value back into `c` for us to inspect. */
    if (__atomic_compare_exchange_n(&m->state, &c, 1,
                                    0, __ATOMIC_ACQUIRE, __ATOMIC_RELAXED))
        return;

    /* ---- SLOW PATH (contended) ------------------------------------------ */
    /* We arrive here holding a non-zero `c` (1 or 2). The invariant we must
     * maintain: before sleeping, the word MUST read 2, so that whoever unlocks
     * knows a sleeper exists and issues a wake. */
    if (c != 2)
        /* It was 1 (locked, no recorded waiter). Announce ourselves as a waiter
         * by swapping the word to 2, learning the value it held. XCHG returns
         * the previous value; ACQUIRE because a returned 0 means we just grabbed
         * a lock that a concurrent unlock released. */
        c = __atomic_exchange_n(&m->state, 2, __ATOMIC_ACQUIRE);

    while (c != 0) {
        /* The word is 2 (locked+contended). Ask the kernel to sleep, but ONLY
         * if it is still 2 — FUTEX_WAIT re-checks *state == 2 under its bucket
         * lock. If unlock already stored 0 in the race window, WAIT returns
         * -EAGAIN immediately and we loop without sleeping: no lost wakeup. */
        sp_futex_wait(&m->state, 2);

        /* Woken (or EAGAIN'd). Re-assert the contended state and re-read: swap
         * 2 in again. If the previous value was 0 the lock was free and we have
         * now taken it (loop exits). If it was still 1/2, someone else holds it;
         * we set it back to 2 (recording that we, a waiter, are here) and loop
         * to sleep again. Always leaving 2 behind on failure is what keeps the
         * "there is a waiter" flag armed. */
        c = __atomic_exchange_n(&m->state, 2, __ATOMIC_ACQUIRE);
    }
    /* Fell out with c == 0: our XCHG turned a free (0) lock into 2 and returned
     * 0, so we own it. We hold it as state==2 (conservatively "contended"),
     * which is safe: the worst case is one extra, harmless FUTEX_WAKE at unlock. */
}

void sp_mutex_unlock(sp_mutex *m)
{
    /* Atomically decrement and read the OLD value. RELEASE publishes everything
     * we did in the critical section to the next acquirer.
     *   old == 1: it was LOCKED with no waiters -> now 0, and NObody is asleep,
     *             so we are done WITHOUT a syscall (the fast unlock path).
     *   old == 2: it was LOCKED+CONTENDED -> fetch_sub left it at 1, but there
     *             may be a sleeper, so we must fully release and wake one. */
    if (__atomic_fetch_sub(&m->state, 1, __ATOMIC_RELEASE) != 1) {
        /* Store 0 to actually free the lock. RELEASE (not relaxed) so the value
         * a woken waiter reads carries our happens-before edge even under the
         * strictest release-sequence rules. */
        __atomic_store_n(&m->state, 0, __ATOMIC_RELEASE);
        /* Wake exactly ONE waiter. Waking all would cause a thundering herd:
         * every woken thread races for the one lock, all but one lose and sleep
         * again, having paid two syscalls for nothing. */
        sp_futex_wake(&m->state, 1);
    }
}

/* ===========================================================================
 * CONDITION VARIABLE — sequence-counter futex condvar.
 *
 * Contract (identical to pthread_cond): the caller must hold `m` when calling
 * wait, and must re-test the predicate in a loop, because wait may return
 * spuriously. wait atomically releases `m` and blocks; on return it re-holds m.
 * =========================================================================== */

void sp_cond_init(sp_cond *c)
{
    __atomic_store_n(&c->seq, 0, __ATOMIC_RELAXED);
}

void sp_cond_wait(sp_cond *c, sp_mutex *m)
{
    /* Sample the sequence NUMBER while we still hold the mutex. This ordering is
     * the whole trick: because a signaler must hold `m` to change the predicate
     * and only then bumps seq, sampling seq under `m` means any signal that
     * happens after we drop `m` is guaranteed to leave seq != our sample. */
    uint32_t observed = __atomic_load_n(&c->seq, __ATOMIC_ACQUIRE);

    /* Release the mutex so a signaler can make progress (and take the mutex to
     * change the predicate). */
    sp_mutex_unlock(m);

    /* Sleep only if no signal has happened since we sampled. If cond_signal ran
     * in the window between the unlock above and this syscall, seq now differs
     * from `observed`, FUTEX_WAIT returns -EAGAIN, and we fall through at once
     * instead of sleeping through the wakeup we were meant to catch. */
    sp_futex_wait(&c->seq, observed);

    /* Re-acquire the mutex before returning, as the contract promises. The
     * caller loops and re-checks the predicate, absorbing any spurious wake. */
    sp_mutex_lock(m);
}

void sp_cond_signal(sp_cond *c)
{
    /* Change the futex value so a racing waiter's FUTEX_WAIT fails its re-check.
     * RELEASE pairs with the ACQUIRE load in wait, publishing the predicate
     * update the signaler made under the mutex. */
    __atomic_fetch_add(&c->seq, 1, __ATOMIC_RELEASE);
    /* Wake one sleeper. If none is parked this is a cheap kernel no-op. */
    sp_futex_wake(&c->seq, 1);
}

void sp_cond_broadcast(sp_cond *c)
{
    /* One bump invalidates every sleeper's sampled value at once. */
    __atomic_fetch_add(&c->seq, 1, __ATOMIC_RELEASE);
    /* Wake everyone. They will all contend for the mutex on return; a production
     * condvar uses FUTEX_CMP_REQUEUE to move all-but-one onto the mutex's wait
     * queue instead, avoiding the stampede (see README "Going further"). */
    sp_futex_wake(&c->seq, SP_INT_MAX);
}

/* ===========================================================================
 * COUNTING SEMAPHORE — permits in a single futex word.
 * =========================================================================== */

void sp_sem_init(sp_sem *s, uint32_t value)
{
    __atomic_store_n(&s->count, value, __ATOMIC_RELAXED);
}

sp_bool sp_sem_trywait(sp_sem *s)
{
    uint32_t c = __atomic_load_n(&s->count, __ATOMIC_RELAXED);
    while (c > 0) {
        /* Try to claim one permit: c -> c-1. ACQUIRE on success so the work
         * guarded by the permit cannot be reordered before the acquire. The
         * _weak form may fail spuriously; on any failure the builtin reloads the
         * current count into `c` and we retry the loop. */
        if (__atomic_compare_exchange_n(&s->count, &c, c - 1,
                                        1 /* weak */, __ATOMIC_ACQUIRE,
                                        __ATOMIC_RELAXED))
            return SP_TRUE;
    }
    return SP_FALSE;                        /* no permit available right now    */
}

void sp_sem_wait(sp_sem *s)
{
    for (;;) {
        /* Fast path: grab a permit in userspace if one exists. */
        uint32_t c = __atomic_load_n(&s->count, __ATOMIC_RELAXED);
        while (c > 0) {
            if (__atomic_compare_exchange_n(&s->count, &c, c - 1,
                                            1, __ATOMIC_ACQUIRE, __ATOMIC_RELAXED))
                return;
            /* CAS failed: `c` was refreshed to the current count; re-test > 0. */
        }
        /* Slow path: count is 0. Sleep until it is observed to change away from
         * 0. If a post lands between the load above and here, the value is no
         * longer 0, FUTEX_WAIT returns -EAGAIN, and we loop to grab the permit
         * — so a post can never be lost. */
        sp_futex_wait(&s->count, 0);
    }
}

void sp_sem_post(sp_sem *s)
{
    /* Release a permit. RELEASE so work done before post() is visible to the
     * thread that will consume this permit (its wait() does an ACQUIRE). */
    __atomic_fetch_add(&s->count, 1, __ATOMIC_RELEASE);
    /* Wake one potential sleeper. We wake unconditionally: without a separate
     * waiter counter we cannot cheaply know if anyone is parked, and a wake with
     * no sleepers is a cheap no-op. Trading that syscall away is the stretch. */
    sp_futex_wake(&s->count, 1);
}

/* ===========================================================================
 * READER-WRITER LOCK — reader-preferring, single 32-bit state word.
 *
 * state == 0                : free
 * 0 < state <= SP_RW_RDMASK : (state) readers active, writer bit clear
 * state == SP_RW_WRITER     : one writer active (top bit set, no readers)
 * =========================================================================== */

void sp_rwlock_init(sp_rwlock *rw)
{
    __atomic_store_n(&rw->state, 0, __ATOMIC_RELAXED);
}

sp_bool sp_rwlock_tryrdlock(sp_rwlock *rw)
{
    uint32_t s = __atomic_load_n(&rw->state, __ATOMIC_RELAXED);
    /* A reader may enter iff no writer holds the lock. Bump the reader count. */
    while (!(s & SP_RW_WRITER)) {
        if (__atomic_compare_exchange_n(&rw->state, &s, s + 1,
                                        1, __ATOMIC_ACQUIRE, __ATOMIC_RELAXED))
            return SP_TRUE;                 /* joined the readers               */
    }
    return SP_FALSE;                        /* a writer holds it                */
}

void sp_rwlock_rdlock(sp_rwlock *rw)
{
    for (;;) {
        uint32_t s = __atomic_load_n(&rw->state, __ATOMIC_RELAXED);
        if (s & SP_RW_WRITER) {
            /* A writer holds the lock. Sleep until the word changes (writer will
             * store 0 and wake us). FUTEX_WAIT re-checks state == s, so if the
             * writer already released in the race window we simply loop. */
            sp_futex_wait(&rw->state, s);
            continue;
        }
        /* No writer: try to add ourselves to the reader count. ACQUIRE so our
         * reads of the protected data cannot be hoisted before we are counted
         * in — pairs with the writer's RELEASE on unlock. */
        if (__atomic_compare_exchange_n(&rw->state, &s, s + 1,
                                        1, __ATOMIC_ACQUIRE, __ATOMIC_RELAXED))
            return;
        /* CAS lost a race (reader/writer count changed): retry from the top. */
    }
}

void sp_rwlock_rdunlock(sp_rwlock *rw)
{
    /* Drop one reader. RELEASE so anything a reader wrote is visible before the
     * count falls (readers usually only read, but the ordering must be correct
     * for those that don't). fetch_sub returns the OLD value. */
    uint32_t prev = __atomic_fetch_sub(&rw->state, 1, __ATOMIC_RELEASE);
    if (prev == 1)
        /* We were the LAST reader: the word just became 0, so a writer parked in
         * rdlock/wrlock can now proceed. Wake one. We wake ONLY on the 1->0
         * edge because that is the only moment a writer can acquire; the futex
         * value re-check in the writer's WAIT makes any earlier count changes
         * (which sent no wake) safe. */
        sp_futex_wake(&rw->state, 1);
}

sp_bool sp_rwlock_trywrlock(sp_rwlock *rw)
{
    uint32_t expected = 0;
    /* A writer may enter only from a completely free lock (no readers, no
     * writer): CAS 0 -> WRITER. */
    return __atomic_compare_exchange_n(&rw->state, &expected, SP_RW_WRITER,
                                       0, __ATOMIC_ACQUIRE, __ATOMIC_RELAXED)
               ? SP_TRUE : SP_FALSE;
}

void sp_rwlock_wrlock(sp_rwlock *rw)
{
    for (;;) {
        uint32_t s = __atomic_load_n(&rw->state, __ATOMIC_RELAXED);
        if (s == 0) {
            /* Free: claim it exclusively. ACQUIRE pairs with the RELEASE in the
             * previous holder's unlock. */
            if (__atomic_compare_exchange_n(&rw->state, &s, SP_RW_WRITER,
                                            0, __ATOMIC_ACQUIRE, __ATOMIC_RELAXED))
                return;
            /* Lost the race to another writer or an arriving reader; retry. */
        } else {
            /* Readers and/or a writer are in. Sleep until the word changes.
             * WAIT re-checks state == s: if it already dropped to 0 we loop and
             * grab it instead of sleeping. We are woken on the reader 1->0 edge
             * (rdunlock) or by a writer's unlock. Because we only ever sleep
             * when state != 0 and are always woken when it reaches 0, no wakeup
             * is lost — though a stream of readers can starve us (reader-pref). */
            sp_futex_wait(&rw->state, s);
        }
    }
}

void sp_rwlock_wrunlock(sp_rwlock *rw)
{
    /* Release exclusive ownership: store 0. RELEASE so all writer updates are
     * visible before any reader/writer observes the lock as free. */
    __atomic_store_n(&rw->state, 0, __ATOMIC_RELEASE);
    /* Wake EVERYONE: any number of readers plus writers may be parked, and after
     * a writer leaves they should all get a chance. Woken readers CAS the count
     * up (reader-preference tends to let them in first); a woken writer CASes
     * 0 -> WRITER. The losers simply loop and re-park. */
    sp_futex_wake(&rw->state, SP_INT_MAX);
}
