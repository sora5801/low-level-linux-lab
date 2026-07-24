/* ===========================================================================
 * forkserver.h — the wire protocol shared by the fuzzer and every target.
 * ===========================================================================
 *
 * A coverage-guided fuzzer is two cooperating processes talking over three
 * kernel objects. This header is the *contract* between them, so both sides
 * agree on the exact same numbers. Get one of these wrong and the two halves
 * deadlock silently — which is why AFL, honggfuzz, and libFuzzer all pin these
 * constants in a single shared header just like this.
 *
 * THE THREE SHARED KERNEL OBJECTS
 * -------------------------------
 *   1. A SysV shared-memory segment (the "coverage bitmap" / "trace_bits").
 *      The instrumented target WRITES edge hits into it; the fuzzer READS it
 *      after each run to decide "did this input reach anything NEW?". Because
 *      it is real shared memory (one physical page range mapped into both
 *      address spaces), the fuzzer sees the target's writes with zero copies
 *      and zero syscalls per edge — the whole reason AFL is fast.
 *
 *   2/3. Two pipes forming the "fork-server" control channel:
 *        - CTL pipe: fuzzer -> target. "Please run one test case now."
 *        - ST  pipe: target -> fuzzer. "Here is the child pid / exit status."
 *
 * WHY A FORK SERVER AT ALL?
 * -------------------------
 * The naive fuzzer does execve() per test case. execve is expensive: it tears
 * down the address space, re-maps the ELF, re-runs the dynamic loader, re-runs
 * every C++ static constructor, rebuilds libc's state... often milliseconds.
 * A fuzzer wants *hundreds of thousands* of executions per second, so paying
 * that every time is fatal.
 *
 * The fork server trick: exec the target ONCE. Let it run all its expensive
 * one-time init (load tables, map the shm, reach main). Then park it in a loop.
 * For each test case the parked process just fork()s — copy-on-write, so the
 * child starts already-initialised in microseconds — and the child runs the
 * one input and dies. We pay the init cost once, not N times. This is the
 * single most important performance idea in modern fuzzing.
 *
 * DEFENSE / BLUE-TEAM NOTE
 * ------------------------
 * The same instrumentation that guides a fuzzer is exactly what a *sanitizer*
 * build ships to production-adjacent test rigs to catch memory-safety bugs
 * BEFORE an attacker fuzzes them for you. Coverage-guided fuzzing in CI is a
 * defensive practice: you are the one who finds the crash, on your own box,
 * with a stack trace, instead of reading about it in a CVE. See README.
 * ===========================================================================
 */
#ifndef COVFUZZ_FORKSERVER_H
#define COVFUZZ_FORKSERVER_H

/* ---------------------------------------------------------------------------
 * The coverage bitmap geometry.
 *
 * AFL's famous choice: a 64 KiB map (2^16 bytes). Each byte is one "edge"
 * bucket. 64 KiB is a sweet spot: big enough that hash collisions between
 * distinct edges are rare for real programs, small enough to memset and scan
 * in a few microseconds (it fits comfortably in L2 cache). The power-of-two
 * size also lets us replace a modulo with a single AND mask (idx & MAP_SIZE-1),
 * which matters because that masking happens on EVERY edge the target executes.
 * --------------------------------------------------------------------------- */
#define MAP_SIZE_POW2 16
#define MAP_SIZE      (1U << MAP_SIZE_POW2)   /* 65536 bytes = 64 KiB          */

/* ---------------------------------------------------------------------------
 * Fixed file descriptors for the fork-server control channel.
 *
 * The fuzzer creates two pipes, then dup2()s their proper ends onto these two
 * hard-coded fd numbers in the child *before* execve. The target's runtime
 * then blindly read()s fd 198 and write()s fd 199 — it never has to be told
 * which fds to use, because we agreed here. 198/199 are AFL's exact numbers;
 * they are chosen high enough to almost never collide with fds the target
 * opens on its own (stdin/out/err are 0/1/2, most programs stay under ~20).
 * --------------------------------------------------------------------------- */
#define FORKSRV_CTL_FD 198   /* fuzzer WRITES the "go" command here           */
#define FORKSRV_ST_FD  199   /* target WRITES pid + status here               */

/* ---------------------------------------------------------------------------
 * How the target learns which shared-memory segment to attach.
 *
 * The fuzzer shmget()s the segment, gets back an integer id, formats it as a
 * decimal string, and putenv()s it under this name before exec. The target's
 * runtime getenv()s it, atoi()s it, and shmat()s that id. Passing an IPC id
 * through the environment is exactly how classic AFL wires the two processes
 * together — simple, no filesystem, survives fork.
 * --------------------------------------------------------------------------- */
#define SHM_ENV_VAR "__COVFUZZ_SHM_ID"

/* The 4-byte handshake the fork server sends up the ST pipe the instant it is
 * alive and parked in its loop. The fuzzer blocks on reading exactly these 4
 * bytes; receiving them proves the target got far enough to map the shm and is
 * ready to serve fork requests. The value itself is arbitrary — only its
 * length (4) is load-bearing in the classic protocol. */
#define FORKSRV_HELLO 0xA1B2C3D4u

/* ---------------------------------------------------------------------------
 * The one function the instrumented target calls to become a fork server.
 *
 * Implemented in rt.c (the instrumentation runtime, compiled WITHOUT coverage
 * so its own edges never pollute the map). The target's main() calls this once.
 * In the PARENT (the server) it loops forever and never returns. In each forked
 * CHILD it returns, so execution falls through into the real target logic that
 * reads the input and runs the parser. See rt.c for the full protocol.
 * --------------------------------------------------------------------------- */
void __afl_start_forkserver(void);

#endif /* COVFUZZ_FORKSERVER_H */
