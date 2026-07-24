/* ===========================================================================
 * io.h — the port-mapped I/O helpers: how the CPU talks to legacy devices.
 * ===========================================================================
 *
 * WHY PORT I/O EXISTS
 * -------------------
 * x86 has TWO separate address spaces: normal memory (accessed by `mov`) and a
 * 65536-entry *I/O port* space, accessed only by the `in`/`out` instructions.
 * The PIC (0x20/0xA0), the PIT timer (0x40-0x43), the keyboard controller
 * (0x60/0x64), the serial UART (0x3F8), and the VGA CRT controller (0x3D4/5)
 * all live in that port space. There is no way to reach them with a pointer;
 * you MUST use `in`/`out`. Those instructions are privileged (they fault in
 * ring 3), but our whole kernel runs in ring 0, so they just work.
 *
 * These are `static inline` so every call site gets the two-instruction
 * sequence emitted directly — a function call to wrap a single `out` would cost
 * more than the payload. The `volatile` on each asm forbids the optimizer from
 * reordering or deleting the access: talking to hardware is a side effect the
 * compiler cannot see, so we must pin it in place.
 *
 * asm/demo.c models the *pure integer* half of this (which port, which byte)
 * with no privileged instruction, so the teaching assembly stays host-portable.
 * =========================================================================== */
#ifndef KERNEL_IO_H
#define KERNEL_IO_H

#include "types.h"

/* outb(port, val): write one byte to an I/O port.
 *   "a"(val)  -> put val in AL (the `out` source is always AL/AX/EAX)
 *   "Nd"(port)-> put port in DX, or use an 8-bit immediate if it fits ("N")
 * `out %al, %dx` drives the byte onto the I/O bus; the device latches it. */
static inline void outb(uint16_t port, uint8_t val)
{
    __asm__ volatile("outb %0, %1" : : "a"(val), "Nd"(port));
}

/* inb(port): read one byte from an I/O port into AL, return it. Used to poll a
 * status register (serial LSR, keyboard status) or drain a data port (0x60). */
static inline uint8_t inb(uint16_t port)
{
    uint8_t ret;
    __asm__ volatile("inb %1, %0" : "=a"(ret) : "Nd"(port));
    return ret;
}

/* io_wait(): burn one I/O cycle by writing an unused port (0x80, the POST diag
 * port that no real device answers). The old 8259 PIC needs a short settle time
 * between the ICW bytes we stream at it during remapping; a dummy `out` is the
 * canonical, portable way to insert that delay without a calibrated timer. */
static inline void io_wait(void)
{
    outb(0x80, 0);
}

#endif /* KERNEL_IO_H */
