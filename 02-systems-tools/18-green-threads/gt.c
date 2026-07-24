/* ===========================================================================
 * gt.c — the cooperative scheduler: mmap'd stacks, a run queue, and the glue
 *         around the hand-written switch in switch.S.
 * ===========================================================================
 *
 * Reading order:
 *   1. Stack allocation      — mmap a stack + a PROT_NONE guard page.
 *   2. The fake initial frame — how a brand-new task is made resumable.
 *   3. The run queue          — a plain intrusive FIFO.
 *   4. spawn / yield / join   — the public primitives.
 *   5. gt_run                 — the scheduler loop that drives it all.
 *
 * The one non-C mechanism, gt_switch(), lives in switch.S; gt_trampoline (the
 * launch pad for a new task) and gt_coro_exit (called when a task's entry fn
 * returns) form the C<->asm boundary. Everything else is portable C.
 *
 * Platform: Linux / WSL. Uses mmap(2), mprotect(2), munmap(2), sysconf(3).
 * ===========================================================================
 */

#include "gt.h"

#include <sys/mman.h>   /* mmap, munmap, mprotect, MAP_*, PROT_*  */
#include <unistd.h>     /* sysconf, _SC_PAGESIZE                  */
#include <stdint.h>     /* uintptr_t, uint64_t                    */
#include <stdio.h>      /* fprintf (error reporting only)         */
#include <stdlib.h>     /* malloc, free                           */
#include <string.h>     /* (unused syscalls aside) — kept minimal */
#include <errno.h>      /* errno for mmap/mprotect diagnostics    */

/* ---------------------------------------------------------------------------
 * Symbols provided by switch.S. Declared here so the C side can (a) call the
 * switch and (b) take the address of the trampoline to plant in a fake frame.
 * ------------------------------------------------------------------------- */
extern void gt_switch(gt_context *from, gt_context *to);
extern void gt_trampoline(void);        /* never called directly from C       */

/* Default USABLE stack per task if the caller passes 0. 64 KiB is generous for
 * cooperative tasks that don't recurse deeply; the guard page catches the rest. */
#define GT_DEFAULT_STACK  (64 * 1024)

/* ---------------------------------------------------------------------------
 * Global scheduler state. A cooperative N:1 library has exactly ONE running
 * context at a time, so a handful of file-scope globals is the honest model —
 * there is no data race here because there is no parallelism (no second thread
 * touches these). A true M:N design (see the README) would make all of this
 * per-worker and add real synchronisation.
 * ------------------------------------------------------------------------- */
static gt_task  g_sched;              /* the scheduler itself, on the OS stack  */
static gt_task *g_current;            /* the task currently on the CPU          */
static gt_task *g_runq_head;          /* FIFO run queue: dequeue at head...     */
static gt_task *g_runq_tail;          /* ...enqueue at tail                     */
static gt_task *g_all_head;           /* every task ever spawned, for teardown  */
static int      g_next_id = 1;        /* human-friendly ids; 0 is the scheduler */
static long     g_pagesize;           /* cached sysconf(_SC_PAGESIZE)           */

/* ===========================================================================
 * 0. Small helpers
 * ======================================================================== */

/* Round `n` up to a multiple of `a` (a MUST be a power of two). The classic
 * branch-free trick: add (a-1) to push past the boundary, then mask the low
 * bits off to floor back down to a multiple. */
static size_t align_up(size_t n, size_t a) {
    return (n + (a - 1)) & ~(a - 1);
}

/* ===========================================================================
 * 1. Per-task stack: mmap + a guard page
 * ---------------------------------------------------------------------------
 * We give every task a fresh anonymous mapping and turn its LOWEST page into a
 * guard: PROT_NONE means any access faults. Because x86-64 stacks grow DOWNWARD
 * (rsp decreases as you push / call), a stack overflow walks off the low end of
 * the usable region straight into the guard page and takes an immediate SIGSEGV
 * — instead of silently smearing over another task's stack, which is the kind
 * of bug that costs a weekend. See guard_demo.c for a live demonstration.
 *
 *   layout (addresses increase upward):
 *
 *        base + len   ->  +-----------------------------+  <- stack TOP (rsp starts here)
 *                         |         usable stack        |
 *                         |            ...              |   grows downward
 *        base + page  ->  +-----------------------------+
 *                         |   GUARD PAGE (PROT_NONE)    |   overflow faults here
 *        base         ->  +-----------------------------+
 *
 * Returns the mmap base (which is what munmap needs) and reports, via *out_top,
 * the 16-byte-aligned highest usable address (where rsp begins). NULL on error.
 * ======================================================================== */
