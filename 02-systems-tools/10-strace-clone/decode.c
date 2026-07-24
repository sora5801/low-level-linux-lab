/* ===========================================================================
 * decode.c — argument & return-value decoding for the strace clone.
 * ===========================================================================
 *
 * Everything here answers one question: given a raw 64-bit word that came out
 * of a tracee register, plus a type tag from the syscall table, what text does
 * a human want to see? Three techniques appear, in order of interest:
 *
 *   1. FOLLOWING A POINTER INTO ANOTHER PROCESS. A syscall arg like the path in
 *      openat is a pointer that is only valid in the *tracee's* address space.
 *      We cannot dereference it directly; we must ask the kernel to copy it out,
 *      one word at a time, with ptrace(PTRACE_PEEKDATA). read_tracee_str() does
 *      exactly that and handles the two failure modes that matter: an unmapped
 *      address (EFAULT on the first word) and a string that runs off the end of
 *      a mapped page (a short read we accept gracefully).
 *
 *   2. THE FLAG-DECODE TABLE WALK. open(2)'s flags, mmap(2)'s prot/flags, etc.
 *      are bitmasks. decode_flags() walks a small {mask,name} table, ORs in the
 *      name of every bit that is set, and prints any leftover bits as hex. This
 *      is the single most reused routine in the file — it is extracted verbatim
 *      into asm/demo.c so its assembly can be annotated.
 *
 *   3. NEGATIVE-ERRNO RETURN VALUES. The kernel returns errors as the small
 *      negative range [-4095, -1]; glibc's syscall wrappers turn those into
 *      -1 + errno, but at the ptrace boundary we see the raw negative. We map it
 *      back to "ENOENT (No such file or directory)" the way strace does.
 *
 * Platform: Linux only (it calls ptrace). This file is therefore NOT part of
 * the self-contained-assembly deliverable; see asm/demo.c for that.
 * ===========================================================================
 */
#include <sys/ptrace.h>   /* ptrace(), PTRACE_PEEKDATA                        */
#include <errno.h>        /* errno — PEEKDATA reports failure only via errno  */
#include <stdio.h>        /* snprintf for safe, bounded number formatting     */
#include <string.h>       /* strlen                                          */

#include "decode.h"
#include "syscall_table.h"

/* ===========================================================================
 * 1. Reading a C string out of the tracee's memory.
 * ===========================================================================
 *
 * PTRACE_PEEKDATA copies ONE machine word (8 bytes on x86-64) from the tracee's
 * data space and *returns it as the ptrace() return value*. That is a problem:
 * the legitimately-read word might be -1 (0xffff...ff), which is also ptrace's
 * error sentinel. The documented fix — the reason this looks fussy — is to zero
 * errno before the call and inspect errno after: a return of -1 is only an error
 * if errno became nonzero. We check EFAULT (address not mapped) and EIO.
 *
 * We copy word-by-word and scan each word's bytes for the NUL terminator. x86-64
 * is little-endian, so byte 0 of the word is the lowest-addressed byte — we
 * shift right by 8*b to pull out byte b in ascending address order.
 *
 * A faster real-world approach is process_vm_readv(2), which copies a whole
 * range in one syscall with no per-word trap; we use the classic PEEKDATA loop
 * here because it makes the "another process's memory is not yours" point
 * viscerally, and because it needs no extra headers.
 * =========================================================================== */
long read_tracee_str(pid_t pid, unsigned long addr, char *buf, long cap)
{
    long out = 0;                          /* bytes committed to buf so far    */
    const long W = (long)sizeof(long);     /* 8: bytes copied per PEEKDATA     */

    if (cap <= 0)
        return 0;

    while (out < cap - 1) {                /* leave room for the NUL           */
        errno = 0;                         /* MUST clear: -1 is a valid word   */
        long word = ptrace(PTRACE_PEEKDATA, pid, (void *)(addr + (unsigned long)out), 0);
        if (word == -1 && errno != 0) {
            /* First word unreadable => nothing to show (bad pointer). Later
             * word unreadable => return the prefix we already have. */
            return (out == 0) ? -1 : out;
        }
        /* Spill this word's bytes into buf, stopping at NUL or capacity. */
        for (long b = 0; b < W && out < cap - 1; b++, out++) {
            char c = (char)((word >> (8 * b)) & 0xff);
            buf[out] = c;
            if (c == '\0')
                return out;                /* found the terminator: done       */
        }
    }
    buf[out] = '\0';                       /* ran out of room: terminate       */
    return out;
}

/* ===========================================================================
 * 2. The flag-decode table walk (the routine mirrored in asm/demo.c).
 * ===========================================================================
 */

/* One bit (or bit-group) of a bitmask and the symbol it stands for. */
struct flag { unsigned long mask; const char *name; };

