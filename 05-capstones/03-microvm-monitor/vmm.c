/* ===========================================================================
 * vmm.c — /dev/kvm lifecycle, the KVM_RUN loop, and MMIO/PIO dispatch to the
 *         virtio-mmio bus. The monitor's engine.
 * ===========================================================================
 *
 * This file brings a microVM into existence and reacts to every reason the guest
 * hands control back. The mental model to hold: a VM exit is a synchronous,
 * blocking call from the guest into us. KVM_RUN does not return until the guest
 * needs something; run->exit_reason says why. For a microVM whose only devices
 * are virtio-mmio, the HOT exit is KVM_EXIT_MMIO — every virtqueue kick is a
 * write to a device's QueueNotify register in the unbacked MMIO window, which
 * faults here and we route to virtio.c.
 * =========================================================================== */

#include "vmm.h"

#include <stdio.h>      /* fprintf, fwrite, printf                             */
#include <stdlib.h>     /* exit, EXIT_FAILURE                                  */
#include <string.h>     /* memset                                              */
#include <errno.h>      /* errno, EINTR                                        */
#include <fcntl.h>      /* open, O_RDWR, O_CLOEXEC                             */
#include <unistd.h>     /* close                                              */
#include <sys/ioctl.h>  /* ioctl — the ONE syscall the whole KVM API rides on  */
#include <sys/mman.h>   /* mmap, munmap, PROT_*, MAP_*                         */

/* die — print context + strerror(errno) and abort. Teaching code fails loudly;
 * every ioctl below is checked and routed here so you never chase a silent -1. */
static void die(const char *what)
{
    perror(what);
    exit(EXIT_FAILURE);
}

/* ---------------------------------------------------------------------------
 * gpa_to_host — the trust boundary (declared in vmm.h). Translate a guest
 * physical address + length to a host pointer into guest RAM, or NULL if the
 * range is not fully backed. The two checks below are the whole point:
 *   - gpa < mem_size AND gpa+len <= mem_size  keeps us inside the slot;
 *   - the (gpa + len < gpa) guard rejects a length that overflows uint64,
 *     which is how an attacker would try to wrap past the upper bound.
 * Guest-controlled descriptor addresses flow through here; a missing check is a
 * VM escape.
 * ------------------------------------------------------------------------- */
void *gpa_to_host(struct vm *vm, uint64_t gpa, uint64_t len)
{
    if (gpa >= vm->mem_size)            return NULL;   /* start past end of RAM     */
    if (len > vm->mem_size - gpa)       return NULL;   /* end past end (no overflow)*/
    return vm->mem + gpa;
}

/* kvm_exit_name — a KVM_EXIT_* code -> string, for readable diagnostics. */
const char *kvm_exit_name(uint32_t exit_reason)
{
    switch (exit_reason) {
    case KVM_EXIT_UNKNOWN:         return "UNKNOWN";
    case KVM_EXIT_EXCEPTION:       return "EXCEPTION";
    case KVM_EXIT_IO:              return "IO";
    case KVM_EXIT_HYPERCALL:       return "HYPERCALL";
    case KVM_EXIT_DEBUG:           return "DEBUG";
    case KVM_EXIT_HLT:             return "HLT";
    case KVM_EXIT_MMIO:            return "MMIO";
    case KVM_EXIT_IRQ_WINDOW_OPEN: return "IRQ_WINDOW_OPEN";
    case KVM_EXIT_SHUTDOWN:        return "SHUTDOWN";
    case KVM_EXIT_FAIL_ENTRY:      return "FAIL_ENTRY";
    case KVM_EXIT_INTR:            return "INTR";
    case KVM_EXIT_INTERNAL_ERROR:  return "INTERNAL_ERROR";
    default:                       return "?";
    }
}

