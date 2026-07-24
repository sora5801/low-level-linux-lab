/* ===========================================================================
 * idt.c — building the 256-gate IDT and pointing the CPU at it with `lidt`.
 * ===========================================================================
 *
 * The plan: fill all 256 descriptors from a table of stub entry points that
 * isr.S exports (isr_stub_table[]). Vectors 0..31 are the CPU exception stubs;
 * 32..47 are the remapped hardware IRQ stubs; the rest point at a catch-all
 * stub so a stray vector is handled rather than triple-faulting the machine.
 * Every gate uses selector 0x08 (our 64-bit kernel code segment from the boot
 * GDT) and type 0x8E (present, DPL 0, 64-bit interrupt gate — "interrupt" gate,
 * as opposed to a "trap" gate, means the CPU clears IF on entry so a handler is
 * not itself interrupted).
 * =========================================================================== */
#include "idt.h"

#define IDT_ENTRIES 256
#define KERNEL_CS   0x08    /* selector of the 64-bit code segment in the GDT */
#define IDT_GATE    0x8E    /* P=1, DPL=00, type=0xE (64-bit interrupt gate)  */

/* The table the CPU reads, and the pointer we feed to `lidt`. Marked so the
 * table is 16-byte aligned, which keeps each 16-byte gate on its own line and
 * costs nothing. */
static struct idt_entry idt[IDT_ENTRIES] __attribute__((aligned(16)));
static struct idt_ptr   idtr;

/* isr.S exports one entry point per vector, gathered into this array of code
 * addresses (see the `.quad` table at the bottom of isr.S). Declaring it as an
 * array of uintptr_t lets us index it without 256 individual `extern`s. */
extern const uintptr_t isr_stub_table[IDT_ENTRIES];

/* Write one gate: scatter the 64-bit handler address across the three offset
 * fields exactly where the CPU expects to reassemble it. */
static void idt_set_gate(int vec, uintptr_t handler)
{
    idt[vec].offset_low  = (uint16_t)(handler & 0xFFFF);
    idt[vec].selector    = KERNEL_CS;
    idt[vec].ist         = 0;            /* no separate interrupt stack for now */
    idt[vec].type_attr   = IDT_GATE;
    idt[vec].offset_mid  = (uint16_t)((handler >> 16) & 0xFFFF);
    idt[vec].offset_high = (uint32_t)((handler >> 32) & 0xFFFFFFFF);
    idt[vec].zero        = 0;
}

void idt_init(void)
{
    for (int v = 0; v < IDT_ENTRIES; v++)
        idt_set_gate(v, isr_stub_table[v]);

    /* limit = table size minus one, per the `lidt` operand definition. base is
     * the linear address of the table (identity-mapped, so == its C address). */
    idtr.limit = (uint16_t)(sizeof(idt) - 1);
    idtr.base  = (uint64_t)(uintptr_t)&idt[0];

    /* Load the IDT register. `m` hands the assembler the 10-byte idtr operand;
     * from here on any interrupt or exception dispatches through our table. */
    __asm__ volatile("lidt %0" : : "m"(idtr));
}
