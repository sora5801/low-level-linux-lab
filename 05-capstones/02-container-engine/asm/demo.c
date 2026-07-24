/* ===========================================================================
 * demo.c — the clone-flags + capability bitmask composition, as pure logic.
 * ===========================================================================
 *
 * The engine's identity as a container is decided by two bitmasks:
 *
 *   1. the clone(2) FLAG WORD  — which of the CLONE_NEW* bits are OR'd together
 *      decides which kinds of isolation the child is born with (see main.c
 *      compose_clone_flags()).
 *   2. the capability KEEP/DROP masks — which of the 64 capability bits the
 *      payload retains vs. loses decides how much of "root" survives into the
 *      container (see caps.c caps_apply()).
 *
 * This file extracts BOTH as freestanding, header-free routines so we can
 * compile them to assembly and read exactly how "compose a flag set" and
 * "complement a mask over a range" become machine code. It is deliberately
 * self-contained: no #includes, no libc, its own fixed-width types and its own
 * copies of the handful of real kernel constants it uses. That is what lets
 * `clang --target=x86_64-pc-linux-gnu` turn it into Linux assembly on ANY host
 * (see the Makefile `asm` target and the project README's Assembly notes).
 *
 * A correct build exits with status 7 (three independent checks each contribute
 * one bit) so you can verify the logic without a debugger:  ./demo ; echo $?
 * ===========================================================================
 */

/* --- our own minimal type vocabulary (no headers) ------------------------- */
typedef unsigned int        u32;
typedef unsigned long long  u64;

/* --- the REAL kernel CLONE_NEW* values (from <linux/sched.h>) -------------- *
 * Each bit, when set in the clone() flag word, asks for one new namespace. We
 * hardcode the numeric values so this file needs no kernel headers; they are
 * stable UAPI. Note they are NOT contiguous — that non-obvious layout is part of
 * why we compose the word from a table rather than a shift of a counter. */
#define CLONE_NEWNS      0x00020000u   /* new mount namespace                    */
#define CLONE_NEWCGROUP  0x02000000u   /* new cgroup namespace                   */
#define CLONE_NEWUTS     0x04000000u   /* new UTS (hostname) namespace           */
#define CLONE_NEWIPC     0x08000000u   /* new System V IPC namespace             */
#define CLONE_NEWUSER    0x10000000u   /* new user namespace (the rootless key)  */
#define CLONE_NEWPID     0x20000000u   /* new PID namespace                      */
#define CLONE_NEWNET     0x40000000u   /* new network namespace                  */

/* --- our OWN request vocabulary ------------------------------------------- *
 * A caller expresses "which namespaces do I want" as a small dense bitmap; the
 * composer translates those dense bits into the sparse CLONE_NEW* bits. This
 * indirection is exactly what a config parser (or an OCI spec reader) feeds into
 * clone(). */
#define WANT_USER    (1u << 0)
#define WANT_MNT     (1u << 1)
#define WANT_PID     (1u << 2)
#define WANT_NET     (1u << 3)
#define WANT_UTS     (1u << 4)
#define WANT_IPC     (1u << 5)
#define WANT_CGROUP  (1u << 6)

/* The translation table: request bit -> kernel clone flag. Keeping it as data
 * (not a switch) is what a real runtime does, and it makes the generated loop a
 * clean "load pair, test, conditionally OR" that is a joy to annotate. */
struct ns_map { u32 want_bit; u32 clone_flag; };
static const struct ns_map kNsMap[] = {
    { WANT_USER,   CLONE_NEWUSER   },
    { WANT_MNT,    CLONE_NEWNS     },
    { WANT_PID,    CLONE_NEWPID    },
    { WANT_NET,    CLONE_NEWNET    },
    { WANT_UTS,    CLONE_NEWUTS    },
    { WANT_IPC,    CLONE_NEWIPC    },
    { WANT_CGROUP, CLONE_NEWCGROUP },
};
#define NS_MAP_LEN (sizeof kNsMap / sizeof kNsMap[0])

/* ---------------------------------------------------------------------------
 * compose_clone_flags — OR together the CLONE_NEW* bits for the requested set.
 *
 *   want : a bitmap of WANT_* request bits
 *   ->   : the clone(2) flag word to pass (minus the SIGCHLD termination signal,
 *          which main.c ORs in separately)
 *
 * noinline so the .s shows the GENERAL composer as one function to annotate,
 * rather than the optimizer constant-folding the whole thing away at the one
 * call site (which, being fed a compile-time-constant `want`, it otherwise
 * would — a lesson in itself, visible if you delete the attribute).
 * --------------------------------------------------------------------------- */
