/* ===========================================================================
 * elf.h — the ELF64 on-disk format, spelled out for a linker to consume.
 * ===========================================================================
 *
 * WHY WE DEFINE THIS OURSELVES (instead of <elf.h>)
 * -------------------------------------------------
 * A linker's entire job is to read the ELF *relocatable* objects a compiler
 * emits (.o, ET_REL) and stitch them into one ELF *executable* (ET_EXEC). To
 * teach that, the reader needs to SEE the byte layout, not have it hidden in a
 * system header. So we declare every structure and every constant we touch,
 * with the field byte-offsets in comments, and we parse/emit fields with the
 * explicit little-endian helpers at the bottom. That makes this file portable
 * to any host (it builds on Windows just as well as Linux) and, more
 * importantly, makes the format legible.
 *
 * ENDIANNESS. x86-64 ELF is ELFDATA2LSB (little-endian). Rather than cast a
 * byte pointer to `Elf64_Shdr*` — which is undefined behaviour on unaligned
 * offsets and silently wrong on a big-endian host — we read each multi-byte
 * field with rd16/rd32/rd64 and write with wr16/wr32/wr64. A linker that
 * parses a hostile/foreign object must never assume host alignment or byte
 * order; doing it by hand is the lesson.
 * ===========================================================================
 */
#ifndef MINILD_ELF_H
#define MINILD_ELF_H

#include <stdint.h>

/* ---------------------------------------------------------------------------
 * e_ident[]: the 16-byte identification prefix that opens every ELF file.
 * The first four bytes are the magic number 0x7f 'E' 'L' 'F'. The loader (and
 * we) reject anything else before trusting a single other byte.
 * ------------------------------------------------------------------------- */
#define EI_MAG0        0   /* 0x7f                                            */
#define EI_MAG1        1   /* 'E'                                             */
#define EI_MAG2        2   /* 'L'                                             */
#define EI_MAG3        3   /* 'F'                                             */
#define EI_CLASS       4   /* 1=ELF32, 2=ELF64                                */
#define EI_DATA        5   /* 1=little-endian (LSB), 2=big-endian (MSB)       */
#define EI_VERSION     6   /* must be EV_CURRENT (1)                          */
#define EI_OSABI       7   /* 0 = System V                                    */
#define EI_NIDENT     16   /* size of e_ident                                 */

#define ELFMAG0     0x7f
#define ELFMAG1      'E'
#define ELFMAG2      'L'
#define ELFMAG3      'F'
#define ELFCLASS64     2   /* we only handle 64-bit objects                   */
#define ELFDATA2LSB    1   /* little-endian                                   */
#define EV_CURRENT     1

/* e_type — what kind of ELF this is. We READ ET_REL and WRITE ET_EXEC. */
#define ET_REL         1   /* relocatable object (.o) — the linker's input    */
#define ET_EXEC        2   /* executable, fixed load address — our output     */
#define ET_DYN         3   /* shared object / PIE (dynamic case, see README)  */

/* e_machine — the ISA. 62 == AMD x86-64. We reject anything else. */
#define EM_X86_64     62

/* ---------------------------------------------------------------------------
 * Section header sh_type — the KIND of a section.
 * ------------------------------------------------------------------------- */
#define SHT_NULL       0   /* inactive header (index 0 is always this)        */
#define SHT_PROGBITS   1   /* program-defined bytes: .text, .rodata, .data    */
#define SHT_SYMTAB     2   /* symbol table (.symtab)                          */
#define SHT_STRTAB     3   /* string table (.strtab / .shstrtab)              */
#define SHT_RELA       4   /* relocations WITH explicit addend (Elf64_Rela)   */
#define SHT_NOBITS     8   /* occupies NO file bytes but DOES occupy memory:  */
                           /*   this is .bss — zero-initialised data          */

/* sh_flags — attributes of a section, OR'd together. */
#define SHF_WRITE     0x1  /* writable at run time      -> data segment (RW)  */
#define SHF_ALLOC     0x2  /* occupies memory at run time (must be loaded)    */
#define SHF_EXECINSTR 0x4  /* executable machine code   -> text segment (RX)  */

/* ---------------------------------------------------------------------------
 * Symbol table: binding (st_info >> 4) and type (st_info & 0xf).
 * ------------------------------------------------------------------------- */
#define STB_LOCAL      0   /* file-local: not visible to other objects        */
#define STB_GLOBAL     1   /* global, STRONG: exactly one definition allowed  */
#define STB_WEAK       2   /* global, WEAK: yields to any strong definition   */

#define STT_NOTYPE     0
#define STT_OBJECT     1   /* a data object (variable)                        */
#define STT_FUNC       2   /* a function                                      */
#define STT_SECTION    3   /* a symbol standing for a whole section: its      */
                           /*   value is the section base. Relocations that   */
                           /*   reference "somewhere in .rodata" use these.   */