static void *stack_alloc(size_t usable, size_t *out_len, uintptr_t *out_top) {
    /* Round the usable region up to whole pages, then add one guard page. */
    size_t page = (size_t)g_pagesize;
    usable = align_up(usable, page);
    size_t len = usable + page;                 /* usable + one guard page      */

    /* mmap(2): syscall 9 on x86-64.
     *   rdi = addr   = NULL  -> let the kernel choose the address
     *   rsi = length = len
     *   rdx = prot   = PROT_READ|PROT_WRITE (the guard page is downgraded below)
     *   r10 = flags  = MAP_PRIVATE|MAP_ANONYMOUS (fresh zero-filled pages, no fd)
     *   r8  = fd     = -1    (ignored for anonymous)
     *   r9  = offset = 0
     * The kernel installs a new VMA (virtual memory area) and returns its base,
     * or MAP_FAILED == (void*)-1 on error with errno set (ENOMEM etc.). Pages
     * are demand-zero: physical frames are only committed when first touched. */
    void *base = mmap(NULL, len, PROT_READ | PROT_WRITE,
                      MAP_PRIVATE | MAP_ANONYMOUS, -1, 0);
    if (base == MAP_FAILED) {
        fprintf(stderr, "gt: mmap(%zu) failed: errno=%d\n", len, errno);
        return NULL;
    }

    /* mprotect(2): syscall 10 on x86-64.
     *   rdi = addr = base      (must be page-aligned; mmap guarantees it)
     *   rsi = len  = page      (exactly the lowest page)
     *   rdx = prot = PROT_NONE (no read, no write, no execute)
     * Any load/store/execute against this page now raises SIGSEGV. This is the
     * guard. Errors: EINVAL (bad range), ENOMEM (kernel can't split the VMA). */
    if (mprotect(base, page, PROT_NONE) != 0) {
        fprintf(stderr, "gt: mprotect(guard) failed: errno=%d\n", errno);
        munmap(base, len);                      /* don't leak the mapping       */
        return NULL;
    }

    /* rsp starts at the very top and must be 16-byte aligned per the ABI. mmap
     * returns page-aligned memory so base+len is already 16-aligned, but we mask
     * anyway to make the invariant explicit and robust to odd page sizes. */
    uintptr_t top = ((uintptr_t)base + len) & ~(uintptr_t)0xF;

    *out_len = len;
    *out_top = top;
    return base;
}

/* ===========================================================================
 * 2. The fake initial frame
 * ---------------------------------------------------------------------------
 * gt_switch resumes a task by loading its rsp and popping six callee-saved
 * registers followed by a `ret`. A brand-new task has never been switched away
 * from, so nothing has pushed those values. We synthesise them: we lay down, at
 * the top of the new stack, exactly the bytes gt_switch expects to pop, so the
 * FIRST switch into the task lands in gt_trampoline with fn in r12 and arg in
 * r13. This is the single most delicate arithmetic in the library.
 *
 * Stack image we build (addresses INCREASE downward in this table, i.e. the
 * first row is the lowest address = where rsp will point):
 *
 *     ctx.rsp -> [ r15 slot ] = 0                (popped first)
 *                [ r14 slot ] = 0
 *                [ r13 slot ] = arg              -> arrives in r13
 *                [ r12 slot ] = fn               -> arrives in r12
 *                [ rbx slot ] = 0
 *                [ rbp slot ] = 0                (popped last)
 *                [ ret addr ] = gt_trampoline    (the final `ret` jumps here)
 *
 * That is 7 words = 56 bytes below `top`. Since `top` is 16-aligned, after
 * gt_switch pops the 6 registers (48 bytes) and `ret` consumes the 7th word,
 * rsp equals `top` again on entry to gt_trampoline — 16-aligned, as required.
 * ======================================================================== */
