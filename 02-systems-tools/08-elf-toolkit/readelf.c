/* ===========================================================================
 * readelf.c — the readelf(1)-style dumps: header, sections, segments, dynamic,
 *             relocations, and symbol tables.
 * ===========================================================================
 *
 * Every function here READS ONLY the already-validated `struct elf_file`. All
 * table walks re-validate each entry's byte range with ef_ptr/ef_fits before
 * touching it, because a section header can still claim an out-of-bounds body
 * even after the header table itself checked out. The formatting deliberately
 * echoes GNU readelf so you can diff our output against the real tool.
 * ===========================================================================
 */
#include <stdio.h>
#include <inttypes.h>   /* PRIx64 etc. — portable width specifiers for uint64_t */
#include "elftk.h"

/* ===========================================================================
 * Small name-lookup helpers. ELF stores everything as integers; readelf's value
 * is turning them back into the mnemonic names a human reads. Each returns a
 * static string (or fills a caller buffer for the flag-set cases).
 * =========================================================================== */
static const char *et_name(unsigned t)
{
    switch (t) {
    case ET_NONE: return "NONE"; case ET_REL: return "REL (relocatable)";
    case ET_EXEC: return "EXEC (executable)";
    case ET_DYN:  return "DYN (shared object / PIE)";
    case ET_CORE: return "CORE"; default: return "<unknown>";
    }
}
static const char *em_name(unsigned m)
{
    switch (m) {
    case EM_X86_64: return "Advanced Micro Devices X86-64";
    case EM_386:    return "Intel 80386";
    case EM_AARCH64:return "AArch64";
    case EM_ARM:    return "ARM";
    case EM_RISCV:  return "RISC-V";
    default:        return "<unknown>";
    }
}
static const char *sht_name(unsigned t)
{
    switch (t) {
    case SHT_NULL: return "NULL";          case SHT_PROGBITS: return "PROGBITS";
    case SHT_SYMTAB: return "SYMTAB";      case SHT_STRTAB: return "STRTAB";
    case SHT_RELA: return "RELA";          case SHT_HASH: return "HASH";
    case SHT_DYNAMIC: return "DYNAMIC";    case SHT_NOTE: return "NOTE";
    case SHT_NOBITS: return "NOBITS";      case SHT_REL: return "REL";
    case SHT_DYNSYM: return "DYNSYM";      case SHT_INIT_ARRAY: return "INIT_ARRAY";
    case SHT_FINI_ARRAY: return "FINI_ARRAY";
    case SHT_GNU_HASH: return "GNU_HASH";  case SHT_GNU_VERSYM: return "VERSYM";
    case SHT_GNU_VERNEED: return "VERNEED";
    default: return "OTHER";
    }
}
static const char *pt_name(unsigned t)
{
    switch (t) {
    case PT_NULL: return "NULL";      case PT_LOAD: return "LOAD";
    case PT_DYNAMIC: return "DYNAMIC";case PT_INTERP: return "INTERP";
    case PT_NOTE: return "NOTE";      case PT_PHDR: return "PHDR";
    case PT_TLS: return "TLS";
    case PT_GNU_EH_FRAME: return "GNU_EH_FRAME";
    case PT_GNU_STACK: return "GNU_STACK";
    case PT_GNU_RELRO: return "GNU_RELRO";
    case PT_GNU_PROPERTY: return "GNU_PROPERTY";
    default: return "OTHER";
    }
}
static const char *stt_name(unsigned t)
{
    switch (t) {
    case STT_NOTYPE: return "NOTYPE"; case STT_OBJECT: return "OBJECT";
    case STT_FUNC: return "FUNC";     case STT_SECTION: return "SECTION";
    case STT_FILE: return "FILE";     case STT_COMMON: return "COMMON";
    case STT_TLS: return "TLS";       case STT_GNU_IFUNC: return "IFUNC";
    default: return "?";
    }
}
static const char *stb_name(unsigned b)
{
    switch (b) {
    case STB_LOCAL: return "LOCAL"; case STB_GLOBAL: return "GLOBAL";
    case STB_WEAK: return "WEAK";   default: return "?";
    }
}
static const char *stv_name(unsigned v)
{
    switch (v) {
    case STV_DEFAULT: return "DEFAULT"; case STV_INTERNAL: return "INTERNAL";
    case STV_HIDDEN: return "HIDDEN";   case STV_PROTECTED: return "PROTECTED";
    default: return "?";
    }
}
/* x86-64 relocation type names (readelf -r prints these). */
static const char *r_x86_64_name(unsigned t)
{
    switch (t) {
    case R_X86_64_NONE: return "R_X86_64_NONE";
    case R_X86_64_64: return "R_X86_64_64";
    case R_X86_64_PC32: return "R_X86_64_PC32";
    case R_X86_64_GOT32: return "R_X86_64_GOT32";
    case R_X86_64_PLT32: return "R_X86_64_PLT32";
    case R_X86_64_COPY: return "R_X86_64_COPY";
    case R_X86_64_GLOB_DAT: return "R_X86_64_GLOB_DAT";
    case R_X86_64_JUMP_SLOT: return "R_X86_64_JUMP_SLOT";
    case R_X86_64_RELATIVE: return "R_X86_64_RELATIVE";
    case R_X86_64_GOTPCREL: return "R_X86_64_GOTPCREL";
    case R_X86_64_32: return "R_X86_64_32";
    case R_X86_64_32S: return "R_X86_64_32S";
    case R_X86_64_PC64: return "R_X86_64_PC64";
    case R_X86_64_GOTPCRELX: return "R_X86_64_GOTPCRELX";
    case R_X86_64_REX_GOTPCRELX: return "R_X86_64_REX_GOTPCRELX";
    default: return "R_X86_64_<other>";
    }
}

