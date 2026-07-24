/* ===========================================================================
 * nm.c — an nm(1)-style symbol listing.
 * ===========================================================================
 *
 * `nm` boils each symbol down to ONE letter (the "symbol type character"), a
 * value, and a name. The letter is the interesting part: it is computed from
 * the symbol's binding (global vs local => UPPER vs lower case) crossed with
 * WHERE the symbol lives — which is decided by the flags of the section it is
 * defined in, not by any field on the symbol itself. Reproducing that mapping is
 * the whole lesson of this file.
 *
 *   T/t  text (executable) : SHF_ALLOC | SHF_EXECINSTR   (.text)
 *   D/d  initialized data  : SHF_ALLOC | SHF_WRITE, PROGBITS   (.data)
 *   B/b  BSS (zero data)   : SHF_ALLOC | SHF_WRITE, NOBITS     (.bss)
 *   R/r  read-only data    : SHF_ALLOC, not writable           (.rodata)
 *   U    undefined         : referenced here, defined elsewhere (SHN_UNDEF)
 *   w/W  weak              : weak undefined / weak defined
 *   A/a  absolute          : SHN_ABS, a fixed constant, not relocated
 *   C    common            : SHN_COMMON, an uninitialized global tentative def
 * ===========================================================================
 */
#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <inttypes.h>
#include "elftk.h"

/* Decide the nm letter for one symbol. `f` is needed to look at the defining
 * section's flags. The letter is returned already in the right case. */
static char nm_letter(const struct elf_file *f, const Elf64_Sym *s)
{
    unsigned bind = ELF64_ST_BIND(s->st_info);
    int is_global = (bind != STB_LOCAL);
    char c;

    if (s->st_shndx == SHN_UNDEF) {
        /* Undefined: a plain reference is 'U'; a weak reference is 'w' (or 'v'
         * for a weak object). nm shows these lowercase regardless of binding. */
        if (bind == STB_WEAK)
            return (ELF64_ST_TYPE(s->st_info) == STT_OBJECT) ? 'v' : 'w';
        return 'U';
    }
    if (s->st_shndx == SHN_ABS)    return is_global ? 'A' : 'a';
    if (s->st_shndx == SHN_COMMON) return 'C';

    if (bind == STB_WEAK)          /* weak but DEFINED here                      */
        return (ELF64_ST_TYPE(s->st_info) == STT_OBJECT) ? 'V' : 'W';

    /* Defined in a real section: classify by that section's flags. */
    c = '?';
    if (f->shdr && s->st_shndx < f->shnum) {
        const Elf64_Shdr *sh = &f->shdr[s->st_shndx];
        uint64_t fl = sh->sh_flags;
        if (sh->sh_type == SHT_NOBITS && (fl & SHF_ALLOC) && (fl & SHF_WRITE))
            c = 'b';                                   /* .bss                    */
        else if ((fl & SHF_ALLOC) && (fl & SHF_EXECINSTR))
            c = 't';                                   /* .text                   */
        else if ((fl & SHF_ALLOC) && (fl & SHF_WRITE))
            c = 'd';                                   /* .data                   */
        else if (fl & SHF_ALLOC)
            c = 'r';                                   /* .rodata                 */
        else
            c = 'n';                                   /* non-alloc (debug/note)  */
    }
    /* Global symbols get the uppercase form. */
    return is_global ? (char)(c - 'a' + 'A') : c;
}

/* Comparison state passed to qsort via a small context. Because portable qsort
 * has no user-data argument, we stash the pointers in file-scope statics for the
 * duration of the sort — single-threaded tool, so this is safe and simple. */
static const Elf64_Sym *g_syms;
static const char      *g_str;
static size_t           g_strsz;

static int cmp_by_name(const void *pa, const void *pb)
{
    unsigned a = *(const unsigned *)pa, b = *(const unsigned *)pb;
    return strcmp(ef_str(g_str, g_strsz, g_syms[a].st_name),
                  ef_str(g_str, g_strsz, g_syms[b].st_name));
}
static int cmp_by_addr(const void *pa, const void *pb)
{
    unsigned a = *(const unsigned *)pa, b = *(const unsigned *)pb;
    uint64_t va = g_syms[a].st_value, vb = g_syms[b].st_value;
    if (va < vb) return -1;
    if (va > vb) return 1;
    return 0;
}

int nm_list(const struct elf_file *f, int sort_by_addr)
{
    /* Prefer the full .symtab (nm's default). Fall back to .dynsym for a
     * stripped-but-dynamic binary, matching `nm -D` behaviour. */
    const Elf64_Sym *syms = f->symtab; unsigned n = f->symcount;
    const char *str = f->symstr; size_t strsz = f->symstrsz;
    if (!syms) { syms = f->dynsym; n = f->dyncount; str = f->dynstr; strsz = f->dynstrsz; }
    if (!syms || n == 0) {
        fprintf(stderr, "elftk: no symbols (file is stripped?)\n");
        return -1;
    }

    /* Build an index of the symbols we actually print: skip the null symbol 0,
     * unnamed symbols, and FILE/SECTION entries (nm omits those). */
    unsigned *idx = malloc(n * sizeof *idx);
    if (!idx) { perror("malloc"); return -1; }
    unsigned m = 0;
    for (unsigned i = 1; i < n; i++) {
        unsigned t = ELF64_ST_TYPE(syms[i].st_info);
        if (t == STT_FILE || t == STT_SECTION) continue;
        if (ef_str(str, strsz, syms[i].st_name)[0] == '\0') continue;
        idx[m++] = i;
    }

    g_syms = syms; g_str = str; g_strsz = strsz;
    qsort(idx, m, sizeof *idx, sort_by_addr ? cmp_by_addr : cmp_by_name);

    for (unsigned k = 0; k < m; k++) {
        const Elf64_Sym *s = &syms[idx[k]];
        char c = nm_letter(f, s);
        /* Undefined symbols have no value: nm prints spaces where the address
         * would go, then the letter and name. */
        if (s->st_shndx == SHN_UNDEF)
            printf("                 %c %s\n", c, ef_str(str, strsz, s->st_name));
        else
            printf("%016" PRIx64 " %c %s\n",
                   (uint64_t)s->st_value, c, ef_str(str, strsz, s->st_name));
    }

    free(idx);
    return 0;
}
