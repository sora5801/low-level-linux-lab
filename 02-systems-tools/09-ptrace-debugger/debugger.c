/* ===========================================================================
 * debugger.c — the REPL, the wait/dispatch loop, and main().
 * ===========================================================================
 *
 * This file ties the three lower layers together into an interactive debugger:
 *
 *   inferior.c    process control (ptrace/waitpid/proc)
 *   breakpoint.c  int3 breakpoints + step-over
 *   debuginfo.c   ELF symbols + DWARF line table
 *
 * The control loop of ANY ptrace debugger is a variation on:
 *
 *     resume the child (CONT or SINGLESTEP)
 *     waitpid  ->  it stopped; why?
 *         SIGTRAP at (bp_addr + 1)  -> a breakpoint we planted: rewind RIP, report
 *         SIGTRAP elsewhere          -> a single-step completion: report
 *         other signal               -> the program faulted/was signalled: report,
 *                                        remember to forward it on the next resume
 *         exited                     -> done
 *     read a command; repeat
 *
 * ADDRESS SPACES — the one idea that makes everything line up:
 *   DWARF/symbol addresses are LINK-TIME (what the linker assigned). A PIE is
 *   relocated at load, so the RUNTIME address is link_addr + load_base. We convert
 *   at every boundary: link->runtime to plant a breakpoint, runtime->link to name
 *   an address. load_base comes from /proc/<pid>/maps (see inferior.c).
 *
 * REPL commands: break/b, delete/d, continue/c, step/s, regs, reg, mem, bt,
 *                list/l, info, help, quit/q.  (`step` is a single MACHINE
 *                instruction — source-level `next` is called out as a stretch.)
 * ===========================================================================
 */
#define _GNU_SOURCE
#include <stdio.h>
#include <stdlib.h>
#include <string.h>       /* strtok_r, strcmp, strsignal                        */
#include <signal.h>       /* SIGTRAP, SIGKILL                                    */
#include <sys/user.h>     /* struct user_regs_struct                            */
#include <sys/ptrace.h>   /* PTRACE_KILL                                         */
#include <sys/types.h>
#include <unistd.h>
#include <inttypes.h>     /* PRIx64                                             */

#include "debugger.h"

/* ---- address-space conversion helpers ------------------------------------ */
static uint64_t link_to_rt(debugger *dbg, uint64_t link) { return link + dbg->load_base; }
static uint64_t rt_to_link(debugger *dbg, uint64_t rt)   { return rt   - dbg->load_base; }

/* ---------------------------------------------------------------------------
 * print_source_line — echo one line of a source file from disk (best effort).
 * The DWARF file name is usually how the compiler saw it (e.g. "sample.c"), so
 * this works when the debugger runs from the build directory. Purely cosmetic.
 * ------------------------------------------------------------------------- */
static void print_source_line(const char *file, uint32_t line)
{
    if (!file || !line) return;
    FILE *f = fopen(file, "r");
    if (!f) return;
    char   *buf = NULL; size_t cap = 0; uint32_t cur = 0;
    while (getline(&buf, &cap, f) > 0) {
        if (++cur == line) {
            /* Trim the trailing newline so our formatting stays tidy. */
            size_t n = strlen(buf);
            if (n && buf[n - 1] == '\n') buf[n - 1] = '\0';
            printf("    %u\t%s\n", line, buf);
            break;
        }
    }
    free(buf);
    fclose(f);
}

/* ---------------------------------------------------------------------------
 * report_stop — describe where the inferior is parked (runtime RIP).
 * Converts to link space, then asks debuginfo for function + file:line.
 * ------------------------------------------------------------------------- */
static void report_stop(debugger *dbg, uint64_t rt_rip)
{
    uint64_t link = rt_to_link(dbg, rt_rip);
    uint64_t off = 0;
    const char *fn = di_addr_to_func(&dbg->di, link, &off);
    const char *file = NULL; uint32_t line = 0;
    int have_line = di_addr_to_line(&dbg->di, link, &file, &line);

    printf("  stopped at 0x%" PRIx64, rt_rip);
    if (fn) printf(" in %s+0x%" PRIx64, fn, off);
    if (have_line) printf(" (%s:%u)", file, line);
    printf("\n");
    if (have_line) print_source_line(file, line);
}

