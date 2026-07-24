/* ===========================================================================
 * debuginfo.c — turn raw addresses into names and source lines.
 * ===========================================================================
 *
 * ptrace gives us bytes and addresses; humans want "factorial() at sample.c:12".
 * That translation needs two on-disk tables inside the inferior's ELF file:
 *
 *   .symtab       (ELF)   : function name  <-> link-time address + size
 *   .debug_line   (DWARF) : address        <-> (source file, line)
 *
 * We mmap the ELF once (read-only) and read both out of that single mapping.
 *
 * --------------------------------------------------------------------------
 * PART A: ELF walking
 * --------------------------------------------------------------------------
 * An ELF64 file starts with an Elf64_Ehdr. It points at an array of Elf64_Shdr
 * section headers (e_shoff, e_shnum). Section NAMES live in the section-header
 * string table, itself a section indexed by e_shstrndx. So "find the section
 * called .symtab" means: for each shdr, look up shstrtab[shdr.sh_name] and
 * strcmp it. Each Elf64_Sym in .symtab has st_name (offset into .strtab),
 * st_value (address), st_size, and st_info (whose low nibble is the type; we
 * keep STT_FUNC == functions).
 *
 * --------------------------------------------------------------------------
 * PART B: the DWARF line-number program
 * --------------------------------------------------------------------------
 * .debug_line does NOT store rows. It stores a tiny bytecode for a state machine
 * whose registers are (address, file, line, ...). Running the program EMITS rows
 * — this compression is why a megabyte of code needs only kilobytes of line
 * data. We implement the DWARF v2/v3/v4 program (the "classic" header, with
 * NUL-terminated include-dir and file-name lists). DWARF v5 restructured that
 * header (form-coded directory/file entries) and is a documented gap — the
 * Makefile builds the sample with -gdwarf-4 so this parser matches it exactly.
 *
 * The state machine, per the DWARF spec:
 *   registers: address=0, file=1, line=1, column=0, is_stmt=default,
 *              op_index=0, end_sequence=0
 *   special opcode (>= opcode_base):
 *       adj = opcode - opcode_base
 *       address += min_inst_len * (adj / line_range)   (op_index math elided,
 *                                                        max_ops_per_insn == 1)
 *       line    += line_base + (adj % line_range)
 *       EMIT ROW
 *   standard opcodes 1..opcode_base-1: fixed meanings (copy, advance_pc, ...)
 *   extended opcodes (lead byte 0): end_sequence, set_address, define_file
 *
 * See di_run_line_program() for the annotated interpreter.
 * ===========================================================================
 */
#define _GNU_SOURCE
#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <stdint.h>
#include <fcntl.h>
#include <unistd.h>
#include <sys/mman.h>     /* mmap, munmap                                       */
#include <sys/stat.h>     /* fstat                                              */
#include <elf.h>          /* Elf64_Ehdr, Elf64_Shdr, Elf64_Sym, STT_FUNC, ...   */

#include "debugger.h"

/* ===========================================================================
 * A tiny forward-only byte cursor. Every DWARF field is read through this so a
 * malformed section can never walk off the end of the mapping: `end` is the hard
 * stop and every reader checks it.
 * ======================================================================== */
typedef struct {
    const uint8_t *p;      /* next byte to read                                 */
    const uint8_t *end;    /* one past the last valid byte                      */
} cursor;

static int cur_ok(cursor *c, size_t need) { return (size_t)(c->end - c->p) >= need; }

static uint8_t rd_u8(cursor *c) {
    if (!cur_ok(c, 1)) return 0;
    return *c->p++;
}
static uint16_t rd_u16(cursor *c) {
    if (!cur_ok(c, 2)) { c->p = c->end; return 0; }
    uint16_t v = (uint16_t)(c->p[0] | (c->p[1] << 8));   /* DWARF is little-endian */
    c->p += 2; return v;
}
static uint32_t rd_u32(cursor *c) {
    if (!cur_ok(c, 4)) { c->p = c->end; return 0; }
    uint32_t v = (uint32_t)c->p[0] | ((uint32_t)c->p[1] << 8) |
                 ((uint32_t)c->p[2] << 16) | ((uint32_t)c->p[3] << 24);
    c->p += 4; return v;
}
static uint64_t rd_u64(cursor *c) {
    if (!cur_ok(c, 8)) { c->p = c->end; return 0; }
    uint64_t v = 0;
    for (int i = 0; i < 8; i++) v |= (uint64_t)c->p[i] << (8 * i);
    c->p += 8; return v;
}

