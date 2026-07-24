/* ===========================================================================
 * elf.h — a self-contained ELF64 layout, written from the spec (no <elf.h>).
 * ===========================================================================
 *
 * WHY WRITE OUR OWN ELF HEADER?
 * ----------------------------
 * The system <elf.h> exists, but pulling it in would hide the very thing this
 * project is about: the *exact byte layout* of an ELF file. Every struct below
 * is laid out to match the on-disk/in-memory image bit-for-bit, so that
 *
 *      const Elf64_Ehdr *eh = (const Elf64_Ehdr *)mapped_file_base;
 *
 * is a valid, zero-copy reinterpretation of the first 64 bytes of the file.
 * That only works because:
 *
 *   1. ELF is defined with FIXED-WIDTH fields (no `int`, no padding surprises).
 *      We reproduce those widths with typedefs that are the same size on every
 *      LP64 target: `unsigned long long` is 64-bit everywhere we care about.
 *   2. Every field here happens to be naturally aligned inside the struct, so a
 *      well-formed compiler inserts ZERO padding. (ELF was designed this way on
 *      purpose — Addr/Off are 8-byte aligned within the header.) We still parse
 *      defensively rather than trusting alignment blindly; see elftk.c.
 *   3. We only support ELFCLASS64 + ELFDATA2LSB (little-endian 64-bit), which is
 *      what x86-64 Linux emits. On a big-endian host you would have to byte-swap
 *      every multi-byte field; we document that limit in the README rather than
 *      pretend to handle it.
 *
 * This header has NO #includes on purpose, so it can also be fed to the
 * cross-compiler in isolation. The struct sizes are asserted at the bottom.
 * ===========================================================================
 */
#ifndef ELFTK_ELF_H
#define ELFTK_ELF_H

/* --- fixed-width primitives, per the ELF64 spec (Fig 1-2 of the gABI) -------
 * The names mirror the official <elf.h> so anyone who has read the spec feels
 * at home. We avoid <stdint.h> so this file stands alone for asm generation. */
typedef unsigned char      Elf64_uchar;  /* one byte, e_ident[] entries        */
typedef unsigned short     Elf64_Half;   /* 16-bit unsigned (e_type, e_machine)*/
typedef unsigned int       Elf64_Word;   /* 32-bit unsigned (sh_type, flags…)  */
typedef int                Elf64_Sword;  /* 32-bit signed                       */
typedef unsigned long long Elf64_Xword;  /* 64-bit unsigned (sizes, flags)      */
typedef long long          Elf64_Sxword; /* 64-bit signed   (r_addend)          */
typedef unsigned long long Elf64_Addr;   /* 64-bit program virtual address      */
typedef unsigned long long Elf64_Off;    /* 64-bit file offset                  */
typedef unsigned short     Elf64_Section;/* section index (st_shndx)            */

/* --- e_ident[] indices: the 16-byte magic/identification prologue -----------
 * These are file OFFSETS into the very first bytes of the file. The kernel's
 * ELF loader reads exactly these before it trusts anything else. */
#define EI_MAG0        0   /* 0x7f                                              */
#define EI_MAG1        1   /* 'E'                                               */
#define EI_MAG2        2   /* 'L'                                               */
#define EI_MAG3        3   /* 'F'                                               */
#define EI_CLASS       4   /* 1=ELFCLASS32, 2=ELFCLASS64                        */
#define EI_DATA        5   /* 1=LSB (little-endian), 2=MSB (big-endian)         */
#define EI_VERSION     6   /* must be 1 (EV_CURRENT)                            */
#define EI_OSABI       7   /* 0=SysV, 3=Linux, …                               */
#define EI_ABIVERSION  8
#define EI_PAD         9   /* padding, must be zero                            */
#define EI_NIDENT     16   /* sizeof(e_ident)                                  */

#define ELFMAG0     0x7f
#define ELFMAG1      'E'
#define ELFMAG2      'L'
#define ELFMAG3      'F'

#define ELFCLASSNONE 0
#define ELFCLASS32   1
#define ELFCLASS64   2

