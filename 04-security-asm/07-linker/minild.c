/* ===========================================================================
 * minild.c — a minimal STATIC linker for x86-64 Linux ELF objects.
 * ===========================================================================
 *
 * WHAT A LINKER ACTUALLY DOES (the whole job, in order)
 * -----------------------------------------------------
 * A compiler turns each .c into a *relocatable object* (.o, ET_REL): sections
 * of code and data, a symbol table, and RELOCATIONS — "I referenced `foo` here
 * but I don't know its address; whoever places `foo` must patch these bytes."
 * The linker's task:
 *
 *   1. PARSE every input .o: sections, .symtab, .rela.* relocation tables.
 *   2. LAYOUT: concatenate like sections (all .text together, all .data
 *      together, ...) and pack them into loadable SEGMENTS with W^X perms.
 *   3. ASSIGN ADDRESSES: pick a load base (0x400000) and give every section a
 *      final virtual address.
 *   4. RESOLVE SYMBOLS across objects: match each undefined reference to a
 *      definition; enforce precedence (one strong def; weak yields to strong).
 *   5. APPLY RELOCATIONS: for each entry compute S+A, or S+A-P for PC-relative,
 *      and patch the 4/8 target bytes. (This is asm/demo.c — the linker's core.)
 *   6. EMIT an ET_EXEC: ELF header + PROGRAM HEADERS + the laid-out bytes, with
 *      e_entry = address of `_start`. The kernel can execve(2) it directly.
 *
 * SCOPE — read this honestly. This is the TEACHING CORE: a *static* link of a
 * handful of ET_REL objects into a runnable non-PIE executable, supporting the
 * relocation types real freestanding code emits (64, PC32, PLT32, 32, 32S).
 * The DYNAMIC case (PT_INTERP, a GOT/PLT, .dynsym/.dynamic, R_X86_64_GLOB_DAT/
 * JUMP_SLOT/RELATIVE, and ld.so at run time) is explained in the README but not
 * built here — it is a much larger machine and would bury the core lesson.
 *
 * SECURITY / DEFENSE FRAMING. A linker decides a program's memory map, so it is
 * where the classic exploit mitigations are *placed*: W^X (no segment is both
 * writable and executable), PIE/ASLR (ET_DYN + a randomised base), RELRO (mark
 * the GOT read-only after startup), and the stack-executability flag. We build
 * a fixed-address ET_EXEC on purpose so those trade-offs are visible; the
 * README's "defense" section says exactly what each one costs an attacker and
 * how a production linker (ld/lld) turns them on. Legal, on-your-own-box,
 * compile-it-yourself learning only.
 *
 * Build: this is a HOST tool (pure byte manipulation, no Linux headers), so it
 * compiles and runs anywhere — Windows, macOS, Linux. It cross-links Linux .o
 * files into a Linux executable. Running the *output* needs Linux/WSL.
 * ===========================================================================
 */

#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <stdint.h>
#include <stdbool.h>
#include <stdarg.h>   /* va_list for die()                                    */
#include <inttypes.h>

#include "elf.h"

/* ===========================================================================
 * Small utilities
 * ===========================================================================
 */

/* Fatal error: print to stderr and abort the link. A real linker keeps going
 * to report every error at once; we stop at the first for clarity. */
static void die(const char *fmt, ...) {
    va_list ap;
    __builtin_va_start(ap, fmt);
    fputs("minild: error: ", stderr);
    vfprintf(stderr, fmt, ap);
    fputc('\n', stderr);
    __builtin_va_end(ap);
    exit(1);
}

/* malloc-or-die and calloc-or-die: allocation failure is unrecoverable here. */
static void *xmalloc(size_t n) {
    void *p = malloc(n ? n : 1);
    if (!p) die("out of memory (%zu bytes)", n);
    return p;
}
static void *xcalloc(size_t n, size_t sz) {
    void *p = calloc(n ? n : 1, sz ? sz : 1);
    if (!p) die("out of memory (%zu x %zu bytes)", n, sz);
    return p;
}

/* Round `x` up to the next multiple of `a`. `a` must be a power of two.
 * The idiom (x + a-1) & ~(a-1) clears the low bits after bumping into the
 * next boundary. Used constantly for section/segment/page alignment. */
static uint64_t align_up(uint64_t x, uint64_t a) {
    if (a <= 1) return x;
    return (x + (a - 1)) & ~(a - 1);
}

/* Read an entire file into a freshly malloc'd, NUL-padded buffer. The linker
 * memory-maps nothing fancy; it slurps each .o and parses in place. */
static uint8_t *read_file(const char *path, size_t *out_len) {
    FILE *f = fopen(path, "rb");
    if (!f) die("cannot open '%s'", path);
    if (fseek(f, 0, SEEK_END) != 0) die("seek '%s'", path);
    long n = ftell(f);
    if (n < 0) die("tell '%s'", path);
    rewind(f);
    uint8_t *buf = xmalloc((size_t)n + 1);
    if (n > 0 && fread(buf, 1, (size_t)n, f) != (size_t)n)
        die("short read on '%s'", path);
    buf[n] = 0;
    fclose(f);
    *out_len = (size_t)n;
    return buf;
}

