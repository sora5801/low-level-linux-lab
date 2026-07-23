/* ===========================================================================
 * guest.c — the two guest payloads and the CPU state that boots each.
 * ===========================================================================
 *
 * vmm.c knows how to run a guest but not what a guest IS. Here we define two,
 * deliberately chosen to show the same VMM driving wildly different CPU modes:
 *
 *   guest_setup_real_mode() — a 16-bit REAL-MODE program, the mode an x86 boots
 *       in (it is 1978 forever until software says otherwise). No paging, no
 *       protection; a physical address is just CS*16 + IP. It computes one digit
 *       and prints it, showing how INITIAL REGISTER VALUES we set flow into the
 *       guest and out through a port.
 *
 *   guest_setup_long_mode() — a 64-bit LONG-MODE program. Reaching 64-bit mode
 *       on x86 requires a specific ritual: build page tables, enable PAE, set
 *       the LME/LMA bits in EFER, turn on paging and protection together, and
 *       hand the CPU a 64-bit code segment. We do all of that by writing control
 *       registers and segment descriptors through KVM_SET_SREGS — we get to
 *       *skip* the real-mode->protected->long bootstrap dance because KVM lets
 *       us set the architectural state directly. Then the guest loops over a
 *       string, OUTing each byte.
 *
 * Both talk to the outside world the only way a CPU with no drivers can: the
 * `out` instruction to a port, which becomes KVM_EXIT_IO in the run loop.
 *
 * The machine code is given as raw bytes with the mnemonic beside each, because
 * that IS the lesson — there is no assembler in the loop, the guest executes
 * exactly these bytes. asm/demo.c and its annotated .s show the disassembly
 * discipline in the other direction.
 * =========================================================================== */

#include "vmm.h"

#include <stdio.h>
#include <stdlib.h>
#include <string.h>     /* memcpy, memset                                      */
#include <sys/ioctl.h>

/* die() lives in vmm.c; re-declare the minimal helper here so guest.c is
 * self-contained without dragging a header just for one function. */
static void guest_die(const char *what)
{
    perror(what);
    exit(EXIT_FAILURE);
}

/* ===========================================================================
 * REAL MODE
 * =========================================================================== */

/* The 16-bit payload. This is the canonical KVM "hello": it adds two registers
 * we seed from the host, turns the sum into an ASCII digit, and OUTs it, then a
 * newline, then halts. Every byte is one real instruction:
 *
 *   ba f8 03     mov  $0x3f8, %dx     ; dx = SERIAL_PORT (the OUT destination)
 *   00 d8        add  %bl, %al        ; al = al + bl  (we seed al=2, bl=2 -> 4)
 *   04 30        add  $'0', %al       ; 4 + 0x30('0') -> 0x34 = ASCII '4'
 *   ee           out  %al, (%dx)      ; port write -> KVM_EXIT_IO, prints '4'
 *   b0 0a        mov  $0x0a, %al      ; al = '\n'
 *   ee           out  %al, (%dx)      ; prints newline
 *   f4           hlt                  ; -> KVM_EXIT_HLT, ends the run loop
 *
 * The output is therefore the single line "4". The value is not baked into the
 * code — it is (al+bl) computed at run time from registers the HOST chose, which
 * is the whole point: it proves our KVM_SET_REGS actually reached the guest CPU.
 */
static const uint8_t real_mode_code[] = {
    0xba, 0xf8, 0x03,   /* mov  $0x3f8, %dx   */
    0x00, 0xd8,         /* add  %bl, %al      */
    0x04, '0',          /* add  $'0', %al     */
    0xee,               /* out  %al, (%dx)    */
    0xb0, '\n',         /* mov  $'\n', %al    */
    0xee,               /* out  %al, (%dx)    */
    0xf4,               /* hlt                */
};

/* We load the real-mode code at guest-physical 0x1000. Why not 0? Guest-physical
 * 0 is where the real-mode interrupt vector table lives on a real PC; keeping
 * off it is habit. 0x1000 is one page in and comfortably clear. */
#define REAL_MODE_ENTRY 0x1000

