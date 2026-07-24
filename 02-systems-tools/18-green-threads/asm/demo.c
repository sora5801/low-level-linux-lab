/* ===========================================================================
 * demo.c — the scheduler's pure pick-next / run-queue logic, extracted so it
 *          compiles to clean Linux/SysV assembly with nothing from libc in the
 *          way. This is the file the committed asm/demo.{O0,s,O2}.s come from,
 *          and the one asm/demo.annotated.s explains instruction by instruction.
 * ===========================================================================
 *
 * WHY A SEPARATE FILE? The real scheduler (../gt.c) #includes <sys/mman.h>,
 * <unistd.h>, etc., so it cannot be cross-compiled to assembly on a non-Linux
 * host. But the *interesting* part of a scheduler — how it chooses who runs
 * next and how it lays out a brand-new task's stack — is pure register-and-
 * pointer arithmetic with no syscalls. We lift exactly that out here. Every
 * routine below mirrors one in gt.c:
 *
 *   runq_push / runq_pop  <->  the intrusive FIFO run queue in gt.c
 *   pick_next             <->  "dequeue the next GT_READY task" (round-robin)
 *   frame_init            <->  gt.c's fake-initial-frame setup (the 7 stores)
 *   align_up              <->  gt.c's stack rounding helper
 *
 * The file is DELIBERATELY freestanding: it includes no headers and declares
 * its own fixed-width types, so `clang --target=x86_64-pc-linux-gnu -S` yields
 * assembly that is 100% this logic.
 * ===========================================================================
 */

/* LP64: on x86-64 Linux, `long` and pointers are 64-bit. We spell out our own
 * types so no <stdint.h>/<stddef.h> is needed. */
typedef unsigned long  u64;
typedef unsigned long  usize;

/* Task states, matching gt.h's gt_state ordering exactly. */
enum { ST_READY = 0, ST_RUNNING = 1, ST_WAITING = 2, ST_DEAD = 3 };

/* A trimmed task node: just the fields the pure logic touches. The layout
 * mirrors gt_task (ctx first), so the offsets you see in the asm line up with
 * the real struct. */
typedef struct task {
    void        *rsp;        /* offset 0: saved stack pointer (the "context")   */
    int          state;      /* offset 8: ST_* above                            */
    struct task *rq_next;    /* offset 16: intrusive run-queue link             */
    int          id;         /* offset 24                                       */
} task;

/* A minimal FIFO queue header (head/tail), same shape as gt.c's two globals. */
typedef struct runq {
    task *head;
    task *tail;
} runq;

/* ---- align_up: round n up to a multiple of a power-of-two `a` ---------------
 * The branch-free "add (a-1), then mask the low bits" trick. Watch the asm turn
 * this into a lea + and — no division, no branch. */
usize align_up(usize n, usize a) {
    return (n + (a - 1)) & ~(a - 1);
}

/* ---- runq_push: enqueue at the tail (O(1)) ---------------------------------
 * Intrusive: no allocation, we just splice `t` on. Two cases — empty queue
 * (t becomes both head and tail) or non-empty (old tail links to t). */
void runq_push(runq *q, task *t) {
    t->rq_next = 0;
    if (q->tail)
        q->tail->rq_next = t;      /* link old tail -> t                        */
    else
        q->head = t;               /* was empty: t is the new head              */
    q->tail = t;                   /* t is always the new tail                  */
}

/* ---- runq_pop: dequeue from the head (O(1), FIFO) --------------------------
 * Returns the front task or 0 if empty; fixes up the tail when the queue
 * becomes empty. FIFO order is what makes yield() fair round-robin. */
task *runq_pop(runq *q) {
    task *t = q->head;
    if (!t)
        return 0;                  /* empty                                     */
    q->head = t->rq_next;
    if (!q->head)
        q->tail = 0;               /* queue emptied: null the tail too          */
    t->rq_next = 0;
    return t;
}

/* ---- pick_next: dequeue the next RUNNABLE task -----------------------------
 * The scheduler's decision procedure. In the common case the head is READY and
 * this is just runq_pop. But the queue can also hold stale entries (a task that
 * was marked WAITING/DEAD after being queued), so we WALK the list, unlinking
 * and skipping anything not READY, and return the first READY task (or 0). This
 * linked-list walk with an unlink-in-the-middle is the most instructive control
 * flow in the file to read as assembly. */
task *pick_next(runq *q) {
    task *prev = 0;
    task *t = q->head;
    while (t) {
        if (t->state == ST_READY) {
            /* Unlink t from wherever it sits, then hand it out. */
            if (prev)
                prev->rq_next = t->rq_next;   /* bridge over t                  */
            else
                q->head = t->rq_next;         /* t was the head                 */
            if (q->tail == t)
                q->tail = prev;               /* t was the tail                 */
            t->rq_next = 0;
            return t;
        }
        prev = t;                              /* skip non-ready; advance        */
        t = t->rq_next;
    }
    return 0;                                  /* nothing runnable               */
}

/* ---- frame_init: build a brand-new task's fake initial stack frame ----------
 * This is the arithmetic gt.c performs so the FIRST context switch into a task
 * lands in the trampoline with fn in r12 and arg in r13. Given the 16-aligned
 * stack TOP, it writes the 7-word fake frame (r15,r14,r13,r14... see below) and
 * returns the value to store as the task's saved rsp.
 *
 * The store pattern here is the whole point: seven 8-byte writes at descending
 * offsets from `top`, exactly matching the pops in switch.S's gt_switch. */
void *frame_init(u64 top, void *fn, void *arg, void *trampoline) {
    /* 7 words below the top: [r15,r14,r13,r12,rbx,rbp,retaddr]. */
    u64 *frame = (u64 *)(top - 7 * 8);
    frame[0] = 0;                    /* r15 = 0                                 */
    frame[1] = 0;                    /* r14 = 0                                 */
    frame[2] = (u64)arg;             /* r13 -> trampoline moves this into rdi   */
    frame[3] = (u64)fn;              /* r12 -> trampoline calls this            */
    frame[4] = 0;                    /* rbx = 0                                 */
    frame[5] = 0;                    /* rbp = 0                                 */
    frame[6] = (u64)trampoline;      /* return address the final `ret` jumps to */
    return frame;                    /* becomes task->rsp                       */
}

/* A tiny driver so the file has a definite entry point and the optimiser has
 * something concrete to inline into at -O2 (making the asm comparison richer).
 * Returns the id of the task pick_next would run first from a 3-task queue. */
int demo_main(void) {
    task a = { 0, ST_WAITING, 0, 1 };   /* not ready: should be skipped         */
    task b = { 0, ST_READY,   0, 2 };   /* first READY: should be chosen         */
    task c = { 0, ST_READY,   0, 3 };
    runq q = { 0, 0 };
    runq_push(&q, &a);
    runq_push(&q, &b);
    runq_push(&q, &c);
    task *n = pick_next(&q);
    return n ? n->id : -1;              /* expect 2                              */
}