/* ===========================================================================
 * Layout model
 * ---------------------------------------------------------------------------
 * We funnel every allocatable input section into exactly one of four OUTPUT
 * sections, chosen by its FLAGS (not its name — so .text.foo, .rodata.str1.1,
 * .data.rel.ro, etc. all land correctly, exactly as a real linker's "orphan
 * placement" works):
 *
 *     SEG_TEXT   (R|X) : .text     — SHF_EXECINSTR
 *     SEG_RODATA (R)   : .rodata   — SHF_ALLOC, not W, not X
 *     SEG_DATA   (R|W) : .data     — SHF_WRITE, has file bytes (PROGBITS)
 *                        .bss      — SHF_WRITE, NOBITS (zero-filled, no bytes)
 *
 * Grouping by permission is what enforces W^X: code is never writable, data is
 * never executable. Three PT_LOAD segments come straight out of this table.
 * ===========================================================================
 */
enum { OUT_TEXT = 0, OUT_RODATA = 1, OUT_DATA = 2, OUT_BSS = 3, NUM_OUT = 4 };
enum { SEG_TEXT = 0, SEG_RODATA = 1, SEG_DATA = 2, NUM_SEG = 3 };

#define LOAD_BASE   0x400000UL   /* classic non-PIE base; low enough for 32S  */
#define PAGE_SIZE   0x1000UL     /* 4 KiB; the granularity mmap maps segments */

/* One member = one input section contributed by one object into an OUT_* bin,
 * placed at `off_in_out` bytes from that output section's base. */
typedef struct {
    struct Obj *obj;      /* which object it came from                        */
    int         shndx;    /* its section index within that object             */
    uint64_t    off_in_out; /* its offset inside the output section           */
} Member;

/* One of the four output sections: an ordered list of members plus the final
 * placement decided in layout(). */
typedef struct {
    const char *name;     /* ".text" / ".rodata" / ".data" / ".bss"          */
    uint32_t    sh_type;  /* SHT_PROGBITS, or SHT_NOBITS for .bss            */
    uint64_t    sh_flags; /* SHF_* for the emitted section header             */
    int         seg;      /* which PT_LOAD segment it belongs to             */
    Member     *mem;      /* dynamic array of members                        */
    int         nmem, cap;
    uint64_t    size;     /* total bytes (memsz for .bss)                    */
    uint64_t    align;    /* max alignment of any member                     */
    uint64_t    addr;     /* FINAL virtual address of this section's base    */
    uint64_t    off;      /* FINAL file offset of this section's bytes       */
    bool        present;  /* has at least one member                         */
} OutSec;

/* A parsed input object. We keep the raw file bytes and index into them. */
typedef struct Obj {
    const char  *path;
    uint8_t     *data;    /* whole-file buffer                               */
    size_t       len;
    Elf64_Ehdr   eh;      /* decoded ELF header                              */
    int          shnum;   /* number of section headers                       */
    Elf64_Shdr  *sh;      /* decoded section header array [shnum]            */
    const char  *shstr;   /* section-name string table (points into data)   */
    int          symtab_idx; /* index of the .symtab section (-1 if none)    */
    Elf64_Sym   *sym;     /* decoded symbols [nsym] (NULL if none)           */
    int          nsym;
    const char  *str;     /* symbol string table (.strtab)                  */
    /* Per-section placement results, indexed by the object's own shndx: */
    uint64_t    *sec_addr; /* final vaddr of each section (0 if not placed)  */
    uint64_t    *sec_off;  /* final file offset of each section              */
    bool        *sec_placed;
} Obj;

/* A resolved global symbol: name -> final address, with binding for precedence. */
typedef struct {
    const char *name;
    uint64_t    value;    /* final virtual address of the definition         */
    bool        defined;  /* did we see a definition (vs. only references)?  */
    bool        weak;     /* was the (chosen) definition weak?               */
    const char *from;     /* object path of the definition, for diagnostics  */
} GSym;

/* ===========================================================================
 * ELF decoders — pull native structs out of the little-endian file bytes.
 * Every field uses the byte offsets documented in elf.h.
 * ===========================================================================
 */
static void load_ehdr(const uint8_t *b, size_t len, Elf64_Ehdr *e) {
    if (len < 64) die("file too small to be ELF");
    memcpy(e->e_ident, b, EI_NIDENT);
    e->e_type      = rd16(b + 16);
    e->e_machine   = rd16(b + 18);
    e->e_version   = rd32(b + 20);
    e->e_entry     = rd64(b + 24);
    e->e_phoff     = rd64(b + 32);
    e->e_shoff     = rd64(b + 40);
    e->e_flags     = rd32(b + 48);
    e->e_ehsize    = rd16(b + 52);
    e->e_phentsize = rd16(b + 54);
    e->e_phnum     = rd16(b + 56);
    e->e_shentsize = rd16(b + 58);
    e->e_shnum     = rd16(b + 60);
    e->e_shstrndx  = rd16(b + 62);
}
static void load_shdr(const uint8_t *b, Elf64_Shdr *s) {
    s->sh_name      = rd32(b + 0);
    s->sh_type      = rd32(b + 4);
    s->sh_flags     = rd64(b + 8);
    s->sh_addr      = rd64(b + 16);
    s->sh_offset    = rd64(b + 24);
    s->sh_size      = rd64(b + 32);
    s->sh_link      = rd32(b + 40);
    s->sh_info      = rd32(b + 44);
    s->sh_addralign = rd64(b + 48);
    s->sh_entsize   = rd64(b + 56);
}
static void load_sym(const uint8_t *b, Elf64_Sym *s) {
    s->st_name  = rd32(b + 0);
    s->st_info  = b[4];
    s->st_other = b[5];
    s->st_shndx = rd16(b + 6);
    s->st_value = rd64(b + 8);
    s->st_size  = rd64(b + 16);
}
static void load_rela(const uint8_t *b, Elf64_Rela *r) {
    r->r_offset = rd64(b + 0);
    r->r_info   = rd64(b + 8);
    r->r_addend = (int64_t)rd64(b + 16);
}