void guest_setup_real_mode(struct vm *vm)
{
    /* --- Place the code in guest RAM ------------------------------------ */
    /* vm->mem is host VA of guest-physical 0, so guest-physical 0x1000 is at
     * vm->mem + 0x1000. The guest will fetch its first instruction from there
     * once we point CS:IP at it. */
    memcpy(vm->mem + REAL_MODE_ENTRY, real_mode_code, sizeof(real_mode_code));

    /* --- Fix up the segment registers (KVM_GET/SET_SREGS) --------------- */
    /* A freshly created vCPU resets like a real CPU: CS.selector=0xF000 with a
     * hidden CS.base of 0xFFFF0000 and IP=0xFFF0 — i.e. it wants to run the BIOS
     * at the top of the 4 GiB space. We have no BIOS. So we read the whole sregs
     * block, then zero CS so that CS.base=0 and a physical address is simply the
     * offset. We only need to touch CS; DS/ES/SS already reset to base 0. */
    struct kvm_sregs sregs;
    if (ioctl(vm->vcpufd, KVM_GET_SREGS, &sregs) < 0)
        guest_die("KVM_GET_SREGS (real)");

    sregs.cs.base     = 0;      /* physical = CS.base + IP; base 0 => phys = IP */
    sregs.cs.selector = 0;      /* the visible selector, cosmetic in real mode  */

    if (ioctl(vm->vcpufd, KVM_SET_SREGS, &sregs) < 0)
        guest_die("KVM_SET_SREGS (real)");

    /* --- Set the general registers and the entry point (KVM_SET_REGS) --- */
    /* rflags bit 1 is reserved and MUST be 1 for a valid flags image; entering
     * the guest with it clear is a classic KVM_EXIT_FAIL_ENTRY. rip is the
     * offset the CPU fetches from (CS.base is 0, so rip == physical address).
     * rax/rbx are the operands our code adds — change them and the printed digit
     * changes, which is a nice thing to try. */
    struct kvm_regs regs;
    memset(&regs, 0, sizeof(regs));
    regs.rip    = REAL_MODE_ENTRY;   /* first instruction to fetch             */
    regs.rax    = 2;                 /* al = 2                                 */
    regs.rbx    = 2;                 /* bl = 2  -> al+bl = 4 -> prints '4'      */
    regs.rflags = 0x2;               /* reserved bit 1 set; everything else off */

    if (ioctl(vm->vcpufd, KVM_SET_REGS, &regs) < 0)
        guest_die("KVM_SET_REGS (real)");
}

/* ===========================================================================
 * LONG MODE
 * ===========================================================================
 *
 * Reaching 64-bit mode means satisfying the CPU's checklist all at once:
 *   - a set of 4-level page tables the hardware can walk (paging is MANDATORY
 *     in long mode — there is no unpaged 64-bit mode);
 *   - CR4.PAE = 1 (long mode requires the PAE page-table format);
 *   - EFER.LME = 1 (Long Mode Enable) and EFER.LMA = 1 (Long Mode Active);
 *   - CR0.PG = 1 and CR0.PE = 1 (paging + protection on, together);
 *   - a code segment with the L (long) bit set.
 * On real hardware you toggle these in a careful order across a far jump; with
 * KVM we set the final architectural state directly and let VMENTRY validate it.
 */

/* Page-table entry flag bits (identical meaning at every level of the walk). */
#define PDE64_PRESENT (1u << 0)   /* entry is valid; absent => #PF             */
#define PDE64_RW      (1u << 1)   /* writable (else read-only)                 */
#define PDE64_USER    (1u << 2)   /* user-accessible (we run at CPL 0 anyway)  */
#define PDE64_PS      (1u << 7)   /* Page Size: this entry maps a big page...  */
                                  /* ...at the PD level, a 2 MiB page directly */

/* Control-register / EFER bits we assert to be in 64-bit mode. */
#define CR0_PE (1u << 0)          /* Protected Mode Enable                     */
#define CR0_MP (1u << 1)          /* Monitor Coprocessor (sane FPU default)    */
#define CR0_ET (1u << 4)          /* Extension Type (386-era, reads as 1)      */
#define CR0_NE (1u << 5)          /* Numeric Error (native FPU exceptions)     */
#define CR0_WP (1u << 16)         /* Write Protect (honor RO pages at CPL 0)   */
#define CR0_AM (1u << 18)         /* Alignment Mask                            */
#define CR0_PG (1u << 31)         /* Paging Enable — the big one               */
#define CR4_PAE (1u << 5)         /* Physical Address Extension                */
#define EFER_LME (1u << 8)        /* Long Mode Enable                          */
#define EFER_LMA (1u << 10)       /* Long Mode Active (set by CPU; we mirror)  */

