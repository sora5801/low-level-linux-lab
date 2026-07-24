/* ===========================================================================
 * vga.c — driving the 0xB8000 text buffer, cell by cell.
 * ===========================================================================
 *
 * THE HARDWARE MODEL
 * ------------------
 * VGA text mode 3 maps an 80x25 grid of character cells to physical memory
 * starting at 0xB8000. Each cell is 16 bits laid out little-endian:
 *
 *     bit  15                    8 7                     0
 *          +---------------------+-----------------------+
 *          |  attribute byte     |   character byte      |
 *          +---------------------+-----------------------+
 *     attribute = (background<<4) | foreground   (bit 7 = blink on real HW)
 *
 * The cell for (row, col) lives at linear index `row*80 + col`, i.e. byte
 * offset `(row*80 + col) * 2` from 0xB8000. That "(row,col) -> offset" mapping
 * is the single most important computation in this file — asm/demo.c lifts it
 * out verbatim so you can read it in isolation, in assembly.
 *
 * Because we are in long mode with the first 1 GiB identity-mapped by the boot
 * code, the physical address 0xB8000 is ALSO its virtual address, so a plain
 * pointer works — no ioremap, no page-table poking here.
 * =========================================================================== */
#include "vga.h"
#include "io.h"

#define VGA_COLS   80
#define VGA_ROWS   25
/* volatile: this memory is watched by the display hardware, so the compiler
 * must not cache, coalesce, or drop our stores — every write must really land. */
static volatile uint16_t *const VGA_MEM = (volatile uint16_t *)0xB8000;

/* Console cursor state. `static` = private to this translation unit; there is no
 * other CPU yet (uniprocessor teaching kernel), so no locking is needed. */
static uint8_t cur_row = 0;
static uint8_t cur_col = 0;
static uint8_t cur_attr = (VGA_BLACK << 4) | VGA_LGREY;   /* grey on black */

/* ---------------------------------------------------------------------------
 * vga_cell_offset — THE core math: (row, col) -> linear cell index.
 * A cell index, NOT a byte offset; VGA_MEM is a uint16_t* so the compiler
 * scales by 2 for us. This is the exact expression asm/demo.c isolates.
 * ------------------------------------------------------------------------- */
static inline size_t vga_cell_offset(uint8_t row, uint8_t col)
{
    return (size_t)row * VGA_COLS + col;
}

/* Pack a glyph + the current attribute into the 16-bit cell value. */
static inline uint16_t vga_entry(char c)
{
    return (uint16_t)((uint8_t)c) | ((uint16_t)cur_attr << 8);
}

/* ---------------------------------------------------------------------------
 * update_hw_cursor — tell the CRT controller where the blinking cursor sits.
 *
 * The VGA CRTC exposes an *indexed* register file behind two ports: you write
 * the register number to the index port (0x3D4), then read/write its value at
 * the data port (0x3D5). The cursor location is a 16-bit value split across two
 * 8-bit registers: 0x0F (low byte) and 0x0E (high byte). So we take the linear
 * index and hand the CRTC its low and high halves — again the (row,col)->index
 * math, this time feeding hardware instead of a memory store. asm/demo.c models
 * exactly this hi/lo byte split.
 * ------------------------------------------------------------------------- */
static void update_hw_cursor(void)
{
    uint16_t pos = (uint16_t)vga_cell_offset(cur_row, cur_col);
    outb(0x3D4, 0x0F);              /* select CRTC reg 0x0F: cursor location low */
    outb(0x3D5, (uint8_t)(pos & 0xFF));
    outb(0x3D4, 0x0E);              /* select CRTC reg 0x0E: cursor location high*/
    outb(0x3D5, (uint8_t)((pos >> 8) & 0xFF));
}

/* Blank the whole screen to spaces in the current attribute and home the
 * cursor. A ' ' with our attribute paints the background color into every cell.*/
void vga_init(void)
{
    for (size_t i = 0; i < (size_t)VGA_COLS * VGA_ROWS; i++)
        VGA_MEM[i] = vga_entry(' ');
    cur_row = cur_col = 0;
    update_hw_cursor();
}

void vga_set_color(enum vga_color fg, enum vga_color bg)
{
    cur_attr = (uint8_t)((bg << 4) | (fg & 0x0F));
}

/* Scroll the display up by one line when we run off the bottom. We copy every
 * row up one slot (row N <- row N+1) then clear the freed last row. This is the
 * teaching-simple O(rows*cols) memmove; a real console keeps a ring buffer. */
static void vga_scroll(void)
{
    for (size_t r = 1; r < VGA_ROWS; r++)
        for (size_t c = 0; c < VGA_COLS; c++)
            VGA_MEM[vga_cell_offset(r - 1, c)] = VGA_MEM[vga_cell_offset(r, c)];

    for (size_t c = 0; c < VGA_COLS; c++)         /* wipe the now-duplicated last row */
        VGA_MEM[vga_cell_offset(VGA_ROWS - 1, c)] = vga_entry(' ');

    cur_row = VGA_ROWS - 1;                        /* stay on the last line            */
}

void vga_putc(char c)
{
    switch (c) {
    case '\n':                          /* newline: down a row, back to column 0 */
        cur_col = 0;
        cur_row++;
        break;
    case '\r':                          /* carriage return: column 0, same row   */
        cur_col = 0;
        break;
    case '\b':                          /* backspace: step left if we can        */
        if (cur_col) cur_col--;
        break;
    default:
        VGA_MEM[vga_cell_offset(cur_row, cur_col)] = vga_entry(c);
        if (++cur_col >= VGA_COLS) {    /* wrap at the right margin               */
            cur_col = 0;
            cur_row++;
        }
        break;
    }
    if (cur_row >= VGA_ROWS)            /* off the bottom? pull everything up     */
        vga_scroll();
    update_hw_cursor();
}

void vga_puts(const char *s)
{
    for (; *s; s++)
        vga_putc(*s);
}

/* Print a 64-bit value as 0x-prefixed hex, 16 nibbles, most-significant first.
 * We walk nibbles high->low; `(v >> shift) & 0xF` isolates each. Used to dump
 * the fault address / error code from the exception handler. */
void vga_put_hex(uint64_t v)
{
    static const char digits[] = "0123456789abcdef";
    vga_puts("0x");
    for (int shift = 60; shift >= 0; shift -= 4)
        vga_putc(digits[(v >> shift) & 0xF]);
}

/* Print an unsigned value in base 10. We generate digits least-significant
 * first into a small buffer (max 20 digits for a 64-bit value), then emit in
 * reverse. No division library is needed — the compiler lowers /10 and %10 to a
 * multiply-and-shift, which you can see in the generated assembly. */
void vga_put_dec(uint64_t v)
{
    char buf[20];
    int n = 0;
    if (v == 0) { vga_putc('0'); return; }
    while (v) { buf[n++] = (char)('0' + v % 10); v /= 10; }
    while (n--) vga_putc(buf[n]);
}