/* ===========================================================================
 * PARSE one object file: validate it, decode section headers and .symtab.
 * ===========================================================================
 */
static void parse_object(Obj *o) {
    const uint8_t *b = o->data;

    /* --- validate the identity bytes before trusting anything else. --- */
    if (o->len < 64 ||
        b[EI_MAG0] != ELFMAG0 || b[EI_MAG1] != ELFMAG1 ||
        b[EI_MAG2] != ELFMAG2 || b[EI_MAG3] != ELFMAG3)
        die("'%s' is not an ELF file", o->path);
    if (b[EI_CLASS] != ELFCLASS64) die("'%s' is not ELF64", o->path);
    if (b[EI_DATA]  != ELFDATA2LSB) die("'%s' is not little-endian", o->path);

    load_ehdr(b, o->len, &o->eh);
    if (o->eh.e_type != ET_REL)
        die("'%s' is not a relocatable object (ET_REL); type=%u",
            o->path, o->eh.e_type);
    if (o->eh.e_machine != EM_X86_64)
        die("'%s' is not x86-64 (e_machine=%u)", o->path, o->eh.e_machine);
    if (o->eh.e_shentsize != sizeof(Elf64_Shdr))
        die("'%s' has odd section header size", o->path);

    /* --- decode all section headers. --- */
    o->shnum = o->eh.e_shnum;
    o->sh = xcalloc((size_t)o->shnum, sizeof(Elf64_Shdr));
    for (int i = 0; i < o->shnum; i++) {
        uint64_t off = o->eh.e_shoff + (uint64_t)i * o->eh.e_shentsize;
        if (off + sizeof(Elf64_Shdr) > o->len)
            die("'%s' section header %d out of range", o->path, i);
        load_shdr(b + off, &o->sh[i]);
    }

    /* --- the section-name string table (shstrtab), pointed to by e_shstrndx. */
    if (o->eh.e_shstrndx >= o->shnum) die("'%s' bad e_shstrndx", o->path);
    o->shstr = (const char *)(b + o->sh[o->eh.e_shstrndx].sh_offset);

    /* --- per-section placement bookkeeping, filled in during layout(). --- */
    o->sec_addr   = xcalloc((size_t)o->shnum, sizeof(uint64_t));
    o->sec_off    = xcalloc((size_t)o->shnum, sizeof(uint64_t));
    o->sec_placed = xcalloc((size_t)o->shnum, sizeof(bool));

    /* --- locate and decode the symbol table. --- */
    o->symtab_idx = -1;
    for (int i = 0; i < o->shnum; i++) {
        if (o->sh[i].sh_type == SHT_SYMTAB) { o->symtab_idx = i; break; }
    }
    if (o->symtab_idx >= 0) {
        Elf64_Shdr *st = &o->sh[o->symtab_idx];
        if (st->sh_entsize != sizeof(Elf64_Sym))
            die("'%s' odd symbol entry size", o->path);
        o->nsym = (int)(st->sh_size / sizeof(Elf64_Sym));
        o->sym = xcalloc((size_t)o->nsym, sizeof(Elf64_Sym));
        for (int i = 0; i < o->nsym; i++)
            load_sym(b + st->sh_offset + (uint64_t)i * sizeof(Elf64_Sym),
                     &o->sym[i]);
        /* sh_link of a SYMTAB points at its string table (.strtab). */
        o->str = (const char *)(b + o->sh[st->sh_link].sh_offset);
    } else {
        o->nsym = 0; o->sym = NULL; o->str = NULL;
    }
}

/* Classify an input section into an OUT_* bin by its FLAGS. Returns -1 if the
 * section is not allocatable (e.g. .comment, .note, .symtab) and so contributes
 * nothing to the running image. */
static int classify(const Elf64_Shdr *s) {
    if (!(s->sh_flags & SHF_ALLOC)) return -1;    /* not loaded into memory   */
    if (s->sh_flags & SHF_EXECINSTR) return OUT_TEXT;
    if (s->sh_flags & SHF_WRITE)
        return (s->sh_type == SHT_NOBITS) ? OUT_BSS : OUT_DATA;
    return OUT_RODATA;                            /* alloc, read-only         */
}

/* ===========================================================================
 * The linker state that layout() and relocate() share.
 * ===========================================================================
 */
static OutSec g_out[NUM_OUT];   /* the four output sections                  */
static GSym  *g_sym;            /* global symbol table (dynamic array)       */
static int    g_nsym, g_symcap;
static Elf64_Phdr g_phdr[NUM_SEG]; /* one per present PT_LOAD segment        */
static int    g_nphdr;
static uint64_t g_entry;        /* final e_entry (address of _start)         */

/* ---------------------------------------------------------------------------
 * Global symbol table: linear-probe insert/lookup. O(n^2) over the whole link,
 * which is fine at teaching scale and keeps the data structure legible (a real
 * linker uses a hash table). Precedence rules live here.
 * ------------------------------------------------------------------------- */
static GSym *gsym_find(const char *name) {
    for (int i = 0; i < g_nsym; i++)
        if (strcmp(g_sym[i].name, name) == 0) return &g_sym[i];
    return NULL;
}
static GSym *gsym_intern(const char *name) {
    GSym *g = gsym_find(name);
    if (g) return g;
    if (g_nsym == g_symcap) {
        g_symcap = g_symcap ? g_symcap * 2 : 64;
        g_sym = realloc(g_sym, (size_t)g_symcap * sizeof(GSym));
        if (!g_sym) die("out of memory (symbols)");
    }
    g = &g_sym[g_nsym++];
    memset(g, 0, sizeof(*g));
    g->name = name;
    return g;
}