/* Walk `table` (n rows). For every row whose bits are all present in `value`,
 * append its name (|-separated) and clear those bits. Any bits still set at the
 * end are printed as a trailing hex literal. Writes a NUL-terminated result into
 * out[cap] and returns its length. This is THE hot pure-logic routine; keep it
 * identical to asm/demo.c::decode_flags so the annotated asm stays faithful. */
static int decode_flags(unsigned long value, const struct flag *table, int n,
                        char *out, int cap)
{
    int pos = 0;                           /* write cursor into out            */
    int wrote_any = 0;                     /* have we emitted a name yet?      */

    for (int i = 0; i < n && pos < cap - 1; i++) {
        /* mask != 0 guards against a 0-valued entry (like O_RDONLY) swallowing
         * everything: "bits all present" is trivially true for mask 0. Real
         * access-mode handling happens in format_arg before we get here. */
        if (table[i].mask != 0 && (value & table[i].mask) == table[i].mask) {
            if (wrote_any && pos < cap - 1)
                out[pos++] = '|';          /* separator between flag names     */
            int k = 0;
            const char *nm = table[i].name;
            while (nm[k] && pos < cap - 1)  /* copy the name in                */
                out[pos++] = nm[k++];
            value &= ~table[i].mask;        /* consume the bits we named        */
            wrote_any = 1;
        }
    }
    /* Leftover bits we have no name for -> show them so nothing is hidden. */
    if (value != 0) {
        char tmp[24];
        int m = snprintf(tmp, sizeof(tmp), "%s0x%lx", wrote_any ? "|" : "", value);
        for (int j = 0; j < m && pos < cap - 1; j++)
            out[pos++] = tmp[j];
    } else if (!wrote_any && pos < cap - 1) {
        out[pos++] = '0';                  /* nothing set at all: print "0"    */
    }
    out[pos] = '\0';
    return pos;
}

/* --- The flag tables. Constants are transcribed from the Linux uapi headers
 * (<asm-generic/fcntl.h>, <asm-generic/mman-common.h>) and given as octal where
 * the kernel does, so you can diff them against the header by eye. --- */

/* open(2) flags ABOVE the 2-bit access mode. The access mode (O_RDONLY/WRONLY/
 * RDWR) is NOT here; format_arg() decodes value&3 separately because O_RDONLY is
 * the *absence* of bits (value 0) and a table walk cannot match "no bits". */
static const struct flag oflags[] = {
    { 000000100, "O_CREAT"     }, { 000000200, "O_EXCL"      },
    { 000000400, "O_NOCTTY"    }, { 000001000, "O_TRUNC"     },
    { 000002000, "O_APPEND"    }, { 000004000, "O_NONBLOCK"  },
    { 000010000, "O_DSYNC"     }, { 000200000, "O_DIRECTORY" },
    { 000400000, "O_NOFOLLOW"  }, { 002000000, "O_CLOEXEC"   },
    { 004010000, "O_SYNC"      }, { 000100000, "O_LARGEFILE" },
    { 000040000, "O_DIRECT"    }, { 001000000, "O_NOATIME"   },
};

/* mmap/mprotect protection bits (<asm-generic/mman-common.h>). PROT_NONE is 0,
 * handled by decode_flags' "nothing set -> 0" branch. */
static const struct flag protflags[] = {
    { 0x1, "PROT_READ" }, { 0x2, "PROT_WRITE" }, { 0x4, "PROT_EXEC" },
};

/* mmap flags (<asm-generic/mman.h> / <bits/mman.h>). MAP_SHARED and MAP_PRIVATE
 * are a 2-value field but are distinct nonzero bits (1 and 2), so the table
 * walk handles them fine. */
static const struct flag mapflags[] = {
    { 0x01, "MAP_SHARED" }, { 0x02, "MAP_PRIVATE" }, { 0x10, "MAP_FIXED" },
    { 0x20, "MAP_ANONYMOUS" }, { 0x100, "MAP_GROWSDOWN" },
    { 0x0800, "MAP_DENYWRITE" }, { 0x2000, "MAP_LOCKED" },
    { 0x4000, "MAP_NORESERVE" }, { 0x8000, "MAP_POPULATE" },
    { 0x40000, "MAP_STACK" },
};