/* LEB128: variable-length integers. Each byte carries 7 payload bits; the high
 * bit (0x80) means "more bytes follow". Unsigned just accumulates; signed also
 * sign-extends from the final payload bit. This is DWARF's space-saving encoding
 * for the many small numbers in the line program. */
static uint64_t rd_uleb(cursor *c) {
    uint64_t result = 0; int shift = 0; uint8_t b;
    do {
        if (!cur_ok(c, 1)) break;
        b = *c->p++;
        result |= (uint64_t)(b & 0x7f) << shift;   /* low 7 bits are payload      */
        shift += 7;
    } while (b & 0x80);                            /* 0x80 = continuation bit      */
    return result;
}
static int64_t rd_sleb(cursor *c) {
    int64_t result = 0; int shift = 0; uint8_t b = 0;
    do {
        if (!cur_ok(c, 1)) break;
        b = *c->p++;
        result |= (int64_t)(b & 0x7f) << shift;
        shift += 7;
    } while (b & 0x80);
    /* Sign-extend: if the sign bit (0x40) of the last byte is set and we haven't
     * filled all 64 bits, smear ones into the top. */
    if (shift < 64 && (b & 0x40))
        result |= -((int64_t)1 << shift);
    return result;
}
/* Read a NUL-terminated string in place; advance the cursor past the NUL. */
static const char *rd_cstr(cursor *c) {
    const char *s = (const char *)c->p;
    while (c->p < c->end && *c->p) c->p++;
    if (c->p < c->end) c->p++;   /* step over the terminating NUL                */
    return s;
}

/* ===========================================================================
 * Section lookup helpers over the mmap'd ELF.
 * ======================================================================== */

/* Find a section by name. Returns its header and, via out params, a pointer to
 * its bytes and its size. NULL name-table or bad offsets yield NULL. */
static const Elf64_Shdr *find_section(const uint8_t *base, size_t len,
                                      const char *name,
                                      const uint8_t **data, uint64_t *size)
{
    if (len < sizeof(Elf64_Ehdr)) return NULL;
    const Elf64_Ehdr *eh = (const Elf64_Ehdr *)base;

    /* Sanity: correct magic and a 64-bit little-endian ELF. */
    if (memcmp(eh->e_ident, ELFMAG, SELFMAG) != 0) return NULL;
    if (eh->e_ident[EI_CLASS] != ELFCLASS64) return NULL;
    if (eh->e_shoff == 0 || eh->e_shnum == 0) return NULL;

    const Elf64_Shdr *sh = (const Elf64_Shdr *)(base + eh->e_shoff);
    /* The section-header string table holds every section's name. */
    if (eh->e_shstrndx >= eh->e_shnum) return NULL;
    const Elf64_Shdr *shstr = &sh[eh->e_shstrndx];
    const char *strs = (const char *)(base + shstr->sh_offset);

    for (int i = 0; i < eh->e_shnum; i++) {
        const char *sname = strs + sh[i].sh_name;
        if (strcmp(sname, name) == 0) {
            if (data) *data = base + sh[i].sh_offset;
            if (size) *size = sh[i].sh_size;
            return &sh[i];
        }
    }
    return NULL;
}

/* ===========================================================================
 * PART A: load the function symbol table.
 * ======================================================================== */