/* Render sh_flags as the compact "WAX" letter set readelf uses. */
static void sh_flag_str(uint64_t fl, char out[8])
{
    int n = 0;
    if (fl & SHF_WRITE)     out[n++] = 'W';
    if (fl & SHF_ALLOC)     out[n++] = 'A';
    if (fl & SHF_EXECINSTR) out[n++] = 'X';
    if (fl & SHF_MERGE)     out[n++] = 'M';
    if (fl & SHF_STRINGS)   out[n++] = 'S';
    if (fl & SHF_INFO_LINK) out[n++] = 'I';
    if (fl & SHF_TLS)       out[n++] = 'T';
    out[n] = '\0';
}

/* ===========================================================================
 * 1. ELF file header — the `readelf -h` view.
 * =========================================================================== */
int re_file_header(const struct elf_file *f)
{
    const Elf64_Ehdr *e = f->eh;

    printf("ELF Header:\n");
    printf("  Magic:  ");
    for (int i = 0; i < EI_NIDENT; i++) printf(" %02x", e->e_ident[i]);
    printf("\n");
    printf("  Class:                             ELF64\n");
    printf("  Data:                              2's complement, little endian\n");
    printf("  Version:                           %u\n", e->e_ident[EI_VERSION]);
    printf("  OS/ABI:                            %u\n", e->e_ident[EI_OSABI]);
    printf("  Type:                              %s\n", et_name(e->e_type));
    printf("  Machine:                           %s\n", em_name(e->e_machine));
    printf("  Entry point address:               0x%" PRIx64 "\n", (uint64_t)e->e_entry);
    printf("  Start of program headers:          %" PRIu64 " (bytes into file)\n", (uint64_t)e->e_phoff);
    printf("  Start of section headers:          %" PRIu64 " (bytes into file)\n", (uint64_t)e->e_shoff);
    printf("  Flags:                             0x%x\n", e->e_flags);
    printf("  Size of this header:               %u (bytes)\n", e->e_ehsize);
    printf("  Size of program headers:           %u (bytes)\n", e->e_phentsize);
    printf("  Number of program headers:         %u\n", e->e_phnum);
    printf("  Size of section headers:           %u (bytes)\n", e->e_shentsize);
    printf("  Number of section headers:         %u\n", f->shnum);
    printf("  Section header string table index: %u\n", e->e_shstrndx);
    return 0;
}

