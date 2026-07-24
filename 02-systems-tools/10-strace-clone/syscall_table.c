/* ===========================================================================
 * syscall_table.c — the Linux x86-64 syscall map, as plain data.
 * ===========================================================================
 *
 * This file is INTENTIONALLY dependency-free: it includes no system headers,
 * only our own syscall_table.h. That is what lets `clang --target=x86_64-pc-
 * linux-gnu -S` turn it into teaching assembly on any host (see asm/). The
 * price is that we spell out every constant instead of #including it — which is
 * arguably a feature in a teaching lab, because you can see the raw numbers.
 *
 * TWO TABLES, TWO JOBS
 * --------------------
 *   detail[]  — a CURATED set (~45 syscalls) with full argument shapes. These
 *               are the calls a normal program actually makes at startup and in
 *               a typical run (execve, brk, arch_prctl, mmap, openat, read,
 *               write, close, mprotect, exit_group, ...). For these we can print
 *               strace-quality lines.
 *   names[]   — a BROAD name-only map covering the whole x86-64 table, so even a
 *               syscall we don't decode richly still shows up by name.
 *
 * Both use C99 *designated initializers* ([N] = ...). That does two nice things:
 * it decouples source order from the numeric index (so we can list rows in any
 * order), and it leaves every unmentioned slot zero-initialized — a { NULL, 0,
 * {0} } row whose NULL name our lookups treat as "absent". Holes in the syscall
 * space (deleted/unused numbers) therefore cost nothing and read as unknown.
 * ===========================================================================
 */
#include "syscall_table.h"

/* Shorthand so the detail rows below stay one-line and scannable. The trailing
 * A_NONE padding fills the fixed six-slot argt[] for syscalls with < 6 args;
 * format_arg() never looks past `argc`, so the padding is purely cosmetic. */
#define ENT(nm, n, ...) { nm, (signed char)(n), { __VA_ARGS__ } }

/* ---------------------------------------------------------------------------
 * detail[] — the richly-decoded syscalls.
 *
 * Read each row as: number = { "name", argc, {arg0_type, arg1_type, ...} }.
 * The arg types drive format_arg(): A_STR means "this word is a pointer into
 * the tracee; go read the C string it points at"; A_OFLAGS means "decode these
 * bits as open(2) flags"; and so on (see enum arg_type in the header).
 *
 * A few deliberate choices worth calling out:
 *  - write(2) arg1 is A_STR so we quote the bytes being written (strace does
 *    this too). read(2) arg1 is A_PTR because at syscall ENTRY the buffer is not
 *    filled yet — there is nothing meaningful to read.
 *  - mmap(2) arg0 is A_PTR (the hint address) and its offset is A_LONG.
 *  - openat(2) arg0 is A_FD so the dirfd renders as AT_FDCWD when it is -100.
 * --------------------------------------------------------------------------- */
