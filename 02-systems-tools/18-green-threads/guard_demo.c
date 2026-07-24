/* ===========================================================================
 * guard_demo.c — prove the guard page actually catches a stack overflow.
 * ===========================================================================
 *
 * gt.c places a PROT_NONE page just below every task's usable stack. This
 * program spawns ONE task whose entry function recurses without bound, blowing
 * through its stack. Instead of silently corrupting whatever mapping happens to
 * sit below (another task's stack, the heap, ...), rsp descends into the guard
 * page and the very first push/write there raises SIGSEGV. The process dies
 * with a segfault AT the guard boundary — a loud, immediate, debuggable failure.
 *
 * Expected behaviour (Linux / WSL):
 *     $ make guard && ./guard_demo
 *     spawning a task that will overflow its 16 KiB stack...
 *     Segmentation fault (core dumped)      <- the guard page did its job
 *
 * To SEE that the fault is at the guard, run it under a debugger or check dmesg:
 *     $ gdb ./guard_demo -ex run -ex 'bt 3' -ex quit
 *   The faulting address will be one page below the task's stack base.
 *
 * Without the guard page (comment out the mprotect in gt.c), the same overflow
 * would instead scribble over neighbouring memory and crash — if at all — much
 * later, somewhere unrelated. That is the bug the guard page converts into an
 * honest, on-the-spot SIGSEGV.
 * ===========================================================================
 */

#include "gt.h"
#include <stdio.h>

/* Unbounded recursion. `volatile` + the touch of a local array stop the
 * optimiser from turning this into a tail loop (which would never grow the
 * stack) and force a real store each frame so we actually reach the guard.
 *
 * The infinite recursion is DELIBERATE — it is how we drive rsp into the guard
 * page — so we silence the (correct!) -Winfinite-recursion warning here rather
 * than let it break the lab's warning-free build. Both gcc and clang understand
 * this GCC-style pragma. */
#pragma GCC diagnostic push
#pragma GCC diagnostic ignored "-Winfinite-recursion"
static unsigned long recurse(unsigned long depth) {
    volatile char frame_filler[256];        /* burn stack every frame           */
    frame_filler[0] = (char)depth;          /* touch it so it can't be elided   */
    if ((depth & 0x3ff) == 0)               /* occasional progress print         */
        printf("  depth = %lu (rsp ~ %p)\n", depth, (void *)&frame_filler);
    /* Recurse deeper. The return value is threaded through so the call is not a
     * tail call the compiler could flatten. */
    return frame_filler[0] + recurse(depth + 1);
}
#pragma GCC diagnostic pop

static void overflower(void *arg) {
    (void)arg;
    printf("  overflowing now — expect SIGSEGV when rsp hits the guard page\n");
    recurse(0);
    printf("  (unreachable: we should have faulted)\n");
}

int main(void) {
    gt_init();
    printf("spawning a task that will overflow its 16 KiB stack...\n");
    /* A deliberately small stack so we reach the guard quickly. */
    gt_spawn(overflower, NULL, 16 * 1024);
    gt_run();                                /* the SIGSEGV happens inside here   */
    printf("(unreachable: the overflow should have killed us)\n");
    return 0;
}
