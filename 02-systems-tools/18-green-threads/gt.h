/* ===========================================================================
 * gt.h — public API for a tiny cooperative green-thread (coroutine) library.
 * ===========================================================================
 *
 * A "green thread" is a thread scheduled entirely in USER space: the kernel
 * sees one OS thread, but that thread multiplexes many independent stacks and
 * instruction pointers. Switching between them never enters the kernel — no
 * `clone(2)`, no scheduler tick, no context-switch syscall. The whole cost of a
 * switch is ~a dozen `mov`/`push`/`pop` instructions (see switch.S).
 *
 * This is COOPERATIVE (a.k.a. N:1): a running task keeps the CPU until it calls
 * gt_yield() (or gt_join(), or returns). There is no preemption in the teaching
 * core — see the README's "Going further" for the timer-signal preemption path.
 *
 * The model, in one breath:
 *   - Each task has its own mmap'd stack (with a guard page below it).
 *   - A "context" is just the saved stack pointer; every other callee-saved
 *     register lives ON that stack, pushed by the switch routine.
 *   - gt_switch(from, to) (in switch.S) saves the current callee-saved state
 *     onto `from`'s stack and restores `to`'s — swapping which stack is live.
 *   - A FIFO run queue + a scheduler loop (gt_run) turn that raw switch into a
 *     round-robin scheduler with spawn / yield / join.
 *
 * Platform: Linux / WSL only. The stack allocator uses mmap(2)+mprotect(2) and
 * the switch is x86-64 assembly. The generated teaching assembly under asm/ is
 * host-portable (clang cross-targets Linux).
 * ===========================================================================
 */
#ifndef GT_H
#define GT_H

#include <stddef.h>   /* size_t */

/* ---------------------------------------------------------------------------
 * gt_context — the ENTIRE saved state of a suspended task, as far as the CPU
 * is concerned, is a single pointer: its stack pointer.
 *
 * Why only rsp? Because gt_switch() pushes every callee-saved register (rbx,
 * rbp, r12-r15) onto the task's own stack before parking it, and the return
 * address is already on that stack (put there by the `call gt_switch`). So the
 * top of a parked task's stack fully describes how to resume it; we only need
 * to remember WHERE that top is. That single word is `rsp`.
 *
 * INVARIANT the assembly relies on: `rsp` is at offset 0. switch.S does
 * `movq %rsp, (%rdi)` / `movq (%rsi), %rsp` with no displacement. Do not add a
 * field before it.
 * ------------------------------------------------------------------------- */
typedef struct gt_context {
    void *rsp;              /* saved stack pointer of a suspended task */
} gt_context;

/* Task lifecycle states. The scheduler and the primitives below transition a
 * task between these; the run queue only ever holds GT_READY tasks. */
typedef enum gt_state {
    GT_READY,              /* on the run queue, waiting for a turn        */
    GT_RUNNING,            /* currently executing (exactly one at a time) */
    GT_WAITING,            /* blocked in gt_join(), off the run queue     */
    GT_DEAD                /* entry function returned; stack reclaimable  */
} gt_state;

/* ---------------------------------------------------------------------------
 * gt_task — one green thread. The library allocates these; callers treat the
 * pointer as an opaque handle (only `id` and `state` are meant to be read).
 *
 * MEMORY: `ctx` MUST be the first member so that &task->ctx == (gt_context*)task
 * and, more importantly, so the raw asm sees rsp at the very start. `stack_base`
 * is the mmap() return (the low guard page included); it is what we munmap().
 * ------------------------------------------------------------------------- */
typedef struct gt_task {
    gt_context ctx;               /* saved rsp — MUST be first (see above)     */
    gt_state   state;             /* READY / RUNNING / WAITING / DEAD          */
    void      *stack_base;        /* mmap base (guard page + usable); for munmap*/
    size_t     stack_len;         /* full mmap length in bytes                 */
    struct gt_task *rq_next;      /* intrusive run-queue link (FIFO)           */
    struct gt_task *all_next;     /* intrusive "all tasks" link (for teardown) */
    struct gt_task *join_waiter;  /* the one task blocked in gt_join() on us   */
    int        id;                /* small human-friendly id (0 = scheduler)   */
} gt_task;

/* ---- library lifecycle ---------------------------------------------------- */

/* Initialise the scheduler. Call once before any spawn/run. Idempotent. */
void gt_init(void);

/* Run the scheduler loop on the CURRENT OS thread until the run queue drains
 * (every spawned task has finished or is permanently blocked). Returns to its
 * caller on the ordinary C stack. Safe to call again after more spawns. */
void gt_run(void);

/* ---- creating and steering tasks ------------------------------------------ */

/* Spawn a new task that will call fn(arg) when first scheduled. `stack_bytes`
 * is the USABLE stack size (0 => a sensible default); a guard page is added
 * below it. Returns the task handle, or NULL if the stack mmap failed. The
 * task is placed on the run queue immediately but does not run until the
 * scheduler reaches it. */
gt_task *gt_spawn(void (*fn)(void *), void *arg, size_t stack_bytes);

/* Cooperatively give up the CPU: the current task goes to the back of the run
 * queue and the scheduler picks the next ready task. Returns when the current
 * task is scheduled again. Must be called from within a task (not the loop). */
void gt_yield(void);

/* Block the current task until `t` has finished (reached GT_DEAD). Cooperative:
 * internally yields to the scheduler until `t` dies. Must be called from within
 * a task. Reading `t` after join returns is fine (the handle outlives the run;
 * see the README on lifetime). */
void gt_join(gt_task *t);

/* The currently-running task's handle (NULL before the first switch). */
gt_task *gt_self(void);

#endif /* GT_H */