/* Guest-physical addresses for the three page-table levels we build. They sit
 * above the code (at 0) and the string (at 0x1000) but inside guest RAM, and on
 * 0x1000 boundaries because page-table base addresses must be page-aligned. */
#define PML4_ADDR 0x2000          /* top level (points at one PDPT)            */
#define PDPT_ADDR 0x3000          /* points at one PD                          */
#define PD_ADDR   0x4000          /* one 2 MiB entry maps all of guest RAM     */

/* Where the string to print lives, and where the code starts. */
#define LONG_STR_ADDR 0x1000
#define LONG_CODE_ENTRY 0x0

/* The 64-bit payload: walk a NUL-terminated string at %rsi and OUT each byte to
 * dx=SERIAL_PORT, then hlt. Hand-assembled; offsets on the right are the guest-
 * physical address of each instruction (entry is 0), which is how the two short
 * jumps below get their displacements.
 *
 *  off  bytes            instruction              meaning
 *  0    66 ba f8 03      mov  $0x3f8, %dx         dx = SERIAL_PORT
 *  4    be 00 10 00 00   mov  $0x1000, %esi       esi = &string (zero-extends
 *                                                  to rsi; the string is at GPA
 *                                                  0x1000 = LONG_STR_ADDR)
 *  9    .loop:
 *  9    8a 06            mov  (%rsi), %al         al = *rsi (next char)
 *  11   84 c0            test %al, %al            set ZF if al == 0 (NUL)
 *  13   74 06            je   .done               if NUL, jump to hlt (+6)
 *  15   ee               out  %al, (%dx)          print al -> KVM_EXIT_IO
 *  16   48 ff c6         inc  %rsi                advance the pointer
 *  19   eb f4            jmp  .loop               back 12 bytes to .loop
 *  21   .done:
 *  21   f4               hlt                      -> KVM_EXIT_HLT
 */
static const uint8_t long_mode_code[] = {
    0x66, 0xba, 0xf8, 0x03,       /* mov  $0x3f8, %dx      */
    0xbe, 0x00, 0x10, 0x00, 0x00, /* mov  $0x1000, %esi    */
    /* .loop: */
    0x8a, 0x06,                   /* mov  (%rsi), %al      */
    0x84, 0xc0,                   /* test %al, %al         */
    0x74, 0x06,                   /* je   .done            */
    0xee,                         /* out  %al, (%dx)       */
    0x48, 0xff, 0xc6,             /* inc  %rsi             */
    0xeb, 0xf4,                   /* jmp  .loop            */
    /* .done: */
    0xf4,                         /* hlt                   */
};

static const char long_mode_string[] = "Hello from 64-bit long mode via OUT 0x3f8!\n";

/* Fill one kvm_segment as a flat 64-bit descriptor. `code` picks the type: a
 * long-mode code segment (L=1, type=execute/read) or a data segment (type=
 * read/write). base=0, limit=4 GiB in 4 KiB units => the whole flat space. */
static struct kvm_segment flat_segment(uint16_t selector, int code)
{
    struct kvm_segment s;
    memset(&s, 0, sizeof(s));
    s.base     = 0;
    s.limit    = 0xffffffff;   /* with g=1, this is a 4 GiB span               */
    s.selector = selector;
    s.present  = 1;            /* segment is present (else #NP on load)         */
    s.dpl      = 0;            /* descriptor privilege level 0 (kernel)         */
    s.s        = 1;            /* 1 = code/data segment (not a system segment)  */
    s.g        = 1;            /* limit granularity = 4 KiB pages               */
    if (code) {
        s.type = 11;          /* 1011b: code, execute/read, accessed           */
        s.l    = 1;           /* LONG bit: this is a 64-bit code segment        */
        s.db   = 0;           /* MUST be 0 when L=1 (the CPU enforces this)     */
    } else {
        s.type = 3;           /* 0011b: data, read/write, accessed             */
        s.l    = 0;
        s.db   = 1;           /* 32-bit data segment default size              */
    }
    return s;
}