/* ---------------------------------------------------------------------------
 * vm_create — the "bring up a VM" sequence, ioctl by ioctl. Same six-step spine
 * as the sibling 01-kernel/15-kvm-hypervisor (open /dev/kvm; check API version;
 * KVM_CREATE_VM; map + register RAM; KVM_CREATE_VCPU; mmap kvm_run). The comments
 * there go deeper on each ioctl; here we keep it tight and note only what differs
 * for a microVM (bigger, though still tiny, and the MMIO window is deliberately
 * left UNREGISTERED so accesses to it trap).
 * ------------------------------------------------------------------------- */
void vm_create(struct vm *vm, size_t mem_size)
{
    memset(vm, 0, sizeof(*vm));
    vm->mem_size = mem_size;

    /* 1. Open the KVM subsystem. O_CLOEXEC so the fd never leaks across an exec. */
    vm->kvmfd = open("/dev/kvm", O_RDWR | O_CLOEXEC);
    if (vm->kvmfd < 0)
        die("open /dev/kvm (is the kvm module loaded? are you in group 'kvm'?)");

    /* 2. The KVM ABI version has been frozen at 12 for over a decade; a mismatch
     * means our headers and the kernel disagree about struct layouts. */
    int api = ioctl(vm->kvmfd, KVM_GET_API_VERSION, 0);
    if (api < 0) die("KVM_GET_API_VERSION");
    if (api != KVM_API_VERSION) {
        fprintf(stderr, "KVM API version %d, expected %d\n", api, KVM_API_VERSION);
        exit(EXIT_FAILURE);
    }

    /* 3. Create the VM; returns a new fd scoped to this VM. */
    vm->vmfd = ioctl(vm->kvmfd, KVM_CREATE_VM, 0);
    if (vm->vmfd < 0) die("KVM_CREATE_VM");

    /* 4. Guest RAM is a plain anonymous mmap in OUR address space; KVM installs
     * EPT/NPT so the guest sees it at GPA 0. We do NOT map or register the MMIO
     * window (GPA_MMIO_BASE): leaving it unbacked is exactly what makes a guest
     * access there exit to us as KVM_EXIT_MMIO. */
    vm->mem = mmap(NULL, mem_size, PROT_READ | PROT_WRITE,
                   MAP_ANONYMOUS | MAP_PRIVATE, -1, 0);
    if (vm->mem == MAP_FAILED) die("mmap guest RAM");

    struct kvm_userspace_memory_region region = {
        .slot            = 0,
        .flags           = 0,
        .guest_phys_addr = 0,                  /* guest RAM starts at GPA 0        */
        .memory_size     = mem_size,
        .userspace_addr  = (uint64_t)vm->mem,
    };
    if (ioctl(vm->vmfd, KVM_SET_USER_MEMORY_REGION, &region) < 0)
        die("KVM_SET_USER_MEMORY_REGION");

    /* 5. The single vCPU. */
    vm->vcpufd = ioctl(vm->vmfd, KVM_CREATE_VCPU, 0);
    if (vm->vcpufd < 0) die("KVM_CREATE_VCPU");

    /* 6. Map the kvm_run communication page. Its size comes from the kernel (it
     * may append per-exit scratch after the struct), and it MUST be MAP_SHARED so
     * the kernel and we see each other's writes to it on every exit. */
    int run_size = ioctl(vm->kvmfd, KVM_GET_VCPU_MMAP_SIZE, 0);
    if (run_size < 0) die("KVM_GET_VCPU_MMAP_SIZE");
    if ((size_t)run_size < sizeof(struct kvm_run)) {
        fprintf(stderr, "vcpu mmap size %d < sizeof(kvm_run) %zu\n",
                run_size, sizeof(struct kvm_run));
        exit(EXIT_FAILURE);
    }
    vm->run_size = (size_t)run_size;
    vm->run = mmap(NULL, vm->run_size, PROT_READ | PROT_WRITE,
                   MAP_SHARED, vm->vcpufd, 0);
    if (vm->run == MAP_FAILED) die("mmap kvm_run");
}

