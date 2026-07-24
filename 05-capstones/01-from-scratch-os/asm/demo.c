/* ===========================================================================
 * asm/demo.c — the kernel's PURE-LOGIC core: VGA cell math + port addressing.
 * ===========================================================================
 *
 * WHY THIS FILE EXISTS
 * --------------------
 * The real kernel (the kernel/ sources) cannot be turned into teaching asm on an
 * arbitrary host: it is built `-ffreestanding -nostdlib -mno-red-zone`, it
 * executes privileged `in`/`out`/`lidt`/`hlt` instructions, and it runs in a
 * 64-bit environment with no OS underneath. clang on a Windows/macOS/Linux
 * laptop cannot meaningfully compile that to *portable* asm.
 *
 * But the DECISION-MAKING heart of the console driver is pure integer logic
 * with no privilege and no headers: given (row, col) where does the character
 * cell live? given a glyph and colors, what 16-bit value goes in that cell?
 * given a linear cursor position, which two bytes do we hand the CRT
 * controller? given a device base port and a register index, which port do we
 * touch? Those are exactly the computations vga.c and io.h perform, minus the
 * one privileged instruction at the very end — so we lift them here into a
 * self-contained file that includes nothing, declares its own types, and
 * compiles to real assembly on any host.
 *
 * Read the generated asm (asm/demo.s and friends) to SEE:
 *   - a multiply-by-80 become an `lea`/shift chain (strength reduction),
 *   - a byte pack become `shl`+`or`,
 *   - a hi/lo split become a `shr`+`and`,
 *   - and, at -O2, the whole self-test fold to `xor %eax,%eax` because every
 *     input is a compile-time constant.
 * =========================================================================== */

/* Our own fixed-width types, so this file needs no <stdint.h>. On the x86-64
 * SysV target these match the kernel's uint8_t/uint16_t/uint32_t exactly. */
typedef unsigned char  u8;
typedef unsigned short u16;
typedef unsigned int   u32;

/* The text-mode geometry, mirrored from kernel/vga.c. */
#define VGA_COLS 80
#define VGA_ROWS 25

/* ---------------------------------------------------------------------------
 * vga_cell_offset — THE core mapping: (row, col) -> linear cell index.
 *
 * This is the single most-executed expression in the console driver: every
 * character printed evaluates it. It is a CELL index (count of 16-bit cells),
 * not a byte offset — the caller scales by 2 (or the compiler does, when it
 * indexes a u16 array). Watch the compiler turn `row*80` into shifts+adds
 * (80 = 64 + 16 = (row<<6)+(row<<4)) rather than a general `imul`.
 * ABI: row in %edi, col in %esi; the u32 result in %eax.
 * ------------------------------------------------------------------------- */
u32 vga_cell_offset(u32 row, u32 col)
{
    return row * VGA_COLS + col;
}

/* ---------------------------------------------------------------------------
 * vga_byte_offset — the same location expressed as a BYTE offset from 0xB8000.
 * Each cell is two bytes, so this is just the cell index << 1. Kept separate to
 * show the strength-reduced `*2` (an `add %eax,%eax` or `lea (,%rax,2)`).
 * ------------------------------------------------------------------------- */
u32 vga_byte_offset(u32 row, u32 col)
{
    return vga_cell_offset(row, col) * 2u;
}

/* ---------------------------------------------------------------------------
 * vga_entry — pack a glyph and its colors into the 16-bit cell value.
 *
 *     bits 15..12 background | bits 11..8 foreground | bits 7..0 character
 *
 * The attribute byte is (bg<<4)|fg; the cell is (attr<<8)|char. This is two
 * shifts and two ors — the compiler emits exactly that. It is the pure form of
 * vga.c's vga_entry() (minus reading the driver's current-attribute state).
 * ABI: ch in %edi, fg in %esi, bg in %edx; the u16 result in %ax.
 * ------------------------------------------------------------------------- */
u16 vga_entry(u8 ch, u8 fg, u8 bg)
{
    u8 attr = (u8)((bg << 4) | (fg & 0x0F));
    return (u16)(((u16)attr << 8) | (u16)ch);
}

