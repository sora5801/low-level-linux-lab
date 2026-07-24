/* ===========================================================================
 * debugger.h — the shared data model for a ptrace-based debugger.
 * ===========================================================================
 *
 * This header is the "vocabulary" of the whole project. It declares the four
 * kinds of state a debugger juggles and nothing else, so that every .c file
 * agrees on the same structs:
 *
 *   1. `breakpoint`  — one software breakpoint: the address we patched, the
 *                      original byte we clobbered with 0xCC (int3), and whether
 *                      the trap is currently armed.
 *   2. `line_table`  — the address -> (file, line) map we parse out of the
 *                      inferior's DWARF `.debug_line` section.
 *   3. `func_sym`    — one function from the ELF `.symtab` (name, address, size),
 *                      used to symbolise a raw PC in a backtrace.
 *   4. `debugger`    — the top-level session: the traced pid, the load base that
 *                      converts link-time DWARF addresses to runtime addresses,
 *                      and the breakpoint table.
 *
 * WHY A DEBUGGER NEEDS ALL FOUR
 * -----------------------------
 * ptrace(2) only speaks in raw virtual addresses and register bytes. To turn
 * "0x00005555_5555_5189" into "sample.c:12, inside factorial()" you need the
 * ELF symbol table (address -> function) and the DWARF line program (address ->
 * source line). Everything in this file exists to bridge that gap.
 *
 * Platform: Linux / x86-64 only. ptrace, /proc/<pid>/maps, /proc/<pid>/mem and
 * the ELF/DWARF layout here are all Linux specifics.
 * ===========================================================================
 */
#ifndef DEBUGGER_H
#define DEBUGGER_H

#include <stdint.h>     /* uint64_t, uint8_t — exact-width for on-the-wire bytes */
#include <stddef.h>     /* size_t                                                */
#include <sys/types.h>  /* pid_t, ssize_t                                        */

/* We take `struct user_regs_struct` (from <sys/user.h>) by pointer in several
 * prototypes. Forward-declaring it here means callers that only pass the pointer
 * around don't have to drag in <sys/user.h>; the .c files that actually read the
 * register fields include it themselves. */
struct user_regs_struct;

/* ---------------------------------------------------------------------------
 * 1. Breakpoints
 *
 * A *software* breakpoint works by overwriting the first byte of a target
 * instruction with 0xCC — the one-byte `int3` opcode. When the CPU executes it,
 * it raises #BP, the kernel turns that into a SIGTRAP, and ptrace stops the
 * child and wakes us. To make the breakpoint transparent we must remember the
 * byte we destroyed so we can put it back before the instruction actually runs.
 * ------------------------------------------------------------------------- */
#define MAX_BREAKPOINTS 64      /* fixed table: no allocator needed, easy to audit */

typedef struct {
    uint64_t addr;        /* RUNTIME virtual address of the patched byte          */
    uint8_t  saved_byte;  /* the original opcode byte we replaced with 0xCC       */
    int      enabled;     /* 1 = 0xCC is installed in the child right now          */
    int      in_use;      /* 1 = this table slot holds a live breakpoint          */
    int      id;          /* small user-facing number, printed by `break`/`bt`    */
} breakpoint;

/* ---------------------------------------------------------------------------
 * 2. The DWARF line table
 *
 * `.debug_line` does not store a plain array of rows; it stores a *bytecode
 * program* that, when interpreted, emits rows. debuginfo.c runs that program
 * (see the state machine there) and materialises the rows into this table,
 * sorted by address, so we can binary-search a PC to a source line.
 *
 * Each row says: "starting at link-time address `addr`, the code corresponds to
 * `file`:`line`". A row flagged `end` (DWARF's DW_LNE_end_sequence) marks the
 * address just past the end of a contiguous range — it caps the previous row
 * and carries no source line of its own.
 * ------------------------------------------------------------------------- */
typedef struct {
    uint64_t addr;   /* link-time address of the first machine op of this row     */
    uint32_t file;   /* 1-based index into `line_table.files`                     */
    uint32_t line;   /* source line number (1-based; 0 means "no line")           */
    int      end;    /* 1 = end_sequence sentinel (address is one-past-the-range) */
} line_row;

typedef struct {
    line_row *rows;    /* sorted-by-addr array; owns the storage (free in di_free) */
    size_t    n;       /* number of rows                                          */
    char    **files;   /* files[i] = source file name for DWARF file index i       */
    size_t    nfiles;  /* length of `files` (index 0 is a placeholder for v<5)      */
} line_table;

/* ---------------------------------------------------------------------------
 * 3. ELF function symbols
 *
 * From `.symtab` we keep only STT_FUNC entries. `value` is the link-time
 * address of the function's first byte; `size` is its length in bytes, so
 * [value, value+size) is the function's extent — that is how we answer "which
 * function contains this PC?" for a backtrace.
 * ------------------------------------------------------------------------- */
typedef struct {
    char    *name;   /* points into the mmap'd .strtab; NOT separately freed       */
    uint64_t value;  /* link-time start address                                    */
    uint64_t size;   /* byte length (0 for some asm symbols — handled defensively)  */
} func_sym;