void guest_setup_long_mode(struct vm *vm)
{
    /* --- Lay down code and data in guest RAM ---------------------------- */
    memcpy(vm->mem + LONG_CODE_ENTRY, long_mode_code, sizeof(long_mode_code));
    memcpy(vm->mem + LONG_STR_ADDR, long_mode_string, sizeof(long_mode_string));

    /* --- Build the 4-level page tables (identity map) ------------------- */
    /* We want guest-virtual == guest-physical for all of RAM, so the code can
     * use flat addresses. The x86-64 walk is PML4 -> PDPT -> PD -> (page). By
     * setting PDE64_PS in the PD entry, that entry maps a single 2 MiB page
     * DIRECTLY, ending the walk one level early — which is exactly why we made
     * guest RAM 2 MiB: one PD entry covers all of it, so we need just three
     * 8-byte writes to map the entire guest.
     *
     * Each entry stores the physical address of the next table (or the final
     * page) OR-ed with permission flags in the low bits — the low 12 bits of a
     * page-aligned address are always 0, so the CPU masks them off and uses them
     * for flags. Get one flag wrong (forget PRESENT) and the first fetch is a
     * page fault with no IDT to catch it => triple fault => KVM_EXIT_SHUTDOWN. */
    uint64_t *pml4 = (uint64_t *)(vm->mem + PML4_ADDR);
    uint64_t *pdpt = (uint64_t *)(vm->mem + PDPT_ADDR);
    uint64_t *pd   = (uint64_t *)(vm->mem + PD_ADDR);

    pml4[0] = PDE64_PRESENT | PDE64_RW | PDPT_ADDR;   /* PML4[0] -> PDPT        */
    pdpt[0] = PDE64_PRESENT | PDE64_RW | PD_ADDR;     /* PDPT[0] -> PD          */
    pd[0]   = PDE64_PRESENT | PDE64_RW | PDE64_PS;    /* PD[0] -> 2MiB @ phys 0 */

    /* --- Put the CPU into long mode via sregs --------------------------- */
    struct kvm_sregs sregs;
    if (ioctl(vm->vcpufd, KVM_GET_SREGS, &sregs) < 0)
        guest_die("KVM_GET_SREGS (long)");

    /* CR3 holds the physical base of the top-level page table (PML4). The CPU
     * reloads its address translation from here the instant paging turns on. */
    sregs.cr3  = PML4_ADDR;
    sregs.cr4  = CR4_PAE;                                   /* PAE required     */
    sregs.cr0  = CR0_PE | CR0_MP | CR0_ET | CR0_NE | CR0_WP | CR0_AM | CR0_PG;
    sregs.efer = EFER_LME | EFER_LMA;                       /* long mode on     */

    /* Load every segment with a flat descriptor. CS gets the long-mode code
     * descriptor (L=1); the data/stack segments get a flat data descriptor.
     * Selectors 0x08 and 0x10 are conventional GDT indices 1 and 2; KVM uses
     * the cached descriptor fields we set here, so we do not need a real GDT in
     * memory for this to run. */
    sregs.cs = flat_segment(0x08, /*code=*/1);
    struct kvm_segment data = flat_segment(0x10, /*code=*/0);
    sregs.ds = sregs.es = sregs.fs = sregs.gs = sregs.ss = data;

    if (ioctl(vm->vcpufd, KVM_SET_SREGS, &sregs) < 0)
        guest_die("KVM_SET_SREGS (long)");

    /* --- General registers: entry point and a stack -------------------- */
    struct kvm_regs regs;
    memset(&regs, 0, sizeof(regs));
    regs.rip    = LONG_CODE_ENTRY;    /* start of our 64-bit code (phys 0)      */
    regs.rsp    = vm->mem_size;       /* stack top = end of RAM; grows downward */
    regs.rflags = 0x2;                /* reserved bit 1 set, as always          */

    if (ioctl(vm->vcpufd, KVM_SET_REGS, &regs) < 0)
        guest_die("KVM_SET_REGS (long)");
}
