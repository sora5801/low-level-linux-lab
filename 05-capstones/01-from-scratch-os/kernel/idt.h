/* ===========================================================================
 * idt.h — the 64-bit Interrupt Descriptor Table + the saved-register frame.
 * ===========================================================================
 * The IDT is a 256-entry table telling the CPU, "when interrupt vector N fires,
 * jump to THIS handler with THESE privileges." In long mode each entry is 16
 * bytes (double the 32-bit size, because the handler address is now 64 bits).
 * =========================================================================== */
#ifndef KERNEL_IDT_H
#define KERNEL_IDT_H

#include "types.h"

/* ---------------------------------------------------------------------------
 * A 64-bit IDT gate descriptor — 16 bytes, and every field's position is fixed
 * by the CPU, so the layout and packing must be EXACT. The 64-bit handler
 * address is scattered across three fields (low/mid/high) for backward binary
 * compatibility with the old 32-bit descriptor format.
 * ------------------------------------------------------------------------- */
struct idt_entry {
    uint16_t offset_low;    /* handler address bits  0..15                      */
    uint16_t selector;      /* code segment selector to load (our 0x08)         */
    uint8_t  ist;           /* bits 0..2 = Interrupt Stack Table index (0 = none) */
    uint8_t  type_attr;     /* P|DPL|0|type. 0x8E = present, ring0, 64-bit IRQ gate */
    uint16_t offset_mid;    /* handler address bits 16..31                      */
    uint32_t offset_high;   /* handler address bits 32..63                      */
    uint32_t zero;          /* reserved, must be 0                              */
} __attribute__((packed));  /* no padding: the CPU walks these by raw offset    */

/* The operand to the `lidt` instruction: the table's size-minus-one and its
 * base linear address. Packed so `limit` and `base` are adjacent with no gap. */
struct idt_ptr {
    uint16_t limit;
    uint64_t base;
} __attribute__((packed));

/* ---------------------------------------------------------------------------
 * struct regs — the CPU + software register snapshot an ISR stub builds on the
 * stack, and the exact shape the C dispatcher receives a pointer to.
 *
 * THE ORDERING IS LOAD-BEARING. The stack grows DOWN, so the field at the
 * LOWEST address (declared FIRST here) is the LAST thing pushed. isr.S therefore
 * pushes rax first ... r15 last, and the CPU auto-pushed ss..rip on entry. If
 * this struct and the push order in isr.S ever disagree, every field the
 * handler reads is garbage — so they are commented in lockstep.
 * ------------------------------------------------------------------------- */
struct regs {
    uint64_t r15, r14, r13, r12, r11, r10, r9, r8;   /* pushed by our stub, last first */
    uint64_t rbp, rdi, rsi, rdx, rcx, rbx, rax;      /* ...continuing, rax pushed first */
    uint64_t int_no, err_code;                       /* stub pushes these (err may be 0)*/
    uint64_t rip, cs, rflags, rsp, ss;               /* pushed by the CPU on interrupt  */
};

void idt_init(void);   /* build all 256 gates and `lidt` the table */

#endif /* KERNEL_IDT_H */