/* ---------------------------------------------------------------------------
 * vga_cursor_hi / vga_cursor_lo — split a 16-bit cursor position into the two
 * bytes the VGA CRT controller wants written to its 0x0E (high) and 0x0F (low)
 * registers. This is the "VGA cursor-index math" the console uses to move the
 * blinking hardware cursor. A shift-and-mask each; trivial, and that is the
 * point — you can read the machine code at a glance.
 * ABI: pos in %edi; the u8 result in %al.
 * ------------------------------------------------------------------------- */
u8 vga_cursor_hi(u16 pos) { return (u8)((pos >> 8) & 0xFF); }
u8 vga_cursor_lo(u16 pos) { return (u8)(pos & 0xFF); }

/* ---------------------------------------------------------------------------
 * port_reg — the port-I/O helper's PURE half: which I/O port does register
 * `off` of a device based at `base` live on? For the 16550 UART, base is 0x3F8
 * and off is 0..5 (data, IER, FCR, LCR, MCR, LSR). The real driver follows this
 * add with a privileged `outb`/`inb`; here we compute only the address, so the
 * routine is host-portable and its assembly is a single `lea`.
 * ABI: base in %edi, off in %esi; the u16 result in %ax.
 * ------------------------------------------------------------------------- */
u16 port_reg(u16 base, u16 off)
{
    return (u16)(base + off);
}

/* ---------------------------------------------------------------------------
 * serial_thr_empty — decode a UART Line Status Register byte: is the transmit
 * holding register empty (bit 5 = 0x20), i.e. may we send the next byte? This
 * is the exact predicate serial.c spins on, reduced to the bit test with the
 * `inb` removed. Returns 0/1.
 * ABI: lsr in %edi; the u32 result in %eax.
 * ------------------------------------------------------------------------- */
u32 serial_thr_empty(u8 lsr)
{
    return (lsr >> 5) & 1u;
}

/* ---------------------------------------------------------------------------
 * pit_divisor — the timer math from kmain.c: the 8254 counts down from this
 * reload value at a fixed 1193182 Hz, so to fire `hz` times per second we load
 * base/hz. A single unsigned `div`. Shown because it is the one place the kernel
 * does a runtime division, and the generated `div` (not a multiply-by-magic,
 * since hz is a variable here) is worth seeing.
 * ABI: hz in %edi; the u32 result in %eax.
 * ------------------------------------------------------------------------- */
u32 pit_divisor(u32 hz)
{
    return 1193182u / hz;
}

/* ---------------------------------------------------------------------------
 * demo_selftest — prove the logic with constant inputs, no hardware. Returns 0
 * on success or a small code naming the first failing check. Scaffolding, not
 * core logic: at -O2 every check folds away and this becomes `xor %eax,%eax`
 * (return 0). Seeing that erasure in demo.O2.s is itself the lesson.
 * ------------------------------------------------------------------------- */
int demo_selftest(void)
{
    /* row 0 col 0 is cell 0; row 1 col 0 is cell 80; row 24 col 79 is 1999. */
    if (vga_cell_offset(0, 0)   != 0)    return 1;
    if (vga_cell_offset(1, 0)   != 80)   return 2;
    if (vga_cell_offset(24, 79) != 1999) return 3;   /* the last visible cell    */
    if (vga_byte_offset(1, 0)   != 160)  return 4;   /* 80 cells * 2 bytes       */

    /* 'A' (0x41) white-on-blue: attr = (1<<4)|15 = 0x1F, cell = 0x1F41. */
    if (vga_entry(0x41, 15, 1)  != 0x1F41) return 5;

    /* cursor at cell 1999 = 0x07CF -> hi 0x07, lo 0xCF. */
    if (vga_cursor_hi(1999) != 0x07) return 6;
    if (vga_cursor_lo(1999) != 0xCF) return 7;

    /* COM1 LSR is at base 0x3F8 + 5 = 0x3FD. */
    if (port_reg(0x3F8, 5) != 0x3FD) return 8;

    /* LSR with bit 5 set means "ready"; without it, "busy". */
    if (!serial_thr_empty(0x60)) return 9;    /* 0x60 has bit 5 set              */
    if ( serial_thr_empty(0x01)) return 10;   /* 0x01 does not                   */

    /* 100 Hz -> divisor 11931 (1193182 / 100, truncated). */
    if (pit_divisor(100) != 11931) return 11;

    return 0;
}