static void frame_init(gt_task *t, uintptr_t top, void (*fn)(void *), void *arg) {
    /* Reserve the 7-word fake frame just below the top of the stack. */
    uint64_t *frame = (uint64_t *)(top - 7 * sizeof(uint64_t));

    frame[0] = 0;                        /* r15 = 0 (fresh register file)       */
    frame[1] = 0;                        /* r14 = 0                             */
    frame[2] = (uint64_t)(uintptr_t)arg; /* r13 -> gt_trampoline moves to rdi   */
    frame[3] = (uint64_t)(uintptr_t)fn;  /* r12 -> gt_trampoline calls it       */
    frame[4] = 0;                        /* rbx = 0                             */
    frame[5] = 0;                        /* rbp = 0 (clean frame chain base)    */
    frame[6] = (uint64_t)(uintptr_t)gt_trampoline; /* the address `ret` lands on*/

    /* Park the task at the bottom of the fake frame; the first gt_switch pops
     * upward through it. */
    t->ctx.rsp = (void *)frame;
}

/* ===========================================================================
 * 3. The run queue — an intrusive singly-linked FIFO
 * ---------------------------------------------------------------------------
 * "Intrusive" = the link pointer (rq_next) lives inside gt_task itself, so
 * enqueuing costs no allocation. FIFO ordering is what makes yield() a fair,
 * round-robin scheduler: a task that yields goes to the BACK and everyone else
 * runs before it comes up again.
 * ======================================================================== */
static void runq_push(gt_task *t) {
    t->rq_next = NULL;
    if (g_runq_tail)
        g_runq_tail->rq_next = t;        /* link old tail -> new node           */
    else
        g_runq_head = t;                 /* queue was empty: new node is head    */
    g_runq_tail = t;                     /* new node is always the tail          */
}

static gt_task *runq_pop(void) {
    gt_task *t = g_runq_head;
    if (!t)
        return NULL;                     /* empty                               */
    g_runq_head = t->rq_next;
    if (!g_runq_head)
        g_runq_tail = NULL;              /* queue became empty: fix the tail     */
    t->rq_next = NULL;
    return t;
}

/* ===========================================================================
 * 4. Public primitives
 * ======================================================================== */

void gt_init(void) {
    /* Cache the page size once. sysconf(_SC_PAGESIZE) reads it from the auxv the
     * kernel handed us at exec — no syscall per call. Typically 4096 on x86-64. */
    if (g_pagesize == 0)
        g_pagesize = sysconf(_SC_PAGESIZE);
    if (g_pagesize <= 0)
        g_pagesize = 4096;               /* absurd fallback; should never happen */

    g_sched.id = 0;                      /* the scheduler is "task 0"            */
    g_sched.state = GT_RUNNING;
    g_current = &g_sched;                /* before any switch, WE are running    */
}

gt_task *gt_spawn(void (*fn)(void *), void *arg, size_t stack_bytes) {
    if (stack_bytes == 0)
        stack_bytes = GT_DEFAULT_STACK;

    /* The task struct itself is a tiny heap allocation (a few dozen bytes); the
     * big memory is the mmap'd stack. Owner: the library frees both at teardown
     * (gt_run's tail) — see the README's note on task lifetime. */
    gt_task *t = (gt_task *)malloc(sizeof(*t));
    if (!t)
        return NULL;

    size_t len;
    uintptr_t top;
    t->stack_base = stack_alloc(stack_bytes, &len, &top);
    if (!t->stack_base) {
        free(t);
        return NULL;                     /* mmap/mprotect already reported why   */
    }
    t->stack_len    = len;
    t->state        = GT_READY;
    t->join_waiter  = NULL;
    t->id           = g_next_id++;

    /* Lay down the fake frame so the first switch launches fn(arg). */
    frame_init(t, top, fn, arg);

    /* Track it for teardown, then make it runnable. */
    t->all_next = g_all_head;
    g_all_head  = t;
    runq_push(t);
    return t;
}

void gt_yield(void) {
    gt_task *self = g_current;
    /* Put ourselves back on the run queue (FIFO => we run again after everyone
     * else) and hand the CPU to the scheduler. The scheduler will pick the next
     * ready task. When we are eventually resumed, gt_switch returns right here. */
    self->state = GT_READY;
    runq_push(self);
    gt_switch(&self->ctx, &g_sched.ctx);
    /* ---- resumed later; g_current has been restored to `self` by gt_run ---- */
}