/* ---------------------------------------------------------------------------
 * handle_stop — decode a waitpid result after a resume.
 * Returns 1 if the child is alive and stopped, 0 if it exited.
 * ------------------------------------------------------------------------- */
static int handle_stop(debugger *dbg, int stopsig, int exit_code, int alive)
{
    if (alive == 0) {
        if (exit_code >= 0)
            printf("[inferior exited normally, status %d]\n", exit_code);
        else
            printf("[inferior killed by signal %d (%s)]\n",
                   -exit_code, strsignal(-exit_code));
        dbg->running = 0;
        return 0;
    }

    if (stopsig == SIGTRAP) {
        /* A trap: either a planted int3 (RIP is one past the breakpoint) or a
         * single-step completion (RIP is wherever the step landed). */
        struct user_regs_struct regs;
        if (inferior_getregs(dbg->pid, &regs) < 0) return 1;

        breakpoint *bp = bp_find(dbg, regs.rip - 1);
        if (bp) {
            /* Breakpoint hit. The int3 already ran, so RIP == addr+1; rewind it
             * to addr so the real instruction executes when we resume. The 0xCC
             * stays in memory — step_over_breakpoint() will lift it at resume. */
            regs.rip -= 1;
            if (inferior_setregs(dbg->pid, &regs) < 0) return 1;
            printf("[breakpoint #%d hit]\n", bp->id);
        }
        report_stop(dbg, regs.rip);
        return 1;
    }

    /* Any other signal (SIGSEGV, SIGILL, SIGINT, ...) was raised BY the program.
     * We report it and remember to forward it into the child on the next resume,
     * so the program observes its own signal exactly as it would undebugged. */
    struct user_regs_struct regs;
    if (inferior_getregs(dbg->pid, &regs) == 0)
        report_stop(dbg, regs.rip);
    printf("[inferior stopped by signal %d (%s)]\n", stopsig, strsignal(stopsig));
    dbg->pending_sig = stopsig;
    return 1;
}

/* ---------------------------------------------------------------------------
 * do_continue — resume until the next stop.
 * If we are parked on an enabled breakpoint, hop over it first (restore byte,
 * single-step, re-arm) so we don't immediately re-trip on the same 0xCC.
 * ------------------------------------------------------------------------- */
static void do_continue(debugger *dbg)
{
    if (!dbg->running) { printf("inferior is not running\n"); return; }

    struct user_regs_struct regs;
    if (inferior_getregs(dbg->pid, &regs) == 0) {
        breakpoint *at = bp_find(dbg, regs.rip);
        if (at && at->enabled) {
            int r = step_over_breakpoint(dbg);
            if (r == 0) { dbg->running = 0; return; }   /* exited during the step */
            if (r < 0)  return;
        }
    }

    int sig = dbg->pending_sig; dbg->pending_sig = 0;   /* forward once, then clear */
    if (inferior_cont(dbg->pid, sig) < 0) return;

    int stopsig = 0, code = 0;
    int alive = inferior_wait(dbg->pid, &stopsig, &code);
    handle_stop(dbg, stopsig, code, alive);
}

/* do_step — advance exactly one machine instruction, then report. */
static void do_step(debugger *dbg)
{
    if (!dbg->running) { printf("inferior is not running\n"); return; }

    /* step_over_breakpoint handles both cases: if RIP sits on a breakpoint it
     * lifts the 0xCC, steps the true instruction, and re-arms; otherwise it just
     * single-steps. Either way exactly one instruction retires. */
    int r = step_over_breakpoint(dbg);
    if (r == 0) { dbg->running = 0; return; }
    if (r < 0)  return;

    struct user_regs_struct regs;
    if (inferior_getregs(dbg->pid, &regs) == 0)
        report_stop(dbg, regs.rip);
}

/* ---------------------------------------------------------------------------
 * dump_regs — a curated view of user_regs_struct. The full struct has ~27
 * fields; we print the general-purpose set plus RIP/RSP/RBP/RFLAGS, which is
 * what you reach for 99% of the time.
 * ------------------------------------------------------------------------- */