/* ===========================================================================
 * 2. Section headers — the `readelf -S` table.
 * =========================================================================== */
int re_section_headers(const struct elf_file *f)
{
    if (!f->shdr) { printf("\nThere are no sections in this file.\n"); return 0; }

    printf("\nSection Headers:\n");
    printf("  [Nr] %-18s %-14s %-16s %-8s %-6s %-4s %-3s %-3s %-4s\n",
           "Name", "Type", "Address", "Offset", "Size", "ES", "Flg", "Lk", "Al");
    for (unsigned i = 0; i < f->shnum; i++) {
        const Elf64_Shdr *sh = &f->shdr[i];
        char flags[8]; sh_flag_str(sh->sh_flags, flags);
        printf("  [%2u] %-18.18s %-14s %016" PRIx64 " %08" PRIx64 " %06" PRIx64
               " %02" PRIx64 " %-3s %2u %2" PRIu64 "\n",
               i, ef_secname(f, sh), sht_name(sh->sh_type),
               (uint64_t)sh->sh_addr, (uint64_t)sh->sh_offset, (uint64_t)sh->sh_size,
               (uint64_t)sh->sh_entsize, flags, sh->sh_link, (uint64_t)sh->sh_addralign);
    }
    printf("Key to Flags: W(write) A(alloc) X(execute) M(merge) S(strings)"
           " I(info) T(tls)\n");
    return 0;
}

/* ===========================================================================
 * 3. Program headers — the `readelf -l` table, plus the section->segment map.
 *
 * The kernel only ever looks at PROGRAM headers (segments), never sections, to
 * load a process. Printing which sections fall inside each PT_LOAD segment shows
 * the two views lining up: the linker's sections aggregated into the loader's
 * segments.
 * =========================================================================== */
int re_program_headers(const struct elf_file *f)
{
    const Elf64_Ehdr *e = f->eh;
    if (e->e_phoff == 0 || e->e_phnum == 0) {
        printf("\nThere are no program headers in this file.\n");
        return 0;
    }
    if (e->e_phentsize != sizeof(Elf64_Phdr)) {
        fprintf(stderr, "elftk: unexpected e_phentsize %u\n", e->e_phentsize);
        return -1;
    }
    const Elf64_Phdr *ph = ef_ptr(f, e->e_phoff,
                                  (uint64_t)e->e_phnum * sizeof(Elf64_Phdr));
    if (!ph) { fprintf(stderr, "elftk: program headers out of bounds\n"); return -1; }

    printf("\nProgram Headers:\n");
    printf("  %-14s %-8s %-18s %-18s %-8s %-8s %-4s %s\n",
           "Type", "Offset", "VirtAddr", "PhysAddr", "FileSiz", "MemSiz", "Flg", "Align");
    for (unsigned i = 0; i < e->e_phnum; i++) {
        const Elf64_Phdr *p = &ph[i];
        char fl[4]; int n = 0;
        fl[n++] = (p->p_flags & PF_R) ? 'R' : ' ';
        fl[n++] = (p->p_flags & PF_W) ? 'W' : ' ';
        fl[n++] = (p->p_flags & PF_X) ? 'E' : ' ';   /* readelf prints X as 'E'  */
        fl[n] = '\0';
        printf("  %-14s 0x%06" PRIx64 " 0x%016" PRIx64 " 0x%016" PRIx64
               " 0x%06" PRIx64 " 0x%06" PRIx64 " %-4s 0x%" PRIx64 "\n",
               pt_name(p->p_type), (uint64_t)p->p_offset, (uint64_t)p->p_vaddr,
               (uint64_t)p->p_paddr, (uint64_t)p->p_filesz, (uint64_t)p->p_memsz,
               fl, (uint64_t)p->p_align);

        /* PT_INTERP points at the dynamic-loader path string; print it. */
        if (p->p_type == PT_INTERP) {
            const char *interp = ef_ptr(f, p->p_offset, p->p_filesz);
            if (interp) printf("      [Requesting program interpreter: %.*s]\n",
                               (int)p->p_filesz, interp);
        }
    }

    /* Section-to-segment mapping: for each loadable segment, list the sections
     * whose virtual-address range lies inside it. */
    if (f->shdr) {
        printf("\n Section to Segment mapping:\n");
        printf("  Segment Sections...\n");
        for (unsigned i = 0; i < e->e_phnum; i++) {
            const Elf64_Phdr *p = &ph[i];
            if (p->p_type != PT_LOAD && p->p_type != PT_DYNAMIC &&
                p->p_type != PT_NOTE && p->p_type != PT_TLS &&
                p->p_type != PT_GNU_RELRO && p->p_type != PT_INTERP &&
                p->p_type != PT_GNU_EH_FRAME) continue;
            printf("   %02u    ", i);
            for (unsigned j = 0; j < f->shnum; j++) {
                const Elf64_Shdr *sh = &f->shdr[j];
                if (!(sh->sh_flags & SHF_ALLOC)) continue;   /* only mapped ones  */
                /* Section sits in the segment if its vaddr window is contained. */
                if (sh->sh_addr >= p->p_vaddr &&
                    sh->sh_addr + sh->sh_size <= p->p_vaddr + p->p_memsz) {
                    const char *nm = ef_secname(f, sh);
                    if (nm[0]) printf("%s ", nm);
                }
            }
            printf("\n");
        }
    }
    return 0;
}

