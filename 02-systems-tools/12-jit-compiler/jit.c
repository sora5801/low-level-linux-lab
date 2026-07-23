/* ===========================================================================
 * jit.c — turn the op[] IR into native x86-64 and run it. The privileged half.
 * ===========================================================================
 *
 * This file is where bytes become an executable function. The dangerous, OS-
 * facing steps all live here (the encoder in emit.c is deliberately pure):
 *
 *   1. mmap an anonymous, WRITABLE-but-not-executable buffer for the code.
 *   2. Emit machine code into it with the emit_* encoders.
 *   3. Back-patch loop displacements now that all targets are known.
 *   4. mprotect the buffer to READ+EXEC, dropping WRITE  ->  W^X.
 *   5. Make the change visible to instruction fetch (i-cache coherency), then
 *      call into the buffer through a function pointer with the SysV ABI.
 *
 * W^X ("write xor execute") is the security invariant: a page is either
 * writable or executable, NEVER both at once. If a page were writable AND
 * executable, any memory-corruption bug could scribble shellcode into it and
 * jump there. By separating the phases with an mprotect, an attacker who can
 * write can't execute what they wrote, and vice versa. Modern OSes increasingly
 * *enforce* W^X (SELinux, hardened kernels, Apple's pmap), so a real JIT must
 * either do the two-phase dance we do here or use a dual-mapping trick.
 * ===========================================================================
 */
#include "bf.h"
#include "emit.h"

#include <sys/mman.h>   /* mmap, mprotect, munmap, PROT_*, MAP_*   */
#include <stdint.h>     /* uintptr_t                                */
#include <stdlib.h>     /* malloc/free for the backpatch stack      */
#include <string.h>     /* memset not needed but here for clarity   */
#include <stdio.h>      /* snprintf for error messages              */
#include <unistd.h>     /* sysconf(_SC_PAGESIZE)                     */
#include <errno.h>      /* errno after a failed syscall             */

/* Round `n` up to the next multiple of `align` (a power of two). mmap always
 * hands back whole pages, and mprotect operates on whole pages, so the code
 * region must be page-rounded. */
static size_t round_up(size_t n, size_t align)
{
    return (n + align - 1) & ~(align - 1);
}

/* ---------------------------------------------------------------------------
 * bf_jit_compile — IR -> executable machine code.
 *
 * Returns 0 on success, filling *out. On failure returns non-zero and writes a
 * reason into err[0..errlen). On success the caller owns the mapping and must
 * release it with bf_jit_free.
 * --------------------------------------------------------------------------- */