#define STT_FILE       4   /* source file name; ignored by the linker         */

/* st_shndx — the special "section indices" a symbol can carry. */
#define SHN_UNDEF      0        /* symbol is UNDEFINED here: must be imported  */
#define SHN_ABS   0xfff1        /* an absolute value, not tied to a section    */
#define SHN_COMMON 0xfff2       /* a tentative (COMMON) definition; see README */

/* Decompose st_info and r_info. These packings are part of the ABI. */
#define ELF64_ST_BIND(i)   ((i) >> 4)
#define ELF64_ST_TYPE(i)   ((i) & 0xf)
#define ELF64_R_SYM(i)     ((uint32_t)((i) >> 32))          /* upper 32 bits  */
#define ELF64_R_TYPE(i)    ((uint32_t)((i) & 0xffffffffUL)) /* lower 32 bits  */
#define ELF64_R_INFO(s,t)  (((uint64_t)(s) << 32) | (uint32_t)(t))

/* ---------------------------------------------------------------------------
 * The x86-64 relocation types this teaching linker applies. Each says: take
 * the symbol's value S and the addend A, maybe subtract the patch address P,
 * and write the result into the field at r_offset. See minild.c apply_reloc().
 *
 *   name          field  formula        meaning
 *   R_X86_64_64     8B    S + A          absolute 64-bit address
 *   R_X86_64_PC32   4B    S + A - P      32-bit PC-relative (e.g. lea foo(%rip))
 *   R_X86_64_PLT32  4B    S + A - P      call foo@PLT; == PC32 once we bind it
 *   R_X86_64_32     4B    S + A          absolute, zero-extended to 32 bits
 *   R_X86_64_32S    4B    S + A          absolute, sign-extended to 32 bits
 * ------------------------------------------------------------------------- */
#define R_X86_64_64     1
#define R_X86_64_PC32   2
#define R_X86_64_PLT32  4
#define R_X86_64_32    10
#define R_X86_64_32S   11

/* ---------------------------------------------------------------------------
 * Program header p_type — how the KERNEL/loader views the file. Sections are
 * for the linker; SEGMENTS (program headers) are for execve(2). We WRITE these.
 * ------------------------------------------------------------------------- */
#define PT_LOAD        1   /* a chunk to mmap into the address space          */
#define PT_INTERP      3   /* path of the dynamic loader (dynamic case only)  */
#define PT_PHDR        6   /* describes the program header table itself       */

/* p_flags — segment permissions. Note the deliberate W^X discipline: a text  */
/* segment is R|X and a data segment is R|W, never R|W|X.                     */
#define PF_X          0x1  /* executable                                      */
#define PF_W          0x2  /* writable                                        */
#define PF_R          0x4  /* readable                                        */

/* ===========================================================================
 * The five structures, as native C structs. We only ever populate these by
 * hand from raw bytes (reading) or serialise them to raw bytes (writing), so
 * their in-memory padding is irrelevant — but the ABI sizes below MUST hold,
 * and every load/store uses the documented byte offset.
 * ===========================================================================
 */

/* Elf64_Ehdr — 64 bytes. The very first thing in the file. */
typedef struct {
    unsigned char e_ident[EI_NIDENT]; /*  0: magic + class/data/version       */
    uint16_t e_type;      /* 16: ET_REL / ET_EXEC                             */
    uint16_t e_machine;   /* 18: EM_X86_64                                    */
    uint32_t e_version;   /* 20: EV_CURRENT                                   */
    uint64_t e_entry;     /* 24: virtual address of the first instruction     */
    uint64_t e_phoff;     /* 32: file offset of the program header table      */
    uint64_t e_shoff;     /* 40: file offset of the section header table      */
    uint32_t e_flags;     /* 48: processor flags (0 for x86-64)               */
    uint16_t e_ehsize;    /* 52: size of THIS header (64)                     */
    uint16_t e_phentsize; /* 54: size of one program header (56)              */
    uint16_t e_phnum;     /* 56: number of program headers                    */
    uint16_t e_shentsize; /* 58: size of one section header (64)              */
    uint16_t e_shnum;     /* 60: number of section headers                    */
    uint16_t e_shstrndx;  /* 62: section index of the section-name strtab     */
} Elf64_Ehdr;             /* 64 total                                         */

