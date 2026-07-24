/* ===========================================================================
 * objdump.c — objdump -d "lite": linear-sweep the .text section and annotate
 *             branch targets with symbol names; plus an addr2line-style lookup.
 * ===========================================================================
 *
 * Two capabilities, one shared engine:
 *
 *   1. A SORTED symbol index built once from the symbol table (functions and
 *      sized objects with a value). Sorting by address turns "which symbol owns
 *      this address?" into a BINARY SEARCH — O(log n) instead of O(n). That
 *      search is the exact routine extracted into asm/demo.c, because it is the
 *      pure-logic heart of every symbolizer (backtraces, profilers, addr2line).
 *
 *   2. A linear sweep that decodes .text one instruction at a time with the
 *      disasm.c backend and, for each PC-relative branch/call, looks up the
 *      target in that index to print `call 0x... <printf>` the way objdump does.
 * ===========================================================================
 */
#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <inttypes.h>
#include "elftk.h"
#include "disasm.h"

/* One entry of the address-sorted symbol index. `name` aliases into the mapped
 * string table (no ownership); the array itself is heap-allocated. */
struct syment {
    uint64_t    addr;   /* st_value: the symbol's virtual address                */
    uint64_t    size;   /* st_size: 0 when the toolchain didn't record it        */
    const char *name;
};

/* qsort comparator: ascending by address, then by (larger size first) so that
 * when two symbols share an address the more specific one sorts earlier. */
static int cmp_sym(const void *pa, const void *pb)
{
    const struct syment *a = pa, *b = pb;
    if (a->addr < b->addr) return -1;
    if (a->addr > b->addr) return 1;
    if (a->size > b->size) return -1;
    if (a->size < b->size) return 1;
    return 0;
}

/* Build the sorted index from whichever symbol table is present. Returns the
 * array (caller frees) and writes the count through *out_n. */
static struct syment *build_index(const struct elf_file *f, unsigned *out_n)
{
    const Elf64_Sym *syms = f->symtab; unsigned n = f->symcount;
    const char *str = f->symstr; size_t strsz = f->symstrsz;
    if (!syms) { syms = f->dynsym; n = f->dyncount; str = f->dynstr; strsz = f->dynstrsz; }
    *out_n = 0;
    if (!syms || n == 0) return NULL;

    struct syment *arr = malloc(n * sizeof *arr);
    if (!arr) return NULL;
    unsigned m = 0;
    for (unsigned i = 1; i < n; i++) {          /* skip the null symbol 0        */
        unsigned t = ELF64_ST_TYPE(syms[i].st_info);
        if (t != STT_FUNC && t != STT_OBJECT && t != STT_NOTYPE) continue;
        if (syms[i].st_value == 0) continue;    /* undefined / no address        */
        const char *nm = ef_str(str, strsz, syms[i].st_name);
        if (nm[0] == '\0') continue;
        arr[m].addr = syms[i].st_value;
        arr[m].size = syms[i].st_size;
        arr[m].name = nm;
        m++;
    }
    qsort(arr, m, sizeof *arr, cmp_sym);
    *out_n = m;
    return arr;
}

/* ---------------------------------------------------------------------------
 * sym_lookup — the binary search. Find the symbol whose range covers `addr`:
 * the greatest entry with entry.addr <= addr. Returns its index, or -1 if addr
 * is below every symbol. (This mirrors asm/demo.c::sym_by_addr exactly.)
 * --------------------------------------------------------------------------- */
static int sym_lookup(const struct syment *arr, unsigned n, uint64_t addr)
{
    int lo = 0, hi = (int)n - 1, ans = -1;
    while (lo <= hi) {
        int mid = lo + (hi - lo) / 2;           /* avoids lo+hi overflow          */
        if (arr[mid].addr <= addr) {            /* candidate; look right for a    */
            ans = mid;                          /*   closer (higher) match        */
            lo = mid + 1;
        } else {
            hi = mid - 1;
        }
    }
    return ans;
}

/* ===========================================================================
 * addr2line-lite: print "name+0xoffset" for an address.
 * =========================================================================== */
