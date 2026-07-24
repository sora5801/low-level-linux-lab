/* ===========================================================================
 * serial.h — the COM1 (0x3F8) 16550 UART interface.
 * ===========================================================================
 * The serial port is the kernel-developer's best friend: QEMU can pipe COM1
 * straight to your terminal (`-serial stdio`), so kernel output survives even
 * when the VGA console is unavailable, and you can capture a full boot log to a
 * file. Every message we print to VGA we ALSO write here.
 * =========================================================================== */
#ifndef KERNEL_SERIAL_H
#define KERNEL_SERIAL_H

#include "types.h"

void serial_init(void);            /* program the UART: 38400 8N1, FIFOs on   */
void serial_putc(char c);          /* blocking write of one byte              */
void serial_puts(const char *s);   /* a NUL-terminated string                 */

#endif /* KERNEL_SERIAL_H */