void vm_destroy(struct vm *vm)
{
    if (vm->run && vm->run != MAP_FAILED) munmap(vm->run, vm->run_size);
    if (vm->mem && vm->mem != MAP_FAILED) munmap(vm->mem, vm->mem_size);
    if (vm->vcpufd >= 0) close(vm->vcpufd);
    if (vm->vmfd   >= 0) close(vm->vmfd);
    if (vm->kvmfd  >= 0) close(vm->kvmfd);
}

/* ---------------------------------------------------------------------------
 * handle_mmio — service one KVM_EXIT_MMIO by routing it to the virtio bus.
 *
 * run->mmio gives the faulting guest-physical address, up to 8 bytes of data,
 * the length, and the direction. For a WRITE, run->mmio.data holds the bytes the
 * guest stored; for a READ, we must FILL run->mmio.data with what the device
 * returns before re-entering. This is the entire device bus: a fault address ->
 * a device -> a register.
 * ------------------------------------------------------------------------- */
static void handle_mmio(struct vm *vm)
{
    struct kvm_run *run = vm->run;
    uint64_t gpa = run->mmio.phys_addr;
    uint32_t len = run->mmio.len;

    struct virtio_dev *dev = virtio_bus_find(vm, gpa);
    if (!dev) {
        /* An MMIO access with no device behind it. Report and float 0xff on reads
         * (what an ISA bus does when nothing drives the lines). */
        fprintf(stderr, "[vmm] MMIO %s to unmapped gpa=0x%llx len=%u\n",
                run->mmio.is_write ? "write" : "read",
                (unsigned long long)gpa, len);
        if (!run->mmio.is_write) memset(run->mmio.data, 0xff, len);
        return;
    }

    if (run->mmio.is_write) {
        /* Reassemble the little-endian value the guest wrote from the byte array. */
        uint64_t val = 0;
        for (uint32_t i = 0; i < len && i < 8; i++)
            val |= (uint64_t)run->mmio.data[i] << (8 * i);
        virtio_mmio_write(vm, dev, gpa, len, val);
    } else {
        uint64_t val = 0;
        virtio_mmio_read(vm, dev, gpa, len, &val);
        /* Scatter the returned value back into the little-endian byte array KVM
         * will copy into the guest's destination register. */
        for (uint32_t i = 0; i < len && i < 8; i++)
            run->mmio.data[i] = (uint8_t)(val >> (8 * i));
    }
}

/* ---------------------------------------------------------------------------
 * handle_io — service one KVM_EXIT_IO. We keep a legacy 16550-style serial port
 * at 0x3f8 (OUT prints a byte) purely for parity with the sibling project, so a
 * guest that prefers PIO to virtio still has a console. Our default guest uses
 * virtio + hlt and never comes here.
 * ------------------------------------------------------------------------- */
