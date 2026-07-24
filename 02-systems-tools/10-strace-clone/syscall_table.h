/* ===========================================================================
 * syscall_table.h — the number->name->arg-shape map that a strace clone needs.
 * ===========================================================================
 *
 * A raw ptrace trace gives you a *number* (in orig_rax) and six machine words
 * (in the argument registers). To print anything human-readable we must answer
 * two questions for each number:
 *
 *   1. What is this syscall CALLED?           ->  the name table  (names[])
 *   2. What SHAPE are its arguments?          ->  the detail table (detail[])
 *      i.e. is arg0 an fd? a pointer? a NUL-terminated string to fetch from the
 *      tracee? a bitmask of open(2) flags? That shape is what lets us render
 *      openat(AT_FDCWD, "/etc/passwd", O_RDONLY) instead of a row of hex.
 *
 * IMPORTANT (why this header pulls in NOTHING): this file and syscall_table.c
 * are written to be *self-contained* — no <...> system headers — so the whole
 * translation unit compiles to didactic assembly with a cross-targeting clang
 * (see asm/). That is only possible if every type here is one we declare
 * ourselves. Hence the plain enum + a POD struct, no libc, no kernel headers.
 *
 * The numbers are the Linux x86-64 syscall ABI (arch/x86/entry/syscalls/
 * syscall_64.tbl). They are stable: 0 has meant read(2) for the life of the
 * architecture. A DIFFERENT architecture (arm64, x86-32) has DIFFERENT numbers,
 * which is exactly why real strace ships one table per ABI.
 * ===========================================================================
 */
#ifndef SYSCALL_TABLE_H
#define SYSCALL_TABLE_H

/* ---------------------------------------------------------------------------
 * arg_type — how to render one argument.
 *
 * The tracer reads six raw 64-bit words out of the tracee's registers; this tag
 * tells format_arg() how to turn a given word into text. Keeping it a small
 * enum (fits in an unsigned char) lets each table row stay tiny — six bytes of
 * arg-type plus a name pointer and a count.
 * --------------------------------------------------------------------------- */
enum arg_type {
    A_NONE = 0,   /* slot unused: the syscall has fewer than six real args     */
    A_INT,        /* signed 32-bit int  -> print decimal                       */
    A_LONG,       /* signed 64-bit long -> print decimal (e.g. lseek offset)   */
    A_HEX,        /* opaque word        -> print 0x...                         */
    A_PTR,        /* pointer            -> print 0x... , or NULL for 0         */
    A_STR,        /* char* IN THE TRACEE -> follow it, read the bytes, quote   */
    A_FD,         /* file descriptor    -> decimal, but -100 => AT_FDCWD       */
    A_SIZE,       /* size_t             -> unsigned decimal                    */
    A_OFLAGS,     /* open(2) flags      -> O_RDONLY|O_CREAT|...                */
    A_MODE,       /* mode_t             -> octal (0644)                        */
    A_PROT,       /* mmap/mprotect prot -> PROT_READ|PROT_WRITE|...            */
    A_MFLAGS,     /* mmap flags         -> MAP_PRIVATE|MAP_ANONYMOUS|...       */
    A_SIGNUM,     /* signal number      -> SIGKILL, SIGSEGV, ...              */
    A_WHENCE      /* lseek whence       -> SEEK_SET|SEEK_CUR|SEEK_END         */
};

/* ---------------------------------------------------------------------------
 * syscall_ent — one fully-decoded row.
 *
 * `argc` is signed on purpose: a value of -1 is our sentinel for "we know the
 * NAME of this syscall but not its argument shape", so the tracer falls back to
 * dumping all six registers as hex. That keeps the table honest — we never
 * pretend to decode an argument we haven't actually described.
 * --------------------------------------------------------------------------- */
struct syscall_ent {
    const char   *name;      /* e.g. "openat"; NULL means "number unknown"     */
    signed char   argc;      /* 0..6 real args, or -1 = "name known, args not" */
    unsigned char argt[6];   /* an enum arg_type per argument slot             */
};

/* Detailed lookup: returns a curated row (name + argc + argt) for the syscalls
 * we decode richly, or NULL for everything else. Never allocates; the returned
 * pointer aims at static, read-only table storage that lives for the program. */
const struct syscall_ent *syscall_detail(long nr);

/* Name-only lookup: covers the full x86-64 set so even syscalls we don't decode
 * still print with their real name. Returns NULL for numbers off the end of the
 * table (then the caller prints a bare "syscall_<nr>"). */
const char *syscall_name(long nr);

/* Highest table index + 1. Handy for tests and for bounds sanity checks. */
long syscall_table_size(void);

#endif /* SYSCALL_TABLE_H */
