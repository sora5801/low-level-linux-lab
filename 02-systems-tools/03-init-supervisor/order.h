/* ===========================================================================
 * order.h — the PURE-LOGIC core of the supervisor, deliberately header-free.
 * ===========================================================================
 *
 * WHY THIS FILE HAS NO #includes
 * ------------------------------
 * Everything a supervisor does that is *interesting to a CPU* — as opposed to
 * interesting to the kernel — lives here: decide the order to start services in
 * (a topological sort of the dependency graph), and decide how long to wait
 * before restarting a service that keeps dying (exponential backoff). Neither
 * needs a single system call, so this translation unit includes NOTHING. That
 * is not an accident: because it pulls in no libc/system headers, clang can
 * compile it straight to Linux assembly with
 *
 *     clang --target=x86_64-pc-linux-gnu -S -O1 order.c
 *
 * on any host, so it is one of the project's committed teaching-asm files
 * (asm/order.s). Keeping the algorithmic heart free of headers is a habit worth
 * copying: it is unit-testable, portable, and its machine code is legible.
 *
 * We therefore declare our OWN integer type instead of pulling <stddef.h> for
 * size_t. On the LP64 data model Linux uses on x86-64, `unsigned long` is 64
 * bits and holds any array index or millisecond count without truncation.
 * ===========================================================================
 */
#ifndef SUP_ORDER_H
#define SUP_ORDER_H

/* Our stand-in for size_t / uint64. 64-bit under LP64 — see note above. */
typedef unsigned long sup_u64;

/* Upper bound the caller promises never to exceed. It bounds the on-stack
 * scratch arrays inside the topological sort so we never touch the heap. */
#define SUP_MAX_NODES 64

/* ---------------------------------------------------------------------------
 * sup_toposort — order services so every prerequisite starts before its dependent.
 *
 * INPUT (a dependency graph as an adjacency MATRIX):
 *   n     : number of services, 0 <= n <= SUP_MAX_NODES.
 *   edges : pointer to an n*n row-major matrix of bytes. edges[i*n + j] != 0
 *           means "service i depends on service j", i.e. j is a prerequisite of
 *           i and MUST be started first. (A directed edge j -> i in start order.)
 * OUTPUT:
 *   order : caller-provided array of at least n ints. On success it is filled
 *           with a permutation of 0..n-1 giving a valid start order.
 * RETURNS:
 *   the number of nodes placed into `order` (== n on success). If it returns a
 *   value < n, the graph contained a CYCLE (e.g. A after B, B after A) and the
 *   returned count is how many services could be ordered before the deadlock —
 *   the caller treats "< n" as a fatal config error.
 *
 * Algorithm: Kahn's algorithm. Compute each node's in-degree (its number of
 * unmet prerequisites), repeatedly emit any node whose in-degree has fallen to
 * zero, and relax the in-degree of everything that depended on it. It is O(n^2)
 * here because we use a dense matrix; for 64 services that is at most 4096 byte
 * reads, which is nothing. The asm for this routine is what asm/demo.annotated.s
 * walks through instruction by instruction.
 * --------------------------------------------------------------------------- */
int sup_toposort(int n, const unsigned char *edges, int *order);

/* ---------------------------------------------------------------------------
 * sup_backoff_delay_ms — how long to wait before the Nth restart of a service.
 *
 * A service that crash-loops must not be restarted in a tight fork() storm: that
 * pins a CPU and floods the logs. The standard fix is EXPONENTIAL BACKOFF —
 * double the wait each time, up to a ceiling:
 *
 *     delay = min(base_ms << restart_count, cap_ms)
 *
 * restart_count is 0 for the first restart, 1 for the second, and so on. The
 * left shift is a doubling; we clamp the shift amount so it can never reach 64
 * (which would be undefined behaviour on a 64-bit shift) and clamp the result to
 * cap_ms so a long-lived crash loop settles at a fixed, sane retry interval.
 * Pure integer math, no allocation, no syscalls — the second routine annotated
 * in asm/demo.annotated.s.
 * --------------------------------------------------------------------------- */
sup_u64 sup_backoff_delay_ms(unsigned restart_count, sup_u64 base_ms, sup_u64 cap_ms);

#endif /* SUP_ORDER_H */
