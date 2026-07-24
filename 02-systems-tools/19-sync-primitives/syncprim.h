/* ===========================================================================
 * syncprim.h — hand-rolled synchronization primitives on top of futex(2).
 * ===========================================================================
 *
 * This header declares five locks that most programmers only ever consume from
 * a library: a spinlock, a mutex, a condition variable, a counting semaphore,
 * and a reader-writer lock. Here we BUILD them, from two ingredients only:
 *
 *   1. C11/GCC atomic read-modify-write builtins (`__atomic_*`) with EXPLICIT
 *      memory ordering. These compile to ordinary loads/stores plus, where
 *      needed, `lock`-prefixed instructions — no operating system involved.
 *
 *   2. The Linux futex(2) syscall, used ONLY when a thread must actually sleep.
 *      "futex" = *fast userspace mutex*: the fast, uncontended path never enters
 *      the kernel at all; the kernel is asked to park/wake a thread only when
 *      there is genuine contention. That split — a userspace atomic for the
 *      common case, a syscall for the rare blocking case — is the whole idea.
 *
 * WHY THIS FILE IS FREESTANDING (no <pthread.h>, no <stdatomic.h>, no libc)
 * ------------------------------------------------------------------------
 * The implementation in syncprim.c pulls in NO system headers: it issues the
 * futex syscall with a raw inline-asm `syscall` instruction and uses compiler
 * atomic builtins for the ordering. That is deliberate — it means the entire
 * real implementation cross-compiles to Linux assembly on any host, so the
 * committed asm/ files reflect *actual project code*, not a toy. The only
 * header we lean on is <stdint.h>, which the compiler itself provides even in
 * freestanding mode (it is not part of the C library).
 *
 * A futex word is ALWAYS a naturally-aligned 32-bit integer. The kernel keys
 * its wait queues on the *physical address* of that word, and it only ever
 * loads/compares 32 bits. So every "futex word" below is a uint32_t, and we
 * hand its address straight to the syscall.
 *
 * Platform: Linux (or WSL2). The atomics are portable; futex(2) is Linux-only.
 * ===========================================================================
 */
#ifndef SYNCPRIM_H
#define SYNCPRIM_H

#include <stdint.h>   /* uint32_t / int32_t — freestanding, compiler-provided  */

/* A tiny bool without <stdbool.h> so the header stays freestanding. */
#ifndef __cplusplus
typedef int sp_bool;
#endif
#define SP_TRUE  1
#define SP_FALSE 0

/* The x86-64 cache line is 64 bytes. Two futex words that share one line
 * "false-share": a write to either bounces the line between cores even though
 * the logical objects are independent. Hot locks in a real system should be
 * padded/aligned to a line. We expose the constant and use _Alignas selectively
 * rather than bloating every struct; see the note on false sharing in README. */
#define SP_CACHELINE 64

/* ---------------------------------------------------------------------------
 * spinlock — the simplest lock: never sleeps, just burns CPU until it wins.
 *
 * `locked` is the whole state: 0 = free, 1 = held. There is no futex here and
 * no syscall EVER; a spinlock is correct only when the critical section is a
 * handful of instructions and you would rather burn cycles than pay the ~1-2 us
 * of a context switch. Held too long, it wastes an entire core.
 * ------------------------------------------------------------------------- */
typedef struct sp_spinlock {
    uint32_t locked;    /* accessed ONLY via __atomic_* — never a plain read   */
} sp_spinlock;

#define SP_SPINLOCK_INIT { 0 }

/* ---------------------------------------------------------------------------
 * mutex — the classic fast-path / slow-path futex lock (Drepper's mutex2).
 *
 * `state` is a THREE-state futex word, and the three states are the crux of the
 * whole design (they are what make wakeups impossible to lose):
 *
 *     0  UNLOCKED         — free.
 *     1  LOCKED           — held, and NO thread is (known to be) waiting.
 *     2  LOCKED_CONTENDED — held, and at least one thread may be asleep in the
 *                           kernel on this word, so unlock MUST issue a wake.
 *
 * Why not just two states (0/1)? Because unlock would then never know whether a
 * FUTEX_WAKE is required. If it always woke, every unlock pays a syscall even
 * with no waiters (slow). If it never woke, a thread that went to sleep would
 * sleep forever (a *lost wakeup* — deadlock). The third state records "there is
 * a sleeper" so unlock wakes exactly when it must, and never otherwise.
 * ------------------------------------------------------------------------- */
typedef struct sp_mutex {
    uint32_t state;     /* 0 unlocked / 1 locked / 2 locked+waiters            */
} sp_mutex;

#define SP_MUTEX_INIT { 0 }

