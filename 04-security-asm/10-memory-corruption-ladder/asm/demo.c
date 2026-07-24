/* ===========================================================================
 * demo.c — the WHOLE bug, in one self-contained function.
 * ===========================================================================
 *
 * This file exists so you can read, in assembly, *exactly why* a stack buffer
 * overflow lets an attacker take control of the CPU. It is the pure-logic core
 * extracted from targets/vuln_stack.c: an unbounded copy into a fixed-size
 * stack buffer. No system headers, no libc — so it compiles to clean, minimal
 * assembly and the stack frame is the only thing on screen.
 *
 * Compile ONLY to assembly (we never link this; it has no real main):
 *
 *   clang --target=x86_64-pc-linux-gnu -S -O0 -fno-asynchronous-unwind-tables \
 *         -fno-jump-tables -fno-omit-frame-pointer demo.c -o demo.O0.s
 *   clang --target=x86_64-pc-linux-gnu -S -O1 -fno-asynchronous-unwind-tables \
 *         -fno-jump-tables -fno-omit-frame-pointer demo.c -o demo.s
 *   clang --target=x86_64-pc-linux-gnu -S -O2 -fno-asynchronous-unwind-tables \
 *         -fno-jump-tables demo.c -o demo.O2.s
 *
 * Then read asm/demo.annotated.s, where every instruction is commented and the
 * saved return address is pointed at explicitly.
 *
 * ---------------------------------------------------------------------------
 * THE STACK FRAME OF vulnerable(), and why the overflow wins
 * ---------------------------------------------------------------------------
 * On x86-64 the stack grows DOWNWARD (toward lower addresses), but a copy like
 * strcpy writes UPWARD (toward higher addresses), from &buf[0] onward. Those
 * two directions point at each other, and that collision is the whole exploit.
 *
 * Here is the frame clang ACTUALLY builds (read it off asm/demo.s; buf lands at
 * -80(%rbp), NOT -72 — see the note on slack below):
 *
 *      higher addresses
 *      ┌───────────────────────────┐
 *      │  ...caller's frame...      │
 *      ├───────────────────────────┤  <- rbp + 8   (offset +8 from rbp)
 *      │  SAVED RETURN ADDRESS      │  the `ret` at the end of vulnerable() pops
 *      │  (8 bytes)                 │  THIS into rip. Overwrite it -> you choose
 *      ├───────────────────────────┤  <- rbp        where the CPU executes next.
 *      │  SAVED rbp (8 bytes)       │  caller's frame pointer, restored by `pop`.
 *      ├───────────────────────────┤  <- rbp - 8
 *      │  8 bytes of slack /        │  spilled `attacker` pointer (-O0) or the
 *      │  saved rbx + padding       │  saved rbx (-O1); alignment fill lives here
 *      ├───────────────────────────┤  <- rbp - 16
 *      │  buf[63] ...               │
 *      │  ...                       │  the 64-byte buffer. The copy starts at
 *      │  buf[0]  <-- copy starts   │  buf[0] and marches UP toward the ret addr.
 *      └───────────────────────────┘  <- rbp - 80   (rsp is here or below)
 *      lower addresses
 *
 * OFFSET FROM buf[0] TO THE SAVED RETURN ADDRESS:
 *
 *   The NAIVE guess is 64 (buffer) + 8 (saved rbp) = 72. That guess is WRONG
 *   here, and learning why is the point. The compiler must keep rsp 16-byte
 *   aligned at every `call`, so it rounds the local area up: 64 bytes of buf
 *   plus an 8-byte spill = 72, rounded up to the next multiple of 16 = 80. It
 *   then parks buf at the very bottom, at -80(%rbp). So the real distance is:
 *
 *        80  (from buf[0] up to the saved rbp — the whole reserved area)
 *      +  8  (overwrite the saved rbp itself)
 *      = 88  bytes of padding, THEN 8 bytes that land on the return address.
 *
 *   my_strcpy() never checks that count. Feed it 88 bytes of filler and the
 *   next 8 bytes overwrite the return address. That number, 88 for THIS build,
 *   is the "offset". Because it depends on compiler, version, and flags, real
 *   exploit work never guesses it — it MEASURES it with a De Bruijn / "cyclic"
 *   pattern (see ../exploits/common.py `cyclic`). The annotated assembly shows
 *   you exactly where the 80 and the +8 come from in clang's real output.
 * ===========================================================================
 */

/* We are freestanding: define our own fixed-width-ish types instead of pulling
 * in <stddef.h>/<stdint.h>. On the LP64 model Linux uses, unsigned long is the
 * 64-bit pointer-sized integer. */
typedef unsigned long usize;

/* ---------------------------------------------------------------------------
 * my_strcpy — a faithful reimplementation of libc strcpy(3), bug and all.
 *
 * CONTRACT (identical to the real strcpy): copy bytes from `src` to `dst`,
 * including the terminating NUL, and return `dst`. The DANGER, also identical
 * to the real strcpy, is that the length is defined entirely by the DATA (where
 * the first NUL sits in `src`), never by the SIZE of `dst`. The function has no
 * idea how big `dst` is, so it cannot stop at the buffer's end. If `src` is
 * longer than `dst`, it writes past the end — a classic out-of-bounds write.
 *
 * We mark it noinline so it stays a real `call` in the assembly: that keeps the
 * buffer's address escaping into a function argument, which forces the compiler
 * to place `buf` on the stack (address-taken locals cannot live only in
 * registers). That is what makes the frame layout visible in demo.s at -O1.
 * --------------------------------------------------------------------------- */
__attribute__((noinline))
char *my_strcpy(char *dst, const char *src)
{
    char *d = dst;              /* walking write pointer, starts at buf[0]      */
    /* Copy byte-by-byte. The loop condition IS the assignment: it stores the
     * byte, then tests whether that byte was the NUL terminator. When a 0 byte
     * is copied, (0) is false and the loop stops — AFTER writing the 0. There
     * is no length parameter here, and that is precisely the vulnerability. */
    while ((*d++ = *src++) != 0) {
        /* no bounds check — the write pointer d can march right off the end of
         * dst and over saved rbp, then over the saved return address. */
    }
    return dst;                /* strcpy returns the destination pointer       */
}

/* An external "consumer" we never define here. Declaring it (but calling it)
 * forces the compiler to keep `buf` materialized on the stack and prevents the
 * optimizer from proving the buffer is dead and deleting the whole copy. Think
 * of it as "something later reads the string" (a log line, a comparison, ...).
 * Because we only compile to assembly (-S), the unresolved symbol is fine. */
extern void consume(const char *s);

/* ---------------------------------------------------------------------------
 * vulnerable — the function whose stack frame gets smashed.
 *
 * `attacker` is fully attacker-controlled input (in the real target it comes
 * from read(2)/argv). We copy it into a 64-byte buffer with no length limit.
 * The moment strlen(attacker) >= 64 the copy overruns `buf`; once it passes the
 * 88-byte offset computed above it overwrites THIS function's saved return
 * address, and whatever 8 bytes land there become the next value of rip when
 * `vulnerable` executes its `ret`.
 * --------------------------------------------------------------------------- */
int vulnerable(const char *attacker)
{
    char buf[64];              /* the fixed-size target buffer, on the stack    */

    my_strcpy(buf, attacker);  /* THE BUG: unbounded copy into a 64-byte space  */

    consume(buf);              /* pretend to use the result (keeps buf alive)   */
    return 0;
}
