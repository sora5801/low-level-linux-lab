/* ===========================================================================
 * inferior.c — everything that touches the traced process directly.
 * ===========================================================================
 *
 * This is the syscall floor of the debugger. Every function here is a thin,
 * heavily-checked wrapper over ONE kernel facility:
 *
 *   - ptrace(2)            : the debugging syscall (register/memory/step/cont)
 *   - fork(2) + execve(2)  : create and become the inferior
 *   - waitpid(2)           : learn why the child stopped
 *   - /proc/<pid>/mem      : bulk memory read/write
 *   - /proc/<pid>/maps     : find the runtime load address (for PIE/ASLR)
 *
 * ptrace's C prototype is:
 *
 *     long ptrace(enum __ptrace_request request, pid_t pid,
 *                 void *addr, void *data);
 *
 * The kernel entry is syscall #101 on x86-64. The glibc wrapper reorders things
 * so that, for the "PEEK" requests, the fetched word is the *return value*
 * rather than being written through `data`. That single quirk is the source of
 * the classic errno dance in inferior_peek() below.
 * ===========================================================================
 */

/* _GNU_SOURCE exposes the glibc `enum __ptrace_request` names and pread/pwrite. */
#define _GNU_SOURCE
#include <sys/ptrace.h>   /* ptrace, PTRACE_* requests                          */
#include <sys/types.h>
#include <sys/wait.h>     /* waitpid, WIFSTOPPED, WSTOPSIG, WIFEXITED, ...      */
#include <sys/user.h>     /* struct user_regs_struct                            */
#include <sys/personality.h> /* ADDR_NO_RANDOMIZE (optional ASLR disable)       */
#include <unistd.h>       /* fork, execve, pread, pwrite, close                 */
#include <fcntl.h>        /* open, O_RDONLY, O_RDWR                             */
#include <errno.h>        /* errno, EINTR                                        */
#include <string.h>       /* strstr, strtok_r                                    */
#include <stdio.h>        /* perror, snprintf, FILE, getline                    */
#include <stdlib.h>       /* strtoull                                           */
#include <stdint.h>

#include "debugger.h"

/* ---------------------------------------------------------------------------
 * inferior_spawn — fork a child, put it under trace, and exec the target.
 *
 * The canonical ptrace bootstrap:
 *
 *   CHILD:  ptrace(PTRACE_TRACEME, 0, 0, 0);   // "trace me, parent"
 *           execve(prog, argv, envp);          // replace image with the target
 *
 *   PARENT: waitpid(child) -> stops with SIGTRAP the instant execve completes.
 *
 * PTRACE_TRACEME makes the CHILD ask to be traced by its parent. The very next
 * successful execve then generates a SIGTRAP *before the first user instruction
 * runs*, because the kernel stops a freshly-exec'd tracee so the debugger can
 * install breakpoints before anything executes. We consume that stop here so the
 * caller gets a child that is alive and parked at its entry point.
 * ------------------------------------------------------------------------- */
pid_t inferior_spawn(char *const argv[])
{
    pid_t pid = fork();
    if (pid < 0) {                 /* fork failed: out of pids / memory          */
        perror("fork");
        return -1;
    }

    if (pid == 0) {
        /* ---- CHILD ---------------------------------------------------------
         * From here we run in the child until execve replaces us. Anything that
         * fails here must _exit(), never return, or we would run the parent's
         * REPL in a second process. */

        /* PTRACE_TRACEME: request #0. addr/data ignored. On return the child is
         * marked as traced by its real parent. It does NOT stop yet — the stop
         * is armed to fire on the next execve. */
        if (ptrace(PTRACE_TRACEME, 0, (void *)0, (void *)0) < 0) {
            perror("PTRACE_TRACEME");
            _exit(127);
        }

        /* Best-effort: disable ASLR for THIS child so its load base is stable
         * run-to-run, which makes the taught addresses reproducible. This only
         * affects the child (personality is per-process) and is not fatal if the
         * kernel refuses it. We still compute the true base from /proc/maps, so
         * correctness does not depend on this line. */
        (void)personality(ADDR_NO_RANDOMIZE);

        /* execve(2): syscall #59. args: rdi=path, rsi=argv, rdx=envp. Replaces
         * the process image; on success it DOES NOT RETURN. The pending TRACEME
         * stop fires as the new image is installed. */
        execvp(argv[0], argv);

        /* Only reached if exec failed (e.g. ENOENT). */
        perror("execvp");
        _exit(127);
    }

    /* ---- PARENT ------------------------------------------------------------
     * Reap the automatic post-execve stop. Until we see it, the child is not yet
     * guaranteed to be at its entry point. */
    int stopsig = 0, code = 0;
    if (inferior_wait(pid, &stopsig, &code) != 1) {
        /* The child died before stopping (e.g. exec failed). Nothing to trace. */
        return -1;
    }

    /* PTRACE_O_EXITKILL: request the kernel send SIGKILL to the tracee if WE (the
     * tracer) die. Without it, a crash in the debugger would leave the inferior
     * stopped forever as a zombie-ish orphan. This is pure hygiene, hence the
     * best-effort (void) cast — an older kernel that lacks the option is fine. */
    (void)ptrace(PTRACE_SETOPTIONS, pid, (void *)0, (void *)PTRACE_O_EXITKILL);

    return pid;
}

