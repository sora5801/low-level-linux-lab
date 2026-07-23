/* ===========================================================================
 * bf.h — the intermediate representation shared by the parser, interpreter,
 *        and JIT, plus the small I/O interface the generated code calls back to.
 * ===========================================================================
 *
 * Brainfuck has eight instructions: > < + - . , [ ]. Rather than interpret the
 * raw source text (which is slow and awkward to JIT), we parse it once into a
 * flat array of `op` structs — a classic "bytecode" IR. Both back ends (the
 * tree-walking interpreter and the native-code JIT) consume this same array, so
 * the performance comparison is apples-to-apples: same front end, same peephole
 * optimizations, only the execution strategy differs.
 * ===========================================================================
 */
#ifndef BF_H
#define BF_H

#include <stddef.h>   /* size_t */

/* ---------------------------------------------------------------------------
 * The IR opcodes. Note these are already *optimized* forms, not a 1:1 mapping
 * of source characters — the parser folds runs and recognizes idioms:
 *
 *   OP_ADD   fold of consecutive '+'/'-' : cell += arg   (arg is a byte delta)
 *   OP_MOVE  fold of consecutive '>'/'<' : ptr  += arg   (arg is a signed delta)
 *   OP_CLEAR peephole for "[-]" / "[+]"  : cell  = 0      (whole loop -> 1 store)
 *   OP_OUT   '.'  : output cell (arg = repeat count, folded '.' runs)
 *   OP_IN    ','  : input to cell
 *   OP_JZ    '['  : if cell==0 jump to arg (index just past matching ']')
 *   OP_JNZ   ']'  : if cell!=0 jump to arg (index just past matching '[')
 *   OP_HALT  end of program
 *
 * The JZ/JNZ `arg` is only used by the interpreter (it is an *IR index*). The
 * JIT ignores it and instead back-patches native rel32 displacements, because
 * machine-code offsets bear no relation to IR indices.
 * --------------------------------------------------------------------------- */
typedef enum {
    OP_ADD,
    OP_MOVE,
    OP_CLEAR,
    OP_OUT,
    OP_IN,
    OP_JZ,
    OP_JNZ,
    OP_HALT
} op_kind;

typedef struct {
    op_kind kind;
    int     arg;   /* delta, repeat count, or jump target — meaning per kind    */
} op;

/* A parsed program: a heap array of ops terminated by an OP_HALT. `n` counts
 * the ops INCLUDING the trailing OP_HALT. Ownership: bf_parse mallocs it, the
 * caller frees with bf_free_program. */
typedef struct {
    op    *ops;
    size_t n;
} bf_program;

/* ---------------------------------------------------------------------------
 * I/O callbacks. The interpreter calls these directly; the JIT's generated
 * machine code calls the very same function pointers (they are passed to the
 * generated function as SysV arguments and invoked with `call *reg`). Keeping
 * one interface for both back ends is what makes their outputs identical.
 *
 *   read_byte : return the next input byte (0..255), or 0 at end-of-input.
 *               (This JIT uses the "EOF -> 0" convention; documented in README.)
 *   write_byte: emit the low 8 bits of `v` to output.
 * --------------------------------------------------------------------------- */
typedef struct {
    long (*read_byte)(void);
    void (*write_byte)(long v);
} bf_io;

/* Parser (parse.c). Returns 0 on success. On failure returns non-zero and, if
 * `err`/`errlen` are provided, writes a human-readable reason (e.g. unmatched
 * bracket) there. */
int  bf_parse(const char *src, size_t src_len, bf_program *out,
              char *err, size_t errlen);
void bf_free_program(bf_program *p);

/* Interpreter (interp.c): the baseline we benchmark the JIT against. */
void bf_interp(const bf_program *prog, unsigned char *tape, const bf_io *io);

/* JIT (jit.c). See jit.c for the full contract; declared here so main.c can
 * drive it. Returns 0 on success. */
typedef struct {
    unsigned char *code;      /* executable machine code (mmap'd, then RX)      */
    size_t         code_len;  /* bytes of code actually emitted                 */
    size_t         map_len;   /* full page-rounded mapping length (for munmap)  */
} jit_code;

int  bf_jit_compile(const bf_program *prog, jit_code *out,
                    char *err, size_t errlen);
void bf_jit_run(const jit_code *jc, unsigned char *tape, const bf_io *io);
void bf_jit_free(jit_code *jc);

#endif /* BF_H */