/* Record a DEFINITION, enforcing precedence:
 *   - strong (STB_GLOBAL) vs strong  -> multiple-definition error
 *   - strong beats weak (either order)
 *   - weak vs weak -> first one wins (deterministic)
 * This is exactly why `int foo;` in two files may link but two `int foo = 1;`
 * do not (with -fno-common): the initialised ones are strong. */
static void gsym_define(const char *name, uint64_t value, bool weak,
                        const char *from) {
    GSym *g = gsym_intern(name);
    if (g->defined) {
        if (!g->weak && !weak)
            die("multiple definition of '%s' (in %s and %s)",
                name, g->from, from);
        if (g->weak && !weak) {          /* new strong overrides old weak     */
            g->value = value; g->weak = false; g->from = from;
        }
        /* else keep the existing (strong, or first weak) definition          */
        return;
    }
    g->defined = true; g->value = value; g->weak = weak; g->from = from;
}

/* ===========================================================================
 * LAYOUT — gather sections into the four bins, then assign every section a
 * final file offset and virtual address, then build the PT_LOAD program
 * headers. After this runs, obj->sec_addr[i]/sec_off[i] are known for every
 * placed section, which is what symbol resolution and relocation need.
 * ===========================================================================
 */
static void outsec_add(OutSec *os, Obj *o, int shndx) {
    Elf64_Shdr *s = &o->sh[shndx];
    uint64_t al = s->sh_addralign ? s->sh_addralign : 1;
    /* Bump the running size up to this member's alignment before placing it. */
    os->size = align_up(os->size, al);
    if (os->nmem == os->cap) {
        os->cap = os->cap ? os->cap * 2 : 8;
        os->mem = realloc(os->mem, (size_t)os->cap * sizeof(Member));
        if (!os->mem) die("out of memory (members)");
    }
    os->mem[os->nmem++] = (Member){ .obj = o, .shndx = shndx,
                                    .off_in_out = os->size };
    os->size += s->sh_size;
    if (al > os->align) os->align = al;
    os->present = true;
}

