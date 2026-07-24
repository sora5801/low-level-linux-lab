/* ===========================================================================
 * pic.h — the 8259A Programmable Interrupt Controller interface.
 * ===========================================================================
 * Two cascaded 8259A chips route the 16 legacy hardware IRQ lines to the CPU.
 * We must REMAP them before enabling interrupts (see pic.c for why), then mask
 * off everything except the lines we actually service, and acknowledge each
 * interrupt with an End-Of-Interrupt so the chip will deliver the next one.
 * =========================================================================== */
#ifndef KERNEL_PIC_H
#define KERNEL_PIC_H

#include "types.h"

/* After remap, hardware IRQ n arrives as CPU interrupt vector (PIC_OFFSET + n).
 * We put the 16 IRQs at vectors 32..47, immediately above the 32 CPU-reserved
 * exception vectors (0..31). Picking anything below 32 would collide. */
#define PIC1_OFFSET 0x20   /* master: IRQ0..7  -> vectors 32..39 */
#define PIC2_OFFSET 0x28   /* slave:  IRQ8..15 -> vectors 40..47 */

void pic_remap(void);              /* relocate both PICs to 0x20/0x28          */
void pic_set_mask(uint8_t irq);    /* disable one IRQ line                     */
void pic_clear_mask(uint8_t irq);  /* enable  one IRQ line                     */
void pic_send_eoi(uint8_t irq);    /* acknowledge an interrupt we just handled */

#endif /* KERNEL_PIC_H */
