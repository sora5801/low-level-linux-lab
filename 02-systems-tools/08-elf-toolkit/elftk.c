/* ===========================================================================
 * elftk.c — the loader (mmap + validate + index) and the CLI front door.
 * ===========================================================================
 *
 * Platform: Linux / WSL. We use mmap(2) to get the file bytes as one contiguous
 * read-only region, which is exactly how the kernel's own ELF loader sees a
 * binary — and it lets us treat on-disk structures as C structs with zero copies
 * (see elf.h for why that layout is safe).
 *
 * Ownership: ef_open() mmaps the file; ef_close() munmaps it. Nothing else on
 * the struct is heap-allocated, so there is exactly one resource to release.
 * ===========================================================================
 */
#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <fcntl.h>      /* open(2)                                              */
#include <unistd.h>     /* close(2)                                             */
#include <sys/mman.h>   /* mmap(2), munmap(2)                                   */
#include <sys/stat.h>   /* fstat(2)                                            */
#include <errno.h>

#include "elftk.h"

/* ---------------------------------------------------------------------------
 * Bounds helpers. The single most important defensive primitive in a parser:
 * confirm a claimed [off, off+len) window is inside the mapping before we ever
 * dereference it. We use uint64_t math and check for overflow so a hostile
 * `off + len` that wraps around cannot slip past the comparison.
 * --------------------------------------------------------------------------- */
int ef_fits(const struct elf_file *f, uint64_t off, uint64_t len)
{
    if (off > f->size) return 0;               /* start already past the end     */
    if (len > f->size - off) return 0;         /* subtract avoids off+len wrap   */
    return 1;
}

const void *ef_ptr(const struct elf_file *f, uint64_t off, uint64_t len)
{
    if (!ef_fits(f, off, len)) return NULL;
    return f->data + off;
}

const char *ef_str(const char *strtab, size_t strsz, uint64_t idx)
{
    if (!strtab || idx >= strsz) return "";    /* out of range -> empty string   */
    /* The table might not be NUL-terminated by a malicious file; scan-limited by
     * strsz so we never read past it. If no NUL is found we still return the
     * pointer, but callers print with %s only after this guarantees a NUL below. */
    const char *p = strtab + idx;
    const char *end = strtab + strsz;
    for (const char *q = p; q < end; q++)
        if (*q == '\0') return p;              /* found a terminator in-bounds   */
    return "";                                  /* unterminated -> treat as empty */
}

const char *ef_secname(const struct elf_file *f, const Elf64_Shdr *sh)
{
    return ef_str(f->shstr, f->shstrsz, sh->sh_name);
}

/* ---------------------------------------------------------------------------
 * resolve_tables — after the header is validated, find the section header table,
 * the section-name string table, and the two symbol tables. This is where the
 * classic ELF "escape hatch" quirks live, and they are worth understanding:
 *
 *   - e_shnum == 0 but e_shoff != 0  =>  the REAL section count is too big for
 *     the 16-bit field, so it is stored in sh[0].sh_size (section 0 is otherwise
 *     unused). Same idea for e_phnum via PN_XNUM (rare; we don't need it).
 *   - e_shstrndx == SHN_XINDEX (0xffff)  =>  the real string-table index is in
 *     sh[0].sh_link.
 *
 * Real toolchains emit these for objects with tens of thousands of sections; a
 * parser that ignores them silently mis-reads such files.
 * --------------------------------------------------------------------------- */
