/* ===========================================================================
 * vga.h — the 80x25 VGA text-mode console interface.
 * ===========================================================================
 * The BIOS leaves us in VGA text mode 3: a grid of 80 columns x 25 rows whose
 * character cells are memory-mapped at physical 0xB8000. Each cell is TWO bytes
 * (character byte + attribute byte), so writing text is literally storing 16-bit
 * values into that array. This header exposes the small console API kmain uses.
 * =========================================================================== */
#ifndef KERNEL_VGA_H
#define KERNEL_VGA_H

#include "types.h"

/* The 16 CGA/VGA text colors (4-bit). Foreground can be any of the 16; the
 * background can be the low 8 (bit 7 of the attribute byte is "blink" on real
 * hardware). We only need a couple, but naming them documents the attribute. */
enum vga_color {
    VGA_BLACK = 0,  VGA_BLUE = 1,   VGA_GREEN = 2,      VGA_CYAN = 3,
    VGA_RED = 4,    VGA_MAGENTA = 5,VGA_BROWN = 6,      VGA_LGREY = 7,
    VGA_DGREY = 8,  VGA_LBLUE = 9,  VGA_LGREEN = 10,    VGA_LCYAN = 11,
    VGA_LRED = 12,  VGA_LMAGENTA=13,VGA_YELLOW = 14,    VGA_WHITE = 15,
};

void vga_init(void);                       /* clear screen, home the cursor      */
void vga_set_color(enum vga_color fg, enum vga_color bg);
void vga_putc(char c);                     /* one char; handles \n, \r, \b, scroll*/
void vga_puts(const char *s);              /* a NUL-terminated string             */
void vga_put_hex(uint64_t v);              /* "0x…" — for dumping register state  */
void vga_put_dec(uint64_t v);              /* base-10 — for the tick counter      */

#endif /* KERNEL_VGA_H */
