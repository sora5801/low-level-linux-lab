/* ===========================================================================
 * vmm.c — /dev/kvm lifecycle and the KVM_RUN dispatch loop.
 * ===========================================================================
 *
 * This file is the "hypervisor engine": it knows how to bring a VM into
 * existence and how to react to every reason the guest hands control back. It
 * knows NOTHING about what the guest actually is — that lives in guest.c. Read
 * this file top to bottom and you have read a complete, if minimal, VMM.
 *
 * The one idea to keep in your head: a VM exit is a synchronous, blocking
 * function call from the guest into us. KVM_RUN does not return until the guest
 * needs something. When it returns, `vm->run->exit_reason` says why, and the
 * union inside kvm_run carries the details (which port, which address, how many
 * bytes). We service it and call KVM_RUN again, exactly like an event loop whose
 * events are "the guest tried to do X."
 * =========================================================================== */

#include "vmm.h"

#include <stdio.h>      /* fprintf, fwrite, perror                             */
#include <stdlib.h>     /* exit, EXIT_FAILURE                                  */
#include <string.h>     /* memset                                             */
#include <errno.h>      /* errno                                              */
#include <fcntl.h>      /* open, O_RDWR, O_CLOEXEC                             */
#include <unistd.h>     /* close, read                                        */
#include <sys/ioctl.h>  /* ioctl — the ONE syscall the entire KVM API rides on */
#include <sys/mman.h>   /* mmap, munmap, PROT_*, MAP_*                         */

/* ---------------------------------------------------------------------------
 * die — print context + strerror(errno) and abort. Teaching code fails LOUDLY
 * and immediately; every ioctl below is checked and routed here so you never
 * chase a silent -1. In production a VMM would unwind and report, not exit(3).
 * ------------------------------------------------------------------------- */
static void die(const char *what)
{
    /* perror prints "what: <errno string>", e.g. "KVM_CREATE_VM: Permission
     * denied", which is exactly the breadcrumb you want when an ioctl fails. */
    perror(what);
    exit(EXIT_FAILURE);
}

/* ---------------------------------------------------------------------------
 * kvm_exit_name — a KVM_EXIT_* code -> string, for readable diagnostics.
 *
 * This is the readable twin of the dispatch logic extracted into asm/demo.c.
 * We keep it a plain switch (not a table) so it stays obviously correct and so
 * the compiler is free to lay it out however it likes; the *asm* study of this
 * shape lives in asm/. Every arm names the real meaning of the exit so a
 * surprising exit in the loop below prints something you can grep the kernel
 * for.
 * ------------------------------------------------------------------------- */
const char *kvm_exit_name(uint32_t exit_reason)
{
    switch (exit_reason) {
    case KVM_EXIT_UNKNOWN:        return "UNKNOWN";        /* hw reason in .hw   */
    case KVM_EXIT_EXCEPTION:      return "EXCEPTION";
    case KVM_EXIT_IO:             return "IO";             /* port in/out        */
    case KVM_EXIT_HYPERCALL:      return "HYPERCALL";
    case KVM_EXIT_DEBUG:          return "DEBUG";          /* guest hit a bp     */
    case KVM_EXIT_HLT:            return "HLT";            /* guest executed hlt */
    case KVM_EXIT_MMIO:           return "MMIO";           /* access to no-RAM   */
    case KVM_EXIT_IRQ_WINDOW_OPEN:return "IRQ_WINDOW_OPEN";
    case KVM_EXIT_SHUTDOWN:       return "SHUTDOWN";       /* triple fault, etc. */
    case KVM_EXIT_FAIL_ENTRY:     return "FAIL_ENTRY";     /* VMENTRY refused    */
    case KVM_EXIT_INTR:           return "INTR";           /* host signal        */
    case KVM_EXIT_INTERNAL_ERROR: return "INTERNAL_ERROR";
    default:                      return "?";
    }
}

/* ---------------------------------------------------------------------------
 * vm_create — the full "bring up a VM" sequence, ioctl by ioctl.
 *
 * Each step below maps to exactly one KVM concept. The order matters: you need
 * the vmfd before you can give it memory or a vCPU, and you need the vcpufd and
 * the mmap size before you can share the kvm_run page.
 * ------------------------------------------------------------------------- */