/* ---------------------------------------------------------------------------
 * cond — a condition variable, always used WITH a mutex.
 *
 * The entire state is a monotonically increasing `seq` counter. Every signal or
 * broadcast bumps it. A waiter samples `seq` *while holding the mutex*, releases
 * the mutex, then asks the kernel to sleep "only if seq still equals my sample."
 * If a signaler bumped seq in the race window between unlock and the syscall,
 * the futex value no longer matches and the kernel returns immediately instead
 * of sleeping — that atomic compare-and-sleep is exactly what prevents the
 * lost-wakeup that a naive condvar would suffer.
 * ------------------------------------------------------------------------- */
typedef struct sp_cond {
    uint32_t seq;       /* bumped on every signal/broadcast; the futex word    */
} sp_cond;

#define SP_COND_INIT { 0 }

/* ---------------------------------------------------------------------------
 * sem — a counting semaphore. `count` = number of permits currently available.
 *
 * wait() decrements when a permit exists (pure userspace CAS, no syscall) and
 * otherwise sleeps until a post() makes count > 0. post() increments and wakes
 * one sleeper. The futex value protocol is trivial here: a sleeper waits on the
 * value 0, and any post changes the value away from 0, so a post that races a
 * would-be sleeper cannot be lost.
 * ------------------------------------------------------------------------- */
typedef struct sp_sem {
    uint32_t count;     /* available permits (>= 0); the futex word            */
} sp_sem;

#define SP_SEM_INIT(n) { (n) }

/* ---------------------------------------------------------------------------
 * rwlock — many concurrent readers OR one exclusive writer. Reader-preference.
 *
 * A single 32-bit `state` word encodes everything:
 *
 *     state == 0                : free
 *     0 < state <= READER_MAX   : that many readers hold the lock
 *     state == SP_RW_WRITER     : one writer holds the lock (top bit set)
 *
 * The high bit (SP_RW_WRITER) is the "a writer is in" flag; the low bits are the
 * live reader count. A reader can only enter when the writer bit is clear; a
 * writer can only enter when the word is exactly 0. This variant is READER-
 * PREFERRING: a steady stream of readers can starve a writer. That trade-off is
 * deliberate and documented (writer-preference is the stretch goal).
 * ------------------------------------------------------------------------- */
#define SP_RW_WRITER   0x80000000u   /* high bit: a writer holds the lock      */
#define SP_RW_RDMASK   0x7fffffffu   /* low 31 bits: number of active readers  */

typedef struct sp_rwlock {
    uint32_t state;     /* writer bit | reader count; the futex word           */
} sp_rwlock;

#define SP_RWLOCK_INIT { 0 }

/* ===========================================================================
 * Public API. Every function checks the fast path in userspace first and only
 * calls into the kernel (futex) on the slow, contended path.
 * =========================================================================== */

/* spinlock ------------------------------------------------------------------ */
void    sp_spin_init(sp_spinlock *s);
void    sp_spin_lock(sp_spinlock *s);       /* spins with PAUSE until acquired */
sp_bool sp_spin_trylock(sp_spinlock *s);    /* one attempt; SP_TRUE on success */
void    sp_spin_unlock(sp_spinlock *s);

/* mutex --------------------------------------------------------------------- */
void    sp_mutex_init(sp_mutex *m);
void    sp_mutex_lock(sp_mutex *m);         /* fast-path CAS; futex on contention */
sp_bool sp_mutex_trylock(sp_mutex *m);      /* never blocks/syscalls           */
void    sp_mutex_unlock(sp_mutex *m);       /* futex-wakes only if contended    */

/* condition variable -------------------------------------------------------- */
void    sp_cond_init(sp_cond *c);
void    sp_cond_wait(sp_cond *c, sp_mutex *m);  /* atomically unlock+sleep, relock */
void    sp_cond_signal(sp_cond *c);             /* wake one waiter             */
void    sp_cond_broadcast(sp_cond *c);          /* wake all waiters            */

/* counting semaphore -------------------------------------------------------- */
void    sp_sem_init(sp_sem *s, uint32_t value);
void    sp_sem_wait(sp_sem *s);             /* P/acquire: decrement or sleep    */
sp_bool sp_sem_trywait(sp_sem *s);          /* non-blocking acquire            */
void    sp_sem_post(sp_sem *s);             /* V/release: increment + wake one  */

/* reader-writer lock -------------------------------------------------------- */
void    sp_rwlock_init(sp_rwlock *rw);
void    sp_rwlock_rdlock(sp_rwlock *rw);
sp_bool sp_rwlock_tryrdlock(sp_rwlock *rw);
void    sp_rwlock_rdunlock(sp_rwlock *rw);
void    sp_rwlock_wrlock(sp_rwlock *rw);
sp_bool sp_rwlock_trywrlock(sp_rwlock *rw);
void    sp_rwlock_wrunlock(sp_rwlock *rw);

#endif /* SYNCPRIM_H */
