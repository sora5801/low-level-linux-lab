/* ===========================================================================
 * asm/demo.c — the ARITHMETIC CORE of a linker, extracted standalone.
 * ===========================================================================
 *
 * Everything a linker does — parsing ELF, laying out sections, resolving
 * symbols — exists to serve this one routine: APPLYING A RELOCATION. A
 * relocation says "you left a hole in the code/data here; now that I know where
 * the symbol landed, fill it in." apply_reloc() is that fill-in step, and it is
 * the whole reason `objdump -d` of a linked binary shows real addresses where
 * the .o showed zeros.
 *
 * We isolate it here with NO system headers and our OWN integer types, so the
 * generated assembly (asm/demo.s and friends) is pure logic you can read
 * instruction by instruction — no libc noise. This mirrors minild.c's
 * relocate(); it is the piece worth seeing in machine code.
 *
 * THE FORMULAE (x86-64 System V):
 *
 *     symbol            let S = the symbol's final virtual address
 *     addend            let A = the constant baked into the relocation
 *     patch site        let P = the virtual address of the field being patched
 *
 *   R_X86_64_64    write 8 bytes:  S + A            (absolute pointer)
 *   R_X86_64_PC32  write 4 bytes:  S + A - P        (PC-relative disp32)
 *   R_X86_64_PLT32 write 4 bytes:  S + A - P        (call foo@PLT, bound direct)
 *   R_X86_64_32    write 4 bytes:  S + A  (u32)     (must fit unsigned 32)
 *   R_X86_64_32S   write 4 bytes:  S + A  (i32)     (must fit signed 32)
 *
 * WHY "- P" FOR PC-RELATIVE. An x86 `call rel32` / `lea disp32(%rip)` does not
 * store a target address; it stores the DISTANCE from the instruction to the
 * target. The CPU computes target = (address of next instruction) + disp32. The
 * compiler folds the "next instruction" adjustment into the addend (that is the
 * -4 you see on a `call`: r_offset points at the disp32, four bytes before the
 * instruction ends). So disp = S + A - P lands the CPU exactly on S.
 *
 * WHY THE RANGE CHECKS. The PC32/32/32S fields are only 32 bits. If the linker
 * places code and its target more than +/-2 GiB apart, the true displacement no
 * longer fits and silently wrapping it would send `call` into hyperspace. A
 * real linker stops with "relocation truncated to fit"; we return an error code.
 * ===========================================================================
 */

/* Our own fixed-width types — no <stdint.h>. On the x86-64 LP64 ABI these
 * widths are guaranteed: int=32, long long=64. A linker patches raw bytes, so
 * the exact widths are load-bearing. */
typedef unsigned char      u8;
typedef unsigned int       u32;
typedef unsigned long long u64;
typedef signed   int       i32;
typedef signed   long long i64;

/* The relocation type ids we implement (from the psABI). */
#define R_X86_64_64     1
#define R_X86_64_PC32   2
#define R_X86_64_PLT32  4
#define R_X86_64_32    10
#define R_X86_64_32S   11

/* Return codes from apply_reloc(). */
#define RL_OK           0   /* patched successfully                           */
#define RL_TRUNC        1   /* value did not fit the field ("truncated to fit") */
#define RL_BADTYPE      2   /* relocation type not supported here             */

/* The signed 32-bit range, spelled out so we depend on no headers. */
#define I32_MIN (-2147483647 - 1)
#define I32_MAX ( 2147483647)

/* ---------------------------------------------------------------------------
 * Little-endian stores. x86-64 ELF fields are little-endian, and we NEVER cast
 * `loc` to a wider pointer (u32 or u64) - the patch site can be unaligned (a
 * disp32 sits at an odd offset inside an instruction), and doing it byte-by-
 * byte is both correct and endianness-explicit, the whole point of a linker.
 * ------------------------------------------------------------------------- */
static void put32(u8 *p, u32 v) {
    p[0] = (u8)(v      );
    p[1] = (u8)(v >>  8);
    p[2] = (u8)(v >> 16);
    p[3] = (u8)(v >> 24);
}
static void put64(u8 *p, u64 v) {
    put32(p,     (u32)(v      ));   /* low  4 bytes                            */
    put32(p + 4, (u32)(v >> 32));   /* high 4 bytes                            */
}