/* Signal number -> name. Values are the x86-64 asm-generic assignments. */
static const char *const signames[] = {
    [1]="SIGHUP", [2]="SIGINT", [3]="SIGQUIT", [4]="SIGILL", [5]="SIGTRAP",
    [6]="SIGABRT", [7]="SIGBUS", [8]="SIGFPE", [9]="SIGKILL", [10]="SIGUSR1",
    [11]="SIGSEGV", [12]="SIGUSR2", [13]="SIGPIPE", [14]="SIGALRM",
    [15]="SIGTERM", [16]="SIGSTKFLT", [17]="SIGCHLD", [18]="SIGCONT",
    [19]="SIGSTOP", [20]="SIGTSTP", [21]="SIGTTIN", [22]="SIGTTOU",
    [23]="SIGURG", [24]="SIGXCPU", [25]="SIGXFSZ", [26]="SIGVTALRM",
    [27]="SIGPROF", [28]="SIGWINCH", [29]="SIGIO", [30]="SIGPWR", [31]="SIGSYS",
};

/* errno -> {name, message}, in the strace "-1 ENOENT (No such file...)" style.
 * Only the common low numbers; unknown codes fall back to a bare number. */
struct err_ent { const char *name, *msg; };
static const struct err_ent errno_tab[] = {
    [1]={"EPERM","Operation not permitted"},   [2]={"ENOENT","No such file or directory"},
    [3]={"ESRCH","No such process"},            [4]={"EINTR","Interrupted system call"},
    [5]={"EIO","Input/output error"},           [6]={"ENXIO","No such device or address"},
    [7]={"E2BIG","Argument list too long"},     [8]={"ENOEXEC","Exec format error"},
    [9]={"EBADF","Bad file descriptor"},        [10]={"ECHILD","No child processes"},
    [11]={"EAGAIN","Resource temporarily unavailable"},
    [12]={"ENOMEM","Cannot allocate memory"},   [13]={"EACCES","Permission denied"},
    [14]={"EFAULT","Bad address"},              [16]={"EBUSY","Device or resource busy"},
    [17]={"EEXIST","File exists"},              [19]={"ENODEV","No such device"},
    [20]={"ENOTDIR","Not a directory"},         [21]={"EISDIR","Is a directory"},
    [22]={"EINVAL","Invalid argument"},         [24]={"EMFILE","Too many open files"},
    [25]={"ENOTTY","Inappropriate ioctl for device"},
    [28]={"ENOSPC","No space left on device"},  [29]={"ESPIPE","Illegal seek"},
    [32]={"EPIPE","Broken pipe"},               [38]={"ENOSYS","Function not implemented"},
    [40]={"ELOOP","Too many levels of symbolic links"},
};

/* ===========================================================================
 * 3. Rendering one argument.
 * ===========================================================================
 *
 * A_STR is the interesting case: `val` is a tracee pointer, so we go read the
 * bytes and escape them into a C-string literal (control chars -> \n, \t, or
 * \xNN), truncating long strings with "..." exactly as strace does. Everything
 * else is a local formatting decision on the register value itself.
 * =========================================================================== */

/* Append one source byte to a "..."-style quoted string, escaping as needed. */
static int esc_byte(char c, char *out, int cap, int pos)
{
    unsigned char u = (unsigned char)c;
    const char *rep = 0;
    char two[3] = { '\\', 0, 0 };
    if      (c == '\n') { two[1] = 'n'; rep = two; }
    else if (c == '\t') { two[1] = 't'; rep = two; }
    else if (c == '\r') { two[1] = 'r'; rep = two; }
    else if (c == '\\') { two[1] = '\\'; rep = two; }
    else if (c == '"')  { two[1] = '"'; rep = two; }
    if (rep) {
        for (int i = 0; rep[i] && pos < cap - 1; i++) out[pos++] = rep[i];
        return pos;
    }
    if (u >= 0x20 && u < 0x7f) {           /* printable ASCII: copy as-is      */
        if (pos < cap - 1) out[pos++] = c;
        return pos;
    }
    /* Non-printable: \xNN hex escape. */
    {
        char h[5];
        int m = snprintf(h, sizeof(h), "\\x%02x", u);
        for (int i = 0; i < m && pos < cap - 1; i++) out[pos++] = h[i];
    }
    return pos;
}