void vm_create(struct vm *vm, size_t mem_size)
{
    memset(vm, 0, sizeof(*vm));
    vm->mem_size = mem_size;

    /* --- 1. Open the KVM subsystem -------------------------------------- */
    /* /dev/kvm is a character device (misc major 10). Opening it gives us the
     * "system" fd — the root of the whole ioctl tree. O_CLOEXEC so a future
     * exec() in a forked child does not leak the descriptor into the guest's
     * would-be helper processes (basic hygiene for anything that spawns). */
    vm->kvmfd = open("/dev/kvm", O_RDWR | O_CLOEXEC);
    if (vm->kvmfd < 0)
        die("open /dev/kvm (is the kvm module loaded? are you in group 'kvm'?)");

    /* --- 2. Sanity-check the ABI version -------------------------------- */
    /* KVM_GET_API_VERSION returns the stable ABI version. It has been 12 since
     * Linux 2.6.22; any other value means our <linux/kvm.h> and the running
     * kernel disagree about struct layouts, which would corrupt every ioctl.
     * We refuse to continue rather than scribble on mismatched structs. */
    int api = ioctl(vm->kvmfd, KVM_GET_API_VERSION, 0);
    if (api < 0)
        die("KVM_GET_API_VERSION");
    if (api != KVM_API_VERSION) {   /* KVM_API_VERSION is the 12 from the header */
        fprintf(stderr, "KVM API version %d, expected %d\n", api, KVM_API_VERSION);
        exit(EXIT_FAILURE);
    }

    /* --- 3. Create the VM ----------------------------------------------- */
    /* KVM_CREATE_VM allocates a struct kvm in the kernel and returns a NEW fd
     * scoped to this VM. The argument (0) selects the default machine type; on
     * some arches it encodes the guest physical address width. Everything about
     * memory and vCPUs is now addressed through vmfd, not kvmfd. */
    vm->vmfd = ioctl(vm->kvmfd, KVM_CREATE_VM, 0);
    if (vm->vmfd < 0)
        die("KVM_CREATE_VM");

    /* --- 4. Allocate guest physical RAM --------------------------------- */
    /* The guest's "physical memory" is just an ordinary anonymous mmap in OUR
     * address space. MAP_ANONYMOUS => zero-filled, not backed by a file.
     * PROT_READ|PROT_WRITE because the guest (and we) both read and write it.
     * We do NOT need PROT_EXEC: the *host* CPU never fetches instructions from
     * here; only the *guest* CPU does, and guest execute permission is governed
     * by the guest's own page tables, not this host mapping. */
    vm->mem = mmap(NULL, mem_size, PROT_READ | PROT_WRITE,
                   MAP_ANONYMOUS | MAP_PRIVATE, -1, 0);
    if (vm->mem == MAP_FAILED)
        die("mmap guest RAM");

    /* Now tell KVM: "this host memory region IS the guest's physical RAM
     * starting at guest-physical address 0." KVM installs shadow/EPT page
     * tables so that when the guest dereferences GPA X, the hardware walks to
     * host VA (userspace_addr + X). The invariant that makes this sound:
     * userspace_addr must stay mapped and must not be freed while the region is
     * registered — if we munmap it out from under a running guest, the next
     * guest access faults into the kernel with nowhere to go. We honor that by
     * only unmapping in vm_destroy, after the run loop has ended. */
    struct kvm_userspace_memory_region region = {
        .slot            = 0,                    /* memory slot #; we use one    */
        .flags           = 0,                    /* no dirty logging / read-only */
        .guest_phys_addr = 0,                    /* guest sees RAM at GPA 0      */
        .memory_size     = mem_size,             /* must be page-multiple        */
        .userspace_addr  = (uint64_t)vm->mem,    /* host VA backing the slot     */
    };
    if (ioctl(vm->vmfd, KVM_SET_USER_MEMORY_REGION, &region) < 0)
        die("KVM_SET_USER_MEMORY_REGION");

    /* --- 5. Create the single virtual CPU ------------------------------- */
    /* KVM_CREATE_VCPU adds vCPU index 0 to the VM and returns yet another fd.
     * Each vCPU is a schedulable thread of guest execution; a real SMP guest
     * would create several and KVM_RUN each on its own host thread. We keep one
     * to stay legible. On success the vCPU resets to the x86 architectural
     * power-on state (real mode, CS:IP = F000:FFF0) — guest.c overrides that. */
    vm->vcpufd = ioctl(vm->vmfd, KVM_CREATE_VCPU, 0);
    if (vm->vcpufd < 0)
        die("KVM_CREATE_VCPU");

    /* --- 6. Map the kvm_run communication page -------------------------- */
    /* KVM shares per-vCPU state with us through a memory page we mmap over the
     * vcpufd. Its size is not sizeof(struct kvm_run) — the kernel may append
     * per-exit scratch space (e.g. the bytes of a large PIO string) after the
     * struct — so we must ASK for the size with KVM_GET_VCPU_MMAP_SIZE and map
     * exactly that. Querying it on the *system* fd (kvmfd) is correct: the size
     * is a property of the kernel build, identical for every vCPU. */
    int run_size = ioctl(vm->kvmfd, KVM_GET_VCPU_MMAP_SIZE, 0);
    if (run_size < 0)
        die("KVM_GET_VCPU_MMAP_SIZE");
    if ((size_t)run_size < sizeof(struct kvm_run)) {
        fprintf(stderr, "vcpu mmap size %d < sizeof(kvm_run) %zu\n",
                run_size, sizeof(struct kvm_run));
        exit(EXIT_FAILURE);
    }
    vm->run_size = (size_t)run_size;

    /* MAP_SHARED is mandatory here (unlike the guest RAM above): the kernel and
     * we must see each other's writes to this page. The kernel writes
     * exit_reason and the exit union before returning from KVM_RUN; we write
     * fields like io data for an IN before re-entering. A private (COW) mapping
     * would silently break that two-way channel. Offset 0 selects the kvm_run
     * page (nonzero offsets select coalesced-MMIO rings we do not use). */
    vm->run = mmap(NULL, vm->run_size, PROT_READ | PROT_WRITE,
                   MAP_SHARED, vm->vcpufd, 0);
    if (vm->run == MAP_FAILED)
        die("mmap kvm_run");
}

