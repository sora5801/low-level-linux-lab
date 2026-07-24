/* ===========================================================================
 * boot.h — the Linux x86-64 boot protocol "zero page" (struct boot_params).
 * ===========================================================================
 *
 * WHY THIS FILE EXISTS
 * --------------------
 * A Firecracker-style microVM does NOT run a BIOS or a bootloader. It uses the
 * Linux 64-bit boot protocol (Documentation/x86/boot.rst) to jump STRAIGHT into
 * the kernel's 64-bit entry point — that direct entry is a big part of why a
 * microVM boots in milliseconds. The contract at that entry point is exact:
 *
 *   - the CPU is already in 64-bit long mode, paging on, with a flat GDT;
 *   - %rsi points at a filled `struct boot_params` (historically called the
 *     "zero page" because it is one zeroed 4 KiB page with fields at fixed
 *     offsets);
 *   - the kernel image sits at its load address (0x100000 for a bzImage's
 *     protected-mode/64-bit half).
 *
 * This header defines the *layout* of that zero page — the handful of fields a
 * VMM must fill — as OFFSET constants plus the one genuinely-structured piece,
 * the E820 memory-map entry. It is self-contained (own fixed-width types, no
 * <asm/bootparam.h>) so it, unlike the KVM code, reads anywhere.
 *
 * HONEST SCOPE: our teaching-core reproduces the ENTRY CONTRACT (long mode, GDT,
 * page tables, %rsi -> a filled boot_params with an E820 map) and enters our own
 * 64-bit payload. It does NOT parse a real bzImage setup header, honor its
 * xloadflags, or copy a kernel to 0x100000 — those are documented in the README
 * as the remaining production steps. Filling the zero page here shows exactly
 * what a real kernel would consume the instant it starts.
 *
 * THE ZERO PAGE IS OFFSET-ADDRESSED
 * ---------------------------------
 * `struct boot_params` is 4096 bytes with sub-structs (screen_info, the
 * setup_header, the E820 table) at architecturally fixed offsets. Rather than
 * reproduce the whole 4 KiB struct with exact padding (error-prone and huge), we
 * write the few fields we need by their well-known absolute offsets into a zeroed
 * page. The offsets below are from arch/x86/include/uapi/asm/bootparam.h and are
 * ABI-stable.
 * =========================================================================== */

#ifndef BOOT_H
#define BOOT_H

typedef unsigned char      bp_u8;
typedef unsigned int       bp_u32;
typedef unsigned long long bp_u64;

/* --- fields inside struct boot_params, by absolute byte offset in the page --- */

/* The number of valid entries in the E820 table below (a single u8 at 0x1e8).
 * The kernel reads this to learn its RAM map — how much memory it has and which
 * ranges are usable vs reserved. Without it, the kernel sees no memory and dies
 * immediately. */
#define BP_OFF_E820_ENTRIES   0x1e8

/* The setup_header sub-struct begins at 0x1f1. The three fields below are the
 * ones a loader that jumps to the 64-bit entry still fills, given by their
 * absolute offset in the page. */
#define BP_OFF_TYPE_OF_LOADER 0x210  /* u8:  who loaded the kernel. 0xFF = "undefined
                                      *      / custom loader" — legal, tells the
                                      *      kernel not to assume a known bootloader.*/
#define BP_OFF_LOADFLAGS      0x211  /* u8:  see BP_LOADFLAG_* below                 */
#define BP_OFF_RAMDISK_IMAGE  0x218  /* u32: GPA of the initramfs (0 = none)         */
#define BP_OFF_RAMDISK_SIZE   0x21c  /* u32: initramfs size in bytes                 */
#define BP_OFF_CMD_LINE_PTR   0x228  /* u32: GPA of the NUL-terminated kernel cmdline*/

/* The E820 memory map array begins at 0x2d0, up to 128 entries of 20 bytes each. */
#define BP_OFF_E820_TABLE     0x2d0

/* loadflags bits (BP_OFF_LOADFLAGS). */
#define BP_LOADFLAG_LOADED_HIGH 0x01  /* protected-mode kernel loaded at 0x100000    */
#define BP_LOADFLAG_CAN_USE_HEAP 0x80 /* heap_end_ptr is valid                       */

/* ---------------------------------------------------------------------------
 * struct boot_e820_entry — one range in the physical memory map (E820 is the
 * decades-old BIOS interface name; the kernel keeps the shape). PACKED to its
 * exact 20-byte on-page layout: 8 (addr) + 8 (size) + 4 (type). A real BIOS filled
 * these; a VMM fills them itself because there is no BIOS.
 * ------------------------------------------------------------------------- */
struct boot_e820_entry {
    bp_u64 addr;    /* start guest-physical address of the range                   */
    bp_u64 size;    /* length of the range in bytes                                */
    bp_u32 type;    /* BP_E820_* below                                             */
} __attribute__((packed));

/* E820 range types. The kernel will only allocate from RAM ranges. */
#define BP_E820_RAM      1u   /* usable RAM                                        */
#define BP_E820_RESERVED 2u   /* do not touch (firmware, MMIO holes, ...)          */

#endif /* BOOT_H */