static void layout(Obj *objs, int nobj) {
    /* Initialise the four output-section descriptors. */
    g_out[OUT_TEXT]   = (OutSec){ .name=".text",   .sh_type=SHT_PROGBITS,
        .sh_flags=SHF_ALLOC|SHF_EXECINSTR, .seg=SEG_TEXT,   .align=1 };
    g_out[OUT_RODATA] = (OutSec){ .name=".rodata", .sh_type=SHT_PROGBITS,
        .sh_flags=SHF_ALLOC,               .seg=SEG_RODATA, .align=1 };
    g_out[OUT_DATA]   = (OutSec){ .name=".data",   .sh_type=SHT_PROGBITS,
        .sh_flags=SHF_ALLOC|SHF_WRITE,     .seg=SEG_DATA,   .align=1 };
    g_out[OUT_BSS]    = (OutSec){ .name=".bss",    .sh_type=SHT_NOBITS,
        .sh_flags=SHF_ALLOC|SHF_WRITE,     .seg=SEG_DATA,   .align=1 };

    /* --- Pass 1: bin every allocatable input section, in input order. --- */
    for (int oi = 0; oi < nobj; oi++) {
        Obj *o = &objs[oi];
        for (int i = 0; i < o->shnum; i++) {
            /* Reject COMMON symbols up front: modern clang/gcc default to
             * -fno-common so tentative definitions already sit in .bss. We do
             * not implement COMMON allocation; say so rather than mislink. */
            int bin = classify(&o->sh[i]);
            if (bin < 0) continue;
            outsec_add(&g_out[bin], o, i);
        }
    }

    /* Which of the three segments actually carry content? (Text always does:
     * it holds _start.) This decides how many program headers we emit. */
    bool seg_present[NUM_SEG] = { false, false, false };
    for (int k = 0; k < NUM_OUT; k++)
        if (g_out[k].present) seg_present[g_out[k].seg] = true;
    g_nphdr = 0;
    for (int s = 0; s < NUM_SEG; s++) if (seg_present[s]) g_nphdr++;

    /* Bytes the ELF header + program headers occupy at the very start of the
     * file. We map them into the TEXT segment (the classic trick: the headers
     * live at vaddr LOAD_BASE, and .text follows them). */
    uint64_t hdrs = sizeof(Elf64_Ehdr) + (uint64_t)g_nphdr * sizeof(Elf64_Phdr);

    /* --- Pass 2: assign file offsets + virtual addresses, segment by segment.
     * Invariant we maintain for every segment: p_vaddr - p_offset is a multiple
     * of PAGE_SIZE, so the kernel's mmap constraint
     *     p_vaddr % p_align == p_offset % p_align
     * holds (we use p_align = PAGE_SIZE). We keep vaddr and file offset moving
     * in lockstep (same deltas) inside a segment so a byte at file offset F
     * lands at vaddr (F + seg_delta). --- */
    uint64_t off = 0, va = 0;
    int phi = 0;

    /* ---- SEGMENT 0: TEXT (R|X). Starts at file 0 / vaddr LOAD_BASE and
     * includes the headers, so .text begins right after them. ---- */
    {
        off = 0;
        va  = LOAD_BASE;                 /* delta = va - off = LOAD_BASE       */
        uint64_t seg_off = off, seg_va = va;
        off += hdrs; va += hdrs;         /* skip past ehdr+phdrs               */

        OutSec *os = &g_out[OUT_TEXT];
        if (os->present) {
            uint64_t pad = align_up(va, os->align) - va;
            off += pad; va += pad;       /* align .text; delta unchanged       */
            os->off = off; os->addr = va;
            off += os->size; va += os->size;
        }
        g_phdr[phi++] = (Elf64_Phdr){
            .p_type=PT_LOAD, .p_flags=PF_R|PF_X,
            .p_offset=seg_off, .p_vaddr=seg_va, .p_paddr=seg_va,
            .p_filesz=off - seg_off, .p_memsz=va - seg_va, .p_align=PAGE_SIZE };
    }

    /* ---- SEGMENT 1: RODATA (R). New page. ---- */
    if (seg_present[SEG_RODATA]) {
        off = align_up(off, PAGE_SIZE);
        va  = align_up(va,  PAGE_SIZE);
        uint64_t seg_off = off, seg_va = va;
        OutSec *os = &g_out[OUT_RODATA];
        uint64_t pad = align_up(va, os->align) - va;
        off += pad; va += pad;
        os->off = off; os->addr = va;
        off += os->size; va += os->size;
        g_phdr[phi++] = (Elf64_Phdr){
            .p_type=PT_LOAD, .p_flags=PF_R,
            .p_offset=seg_off, .p_vaddr=seg_va, .p_paddr=seg_va,
            .p_filesz=off - seg_off, .p_memsz=va - seg_va, .p_align=PAGE_SIZE };
    }

    /* ---- SEGMENT 2: DATA (R|W) = .data (file bytes) then .bss (memsz only).
     * The segment's filesz stops at the end of .data; memsz extends over .bss,
     * which the kernel zero-fills. That gap is exactly what makes .bss free on
     * disk. ---- */
    if (seg_present[SEG_DATA]) {
        off = align_up(off, PAGE_SIZE);
        va  = align_up(va,  PAGE_SIZE);
        uint64_t seg_off = off, seg_va = va;
        uint64_t file_end = off;         /* tracks the end of PROGBITS bytes   */

        OutSec *data = &g_out[OUT_DATA];
        if (data->present) {
            uint64_t pad = align_up(va, data->align) - va;
            off += pad; va += pad;
            data->off = off; data->addr = va;
            off += data->size; va += data->size;
            file_end = off;              /* .data DOES consume file bytes      */
        }
        OutSec *bss = &g_out[OUT_BSS];
        if (bss->present) {
            uint64_t pad = align_up(va, bss->align) - va;
            va += pad;                   /* .bss advances vaddr only...        */
            bss->addr = va;
            /* ...but we still record a nominal file offset for its section
             * header; no bytes are written there. */
            bss->off = file_end + (bss->addr - (seg_va + (file_end - seg_off)));
            va += bss->size;
        }
        g_phdr[phi++] = (Elf64_Phdr){
            .p_type=PT_LOAD, .p_flags=PF_R|PF_W,
            .p_offset=seg_off, .p_vaddr=seg_va, .p_paddr=seg_va,
            .p_filesz=file_end - seg_off,   /* file bytes: up to end of .data  */
            .p_memsz=va - seg_va,           /* memory: includes .bss           */
            .p_align=PAGE_SIZE };
    }

    /* --- Propagate section placement back to each member so relocation can
     * find "where did THIS object's section end up". --- */
    for (int k = 0; k < NUM_OUT; k++) {
        OutSec *os = &g_out[k];
        for (int m = 0; m < os->nmem; m++) {
            Member *me = &os->mem[m];
            me->obj->sec_addr[me->shndx]   = os->addr + me->off_in_out;
            me->obj->sec_off[me->shndx]    = os->off  + me->off_in_out;
            me->obj->sec_placed[me->shndx] = true;
        }
    }
}

/* ===========================================================================
 * SYMBOL RESOLUTION — walk every object's .symtab and register each defined
 * global/weak symbol at its final address. Must run AFTER layout so section
 * addresses exist. Local symbols are resolved on demand during relocation.
 * ===========================================================================
 */
static void resolve_symbols(Obj *objs, int nobj, const char *entry_name) {
    for (int oi = 0; oi < nobj; oi++) {
        Obj *o = &objs[oi];
        for (int i = 0; i < o->nsym; i++) {
            Elf64_Sym *s = &o->sym[i];
            int bind = ELF64_ST_BIND(s->st_info);
            int type = ELF64_ST_TYPE(s->st_info);
            const char *name = o->str + s->st_name;

            if (s->st_shndx == SHN_UNDEF) continue;   /* a reference, not def  */
            if (bind == STB_LOCAL) continue;          /* handled per-object    */
            if (type == STT_FILE) continue;           /* metadata              */
            if (name[0] == '\0') continue;            /* nameless              */

            if (s->st_shndx == SHN_COMMON)
                die("'%s' defines COMMON symbol '%s'; recompile the inputs "
                    "with -fno-common (this teaching linker requires it)",
                    o->path, name);
            if (s->st_shndx == SHN_ABS) {             /* absolute constant     */
                gsym_define(name, s->st_value, bind == STB_WEAK, o->path);
                continue;
            }
            if (s->st_shndx >= o->shnum || !o->sec_placed[s->st_shndx])
                continue;   /* defined in a non-allocated section: ignore      */

            /* Final address = (section base) + (offset within section). */
            uint64_t val = o->sec_addr[s->st_shndx] + s->st_value;
            gsym_define(name, val, bind == STB_WEAK, o->path);
        }
    }

    /* The entry point (default `_start`) must be defined; e_entry needs it. */
    GSym *e = gsym_find(entry_name);
    if (!e || !e->defined)
        die("undefined entry symbol '%s' — is _start defined and global?",
            entry_name);
    g_entry = e->value;
}