static void dump_regs(debugger *dbg)
{
    if (!dbg->running) { printf("inferior is not running\n"); return; }
    struct user_regs_struct r;
    if (inferior_getregs(dbg->pid, &r) < 0) return;

    printf("  rip 0x%016llx  rsp 0x%016llx  rbp 0x%016llx\n",
           (unsigned long long)r.rip, (unsigned long long)r.rsp,
           (unsigned long long)r.rbp);
    printf("  rax 0x%016llx  rbx 0x%016llx  rcx 0x%016llx  rdx 0x%016llx\n",
           (unsigned long long)r.rax, (unsigned long long)r.rbx,
           (unsigned long long)r.rcx, (unsigned long long)r.rdx);
    printf("  rsi 0x%016llx  rdi 0x%016llx  r8  0x%016llx  r9  0x%016llx\n",
           (unsigned long long)r.rsi, (unsigned long long)r.rdi,
           (unsigned long long)r.r8,  (unsigned long long)r.r9);
    printf("  r10 0x%016llx  r11 0x%016llx  r12 0x%016llx  r13 0x%016llx\n",
           (unsigned long long)r.r10, (unsigned long long)r.r11,
           (unsigned long long)r.r12, (unsigned long long)r.r13);
    printf("  r14 0x%016llx  r15 0x%016llx  eflags 0x%08llx\n",
           (unsigned long long)r.r14, (unsigned long long)r.r15,
           (unsigned long long)r.eflags);
}

/* ---------------------------------------------------------------------------
 * set_reg — write one named register via GETREGS/modify/SETREGS.
 * Demonstrates SETREGS: you can redirect control flow by writing RIP, or fake a
 * return value by writing RAX. We support the common names.
 * ------------------------------------------------------------------------- */
static void set_reg(debugger *dbg, const char *name, uint64_t val)
{
    if (!dbg->running) { printf("inferior is not running\n"); return; }
    struct user_regs_struct r;
    if (inferior_getregs(dbg->pid, &r) < 0) return;

    unsigned long long *slot = NULL;
    if      (!strcmp(name, "rip")) slot = (unsigned long long *)&r.rip;
    else if (!strcmp(name, "rsp")) slot = (unsigned long long *)&r.rsp;
    else if (!strcmp(name, "rbp")) slot = (unsigned long long *)&r.rbp;
    else if (!strcmp(name, "rax")) slot = (unsigned long long *)&r.rax;
    else if (!strcmp(name, "rbx")) slot = (unsigned long long *)&r.rbx;
    else if (!strcmp(name, "rcx")) slot = (unsigned long long *)&r.rcx;
    else if (!strcmp(name, "rdx")) slot = (unsigned long long *)&r.rdx;
    else if (!strcmp(name, "rsi")) slot = (unsigned long long *)&r.rsi;
    else if (!strcmp(name, "rdi")) slot = (unsigned long long *)&r.rdi;
    else { printf("unknown register '%s'\n", name); return; }

    *slot = val;
    if (inferior_setregs(dbg->pid, &r) == 0)
        printf("  %s = 0x%" PRIx64 "\n", name, val);
}

/* ---------------------------------------------------------------------------
 * mem_read — hex + ASCII dump of `len` bytes at a runtime address.
 * ------------------------------------------------------------------------- */
static void mem_read(debugger *dbg, uint64_t addr, size_t len)
{
    if (!dbg->running) { printf("inferior is not running\n"); return; }
    if (len == 0 || len > 4096) { printf("length must be 1..4096\n"); return; }

    unsigned char *buf = malloc(len);
    if (!buf) return;
    ssize_t n = inferior_read_mem(dbg->pid, addr, buf, len);
    if (n < 0) { free(buf); return; }

    for (ssize_t i = 0; i < n; i += 16) {
        printf("  0x%" PRIx64 ": ", addr + (uint64_t)i);
        for (ssize_t j = 0; j < 16; j++) {
            if (i + j < n) printf("%02x ", buf[i + j]);
            else           printf("   ");
        }
        printf(" |");
        for (ssize_t j = 0; j < 16 && i + j < n; j++) {
            unsigned char ch = buf[i + j];
            putchar((ch >= 32 && ch < 127) ? ch : '.');
        }
        printf("|\n");
    }
    free(buf);
}

/* mem_write — poke an 8-byte word at a runtime address (little-endian). */
static void mem_write(debugger *dbg, uint64_t addr, uint64_t val)
{
    if (!dbg->running) { printf("inferior is not running\n"); return; }
    if (inferior_write_mem(dbg->pid, addr, &val, sizeof val) == (ssize_t)sizeof val)
        printf("  wrote 0x%" PRIx64 " to 0x%" PRIx64 "\n", val, addr);
}