static int resolve_tables(struct elf_file *f)
{
    const Elf64_Ehdr *eh = f->eh;

    f->shdr = NULL; f->shnum = 0;
    f->shstr = NULL; f->shstrsz = 0;
    f->symtab = f->dynsym = NULL;
    f->symcount = f->dyncount = 0;
    f->symstr = f->dynstr = NULL;
    f->symstrsz = f->dynstrsz = 0;

    if (eh->e_shoff == 0)      /* a fully-linked binary can legitimately lack    */
        return 0;              /* section headers; that is not an error.         */

    /* The section header table must be an array of e_shentsize-byte entries.
     * We only support the standard 64-byte entry. */
    if (eh->e_shentsize != sizeof(Elf64_Shdr)) {
        fprintf(stderr, "elftk: unexpected e_shentsize %u\n", eh->e_shentsize);
        return -1;
    }
    /* Validate at least section 0 so we can read the XINDEX escape values. */
    const Elf64_Shdr *sh0 = ef_ptr(f, eh->e_shoff, sizeof(Elf64_Shdr));
    if (!sh0) { fprintf(stderr, "elftk: section headers out of bounds\n"); return -1; }

    unsigned shnum = eh->e_shnum;
    if (shnum == 0) shnum = (unsigned)sh0->sh_size;   /* the e_shnum==0 escape   */

    /* Now that we know the count, validate the WHOLE table in one shot. */
    if (!ef_fits(f, eh->e_shoff, (uint64_t)shnum * sizeof(Elf64_Shdr))) {
        fprintf(stderr, "elftk: section header table out of bounds\n");
        return -1;
    }
    f->shdr = sh0;
    f->shnum = shnum;

    /* The section-name string table index, with the XINDEX escape. */
    unsigned shstrndx = eh->e_shstrndx;
    if (shstrndx == SHN_XINDEX) shstrndx = sh0->sh_link;
    if (shstrndx != SHN_UNDEF && shstrndx < shnum) {
        const Elf64_Shdr *ss = &f->shdr[shstrndx];
        const void *base = ef_ptr(f, ss->sh_offset, ss->sh_size);
        if (base) { f->shstr = (const char *)base; f->shstrsz = (size_t)ss->sh_size; }
    }

    /* Scan for SYMTAB and DYNSYM sections; each links to its own string table
     * via sh_link. sh_entsize gives the per-symbol size (must be 24). */
    for (unsigned i = 0; i < shnum; i++) {
        const Elf64_Shdr *sh = &f->shdr[i];
        if (sh->sh_type != SHT_SYMTAB && sh->sh_type != SHT_DYNSYM) continue;
        if (sh->sh_entsize != sizeof(Elf64_Sym)) continue;

        const Elf64_Sym *syms = ef_ptr(f, sh->sh_offset, sh->sh_size);
        if (!syms) continue;                    /* skip a corrupt table          */
        unsigned count = (unsigned)(sh->sh_size / sizeof(Elf64_Sym));

        /* Resolve the linked string table (bounds-checked). */
        const char *str = NULL; size_t strsz = 0;
        if (sh->sh_link < shnum) {
            const Elf64_Shdr *st = &f->shdr[sh->sh_link];
            const void *sb = ef_ptr(f, st->sh_offset, st->sh_size);
            if (sb) { str = (const char *)sb; strsz = (size_t)st->sh_size; }
        }

        if (sh->sh_type == SHT_SYMTAB) {
            f->symtab = syms; f->symcount = count;
            f->symstr = str;  f->symstrsz = strsz;
        } else {
            f->dynsym = syms; f->dyncount = count;
            f->dynstr = str;  f->dynstrsz = strsz;
        }
    }
    return 0;
}

/* ---------------------------------------------------------------------------
 * ef_open — map the file and validate the ELF64 identification prologue.
 * --------------------------------------------------------------------------- */
int ef_open(const char *path, struct elf_file *f)
{
    memset(f, 0, sizeof *f);
    f->path = path;

    int fd = open(path, O_RDONLY);
    if (fd < 0) { fprintf(stderr, "elftk: open %s: %s\n", path, strerror(errno)); return -1; }

    struct stat st;
    if (fstat(fd, &st) < 0) {
        fprintf(stderr, "elftk: fstat %s: %s\n", path, strerror(errno));
        close(fd); return -1;
    }
    if (st.st_size < (off_t)sizeof(Elf64_Ehdr)) {
        fprintf(stderr, "elftk: %s: too small to be ELF64\n", path);
        close(fd); return -1;
    }

    /* MAP_PRIVATE + PROT_READ: a copy-on-write read-only view. We never write,
     * so no pages are ever actually copied; the kernel just shares the page
     * cache. The mapping outlives the fd, so we can close(fd) immediately. */
    void *base = mmap(NULL, (size_t)st.st_size, PROT_READ, MAP_PRIVATE, fd, 0);
    close(fd);
    if (base == MAP_FAILED) {
        fprintf(stderr, "elftk: mmap %s: %s\n", path, strerror(errno));
        return -1;
    }
    f->data = (const uint8_t *)base;
    f->size = (size_t)st.st_size;

    /* Validate e_ident: magic, 64-bit class, little-endian data. Our struct
     * casts are only valid for ELFCLASS64 + ELFDATA2LSB (x86-64). */
    const uint8_t *id = f->data;
    if (id[EI_MAG0] != ELFMAG0 || id[EI_MAG1] != ELFMAG1 ||
        id[EI_MAG2] != ELFMAG2 || id[EI_MAG3] != ELFMAG3) {
        fprintf(stderr, "elftk: %s: not an ELF file (bad magic)\n", path);
        ef_close(f); return -1;
    }
    if (id[EI_CLASS] != ELFCLASS64) {
        fprintf(stderr, "elftk: %s: not ELFCLASS64 (this tool is 64-bit only)\n", path);
        ef_close(f); return -1;
    }
    if (id[EI_DATA] != ELFDATA2LSB) {
        fprintf(stderr, "elftk: %s: not little-endian (big-endian unsupported)\n", path);
        ef_close(f); return -1;
    }

    f->eh = (const Elf64_Ehdr *)f->data;   /* now safe to view the header        */
    if (resolve_tables(f) != 0) { ef_close(f); return -1; }
    return 0;
}