int format_arg(pid_t pid, int type, unsigned long val, char *out, int cap)
{
    if (cap <= 0)
        return 0;

    switch (type) {
    case A_INT:
        return snprintf(out, cap, "%d", (int)val);
    case A_LONG:
        return snprintf(out, cap, "%ld", (long)val);
    case A_SIZE:
        return snprintf(out, cap, "%lu", val);
    case A_HEX:
        return snprintf(out, cap, "%#lx", val);

    case A_PTR:
        /* A null pointer is meaningful (e.g. mmap hint) -> print NULL. */
        return val ? snprintf(out, cap, "%#lx", val)
                   : snprintf(out, cap, "NULL");

    case A_FD:
        /* dirfd sentinel used by the *at() syscalls. AT_FDCWD is -100. libc may
         * place it in the register either sign-extended (0xffff...ff9c) or
         * zero-extended (0x0000...ff9c) depending on how the wrapper moved the
         * 32-bit int, so we compare the low 32 bits as a signed int — which
         * matches both forms. */
        if ((int)val == -100)
            return snprintf(out, cap, "AT_FDCWD");
        return snprintf(out, cap, "%d", (int)val);

    case A_MODE:
        /* File mode is conventionally shown in octal, e.g. 0644. */
        return snprintf(out, cap, "0%lo", val);

    case A_STR: {
        if (val == 0)
            return snprintf(out, cap, "NULL");
        char raw[65];                       /* read a bounded chunk from tracee */
        long n = read_tracee_str(pid, val, raw, (long)sizeof(raw));
        if (n < 0)                          /* unreadable pointer               */
            return snprintf(out, cap, "%#lx", val);
        int pos = 0;
        if (pos < cap - 1) out[pos++] = '"';
        long show = n;
        int truncated = 0;
        if (show > 32) { show = 32; truncated = 1; }  /* strace-style clamp     */
        for (long i = 0; i < show; i++)
            pos = esc_byte(raw[i], out, cap, pos);
        if (pos < cap - 1) out[pos++] = '"';
        if (truncated) {                    /* mark that we cut it off          */
            const char *e = "...";
            for (int i = 0; e[i] && pos < cap - 1; i++) out[pos++] = e[i];
        }
        out[pos] = '\0';
        return pos;
    }

    case A_OFLAGS: {
        /* Access mode lives in the low two bits and is NOT a bitmask (O_RDONLY
         * is 0), so decode it first, then hand the rest to the flag walker. */
        static const char *acc[] = { "O_RDONLY", "O_WRONLY", "O_RDWR", "0x3" };
        int pos = snprintf(out, cap, "%s", acc[val & 3]);
        unsigned long rest = val & ~3UL;
        if (rest) {
            if (pos < cap - 1) out[pos++] = '|';
            pos += decode_flags(rest, oflags,
                                (int)(sizeof(oflags)/sizeof(oflags[0])),
                                out + pos, cap - pos);
        }
        return pos;
    }

    case A_PROT:
        return decode_flags(val, protflags,
                            (int)(sizeof(protflags)/sizeof(protflags[0])),
                            out, cap);
    case A_MFLAGS:
        return decode_flags(val, mapflags,
                            (int)(sizeof(mapflags)/sizeof(mapflags[0])),
                            out, cap);

    case A_SIGNUM: {
        long s = (long)val;
        if (s >= 0 && s < (long)(sizeof(signames)/sizeof(signames[0])) && signames[s])
            return snprintf(out, cap, "%s", signames[s]);
        return snprintf(out, cap, "%ld", s);
    }

    case A_WHENCE: {
        static const char *w[] = { "SEEK_SET", "SEEK_CUR", "SEEK_END" };
        if (val < 3)
            return snprintf(out, cap, "%s", w[val]);
        return snprintf(out, cap, "%lu", val);
    }

    default: /* A_NONE or anything unforeseen -> raw hex, never hide a value. */
        return snprintf(out, cap, "%#lx", val);
    }
}

/* ===========================================================================
 * Return-value formatting.
 * ===========================================================================
 *
 * The kernel packs errors into the top of the address space as small negatives:
 * a return in [-4095, -1] is "-errno". We detect that band and print the
 * symbolic form; otherwise it's a normal result (an fd, a byte count, an
 * address). `hex` asks for 0x formatting, used for mmap/brk which return
 * pointers rather than counts.
 * =========================================================================== */
int format_retval(long ret, int hex, char *out, int cap)
{
    if (ret < 0 && ret >= -4095) {
        long e = -ret;
        const char *nm = 0, *msg = 0;
        if (e < (long)(sizeof(errno_tab)/sizeof(errno_tab[0])) && errno_tab[e].name) {
            nm = errno_tab[e].name;
            msg = errno_tab[e].msg;
        }
        if (nm)
            return snprintf(out, cap, "-1 %s (%s)", nm, msg);
        return snprintf(out, cap, "-1 errno %ld", e);
    }
    if (hex)
        return snprintf(out, cap, "%#lx", (unsigned long)ret);
    return snprintf(out, cap, "%ld", ret);
}

/* Signal number -> name, reusing the table above. A tiny static buffer serves
 * the "unknown signal" path; it is not reentrant, which is fine for our
 * single-threaded tracer that prints one message at a time. */
const char *signal_name(int sig)
{
    static char fallback[16];
    if (sig >= 0 && sig < (int)(sizeof(signames)/sizeof(signames[0])) && signames[sig])
        return signames[sig];
    snprintf(fallback, sizeof(fallback), "SIG_%d", sig);
    return fallback;
}