static void load_symbols(debuginfo *di, const uint8_t *base, size_t len)
{
    const uint8_t *symdata = NULL, *strdata = NULL;
    uint64_t symsize = 0, strsize = 0;

    /* .symtab holds the symbols; its sh_link would name the string table, but by
     * convention that is ".strtab", which we fetch by name for clarity. */
    const Elf64_Shdr *symhdr =
        find_section(base, len, ".symtab", &symdata, &symsize);
    find_section(base, len, ".strtab", &strdata, &strsize);
    if (!symhdr || !symdata || !strdata) {
        /* Stripped binary or none built with symbols: not fatal, just no names. */
        di->funcs = NULL; di->nfuncs = 0;
        return;
    }

    size_t count = (size_t)(symsize / sizeof(Elf64_Sym));
    const Elf64_Sym *syms = (const Elf64_Sym *)symdata;

    /* First pass: count STT_FUNC symbols so we allocate exactly once. */
    size_t nfunc = 0;
    for (size_t i = 0; i < count; i++)
        if (ELF64_ST_TYPE(syms[i].st_info) == STT_FUNC && syms[i].st_value != 0)
            nfunc++;

    di->funcs = calloc(nfunc, sizeof(func_sym));
    if (!di->funcs) { di->nfuncs = 0; return; }

    size_t k = 0;
    for (size_t i = 0; i < count && k < nfunc; i++) {
        if (ELF64_ST_TYPE(syms[i].st_info) != STT_FUNC) continue;
        if (syms[i].st_value == 0) continue;
        /* st_name is a byte offset into .strtab; guard it against corruption. */
        const char *nm = (syms[i].st_name < strsize)
                         ? (const char *)(strdata + syms[i].st_name) : "";
        di->funcs[k].name  = (char *)nm;   /* points INTO the mapping; not freed  */
        di->funcs[k].value = syms[i].st_value;
        di->funcs[k].size  = syms[i].st_size;
        k++;
    }
    di->nfuncs = k;
}

/* ===========================================================================
 * PART B: the DWARF v2/3/4 line-number program interpreter.
 * ======================================================================== */

/* Grow the row array geometrically; the debugger only builds this once at load. */
static int lt_push(line_table *lt, size_t *cap, line_row row)
{
    if (lt->n == *cap) {
        size_t ncap = *cap ? *cap * 2 : 256;
        line_row *nr = realloc(lt->rows, ncap * sizeof(line_row));
        if (!nr) return -1;
        lt->rows = nr; *cap = ncap;
    }
    lt->rows[lt->n++] = row;
    return 0;
}

/* Standard DWARF line opcodes (DWARF v4 §6.2.5.2). */
enum {
    DW_LNS_copy = 1, DW_LNS_advance_pc, DW_LNS_advance_line, DW_LNS_set_file,
    DW_LNS_set_column, DW_LNS_negate_stmt, DW_LNS_set_basic_block,
    DW_LNS_const_add_pc, DW_LNS_fixed_advance_pc, DW_LNS_set_prologue_end,
    DW_LNS_set_epilogue_begin, DW_LNS_set_isa
};
/* Extended opcodes (lead byte 0, then a length, then this sub-opcode). */
enum { DW_LNE_end_sequence = 1, DW_LNE_set_address = 2, DW_LNE_define_file = 3 };