/* ===========================================================================
 * RELOCATION — the arithmetic core of a linker. For each relocation entry we
 * compute the symbol value S and addend A, then patch the target field:
 *
 *     R_X86_64_64    : *(u64*)P_bytes = S + A
 *     R_X86_64_PC32  : *(u32*)P_bytes = S + A - P     (P = vaddr of the field)
 *     R_X86_64_PLT32 : same as PC32 for a static link (no PLT needed)
 *     R_X86_64_32    : *(u32*)P_bytes = S + A          (must fit unsigned 32)
 *     R_X86_64_32S   : *(u32*)P_bytes = S + A          (must fit signed 32)
 *
 * `image` is the output file buffer; we patch bytes at (target sec file offset
 * + r_offset). This exact routine is extracted, standalone, into asm/demo.c.
 * ===========================================================================
 */

/* Resolve the symbol referenced by a relocation to its final address S.
 * Handles: section symbols (STT_SECTION -> section base), locals (their own
 * object's placement), and globals (the cross-object table). */
static uint64_t reloc_symbol_value(Obj *o, uint32_t symidx, const char *what) {
    if (symidx >= (uint32_t)o->nsym)
        die("%s: symbol index %u out of range in %s", what, symidx, o->path);
    Elf64_Sym *s = &o->sym[symidx];
    int bind = ELF64_ST_BIND(s->st_info);
    int type = ELF64_ST_TYPE(s->st_info);
    const char *name = o->str + s->st_name;

    if (s->st_shndx == SHN_ABS) return s->st_value;   /* absolute             */

    if (s->st_shndx == SHN_UNDEF) {
        /* Undefined here: it must be defined by some other object. */
        GSym *g = gsym_find(name);
        if (!g || !g->defined)
            die("undefined reference to '%s' (from %s)", name, o->path);
        return g->value;
    }

    /* A section symbol's value is the base address of that section. These are
     * how references to anonymous data ("this string literal", "that jump
     * table") are expressed: symbol = .rodata, and r_addend picks the offset. */
    if (type == STT_SECTION) {
        if (s->st_shndx >= o->shnum || !o->sec_placed[s->st_shndx])
            die("%s: section symbol targets unplaced section in %s",
                what, o->path);
        return o->sec_addr[s->st_shndx];
    }

    /* A named local definition: resolve within this object. */
    if (bind == STB_LOCAL) {
        if (s->st_shndx >= o->shnum || !o->sec_placed[s->st_shndx])
            die("%s: local '%s' in unplaced section in %s", what, name, o->path);
        return o->sec_addr[s->st_shndx] + s->st_value;
    }

    /* A named global/weak: prefer the resolved global (it may have been
     * overridden by a strong definition in another object). */
    GSym *g = name[0] ? gsym_find(name) : NULL;
    if (g && g->defined) return g->value;
    if (s->st_shndx >= o->shnum || !o->sec_placed[s->st_shndx])
        die("%s: '%s' unresolved in %s", what, name, o->path);
    return o->sec_addr[s->st_shndx] + s->st_value;
}

/* Range-check helpers for the truncating relocation types. A real linker
 * prints "relocation truncated to fit" — the symptom of code/data placed too
 * far apart for a 32-bit displacement. We keep the check because getting it
 * wrong silently corrupts the program. */
static void check_s32(int64_t v, const char *what) {
    if (v < INT32_MIN || v > INT32_MAX)
        die("%s: value %lld does not fit in signed 32 bits (too far apart)",
            what, (long long)v);
}
static void check_u32(uint64_t v, const char *what) {
    if (v > 0xffffffffUL)
        die("%s: value %llu does not fit in unsigned 32 bits",
            what, (unsigned long long)v);
}

static void relocate(Obj *objs, int nobj, uint8_t *image) {
    for (int oi = 0; oi < nobj; oi++) {
        Obj *o = &objs[oi];
        for (int si = 0; si < o->shnum; si++) {
            Elf64_Shdr *rs = &o->sh[si];
            if (rs->sh_type != SHT_RELA) continue;    /* only RELA sections    */

            /* sh_info of a RELA section = the section it patches. Skip it if
             * that target isn't part of the loaded image (e.g. .rela.eh_frame
             * against a discarded section). */
            int tgt = (int)rs->sh_info;
            if (tgt < 0 || tgt >= o->shnum || !o->sec_placed[tgt]) continue;

            uint64_t tgt_off  = o->sec_off[tgt];   /* file offset of target    */
            uint64_t tgt_addr = o->sec_addr[tgt];  /* vaddr of target base     */
            int nrel = (int)(rs->sh_size / sizeof(Elf64_Rela));

            for (int r = 0; r < nrel; r++) {
                Elf64_Rela rel;
                load_rela(o->data + rs->sh_offset + (uint64_t)r * sizeof(Elf64_Rela),
                          &rel);
                uint32_t symidx = ELF64_R_SYM(rel.r_info);
                uint32_t rtype  = ELF64_R_TYPE(rel.r_info);

                uint64_t S = reloc_symbol_value(o, symidx, o->path);
                int64_t  A = rel.r_addend;
                uint64_t P = tgt_addr + rel.r_offset;   /* vaddr being patched */
                uint8_t *loc = image + tgt_off + rel.r_offset; /* file bytes   */

                switch (rtype) {
                case R_X86_64_64:                 /* S + A, 64-bit absolute    */
                    wr64(loc, S + (uint64_t)A);
                    break;
                case R_X86_64_PC32:               /* S + A - P, 32-bit rel     */
                case R_X86_64_PLT32: {            /* PLT bound direct == PC32  */
                    int64_t v = (int64_t)(S + (uint64_t)A) - (int64_t)P;
                    check_s32(v, o->path);
                    wr32(loc, (uint32_t)(int32_t)v);
                    break;
                }
                case R_X86_64_32: {               /* S + A, unsigned 32        */
                    uint64_t v = S + (uint64_t)A;
                    check_u32(v, o->path);
                    wr32(loc, (uint32_t)v);
                    break;
                }
                case R_X86_64_32S: {              /* S + A, signed 32          */
                    int64_t v = (int64_t)(S + (uint64_t)A);
                    check_s32(v, o->path);
                    wr32(loc, (uint32_t)(int32_t)v);
                    break;
                }
                default:
                    die("%s: unsupported relocation type %u at offset 0x%" PRIx64
                        " — this teaching linker handles 64/PC32/PLT32/32/32S "
                        "only (GOT/TLS relocs are the dynamic case, see README)",
                        o->path, rtype, rel.r_offset);
                }
            }
        }
    }
}

