/* ===========================================================================
 * symbolize.h — turn a runtime instruction pointer into a function NAME.
 * ===========================================================================
 * A sample is just a number: the value of %rip when the timer fired. To make a
 * flame graph readable we need "which function was that address in?". We resolve
 * it with dladdr(3), which maps an address in OUR OWN address space to the
 * nearest exported symbol. See symbolize.c for what that can and cannot do.
 * =========================================================================== */
#ifndef SYMBOLIZE_H
#define SYMBOLIZE_H

#include <stddef.h>     /* size_t   */
#include <stdint.h>     /* uint64_t */

/* Write a human name for the code at `ip` into `buf` (always NUL-terminated,
 * never overruns `buflen`). Falls back to "module+0xoffset" and finally a raw
 * hex address when no symbol is available. We deliberately return the function
 * name WITHOUT the +offset within the function, so that every sample that landed
 * anywhere inside foo() folds to the single key "foo" — which is exactly the
 * grouping a flame graph wants. */
void sym_name(uint64_t ip, char *buf, size_t buflen);

#endif /* SYMBOLIZE_H */
