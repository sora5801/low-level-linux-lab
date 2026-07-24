/* ===========================================================================
 * sample.c — the tiny program we debug with ptrace-dbg.
 * ===========================================================================
 *
 * It is deliberately shaped to exercise every debugger feature:
 *
 *   - `factorial` is RECURSIVE, so a breakpoint inside it and a `bt` show a
 *     genuine multi-frame stack (factorial <- factorial <- ... <- main).
 *   - `sum_to` has a LOOP and locals, good for single-stepping and watching a
 *     register (the accumulator) change with `regs`.
 *   - `main` calls both and prints, so `continue` reaches a real exit.
 *
 * BUILD NOTES (see the Makefile):
 *   -g -gdwarf-4                emit a DWARF v4 line table our parser understands
 *   -fno-omit-frame-pointer     keep %rbp chained so `bt` can walk it
 *   -O0                         one source line per basic block => clean stepping
 * Compiled as a normal PIE by default, which is exactly why the debugger reads
 * /proc/<pid>/maps to find the load base.
 * ===========================================================================
 */
#include <stdio.h>

/* Recursive factorial: each call is its own stack frame. Break here and `bt`. */
long factorial(int n)
{
    if (n <= 1)
        return 1;
    long sub = factorial(n - 1);   /* the recursion: a nested frame per call     */
    return (long)n * sub;
}

/* Iterative sum 1..n: a loop with an accumulator local, ideal for `step`+`regs`. */
long sum_to(int n)
{
    long acc = 0;
    for (int i = 1; i <= n; i++)
        acc += i;                  /* watch `acc` grow one instruction at a time  */
    return acc;
}

int main(void)
{
    int n = 5;
    long f = factorial(n);         /* good `break factorial` target               */
    long s = sum_to(n);            /* good `break sum_to` target                  */

    printf("factorial(%d) = %ld\n", n, f);
    printf("sum_to(%d)    = %ld\n", n, s);
    return 0;
}
