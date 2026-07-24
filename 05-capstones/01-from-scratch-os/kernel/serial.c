/* ===========================================================================
 * serial.c — bringing up the 16550 UART on COM1 and printing to it.
 * ===========================================================================
 *
 * THE UART REGISTER FILE (base 0x3F8, one byte each)
 * --------------------------------------------------
 *   +0  Data / divisor-low   (RBR/THR when DLAB=0; DLL when DLAB=1)
 *   +1  Interrupt enable / divisor-high (IER when DLAB=0; DLM when DLAB=1)
 *   +2  Interrupt ID / FIFO control (IIR read / FCR write)
 *   +3  Line control  (LCR)  — the top bit (DLAB) swaps +0/+1 to the divisor
 *   +4  Modem control (MCR)
 *   +5  Line status   (LSR)  — bit 5 (0x20) = "transmit holding register empty"
 *
 * The baud rate is set by a 16-bit *divisor* of the 115200 Hz base clock. To
 * write it you set DLAB=1 (LCR bit 7), which re-purposes +0/+1 as the divisor's
 * low/high bytes, write the divisor, then clear DLAB to return +0/+1 to normal
 * data/interrupt use. Divisor 3 => 115200/3 = 38400 baud.
 * =========================================================================== */
#include "serial.h"
#include "io.h"

#define COM1 0x3F8

void serial_init(void)
{
    outb(COM1 + 1, 0x00);   /* IER = 0: no UART interrupts (we poll on output)   */
    outb(COM1 + 3, 0x80);   /* LCR: set DLAB=1 so +0/+1 address the divisor      */
    outb(COM1 + 0, 0x03);   /* divisor low  = 3   -> 115200/3 = 38400 baud       */
    outb(COM1 + 1, 0x00);   /* divisor high = 0                                  */
    outb(COM1 + 3, 0x03);   /* LCR: DLAB=0, 8 data bits, no parity, 1 stop (8N1) */
    outb(COM1 + 2, 0xC7);   /* FCR: enable+clear RX/TX FIFOs, 14-byte trigger    */
    outb(COM1 + 4, 0x0B);   /* MCR: DTR|RTS|OUT2 — OUT2 gates the IRQ line on PC */
}

/* Is the transmit holding register empty (ready for the next byte)? LSR bit 5.
 * This single bit-test is the "port I/O helper as pure logic" that asm/demo.c
 * models with serial_thr_empty(): no privileged `in`, just the mask. */
static inline int tx_ready(void)
{
    return inb(COM1 + 5) & 0x20;
}

void serial_putc(char c)
{
    /* Terminals expect CR before LF for a clean column reset; the VGA console
     * handles \n on its own, but a raw serial line does not, so we translate. */
    if (c == '\n') {
        while (!tx_ready()) { }     /* spin until the UART can accept a byte     */
        outb(COM1, '\r');
    }
    while (!tx_ready()) { }         /* poll THR-empty: the whole flow control    */
    outb(COM1, (uint8_t)c);
}

void serial_puts(const char *s)
{
    for (; *s; s++)
        serial_putc(*s);
}
