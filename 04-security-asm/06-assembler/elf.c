/* ===========================================================================
 * elf.c — write a RELOCATABLE ELF64 object (.o) that `ld` can link.
 * ===========================================================================
 *
 * A relocatable object is not a program you can run; it is raw material for the
 * linker. Its job is to carry, in a standard container:
 *
 *     .text / .data   the assembled bytes
 *     .symtab         a table of names -> (which section, what offset)
 *     .strtab         the characters those names are made of
 *     .rela.text      "patch these spots at link time" notes
 *     .shstrtab       the names of the sections themselves
 *
 * Getting a single field wrong here produces a file the linker rejects with an
 * unhelpful error, so every struct below is written FIELD BY FIELD, little-
 * endian, with the byte size of each field spelled out. We never memcpy a C
 * struct into the file — struct padding and host endianness must not leak in.
 *
 * FIXED SECTION LAYOUT (indices are referenced by sh_link / st_shndx):
 *     0  NULL         (every ELF's section 0 is the null section)
 *     1  .text        code
 *     2  .data        initialised data
 *     3  .shstrtab    section-name strings   (e_shstrndx points here)
 *     4  .symtab      symbols                (sh_link -> 5, sh_info -> 1st global)
 *     5  .strtab      symbol-name strings
 *     6  .rela.text   relocations for .text  (sh_link -> 4, sh_info -> 1)
 * ===========================================================================
 */
#include "asm.h"
#include <stdio.h>
#include <string.h>

/* ELF constants (subset). Named exactly as in <elf.h> for cross-reference. */
#define ET_REL            1
#define EM_X86_64        62
#define EV_CURRENT        1
#define ELFCLASS64        2
#define ELFDATA2LSB       1

#define SHT_PROGBITS      1
#define SHT_SYMTAB        2
#define SHT_STRTAB        3
#define SHT_RELA          4

#define SHF_WRITE         0x1
#define SHF_ALLOC         0x2
#define SHF_EXECINSTR     0x4

#define STB_LOCAL         0
#define STB_GLOBAL        1
#define STT_NOTYPE        0
#define STT_SECTION       3

/* Our two logical sections map to these ELF section-header indices. */
static uint16_t elf_shndx(int section)
{
    return (section == SEC_TEXT) ? 1 : 2;   /* SEC_DATA -> 2                   */
}

/* Round `x` up to a multiple of `a` (a is a power of two here). */
static uint64_t align_up(uint64_t x, uint64_t a) { return (x + a - 1) / a * a; }

/* Add a NUL-terminated string to a string table, returning its byte offset.
 * A string table always begins with a single NUL so that offset 0 == "". */
static uint32_t strtab_add(Buf *b, const char *s)
{
    if (b->len == 0) buf_u8(b, 0);          /* reserve offset 0 for ""        */
    if (s == NULL || s[0] == '\0') return 0;
    uint32_t off = (uint32_t)b->len;
    buf_bytes(b, s, strlen(s) + 1);         /* include the terminating NUL    */
    return off;
}

/* --- fixed-width record writers (match the ELF64 struct layouts) ----------- */

static void put_sym(Buf *b, uint32_t name, uint8_t info, uint8_t other,
                    uint16_t shndx, uint64_t value, uint64_t size)
{
    buf_u32(b, name);       /* st_name  : offset into .strtab                 */
    buf_u8 (b, info);       /* st_info  : (bind<<4)|type                      */
    buf_u8 (b, other);      /* st_other : visibility (0 = default)            */
    buf_u16(b, shndx);      /* st_shndx : section index, or 0 = UNDEF         */
    buf_u64(b, value);      /* st_value : offset within its section           */
    buf_u64(b, size);       /* st_size  : 0 (we don't track sizes)            */
}

static void put_rela(Buf *b, uint64_t off, uint64_t info, int64_t addend)
{
    buf_u64(b, off);            /* r_offset : where in .text to patch          */
    buf_u64(b, info);           /* r_info   : (sym_index<<32)|type             */
    buf_u64(b, (uint64_t)addend);/* r_addend: explicit addend (RELA)           */
}

static void put_shdr(Buf *f, uint32_t name, uint32_t type, uint64_t flags,
                     uint64_t addr, uint64_t offset, uint64_t size,
                     uint32_t link, uint32_t info, uint64_t align, uint64_t entsize)
{
    buf_u32(f, name);   buf_u32(f, type);  buf_u64(f, flags); buf_u64(f, addr);
    buf_u64(f, offset); buf_u64(f, size);  buf_u32(f, link);  buf_u32(f, info);
    buf_u64(f, align);  buf_u64(f, entsize);
}

