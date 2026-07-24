/* ===========================================================================
 * caps.c — capability composition and drop.
 * ===========================================================================
 *
 * A Linux "capability" is one bit of root's power carved off on its own:
 * CAP_NET_RAW (open raw sockets), CAP_SYS_ADMIN (mount, and a hundred other
 * things), CAP_CHOWN, and ~40 more. A process carries several capability SETS,
 * each a 64-bit bitmap:
 *
 *     effective   — the caps the kernel checks RIGHT NOW on a privileged op.
 *     permitted   — the ceiling the effective set may be raised to.
 *     inheritable — caps that can pass across execve via file-cap rules.
 *     bounding     — a per-process CEILING: a cap absent here can never re-enter
 *                    permitted, not even across execve of a set-uid-root binary.
 *     ambient     — caps that survive execve of a NON-privileged program.
 *
 * WHAT A CONTAINER WANTS.  Not "root" (all 64 bits) and not "nothing" — a
 * production engine keeps a small DEFAULT SET (Docker's is 14 caps) that lets
 * normal software work (chown files, bind port 80, send pings) while denying the
 * dangerous majority (SYS_ADMIN, SYS_MODULE, SYS_PTRACE, ...). We model exactly
 * that: compose a 64-bit KEEP mask, then remove every OTHER bit from the
 * bounding set and shrink the live sets to the keep mask.
 *
 * THE BITMASK COMPOSITION LOGIC in this file — build the keep mask, derive the
 * "drop" mask as its complement over the defined caps, and test membership bit
 * by bit — is exactly what asm/demo.c extracts and hand-annotates, because it is
 * the most instructive pure-logic core of the whole engine (see README Assembly
 * notes).
 *
 * ORDER MATTERS (and it is the whole point of doing this in the child, last):
 *   1. Trim the BOUNDING set first, while we still hold CAP_SETPCAP. Dropping a
 *      bounding bit is itself privileged, so it must happen before step 2 clears
 *      our effective set. The bounding trim is the DURABLE enforcement: it
 *      survives the upcoming execve and caps what the payload can ever regain.
 *   2. capset(2) the effective/permitted/inheritable of THIS thread down to the
 *      keep mask, taking effect immediately (before we even execve).
 * ===========================================================================
 */
#define _GNU_SOURCE
#include "engine.h"
#include "util.h"

#include <errno.h>
#include <linux/capability.h>  /* _LINUX_CAPABILITY_VERSION_3, cap data structs */
#include <stdint.h>
#include <sys/prctl.h>         /* prctl, PR_CAPBSET_DROP                        */
#include <sys/syscall.h>       /* syscall, SYS_capset                          */
#include <unistd.h>

/* The highest capability number the kernel currently defines is CAP_CHECKPOINT_
 * RESTORE (40); we scan a little past it for headroom. prctl() returns EINVAL
 * for numbers the running kernel does not define, which we ignore. */
#define CAP_SCAN_MAX 63

/* Docker's default kept set, by capability number (see capabilities(7)). Each
 * is deliberately low-risk-yet-useful; everything ELSE is dropped.
 *   0  CHOWN            1  DAC_OVERRIDE      3  FOWNER          4  FSETID
 *   5  KILL             6  SETGID            7  SETUID          8  SETPCAP
 *  10  NET_BIND_SERVICE 13  NET_RAW         18  SYS_CHROOT      27  MKNOD
 *  29  AUDIT_WRITE      31  SETFCAP
 * Notably ABSENT: SYS_ADMIN(21), SYS_PTRACE(19), SYS_MODULE(16), SYS_RAWIO(17),
 * NET_ADMIN(12), SYS_TIME(25) — the caps a container escape would love. */
static const int kDefaultKeep[] = {
    0, 1, 3, 4, 5, 6, 7, 8, 10, 13, 18, 27, 29, 31
};

/* Build the keep mask by setting one bit per kept capability number. Returning a
 * 64-bit word (not touching any global) keeps this pure and easy to reason
 * about — which is why demo.c can mirror it verbatim. */
unsigned long long caps_default_keep(void)
{
    unsigned long long mask = 0;
    for (size_t i = 0; i < sizeof kDefaultKeep / sizeof kDefaultKeep[0]; i++)
        mask |= (1ULL << kDefaultKeep[i]);   /* OR in bit N for cap number N     */
    return mask;
}

/* Membership test: is capability `cap` present in `keep`? A one-liner, but named
 * because it is THE decision made 64 times below and in demo.c. */
static int cap_kept(unsigned long long keep, int cap)
{
    return (keep >> cap) & 1ULL;             /* shift the bit down, mask to 0/1  */
}

void caps_apply(unsigned long long keep)
{
    /* --- Step 1: trim the BOUNDING set to exactly `keep`. -----------------
     * For every defined capability NOT in the keep mask, drop the bounding bit.
     * PR_CAPBSET_DROP is one-way and needs CAP_SETPCAP, which we still hold. A
     * dropped bounding bit can never return to permitted, even across execve —
     * this is the enforcement that outlives us into the payload. */
    for (int cap = 0; cap <= CAP_SCAN_MAX; cap++) {
        if (cap_kept(keep, cap))
            continue;                        /* keep it: leave the bounding bit  */
        if (prctl(PR_CAPBSET_DROP, cap, 0, 0, 0) == -1 && errno != EINVAL)
            warn("PR_CAPBSET_DROP");         /* EINVAL = undefined cap #, benign */
    }

    /* --- Step 2: capset(2) the live sets down to `keep`. ------------------
     * The v3 capability ABI splits 64 caps across two 32-bit "data" words
     * (data[0] = caps 0..31, data[1] = caps 32..63), so we slice the mask.
     * We set effective = permitted = keep, and inheritable = 0 (we do not want
     * these caps to leak into grandchildren via file-cap inheritance). */
    struct __user_cap_header_struct hdr;
    hdr.version = _LINUX_CAPABILITY_VERSION_3;   /* 0x20080522: the 64-bit ABI   */
    hdr.pid     = 0;                             /* 0 => operate on this thread   */

    struct __user_cap_data_struct data[2];
    uint32_t lo = (uint32_t)(keep & 0xffffffffULL);        /* caps 0..31          */
    uint32_t hi = (uint32_t)((keep >> 32) & 0xffffffffULL);/* caps 32..63         */

    data[0].effective = lo;  data[0].permitted = lo;  data[0].inheritable = 0;
    data[1].effective = hi;  data[1].permitted = hi;  data[1].inheritable = 0;

    /* capset(2) = syscall 126. Args: header ptr in rdi, data ptr in rsi. A cap
     * we ask to keep must already be permitted (it is — we are root in the user
     * namespace with a full set), so this only ever shrinks. */
    if (syscall(SYS_capset, &hdr, data) == -1)
        die("capset");
}
