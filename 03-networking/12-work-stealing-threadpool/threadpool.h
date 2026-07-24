/* ===========================================================================
 * threadpool.h — a work-stealing thread pool built on the Chase-Lev deque.
 * ===========================================================================
 *
 * Structure (one deque per worker; see chase_lev.h):
 *
 *   worker 0        worker 1        worker 2        worker 3
 *   [deque]         [deque]         [deque]         [deque]
 *   push/take       push/take       push/take       push/take   <- owner, LIFO
 *      ^  \____steal____^  \____steal____^  \____steal____^      <- thieves, FIFO
 *      |
 *   injector (mutex FIFO)  <- external submits land here (owner-only push means
 *                             a non-worker thread may NOT touch a worker deque)
 *
 * Idle workers PARK on a futex instead of spinning, so an idle pool burns no CPU.
 * A monotonically-increasing `gate` word acts as an eventcount: any submit bumps
 * it, which both wakes parked workers (FUTEX_WAKE) and closes the lost-wakeup
 * window (a worker that read the old gate value will fail its FUTEX_WAIT compare
 * and re-check instead of sleeping through the new work).
 *
 * Platform: Linux only. It uses the raw futex(2) syscall, sched_setaffinity(2),
 * pthreads, and C11 atomics. It will not build on macOS/Windows (no futex, no
 * sched_setaffinity). See the README for WSL/VM instructions.
 * ===========================================================================
 */
#ifndef THREADPOOL_H
#define THREADPOOL_H

/* A unit of work: call fn(arg). The pool owns neither `arg`'s lifetime nor what
 * it points to — fn is responsible for freeing arg if it was heap-allocated
 * (the demo does exactly this). The Task box itself is freed by the pool. */
typedef void (*tp_task_fn)(void *arg);

typedef struct tp_pool tp_pool;

/* Create a pool with `nworkers` worker threads, each pinned to a CPU. Pass 0 to
 * use one worker per online CPU (sysconf(_SC_NPROCESSORS_ONLN)). Returns NULL on
 * failure (allocation, thread spawn, ...). */
tp_pool *tp_create(int nworkers);

/* Submit fn(arg) to the pool. Safe to call from OUTSIDE the pool (e.g. main) and
 * from INSIDE a running task (the common fork-join case). Returns 0, or -1 if
 * the task box could not be allocated. See threadpool.c for how the two callers
 * take different paths (worker-local push vs. the shared injector). */
int tp_submit(tp_pool *p, tp_task_fn fn, void *arg);

/* Block the CALLING thread until every submitted task (transitively) has
 * completed — i.e. the outstanding-task count reaches zero. Intended to be
 * called from a non-worker thread (main). Calling it from within a task would
 * deadlock that worker, so don't. */
void tp_wait(tp_pool *p);

/* Signal shutdown, wake and join all workers, then free everything. Call tp_wait
 * first for a graceful drain; any tasks still queued at destroy time are
 * abandoned (see the README's "clean shutdown" note). */
void tp_destroy(tp_pool *p);

/* Number of workers (handy for the demo to report load balance). */
int tp_nworkers(tp_pool *p);

/* Tasks executed by worker `i` since creation (single-writer counter, safe to
 * read after tp_destroy has joined the threads — or approximately at any time).
 * Returns -1 if i is out of range. */
long tp_worker_tasks(tp_pool *p, int i);

#endif /* THREADPOOL_H */