static void di_run_line_program(debuginfo *di,
                                const uint8_t *ld, uint64_t ldsize)
{
    line_table *lt = &di->lt;
    size_t cap = 0;

    cursor c = { ld, ld + ldsize };

    /* A .debug_line section can hold several units (one per compilation unit).
     * Loop until we've consumed them all. */
    while (c.p < c.end) {
        const uint8_t *unit_start = c.p;

        /* ---- unit header ---------------------------------------------------
         * unit_length: bytes in this unit AFTER this field. 0xffffffff would
         * introduce 64-bit DWARF (a following 8-byte length); we handle only the
         * common 32-bit form and bail otherwise. */
        uint32_t unit_len = rd_u32(&c);
        if (unit_len == 0xffffffffu) break;            /* 64-bit DWARF: not handled */
        const uint8_t *unit_end = c.p + unit_len;      /* hard end of this unit     */
        if (unit_end > c.end) unit_end = c.end;

        uint16_t version = rd_u16(&c);
        if (version < 2 || version > 4) {
            /* v5 header layout differs (address_size/seg_sel + form-coded file
             * tables). Skip this unit rather than misparse it. */
            c.p = unit_end;
            continue;
        }

        uint32_t header_len = rd_u32(&c);   /* bytes from HERE to the first opcode  */
        const uint8_t *prog_start = c.p + header_len;

        uint8_t  min_inst_len = rd_u8(&c);
        /* v4 adds max_ops_per_insn (VLIW); for x86 it is always 1, and we assume
         * 1 so the op_index arithmetic collapses away. */
        uint8_t  max_ops = (version >= 4) ? rd_u8(&c) : 1;
        if (max_ops == 0) max_ops = 1;
        (void)max_ops;                      /* assumed 1 on x86 (no VLIW op_index)   */
        uint8_t  default_is_stmt = rd_u8(&c);
        (void)default_is_stmt;              /* is_stmt not needed to map addr->line  */
        int8_t   line_base  = (int8_t)rd_u8(&c);   /* signed! the special-opcode bias */
        uint8_t  line_range = rd_u8(&c);
        if (line_range == 0) line_range = 1;       /* guard: spec says >=1; avoid /0  */
        uint8_t  opcode_base = rd_u8(&c);

        /* standard_opcode_lengths[]: for each standard opcode, how many LEB
         * operands it takes. We read them so we can SKIP any opcode we don't
         * implement without desyncing the stream. */
        uint8_t std_len[32] = {0};
        for (int i = 1; i < opcode_base && i < 32; i++)
            std_len[i] = rd_u8(&c);
        for (int i = 32; i < opcode_base; i++) (void)rd_u8(&c);   /* discard extras */

        /* include_directories: NUL-terminated strings, list ends at an empty one.
         * We don't need the directory paths for file:line reporting, so we walk
         * past them. */
        while (c.p < unit_end) {
            const char *d = rd_cstr(&c);
            if (d[0] == '\0') break;
        }

        /* file_names: {name, dir_index(uleb), mtime(uleb), size(uleb)} until an
         * empty name. DWARF v<5 file indices are 1-based, so slot 0 is a filler. */
        char  **files = NULL;
        size_t  nfiles = 1;         /* reserve index 0                             */
        size_t  fcap = 8;
        files = calloc(fcap, sizeof(char *));
        if (files) files[0] = (char *)"<0>";
        while (c.p < unit_end) {
            const char *name = rd_cstr(&c);
            if (name[0] == '\0') break;
            (void)rd_uleb(&c);      /* dir index  — unused here                    */
            (void)rd_uleb(&c);      /* mtime      — unused                         */
            (void)rd_uleb(&c);      /* file size  — unused                         */
            if (files) {
                if (nfiles == fcap) {
                    fcap *= 2;
                    char **nf = realloc(files, fcap * sizeof(char *));
                    if (!nf) break;
                    files = nf;
                }
                files[nfiles++] = (char *)name;   /* points INTO the mapping      */
            }
        }
        /* The last unit's file table wins as the debuginfo-wide table. For a
         * single-CU teaching program that is exactly what we want. */
        if (files) {
            free(lt->files);
            lt->files  = files;
            lt->nfiles = nfiles;
        }

        /* ---- run the state machine ----------------------------------------- */
        c.p = prog_start;           /* jump to the first opcode                    */

        /* State machine registers (reset at the start and after end_sequence).
         * The real machine also tracks is_stmt/basic_block/column; we only need
         * (address,file,line) to answer addr->line, so we omit the rest. */
        uint64_t address = 0;
        uint32_t file = 1, line = 1;

        while (c.p < unit_end) {
            uint8_t op = rd_u8(&c);

            if (op == 0) {
                /* ---- extended opcode: 0, uleb length, sub-opcode ---------- */
                uint64_t exlen = rd_uleb(&c);
                const uint8_t *ex_end = c.p + exlen;
                uint8_t sub = rd_u8(&c);
                switch (sub) {
                case DW_LNE_end_sequence: {
                    /* Emit the terminating row: it caps the previous range and
                     * carries `end=1` (no source line of its own). Then reset. */
                    line_row r = { address, file, line, 1 };
                    lt_push(lt, &cap, r);
                    address = 0; file = 1; line = 1;   /* reset for the next sequence */
                    break;
                }
                case DW_LNE_set_address:
                    /* The only place a full absolute address enters the machine.
                     * Operand is address_size bytes; on x86-64 that is 8. */
                    address = rd_u64(&c);
                    break;
                case DW_LNE_define_file:
                    /* rarely used; skip its operands via the declared length */
                    break;
                default:
                    break;
                }
                c.p = ex_end;       /* trust the length to resynchronise           */
            }
            else if (op < opcode_base) {
                /* ---- standard opcode ------------------------------------- */
                switch (op) {
                case DW_LNS_copy: {
                    /* Emit a row for the current (address,file,line). */
                    line_row r = { address, file, line, 0 };
                    lt_push(lt, &cap, r);
                    break;
                }
                case DW_LNS_advance_pc:
                    /* address += min_inst_len * operand  (op_index math elided) */
                    address += min_inst_len * rd_uleb(&c);
                    break;
                case DW_LNS_advance_line:
                    line = (uint32_t)((int64_t)line + rd_sleb(&c));   /* signed!   */
                    break;
                case DW_LNS_set_file:
                    file = (uint32_t)rd_uleb(&c);
                    break;
                case DW_LNS_set_column:
                    (void)rd_uleb(&c);
                    break;
                case DW_LNS_negate_stmt:
                    /* toggles is_stmt in the full machine; irrelevant to addr->line */
                    break;
                case DW_LNS_set_basic_block:
                    break;
                case DW_LNS_const_add_pc: {
                    /* Advance address by the amount special opcode 255 would, but
                     * without emitting a row. adj = 255 - opcode_base. */
                    uint8_t adj = (uint8_t)(255 - opcode_base);
                    address += min_inst_len * (adj / line_range);
                    break;
                }
                case DW_LNS_fixed_advance_pc:
                    /* Operand is a raw uhalf (NOT scaled by min_inst_len). */
                    address += rd_u16(&c);
                    break;
                case DW_LNS_set_prologue_end:
                case DW_LNS_set_epilogue_begin:
                    break;
                case DW_LNS_set_isa:
                    (void)rd_uleb(&c);
                    break;
                default:
                    /* Unknown standard opcode: skip its declared LEB operands so
                     * we stay byte-aligned with the stream. */
                    for (int i = 0; i < std_len[op & 31]; i++) (void)rd_uleb(&c);
                    break;
                }
            }
            else {
                /* ---- special opcode: the common, compact case ------------- *
                 * One byte both advances the address AND the line AND emits a
                 * row. This is where most of the table comes from.            */
                uint8_t adj = (uint8_t)(op - opcode_base);
                address += min_inst_len * (adj / line_range);
                line     = (uint32_t)((int64_t)line +
                                      line_base + (adj % line_range));
                line_row r = { address, file, line, 0 };
                lt_push(lt, &cap, r);
            }
        }

        c.p = unit_end;             /* next unit (if any)                          */
        (void)unit_start;
    }
}