static const struct syscall_ent detail[] = {
    [0]   = ENT("read",         3, A_FD, A_PTR, A_SIZE),
    [1]   = ENT("write",        3, A_FD, A_STR, A_SIZE),
    [2]   = ENT("open",         3, A_STR, A_OFLAGS, A_MODE),
    [3]   = ENT("close",        1, A_FD),
    [4]   = ENT("stat",         2, A_STR, A_PTR),
    [5]   = ENT("fstat",        2, A_FD, A_PTR),
    [6]   = ENT("lstat",        2, A_STR, A_PTR),
    [8]   = ENT("lseek",        3, A_FD, A_LONG, A_WHENCE),
    [9]   = ENT("mmap",         6, A_PTR, A_SIZE, A_PROT, A_MFLAGS, A_FD, A_LONG),
    [10]  = ENT("mprotect",     3, A_PTR, A_SIZE, A_PROT),
    [11]  = ENT("munmap",       2, A_PTR, A_SIZE),
    [12]  = ENT("brk",          1, A_PTR),
    [13]  = ENT("rt_sigaction", 4, A_SIGNUM, A_PTR, A_PTR, A_SIZE),
    [14]  = ENT("rt_sigprocmask", 4, A_INT, A_PTR, A_PTR, A_SIZE),
    [16]  = ENT("ioctl",        3, A_FD, A_HEX, A_HEX),
    [17]  = ENT("pread64",      4, A_FD, A_PTR, A_SIZE, A_LONG),
    [20]  = ENT("writev",       3, A_FD, A_PTR, A_INT),
    [21]  = ENT("access",       2, A_STR, A_INT),
    [39]  = ENT("getpid",       0),
    [59]  = ENT("execve",       3, A_STR, A_PTR, A_PTR),
    [60]  = ENT("exit",         1, A_INT),
    [62]  = ENT("kill",         2, A_INT, A_SIGNUM),
    [63]  = ENT("uname",        1, A_PTR),
    [72]  = ENT("fcntl",        3, A_FD, A_INT, A_HEX),
    [79]  = ENT("getcwd",       2, A_PTR, A_SIZE),
    [89]  = ENT("readlink",     3, A_STR, A_PTR, A_SIZE),
    [96]  = ENT("gettimeofday", 2, A_PTR, A_PTR),
    [102] = ENT("getuid",       0),
    [104] = ENT("getgid",       0),
    [107] = ENT("geteuid",      0),
    [108] = ENT("getegid",      0),
    [158] = ENT("arch_prctl",   2, A_INT, A_HEX),
    [202] = ENT("futex",        6, A_PTR, A_INT, A_INT, A_PTR, A_PTR, A_INT),
    [217] = ENT("getdents64",   3, A_FD, A_PTR, A_SIZE),
    [218] = ENT("set_tid_address", 1, A_PTR),
    [228] = ENT("clock_gettime", 2, A_INT, A_PTR),
    [231] = ENT("exit_group",   1, A_INT),
    [257] = ENT("openat",       4, A_FD, A_STR, A_OFLAGS, A_MODE),
    [262] = ENT("newfstatat",   4, A_FD, A_STR, A_PTR, A_INT),
    [273] = ENT("set_robust_list", 2, A_PTR, A_SIZE),
    [302] = ENT("prlimit64",    4, A_INT, A_INT, A_PTR, A_PTR),
    [318] = ENT("getrandom",    3, A_PTR, A_SIZE, A_HEX),
    [332] = ENT("statx",        5, A_FD, A_STR, A_INT, A_HEX, A_PTR),
    [334] = ENT("rseq",         4, A_PTR, A_SIZE, A_INT, A_HEX),
};

/* ---------------------------------------------------------------------------
 * names[] — every x86-64 syscall number we know, mapped to its name.
 *
 * This is the "so it at least prints correctly" safety net for the many
 * syscalls detail[] doesn't decode by argument. The numbers come straight from
 * the kernel's syscall_64.tbl. Gaps (e.g. numbers reserved or never used on
 * x86-64) are simply left NULL and read as unknown.
 * --------------------------------------------------------------------------- */