/* ---------------------------------------------------------------------------
 * vm_destroy — tear everything down in the reverse order it was built.
 *
 * Closing the fds is what actually frees the in-kernel VM/vCPU objects (they
 * are reference-counted by their file descriptors). We munmap first so no
 * mapping outlives the object it refers to.
 * ------------------------------------------------------------------------- */
void vm_destroy(struct vm *vm)
{
    if (vm->run && vm->run != MAP_FAILED)
        munmap(vm->run, vm->run_size);      /* release the shared kvm_run page  */
    if (vm->mem && vm->mem != MAP_FAILED)
        munmap(vm->mem, vm->mem_size);      /* release guest RAM (now safe)     */
    if (vm->vcpufd >= 0) close(vm->vcpufd); /* drops the vCPU                   */
    if (vm->vmfd   >= 0) close(vm->vmfd);   /* drops the VM                     */
    if (vm->kvmfd  >= 0) close(vm->kvmfd);  /* drops our handle on KVM          */
}

/* ---------------------------------------------------------------------------
 * handle_io — service one KVM_EXIT_IO.
 *
 * When the guest runs IN/OUT on a port KVM does not itself emulate, KVM stops
 * the guest and returns here with run->io filled in:
 *
 *   .direction   KVM_EXIT_IO_OUT (guest -> device) or KVM_EXIT_IO_IN (device ->
 *                guest). For OUT we read the bytes; for IN we must PROVIDE them.
 *   .size        bytes per datum (1/2/4).
 *   .count       number of data (a "rep outs/ins" can move many at once).
 *   .port        the 16-bit port number.
 *   .data_offset BYTE offset, from the start of the kvm_run page, of the data
 *                buffer. Crucially it is an offset, not a pointer — the kernel
 *                cannot hand us one of its own addresses, so it hands us a
 *                self-relative cursor into the page we both map. We turn it back
 *                into a pointer by adding it to (uint8_t *)run. That pointer
 *                arithmetic is the single most bug-prone line in a VMM: get the
 *                base wrong and you read kernel-adjacent bytes.
 *
 * We implement exactly one device: a write-only serial console at SERIAL_PORT.
 * An OUT there prints; anything else we report and ignore (a real VMM would
 * route to a device model or return 0xff for unhandled reads).
 * ------------------------------------------------------------------------- */