/* Sort rows by address (stable-ish insertion is fine; tables are small). qsort
 * comparator over uint64_t addresses. */
static int row_cmp(const void *a, const void *b)
{
    uint64_t x = ((const line_row *)a)->addr;
    uint64_t y = ((const line_row *)b)->addr;
    return (x > y) - (x < y);
}

/* ===========================================================================
 * di_load — mmap the ELF and populate symbols + line table.
 * ======================================================================== */
int di_load(debuginfo *di, const char *path)
{
    memset(di, 0, sizeof *di);
    di->path = strdup(path);

    int fd = open(path, O_RDONLY);
    if (fd < 0) { perror(path); return -1; }

    struct stat st;
    if (fstat(fd, &st) < 0 || st.st_size <= 0) {
        perror("fstat"); close(fd); return -1;
    }

    /* MAP_PRIVATE, PROT_READ: we never write the file; every string pointer we
     * hand out points into THIS mapping, so it stays valid until di_free. */
    void *map = mmap(NULL, (size_t)st.st_size, PROT_READ, MAP_PRIVATE, fd, 0);
    close(fd);                       /* the mapping keeps its own reference        */
    if (map == MAP_FAILED) { perror("mmap"); return -1; }

    di->map     = map;
    di->map_len = (size_t)st.st_size;

    const uint8_t *base = (const uint8_t *)map;
    const Elf64_Ehdr *eh = (const Elf64_Ehdr *)base;
    /* PIE (ET_DYN) vs fixed (ET_EXEC) decides whether link addresses need a base. */
    di->is_pie = (eh->e_type == ET_DYN);

    load_symbols(di, base, di->map_len);

    const uint8_t *ld = NULL; uint64_t ldsize = 0;
    if (find_section(base, di->map_len, ".debug_line", &ld, &ldsize) && ld)
        di_run_line_program(di, ld, ldsize);

    if (di->lt.n > 1)
        qsort(di->lt.rows, di->lt.n, sizeof(line_row), row_cmp);

    di->ok = (di->nfuncs > 0) || (di->lt.n > 0);
    if (!di->ok)
        fprintf(stderr,
                "warning: no symbols or .debug_line in %s "
                "(build the target with -g -gdwarf-4)\n", path);
    return 0;
}