/* ---------------------------------------------------------------------------
 * backtrace — walk the saved-frame-pointer chain.
 *
 * With frame pointers (compile the target with -fno-omit-frame-pointer), each
 * call leaves this layout:
 *
 *     higher addresses
 *       ...
 *       [ return address ]   <- rbp + 8   (pushed by `call`)
 *       [ saved caller rbp ]  <- rbp       (pushed by the callee prologue)
 *       [ callee locals ... ] <- below rbp
 *
 * So from a frame's rbp we read the caller's rbp at *(rbp) and the return
 * address at *(rbp+8). Following the saved rbp links walks up the stack until we
 * hit a NULL rbp (the outermost frame). We subtract 1 from each return address
 * before symbolising, because it points to the instruction AFTER the call — its
 * line is the caller's next statement, but the CALL SITE is one byte earlier.
 * ------------------------------------------------------------------------- */
static void backtrace(debugger *dbg)
{
    if (!dbg->running) { printf("inferior is not running\n"); return; }
    struct user_regs_struct r;
    if (inferior_getregs(dbg->pid, &r) < 0) return;

    uint64_t pc = r.rip;
    uint64_t fp = r.rbp;

    for (int depth = 0; depth < 64; depth++) {
        /* Symbolise the current PC. Frame 0 uses RIP directly; deeper frames use
         * (return_addr - 1) to land inside the call instruction. */
        uint64_t sym_pc = (depth == 0) ? pc : pc - 1;
        uint64_t link = rt_to_link(dbg, sym_pc);
        uint64_t off = 0;
        const char *fn = di_addr_to_func(&dbg->di, link, &off);
        const char *file = NULL; uint32_t line = 0;
        int hl = di_addr_to_line(&dbg->di, link, &file, &line);

        printf("  #%-2d 0x%016" PRIx64 " %s", depth, pc, fn ? fn : "??");
        if (fn) printf("+0x%" PRIx64, off);
        if (hl) printf(" at %s:%u", file, line);
        printf("\n");

        if (fp == 0) break;            /* reached the outermost frame              */

        /* Read [saved_rbp, return_addr] = two words at fp. */
        uint64_t frame[2] = {0, 0};
        if (inferior_read_mem(dbg->pid, fp, frame, sizeof frame) != (ssize_t)sizeof frame)
            break;                     /* unreadable: stop rather than guess        */
        uint64_t saved_rbp = frame[0];
        uint64_t ret_addr  = frame[1];

        if (ret_addr == 0) break;
        /* Guard against a cycle / garbage: rbp must move UP the stack each step. */
        if (saved_rbp <= fp) break;

        pc = ret_addr;
        fp = saved_rbp;
    }
}

/* ---------------------------------------------------------------------------
 * resolve_location — turn a `break` argument into a RUNTIME address.
 *   *0xADDR    absolute runtime address (no relocation)
 *   FILE:LINE  DWARF line lookup, relocated
 *   LINE       DWARF line lookup in any file, relocated
 *   NAME       ELF function symbol, relocated
 * Returns 1 on success.
 * ------------------------------------------------------------------------- */
static int resolve_location(debugger *dbg, const char *loc, uint64_t *rt_addr)
{
    if (loc[0] == '*') {                       /* raw runtime address              */
        *rt_addr = strtoull(loc + 1, NULL, 0);
        return 1;
    }

    const char *colon = strchr(loc, ':');
    if (colon) {                               /* FILE:LINE                        */
        char file[256];
        size_t flen = (size_t)(colon - loc);
        if (flen >= sizeof file) flen = sizeof file - 1;
        memcpy(file, loc, flen); file[flen] = '\0';
        uint32_t line = (uint32_t)strtoul(colon + 1, NULL, 10);
        uint64_t link;
        if (di_line_to_addr(&dbg->di, file, line, &link)) {
            *rt_addr = link_to_rt(dbg, link);
            return 1;
        }
        fprintf(stderr, "no code at %s\n", loc);
        return 0;
    }

    /* Bare number => line in any file. */
    char *endp = NULL;
    unsigned long asnum = strtoul(loc, &endp, 10);
    if (endp && *endp == '\0' && endp != loc) {
        uint64_t link;
        if (di_line_to_addr(&dbg->di, "", (uint32_t)asnum, &link)) {
            *rt_addr = link_to_rt(dbg, link);
            return 1;
        }
        fprintf(stderr, "no code at line %lu\n", asnum);
        return 0;
    }

    /* Otherwise: a function name. */
    uint64_t link;
    if (di_func_to_addr(&dbg->di, loc, &link)) {
        *rt_addr = link_to_rt(dbg, link);
        return 1;
    }
    fprintf(stderr, "cannot resolve '%s' (unknown function/line)\n", loc);
    return 0;
}

