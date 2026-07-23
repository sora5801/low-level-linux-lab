/* ===========================================================================
 * symbolize.c — address -> function name, via dladdr(3).
 * ===========================================================================
 *
 * HOW dladdr WORKS (and why it needs -rdynamic)
 * ---------------------------------------------
 * dladdr() takes an address and searches the loaded shared objects' DYNAMIC
 * symbol tables (.dynsym) for the symbol whose value is the largest one <= that
 * address — i.e. the function the address falls inside. It fills in:
 *
 *     Dl_info { dli_fname; dli_fbase; dli_sname; dli_saddr; }
 *              module path  load addr  symbol   symbol addr
 *
 * The catch: dladdr only sees symbols that are EXPORTED into .dynsym. For a
 * shared library that is its public API. For the MAIN EXECUTABLE, by default
 * almost nothing is exported, so dladdr would resolve every address in your own
 * program to nothing. Linking the profiler with `-rdynamic` (a.k.a.
 * -Wl,--export-dynamic) copies the executable's global symbols into .dynsym, so
 * dladdr can name our own functions. That flag is in the Makefile and is the
 * whole reason our workload frames get real names instead of hex.
 *
 * WHAT THIS CANNOT DO (honest scope)
 * ----------------------------------
 *   * `static` functions are NOT in .dynsym even with -rdynamic, so an address
 *     inside one resolves to the nearest *exported* symbol below it — which may
 *     be the wrong function. Our workload functions are therefore declared with
 *     external linkage so they name correctly.
 *   * It only sees OUR address space. In perf attach mode (-p PID) the samples
 *     are addresses in a DIFFERENT process, where our dladdr is meaningless;
 *     those fold as raw hex. A real profiler (perf, pprof) instead reads the
 *     target's /proc/PID/maps, finds the backing ELF for each address, and looks
 *     the address up in that file's symbol table / DWARF. That machinery is out
 *     of scope for this teaching core; the README says so.
 * =========================================================================== */
#define _GNU_SOURCE     /* dladdr is a GNU extension; this exposes it + Dl_info */
#include "symbolize.h"

#include <dlfcn.h>      /* dladdr, Dl_info                                     */
#include <stdio.h>      /* snprintf                                            */
#include <string.h>     /* memset, strrchr                                     */

void sym_name(uint64_t ip, char *buf, size_t buflen)
{
    Dl_info info;
    /* dladdr does not clear the struct on failure, so we do — otherwise a stale
     * dli_fname from a previous call could be read as valid below. */
    memset(&info, 0, sizeof info);

    /* dladdr returns NONZERO on success (the opposite of most libc calls, a
     * classic footgun). We still guard dli_sname separately: dladdr can succeed
     * at the module level (found the .so) yet have no symbol for the address. */
    int ok = dladdr((void *)(uintptr_t)ip, &info);

    if (ok && info.dli_sname) {
        /* Best case: a named symbol. Emit just the name so all addresses inside
         * the same function collapse to one flame-graph frame. */
        snprintf(buf, buflen, "%s", info.dli_sname);
        return;
    }

    if (ok && info.dli_fname) {
        /* We know the module but not the function (e.g. a stripped .so). Report
         * module basename + offset from its load address — still useful, and
         * addr2line on that offset recovers the function. */
        const char *slash = strrchr(info.dli_fname, '/');
        const char *base  = slash ? slash + 1 : info.dli_fname;
        unsigned long off = (unsigned long)(ip - (uint64_t)(uintptr_t)info.dli_fbase);
        snprintf(buf, buflen, "%s+0x%lx", base, off);
        return;
    }

    /* Nothing resolved (e.g. a kernel address, or a foreign process in attach
     * mode). Fall back to the raw address; flamegraph.pl accepts hex frames. */
    snprintf(buf, buflen, "0x%llx", (unsigned long long)ip);
}