#define ELFDATANONE  0
#define ELFDATA2LSB  1   /* two's-complement, little-endian (x86-64)           */
#define ELFDATA2MSB  2

#define EV_CURRENT   1

/* --- e_type: object file kind ---------------------------------------------- */
#define ET_NONE   0
#define ET_REL    1   /* relocatable (.o) — has SHT_RELA, no program headers   */
#define ET_EXEC   2   /* executable, fixed load address (-no-pie)              */
#define ET_DYN    3   /* shared object OR a PIE executable (position-indep.)   */
#define ET_CORE   4   /* core dump                                             */

/* --- e_machine: a few common architectures --------------------------------- */
#define EM_NONE    0
#define EM_386     3
#define EM_X86_64 62
#define EM_ARM    40
#define EM_AARCH64 183
#define EM_RISCV  243

/* ===========================================================================
 * The ELF header — the first 64 bytes of every ELF64 file.
 * ===========================================================================
 * Field-by-field (byte offset : size):
 *   0  : 16  e_ident      magic + class/data/version identification
 *   16 : 2   e_type       ET_* (REL/EXEC/DYN/CORE)
 *   18 : 2   e_machine    EM_* target ISA
 *   20 : 4   e_version    EV_CURRENT
 *   24 : 8   e_entry      virtual address of the entry point (_start)
 *   32 : 8   e_phoff      file offset of the PROGRAM header table (0 if none)
 *   40 : 8   e_shoff      file offset of the SECTION header table (0 if none)
 *   48 : 4   e_flags      processor-specific flags (0 on x86-64)
 *   52 : 2   e_ehsize     size of THIS header (== 64 for ELF64)
 *   54 : 2   e_phentsize  size of one program header entry (== 56)
 *   56 : 2   e_phnum      number of program headers
 *   58 : 2   e_shentsize  size of one section header entry (== 64)
 *   60 : 2   e_shnum      number of section headers (0 => real count in sh[0])
 *   62 : 2   e_shstrndx   section index of the section-name string table
 * =========================================================================== */
typedef struct {
    Elf64_uchar e_ident[EI_NIDENT];
    Elf64_Half  e_type;
    Elf64_Half  e_machine;
    Elf64_Word  e_version;
    Elf64_Addr  e_entry;
    Elf64_Off   e_phoff;
    Elf64_Off   e_shoff;
    Elf64_Word  e_flags;
    Elf64_Half  e_ehsize;
    Elf64_Half  e_phentsize;
    Elf64_Half  e_phnum;
    Elf64_Half  e_shentsize;
    Elf64_Half  e_shnum;
    Elf64_Half  e_shstrndx;
} Elf64_Ehdr;

/* ===========================================================================
 * Section header — describes ONE section (64 bytes). The section header table
 * is an array of these at e_shoff. Sections are the *linker's* view of the file
 * (.text, .data, .symtab, …); they are optional at run time.
 * =========================================================================== */
typedef struct {
    Elf64_Word  sh_name;      /* offset into .shstrtab of this section's name   */
    Elf64_Word  sh_type;      /* SHT_* (PROGBITS/SYMTAB/STRTAB/RELA/…)          */
    Elf64_Xword sh_flags;     /* SHF_* (WRITE/ALLOC/EXECINSTR/…)                */
    Elf64_Addr  sh_addr;      /* virtual address if SHF_ALLOC, else 0           */
    Elf64_Off   sh_offset;    /* file offset of the section's bytes             */
    Elf64_Xword sh_size;      /* size in bytes (in the file, unless SHT_NOBITS) */
    Elf64_Word  sh_link;      /* type-specific link (e.g. SYMTAB -> its STRTAB) */
    Elf64_Word  sh_info;      /* type-specific extra info                       */
    Elf64_Xword sh_addralign; /* required alignment of sh_addr (power of two)   */
    Elf64_Xword sh_entsize;   /* size of one entry if the section is a table    */
} Elf64_Shdr;