void di_free(debuginfo *di)
{
    if (di->map && di->map != MAP_FAILED) munmap(di->map, di->map_len);
    free(di->funcs);
    free(di->lt.rows);
    free(di->lt.files);
    free(di->path);
    memset(di, 0, sizeof *di);
}

/* ===========================================================================
 * Queries. All operate in LINK-TIME address space; the caller subtracts the
 * runtime load base before calling and adds it back to any address returned.
 * ======================================================================== */

/* di_addr_to_line — binary search the sorted rows.
 *
 * A source line covers the half-open range [rows[i].addr, rows[i+1].addr). We
 * find the greatest row whose addr <= link_addr, then confirm link_addr is below
 * the NEXT row's addr and that our row is not an end_sequence sentinel. This is
 * the same "upper_bound then step back" search that asm/demo.c annotates. */
int di_addr_to_line(debuginfo *di, uint64_t link_addr,
                    const char **file, uint32_t *line)
{
    line_table *lt = &di->lt;
    if (lt->n == 0) return 0;

    /* Find first row with addr > link_addr (upper bound), then back up one. */
    size_t lo = 0, hi = lt->n;
    while (lo < hi) {
        size_t mid = lo + (hi - lo) / 2;
        if (lt->rows[mid].addr <= link_addr) lo = mid + 1;
        else                                 hi = mid;
    }
    if (lo == 0) return 0;                    /* before the first row              */
    line_row *r = &lt->rows[lo - 1];
    if (r->end) return 0;                     /* the sentinel row: address is a gap */

    if (file) {
        const char *name = "?";
        if (r->file < lt->nfiles && lt->files) name = lt->files[r->file];
        *file = name;
    }
    if (line) *line = r->line;
    return 1;
}

/* di_addr_to_func — linear scan for the function whose [value,value+size)
 * contains link_addr. Small symbol counts make this fine. */
const char *di_addr_to_func(debuginfo *di, uint64_t link_addr, uint64_t *off)
{
    for (size_t i = 0; i < di->nfuncs; i++) {
        func_sym *f = &di->funcs[i];
        uint64_t end = f->value + (f->size ? f->size : 1);
        if (link_addr >= f->value && link_addr < end) {
            if (off) *off = link_addr - f->value;
            return f->name;
        }
    }
    return NULL;
}

/* di_line_to_addr — first row matching file:line. `file` may be a bare basename
 * ("sample.c"); we match on the basename of each row's file name. */
int di_line_to_addr(debuginfo *di, const char *file, uint32_t line,
                    uint64_t *link_addr)
{
    line_table *lt = &di->lt;
    uint64_t best = 0; int found = 0;

    for (size_t i = 0; i < lt->n; i++) {
        line_row *r = &lt->rows[i];
        if (r->end || r->line != line) continue;
        const char *rf = (r->file < lt->nfiles && lt->files)
                         ? lt->files[r->file] : "";
        const char *rb = strrchr(rf, '/'); rb = rb ? rb + 1 : rf;
        /* Match either the exact stored name or its basename, so "sample.c"
         * resolves regardless of how the compiler recorded the path. */
        if (file[0] && strcmp(rb, file) != 0 && strcmp(rf, file) != 0)
            continue;
        /* Pick the LOWEST address for this line (its first instruction). */
        if (!found || r->addr < best) { best = r->addr; found = 1; }
    }
    if (!found) return 0;
    *link_addr = best;
    return 1;
}

/* di_func_to_addr — exact function-name match -> entry address. */
int di_func_to_addr(debuginfo *di, const char *name, uint64_t *link_addr)
{
    for (size_t i = 0; i < di->nfuncs; i++) {
        if (di->funcs[i].name && strcmp(di->funcs[i].name, name) == 0) {
            *link_addr = di->funcs[i].value;
            return 1;
        }
    }
    return 0;
}