/* ===========================================================================
 * EMIT — assemble the output ELF image and write it to disk.
 * ===========================================================================
 */
static void write_output(Obj *objs, int nobj, const char *out_path) {
    (void)nobj; (void)objs;

    /* --- Decide the total file size: the end of the last segment's file
     * bytes, then a section header table + shstrtab appended for inspectability
     * (readelf -S / objdump -d). Section headers are NOT needed to RUN — the
     * kernel reads only program headers — but they make the output legible. */
    uint64_t file_end = sizeof(Elf64_Ehdr) + (uint64_t)g_nphdr * sizeof(Elf64_Phdr);
    for (int p = 0; p < g_nphdr; p++) {
        uint64_t e = g_phdr[p].p_offset + g_phdr[p].p_filesz;
        if (e > file_end) file_end = e;
    }

    /* Emit section headers for: NULL(0), each present OUT_* section, .shstrtab.
     * Build the .shstrtab blob first so we know its size and each sh_name. */
    const char *names[NUM_OUT + 2];
    int   nsec = 0;
    int   out_secidx[NUM_OUT];             /* OUT_* -> emitted section index    */
    /* shstrtab starts with a leading NUL (index 0 == empty name). */
    char  strbuf[256];
    uint32_t stroff[NUM_OUT + 2];
    size_t strlen_total = 1; strbuf[0] = '\0';
    names[nsec] = "";  stroff[nsec] = 0; nsec++;   /* the NULL section (idx 0) */

    for (int k = 0; k < NUM_OUT; k++) {
        if (!g_out[k].present) { out_secidx[k] = 0; continue; }
        stroff[nsec] = (uint32_t)strlen_total;
        strcpy(strbuf + strlen_total, g_out[k].name);
        strlen_total += strlen(g_out[k].name) + 1;
        names[nsec] = g_out[k].name;
        out_secidx[k] = nsec;
        nsec++;
    }
    /* the shstrtab section names itself */
    int shstr_secidx = nsec;
    uint32_t shstr_nameoff = (uint32_t)strlen_total;
    strcpy(strbuf + strlen_total, ".shstrtab");
    strlen_total += strlen(".shstrtab") + 1;
    names[nsec] = ".shstrtab"; stroff[nsec] = shstr_nameoff; nsec++;

    /* Place the shstrtab bytes, then the section header table, after segments. */
    uint64_t shstr_off = align_up(file_end, 1);
    uint64_t shtab_off = align_up(shstr_off + strlen_total, 8);
    uint64_t total = shtab_off + (uint64_t)nsec * sizeof(Elf64_Shdr);

    uint8_t *image = xcalloc(total, 1);

    /* --- Copy each PROGBITS input section's bytes into place. .bss (NOBITS)
     * contributes nothing: the buffer is already zero. --- */
    for (int k = 0; k < NUM_OUT; k++) {
        if (k == OUT_BSS) continue;
        OutSec *os = &g_out[k];
        for (int m = 0; m < os->nmem; m++) {
            Member *me = &os->mem[m];
            Elf64_Shdr *s = &me->obj->sh[me->shndx];
            if (s->sh_type == SHT_NOBITS) continue;
            memcpy(image + os->off + me->off_in_out,
                   me->obj->data + s->sh_offset, s->sh_size);
        }
    }

    /* --- Apply all relocations onto the copied bytes. --- */
    relocate(objs, nobj, image);

    /* --- ELF header. --- */
    {
        uint8_t *e = image;
        memset(e, 0, sizeof(Elf64_Ehdr));
        e[EI_MAG0]=ELFMAG0; e[EI_MAG1]=ELFMAG1;
        e[EI_MAG2]=ELFMAG2; e[EI_MAG3]=ELFMAG3;
        e[EI_CLASS]=ELFCLASS64; e[EI_DATA]=ELFDATA2LSB; e[EI_VERSION]=EV_CURRENT;
        wr16(e+16, ET_EXEC);        /* fixed-address executable (non-PIE)      */
        wr16(e+18, EM_X86_64);
        wr32(e+20, EV_CURRENT);
        wr64(e+24, g_entry);        /* the address of _start                   */
        wr64(e+32, sizeof(Elf64_Ehdr));  /* e_phoff: phdrs follow the ehdr     */
        wr64(e+40, shtab_off);      /* e_shoff                                 */
        wr32(e+48, 0);              /* e_flags                                 */
        wr16(e+52, sizeof(Elf64_Ehdr));
        wr16(e+54, sizeof(Elf64_Phdr));
        wr16(e+56, (uint16_t)g_nphdr);
        wr16(e+58, sizeof(Elf64_Shdr));
        wr16(e+60, (uint16_t)nsec);
        wr16(e+62, (uint16_t)shstr_secidx);   /* e_shstrndx                    */
    }

    /* --- Program headers, right after the ELF header. --- */
    for (int p = 0; p < g_nphdr; p++) {
        uint8_t *ph = image + sizeof(Elf64_Ehdr) + (size_t)p * sizeof(Elf64_Phdr);
        wr32(ph+0,  g_phdr[p].p_type);
        wr32(ph+4,  g_phdr[p].p_flags);
        wr64(ph+8,  g_phdr[p].p_offset);
        wr64(ph+16, g_phdr[p].p_vaddr);
        wr64(ph+24, g_phdr[p].p_paddr);
        wr64(ph+32, g_phdr[p].p_filesz);
        wr64(ph+40, g_phdr[p].p_memsz);
        wr64(ph+48, g_phdr[p].p_align);
    }

    /* --- shstrtab bytes. --- */
    memcpy(image + shstr_off, strbuf, strlen_total);

    /* --- Section header table. Index 0 is the mandatory all-zero NULL entry. */
    {
        /* NULL section: already zero. Now the OUT_* sections in emit order. */
        for (int k = 0; k < NUM_OUT; k++) {
            if (!g_out[k].present) continue;
            OutSec *os = &g_out[k];
            int idx = out_secidx[k];
            uint8_t *sh = image + shtab_off + (size_t)idx * sizeof(Elf64_Shdr);
            wr32(sh+0,  stroff[idx]);       /* sh_name                         */
            wr32(sh+4,  os->sh_type);
            wr64(sh+8,  os->sh_flags);
            wr64(sh+16, os->addr);          /* sh_addr = run-time vaddr        */
            wr64(sh+24, os->off);           /* sh_offset                       */
            wr64(sh+32, os->size);
            wr32(sh+40, 0);                 /* sh_link                         */
            wr32(sh+44, 0);                 /* sh_info                         */
            wr64(sh+48, os->align);
            wr64(sh+56, 0);                 /* sh_entsize                      */
        }
        /* .shstrtab section header. */
        uint8_t *sh = image + shtab_off + (size_t)shstr_secidx * sizeof(Elf64_Shdr);
        wr32(sh+0,  shstr_nameoff);
        wr32(sh+4,  SHT_STRTAB);
        wr64(sh+8,  0);
        wr64(sh+16, 0);
        wr64(sh+24, shstr_off);
        wr64(sh+32, strlen_total);
        wr32(sh+40, 0);
        wr32(sh+44, 0);
        wr64(sh+48, 1);
        wr64(sh+56, 0);
    }

    /* --- Flush the image to disk. --- */
    FILE *f = fopen(out_path, "wb");
    if (!f) die("cannot create '%s'", out_path);
    if (fwrite(image, 1, total, f) != total) die("short write on '%s'", out_path);
    fclose(f);
    free(image);

    /* On Unix a real linker chmod's the output executable; we leave that to
     * the Makefile (`chmod +x`) so this stays a portable host tool. */
    printf("minild: wrote %s  (entry _start = 0x%" PRIx64 ", %d segments, "
           "%d sections, %" PRIu64 " bytes)\n",
           out_path, g_entry, g_nphdr, nsec, total);
}