/* --- sh_type values --------------------------------------------------------- */
#define SHT_NULL           0  /* inactive; index 0 is always this               */
#define SHT_PROGBITS       1  /* program-defined bytes (.text, .data, .rodata)  */
#define SHT_SYMTAB         2  /* full symbol table (.symtab)                    */
#define SHT_STRTAB         3  /* string table (.strtab/.shstrtab/.dynstr)       */
#define SHT_RELA           4  /* relocations WITH explicit addend (x86-64 uses) */
#define SHT_HASH           5  /* symbol hash table                              */
#define SHT_DYNAMIC        6  /* .dynamic array (Elf64_Dyn[])                   */
#define SHT_NOTE           7  /* .note.* — vendor/ABI notes                     */
#define SHT_NOBITS         8  /* occupies NO file bytes (.bss)                  */
#define SHT_REL            9  /* relocations WITHOUT addend (implicit in target)*/
#define SHT_SHLIB         10
#define SHT_DYNSYM        11  /* minimal dynamic-linking symbol table (.dynsym) */
#define SHT_INIT_ARRAY    14
#define SHT_FINI_ARRAY    15
#define SHT_GNU_HASH  0x6ffffff6 /* GNU-style symbol hash                       */
#define SHT_GNU_VERSYM 0x6fffffff
#define SHT_GNU_VERNEED 0x6ffffffe

/* --- sh_flags bits ---------------------------------------------------------- */
#define SHF_WRITE      0x1  /* writable at run time (.data, .bss)               */
#define SHF_ALLOC      0x2  /* occupies memory during execution                 */
#define SHF_EXECINSTR  0x4  /* executable machine code (.text)                  */
#define SHF_MERGE      0x10
#define SHF_STRINGS    0x20
#define SHF_INFO_LINK  0x40
#define SHF_TLS        0x400

/* --- special section indices ------------------------------------------------ */
#define SHN_UNDEF      0        /* the "no section" / undefined symbol index     */
#define SHN_LORESERVE  0xff00
#define SHN_ABS        0xfff1   /* symbol value is an absolute constant          */
#define SHN_COMMON     0xfff2   /* uninitialized common block                    */
#define SHN_XINDEX     0xffff   /* real index is elsewhere (see e_shnum quirk)   */

/* ===========================================================================
 * Program header — describes ONE segment (56 bytes). The program header table
 * is the *kernel's/loader's* view: it says which byte ranges to mmap where and
 * with what protection. Present in ET_EXEC/ET_DYN, absent in a plain .o.
 * =========================================================================== */
typedef struct {
    Elf64_Word  p_type;   /* PT_* (LOAD/DYNAMIC/INTERP/…)                       */
    Elf64_Word  p_flags;  /* PF_* (R/W/X) — note: comes BEFORE the addresses    */
    Elf64_Off   p_offset; /* file offset of the segment's first byte            */
    Elf64_Addr  p_vaddr;  /* virtual address to map it at                       */
    Elf64_Addr  p_paddr;  /* physical address (unused on Linux; == p_vaddr)     */
    Elf64_Xword p_filesz; /* bytes present in the file                          */
    Elf64_Xword p_memsz;  /* bytes in memory (>= p_filesz; extra is zeroed .bss)*/
    Elf64_Xword p_align;  /* p_vaddr ≡ p_offset (mod p_align)                   */
} Elf64_Phdr;

/* --- p_type values ---------------------------------------------------------- */
#define PT_NULL      0
#define PT_LOAD      1  /* a mappable segment (the ones the kernel actually mmaps)*/
#define PT_DYNAMIC   2  /* points at the .dynamic array for the dynamic linker    */
#define PT_INTERP    3  /* path of the ELF interpreter (ld-linux.so) for dynamics */
#define PT_NOTE      4
#define PT_PHDR      6  /* the program header table describing itself             */
#define PT_TLS       7
#define PT_GNU_EH_FRAME 0x6474e550 /* exception-handling frame index             */
#define PT_GNU_STACK    0x6474e551 /* stack permissions (NX marker)              */
#define PT_GNU_RELRO    0x6474e552 /* make part of the GOT read-only after reloc */
#define PT_GNU_PROPERTY 0x6474e553

