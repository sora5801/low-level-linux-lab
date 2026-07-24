/* ===========================================================================
 * pic.c — remapping and driving the twin 8259A interrupt controllers.
 * ===========================================================================
 *
 * WHY REMAP AT ALL?
 * -----------------
 * At power-on the BIOS programs the master PIC to deliver IRQ0..7 as CPU
 * interrupt vectors 0x08..0x0F. In real mode that was fine, but in protected/
 * long mode the CPU RESERVES vectors 0..31 for exceptions: vector 8 is the
 * Double Fault, vector 13 is the General Protection fault, vector 14 is a Page
 * Fault, and so on. If we left the PIC alone, a timer tick (IRQ0 -> vector 8)
 * would be indistinguishable from a double fault. So the first thing any long-
 * mode kernel does is REMAP the PICs to vectors 32..47, clear of the reserved
 * range. That reprogramming is a fixed 4-word "initialization command word"
 * (ICW1..ICW4) handshake, streamed to each chip's command+data port pair.
 *
 * PORTS:  master command 0x20 / data 0x21 ;  slave command 0xA0 / data 0xA1.
 * =========================================================================== */
#include "pic.h"
#include "io.h"

#define PIC1_CMD  0x20
#define PIC1_DATA 0x21
#define PIC2_CMD  0xA0
#define PIC2_DATA 0xA1
#define PIC_EOI   0x20    /* the "End Of Interrupt" command byte (OCW2)         */

#define ICW1_INIT 0x11    /* start init sequence + expect ICW4                  */
#define ICW4_8086 0x01    /* 8086/88 mode (vs the ancient 8080 mode)            */

void pic_remap(void)
{
    /* Read and preserve the current interrupt masks; the BIOS may have set up
     * a sensible state and we only want to change the vector base, not which
     * lines are enabled. We restore these exact masks at the end. */
    uint8_t mask1 = inb(PIC1_DATA);
    uint8_t mask2 = inb(PIC2_DATA);

    /* ICW1: begin initialization on both chips. After this they expect ICW2,
     * ICW3, ICW4 in order on their DATA ports. io_wait() gives the venerable
     * chip a moment to latch each byte (harmless on QEMU, correct on metal). */
    outb(PIC1_CMD, ICW1_INIT); io_wait();
    outb(PIC2_CMD, ICW1_INIT); io_wait();

    /* ICW2: the vector base — the whole point of remapping. IRQ0 now enters as
     * vector 0x20 (32); IRQ8 as 0x28 (40). */
    outb(PIC1_DATA, PIC1_OFFSET); io_wait();
    outb(PIC2_DATA, PIC2_OFFSET); io_wait();

    /* ICW3: describe the master/slave wiring. The slave PIC is cascaded onto
     * the master's IRQ2 line: tell the master "a slave hangs on line 2"
     * (bitmask 1<<2 = 0x04) and tell the slave "you are cascade identity 2". */
    outb(PIC1_DATA, 0x04); io_wait();
    outb(PIC2_DATA, 0x02); io_wait();

    /* ICW4: select 8086 mode so the chips use normal PC-style EOI semantics. */
    outb(PIC1_DATA, ICW4_8086); io_wait();
    outb(PIC2_DATA, ICW4_8086); io_wait();

    /* Restore the saved masks (OCW1). kmain will then unmask only IRQ0/IRQ1. */
    outb(PIC1_DATA, mask1);
    outb(PIC2_DATA, mask2);
}

/* Set (disable) one IRQ line by setting its bit in the mask register. IRQs 0-7
 * live on the master's mask, 8-15 on the slave's (offset by 8). */
void pic_set_mask(uint8_t irq)
{
    uint16_t port = (irq < 8) ? PIC1_DATA : PIC2_DATA;
    if (irq >= 8) irq -= 8;
    outb(port, inb(port) | (uint8_t)(1u << irq));
}

/* Clear (enable) one IRQ line by clearing its mask bit. */
void pic_clear_mask(uint8_t irq)
{
    uint16_t port = (irq < 8) ? PIC1_DATA : PIC2_DATA;
    if (irq >= 8) irq -= 8;
    outb(port, inb(port) & (uint8_t)~(1u << irq));
}

/* Acknowledge a handled interrupt. The chip will not raise the next interrupt
 * of equal-or-lower priority until it gets this EOI. CRUCIAL detail: for an IRQ
 * that came from the SLAVE (>=8), BOTH chips must be told — the slave to clear
 * its own in-service bit, then the master to clear the cascade (IRQ2) bit. */
void pic_send_eoi(uint8_t irq)
{
    if (irq >= 8)
        outb(PIC2_CMD, PIC_EOI);   /* EOI to the slave first */
    outb(PIC1_CMD, PIC_EOI);       /* always EOI the master  */
}