static const char *const names[] = {
    [0]="read", [1]="write", [2]="open", [3]="close", [4]="stat", [5]="fstat",
    [6]="lstat", [7]="poll", [8]="lseek", [9]="mmap", [10]="mprotect",
    [11]="munmap", [12]="brk", [13]="rt_sigaction", [14]="rt_sigprocmask",
    [15]="rt_sigreturn", [16]="ioctl", [17]="pread64", [18]="pwrite64",
    [19]="readv", [20]="writev", [21]="access", [22]="pipe", [23]="select",
    [24]="sched_yield", [25]="mremap", [26]="msync", [27]="mincore",
    [28]="madvise", [29]="shmget", [30]="shmat", [31]="shmctl", [32]="dup",
    [33]="dup2", [34]="pause", [35]="nanosleep", [36]="getitimer", [37]="alarm",
    [38]="setitimer", [39]="getpid", [40]="sendfile", [41]="socket",
    [42]="connect", [43]="accept", [44]="sendto", [45]="recvfrom",
    [46]="sendmsg", [47]="recvmsg", [48]="shutdown", [49]="bind", [50]="listen",
    [51]="getsockname", [52]="getpeername", [53]="socketpair", [54]="setsockopt",
    [55]="getsockopt", [56]="clone", [57]="fork", [58]="vfork", [59]="execve",
    [60]="exit", [61]="wait4", [62]="kill", [63]="uname", [64]="semget",
    [65]="semop", [66]="semctl", [67]="shmdt", [68]="msgget", [69]="msgsnd",
    [70]="msgrcv", [71]="msgctl", [72]="fcntl", [73]="flock", [74]="fsync",
    [75]="fdatasync", [76]="truncate", [77]="ftruncate", [78]="getdents",
    [79]="getcwd", [80]="chdir", [81]="fchdir", [82]="rename", [83]="mkdir",
    [84]="rmdir", [85]="creat", [86]="link", [87]="unlink", [88]="symlink",
    [89]="readlink", [90]="chmod", [91]="fchmod", [92]="chown", [93]="fchown",
    [94]="lchown", [95]="umask", [96]="gettimeofday", [97]="getrlimit",
    [98]="getrusage", [99]="sysinfo", [100]="times", [101]="ptrace",
    [102]="getuid", [103]="syslog", [104]="getgid", [105]="setuid",
    [106]="setgid", [107]="geteuid", [108]="getegid", [109]="setpgid",
    [110]="getppid", [111]="getpgrp", [112]="setsid", [113]="setreuid",
    [114]="setregid", [115]="getgroups", [116]="setgroups", [117]="setresuid",
    [118]="getresuid", [119]="setresgid", [120]="getresgid", [121]="getpgid",
    [122]="setfsuid", [123]="setfsgid", [124]="getsid", [125]="capget",
    [126]="capset", [127]="rt_sigpending", [128]="rt_sigtimedwait",
    [129]="rt_sigqueueinfo", [130]="rt_sigsuspend", [131]="sigaltstack",
    [132]="utime", [133]="mknod", [134]="uselib", [135]="personality",
    [136]="ustat", [137]="statfs", [138]="fstatfs", [139]="sysfs",
    [140]="getpriority", [141]="setpriority", [157]="prctl", [158]="arch_prctl",
    [186]="gettid", [201]="time", [202]="futex", [204]="sched_getaffinity",
    [213]="epoll_create", [217]="getdents64", [218]="set_tid_address",
    [221]="fadvise64", [228]="clock_gettime", [229]="clock_getres",
    [230]="clock_nanosleep", [231]="exit_group", [232]="epoll_wait",
    [233]="epoll_ctl", [234]="tgkill", [257]="openat", [258]="mkdirat",
    [259]="mknodat", [260]="fchownat", [262]="newfstatat", [263]="unlinkat",
    [264]="renameat", [265]="linkat", [266]="symlinkat", [267]="readlinkat",
    [268]="fchmodat", [269]="faccessat", [273]="set_robust_list",
    [280]="utimensat", [281]="epoll_pwait", [284]="eventfd", [288]="accept4",
    [290]="eventfd2", [291]="epoll_create1", [292]="dup3", [293]="pipe2",
    [302]="prlimit64", [316]="renameat2", [318]="getrandom", [319]="memfd_create",
    [332]="statx", [334]="rseq", [435]="clone3", [439]="faccessat2",
};

/* Number of slots in names[]; also the "highest known number + 1" bound. */
#define NAMES_N ((long)(sizeof(names) / sizeof(names[0])))
#define DETAIL_N ((long)(sizeof(detail) / sizeof(detail[0])))

/* Detailed lookup. Bounds-check first (a hostile or corrupt orig_rax could be
 * anything), then reject holes: a zero-initialized row has name == NULL, which
 * we report as "no rich decode available" by returning NULL. */
const struct syscall_ent *syscall_detail(long nr)
{
    if (nr < 0 || nr >= DETAIL_N)
        return 0;                 /* out of range -> undecoded                */
    if (detail[nr].name == 0)
        return 0;                 /* a hole in the curated set -> undecoded   */
    return &detail[nr];
}

/* Name-only lookup with the same bounds discipline. */
const char *syscall_name(long nr)
{
    if (nr < 0 || nr >= NAMES_N)
        return 0;
    return names[nr];             /* may itself be NULL for a table hole      */
}

/* Exposed for tests: how many name slots exist. */
long syscall_table_size(void)
{
    return NAMES_N;
}