void ef_close(struct elf_file *f)
{
    if (f->data) {
        munmap((void *)f->data, f->size);  /* release the mapping                */
        f->data = NULL;
    }
}

/* ===========================================================================
 * CLI — a tiny dispatcher. Usage mirrors readelf/nm/objdump flag names so the
 * mapping to the real tools is obvious.
 * =========================================================================== */
static void usage(const char *argv0)
{
    fprintf(stderr,
        "usage: %s <command> <file> [args]\n"
        "  commands (readelf-style):\n"
        "    -h | header        ELF file header\n"
        "    -S | sections      section headers\n"
        "    -l | segments      program headers + section->segment map\n"
        "    -d | dynamic       the .dynamic array\n"
        "    -r | relocs        relocation tables\n"
        "    -s | symbols       .symtab and .dynsym\n"
        "    -a | all           everything above (readelf -a)\n"
        "  other tools:\n"
        "    nm  [-n]           nm-style symbol list (-n: sort by address)\n"
        "    -D | disasm        linear-sweep disassemble the .text section\n"
        "    addr2line <hex>    print the symbol containing an address\n",
        argv0);
}

int main(int argc, char **argv)
{
    if (argc < 3) { usage(argv[0]); return 2; }
    const char *cmd  = argv[1];
    const char *path = argv[2];

    struct elf_file f;
    if (ef_open(path, &f) != 0) return 1;

    int rc = 0;
    if (!strcmp(cmd, "-h") || !strcmp(cmd, "header"))        rc = re_file_header(&f);
    else if (!strcmp(cmd, "-S") || !strcmp(cmd, "sections")) rc = re_section_headers(&f);
    else if (!strcmp(cmd, "-l") || !strcmp(cmd, "segments")) rc = re_program_headers(&f);
    else if (!strcmp(cmd, "-d") || !strcmp(cmd, "dynamic"))  rc = re_dynamic(&f);
    else if (!strcmp(cmd, "-r") || !strcmp(cmd, "relocs"))   rc = re_relocations(&f);
    else if (!strcmp(cmd, "-s") || !strcmp(cmd, "symbols"))  rc = re_symbols(&f);
    else if (!strcmp(cmd, "-a") || !strcmp(cmd, "all"))      rc = re_all(&f);
    else if (!strcmp(cmd, "nm")) {
        int by_addr = (argc >= 4 && !strcmp(argv[3], "-n"));
        rc = nm_list(&f, by_addr);
    }
    else if (!strcmp(cmd, "-D") || !strcmp(cmd, "disasm"))   rc = od_disasm(&f);
    else if (!strcmp(cmd, "addr2line")) {
        if (argc < 4) { fprintf(stderr, "elftk: addr2line needs an address\n"); rc = -1; }
        else {
            /* Accept 0x-prefixed or bare hex. strtoull with base 0 handles both. */
            uint64_t addr = strtoull(argv[3], NULL, 0);
            rc = od_addr2line(&f, addr);
        }
    }
    else { usage(argv[0]); rc = -1; }

    ef_close(&f);
    return rc == 0 ? 0 : 1;
}