/* ===========================================================================
 * 4. Dynamic section — the `readelf -d` table.
 * =========================================================================== */
static const char *dt_name(int64_t tag)
{
    switch (tag) {
    case DT_NULL: return "NULL";        case DT_NEEDED: return "NEEDED";
    case DT_PLTRELSZ: return "PLTRELSZ";case DT_PLTGOT: return "PLTGOT";
    case DT_HASH: return "HASH";        case DT_STRTAB: return "STRTAB";
    case DT_SYMTAB: return "SYMTAB";    case DT_RELA: return "RELA";
    case DT_RELASZ: return "RELASZ";    case DT_RELAENT: return "RELAENT";
    case DT_STRSZ: return "STRSZ";      case DT_SYMENT: return "SYMENT";
    case DT_INIT: return "INIT";        case DT_FINI: return "FINI";
    case DT_SONAME: return "SONAME";    case DT_RPATH: return "RPATH";
    case DT_REL: return "REL";          case DT_RELSZ: return "RELSZ";
    case DT_RELENT: return "RELENT";    case DT_PLTREL: return "PLTREL";
    case DT_DEBUG: return "DEBUG";      case DT_JMPREL: return "JMPREL";
    case DT_BIND_NOW: return "BIND_NOW";
    case DT_INIT_ARRAY: return "INIT_ARRAY";
    case DT_FINI_ARRAY: return "FINI_ARRAY";
    case DT_INIT_ARRAYSZ: return "INIT_ARRAYSZ";
    case DT_FINI_ARRAYSZ: return "FINI_ARRAYSZ";
    case DT_RUNPATH: return "RUNPATH";  case DT_FLAGS: return "FLAGS";
    case DT_GNU_HASH: return "GNU_HASH";case DT_FLAGS_1: return "FLAGS_1";
    case DT_RELACOUNT: return "RELACOUNT";
    default: return "OTHER";
    }
}

