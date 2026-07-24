/* ===========================================================================
 * sched.c — cooperative green threads (coroutines) on ucontext.
 * ===========================================================================
 *
 * Platform: Linux/x86-64. Two tasks hand control back and forth via gtYield(),
 * driven by a round-robin scheduler, all in one OS thread. The whole point of a
 * green thread is that a context switch is NOT a syscall — swapcontext() just
 * saves the callee-saved registers + stack pointer and loads another set. No
 * kernel scheduler, no ~1µs trap; a switch is tens of nanoseconds.
 *
 * WHY NO LOCKS: this scheduler is cooperative and single-threaded. A task runs
 * uninterrupted until it *chooses* to yield, so there is no preemption and thus
 * no data race on shared state (the ready queue, `current`) between yields. That
 * is the concurrency model coroutines give you: interleaving without atomics.
 * (The tradeoff: one task that never yields starves the rest.)
 *
 * Each task's stack is its own mmap'd region with a PROT_NONE GUARD PAGE at the
 * low end. Stacks grow downward, so an overflow runs off the bottom into the
 * guard and faults immediately (SIGSEGV) instead of silently smashing a neighbour.
 *
 * We build on POSIX <ucontext.h> for a correct, portable switch. Sibling
 * 02-systems-tools/18-green-threads shows the ~20-instruction hand-written
 * `switch.S` that replaces swapcontext once you understand exactly which
 * registers must be preserved (rbx, rbp, r12-r15, rsp, and the return address).
 */
#define _GNU_SOURCE
#include <stdio.h>
#include <stdlib.h>
#include <ucontext.h>
#include <sys/mman.h>
#include <unistd.h>

#include "sched.h"

#define TASK_STACK_BYTES (64u * 1024u)      /* 64 KiB usable stack per task       */

typedef void (*TaskFn)(void *arg);

typedef struct Task {
    ucontext_t   ctx;          /* saved register/stack context when not running   */
    struct Task *next;         /* ready-queue link                                */
    void        *map;          /* mmap base: [guard page | usable stack]          */
    size_t       mapLen;       /* bytes to munmap                                 */
    TaskFn       fn;
    void        *arg;
    int          id;
    int          done;         /* set by the trampoline when fn returns           */
} Task;

/* Scheduler state (single-threaded, so plain globals are fine). */
static ucontext_t schedCtx;    /* the scheduler's own context (returned to on yield)*/
static Task      *readyHead = NULL;
static Task      *readyTail = NULL;
static Task      *current   = NULL;   /* the task currently on the CPU            */
static int        nextId    = 1;

static void enqueue(Task *t)
{
    t->next = NULL;
    if (readyTail != NULL) readyTail->next = t;
    else                   readyHead = t;
    readyTail = t;
}
static Task *dequeue(void)
{
    Task *t = readyHead;
    if (t != NULL) {
        readyHead = t->next;
        if (readyHead == NULL) readyTail = NULL;
    }
    return t;
}

/* First code that runs on a fresh task's stack. makecontext can't pass a pointer
 * argument portably, so we read the task from the `current` global instead. When
 * fn returns, mark done and swap back to the scheduler (which will reap us). */
static void taskTrampoline(void)
{
    current->fn(current->arg);
    current->done = 1;
    swapcontext(&current->ctx, &schedCtx);   /* return to scheduler; never resumes */
}

/* Create a task: map a guarded stack, initialize a ucontext that will start in
 * taskTrampoline, and put it on the ready queue. */
static Task *gtSpawn(TaskFn fn, void *arg)
{
    long   pg      = sysconf(_SC_PAGESIZE);
    size_t pagesz  = (pg > 0) ? (size_t)pg : 4096u;
    size_t mapLen  = pagesz + TASK_STACK_BYTES;    /* guard page + stack          */

    /* mmap the whole region RW, then revoke access to the lowest page so a
     * downward stack overflow faults on the guard instead of corrupting memory. */
    char *base = (char *)mmap(NULL, mapLen, PROT_READ | PROT_WRITE,
                              MAP_PRIVATE | MAP_ANONYMOUS, -1, 0);
    if (base == MAP_FAILED) { perror("lumen sched: mmap"); exit(1); }
    if (mprotect(base, pagesz, PROT_NONE) != 0) {
        perror("lumen sched: mprotect guard");
        exit(1);
    }

    Task *t = (Task *)calloc(1, sizeof(Task));
    if (t == NULL) { fprintf(stderr, "lumen sched: OOM task\n"); exit(1); }
    t->map = base; t->mapLen = mapLen; t->fn = fn; t->arg = arg;
    t->id  = nextId++; t->done = 0;

    getcontext(&t->ctx);                       /* seed with the current context   */
    t->ctx.uc_stack.ss_sp   = base + pagesz;   /* usable stack sits ABOVE the guard*/
    t->ctx.uc_stack.ss_size = TASK_STACK_BYTES;
    t->ctx.uc_link          = NULL;            /* we return via swapcontext, not this*/
    makecontext(&t->ctx, taskTrampoline, 0);   /* entry point on the new stack     */

    enqueue(t);
    return t;
}

/* Cooperative reschedule: put the running task at the back of the queue and jump
 * to the scheduler. swapcontext SAVES our registers into current->ctx and LOADS
 * schedCtx — when we're next scheduled, execution resumes right here. */
static void gtYield(void)
{
    Task *self = current;
    enqueue(self);
    swapcontext(&self->ctx, &schedCtx);
}

/* Round-robin: run each ready task until the queue drains. When control comes
 * back (task yielded or finished), reap finished tasks. */
static void gtRun(void)
{
    for (;;) {
        Task *t = dequeue();
        if (t == NULL) break;               /* nothing ready: all done            */
        current = t;
        swapcontext(&schedCtx, &t->ctx);    /* save scheduler, jump into the task  */
        current = NULL;                     /* back in the scheduler               */
        if (t->done) {                      /* task returned: free its stack        */
            munmap(t->map, t->mapLen);
            free(t);
        }
    }
}

/* The task body used by the demo: announce a few steps, yielding between each so
 * the other coroutine interleaves. */
static void worker(void *arg)
{
    const char *name = (const char *)arg;
    for (int step = 0; step < 3; step++) {
        printf("  [task %d: %s] step %d\n", current->id, name, step);
        gtYield();
    }
    printf("  [task %d: %s] finished\n", current->id, name);
}

int coroDemo(void)
{
    printf("coro: two cooperative green threads, each on its own guarded mmap stack\n");
    printf("coro: swapcontext switches are userspace-only (no syscall per switch)\n");

    getcontext(&schedCtx);      /* initialize the scheduler context structure      */
    gtSpawn(worker, (void *)"ping");
    gtSpawn(worker, (void *)"pong");
    gtRun();                    /* drives the interleave to completion             */

    printf("coro: ready queue empty; all tasks joined\n");
    return 0;
}
