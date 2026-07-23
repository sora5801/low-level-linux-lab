/* ===========================================================================
 * asm/demo.c — the VMM's PURE-LOGIC core: the VM-exit dispatch, standalone.
 * ===========================================================================
 *
 * WHY THIS FILE EXISTS
 * --------------------
 * vmm.c and guest.c are real userspace C, but they #include <linux/kvm.h>,
 * <sys/ioctl.h>, <sys/mman.h> — Linux-only headers that are absent on the
 * Windows host this repo may live on, so clang cannot turn those files into
 * assembly here. The *decision-making heart* of the VMM, though, is pure
 * integer logic with no kernel dependency: given a KVM_EXIT_* reason code,
 * decide what the run loop should DO. That is the hottest, most instructive
 * routine in the whole program — it runs on every single VM exit — so we lift
 * it out into this self-contained file that includes nothing and declares its
 * own types, and generate real assembly from it.
 *
 * The functions here are byte-for-byte the classification vmm.c's run loop
 * performs in its `switch (run->exit_reason)`; only the ioctl() plumbing around
 * them is gone. Read the generated asm to SEE the compiler turn a C `switch`
 * into a chain of compares (we build with -fno-jump-tables, so there is no jump
 * table — every case is an explicit `cmp`/`je`, which is the clearest possible
 * form to trace), and to see a >16-byte struct returned via a hidden pointer
 * while a small one comes back packed in a register.
 *
 * The exit codes below are the REAL values from <linux/kvm.h>; we spell them out
 * so this file needs no headers. They are ABI-stable — KVM_EXIT_IO has been 2
 * for the life of the interface.
 * =========================================================================== */

/* Our own fixed-width type, so this file needs no <stdint.h>. On the x86-64
 * SysV target this is exactly the kernel's __u32 / unsigned int. */
typedef unsigned int u32;

/* KVM_EXIT_* reason codes (from <linux/kvm.h>), verbatim. run->exit_reason is
 * set to one of these by the kernel before KVM_RUN returns. */
#define KVM_EXIT_UNKNOWN         0u
#define KVM_EXIT_EXCEPTION       1u
#define KVM_EXIT_IO              2u   /* guest did IN/OUT on an unhandled port  */
#define KVM_EXIT_HYPERCALL       3u
#define KVM_EXIT_DEBUG           4u
#define KVM_EXIT_HLT             5u   /* guest executed `hlt`                   */
#define KVM_EXIT_MMIO            6u   /* access to a GPA with no memory slot    */
#define KVM_EXIT_IRQ_WINDOW_OPEN 7u
#define KVM_EXIT_SHUTDOWN        8u   /* triple fault / reset                   */
#define KVM_EXIT_FAIL_ENTRY      9u   /* VMENTRY refused our guest state        */
#define KVM_EXIT_INTR           10u   /* host signal interrupted KVM_RUN        */
#define KVM_EXIT_INTERNAL_ERROR 17u

/* Direction values for a KVM_EXIT_IO (run->io.direction). */
#define KVM_EXIT_IO_IN  0u
#define KVM_EXIT_IO_OUT 1u

/* What the run loop should do about an exit. This is the VMM's whole policy in
 * one enum: emulate a device, stop cleanly, stop with an error, transparently
 * re-enter the guest, or admit we do not handle it. */
enum vmm_action {
    ACT_UNHANDLED    = 0,   /* reason we do not model; caller should report    */
    ACT_EMULATE_IO   = 1,   /* KVM_EXIT_IO   -> service a port access          */
    ACT_EMULATE_MMIO = 2,   /* KVM_EXIT_MMIO -> service a memory-mapped access */
    ACT_STOP_OK      = 3,   /* clean end (guest halted)                        */
    ACT_STOP_ERR     = 4,   /* fatal (shutdown / failed entry / internal)      */
    ACT_REENTER      = 5,   /* transient; just call KVM_RUN again              */
};

/* -------------------------------------------------------------------------
 * kvm_exit_action — THE dispatch. Map an exit reason to a policy action.
 *
 * This is the routine that runs on every VM exit, so it is the one worth
 * reading in assembly. Built with -fno-jump-tables, the `switch` becomes a
 * straight-line sequence of `cmp $imm, %edi` / `je` — trace it top to bottom and
 * you are literally single-stepping the VMM's decision for each exit kind.
 * ABI: reason in %edi (arg0); the enum (an int) returns in %eax.
 * ------------------------------------------------------------------------- */
enum vmm_action kvm_exit_action(u32 reason)
{
    switch (reason) {
    case KVM_EXIT_IO:
        return ACT_EMULATE_IO;              /* the common case: a port write   */
    case KVM_EXIT_MMIO:
        return ACT_EMULATE_MMIO;            /* the device-memory cousin        */
    case KVM_EXIT_HLT:
        return ACT_STOP_OK;                 /* guest said "done"               */
    case KVM_EXIT_SHUTDOWN:
    case KVM_EXIT_FAIL_ENTRY:
    case KVM_EXIT_INTERNAL_ERROR:
        return ACT_STOP_ERR;                /* three roads to the same failure */
    case KVM_EXIT_INTR:
    case KVM_EXIT_IRQ_WINDOW_OPEN:
        return ACT_REENTER;                 /* transient; loop around again    */
    default:
        return ACT_UNHANDLED;               /* everything else: surface it     */
    }
}

