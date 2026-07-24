/* =============================================================================
 * boot.ld — lay out the MBR at 0x7C00 and stamp the boot signature at byte 510.
 * =============================================================================
 *
 * boot.S is assembled to an ELF object; this script links it to a flat image
 * whose first byte is at 0x7C00 (where the BIOS loads and runs a boot sector).
 * It also handles the two hard requirements of an MBR in ONE place:
 *   1. the whole thing is exactly 512 bytes, and
 *   2. bytes 510..511 are 0x55,0xAA (the little-endian word 0xAA55) — the flag
 *      the BIOS checks before it will boot the sector.
 * If boot.S ever grows past 510 bytes, the `. = 0x7C00 + 510` assignment moves
 * the location counter BACKWARD and `ld` errors out — a compile-time guarantee
 * that we never silently overflow the sector.
 * ============================================================================= */

OUTPUT_FORMAT("elf64-x86-64")
ENTRY(realmode_start)

SECTIONS
{
    . = 0x7C00;                 /* the canonical load address of a boot sector     */

    .boot :
    {
        *(.text)                /* all boot code, in source order                  */
        *(.rodata*)
        *(.data)

        . = 0x7C00 + 510;       /* pad out to the 511th byte (offset 510)          */
        SHORT(0xAA55);          /* emit 0x55 at 510, 0xAA at 511 (little-endian)   */
    }

    /DISCARD/ :
    {
        *(.note .note.*)
        *(.comment)
        *(.eh_frame .eh_frame_hdr)
        *(.bss .bss.*)
    }
}