/* ---------------------------------------------------------------------------
 * 4. Parsed debug info for one ELF file
 *
 * We mmap the whole executable read-only and point everything else *into* that
 * mapping (symbol names, the raw line program) so there is exactly one owner of
 * the bytes: `map`. di_free() unmaps it and frees the two arrays we built.
 * ------------------------------------------------------------------------- */
typedef struct {
    void      *map;      /* MAP_PRIVATE, PROT_READ mapping of the whole ELF file    */
    size_t     map_len;  /* its length, for munmap                                  */
    int        is_pie;   /* 1 if e_type == ET_DYN (PIE): addresses need a load base */
    line_table lt;       /* the address -> source-line map                          */
    func_sym  *funcs;    /* address -> function-name table (owned)                  */
    size_t     nfuncs;
    char      *path;     /* strdup of the exe path (owned)                          */
    int        ok;       /* 1 if we found *some* usable debug info                   */
} debuginfo;

/* ---------------------------------------------------------------------------
 * 5. The whole debugging session
 * ------------------------------------------------------------------------- */
typedef struct {
    pid_t      pid;          /* the traced child (0 if none)                        */
    char      *prog;         /* path to the inferior binary (owned)                 */
    uint64_t   load_base;    /* runtime base: runtime_addr = link_addr + load_base  */
    int        running;      /* 1 while the inferior is alive and stopped by us     */
    breakpoint bps[MAX_BREAKPOINTS];
    int        next_bp_id;   /* monotonically increasing id for the next breakpoint */
    int        pending_sig;  /* a non-trap signal to inject on the next resume (0=none) */
    debuginfo  di;           /* symbols + line table for `prog`                     */
} debugger;

/* ===========================================================================
 * inferior.c — low-level process control (ptrace/waitpid/proc plumbing)
 * ======================================================================== */

/* Spawn `argv[0]` under trace. Returns the child pid stopped at its first
 * instruction (the post-execve SIGTRAP already consumed), or -1 on failure. */
pid_t inferior_spawn(char *const argv[]);

/* waitpid() wrapper that retries on EINTR. On a normal stop, returns 1 and sets
 * *stopsig to the delivering signal (e.g. SIGTRAP). On child exit, returns 0 and
 * sets *exit_code. Returns -1 on a real error. */
int inferior_wait(pid_t pid, int *stopsig, int *exit_code);

int  inferior_getregs(pid_t pid, struct user_regs_struct *regs);
int  inferior_setregs(pid_t pid, const struct user_regs_struct *regs);

/* Word-granular text access (PTRACE_PEEKTEXT/POKETEXT). *ok reports success,
 * because -1 is a legal word value and must be disambiguated via errno. */
long inferior_peek(pid_t pid, uint64_t addr, int *ok);
int  inferior_poke(pid_t pid, uint64_t addr, uint64_t word);

/* Byte-range access via /proc/<pid>/mem (pread/pwrite). Returns bytes moved. */
ssize_t inferior_read_mem(pid_t pid, uint64_t addr, void *buf, size_t len);
ssize_t inferior_write_mem(pid_t pid, uint64_t addr, const void *buf, size_t len);

int inferior_cont(pid_t pid, int sig);   /* PTRACE_CONT with signal `sig` (0=none) */
int inferior_step(pid_t pid, int sig);   /* PTRACE_SINGLESTEP one instruction       */

/* Parse /proc/<pid>/maps to find the runtime base address of the mapping backed
 * by `exe_path`. Returns 0 and sets *base on success, -1 otherwise. */
int proc_load_base(pid_t pid, const char *exe_path, uint64_t *base);

/* ===========================================================================
 * breakpoint.c — software (int3) breakpoint management
 * ======================================================================== */

breakpoint *bp_add(debugger *dbg, uint64_t addr);   /* allocate + arm at addr      */
breakpoint *bp_find(debugger *dbg, uint64_t addr);  /* lookup by address, or NULL   */
int         bp_remove(debugger *dbg, int id);       /* disable + free slot          */
void        bp_enable(debugger *dbg, breakpoint *bp);   /* install 0xCC             */
void        bp_disable(debugger *dbg, breakpoint *bp);  /* restore original byte    */

/* Correctly resume across a breakpoint we are parked on: temporarily remove the
 * 0xCC, single-step the real instruction, then re-arm. Without this you would
 * either loop on the same int3 forever or execute a corrupted opcode. */
int step_over_breakpoint(debugger *dbg);

/* ===========================================================================
 * debuginfo.c — ELF symbol table + DWARF .debug_line parsing
 * ======================================================================== */

int  di_load(debuginfo *di, const char *path);
void di_free(debuginfo *di);

/* Map a link-time address to source file + line. Returns 1 on hit. */
int  di_addr_to_line(debuginfo *di, uint64_t link_addr,
                     const char **file, uint32_t *line);

/* Map a link-time address to the containing function (and offset). NULL = none. */
const char *di_addr_to_func(debuginfo *di, uint64_t link_addr, uint64_t *off);

/* Resolve a "file:line" location to the first link-time address of that line. */
int  di_line_to_addr(debuginfo *di, const char *file, uint32_t line,
                     uint64_t *link_addr);

/* Resolve a function name to its link-time entry address. */
int  di_func_to_addr(debuginfo *di, const char *name, uint64_t *link_addr);

#endif /* DEBUGGER_H */