void gt_join(gt_task *t) {
    gt_task *self = g_current;
    /* Cooperative join: while the target is alive, mark ourselves WAITING (so
     * the scheduler does NOT put us back on the run queue) and switch away. When
     * `t` finally dies, gt_coro_exit re-arms us as READY, so we wake, re-check,
     * and fall out of the loop. Single-waiter only (documented in gt.h). */
    while (t->state != GT_DEAD) {
        self->state = GT_WAITING;
        t->join_waiter = self;           /* "wake me when you die"               */
        gt_switch(&self->ctx, &g_sched.ctx);
    }
}

gt_task *gt_self(void) {
    return g_current;
}

/* ---------------------------------------------------------------------------
 * gt_coro_exit — the C landing pad called by gt_trampoline when a task's entry
 * function returns. Runs while still ON the finished task's stack, so it must
 * NOT free that stack here; it flips state, wakes a joiner, and switches to the
 * scheduler, which reclaims the (now-inactive) stack. Declared noreturn in the
 * sense that it never comes back to its caller — control leaves via gt_switch.
 * It is called from asm (switch.S), hence the external linkage and C name.
 * ------------------------------------------------------------------------- */
void gt_coro_exit(void);   /* forward decl with C linkage (this IS C) */
void gt_coro_exit(void) {
    gt_task *self = g_current;
    self->state = GT_DEAD;

    /* If someone is blocked in gt_join() on us, make them runnable again. */
    if (self->join_waiter) {
        self->join_waiter->state = GT_READY;
        runq_push(self->join_waiter);
        self->join_waiter = NULL;
    }

    /* Final switch off this stack. We never come back, so this stack is safe for
     * the scheduler to munmap once control returns to it. */
    gt_switch(&self->ctx, &g_sched.ctx);

    /* Unreachable: gt_switch above never returns into a dead task. */
    __builtin_unreachable();
}

/* ===========================================================================
 * 5. The scheduler loop
 * ---------------------------------------------------------------------------
 * Runs on the ordinary OS-thread stack (g_sched's "context" simply records
 * where that stack was parked during a switch). Each iteration: pick the next
 * ready task, switch into it, and — when it switches back — reclaim it if it
 * died. Loop until the run queue is empty.
 * ======================================================================== */
void gt_run(void) {
    for (;;) {
        gt_task *t = runq_pop();
        if (!t)
            break;                       /* nothing ready: the run is over       */

        t->state  = GT_RUNNING;
        g_current = t;
        /* Save the scheduler's context and jump into the task. Control returns
         * here when the task yields, blocks in join, or exits. */
        gt_switch(&g_sched.ctx, &t->ctx);

        /* Back in the scheduler. We are the running context again. */
        g_current = &g_sched;

        /* If the task finished, reclaim its stack now — it is no longer running
         * on it, and we are safely on the OS stack. We keep the tiny task struct
         * around (a joiner may still read t->state; and teardown frees it). */
        if (t->state == GT_DEAD && t->stack_base) {
            /* munmap(2): syscall 11. rdi=addr(base), rsi=length(full mapping).
             * Returns the whole VMA — guard page included — to the kernel. */
            munmap(t->stack_base, t->stack_len);
            t->stack_base = NULL;
        }
        /* If the task is GT_READY it already re-queued itself in gt_yield.
         * If GT_WAITING it is blocked in join and will be re-armed on the
         * target's death. Either way, nothing more to do here. */
    }

    /* Run drained. Free every task struct we created and any stack that somehow
     * survived (e.g. a task left permanently WAITING in a deadlock). This makes
     * the library leak-free per run; a fresh gt_run after more spawns starts
     * clean. Ownership ends here. */
    gt_task *p = g_all_head;
    while (p) {
        gt_task *next = p->all_next;
        if (p->stack_base)
            munmap(p->stack_base, p->stack_len);
        free(p);
        p = next;
    }
    g_all_head = NULL;
    g_runq_head = g_runq_tail = NULL;
    g_current = &g_sched;
}
