/* ===========================================================================
 * types.h — fixed-width integer types for a FREESTANDING kernel.
 * ===========================================================================
 *
 * WHY THIS FILE EXISTS
 * --------------------
 * We compile the kernel with `-ffreestanding -nostdlib`, which means there is
 * NO libc and therefore NO <stdint.h>, <stddef.h>, <stdbool.h> to include.
 * (`-ffreestanding` promises the compiler we are not a hosted C program, so it
 * must not assume libc exists.) A freestanding implementation is still allowed
 * to ship a handful of headers, but to keep this teaching kernel utterly self-
 * contained — every byte visible — we declare the types we need ourselves.
 *
 * The exact widths below are the ones the x86-64 System V ABI guarantees for
 * the `-m64` target we build with. If you ever retarget this kernel to another
 * ABI, this is the one file you would revisit.
 * =========================================================================== */
#ifndef KERNEL_TYPES_H
#define KERNEL_TYPES_H

/* On x86-64 SysV: char=1, short=2, int=4, long=8, long long=8, pointer=8. We
 * pick the smallest type of each width so sizeof is exact on this target. */
typedef unsigned char       uint8_t;    /* one byte; a raw scancode, a port value */
typedef signed   char       int8_t;
typedef unsigned short      uint16_t;   /* a VGA cell (char<<0 | attr<<8)          */
typedef signed   short      int16_t;
typedef unsigned int        uint32_t;   /* a PIT divisor, an IDT offset slice      */
typedef signed   int        int32_t;
typedef unsigned long long  uint64_t;   /* a 64-bit register / physical address    */
typedef signed   long long  int64_t;

/* size_t must be wide enough to index any object; on LP64 that is 8 bytes. We
 * use `unsigned long` (also 8 bytes here) so it matches pointer arithmetic. */
typedef unsigned long       size_t;
/* uintptr_t: an integer wide enough to hold a pointer. Used when we do address
 * math on MMIO regions (0xB8000) or the ISR stub table without a real pointer. */
typedef unsigned long       uintptr_t;

/* We have no <stdbool.h>; a plain int-backed bool keeps the driver code readable
 * without pulling a header. `true`/`false` are the obvious 1/0. */
typedef int bool;
#define true  1
#define false 0

/* A NULL that does not need <stddef.h>. Cast to (void*) so it types cleanly. */
#define NULL ((void *)0)

#endif /* KERNEL_TYPES_H */