static void list_breakpoints(debugger *dbg)
{
    int any = 0;
    for (int i = 0; i < MAX_BREAKPOINTS; i++) {
        breakpoint *bp = &dbg->bps[i];
        if (!bp->in_use) continue;
        any = 1;
        uint64_t link = rt_to_link(dbg, bp->addr);
        uint64_t off = 0;
        const char *fn = di_addr_to_func(&dbg->di, link, &off);
        const char *file = NULL; uint32_t line = 0;
        int hl = di_addr_to_line(&dbg->di, link, &file, &line);
        printf("  #%-2d 0x%" PRIx64 " %s", bp->id, bp->addr, bp->enabled ? "" : "(disabled) ");
        if (fn) printf("in %s", fn);
        if (hl) printf(" at %s:%u", file, line);
        printf("\n");
    }
    if (!any) printf("  no breakpoints\n");
}

static void print_help(void)
{
    printf(
    "commands:\n"
    "  break|b  <loc>     set a breakpoint. <loc> = FUNC | FILE:LINE | LINE | *0xADDR\n"
    "  delete|d <id>      remove breakpoint #id\n"
    "  info               list breakpoints\n"
    "  continue|c         run until the next breakpoint/signal/exit\n"
    "  step|s             execute one machine instruction\n"
    "  regs               dump general-purpose registers\n"
    "  reg <name> <val>   write a register (rip,rsp,rbp,rax,rbx,rcx,rdx,rsi,rdi)\n"
    "  mem <addr> [len]   hex-dump len bytes (default 64) at runtime addr\n"
    "  memw <addr> <val>  write an 8-byte word at runtime addr\n"
    "  bt                 backtrace (frame-pointer walk)\n"
    "  list|l             show the current source line\n"
    "  help|h             this text\n"
    "  quit|q             kill the inferior and exit\n");
}

/* ---------------------------------------------------------------------------
 * repl — read a line, split into tokens, dispatch. Returns when the user quits
 * or EOF/child-death ends the session.
 * ------------------------------------------------------------------------- */