int re_dynamic(const struct elf_file *f)
{
    if (!f->shdr) return 0;
    /* Find the SHT_DYNAMIC section. */
    const Elf64_Shdr *dsh = NULL;
    for (unsigned i = 0; i < f->shnum; i++)
        if (f->shdr[i].sh_type == SHT_DYNAMIC) { dsh = &f->shdr[i]; break; }
    if (!dsh) { printf("\nThere is no dynamic section in this file.\n"); return 0; }

    const Elf64_Dyn *dyn = ef_ptr(f, dsh->sh_offset, dsh->sh_size);
    if (!dyn) { fprintf(stderr, "elftk: .dynamic out of bounds\n"); return -1; }
    unsigned n = (unsigned)(dsh->sh_size / sizeof(Elf64_Dyn));

    /* The string table used for NEEDED/SONAME names is the section .dynamic
     * links to (its sh_link), which is normally .dynstr. */
    const char *str = f->dynstr; size_t strsz = f->dynstrsz;
    if (dsh->sh_link < f->shnum) {
        const Elf64_Shdr *st = &f->shdr[dsh->sh_link];
        const void *sb = ef_ptr(f, st->sh_offset, st->sh_size);
        if (sb) { str = (const char *)sb; strsz = (size_t)st->sh_size; }
    }

    printf("\nDynamic section at offset 0x%" PRIx64 " contains %u entries:\n",
           (uint64_t)dsh->sh_offset, n);
    printf("  %-18s %-20s %s\n", "Tag", "Type", "Name/Value");
    for (unsigned i = 0; i < n; i++) {
        const Elf64_Dyn *d = &dyn[i];
        printf("  0x%016" PRIx64 " %-20s ", (uint64_t)d->d_tag, dt_name(d->d_tag));
        /* Tags whose value is a .dynstr offset get their string printed. */
        if (d->d_tag == DT_NEEDED)
            printf("Shared library: [%s]\n", ef_str(str, strsz, d->d_un.d_val));
        else if (d->d_tag == DT_SONAME)
            printf("Library soname: [%s]\n", ef_str(str, strsz, d->d_un.d_val));
        else if (d->d_tag == DT_RPATH || d->d_tag == DT_RUNPATH)
            printf("Library path: [%s]\n", ef_str(str, strsz, d->d_un.d_val));
        else
            printf("0x%" PRIx64 "\n", (uint64_t)d->d_un.d_val);
        if (d->d_tag == DT_NULL) break;    /* the array terminator               */
    }
    return 0;
}

/* ===========================================================================
 * 5. Relocations — the `readelf -r` tables (RELA form; x86-64 uses only RELA).
 *
 * A relocation says "at r_offset, plug in a value computed from symbol S and
 * addend A". We resolve the symbol name through the reloc section's linked
 * symbol table (sh_link), whose own strings come from THAT table's sh_link.
 * =========================================================================== */
int re_relocations(const struct elf_file *f)
{
    if (!f->shdr) return 0;
    int found = 0;

    for (unsigned i = 0; i < f->shnum; i++) {
        const Elf64_Shdr *rs = &f->shdr[i];
        if (rs->sh_type != SHT_RELA) continue;          /* RELA only             */
        if (rs->sh_entsize != sizeof(Elf64_Rela)) continue;

        const Elf64_Rela *rela = ef_ptr(f, rs->sh_offset, rs->sh_size);
        if (!rela) continue;
        unsigned n = (unsigned)(rs->sh_size / sizeof(Elf64_Rela));
        found = 1;

        /* The symbol table this reloc section indexes into, and its strings. */
        const Elf64_Sym *syms = NULL; unsigned symn = 0;
        const char *str = NULL; size_t strsz = 0;
        if (rs->sh_link < f->shnum) {
            const Elf64_Shdr *symsh = &f->shdr[rs->sh_link];
            const void *sb = ef_ptr(f, symsh->sh_offset, symsh->sh_size);
            if (sb) { syms = sb; symn = (unsigned)(symsh->sh_size / sizeof(Elf64_Sym)); }
            if (symsh->sh_link < f->shnum) {
                const Elf64_Shdr *strsh = &f->shdr[symsh->sh_link];
                const void *stb = ef_ptr(f, strsh->sh_offset, strsh->sh_size);
                if (stb) { str = stb; strsz = (size_t)strsh->sh_size; }
            }
        }

        printf("\nRelocation section '%s' at offset 0x%" PRIx64
               " contains %u entries:\n",
               ef_secname(f, rs), (uint64_t)rs->sh_offset, n);
        printf("  %-18s %-18s %-22s %-18s %s\n",
               "Offset", "Info", "Type", "Sym.Value", "Sym.Name + Addend");

        for (unsigned j = 0; j < n; j++) {
            const Elf64_Rela *r = &rela[j];
            unsigned type = (unsigned)ELF64_R_TYPE(r->r_info);
            unsigned symidx = (unsigned)ELF64_R_SYM(r->r_info);

            const char *sname = "";
            uint64_t sval = 0;
            if (syms && symidx < symn) {
                const Elf64_Sym *sym = &syms[symidx];
                sval = sym->st_value;
                /* For STT_SECTION symbols the "name" is the section name. */
                if (ELF64_ST_TYPE(sym->st_info) == STT_SECTION && f->shdr &&
                    sym->st_shndx < f->shnum)
                    sname = ef_secname(f, &f->shdr[sym->st_shndx]);
                else
                    sname = ef_str(str, strsz, sym->st_name);
            }
            printf("  %016" PRIx64 " %016" PRIx64 " %-22s %016" PRIx64 " %s + %" PRIx64 "\n",
                   (uint64_t)r->r_offset, (uint64_t)r->r_info, r_x86_64_name(type),
                   sval, sname, (uint64_t)(int64_t)r->r_addend);
        }
    }
    if (!found) printf("\nThere are no relocations in this file.\n");
    return 0;
}

