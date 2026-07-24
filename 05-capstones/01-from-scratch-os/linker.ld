/* =============================================================================
 * linker.ld — where the 64-bit kernel's sections live in physical memory.
 * =============================================================================
 *
 * The boot sector loads the flat kernel image to physical 0x10000 and jumps
 * there, so the kernel MUST be linked to run at 0x10000: every absolute address
 * the compiler bakes in (a jump-table entry, the address of `idt[]`, a string
 * literal pointer) has to match where the bytes actually sit. We build with no
 * PIE, so link address == load address == run address — the simplest possible
 * story, and the reason `_start` must be the very first byte.
 * ============================================================================= */

OUTPUT_FORMAT("elf64-x86-64")   /* we objcopy this ELF down to a flat binary later */
ENTRY(_start)                   /* the ELF entry symbol (informational for a flat
                                 * image; the boot sector jumps to 0x10000 directly) */

SECTIONS
{
    . = 0x10000;                /* the load/run address the boot sector uses       */

    /* .text.boot MUST come first so entry.S's _start is at offset 0 of the image
     * — that is the exact address the boot sector's final `jmp` targets. */
    .text : ALIGN(16)
    {
        *(.text.boot)           /* entry.S: _start                                 */
        *(.text .text.*)        /* all other code (kmain, drivers, isr stubs)      */
    }

    /* Read-only data: string literals, the EXC_NAME/SCAN2ASCII tables, and the
     * isr_stub_table the IDT is built from. */
    .rodata : ALIGN(16)
    {
        *(.rodata .rodata.*)
    }

    /* Initialized, writable data (e.g. vga.c's cur_attr). Present in the image. */
    .data : ALIGN(16)
    {
        *(.data .data.*)
    }

    /* Zero-initialized data: NOT stored in the image (NOBITS). entry.S zeroes
     * the [__bss_start, __bss_end) range at boot, so we export those bounds. The
     * kernel stack and the IDT live here. */
    . = ALIGN(16);
    __bss_start = .;
    .bss : ALIGN(16)
    {
        *(.bss .bss.*)
        *(COMMON)
    }
    . = ALIGN(16);
    __bss_end = .;

    /* Toolchain metadata we neither load nor need in a freestanding binary. */
    /DISCARD/ :
    {
        *(.note .note.*)
        *(.comment)
        *(.eh_frame .eh_frame_hdr)
    }
}