/* ---------------------------------------------------------------------------
 * inferior_wait — block until the child changes state, retrying on EINTR.
 *
 * waitpid(2) can be interrupted by a signal delivered to the DEBUGGER (not the
 * child), returning -1/EINTR. That is not an error we care about — we simply
 * reissue the wait. Any other -1 is real.
 *
 * Return: 1 = stopped (see *stopsig), 0 = exited/killed (see *exit_code), -1 err.
 * ------------------------------------------------------------------------- */
int inferior_wait(pid_t pid, int *stopsig, int *exit_code)
{
    int status;
    for (;;) {
        pid_t r = waitpid(pid, &status, 0);
        if (r < 0) {
            if (errno == EINTR) continue;   /* a signal hit US; go back to sleep  */
            perror("waitpid");
            return -1;
        }
        break;
    }

    if (WIFSTOPPED(status)) {
        /* The child is stopped by a signal. WSTOPSIG extracts which one — for our
         * breakpoints and single-steps this is SIGTRAP. */
        if (stopsig) *stopsig = WSTOPSIG(status);
        return 1;
    }
    if (WIFEXITED(status)) {
        if (exit_code) *exit_code = WEXITSTATUS(status);   /* normal exit(code)   */
        return 0;
    }
    if (WIFSIGNALED(status)) {
        if (exit_code) *exit_code = -WTERMSIG(status);     /* killed by a signal   */
        return 0;
    }
    return -1;   /* WIFCONTINUED or other: not expected in our simple loop         */
}

/* ---------------------------------------------------------------------------
 * Register access. GETREGS/SETREGS copy the ENTIRE user_regs_struct in one
 * ptrace call: rdi..r15, rip, rsp, rflags, the segment registers, etc. The
 * kernel copies it to/from `data`, which must point at a full struct.
 * ------------------------------------------------------------------------- */
int inferior_getregs(pid_t pid, struct user_regs_struct *regs)
{
    /* PTRACE_GETREGS: addr ignored, data = &regs (kernel writes the struct). */
    if (ptrace(PTRACE_GETREGS, pid, (void *)0, regs) < 0) {
        perror("PTRACE_GETREGS");
        return -1;
    }
    return 0;
}

int inferior_setregs(pid_t pid, const struct user_regs_struct *regs)
{
    /* SETREGS reads the struct FROM us. The cast drops const only because the
     * ptrace prototype is not const-correct; the kernel does not modify it. */
    if (ptrace(PTRACE_SETREGS, pid, (void *)0, (void *)regs) < 0) {
        perror("PTRACE_SETREGS");
        return -1;
    }
    return 0;
}

/* ---------------------------------------------------------------------------
 * inferior_peek — read one 8-byte word from the child's text/data.
 *
 * THE ERRNO DANCE: for PTRACE_PEEKTEXT glibc returns the fetched word as the
 * function's return value. But a word can legitimately be (long)-1 == 0xFFFF...F.
 * ptrace also uses -1 to signal failure. The only way to tell a real -1 word
 * from an error is to clear errno first and inspect it after: on success the
 * wrapper leaves errno untouched, on failure it sets it. Getting this wrong is a
 * famous source of phantom breakpoint bugs.
 * ------------------------------------------------------------------------- */
long inferior_peek(pid_t pid, uint64_t addr, int *ok)
{
    errno = 0;
    /* PEEKTEXT and PEEKDATA are identical on Linux (unified address space); we
     * use PEEKTEXT for code and it works for data too. addr = target address. */
    long word = ptrace(PTRACE_PEEKTEXT, pid, (void *)addr, (void *)0);
    if (word == -1 && errno != 0) {
        if (ok) *ok = 0;
        return -1;
    }
    if (ok) *ok = 1;
    return word;
}

/* inferior_poke — write one 8-byte word. POKETEXT takes the word in `data`. */
int inferior_poke(pid_t pid, uint64_t addr, uint64_t word)
{
    if (ptrace(PTRACE_POKETEXT, pid, (void *)addr, (void *)word) < 0) {
        perror("PTRACE_POKETEXT");
        return -1;
    }
    return 0;
}