__attribute__((noinline))
u32 compose_clone_flags(u32 want)
{
    u32 flags = 0;
    for (u32 i = 0; i < NS_MAP_LEN; i++)
        if (want & kNsMap[i].want_bit)         /* did the caller ask for this ns? */
            flags |= kNsMap[i].clone_flag;      /* yes -> OR in the kernel bit     */
    return flags;
}

/* --- capability numbers we care about (from <linux/capability.h>) ---------- */
#define CAP_NET_RAW    13
#define CAP_SYS_ADMIN  21
#define CAP_LAST_CAP   40   /* CAP_CHECKPOINT_RESTORE — highest defined today    */

/* Docker's default KEEP set, by capability number (see caps.c). */
static const unsigned char kKeep[] = {
    0, 1, 3, 4, 5, 6, 7, 8, 10, 13, 18, 27, 29, 31
};
#define KEEP_LEN (sizeof kKeep / sizeof kKeep[0])

/* ---------------------------------------------------------------------------
 * cap_keep_mask — build the 64-bit keep mask by setting one bit per kept cap.
 * The mirror of caps_default_keep() in caps.c. noinline for the same reason. */
__attribute__((noinline))
u64 cap_keep_mask(void)
{
    u64 mask = 0;
    for (u32 i = 0; i < KEEP_LEN; i++)
        mask |= (1ULL << kKeep[i]);            /* set bit for cap number kKeep[i] */
    return mask;
}

/* ---------------------------------------------------------------------------
 * cap_drop_mask — the caps we must strip: every DEFINED capability (0..LAST_CAP)
 * that is NOT in the keep mask. This is the exact set caps_apply() walks to trim
 * the bounding set. It is a masked complement:
 *
 *     all_defined = (1 << (LAST_CAP + 1)) - 1     // bits 0..LAST_CAP set
 *     drop        = all_defined & ~keep
 *
 * Doing it in 64-bit is essential: LAST_CAP is 40, so the "all defined" mask
 * overflows a 32-bit type. Watch the .s use the 64-bit registers (rax/rdx, the
 * movabsq immediate) throughout — that is the payoff of the u64. */
__attribute__((noinline))
u64 cap_drop_mask(u64 keep)
{
    u64 all_defined = ((u64)1 << (CAP_LAST_CAP + 1)) - 1ULL;  /* bits 0..40       */
    return all_defined & ~keep;                              /* defined-but-not-kept */
}

/* Membership test: is capability `cap` present in `keep`? THE decision made 64
 * times by caps_apply()'s bounding-set loop, isolated here. */
static int cap_is_kept(u64 keep, int cap)
{
    return (int)((keep >> cap) & 1ULL);        /* shift the bit down, mask to 0/1 */
}

/* ---------------------------------------------------------------------------
 * main — three independent checks, each contributing one bit to the exit code:
 *   bit0: the composed flag word actually contains NEWUSER and NEWNET.
 *   bit1: CAP_NET_RAW is KEPT (a container can still ping).
 *   bit2: CAP_SYS_ADMIN is in the DROP mask (the dangerous cap is stripped).
 * A correct build therefore exits 1 + 2 + 4 = 7.
 * --------------------------------------------------------------------------- */
int main(void)
{
    int ok = 0;

    /* Compose the flag word we'd hand clone() for a full container. */
    u32 f = compose_clone_flags(WANT_USER | WANT_MNT | WANT_PID |
                                WANT_NET  | WANT_UTS | WANT_IPC);
    ok |= ((f & CLONE_NEWUSER) && (f & CLONE_NEWNET)) ? 1 : 0;

    /* Compose the capability masks and probe two representative caps. */
    u64 keep = cap_keep_mask();
    u64 drop = cap_drop_mask(keep);
    ok |= cap_is_kept(keep, CAP_NET_RAW)      ? 2 : 0;   /* kept    */
    ok |= ((drop >> CAP_SYS_ADMIN) & 1ULL)    ? 4 : 0;   /* dropped */

    return ok;                                            /* expect 7 */
}