/* -------------------------------------------------------------------------
 * io_batch_bytes — total bytes a port-I/O exit moves: size * count.
 *
 * A `rep outs`/`rep ins` can transfer `count` data of `size` bytes each in one
 * exit; the data live in a run-relative buffer. This product is exactly the
 * length the run loop must copy, and the bound it must not exceed. Watch the
 * compiler emit a single `imul` (or, at higher -O, an `lea`/shift if `size` is a
 * known power of two — but here both are runtime values, so it is a multiply).
 * ABI: size in %edi, count in %esi; product in %eax.
 * ------------------------------------------------------------------------- */
u32 io_batch_bytes(u32 size, u32 count)
{
    return size * count;
}

/* -------------------------------------------------------------------------
 * serial_is_console_write — decode: is this IO exit a write to our console?
 *
 * The run loop prints only when the guest does an OUT (direction == OUT) to the
 * agreed serial port. This folds the two conditions into one branchless-friendly
 * boolean, returned as 0/1. It is the pure form of the `if` in handle_io().
 * ABI: direction %edi, port %esi, console_port %edx; result (0/1) in %eax.
 * ------------------------------------------------------------------------- */
u32 serial_is_console_write(u32 direction, u32 port, u32 console_port)
{
    return (direction == KVM_EXIT_IO_OUT) & (port == console_port);
}

/* A decoded exit: the action to take plus, for a stop, whether it was an error.
 * Two u32s = 8 bytes, which the SysV ABI returns PACKED IN A SINGLE register
 * (%rax) — no hidden pointer, no memory round-trip. The annotated asm points out
 * exactly where the compiler shifts `is_error` into the high 32 bits of %rax. */
struct exit_decision {
    u32 action;     /* one of enum vmm_action                                */
    u32 is_error;   /* 1 iff action == ACT_STOP_ERR, precomputed for callers  */
};

/* -------------------------------------------------------------------------
 * decode_exit — classify an exit AND flag whether it is the error kind, in one
 * shot. This is what a run loop wants: a single value it can branch on. The two
 * fields come back in one register because the struct is <= 16 bytes.
 * ABI: reason %edi; the packed struct returns in %rax.
 * ------------------------------------------------------------------------- */
struct exit_decision decode_exit(u32 reason)
{
    enum vmm_action a = kvm_exit_action(reason);
    struct exit_decision d;
    d.action   = (u32)a;
    d.is_error = (a == ACT_STOP_ERR);
    return d;
}

/* -------------------------------------------------------------------------
 * demo_selftest — prove the dispatch is right without any kernel. Returns 0 on
 * success, or a small nonzero code naming the first failing check (run under a
 * debugger or inspect `echo $?`). Scaffolding, not core logic — kept out of the
 * annotated walkthrough. The optimizer folds every check over constant inputs,
 * so at -O2 this collapses to `xor %eax,%eax` (return 0); that erasure is itself
 * a lesson worth seeing in demo.O2.s.
 * ------------------------------------------------------------------------- */
int demo_selftest(void)
{
    /* the common path */
    if (kvm_exit_action(KVM_EXIT_IO)   != ACT_EMULATE_IO)   return 1;
    if (kvm_exit_action(KVM_EXIT_MMIO) != ACT_EMULATE_MMIO) return 2;
    if (kvm_exit_action(KVM_EXIT_HLT)  != ACT_STOP_OK)      return 3;

    /* all three fatal reasons collapse to one action */
    if (kvm_exit_action(KVM_EXIT_SHUTDOWN)       != ACT_STOP_ERR) return 4;
    if (kvm_exit_action(KVM_EXIT_FAIL_ENTRY)     != ACT_STOP_ERR) return 5;
    if (kvm_exit_action(KVM_EXIT_INTERNAL_ERROR) != ACT_STOP_ERR) return 6;

    /* transient reasons ask for a plain re-enter */
    if (kvm_exit_action(KVM_EXIT_INTR)            != ACT_REENTER) return 7;
    if (kvm_exit_action(KVM_EXIT_IRQ_WINDOW_OPEN) != ACT_REENTER) return 8;

    /* an unmodeled reason is surfaced, not silently swallowed */
    if (kvm_exit_action(KVM_EXIT_HYPERCALL) != ACT_UNHANDLED) return 9;

    /* batch length and console decode */
    if (io_batch_bytes(4, 8) != 32) return 10;         /* 4 bytes * 8 data     */
    if (!serial_is_console_write(KVM_EXIT_IO_OUT, 0x3f8, 0x3f8)) return 11;
    if ( serial_is_console_write(KVM_EXIT_IO_IN,  0x3f8, 0x3f8)) return 12; /* IN */
    if ( serial_is_console_write(KVM_EXIT_IO_OUT, 0x2f8, 0x3f8)) return 13; /* COM2 */

    /* the packed decision */
    struct exit_decision d = decode_exit(KVM_EXIT_SHUTDOWN);
    if (d.action != ACT_STOP_ERR || d.is_error != 1) return 14;

    return 0;
}