/* --- p_flags bits ----------------------------------------------------------- */
#define PF_X 0x1  /* execute */
#define PF_W 0x2  /* write   */
#define PF_R 0x4  /* read    */

/* ===========================================================================
 * Symbol table entry (24 bytes). One per symbol in .symtab/.dynsym.
 *
 * st_info packs the BINDING (LOCAL/GLOBAL/WEAK) in the high nibble and the TYPE
 * (FUNC/OBJECT/…) in the low nibble — that is why the macros below shift/mask.
 * =========================================================================== */
typedef struct {
    Elf64_Word    st_name;   /* offset into the linked STRTAB of the symbol name */
    Elf64_uchar   st_info;   /* bind<<4 | type  (use ELF64_ST_BIND/TYPE)         */
    Elf64_uchar   st_other;  /* visibility in the low 2 bits (STV_*)             */
    Elf64_Section st_shndx;  /* section index this symbol is defined in          */
    Elf64_Addr    st_value;  /* value: a virtual address for defined FUNC/OBJECT */
    Elf64_Xword   st_size;   /* size in bytes of the object/function (0 if unknown)*/
} Elf64_Sym;

/* Pull the binding (high nibble) and type (low nibble) out of st_info. */
#define ELF64_ST_BIND(i)   ((i) >> 4)
#define ELF64_ST_TYPE(i)   ((i) & 0xf)
#define ELF64_ST_VISIBILITY(o) ((o) & 0x3)

/* --- symbol binding (st_info >> 4) ------------------------------------------ */
#define STB_LOCAL   0  /* not visible outside the object file                   */
#define STB_GLOBAL  1  /* visible to all objects being combined                 */
#define STB_WEAK    2  /* like global, but a strong definition overrides it     */

/* --- symbol type (st_info & 0xf) -------------------------------------------- */
#define STT_NOTYPE  0
#define STT_OBJECT  1  /* a data object (variable)                              */
#define STT_FUNC    2  /* a function / executable code                          */
#define STT_SECTION 3  /* the section itself (used by relocations)              */
#define STT_FILE    4  /* source file name                                      */
#define STT_COMMON  5
#define STT_TLS     6
#define STT_GNU_IFUNC 10 /* indirect function (resolver chosen at load time)    */

/* --- symbol visibility (st_other & 0x3) ------------------------------------- */
#define STV_DEFAULT   0
#define STV_INTERNAL  1
#define STV_HIDDEN    2
#define STV_PROTECTED 3

/* ===========================================================================
 * Relocation entries. x86-64 uses the "A" (addend) form exclusively: SHT_RELA.
 * r_info packs the symbol table index (high 32 bits) and the relocation TYPE
 * (low 32 bits). r_addend is a signed constant added into the computation.
 * =========================================================================== */
typedef struct {
    Elf64_Addr   r_offset; /* where to patch: a file/section offset or vaddr    */
    Elf64_Xword  r_info;   /* sym<<32 | type  (use ELF64_R_SYM/TYPE)            */
} Elf64_Rel;

typedef struct {
    Elf64_Addr   r_offset;
    Elf64_Xword  r_info;
    Elf64_Sxword r_addend; /* the explicit "+A" in formulas like S + A          */
} Elf64_Rela;

#define ELF64_R_SYM(i)   ((i) >> 32)          /* symbol table index             */
#define ELF64_R_TYPE(i)  ((i) & 0xffffffffULL) /* architecture reloc type        */

/* A few x86-64 relocation types (from the x86-64 psABI). These name-strings are
 * printed by readelf -r; we cover the ones a normal .o actually contains. */