int bf_jit_compile(const bf_program *prog, jit_code *out,
                   char *err, size_t errlen)
{
    /* ---- 1. size the buffer with a proven upper bound --------------------
     * We must allocate before we emit, so we need a guaranteed-sufficient size.
     * The largest single op is a loop bracket at 9 bytes (cmpb=3 + jcc=6). Add
     * the fixed prologue (~27 bytes) and epilogue (~11 bytes). We budget a
     * generous 16 bytes per op plus 64 for the frame; this can never be
     * exceeded, so the encoder's overflow flag should stay clear. Sizing high
     * is cheap (we only touch the pages we write) and removes a whole class of
     * bugs. */
    size_t max_bytes = 64 + prog->n * 16;
    long pg = sysconf(_SC_PAGESIZE);          /* usually 4096; ask, don't assume */
    if (pg <= 0) pg = 4096;
    size_t map_len = round_up(max_bytes, (size_t)pg);

    /* ---- 2. mmap a writable (not yet executable) code buffer -------------
     * mmap(2): syscall 9. Args in order:
     *   rdi addr   = NULL  -> let the kernel choose the address
     *   rsi length = map_len (page multiple)
     *   rdx prot   = PROT_READ|PROT_WRITE   (writable so we can emit; NOT exec)
     *   r10 flags  = MAP_PRIVATE|MAP_ANONYMOUS (fresh zeroed pages, no file)
     *   r8  fd     = -1    (ignored for anonymous)
     *   r9  offset = 0
     * The kernel reserves page table entries and returns the base address, or
     * MAP_FAILED ((void*)-1) on error with errno set (ENOMEM if out of address
     * space / commit limit). We MUST check for MAP_FAILED — an unchecked mmap
     * that we then write to would fault. */
    void *mem = mmap(NULL, map_len, PROT_READ | PROT_WRITE,
                     MAP_PRIVATE | MAP_ANONYMOUS, -1, 0);
    if (mem == MAP_FAILED) {
        if (err) snprintf(err, errlen, "mmap failed: %s", strerror(errno));
        return -1;
    }

    /* ---- 3. emit code into the buffer -----------------------------------*/
    code_buf c = { .buf = (u8 *)mem, .len = 0, .cap = map_len, .overflow = 0 };

    /* Backpatch stack: for each open '[' we remember the buffer offset of its
     * je's rel32 field, so the matching ']' can compute and write the forward
     * displacement (and hand back its own offset for the backward one). Depth
     * is bounded by prog->n. */
    u64 *stk = malloc((prog->n + 1) * sizeof *stk);
    if (!stk) {
        if (err) snprintf(err, errlen, "out of memory (backpatch stack)");
        munmap(mem, map_len);
        return -1;
    }
    size_t sp = 0;

    emit_prologue(&c);

    for (size_t i = 0; i < prog->n; i++) {
        const op o = prog->ops[i];
        switch (o.kind) {

        case OP_ADD:
            /* cell += arg  (arg already folded to a byte). One `addb`. */
            emit_add_cell(&c, (u8)o.arg);
            break;

        case OP_MOVE:
            /* ptr += arg. One `addq $imm32,%rbx`; negative arg decrements. */
            emit_add_ptr(&c, (i32)o.arg);
            break;

        case OP_CLEAR:
            /* "[-]" peephole -> a single `movb $0,(%rbx)`. */
            emit_set_cell(&c, 0);
            break;

        case OP_OUT:
            /* Emit the output sequence once per folded repeat. Typically arg==1;
             * runs of '.' are rare but we honor the count. */
            for (int k = 0; k < o.arg; k++)
                emit_output(&c);
            break;

        case OP_IN:
            emit_input(&c);
            break;

        case OP_JZ: {
            /* '[': emit `cmpb $0,(%rbx); je <patch later>` and push the rel32
             * field offset. The forward target is unknown until we reach ']'. */
            u64 site = emit_loop_open(&c);
            stk[sp++] = site;
            break;
        }

        case OP_JNZ: {
            /* ']': emit `cmpb $0,(%rbx); jne <back to body>`. We know BOTH
             * targets now:
             *   - our jne must jump back to the loop body, which starts right
             *     after the matching '['s je (i.e. at the offset that WAS c.len
             *     when we pushed — but simpler: it is `open_site + 4`, the byte
             *     just past the '['s rel32 field);
             *   - the '['s je must jump forward to here, just past our jne's
             *     rel32 field (c.len after emit_loop_close). */
            if (sp == 0) {                     /* IR guarantees balance, but be safe */
                if (err) snprintf(err, errlen, "internal: unbalanced ']' in IR");
                free(stk); munmap(mem, map_len); return -1;
            }
            u64 open_site  = stk[--sp];        /* rel32 field of the '['s je   */
            u64 body_start = open_site + 4;    /* first byte of the loop body  */
            u64 close_site = emit_loop_close(&c);   /* rel32 field of our jne  */
            u64 after_close = close_site + 4;  /* loop-exit target (past jne)  */

            patch_rel32(&c, close_site, body_start);   /* jne  -> loop body    */
            patch_rel32(&c, open_site,  after_close);  /* je   -> loop exit    */
            break;
        }

        case OP_HALT:
            /* The IR's terminator maps to the function epilogue + ret. */
            emit_epilogue(&c);
            break;
        }
    }

    free(stk);

    /* The encoder refuses out-of-bounds writes and records it. If this fired,
     * our size bound was wrong (a bug) — fail loudly rather than execute a
     * truncated function. */
    if (c.overflow) {
        if (err) snprintf(err, errlen,
            "code buffer overflow (emitted past %zu bytes)", map_len);
        munmap(mem, map_len);
        return -1;
    }

    /* ---- 4. flip the page(s) to READ+EXEC: enforce W^X -------------------
     * mprotect(2): syscall 10. Args:
     *   rdi addr = mem (page-aligned base — required; mmap guarantees it)
     *   rsi len  = map_len
     *   rdx prot = PROT_READ|PROT_EXEC   (drop PROT_WRITE!)
     * After this returns 0 the region is executable and NO LONGER writable, so
     * any later attempt to modify the code faults. A hostile or hardened kernel
     * that forbids the W->X transition would return -1/EACCES here; we surface
     * that instead of blindly calling into a non-executable page. */
    if (mprotect(mem, map_len, PROT_READ | PROT_EXEC) != 0) {
        if (err) snprintf(err, errlen, "mprotect RX failed: %s", strerror(errno));
        munmap(mem, map_len);
        return -1;
    }

    /* ---- 5. instruction-cache coherency ---------------------------------
     * We just wrote instructions through the DATA side of the memory system,
     * then intend to FETCH them through the instruction side. On x86-64 the
     * hardware keeps the L1 instruction cache coherent with data stores
     * automatically (the SDM's self-modifying-code rules), and the mprotect
     * syscall's kernel round-trip is itself a serializing event, so no explicit
     * flush is required on this ISA. On ARM/AArch64, RISC-V, etc. the caches
     * are NOT coherent and you MUST flush D-cache to the point of unification,
     * invalidate I-cache, and execute an instruction-synchronization barrier.
     * `__builtin___clear_cache` expands to exactly those barriers on those
     * targets and to nothing on x86 — so calling it makes the JIT portable and
     * documents the requirement in code. */
    __builtin___clear_cache((char *)mem, (char *)mem + c.len);

    out->code     = (u8 *)mem;
    out->code_len = c.len;
    out->map_len  = map_len;
    return 0;
}

