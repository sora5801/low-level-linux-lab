/* ===========================================================================
 * elftk.h — the shared model of a loaded ELF file, plus subcommand prototypes.
 * ===========================================================================
 *
 * The whole toolkit works over a single read-only memory mapping of the input
 * file. `struct elf_file` is that mapping plus the handful of pre-resolved
 * pointers (section header table, string tables, symbol tables) that every
 * subcommand needs. Building those pointers ONCE, with bounds checks, is what
 * keeps readelf.c / nm.c / objdump.c short and free of defensive noise.
 *
 * Trust boundary: the input is an UNTRUSTED file. A malformed or malicious ELF
 * can point its tables anywhere, overlap them, or claim absurd sizes. So every
 * offset from the file is validated against the mapping bounds before we form a
 * pointer to it (see ef_ptr / ef_fits in elftk.c). This is the security lesson
 * that hides inside every parser: never trust a length field.
 * =========================================================================== */
#ifndef ELFTK_H
#define ELFTK_H

#include <stddef.h>   /* size_t                                                 */
#include <stdint.h>   /* uint8_t/uint64_t for the driver side (not the on-disk   */
                      /*   structs, which live in elf.h with their own widths)   */
#include "elf.h"

/* A fully-parsed, ready-to-query ELF image. All pointers alias into `data`;
 * nothing here owns heap memory except what the caller frees via ef_close(). */
struct elf_file {
    const char       *path;     /* for diagnostics                              */
    const uint8_t    *data;     /* mmap base (PROT_READ), the whole file        */
    size_t            size;     /* file size in bytes                           */

    const Elf64_Ehdr *eh;       /* == data, after magic/class validation        */

    const Elf64_Shdr *shdr;     /* section header table (may be NULL)           */
    unsigned          shnum;    /* resolved count (handles the e_shnum==0 quirk)*/
    const char       *shstr;    /* .shstrtab base (section name strings)        */
    size_t            shstrsz;

    /* The two symbol tables we care about, resolved by scanning section types.
     * Either may be absent (a stripped binary has no .symtab). */
    const Elf64_Sym  *symtab;   /* .symtab (full/link-time symbols)             */
    unsigned          symcount;
    const char       *symstr;   /* .strtab linked from .symtab                  */
    size_t            symstrsz;

    const Elf64_Sym  *dynsym;   /* .dynsym (runtime symbols)                    */
    unsigned          dyncount;
    const char       *dynstr;   /* .dynstr linked from .dynsym                  */
    size_t            dynstrsz;
};

/* --- loader (elftk.c) ------------------------------------------------------- */
/* Map `path`, validate it as an ELF64 LSB image, and fill `f`. Returns 0 on
 * success, -1 on error (message already printed to stderr). */
int  ef_open(const char *path, struct elf_file *f);
void ef_close(struct elf_file *f);

/* Bounds-checked helpers shared by every subcommand. */
/* True if the byte range [off, off+len) lies wholly inside the mapping. */
int          ef_fits(const struct elf_file *f, uint64_t off, uint64_t len);
/* Return a pointer to file offset `off` if [off,off+len) fits, else NULL. */
const void  *ef_ptr(const struct elf_file *f, uint64_t off, uint64_t len);
/* Safe string-table lookup: returns a pointer to the NUL-terminated string at
 * `strtab+idx`, or "" if it would run past the table. Never over-reads. */
const char  *ef_str(const char *strtab, size_t strsz, uint64_t idx);
/* Name of section header `sh` via .shstrtab (or "" if out of range). */
const char  *ef_secname(const struct elf_file *f, const Elf64_Shdr *sh);

/* --- subcommands ------------------------------------------------------------ */
/* readelf.c — the readelf(1)-style dumps. Each returns 0/-1. */
int re_file_header(const struct elf_file *f);
int re_section_headers(const struct elf_file *f);
int re_program_headers(const struct elf_file *f);
int re_dynamic(const struct elf_file *f);
int re_relocations(const struct elf_file *f);
int re_symbols(const struct elf_file *f);
int re_all(const struct elf_file *f);

/* nm.c — nm(1)-style symbol listing. sort_by_addr picks -n behaviour. */
int nm_list(const struct elf_file *f, int sort_by_addr);

/* objdump.c — objdump -d lite + addr2line. */
int od_disasm(const struct elf_file *f);          /* linear-sweep the .text     */
int od_addr2line(const struct elf_file *f, uint64_t addr); /* symbolize one addr */

#endif /* ELFTK_H */