static void handle_io(struct vm *vm)
{
    struct kvm_run *run = vm->run;
    uint8_t *data = (uint8_t *)run + run->io.data_offset;   /* self-relative cursor */

    if (run->io.direction == KVM_EXIT_IO_OUT && run->io.port == SERIAL_PORT) {
        for (uint32_t i = 0; i < run->io.count; i++)
            fwrite(data + (size_t)i * run->io.size, 1, run->io.size, stdout);
        fflush(stdout);
        return;
    }
    if (run->io.direction == KVM_EXIT_IO_IN) {
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
 * vm_run_loop — KVM_RUN until the guest halts. The engine.
 *
 * Each iteration: ioctl(KVM_RUN) enters guest mode and BLOCKS until the guest
 * exits; then we switch on run->exit_reason. EINTR/KVM_EXIT_INTR just re-enter
 * (that is the mechanism a real VMM uses to break out and inject an interrupt).
 * ------------------------------------------------------------------------- */
int vm_run_loop(struct vm *vm)
{
    for (;;) {
        int rc = ioctl(vm->vcpufd, KVM_RUN, 0);
        if (rc < 0) {
            if (errno == EINTR) continue;   /* host signal; re-run                 */
            die("KVM_RUN");
        }

        switch (vm->run->exit_reason) {
        case KVM_EXIT_MMIO:                 /* the virtio kick path — the hot case  */
            handle_mmio(vm);
            break;
        case KVM_EXIT_IO:                   /* legacy serial fallback               */
            handle_io(vm);
            break;
        case KVM_EXIT_HLT:                  /* guest said "done"                    */
            return 0;
        case KVM_EXIT_INTR:                 /* host signal fielded by KVM; re-enter */
            continue;
        case KVM_EXIT_FAIL_ENTRY:
            fprintf(stderr, "[vmm] FAIL_ENTRY: hardware reason 0x%llx\n",
                    (unsigned long long)vm->run->fail_entry.hardware_entry_failure_reason);
            return 1;
        case KVM_EXIT_INTERNAL_ERROR:
            fprintf(stderr, "[vmm] INTERNAL_ERROR: suberror %u\n",
                    vm->run->internal.suberror);
            return 1;
        case KVM_EXIT_SHUTDOWN:             /* triple fault: a guest fault with no IDT*/
            fprintf(stderr, "[vmm] guest SHUTDOWN (triple fault?)\n");
            return 1;
        default:
            fprintf(stderr, "[vmm] unexpected exit: %s (%u)\n",
                    kvm_exit_name(vm->run->exit_reason), vm->run->exit_reason);
            return 1;
        }
    }
}

/* ---------------------------------------------------------------------------
 * main — assemble the virtio-mmio bus, build the microVM, run the guest.
 * ------------------------------------------------------------------------- */
int main(void)
{
    /* The bus: three virtio-mmio devices, each in its own VIRTIO_MMIO_STRIDE
     * window. Only the console is fully modeled; blk and net are honest stubs
     * that answer discovery/negotiation but move no real data. */
    struct virtio_dev devs[3];
    virtio_dev_init(&devs[0], VDEV_CONSOLE, GPA_MMIO_BASE + 0 * VIRTIO_MMIO_STRIDE);
    virtio_dev_init(&devs[1], VDEV_BLOCK,   GPA_MMIO_BASE + 1 * VIRTIO_MMIO_STRIDE);
    virtio_dev_init(&devs[2], VDEV_NET,     GPA_MMIO_BASE + 2 * VIRTIO_MMIO_STRIDE);

    struct vm vm;
    vm_create(&vm, GUEST_RAM_SIZE);   /* steps 1-6: fd, RAM, vCPU, kvm_run          */
    vm.devs = devs;
    vm.ndev = 3;

    printf("== microVM monitor: virtio-console over MMIO ==\n");
    printf("guest RAM: %u KiB at GPA 0; virtio-mmio bus at GPA 0x%llx\n",
           GUEST_RAM_SIZE / 1024u, (unsigned long long)GPA_MMIO_BASE);
    printf("devices: console@0x%llx  blk@0x%llx(stub)  net@0x%llx(stub)\n",
           (unsigned long long)devs[0].mmio_base,
           (unsigned long long)devs[1].mmio_base,
           (unsigned long long)devs[2].mmio_base);
    printf("---- guest console output ----\n");

    guest_setup(&vm);                 /* payload, page tables, GDT, boot_params, regs */
    int rc = vm_run_loop(&vm);        /* KVM_RUN until hlt                            */

    printf("\n---- end guest console ----\n");
    printf("virtio-console: %llu kick(s), %llu byte(s) printed, %llu buffer(s) used\n",
           devs[0].notifications, devs[0].bytes_out, devs[0].buffers_used);
    printf("guest exited: %s\n", rc == 0 ? "clean hlt" : "error");

    vm_destroy(&vm);
    return rc;
}