/* Elf64_Shdr — 64 bytes. One per section. */
typedef struct {
    uint32_t sh_name;      /*  0: byte offset into shstrtab of the name       */
    uint32_t sh_type;      /*  4: SHT_*                                       */
    uint64_t sh_flags;     /*  8: SHF_*                                       */
    uint64_t sh_addr;      /* 16: run-time vaddr (0 in a .o; we assign it)    */
    uint64_t sh_offset;    /* 24: file offset of the section's bytes          */
    uint64_t sh_size;      /* 32: size in bytes (memsz for NOBITS)            */
    uint32_t sh_link;      /* 40: type-specific link (symtab->strtab, etc.)   */
    uint32_t sh_info;      /* 44: type-specific; for RELA = section it patches */
    uint64_t sh_addralign; /* 48: required alignment of sh_addr/offset        */
    uint64_t sh_entsize;   /* 56: size of one entry for table sections        */
} Elf64_Shdr;              /* 64 total                                        */

/* Elf64_Sym — 24 bytes. One per symbol in .symtab. */
typedef struct {
    uint32_t st_name;   /*  0: offset into the symbol strtab (.strtab)        */
    uint8_t  st_info;   /*  4: bind<<4 | type                                 */
    uint8_t  st_other;  /*  5: visibility; unused here                        */
    uint16_t st_shndx;  /*  6: defining section index, or SHN_UNDEF/ABS/COMMON */
    uint64_t st_value;  /*  8: in a .o, offset within st_shndx's section      */
    uint64_t st_size;   /* 16: size of the object/function                    */
} Elf64_Sym;            /* 24 total                                           */

/* Elf64_Rela — 24 bytes. One per relocation in a SHT_RELA section. */
typedef struct {
    uint64_t r_offset;  /*  0: where to patch: offset within the target sect. */
    uint64_t r_info;    /*  8: symbol index (hi 32) | type (lo 32)            */
    int64_t  r_addend;  /* 16: the constant A, signed                         */
} Elf64_Rela;           /* 24 total                                           */

/* Elf64_Phdr — 56 bytes. One per segment in our OUTPUT. */
typedef struct {
    uint32_t p_type;    /*  0: PT_LOAD, ...                                   */
    uint32_t p_flags;   /*  4: PF_R|PF_W|PF_X                                 */
    uint64_t p_offset;  /*  8: file offset of the segment's first byte        */
    uint64_t p_vaddr;   /* 16: virtual address to map it at                   */
    uint64_t p_paddr;   /* 24: physical addr (unused on Linux; = p_vaddr)     */
    uint64_t p_filesz;  /* 32: bytes present in the file (0 possible: .bss)   */
    uint64_t p_memsz;   /* 40: bytes to reserve in memory (>= p_filesz)       */
    uint64_t p_align;   /* 48: alignment; the kernel needs                    */
                        /*     p_vaddr % p_align == p_offset % p_align        */
} Elf64_Phdr;           /* 56 total                                          */

/* Sizes are load-bearing: we compute file offsets from them. Assert the ABI. */
_Static_assert(sizeof(Elf64_Ehdr) == 64, "Elf64_Ehdr must be 64 bytes");
_Static_assert(sizeof(Elf64_Shdr) == 64, "Elf64_Shdr must be 64 bytes");
_Static_assert(sizeof(Elf64_Sym)  == 24, "Elf64_Sym must be 24 bytes");
_Static_assert(sizeof(Elf64_Rela) == 24, "Elf64_Rela must be 24 bytes");
_Static_assert(sizeof(Elf64_Phdr) == 56, "Elf64_Phdr must be 56 bytes");

/* ---------------------------------------------------------------------------
 * Explicit little-endian byte access. These are the ONLY way we touch
 * multi-byte on-disk fields, so parsing is correct regardless of host byte
 * order or pointer alignment.
 * ------------------------------------------------------------------------- */
static inline uint16_t rd16(const uint8_t *p) {
    return (uint16_t)(p[0] | (p[1] << 8));
}
static inline uint32_t rd32(const uint8_t *p) {
    return (uint32_t)p[0] | ((uint32_t)p[1] << 8)
         | ((uint32_t)p[2] << 16) | ((uint32_t)p[3] << 24);
}
static inline uint64_t rd64(const uint8_t *p) {
    return (uint64_t)rd32(p) | ((uint64_t)rd32(p + 4) << 32);
}
static inline void wr16(uint8_t *p, uint16_t v) {
    p[0] = (uint8_t)v; p[1] = (uint8_t)(v >> 8);
}
static inline void wr32(uint8_t *p, uint32_t v) {
    p[0] = (uint8_t)v;         p[1] = (uint8_t)(v >> 8);
    p[2] = (uint8_t)(v >> 16); p[3] = (uint8_t)(v >> 24);
}
static inline void wr64(uint8_t *p, uint64_t v) {
    wr32(p, (uint32_t)v);
    wr32(p + 4, (uint32_t)(v >> 32));
}

#endif /* MINILD_ELF_H */