/* ===========================================================================
 * main — argument parsing and the pipeline: parse -> layout -> resolve ->
 * (relocate happens inside emit) -> write.
 * ===========================================================================
 */
static void usage(const char *argv0) {
    fprintf(stderr,
        "usage: %s [-o out] [-e entry] input1.o [input2.o ...]\n"
        "  -o out    output executable path (default a.out)\n"
        "  -e entry  entry symbol (default _start)\n"
        "A minimal STATIC linker: lays out sections, resolves symbols across\n"
        "objects, applies R_X86_64_{64,PC32,PLT32,32,32S}, emits an ET_EXEC.\n",
        argv0);
    exit(2);
}

int main(int argc, char **argv) {
    const char *out_path = "a.out";
    const char *entry    = "_start";
    const char *inputs[64];
    int ninput = 0;

    for (int i = 1; i < argc; i++) {
        if (strcmp(argv[i], "-o") == 0) {
            if (++i >= argc) usage(argv[0]);
            out_path = argv[i];
        } else if (strcmp(argv[i], "-e") == 0) {
            if (++i >= argc) usage(argv[0]);
            entry = argv[i];
        } else if (strcmp(argv[i], "-h") == 0 || strcmp(argv[i], "--help") == 0) {
            usage(argv[0]);
        } else {
            if (ninput >= (int)(sizeof(inputs)/sizeof(inputs[0])))
                die("too many inputs (max %zu)", sizeof(inputs)/sizeof(inputs[0]));
            inputs[ninput++] = argv[i];
        }
    }
    if (ninput == 0) usage(argv[0]);

    /* Parse every input object. */
    Obj *objs = xcalloc((size_t)ninput, sizeof(Obj));
    for (int i = 0; i < ninput; i++) {
        objs[i].path = inputs[i];
        objs[i].data = read_file(inputs[i], &objs[i].len);
        parse_object(&objs[i]);
    }

    /* The pipeline. */
    layout(objs, ninput);                 /* sections -> segments + addresses  */
    resolve_symbols(objs, ninput, entry); /* cross-object symbol binding       */
    write_output(objs, ninput, out_path); /* copy + relocate + emit ELF        */

    /* We deliberately do not free per-object buffers: the process exits now,
     * and the OS reclaims everything. Freeing here would only add noise. */
    return 0;
}