#define R_X86_64_NONE       0
#define R_X86_64_64         1  /* S + A            (64-bit absolute)             */
#define R_X86_64_PC32       2  /* S + A - P        (32-bit PC-relative)         */
#define R_X86_64_GOT32      3
#define R_X86_64_PLT32      4  /* L + A - P        (call into the PLT)          */
#define R_X86_64_COPY       5
#define R_X86_64_GLOB_DAT   6  /* set a GOT entry to a symbol address           */
#define R_X86_64_JUMP_SLOT  7  /* lazy PLT slot, resolved by the dynamic linker */
#define R_X86_64_RELATIVE   8  /* B + A  (base + addend; no symbol) — PIE rebase */
#define R_X86_64_GOTPCREL   9
#define R_X86_64_32        10
#define R_X86_64_32S       11
#define R_X86_64_16        12
#define R_X86_64_PC16      13
#define R_X86_64_8         14
#define R_X86_64_PC8       15
#define R_X86_64_PC64      24
#define R_X86_64_GOTPCRELX 41  /* relaxable GOTPCREL (linker may rewrite it)    */
#define R_X86_64_REX_GOTPCRELX 42

/* ===========================================================================
 * Dynamic section entry (16 bytes). The .dynamic array is a list of tagged
 * key/value pairs the dynamic linker walks: which libraries to load, where the
 * relocation and symbol tables are, etc. Terminated by a DT_NULL entry.
 * =========================================================================== */
typedef struct {
    Elf64_Sxword d_tag;      /* DT_* key                                        */
    union {
        Elf64_Xword d_val;   /* an integer value (a size, a flags word)         */
        Elf64_Addr  d_ptr;   /* a virtual address (a table location)            */
    } d_un;
} Elf64_Dyn;

/* --- a subset of DT_* tags we decode ---------------------------------------- */
#define DT_NULL     0   /* end of the array                                     */
#define DT_NEEDED   1   /* d_val = .dynstr offset of a needed library name      */
#define DT_PLTRELSZ 2
#define DT_PLTGOT   3
#define DT_HASH     4
#define DT_STRTAB   5   /* address of the dynamic string table                  */
#define DT_SYMTAB   6   /* address of the dynamic symbol table                  */
#define DT_RELA     7   /* address of the RELA relocation table                 */
#define DT_RELASZ   8
#define DT_RELAENT  9
#define DT_STRSZ   10
#define DT_SYMENT  11
#define DT_INIT    12
#define DT_FINI    13
#define DT_SONAME  14
#define DT_RPATH   15
#define DT_SYMBOLIC 16
#define DT_REL     17
#define DT_RELSZ   18
#define DT_RELENT  19
#define DT_PLTREL  20
#define DT_DEBUG   21
#define DT_TEXTREL 22
#define DT_JMPREL  23
#define DT_BIND_NOW 24
#define DT_INIT_ARRAY 25
#define DT_FINI_ARRAY 26
#define DT_INIT_ARRAYSZ 27
#define DT_FINI_ARRAYSZ 28
#define DT_RUNPATH  29
#define DT_FLAGS    30
#define DT_GNU_HASH 0x6ffffef5
#define DT_FLAGS_1  0x6ffffffb
#define DT_RELACOUNT 0x6ffffff9

/* ---------------------------------------------------------------------------
 * Static assertions: if the compiler lays these out with unexpected padding,
 * the whole "cast the mapped bytes" premise is broken, so fail the BUILD, not
 * at run time. _Static_assert is C11; every compiler this repo targets has it.
 * --------------------------------------------------------------------------- */
_Static_assert(sizeof(Elf64_Ehdr) == 64, "Elf64_Ehdr must be 64 bytes");
_Static_assert(sizeof(Elf64_Shdr) == 64, "Elf64_Shdr must be 64 bytes");
_Static_assert(sizeof(Elf64_Phdr) == 56, "Elf64_Phdr must be 56 bytes");
_Static_assert(sizeof(Elf64_Sym)  == 24, "Elf64_Sym must be 24 bytes");
_Static_assert(sizeof(Elf64_Rela) == 24, "Elf64_Rela must be 24 bytes");
_Static_assert(sizeof(Elf64_Rel)  == 16, "Elf64_Rel must be 16 bytes");
_Static_assert(sizeof(Elf64_Dyn)  == 16, "Elf64_Dyn must be 16 bytes");

#endif /* ELFTK_ELF_H */