static void handle_io(struct vm *vm)
{
    struct kvm_run *run = vm->run;

    /* Reconstruct the data pointer from the self-relative offset (see above). */
    uint8_t *data = (uint8_t *)run + run->io.data_offset;

    if (run->io.direction == KVM_EXIT_IO_OUT && run->io.port == SERIAL_PORT) {
        /* Print `count` data of `size` bytes each. Our guests use size==1, so
         * this is "print count characters", but we honor the general shape:
         * datum i lives at data + i*size, low byte first. */
        for (uint32_t i = 0; i < run->io.count; i++)
            fwrite(data + (size_t)i * run->io.size, 1, run->io.size, stdout);
        fflush(stdout);
        return;
    }

    if (run->io.direction == KVM_EXIT_IO_IN) {
        /* We model no readable device, so feed the guest all-ones (0xff), the
         * value a real ISA bus floats when nothing drives it. We WRITE into the
         * shared page; KVM copies it into the guest's AL/AX/EAX on re-entry. */
        for (uint32_t i = 0; i < run->io.count; i++)
            for (uint32_t b = 0; b < run->io.size; b++)
                data[(size_t)i * run->io.size + b] = 0xff;
        return;
    }

    fprintf(stderr, "[vmm] unhandled IO %s port 0x%x size %u count %u\n",
            run->io.direction == KVM_EXIT_IO_OUT ? "OUT" : "IN",
            run->io.port, run->io.size, run->io.count);
}

/* ---------------------------------------------------------------------------
 * handle_mmio — service one KVM_EXIT_MMIO.
 *
 * MMIO is the memory-mapped cousin of PIO: the guest touched a GUEST-PHYSICAL
 * address that is NOT backed by any registered memory slot, so KVM cannot let
 * the hardware complete the access and bounces it to us. run->mmio gives the
 * physical address, up to 8 bytes of data, the length, and whether it was a
 * write. This is exactly how virtio-mmio, an APIC, or a framebuffer would be
 * wired. We have no MMIO devices, so we just report — but the plumbing is here
 * so you can see MMIO and PIO are the same idea through two different faults.
 * ------------------------------------------------------------------------- */
static void handle_mmio(struct vm *vm)
{
    struct kvm_run *run = vm->run;
    fprintf(stderr, "[vmm] MMIO %s gpa=0x%llx len=%u\n",
            run->mmio.is_write ? "write" : "read",
            (unsigned long long)run->mmio.phys_addr, run->mmio.len);
    if (!run->mmio.is_write)
        memset(run->mmio.data, 0xff, run->mmio.len);  /* float 0xff on reads */
}

/* ---------------------------------------------------------------------------
 * vm_run_loop — KVM_RUN until the guest halts. THE hypervisor loop.
 *
 * Everything above was setup; this is the engine. Each iteration:
 *   - ioctl(KVM_RUN) enters guest mode and BLOCKS this thread until the guest
 *     exits. On return, the CPU's guest state is saved and run->exit_reason
 *     tells us why we are back. KVM_RUN takes no argument (the 0 is ignored);
 *     all input/output flows through the shared kvm_run page.
 *   - We dispatch on exit_reason. The two we truly service are IO and HLT;
 *     the rest we surface honestly.
 *
 * EINTR handling: a host signal (e.g. SIGALRM, or the scheduler delivering a
 * timer) can interrupt KVM_RUN, which returns -1/EINTR. That is not an error —
 * it is the mechanism a real VMM uses to break OUT of the guest to, say,
 * inject an interrupt. We simply re-enter. (KVM also reports KVM_EXIT_INTR for
 * signals it fields internally; we treat that as "re-run" too.)
 * ------------------------------------------------------------------------- */