static void repl(debugger *dbg)
{
    char   *line = NULL;
    size_t  cap  = 0;

    printf("ptrace-dbg: inferior pid %d, load base 0x%" PRIx64 " (%s)\n",
           (int)dbg->pid, dbg->load_base, dbg->di.is_pie ? "PIE" : "non-PIE");
    printf("type 'help' for commands.\n");

    for (;;) {
        printf("(dbg) ");
        fflush(stdout);
        ssize_t n = getline(&line, &cap, stdin);
        if (n <= 0) { printf("\n"); break; }        /* EOF (Ctrl-D)                */

        char *save = NULL;
        char *cmd = strtok_r(line, " \t\r\n", &save);
        if (!cmd) continue;                         /* blank line                  */

        if (!strcmp(cmd, "quit") || !strcmp(cmd, "q")) {
            break;
        }
        else if (!strcmp(cmd, "help") || !strcmp(cmd, "h")) {
            print_help();
        }
        else if (!strcmp(cmd, "break") || !strcmp(cmd, "b")) {
            char *loc = strtok_r(NULL, " \t\r\n", &save);
            if (!loc) { printf("usage: break <func|file:line|line|*addr>\n"); continue; }
            uint64_t addr;
            if (resolve_location(dbg, loc, &addr)) {
                breakpoint *bp = bp_add(dbg, addr);
                if (bp) printf("  breakpoint #%d at 0x%" PRIx64 "\n", bp->id, addr);
            }
        }
        else if (!strcmp(cmd, "delete") || !strcmp(cmd, "d")) {
            char *ids = strtok_r(NULL, " \t\r\n", &save);
            if (!ids) { printf("usage: delete <id>\n"); continue; }
            bp_remove(dbg, (int)strtol(ids, NULL, 10));
        }
        else if (!strcmp(cmd, "info")) {
            list_breakpoints(dbg);
        }
        else if (!strcmp(cmd, "continue") || !strcmp(cmd, "c")) {
            do_continue(dbg);
            if (!dbg->running) break;               /* child gone: end session     */
        }
        else if (!strcmp(cmd, "step") || !strcmp(cmd, "s")) {
            do_step(dbg);
            if (!dbg->running) break;
        }
        else if (!strcmp(cmd, "regs")) {
            dump_regs(dbg);
        }
        else if (!strcmp(cmd, "reg")) {
            char *name = strtok_r(NULL, " \t\r\n", &save);
            char *val  = strtok_r(NULL, " \t\r\n", &save);
            if (!name || !val) { printf("usage: reg <name> <value>\n"); continue; }
            set_reg(dbg, name, strtoull(val, NULL, 0));
        }
        else if (!strcmp(cmd, "mem")) {
            char *as = strtok_r(NULL, " \t\r\n", &save);
            char *ls = strtok_r(NULL, " \t\r\n", &save);
            if (!as) { printf("usage: mem <addr> [len]\n"); continue; }
            uint64_t addr = strtoull(as, NULL, 0);
            size_t   len  = ls ? (size_t)strtoul(ls, NULL, 0) : 64;
            mem_read(dbg, addr, len);
        }
        else if (!strcmp(cmd, "memw")) {
            char *as = strtok_r(NULL, " \t\r\n", &save);
            char *vs = strtok_r(NULL, " \t\r\n", &save);
            if (!as || !vs) { printf("usage: memw <addr> <value>\n"); continue; }
            mem_write(dbg, strtoull(as, NULL, 0), strtoull(vs, NULL, 0));
        }
        else if (!strcmp(cmd, "bt")) {
            backtrace(dbg);
        }
        else if (!strcmp(cmd, "list") || !strcmp(cmd, "l")) {
            struct user_regs_struct r;
            if (dbg->running && inferior_getregs(dbg->pid, &r) == 0) {
                const char *file = NULL; uint32_t line = 0;
                if (di_addr_to_line(&dbg->di, rt_to_link(dbg, r.rip), &file, &line))
                    print_source_line(file, line);
                else printf("  no source line for 0x%llx\n",
                            (unsigned long long)r.rip);
            }
        }
        else {
            printf("unknown command '%s' (try 'help')\n", cmd);
        }
    }

    free(line);
}

/* ---------------------------------------------------------------------------
 * main — parse args, spawn the inferior, load its debug info, run the REPL,
 * then tear everything down.
 * ------------------------------------------------------------------------- */
int main(int argc, char **argv)
{
    if (argc < 2) {
        fprintf(stderr,
            "usage: %s <program> [args...]\n"
            "  a teaching ptrace debugger. Example:\n"
            "    %s ./sample\n", argv[0], argv[0]);
        return 2;
    }

    debugger dbg;
    memset(&dbg, 0, sizeof dbg);
    dbg.prog = argv[1];
    dbg.next_bp_id = 1;

    /* Spawn the child under trace; it comes back stopped at its entry point. */
    dbg.pid = inferior_spawn(&argv[1]);
    if (dbg.pid < 0) return 1;
    dbg.running = 1;

    /* Parse the ELF: function symbols + DWARF line table. Non-fatal if absent —
     * you can still break on *addresses and inspect registers/memory. */
    di_load(&dbg.di, dbg.prog);

    /* Compute the runtime load base. For a PIE (ET_DYN) the linker's addresses
     * are offsets and we add the base from /proc/<pid>/maps; for a fixed ET_EXEC
     * the addresses are already absolute, so the base is 0. */
    if (dbg.di.is_pie) {
        uint64_t base = 0;
        if (proc_load_base(dbg.pid, dbg.prog, &base) == 0)
            dbg.load_base = base;
        else
            fprintf(stderr, "warning: could not read load base from /proc/%d/maps; "
                            "PIE breakpoints by name may be wrong\n", (int)dbg.pid);
    } else {
        dbg.load_base = 0;
    }

    repl(&dbg);

    /* Teardown: if the child is still alive, kill it (PTRACE_KILL asks the kernel
     * to deliver SIGKILL to the tracee), reap it, then release the mmap'd ELF. */
    if (dbg.running) {
        ptrace(PTRACE_KILL, dbg.pid, (void *)0, (void *)0);
        int s = 0, c = 0;
        inferior_wait(dbg.pid, &s, &c);
    }
    di_free(&dbg.di);
    return 0;
}