/* ===========================================================================
 * 6. Symbol tables — the `readelf -s` view of .dynsym and .symtab.
 * =========================================================================== */
static void dump_symtab(const char *title,
                        const Elf64_Sym *syms, unsigned n,
                        const char *str, size_t strsz)
{
    if (!syms || n == 0) return;
    printf("\nSymbol table '%s' contains %u entries:\n", title, n);
    printf("   Num:    Value          Size Type    Bind   Vis      Ndx Name\n");
    for (unsigned i = 0; i < n; i++) {
        const Elf64_Sym *s = &syms[i];
        unsigned type = ELF64_ST_TYPE(s->st_info);
        unsigned bind = ELF64_ST_BIND(s->st_info);
        unsigned vis  = ELF64_ST_VISIBILITY(s->st_other);

        /* Ndx is usually a numeric section index, but a few reserved values are
         * printed by name (ABS = absolute constant, UND = undefined, etc.). */
        char ndx[8];
        if (s->st_shndx == SHN_UNDEF)       snprintf(ndx, sizeof ndx, "UND");
        else if (s->st_shndx == SHN_ABS)    snprintf(ndx, sizeof ndx, "ABS");
        else if (s->st_shndx == SHN_COMMON) snprintf(ndx, sizeof ndx, "COM");
        else                                snprintf(ndx, sizeof ndx, "%u", s->st_shndx);

        printf("  %4u: %016" PRIx64 " %5" PRIu64 " %-7s %-6s %-8s %3s %s\n",
               i, (uint64_t)s->st_value, (uint64_t)s->st_size,
               stt_name(type), stb_name(bind), stv_name(vis), ndx,
               ef_str(str, strsz, s->st_name));
    }
}

int re_symbols(const struct elf_file *f)
{
    if (!f->dynsym && !f->symtab) {
        printf("\nThis file has no symbol tables (it may be stripped).\n");
        return 0;
    }
    dump_symtab(".dynsym", f->dynsym, f->dyncount, f->dynstr, f->dynstrsz);
    dump_symtab(".symtab", f->symtab, f->symcount, f->symstr, f->symstrsz);
    return 0;
}

/* ===========================================================================
 * -a : everything, in readelf's order.
 * =========================================================================== */
int re_all(const struct elf_file *f)
{
    re_file_header(f);
    re_section_headers(f);
    re_program_headers(f);
    re_dynamic(f);
    re_relocations(f);
    re_symbols(f);
    return 0;
}