/* ===========================================================================
 * apply_reloc — patch ONE relocation. This is the function whose assembly we
 * annotate in asm/demo.annotated.s.
 *
 * ABI (System V AMD64), by which the caller passes arguments:
 *   rdi = loc  (pointer to the field bytes inside the output image)
 *   esi = type (one of R_X86_64_*)
 *   rdx = S    (symbol's final virtual address)
 *   rcx = A    (addend, signed)
 *   r8  = P    (virtual address of the field itself; ignored for absolute)
 *   -> eax = RL_OK / RL_TRUNC / RL_BADTYPE
 * ===========================================================================
 */
int apply_reloc(u8 *loc, u32 type, u64 S, i64 A, u64 P) {
    switch (type) {

    case R_X86_64_64:
        /* Absolute 64-bit pointer: the field simply becomes S + A. No range
         * check — 64 bits holds any address in the process. This is what a
         * function pointer or a vtable slot in .data receives. */
        put64(loc, S + (u64)A);
        return RL_OK;

    case R_X86_64_PC32:
    case R_X86_64_PLT32: {
        /* PC-relative 32-bit displacement: disp = S + A - P. PLT32 is treated
         * identically because in a STATIC link there is no PLT — the call binds
         * straight to S. Compute in signed 64 bits first so we can detect a
         * displacement too large for the 4-byte field. */
        i64 v = (i64)(S + (u64)A) - (i64)P;
        if (v < I32_MIN || v > I32_MAX)
            return RL_TRUNC;
        put32(loc, (u32)(i32)v);        /* narrow to 32 bits, two's complement */
        return RL_OK;
    }

    case R_X86_64_32: {
        /* Absolute, zero-extended into 32 bits. Valid only if the address fits
         * unsigned 32-bit — true for our 0x400000-based non-PIE image, false in
         * general (which is why PIE code never uses this). */
        u64 v = S + (u64)A;
        if (v > 0xffffffffULL)
            return RL_TRUNC;
        put32(loc, (u32)v);
        return RL_OK;
    }

    case R_X86_64_32S: {
        /* Absolute, sign-extended into 32 bits: the value must fit signed 32.
         * This is what `mov $sym, %reg` and absolute memory operands emit under
         * -fno-pic in the low ("small") code model. */
        i64 v = (i64)(S + (u64)A);
        if (v < I32_MIN || v > I32_MAX)
            return RL_TRUNC;
        put32(loc, (u32)(i32)v);
        return RL_OK;
    }

    default:
        /* GOTPCREL, TLS, TPOFF, ... belong to the DYNAMIC linker's job and are
         * out of scope for this static teaching core. Refuse rather than mangle. */
        return RL_BADTYPE;
    }
}

/* ===========================================================================
 * demo_selfcheck — a header-free, syscall-free exercise of apply_reloc so the
 * generated assembly has real control flow to read, and so a caller (or a unit
 * test) can confirm the arithmetic without a full ELF. It fabricates a tiny
 * 16-byte "output image", patches it the way the linker would, and folds the
 * result into a checksum.
 *
 * Returns the checksum. With the constants below the answer is fixed, so any
 * change to apply_reloc that breaks the math changes the return value.
 * ===========================================================================
 */
u64 demo_selfcheck(void) {
    u8 img[16];
    for (int i = 0; i < 16; i++) img[i] = 0;

    /* Scenario 1: a `call` instruction at vaddr 0x401000. Its opcode 0xE8 sits
     * at img[0]; the 4-byte displacement it patches sits at img[1]. The callee
     * S is at 0x401234. The addend is -4 because r_offset (the disp) is 4 bytes
     * before the end of the 5-byte instruction, and the CPU adds disp to the
     * address of the NEXT instruction. So P = 0x401000 + 1 (the disp's vaddr).
     * Expected disp = S + A - P = 0x401234 - 4 - 0x401001 = 0x22F. */
    img[0] = 0xE8;
    (void)apply_reloc(&img[1], R_X86_64_PC32,
                      0x401234ULL, /*A=*/-4, /*P=*/0x401001ULL);

    /* Scenario 2: an absolute 8-byte pointer at img[8] that should hold the
     * address 0x401234 (e.g. a relocated function pointer in .data). */
    (void)apply_reloc(&img[8], R_X86_64_64, 0x401234ULL, 0, 0);

    /* Fold all 16 bytes into a rolling checksum so the optimizer cannot discard
     * the stores; the exact multiplier is arbitrary. */
    u64 sum = 0;
    for (int i = 0; i < 16; i++)
        sum = sum * 131u + img[i];
    return sum;
}