int od_addr2line(const struct elf_file *f, uint64_t addr)
{
    unsigned n; struct syment *arr = build_index(f, &n);
    if (!arr) { fprintf(stderr, "elftk: no symbols to resolve against\n"); return -1; }

    int i = sym_lookup(arr, n, addr);
    if (i < 0) {
        printf("0x%" PRIx64 ": ??\n", addr);
    } else {
        uint64_t off = addr - arr[i].addr;
        /* If the symbol has a known size and the address is past its end, we are
         * really in the gap after it — say so rather than lie. */
        if (arr[i].size && off >= arr[i].size)
            printf("0x%" PRIx64 ": %s+0x%" PRIx64 " (past end, size 0x%" PRIx64 ")\n",
                   addr, arr[i].name, off, arr[i].size);
        else if (off)
            printf("0x%" PRIx64 ": %s+0x%" PRIx64 "\n", addr, arr[i].name, off);
        else
            printf("0x%" PRIx64 ": %s\n", addr, arr[i].name);
    }
    free(arr);
    return 0;
}

/* Print a compact " <name+0x..>" annotation for a resolved target, if any. */
static void annotate(const struct syment *arr, unsigned n, uint64_t target)
{
    int i = sym_lookup(arr, n, target);
    if (i < 0) return;
    uint64_t off = target - arr[i].addr;
    if (arr[i].size && off >= arr[i].size) return;   /* not actually inside it    */
    if (off) printf(" <%s+0x%" PRIx64 ">", arr[i].name, off);
    else     printf(" <%s>", arr[i].name);
}

/* ===========================================================================
 * od_disasm — the linear sweep over .text.
 * =========================================================================== */
int od_disasm(const struct elf_file *f)
{
    if (!f->shdr) { fprintf(stderr, "elftk: no sections to disassemble\n"); return -1; }

    /* Locate the .text section (executable PROGBITS). We match by name to stay
     * close to objdump; any SHF_EXECINSTR PROGBITS section would also work. */
    const Elf64_Shdr *text = NULL;
    for (unsigned i = 0; i < f->shnum; i++) {
        const Elf64_Shdr *sh = &f->shdr[i];
        if (sh->sh_type == SHT_PROGBITS && (sh->sh_flags & SHF_EXECINSTR) &&
            strcmp(ef_secname(f, sh), ".text") == 0) { text = sh; break; }
    }
    if (!text) {
        for (unsigned i = 0; i < f->shnum; i++) {    /* fall back to any code sec */
            const Elf64_Shdr *sh = &f->shdr[i];
            if (sh->sh_type == SHT_PROGBITS && (sh->sh_flags & SHF_EXECINSTR)) {
                text = sh; break;
            }
        }
    }
    if (!text) { fprintf(stderr, "elftk: no executable section found\n"); return -1; }

    const uint8_t *code = ef_ptr(f, text->sh_offset, text->sh_size);
    if (!code) { fprintf(stderr, "elftk: .text out of bounds\n"); return -1; }

    unsigned n; struct syment *arr = build_index(f, &n);   /* may be NULL/stripped */

    printf("\nDisassembly of section %s:\n", ef_secname(f, text));

    uint64_t base = text->sh_addr;             /* virtual address of code[0]      */
    uint64_t sz   = text->sh_size;
    uint64_t off  = 0;                          /* byte offset within .text        */
    unsigned si   = 0;                          /* cursor into the sorted symbols  */

    while (off < sz) {
        uint64_t va = base + off;

        /* Print a "<name>:" label whenever we reach a symbol's start address.
         * Because both the sweep and the symbol array advance monotonically, we
         * just walk `si` forward rather than searching each time. */
        while (arr && si < n && arr[si].addr <= va) {
            if (arr[si].addr == va && arr[si].name[0])
                printf("\n%016" PRIx64 " <%s>:\n", va, arr[si].name);
            si++;
        }

        struct insn in;
        unsigned len = x86_decode(code + off, (unsigned)(sz - off), va, &in);

        /* Address + raw bytes column (objdump-style), then the mnemonic text. */
        printf("  %6" PRIx64 ":\t", va);
        for (unsigned b = 0; b < len; b++) printf("%02x ", code[off + b]);
        /* pad the byte column to a fixed width for alignment (up to ~10 bytes). */
        for (unsigned b = len; b < 10; b++) printf("   ");
        printf("\t%s", in.text);

        /* Annotate a resolved branch/call target with the owning symbol name. */
        if (in.has_target && arr) annotate(arr, n, in.target);
        printf("\n");

        off += len;                            /* len is always >= 1: no infinite loop */
    }

    free(arr);
    return 0;
}