int vm_run_loop(struct vm *vm)
{
    for (;;) {
        int rc = ioctl(vm->vcpufd, KVM_RUN, 0);
        if (rc < 0) {
            if (errno == EINTR)
                continue;               /* interrupted by a host signal; re-run */
            die("KVM_RUN");
        }

        switch (vm->run->exit_reason) {
        case KVM_EXIT_IO:
            handle_io(vm);              /* print a byte, or float 0xff on a read */
            break;

        case KVM_EXIT_MMIO:
            handle_mmio(vm);            /* no device model, but shows the path   */
            break;

        case KVM_EXIT_HLT:
            /* The guest executed `hlt`. For a real CPU that means "idle until an
             * interrupt"; for our tiny guests it is the agreed "I'm done" signal.
             * Clean, expected termination of the run loop. */
            return 0;

        case KVM_EXIT_INTR:
            continue;                   /* host signal fielded by KVM; re-enter  */

        case KVM_EXIT_FAIL_ENTRY:
            /* The CPU refused to ENTER the guest. hardware_entry_failure_reason
             * is the raw VMX/SVM error code — almost always a malformed guest
             * state we set up (bad control regs, unusable segment). This is the
             * exit you hit while learning to build the long-mode tables. */
            fprintf(stderr, "[vmm] FAIL_ENTRY: hardware reason 0x%llx\n",
                    (unsigned long long)
                    vm->run->fail_entry.hardware_entry_failure_reason);
            return 1;

        case KVM_EXIT_INTERNAL_ERROR:
            fprintf(stderr, "[vmm] INTERNAL_ERROR: suberror %u\n",
                    vm->run->internal.suberror);
            return 1;

        case KVM_EXIT_SHUTDOWN:
            /* A triple fault (fault while handling a fault while handling a
             * fault) resets a real CPU; under KVM it surfaces as SHUTDOWN. In a
             * guest we wrote, it means our code faulted with no IDT to catch it. */
            fprintf(stderr, "[vmm] guest SHUTDOWN (triple fault?)\n");
            return 1;

        default:
            fprintf(stderr, "[vmm] unexpected exit: %s (%u)\n",
                    kvm_exit_name(vm->run->exit_reason),
                    vm->run->exit_reason);
            return 1;
        }
    }
}

/* ---------------------------------------------------------------------------
 * main — run the two examples: 16-bit real mode, then 64-bit long mode.
 *
 * We build a fresh VM for each so neither example can see the other's memory or
 * register state — the cleanest way to show two very different guests through
 * the identical run loop above.
 * ------------------------------------------------------------------------- */
static int run_example(const char *label, void (*setup)(struct vm *))
{
    struct vm vm;
    printf("== %s ==\n", label);
    vm_create(&vm, GUEST_MEM_SIZE);   /* steps 1-6: fd, RAM, vCPU, kvm_run     */
    setup(&vm);                       /* load the payload, set initial regs    */
    int rc = vm_run_loop(&vm);        /* KVM_RUN until hlt                      */
    vm_destroy(&vm);                  /* close fds, unmap memory               */
    printf("\n-- guest exited (%s) --\n\n", rc == 0 ? "clean hlt" : "error");
    return rc;
}

int main(int argc, char **argv)
{
    /* Optional arg selects one example; default runs both. Kept trivial so the
     * focus stays on KVM, not argument parsing. */
    const char *which = (argc > 1) ? argv[1] : "both";
    int rc = 0;

    if (strcmp(which, "real") && strcmp(which, "long") && strcmp(which, "both")) {
        fprintf(stderr, "usage: %s [real|long|both]\n", argv[0]);
        return 2;
    }

    if (!strcmp(which, "real") || !strcmp(which, "both"))
        rc |= run_example("real mode: compute a digit, OUT to serial",
                          guest_setup_real_mode);

    if (!strcmp(which, "long") || !strcmp(which, "both"))
        rc |= run_example("long mode: print a string, OUT to serial",
                          guest_setup_long_mode);

    return rc;
}
