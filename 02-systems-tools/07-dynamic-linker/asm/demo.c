/* ===========================================================================
 * demo.c — the relocation-apply inner loop, extracted and self-contained.
 * ===========================================================================
 *
 * This is the single most instructive routine in the dynamic linker: the loop
 * that, for every relocation, computes a target value and writes it into a GOT
 * or PLT slot. It is lifted out of ../loader.c's apply_rela() with all system
 * headers removed and its own types declared, so it compiles to clean x86-64
 * assembly with nothing but:
 *
 *   clang --target=x86_64-pc-linux-gnu -S -O1 \
 *         -fno-asynchronous-unwind-tables -fno-jump-tables \
 *         -fno-omit-frame-pointer demo.c -o demo.s
 *
 * THE ONE FORMULA: every dynamic relocation ultimately writes, at the address
 * P = load_base + r_offset, some function of
 *
 *     S  = the resolved value of the referenced symbol,
 *     A  = the addend (r_addend, the explicit "+A"),
 *     B  = the object's load bias (where it actually got mapped).
 *
 *   R_X86_64_RELATIVE   *P = B + A         (no symbol: just rebase a pointer)
 *   R_X86_64_64         *P = S + A         (absolute 64-bit reference)
 *   R_X86_64_GLOB_DAT   *P = S + A         (data symbol -> a GOT slot)
 *   R_X86_64_JUMP_SLOT  *P = S + A         (function   -> a PLT/GOT slot)
 *   R_X86_64_IRELATIVE  *P = ((fn)(B+A))() (call a resolver, store its result)
 *
 * Watching this compile is the whole lesson: "compute S+A, store to *P" is a
 * few instructions, and a working dynamic linker is that loop plus the
 * bookkeeping to find S. See demo.annotated.s for the line-by-line reading.
 * ========================================================================= */

/* Self-declared fixed-width types (no <stdint.h> in a demo TU). */
typedef unsigned int   u32;
typedef unsigned long  u64;
typedef long           i64;

/* The ELF64 relocation and symbol records, exactly as they sit in the file. */
typedef struct {
    u64 r_offset;   /* where to patch, as a vaddr (P = base + r_offset)  */
    u64 r_info;     /* (symbol index << 32) | relocation type            */
    i64 r_addend;   /* the addend A                                      */
} Elf64_Rela;

typedef struct {
    u32 st_name;    /* name: offset into the string table                */
    unsigned char st_info, st_other;
    unsigned short st_shndx;
    u64 st_value;   /* the symbol's value (address before load bias)     */
    u64 st_size;
} Elf64_Sym;

/* r_info accessors. */
#define R_SYM(i)  ((i) >> 32)
#define R_TYPE(i) ((i) & 0xffffffffUL)

/* The x86-64 relocation types this loop handles. */
#define R_X86_64_NONE       0
#define R_X86_64_64         1
#define R_X86_64_GLOB_DAT   6
#define R_X86_64_JUMP_SLOT  7
#define R_X86_64_RELATIVE   8
#define R_X86_64_IRELATIVE 37

/* Resolve symbol `idx` to its value S. In the real loader this walks the global
 * scope by name; here it does the defining-object computation, S = base +
 * st_value, which is the arithmetic the asm shows. It is deliberately tiny so
 * the generated code stays centred on the S+A store. */
static u64 resolve(u64 base, const Elf64_Sym *symtab, u64 idx)
{
    return base + symtab[idx].st_value;
}

/* Apply `count` RELA relocations for an object loaded at bias `base`. This is
 * the routine we annotate. Returning the number of GOT slots written keeps the
 * loop's result observable (so the optimizer can't delete it). */
u64 apply_relocations(u64 base, const Elf64_Rela *rela, u64 count,
                      const Elf64_Sym *symtab)
{
    u64 written = 0;

    for (u64 i = 0; i < count; i++) {
        const Elf64_Rela *r = &rela[i];
        u64 type = R_TYPE(r->r_info);

        /* P: the GOT/PLT slot (or data word) to patch. */
        u64 *P = (u64 *)(base + r->r_offset);
        i64 A = r->r_addend;

        switch (type) {
        case R_X86_64_NONE:
            continue;                       /* padding; nothing to do        */

        case R_X86_64_RELATIVE:
            *P = base + (u64)A;             /* B + A                         */
            break;

        case R_X86_64_64:
        case R_X86_64_GLOB_DAT:
        case R_X86_64_JUMP_SLOT: {          /* all three are S + A           */
            u64 S = resolve(base, symtab, R_SYM(r->r_info));
            *P = S + (u64)A;
            break;
        }

        case R_X86_64_IRELATIVE: {          /* call resolver at B + A        */
            u64 (*resolver)(void) = (u64 (*)(void))(base + (u64)A);
            *P = resolver();
            break;
        }

        default:
            return written;                 /* unknown type: stop, report    */
        }
        written++;
    }
    return written;
}