/* ---------------------------------------------------------------------------
 * bf_jit_run — call the generated function.
 *
 * The generated code obeys the System V AMD64 ABI (see emit_prologue). Its C
 * prototype is:
 *     void fn(unsigned char *tape,        // rdi
 *             long (*read_byte)(void),    // rsi
 *             void (*write_byte)(long));  // rdx
 *
 * CASTING DATA -> FUNCTION POINTER. ISO C leaves object<->function pointer
 * conversions undefined, but POSIX *requires* them to work (that is how
 * dlsym(3) returns code addresses), and every real JIT relies on it. We launder
 * the pointer through a union to convert without tripping -Wpedantic; the
 * invariant that makes it sound is simply that `code` points at valid,
 * executable machine code implementing that exact prototype — which steps 1-5
 * guarantee.
 * --------------------------------------------------------------------------- */
void bf_jit_run(const jit_code *jc, unsigned char *tape, const bf_io *io)
{
    union {
        unsigned char *data;
        void (*fn)(unsigned char *, long (*)(void), void (*)(long));
    } cast;
    cast.data = jc->code;
    /* Hand the tape and the two I/O callbacks straight to native code. From
     * here control leaves C entirely until the generated `ret` returns. */
    cast.fn(tape, io->read_byte, io->write_byte);
}

/* Release the executable mapping. munmap(2): syscall 11 (addr, len). We map and
 * unmap as a matched pair; after this the code pointer is dangling and must not
 * be called. */
void bf_jit_free(jit_code *jc)
{
    if (!jc || !jc->code) return;
    munmap(jc->code, jc->map_len);
    jc->code = NULL;
    jc->code_len = jc->map_len = 0;
}