int write_elf(Assembler *A, const char *outpath)
{
    Buf symtab, strtab, rela, shstr, f;
    buf_init(&symtab); buf_init(&strtab); buf_init(&rela);
    buf_init(&shstr);  buf_init(&f);

    /* ---- 1. Section-name string table + remember each name's offset ------- */
    (void)strtab_add(&shstr, "");                 /* offset 0 = ""            */
    uint32_t n_text = strtab_add(&shstr, ".text");
    uint32_t n_data = strtab_add(&shstr, ".data");
    uint32_t n_shs  = strtab_add(&shstr, ".shstrtab");
    uint32_t n_sym  = strtab_add(&shstr, ".symtab");
    uint32_t n_str  = strtab_add(&shstr, ".strtab");
    uint32_t n_rela = strtab_add(&shstr, ".rela.text");

    /* ---- 2. Build the symbol table (LOCALS FIRST, then GLOBALS) ----------- */
    /* ELF requires all local symbols to precede global/weak ones, and the
     * symtab's sh_info to be the index of the first global. We honour that by
     * writing in two waves and recording where the globals begin.            */
    int idx = 0;

    /* [0] the mandatory null symbol */
    put_sym(&symtab, 0, 0, 0, 0, 0, 0);           idx++;

    /* [1],[2] section symbols for .text and .data (STT_SECTION, local). Real
     * assemblers route relocations against local labels through these; masm
     * references the named symbols directly, but we still emit the section
     * symbols so the object matches what tools expect to see. */
    put_sym(&symtab, 0, (STB_LOCAL<<4)|STT_SECTION, 0, 1, 0, 0); idx++;
    put_sym(&symtab, 0, (STB_LOCAL<<4)|STT_SECTION, 0, 2, 0, 0); idx++;

    /* local defined labels */
    for (int i = 0; i < A->nsyms; i++) {
        Symbol *s = &A->syms[i];
        if (!(s->defined && !s->is_global)) continue;
        s->symidx = idx++;
        put_sym(&symtab, strtab_add(&strtab, s->name),
                (STB_LOCAL<<4)|STT_NOTYPE, 0, elf_shndx(s->section), s->value, 0);
    }

    int first_global = idx;                       /* everything after is global */

    /* global symbols: defined-global first, then undefined externals (order
     * among globals is unconstrained; this grouping just reads nicely). */
    for (int i = 0; i < A->nsyms; i++) {
        Symbol *s = &A->syms[i];
        if (!(s->is_global && s->defined)) continue;
        s->symidx = idx++;
        put_sym(&symtab, strtab_add(&strtab, s->name),
                (STB_GLOBAL<<4)|STT_NOTYPE, 0, elf_shndx(s->section), s->value, 0);
    }
    for (int i = 0; i < A->nsyms; i++) {
        Symbol *s = &A->syms[i];
        if (s->defined) continue;                 /* undefined => external      */
        s->symidx = idx++;
        put_sym(&symtab, strtab_add(&strtab, s->name),
                (STB_GLOBAL<<4)|STT_NOTYPE, 0, 0 /*UNDEF*/, 0, 0);
    }

    /* ---- 3. Relocations, now that every symbol has an index --------------- */
    for (int i = 0; i < A->nrelocs; i++) {
        Reloc *r = &A->relocs[i];
        Symbol *s = sym_find(A, r->sym);
        if (!s) { fprintf(stderr, "masm: internal: reloc for unknown '%s'\n", r->sym);
                  A->errors++; continue; }
        uint64_t info = ((uint64_t)s->symidx << 32) | (uint64_t)r->type;
        put_rela(&rela, r->offset, info, r->addend);
    }

    /* ---- 4. Compute file offsets (same alignment sequence we write below) - */
    uint64_t o = 64;                              /* ELF header is 64 bytes    */
    uint64_t off_text = align_up(o, 16); o = off_text + A->sec[SEC_TEXT].len;
    uint64_t off_data = align_up(o, 8);  o = off_data + A->sec[SEC_DATA].len;
    uint64_t off_sym  = align_up(o, 8);  o = off_sym  + symtab.len;
    uint64_t off_str  = o;               o = off_str  + strtab.len;   /* align 1 */
    uint64_t off_rela = align_up(o, 8);  o = off_rela + rela.len;
    uint64_t off_shs  = o;               o = off_shs  + shstr.len;    /* align 1 */
    uint64_t shoff    = align_up(o, 8);           /* section header table here  */

    /* ---- 5. Emit the ELF header ------------------------------------------ */
    buf_u8(&f, 0x7f); buf_u8(&f,'E'); buf_u8(&f,'L'); buf_u8(&f,'F'); /* magic  */
    buf_u8(&f, ELFCLASS64);   /* EI_CLASS  : 64-bit                            */
    buf_u8(&f, ELFDATA2LSB);  /* EI_DATA   : little-endian                     */
    buf_u8(&f, EV_CURRENT);   /* EI_VERSION                                    */
    buf_u8(&f, 0);            /* EI_OSABI  : System V                          */
    for (int i = 0; i < 8; i++) buf_u8(&f, 0);   /* EI_ABIVERSION + padding     */

    buf_u16(&f, ET_REL);      /* e_type    : relocatable object                */
    buf_u16(&f, EM_X86_64);   /* e_machine : AMD64                             */
    buf_u32(&f, EV_CURRENT);  /* e_version                                     */
    buf_u64(&f, 0);           /* e_entry   : 0 (not an executable)             */
    buf_u64(&f, 0);           /* e_phoff   : no program headers                */
    buf_u64(&f, shoff);       /* e_shoff   : section header table file offset  */
    buf_u32(&f, 0);           /* e_flags                                       */
    buf_u16(&f, 64);          /* e_ehsize                                      */
    buf_u16(&f, 0);           /* e_phentsize                                   */
    buf_u16(&f, 0);           /* e_phnum                                       */
    buf_u16(&f, 64);          /* e_shentsize : each section header is 64 bytes */
    buf_u16(&f, 7);           /* e_shnum     : 7 sections (indices 0..6)       */
    buf_u16(&f, 3);           /* e_shstrndx  : .shstrtab is section 3          */

    /* ---- 6. Emit section DATA, padding to the offsets computed above ------ */
    buf_align(&f, 16, 0); buf_bytes(&f, A->sec[SEC_TEXT].data, A->sec[SEC_TEXT].len);
    buf_align(&f, 8,  0); buf_bytes(&f, A->sec[SEC_DATA].data, A->sec[SEC_DATA].len);
    buf_align(&f, 8,  0); buf_bytes(&f, symtab.data, symtab.len);
    /*      strtab is byte-aligned */ buf_bytes(&f, strtab.data, strtab.len);
    buf_align(&f, 8,  0); buf_bytes(&f, rela.data,   rela.len);
    /*      shstrtab byte-aligned  */ buf_bytes(&f, shstr.data,  shstr.len);

    /* ---- 7. Emit the SECTION HEADER TABLE --------------------------------- */
    buf_align(&f, 8, 0);      /* f.len now equals `shoff`                      */
    /* 0: NULL */
    put_shdr(&f, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0);
    /* 1: .text  (alloc + executable) */
    put_shdr(&f, n_text, SHT_PROGBITS, SHF_ALLOC|SHF_EXECINSTR, 0,
             off_text, A->sec[SEC_TEXT].len, 0, 0, 16, 0);
    /* 2: .data  (alloc + writable) */
    put_shdr(&f, n_data, SHT_PROGBITS, SHF_ALLOC|SHF_WRITE, 0,
             off_data, A->sec[SEC_DATA].len, 0, 0, 8, 0);
    /* 3: .shstrtab */
    put_shdr(&f, n_shs, SHT_STRTAB, 0, 0, off_shs, shstr.len, 0, 0, 1, 0);
    /* 4: .symtab  (link=.strtab(5); info=index of first global) */
    put_shdr(&f, n_sym, SHT_SYMTAB, 0, 0, off_sym, symtab.len,
             5, (uint32_t)first_global, 8, 24);
    /* 5: .strtab */
    put_shdr(&f, n_str, SHT_STRTAB, 0, 0, off_str, strtab.len, 0, 0, 1, 0);
    /* 6: .rela.text  (link=.symtab(4); info=.text(1)) */
    put_shdr(&f, n_rela, SHT_RELA, 0, 0, off_rela, rela.len, 4, 1, 8, 24);

    /* ---- 8. Flush to disk (BINARY mode: never translate bytes) ------------ */
    int rc = 0;
    FILE *fp = fopen(outpath, "wb");              /* "wb" is essential on Win32 */
    if (!fp) { fprintf(stderr, "masm: cannot open %s for writing\n", outpath); rc = 1; }
    else {
        if (fwrite(f.data, 1, f.len, fp) != f.len) {
            fprintf(stderr, "masm: short write to %s\n", outpath); rc = 1;
        }
        fclose(fp);
    }

    buf_free(&symtab); buf_free(&strtab); buf_free(&rela);
    buf_free(&shstr);  buf_free(&f);
    return rc;
}