/* ---------------------------------------------------------------------------
 * Bulk memory via /proc/<pid>/mem. PEEK/POKE move 8 bytes per syscall, which is
 * painful for dumping a buffer. The proc file lets us pread/pwrite arbitrary
 * ranges in one call, seeking to the virtual address as the file offset. The
 * tracee must be stopped (it is — we only call this at a stop) and we must have
 * permission (we do — we are its tracer).
 * ------------------------------------------------------------------------- */
static int open_proc_mem(pid_t pid, int flags)
{
    char path[64];
    snprintf(path, sizeof path, "/proc/%d/mem", (int)pid);
    int fd = open(path, flags);
    if (fd < 0) perror(path);
    return fd;
}

ssize_t inferior_read_mem(pid_t pid, uint64_t addr, void *buf, size_t len)
{
    int fd = open_proc_mem(pid, O_RDONLY);
    if (fd < 0) return -1;
    /* pread positions at `addr` without a separate lseek and without disturbing
     * any file offset — the address IS the offset into the process's space. */
    ssize_t n = pread(fd, buf, len, (off_t)addr);
    if (n < 0) perror("pread /proc/pid/mem");
    close(fd);
    return n;
}

ssize_t inferior_write_mem(pid_t pid, uint64_t addr, const void *buf, size_t len)
{
    int fd = open_proc_mem(pid, O_RDWR);
    if (fd < 0) return -1;
    ssize_t n = pwrite(fd, buf, len, (off_t)addr);
    if (n < 0) perror("pwrite /proc/pid/mem");
    close(fd);
    return n;
}

/* ---------------------------------------------------------------------------
 * Resume primitives.
 *
 * PTRACE_CONT restarts the child and lets it run until the next signal/stop.
 * PTRACE_SINGLESTEP sets the CPU's TF (trap) flag so the child executes exactly
 * ONE instruction and then stops with SIGTRAP. In both, `data` names a signal to
 * *inject* into the child as it resumes (0 = deliver nothing). We pass the
 * pending signal through when it is a real program signal (e.g. SIGSEGV) but
 * swallow our own SIGTRAPs so the child never sees the debugger's plumbing.
 * ------------------------------------------------------------------------- */
int inferior_cont(pid_t pid, int sig)
{
    if (ptrace(PTRACE_CONT, pid, (void *)0, (void *)(long)sig) < 0) {
        perror("PTRACE_CONT");
        return -1;
    }
    return 0;
}

int inferior_step(pid_t pid, int sig)
{
    if (ptrace(PTRACE_SINGLESTEP, pid, (void *)0, (void *)(long)sig) < 0) {
        perror("PTRACE_SINGLESTEP");
        return -1;
    }
    return 0;
}

/* ---------------------------------------------------------------------------
 * proc_load_base — where did the kernel actually load the executable?
 *
 * A modern PIE binary is an ELF ET_DYN: its DWARF/symbol addresses are offsets
 * from an unknown base the loader picks (ASLR). To place a breakpoint at a
 * source line we must add that base:  runtime = link_addr + load_base.
 *
 * /proc/<pid>/maps lists every mapping, one per line:
 *
 *   55d3e0e00000-55d3e0e01000 r-xp 00001000 08:01 12345  /path/to/prog
 *   ^^^^^^^^^^^^ start          perms                     pathname
 *
 * The load base is the start address of the FIRST (lowest) mapping whose
 * pathname is our executable. (More precisely it is that start minus the file
 * offset of the first PT_LOAD, which is 0 for ordinary PIEs — noted in the
 * README as a simplification.)
 * ------------------------------------------------------------------------- */
int proc_load_base(pid_t pid, const char *exe_path, uint64_t *base)
{
    char path[64];
    snprintf(path, sizeof path, "/proc/%d/maps", (int)pid);
    FILE *f = fopen(path, "r");
    if (!f) { perror(path); return -1; }

    /* We match on the basename so an absolute vs relative invocation path still
     * lines up with the absolute pathname the kernel prints in maps. */
    const char *want = strrchr(exe_path, '/');
    want = want ? want + 1 : exe_path;

    char   *line = NULL;
    size_t  cap  = 0;
    int     found = 0;
    uint64_t lowest = 0;

    while (getline(&line, &cap, f) > 0) {
        /* Only lines that name a file on disk carry a pathname after the inode. */
        char *slash = strchr(line, '/');
        if (!slash) continue;
        char *bn = strrchr(line, '/');
        bn = bn ? bn + 1 : slash;
        /* Trim trailing newline for a clean compare. */
        char *nl = strchr(bn, '\n');
        if (nl) *nl = '\0';
        if (strcmp(bn, want) != 0) continue;

        /* Parse the leading "start-end" and keep the smallest start we see. */
        uint64_t start = strtoull(line, NULL, 16);
        if (!found || start < lowest) { lowest = start; found = 1; }
    }

    free(line);
    fclose(f);

    if (!found) return -1;
    *base = lowest;
    return 0;
}
