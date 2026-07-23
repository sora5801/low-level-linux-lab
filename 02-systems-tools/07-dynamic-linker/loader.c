/* ===========================================================================
 * loader.c — a tiny user-space dynamic linker / loader for x86-64 Linux.
 * ===========================================================================
 *
 * WHAT THIS IS
 * ------------
 * This program does, by hand, the job the kernel normally delegates to
 * `/lib64/ld-linux-x86-64.so.2` (the "dynamic loader", a.k.a. the ELF
 * interpreter). You run it like this on Linux/WSL:
 *
 *     ./loader ./prog arg1 arg2 ...
 *
 * and it will:
 *   1. open and parse the ELF header + program headers of ./prog,
 *   2. mmap() every PT_LOAD segment at the right address with the right
 *      page protections (and zero the .bss tail),
 *   3. walk PT_DYNAMIC to find the relocation tables, symbol table, string
 *      table, DT_NEEDED list, and init arrays,
 *   4. recursively load the shared objects named by DT_NEEDED,
 *   5. apply the relocations (R_X86_64_RELATIVE / GLOB_DAT / JUMP_SLOT /
 *      IRELATIVE / 64, plus the compact DT_RELR form), resolving symbols
 *      against every loaded object — i.e. wire up the GOT and PLT,
 *   6. apply PT_GNU_RELRO (make the just-relocated GOT read-only),
 *   7. run the DT_INIT / DT_INIT_ARRAY constructors, dependencies first,
 *   8. build a fresh initial stack (argc, argv, envp, and a patched auxv),
 *   9. and finally jump to the program's entry point with a clean register
 *      state, exactly as the kernel would.
 *
 * WHY IT HAS NO libc
 * ------------------
 * A dynamic linker is the thing that *sets up* libc. It therefore cannot use
 * libc: at the moment it runs, the C library it is about to relocate is not
 * yet usable. Real ld.so is written freestanding, with its own syscall
 * wrappers and its own memcpy/strlen. We do the same. The bonus for this
 * teaching lab is that a source file with no `#include` compiles straight to
 * Linux assembly with `clang --target=x86_64-pc-linux-gnu -S`, so the .s files
 * we ship reflect *this real code*, not a toy.
 *
 * SCOPE (be honest — this is a teaching core, not glibc's ld.so)
 * --------------------------------------------------------------
 * FULLY WORKS:
 *   - static-PIE executables (ET_DYN, only RELATIVE/RELR/IRELATIVE relocs),
 *   - simple dynamic executables that pull in *our own* freestanding shared
 *     objects (see test/), resolving GLOB_DAT / JUMP_SLOT / R_X86_64_64
 *     symbols among the loaded set, with eager (BIND_NOW-style) binding.
 * DELIBERATELY NOT COVERED (explained in README, would each be a project):
 *   - Thread-Local Storage relocations (DTPMOD64/DTPOFF64/TPOFF64) and the
 *     %fs / TCB / DTV setup they need,
 *   - symbol versioning (DT_VERSYM/VERNEED) — we bind to the base name,
 *   - lazy PLT binding through a runtime resolver trampoline (we bind eagerly;
 *     the README explains how the lazy path works),
 *   - loading real glibc (it needs all of the above).
 *
 * The System V AMD64 ABI, obeyed throughout:
 *   - function args:  rdi, rsi, rdx, rcx, r8, r9   (then the stack)
 *   - return value:   rax
 *   - a `syscall`:    number in rax; args in rdi, rsi, rdx, r10, r8, r9;
 *                     result in rax; the instruction DESTROYS rcx and r11.
 *   - callee-saved:   rbx, rbp, r12-r15.
 * ===========================================================================
 */

/* ---------------------------------------------------------------------------
 * 0. Fixed-width types. We are freestanding: no <stdint.h>. On the LP64 model
 *    Linux uses for x86-64, `long`/`unsigned long` are 64-bit and hold a
 *    pointer, a size, or a syscall return without truncation.
 * ------------------------------------------------------------------------- */
typedef unsigned char      u8;
typedef unsigned short     u16;
typedef unsigned int       u32;
typedef unsigned long      u64;
typedef long               i64;
typedef unsigned long      usize;

#define NULL ((void *)0)

/* ===========================================================================
 * 1. The kernel ABI: syscall numbers and inline-asm wrappers.
 *    These are the *only* way this program touches the outside world.
 * ========================================================================= */

#define SYS_read         0
#define SYS_write        1
#define SYS_open         2
#define SYS_close        3
#define SYS_mmap         9
#define SYS_mprotect    10
#define SYS_munmap      11
#define SYS_pread64     17
#define SYS_openat     257
#define SYS_exit_group 231

/* openat(2) special dirfd meaning "resolve relative to the CWD". */
#define AT_FDCWD (-100)
#define O_RDONLY 0

/* mmap(2) protection bits and flags (from <sys/mman.h>, spelled out here). */
#define PROT_NONE  0x0
#define PROT_READ  0x1
#define PROT_WRITE 0x2
#define PROT_EXEC  0x4
#define MAP_PRIVATE   0x02
#define MAP_FIXED     0x10
#define MAP_ANONYMOUS 0x20

/* The raw syscall sequence. `syscall` clobbers rcx and r11 (it stashes the
 * return RIP in rcx and RFLAGS in r11), and it may touch memory, so we list
 * "rcx","r11","memory" as clobbers — forgetting "memory" is a classic bug that
 * lets the compiler cache a buffer across the call. Args 4/5/6 go in r10/r8/r9
 * for a syscall (NOT rcx/r8/r9 as for a normal call — the kernel uses r10 for
 * arg4 because the `syscall` instruction itself steals rcx). */
static inline long syscall6(long n, long a1, long a2, long a3,
                            long a4, long a5, long a6)
{
    long ret;
    register long r10 __asm__("r10") = a4;
    register long r8  __asm__("r8")  = a5;
    register long r9  __asm__("r9")  = a6;
    __asm__ volatile("syscall"
                     : "=a"(ret)
                     : "a"(n), "D"(a1), "S"(a2), "d"(a3),
                       "r"(r10), "r"(r8), "r"(r9)
                     : "rcx", "r11", "memory");
    return ret;
}
static inline long syscall1(long n, long a1)
{ return syscall6(n, a1, 0, 0, 0, 0, 0); }
static inline long syscall3(long n, long a1, long a2, long a3)
{ return syscall6(n, a1, a2, a3, 0, 0, 0); }
static inline long syscall4(long n, long a1, long a2, long a3, long a4)
{ return syscall6(n, a1, a2, a3, a4, 0, 0); }

/* Typed wrappers. Each returns the kernel's value: >=0 on success, or a small
 * negative number (-errno) on failure. We check every one — the error paths
 * are half the point of a loader (a missing library, a truncated file, an
 * mmap that collides) are all things it must diagnose, not crash on. */
static long sys_write(int fd, const void *buf, u64 n)
{ return syscall3(SYS_write, fd, (long)buf, (long)n); }

static long sys_openat(const char *path)
{ return syscall4(SYS_openat, AT_FDCWD, (long)path, O_RDONLY, 0); }

static long sys_close(int fd)
{ return syscall1(SYS_close, fd); }

static long sys_pread(int fd, void *buf, u64 n, u64 off)
{ return syscall4(SYS_pread64, fd, (long)buf, (long)n, (long)off); }

/* mmap(2): map `len` bytes of `fd` (or anonymous memory) at `addr`. Returns the
 * mapped address, or a value in [-4095, -1] on error. The x86-64 mmap takes a
 * byte offset (unlike the legacy mmap2 which took pages), so we pass `off`
 * directly. */
static long sys_mmap(long addr, u64 len, long prot, long flags, long fd, u64 off)
{ return syscall6(SYS_mmap, addr, (long)len, prot, flags, fd, (long)off); }

static long sys_mprotect(long addr, u64 len, long prot)
{ return syscall3(SYS_mprotect, addr, (long)len, prot); }

__attribute__((noreturn))
static void sys_exit(int code)
{ syscall1(SYS_exit_group, code); __builtin_unreachable(); }

/* An mmap result is an error iff it lands in the top page of the address space,
 * i.e. the kernel returned -errno. This is the standard "is this a MAP_FAILED"
 * test used inside libcs. */
static int mmap_failed(long r) { return (u64)r > (u64)-4096UL; }

/* ===========================================================================
 * 2. Freestanding libc-free helpers (no <string.h>, no <stdio.h>).
 * ========================================================================= */

/* The compiler, especially at -O2, may lower a struct copy or an array zero
 * into a *call* to memcpy/memset. Because we link -nostdlib, those symbols must
 * exist. We provide them. The `volatile` destination pointer defeats clang's
 * "recognise this loop and replace it with a call to memset" pass — otherwise
 * memset would call itself forever. Not fast; a loader does tiny copies. */
__attribute__((used))
void *memset(void *dst, int c, unsigned long n)
{
    volatile unsigned char *p = (volatile unsigned char *)dst;
    while (n--) *p++ = (unsigned char)c;
    return dst;
}
__attribute__((used))
void *memcpy(void *dst, const void *src, unsigned long n)
{
    volatile unsigned char *d = (volatile unsigned char *)dst;
    const unsigned char *s = (const unsigned char *)src;
    while (n--) *d++ = *s++;
    return dst;
}

static usize kstrlen(const char *s)
{ const char *p = s; while (*p) p++; return (usize)(p - s); }

/* Return 1 if the two NUL-terminated strings are equal, else 0. */
static int kstreq(const char *a, const char *b)
{
    while (*a && *a == *b) { a++; b++; }
    return *a == *b;
}

/* Copy string `s` into `dst` (capacity `cap`), NUL-terminating. Returns length
 * written. Used to build "dir/lib.so" paths on the stack. */
static usize kstrcpy(char *dst, usize cap, const char *s)
{
    usize i = 0;
    while (s[i] && i + 1 < cap) { dst[i] = s[i]; i++; }
    dst[i] = 0;
    return i;
}

/* Index of the last '/' in `s`, or -1. Used to derive the directory of the
 * program so we can find its sibling libraries. */
static long klast_slash(const char *s)
{
    long idx = -1;
    for (long i = 0; s[i]; i++) if (s[i] == '/') idx = i;
    return idx;
}

/* ---- diagnostics: write to fd 2 (stderr). No printf; we build bytes. ---- */
static void dstr(const char *s) { sys_write(2, s, kstrlen(s)); }

/* Print a value as 0x-prefixed hex. Handy for dumping addresses/offsets. */
static void dhex(u64 v)
{
    static const char H[] = "0123456789abcdef";
    char buf[19];               /* "0x" + 16 nibbles + NUL */
    buf[0] = '0'; buf[1] = 'x';
    for (int i = 0; i < 16; i++)
        buf[2 + i] = H[(v >> ((15 - i) * 4)) & 0xf];
    buf[18] = 0;
    dstr(buf);
}

/* Fatal error: print a message (and optional hex detail), then kill the
 * process with status 127 — the shell's conventional "loader failed" code. */
__attribute__((noreturn))
static void die(const char *msg)
{ dstr("loader: "); dstr(msg); dstr("\n"); sys_exit(127); }

__attribute__((noreturn))
static void die2(const char *msg, u64 detail)
{ dstr("loader: "); dstr(msg); dstr(" "); dhex(detail); dstr("\n"); sys_exit(127); }

/* Verbose tracing, toggled by the environment variable LDLAB_DEBUG=1. Reading
 * the trace next to the code is the fastest way to understand what a loader
 * actually does step by step. */
static int g_debug = 0;
static void trace(const char *s) { if (g_debug) { dstr("[ld] "); dstr(s); dstr("\n"); } }
static void trace2(const char *s, u64 v) { if (g_debug) { dstr("[ld] "); dstr(s); dstr(" "); dhex(v); dstr("\n"); } }

/* ===========================================================================
 * 3. ELF64 on-disk structures. Natural alignment already matches the ELF64
 *    layout, so no packing pragmas are needed on x86-64.
 * ========================================================================= */

typedef struct {
    u8  e_ident[16];    /* magic + class/data/version                        */
    u16 e_type;         /* ET_EXEC / ET_DYN / ...                            */
    u16 e_machine;      /* EM_X86_64 == 62                                   */
    u32 e_version;
    u64 e_entry;        /* virtual address of the entry point                */
    u64 e_phoff;        /* file offset of the program header table           */
    u64 e_shoff;
    u32 e_flags;
    u16 e_ehsize;
    u16 e_phentsize;    /* size of one program header (== 56)                */
    u16 e_phnum;        /* number of program headers                         */
    u16 e_shentsize, e_shnum, e_shstrndx;
} Elf64_Ehdr;

typedef struct {
    u32 p_type;         /* PT_LOAD / PT_DYNAMIC / PT_INTERP / PT_GNU_RELRO... */
    u32 p_flags;        /* PF_R | PF_W | PF_X                                */
    u64 p_offset;       /* offset of this segment in the file                */
    u64 p_vaddr;        /* virtual address it wants (before load bias)       */
    u64 p_paddr;
    u64 p_filesz;       /* bytes present in the file                         */
    u64 p_memsz;        /* bytes in memory (>= filesz; the excess is .bss)   */
    u64 p_align;
} Elf64_Phdr;

typedef struct { i64 d_tag; u64 d_val; } Elf64_Dyn;

typedef struct {
    u32 st_name;        /* offset into the object's string table             */
    u8  st_info;        /* bind<<4 | type                                    */
    u8  st_other;
    u16 st_shndx;       /* SHN_UNDEF (0) means "not defined here"            */
    u64 st_value;       /* address (before load bias) if defined             */
    u64 st_size;
} Elf64_Sym;

/* RELA: a relocation *with* an explicit addend (x86-64 always uses RELA). */
typedef struct {
    u64 r_offset;       /* where to patch (a vaddr, before load bias)        */
    u64 r_info;         /* sym index (high 32) | reloc type (low 32)         */
    i64 r_addend;       /* the "A" in "S + A"                                */
} Elf64_Rela;

typedef struct { u64 a_type; u64 a_val; } Elf64_auxv_t;

/* ---- selected constants (the full lists live in <elf.h>) ---- */
#define ET_EXEC 2
#define ET_DYN  3
#define EM_X86_64 62

#define PT_LOAD        1
#define PT_DYNAMIC     2
#define PT_INTERP      3
#define PT_PHDR        6
#define PT_TLS         7
#define PT_GNU_RELRO   0x6474e552

#define PF_X 1
#define PF_W 2
#define PF_R 4

#define DT_NULL         0
#define DT_NEEDED       1
#define DT_PLTRELSZ     2
#define DT_PLTGOT       3
#define DT_HASH         4
#define DT_STRTAB       5
#define DT_SYMTAB       6
#define DT_RELA         7
#define DT_RELASZ       8
#define DT_RELAENT      9
#define DT_STRSZ        10
#define DT_SYMENT       11
#define DT_INIT         12
#define DT_SONAME       14
#define DT_RPATH        15
#define DT_REL          17
#define DT_PLTREL       20
#define DT_JMPREL       23
#define DT_INIT_ARRAY   25
#define DT_INIT_ARRAYSZ 27
#define DT_RUNPATH      29
#define DT_FLAGS        30
#define DT_GNU_HASH     0x6ffffef5
#define DT_RELR         0x6fffffef
#define DT_RELRSZ       0x6fffffee
#define DT_RELRENT      0x6fffffed
#define DT_RELACOUNT    0x6ffffff9

/* x86-64 relocation types — the ones a loader must understand. */
#define R_X86_64_NONE       0
#define R_X86_64_64         1   /* S + A, 64-bit absolute                    */
#define R_X86_64_COPY       5   /* copy bytes from S into P (data interpos.) */
#define R_X86_64_GLOB_DAT   6   /* S + A into a GOT slot (data symbol)       */
#define R_X86_64_JUMP_SLOT  7   /* S + A into a PLT/GOT slot (function)      */
#define R_X86_64_RELATIVE   8   /* B + A: rebase a pointer by the load bias  */
#define R_X86_64_DTPMOD64  16   /* TLS: module id            (UNSUPPORTED)   */
#define R_X86_64_DTPOFF64  17   /* TLS: offset in module     (UNSUPPORTED)   */
#define R_X86_64_TPOFF64   18   /* TLS: offset from thread ptr (UNSUPPORTED) */
#define R_X86_64_IRELATIVE 37   /* B + A is a resolver; store its result     */

/* Extract the symbol index / relocation type from r_info. */
#define ELF64_R_SYM(i)  ((i) >> 32)
#define ELF64_R_TYPE(i) ((i) & 0xffffffffUL)

/* Symbol binding / type from st_info. */
#define ELF64_ST_BIND(i) ((i) >> 4)
#define ELF64_ST_TYPE(i) ((i) & 0xf)
#define STB_LOCAL  0
#define STB_GLOBAL 1
#define STB_WEAK   2
#define STT_GNU_IFUNC 10
#define SHN_UNDEF 0

/* auxv entry types the kernel passes on the initial stack; we forward most and
 * override the program-specific ones. */
#define AT_NULL   0
#define AT_PHDR   3
#define AT_PHENT  4
#define AT_PHNUM  5
#define AT_PAGESZ 6
#define AT_BASE   7
#define AT_ENTRY  9
#define AT_EXECFN 31

/* ===========================================================================
 * 4. The link map: one entry per loaded object (executable + each library).
 *    Everything the relocator needs is captured here at load time. All the
 *    pointer-valued fields are already adjusted by the object's load bias, so
 *    the rest of the code can dereference them directly.
 * ========================================================================= */

#define MAX_OBJS     32
#define MAX_NEEDED   16
#define PATH_MAX   4096

struct obj {
    const char *name;           /* how we found it (path or soname)          */
    const char *soname;         /* DT_SONAME, or NULL                        */
    u64  base;                  /* load bias B: runtime_addr = base + vaddr  */
    Elf64_Phdr *phdr;           /* program headers, as mapped in memory      */
    u64  phnum;
    u64  entry;                 /* base + e_entry (meaningful for the exe)   */

    Elf64_Sym  *symtab;         /* DT_SYMTAB (dynamic symbols)               */
    const char *strtab;         /* DT_STRTAB                                 */
    u64 nsyms;                  /* symbol count (from DT_HASH nchain, or GNU)*/

    Elf64_Rela *rela;    u64 relasz;      /* DT_RELA  / DT_RELASZ  (bytes)   */
    Elf64_Rela *jmprel;  u64 pltrelsz;    /* DT_JMPREL/ DT_PLTRELSZ(bytes)   */
    u64        *relr;    u64 relrsz;      /* DT_RELR  / DT_RELRSZ  (bytes)   */

    u32 *hash;                  /* DT_HASH     (Sys V hash: nbucket,nchain,…)*/
    u32 *gnu_hash;              /* DT_GNU_HASH (modern hash table)           */

    u64  init;                  /* DT_INIT function (base-adjusted) or 0     */
    u64 *init_array; u64 init_arraysz;    /* DT_INIT_ARRAY / …SZ (bytes)     */

    u64  relro_start, relro_size;         /* PT_GNU_RELRO region             */

    const char *runpath;        /* DT_RUNPATH/RPATH search dirs, or NULL     */

    u32 needed[MAX_NEEDED];     /* DT_NEEDED strtab offsets, resolved later  */
    int nneeded;

    int relocated;              /* guard against relocating twice            */
    int inited;                 /* guard against running init twice          */
};

static struct obj g_objs[MAX_OBJS];
static int        g_nobjs = 0;

/* Directory that the top-level program lives in — a default place to look for
 * its sibling shared objects, mirroring "the exe's own dir". */
static char g_prog_dir[PATH_MAX];
/* Optional colon-separated search path from the environment. */
static const char *g_env_libpath = NULL;

/* ===========================================================================
 * 5. Page math. The kernel maps at 4 KiB granularity, and ELF guarantees that
 *    for every PT_LOAD, (p_vaddr - p_offset) is a multiple of the page size —
 *    which is exactly what lets us map the file straight into memory.
 * ========================================================================= */
#define PAGE 4096UL
static u64 align_down(u64 v, u64 a) { return v & ~(a - 1); }
static u64 align_up(u64 v, u64 a)   { return (v + a - 1) & ~(a - 1); }

/* ===========================================================================
 * 6. Mapping an object's PT_LOAD segments.
 *
 * The reliable recipe (the same one glibc's _dl_map_segments uses):
 *   (a) compute the total virtual span [minva, maxva) across all PT_LOADs;
 *   (b) reserve that whole span with ONE anonymous PROT_NONE mmap, so the
 *       kernel hands us a single contiguous hole and picks the load address
 *       for a PIE. The load bias B is (reservation - minva);
 *   (c) map each segment on top of the reservation with MAP_FIXED, backed by
 *       the file, with the segment's real protections;
 *   (d) for a segment whose memsz > filesz (a .bss), zero the tail of the last
 *       file page (mmap already does this for a private file mapping) and map
 *       any additional whole pages anonymously.
 * ========================================================================= */
static u64 map_object(int fd, Elf64_Ehdr *eh, Elf64_Phdr *ph, struct obj *o)
{
    /* (a) span of all PT_LOAD segments in virtual-address space. */
    u64 minva = ~0UL, maxva = 0;
    for (u64 i = 0; i < eh->e_phnum; i++) {
        if (ph[i].p_type != PT_LOAD) continue;
        u64 lo = align_down(ph[i].p_vaddr, PAGE);
        u64 hi = align_up(ph[i].p_vaddr + ph[i].p_memsz, PAGE);
        if (lo < minva) minva = lo;
        if (hi > maxva) maxva = hi;
    }
    if (minva == ~0UL) die("no PT_LOAD segments");
    u64 span = maxva - minva;

    /* (b) reserve the whole range. For ET_DYN we pass addr=0 and let the
     * kernel choose; the load bias is (chosen - minva). For ET_EXEC we must
     * honour the fixed vaddrs, so we MAP_FIXED at minva and the bias is 0. */
    long fixed = (eh->e_type == ET_EXEC) ? MAP_FIXED : 0;
    long r = sys_mmap((eh->e_type == ET_EXEC) ? (long)minva : 0, span,
                      PROT_NONE, MAP_PRIVATE | MAP_ANONYMOUS | fixed, -1, 0);
    if (mmap_failed(r)) die2("reserve mmap failed", (u64)-r);
    u64 base = (eh->e_type == ET_EXEC) ? 0 : ((u64)r - minva);
    trace2("load bias", base);

    /* (c)/(d) map each PT_LOAD on top of the reservation. */
    for (u64 i = 0; i < eh->e_phnum; i++) {
        Elf64_Phdr *p = &ph[i];
        if (p->p_type != PT_LOAD) continue;

        int prot = 0;
        if (p->p_flags & PF_R) prot |= PROT_READ;
        if (p->p_flags & PF_W) prot |= PROT_WRITE;
        if (p->p_flags & PF_X) prot |= PROT_EXEC;

        u64 seg_start = align_down(base + p->p_vaddr, PAGE);
        u64 file_off  = align_down(p->p_offset, PAGE);
        /* End of the file-backed part, rounded up to a page. */
        u64 file_end  = align_up(base + p->p_vaddr + p->p_filesz, PAGE);

        if (file_end > seg_start) {
            r = sys_mmap((long)seg_start, file_end - seg_start, prot,
                         MAP_PRIVATE | MAP_FIXED, fd, file_off);
            if (mmap_failed(r)) die2("segment mmap failed", (u64)-r);
        }

        /* .bss: memory beyond the file contents must read as zero. The kernel
         * already zero-fills the slack of the last file page (private file
         * mapping semantics); we only need to add whole anonymous pages beyond
         * that. Those pages are demand-zero. */
        u64 mem_end = align_up(base + p->p_vaddr + p->p_memsz, PAGE);
        if (mem_end > file_end && (p->p_flags & PF_W)) {
            r = sys_mmap((long)file_end, mem_end - file_end, prot,
                         MAP_PRIVATE | MAP_FIXED | MAP_ANONYMOUS, -1, 0);
            if (mmap_failed(r)) die2("bss mmap failed", (u64)-r);
        }
    }

    /* Record where the program headers ended up in memory (needed for the
     * auxv AT_PHDR of the main program). They live inside whichever PT_LOAD
     * covers file offset e_phoff — usually the first one, which maps offset 0.*/
    o->phdr = NULL;
    for (u64 i = 0; i < eh->e_phnum; i++) {
        if (ph[i].p_type == PT_LOAD &&
            ph[i].p_offset <= eh->e_phoff &&
            eh->e_phoff < ph[i].p_offset + ph[i].p_filesz) {
            o->phdr = (Elf64_Phdr *)(base + ph[i].p_vaddr +
                                     (eh->e_phoff - ph[i].p_offset));
            break;
        }
    }
    o->phnum = eh->e_phnum;
    return base;
}

/* ===========================================================================
 * 7. Parsing PT_DYNAMIC into the link-map entry. Every address-valued tag is
 *    a vaddr and must be biased by `base` to become a usable pointer; size and
 *    count tags are taken verbatim.
 * ========================================================================= */
static void parse_dynamic(struct obj *o, Elf64_Dyn *dyn)
{
    u64 pltrel = DT_RELA;               /* DT_PLTREL: are JMPREL entries RELA?*/
    for (Elf64_Dyn *d = dyn; d->d_tag != DT_NULL; d++) {
        u64 v = d->d_val;
        switch (d->d_tag) {
        case DT_SYMTAB:   o->symtab   = (Elf64_Sym *)(o->base + v); break;
        case DT_STRTAB:   o->strtab   = (const char *)(o->base + v); break;
        case DT_RELA:     o->rela     = (Elf64_Rela *)(o->base + v); break;
        case DT_RELASZ:   o->relasz   = v; break;
        case DT_JMPREL:   o->jmprel   = (Elf64_Rela *)(o->base + v); break;
        case DT_PLTRELSZ: o->pltrelsz = v; break;
        case DT_PLTREL:   pltrel      = v; break;
        case DT_RELR:     o->relr     = (u64 *)(o->base + v); break;
        case DT_RELRSZ:   o->relrsz   = v; break;
        case DT_HASH:     o->hash     = (u32 *)(o->base + v); break;
        case DT_GNU_HASH: o->gnu_hash = (u32 *)(o->base + v); break;
        case DT_INIT:     o->init     = o->base + v; break;
        case DT_INIT_ARRAY:   o->init_array   = (u64 *)(o->base + v); break;
        case DT_INIT_ARRAYSZ: o->init_arraysz = v; break;
        case DT_SONAME:   o->soname   = (const char *)(u64)v; break; /* fix ↓ */
        case DT_RUNPATH:  /* fall through */
        case DT_RPATH:    o->runpath  = (const char *)(u64)v; break; /* fix ↓ */
        case DT_NEEDED:
            if (o->nneeded < MAX_NEEDED) o->needed[o->nneeded++] = (u32)v;
            else die("too many DT_NEEDED entries");
            break;
        default: break;                 /* ignore tags we don't model        */
        }
    }

    /* DT_PLTREL other than RELA (i.e. the old REL form) is not produced on
     * x86-64; refuse it rather than silently misinterpret the entries. */
    if (o->jmprel && pltrel != DT_RELA)
        die("PLT relocations are DT_REL, not DT_RELA (unsupported)");

    /* DT_SONAME / DT_RUNPATH were stashed as raw strtab *offsets* above because
     * DT_STRTAB may appear after them in the array. Now that strtab is known,
     * turn the offsets into pointers. */
    if (o->strtab) {
        if (o->soname)  o->soname  = o->strtab + (u64)o->soname;
        if (o->runpath) o->runpath = o->strtab + (u64)o->runpath;
    }

    /* Symbol count: the Sys V hash table stores nchain == number of symbols in
     * its second word. If only GNU_HASH is present we derive the count from it
     * (below). Without either we can still apply RELATIVE relocs but cannot
     * resolve named symbols. */
    if (o->hash)
        o->nsyms = o->hash[1];          /* hash[0]=nbucket, hash[1]=nchain    */
    else if (o->gnu_hash) {
        /* GNU hash layout: [nbuckets][symoffset][bloom_size][bloom_shift]
         * [bloom u64 * bloom_size][buckets u32 * nbuckets][chain u32 ...].
         * The highest symbol index is found by taking the largest bucket and
         * walking its chain to the terminator (low bit set). */
        u32 nbuckets = o->gnu_hash[0];
        u32 symoffset = o->gnu_hash[1];
        u32 bloom_size = o->gnu_hash[2];
        u32 *buckets = (u32 *)((u64 *)&o->gnu_hash[4] + bloom_size);
        u32 *chain = &buckets[nbuckets];
        u32 last = 0;
        for (u32 i = 0; i < nbuckets; i++)
            if (buckets[i] > last) last = buckets[i];
        if (last < symoffset) { o->nsyms = symoffset; }
        else {
            while (!(chain[last - symoffset] & 1)) last++;
            o->nsyms = last + 1;
        }
    }
}

/* ===========================================================================
 * 8. Symbol resolution across the global scope (all loaded objects, in load
 *    order — the default ELF search order). We look symbols up by name.
 *
 * A production loader hashes the name (Sys V or GNU hash) to jump straight to
 * the right bucket; we scan linearly, which is O(n) but unmistakably correct
 * and easy to read. The hash tables are still *used* to know how many symbols
 * exist. The GNU hash algorithm itself is spelled out in asm/demo-lite and the
 * README so you can see the optimisation we chose not to inline here.
 * ========================================================================= */
struct symdef { struct obj *obj; Elf64_Sym *sym; };

/* Search a single object's dynamic symbol table for a *defined* global/weak
 * symbol named `name`. Returns 1 and fills *out on success. */
static int lookup_in(struct obj *o, const char *name, struct symdef *out)
{
    if (!o->symtab || !o->strtab || o->nsyms == 0) return 0;
    for (u64 i = 0; i < o->nsyms; i++) {
        Elf64_Sym *s = &o->symtab[i];
        if (s->st_shndx == SHN_UNDEF) continue;         /* undefined here     */
        u64 bind = ELF64_ST_BIND(s->st_info);
        if (bind != STB_GLOBAL && bind != STB_WEAK) continue;  /* not exported*/
        if (!kstreq(o->strtab + s->st_name, name)) continue;
        out->obj = o; out->sym = s;
        return 1;
    }
    return 0;
}

/* Search every loaded object in load order; the first definition wins, which
 * is exactly the interposition rule (why LD_PRELOAD works: a preloaded object
 * appears earlier in this list and shadows later definitions). A strong global
 * definition beats a weak one, so we remember a weak hit but keep looking. */
static int global_lookup(const char *name, struct symdef *out)
{
    struct symdef weak = { NULL, NULL };
    for (int i = 0; i < g_nobjs; i++) {
        struct symdef d;
        if (lookup_in(&g_objs[i], name, &d)) {
            if (ELF64_ST_BIND(d.sym->st_info) == STB_GLOBAL) { *out = d; return 1; }
            if (!weak.sym) weak = d;    /* first weak seen; may be overridden */
        }
    }
    if (weak.sym) { *out = weak; return 1; }
    return 0;
}

/* Turn a resolved symbol into its final runtime address. For an IFUNC the
 * "address" is produced by *calling* the resolver the symbol points at (this is
 * how, e.g., memcpy picks an AVX vs SSE implementation at load time). */
static u64 symdef_addr(struct symdef *d)
{
    u64 a = d->obj->base + d->sym->st_value;
    if (ELF64_ST_TYPE(d->sym->st_info) == STT_GNU_IFUNC)
        a = ((u64 (*)(void))a)();
    return a;
}

/* ===========================================================================
 * 9. Applying relocations for one object. THIS is the heart of a dynamic
 *    linker: for each entry compute the target value and store it at
 *    P = base + r_offset. See asm/demo.c for this exact inner loop compiled to
 *    assembly and annotated instruction-by-instruction.
 * ========================================================================= */

/* Resolve the symbol referenced by relocation `r` in object `o`, returning its
 * value S (0 for an unresolved weak reference). `type` is only used to phrase
 * the "undefined symbol" error. */
static u64 resolve_reloc_symbol(struct obj *o, Elf64_Rela *r)
{
    u64 symidx = ELF64_R_SYM(r->r_info);
    Elf64_Sym *s = &o->symtab[symidx];
    const char *name = o->strtab + s->st_name;

    /* A LOCAL symbol is resolved within the defining object directly. */
    if (ELF64_ST_BIND(s->st_info) == STB_LOCAL && s->st_shndx != SHN_UNDEF)
        return o->base + s->st_value;

    struct symdef d;
    if (global_lookup(name, &d))
        return symdef_addr(&d);

    /* Not found. A weak undefined reference legitimately resolves to 0
     * (e.g. `if (__weak_fn) __weak_fn();`). A strong one is a hard error. */
    if (ELF64_ST_BIND(s->st_info) == STB_WEAK) return 0;
    dstr("loader: undefined symbol: "); dstr(name); dstr("\n");
    sys_exit(127);
}

/* Apply one RELA table (`rela`, `count` entries). Shared by DT_RELA and the
 * PLT's DT_JMPREL — they have identical layout and differ only in *when* a real
 * loader would bind them (eagerly vs lazily). We bind everything eagerly. */
static void apply_rela(struct obj *o, Elf64_Rela *rela, u64 count)
{
    for (u64 i = 0; i < count; i++) {
        Elf64_Rela *r = &rela[i];
        u64 type = ELF64_R_TYPE(r->r_info);
        u64 *P = (u64 *)(o->base + r->r_offset);   /* the slot to patch      */
        i64 A = r->r_addend;                       /* the explicit addend    */

        switch (type) {
        case R_X86_64_NONE:
            break;

        case R_X86_64_RELATIVE:
            /* B + A: no symbol; just rebase a pointer by the load bias. This is
             * the overwhelmingly common reloc in a PIE (every absolute address
             * baked into .data becomes one of these). */
            *P = o->base + (u64)A;
            break;

        case R_X86_64_64:
            /* S + A: a 64-bit absolute reference to a (possibly external)
             * symbol, e.g. a function-pointer table entry. */
            *P = resolve_reloc_symbol(o, r) + (u64)A;
            break;

        case R_X86_64_GLOB_DAT:
            /* S (+A, usually 0): fill a GOT slot for a *data* symbol so code
             * can load the address indirectly. */
            *P = resolve_reloc_symbol(o, r) + (u64)A;
            break;

        case R_X86_64_JUMP_SLOT:
            /* S (+A=0): fill a PLT/GOT slot for a *function*. With lazy binding
             * this slot would initially point back into the PLT stub and get
             * filled on first call; binding eagerly, we resolve it now. */
            *P = resolve_reloc_symbol(o, r) + (u64)A;
            break;

        case R_X86_64_IRELATIVE:
            /* B + A is the address of a resolver function; store what it
             * returns. Same idea as an IFUNC symbol but with no symbol name. */
            *P = ((u64 (*)(void))(o->base + (u64)A))();
            break;

        case R_X86_64_COPY: {
            /* Copy the *contents* of the external symbol into P (used when an
             * executable has a copy of a library's global variable in its own
             * .bss). Resolve, then memcpy st_size bytes. */
            u64 symidx = ELF64_R_SYM(r->r_info);
            const char *name = o->strtab + o->symtab[symidx].st_name;
            struct symdef d;
            if (!global_lookup(name, &d)) die("R_X86_64_COPY: undefined symbol");
            memcpy(P, (void *)(d.obj->base + d.sym->st_value), d.sym->st_size);
            break;
        }

        case R_X86_64_DTPMOD64:
        case R_X86_64_DTPOFF64:
        case R_X86_64_TPOFF64:
            /* Thread-Local Storage. Handling these correctly requires building
             * a TCB, allocating module TLS blocks, wiring the DTV, and pointing
             * %fs at it — a whole subsystem this teaching core omits. */
            die("TLS relocation encountered (unsupported in this teaching core)");
            break;

        default:
            die2("unhandled relocation type", type);
        }
    }
}

/* Apply the compact DT_RELR table: a bit-packed encoding of a run of
 * R_X86_64_RELATIVE relocations (glibc/lld emit this to shrink PIE binaries).
 * Each entry is either an even address (a new location L; apply *L += base and
 * advance) or an odd bitmap whose bits 1..63 mark which of the next 63 words,
 * starting at the current cursor, also need *word += base. */
static void apply_relr(struct obj *o)
{
    if (!o->relr) return;
    u64 n = o->relrsz / sizeof(u64);
    u64 *where = NULL;
    for (u64 i = 0; i < n; i++) {
        u64 e = o->relr[i];
        if ((e & 1) == 0) {                     /* an address (even) */
            where = (u64 *)(o->base + e);
            *where += o->base;
            where++;
        } else {                                /* a bitmap (odd) */
            u64 *w = where;
            for (u64 bits = e >> 1; bits; bits >>= 1, w++)
                if (bits & 1) *w += o->base;
            where += 63;                        /* 63 data words per bitmap */
        }
    }
}

/* Relocate one object completely: RELR, then DT_RELA, then the PLT (DT_JMPREL).
 * Symbol resolution consults the *global* scope, so this must run only after
 * every object has been mapped. */
static void relocate_object(struct obj *o)
{
    if (o->relocated) return;
    o->relocated = 1;
    trace("relocating");
    apply_relr(o);
    if (o->rela)   apply_rela(o, o->rela,   o->relasz  / sizeof(Elf64_Rela));
    if (o->jmprel) apply_rela(o, o->jmprel, o->pltrelsz / sizeof(Elf64_Rela));
}

/* After relocating, harden PT_GNU_RELRO: the linker groups the GOT and other
 * "written once at startup" data into a page-aligned region and asks that it be
 * made read-only *after* relocation, so a later bug can't overwrite a GOT slot
 * to hijack control flow. This is the runtime half of "RELRO / BIND_NOW". */
static void apply_relro(struct obj *o)
{
    if (o->relro_size == 0) return;
    u64 start = align_down(o->base + o->relro_start, PAGE);
    u64 end   = align_up(o->base + o->relro_start + o->relro_size, PAGE);
    if (end > start) {
        long r = sys_mprotect((long)start, end - start, PROT_READ);
        if (mmap_failed(r)) die2("relro mprotect failed", (u64)-r);
        trace("applied RELRO (GOT now read-only)");
    }
}

/* ===========================================================================
 * 10. Loading one object from a file descriptor: read + validate the ELF
 *     header, read the program headers, map the segments, capture PT_DYNAMIC
 *     and PT_GNU_RELRO. Returns a pointer to the new link-map entry.
 * ========================================================================= */
static struct obj *load_from_fd(int fd, const char *name)
{
    if (g_nobjs >= MAX_OBJS) die("too many shared objects");
    struct obj *o = &g_objs[g_nobjs++];
    /* Zero the fresh slot (bump-allocated in BSS; no malloc anywhere). */
    memset(o, 0, sizeof(*o));
    o->name = name;

    /* Read and validate the 64-byte ELF header via pread (no seeking). */
    Elf64_Ehdr eh;
    long got = sys_pread(fd, &eh, sizeof(eh), 0);
    if (got != (long)sizeof(eh)) die("short read of ELF header");
    if (eh.e_ident[0] != 0x7f || eh.e_ident[1] != 'E' ||
        eh.e_ident[2] != 'L'  || eh.e_ident[3] != 'F')
        die("not an ELF file (bad magic)");
    if (eh.e_ident[4] != 2)    die("not ELFCLASS64");        /* EI_CLASS      */
    if (eh.e_ident[5] != 1)    die("not little-endian");     /* EI_DATA       */
    if (eh.e_machine != EM_X86_64) die("not x86-64");
    if (eh.e_type != ET_DYN && eh.e_type != ET_EXEC) die("not ET_DYN/ET_EXEC");
    if (eh.e_phentsize != sizeof(Elf64_Phdr)) die("unexpected e_phentsize");
    if (eh.e_phnum == 0 || eh.e_phnum > 256) die("implausible e_phnum");

    /* Read the program header table into a fixed stack buffer. */
    static Elf64_Phdr ph[256];      /* static: keeps the frame small          */
    u64 phsz = (u64)eh.e_phnum * sizeof(Elf64_Phdr);
    got = sys_pread(fd, ph, phsz, eh.e_phoff);
    if (got != (long)phsz) die("short read of program headers");

    /* Map the loadable segments; this also computes the load bias. */
    o->base = map_object(fd, &eh, ph, o);
    o->entry = o->base + eh.e_entry;

    /* Locate PT_DYNAMIC and PT_GNU_RELRO among the (now in-memory) headers. */
    Elf64_Dyn *dyn = NULL;
    for (u64 i = 0; i < eh.e_phnum; i++) {
        if (ph[i].p_type == PT_DYNAMIC)
            dyn = (Elf64_Dyn *)(o->base + ph[i].p_vaddr);
        else if (ph[i].p_type == PT_GNU_RELRO) {
            o->relro_start = ph[i].p_vaddr;
            o->relro_size  = ph[i].p_memsz;
        } else if (ph[i].p_type == PT_TLS && ph[i].p_memsz)
            /* Warn but continue: the exe may declare TLS yet, in our simple
             * tests, never actually touch it. A TLS *relocation* is the real
             * blocker and is caught in apply_rela(). */
            trace("note: PT_TLS present (TLS is not set up by this loader)");
    }
    if (dyn) parse_dynamic(o, dyn);
    return o;
}

/* Open a file by path and load it, or die. */
static struct obj *load_path(const char *path)
{
    trace(path);
    long fd = sys_openat(path);
    if (fd < 0) die2("cannot open object", (u64)-fd);
    struct obj *o = load_from_fd((int)fd, path);
    sys_close((int)fd);
    return o;
}

/* ===========================================================================
 * 11. DT_NEEDED resolution: find and load a dependency by soname.
 *     Search order (a small, teaching-sized subset of the real algorithm):
 *       1. the importer's DT_RUNPATH/DT_RPATH (colon-separated),
 *       2. $LDLAB_LIBRARY_PATH from the environment,
 *       3. the directory the main program lives in,
 *       4. the current directory.
 *     We do NOT search /lib, /usr/lib, or ld.so.cache: this loader is for the
 *     self-contained objects in test/, not the system's glibc.
 * ========================================================================= */

/* Is a library with this soname/name already loaded? (Avoid double-loading a
 * diamond dependency.) Compare against each object's soname and its load name.*/
static struct obj *already_loaded(const char *name)
{
    for (int i = 0; i < g_nobjs; i++) {
        if (g_objs[i].soname && kstreq(g_objs[i].soname, name)) return &g_objs[i];
        if (g_objs[i].name && kstreq(g_objs[i].name, name))     return &g_objs[i];
    }
    return NULL;
}

/* Try "dir/name"; on success load it and return the new object. `dir` may be a
 * slice of a colon-separated list, so `dirlen` bounds it. */
static struct obj *try_dir(const char *dir, u64 dirlen, const char *name)
{
    char path[PATH_MAX];
    u64 n = 0;
    for (u64 i = 0; i < dirlen && n + 1 < PATH_MAX; i++) path[n++] = dir[i];
    if (n && path[n - 1] != '/' && n + 1 < PATH_MAX) path[n++] = '/';
    n += kstrcpy(path + n, PATH_MAX - n, name);
    path[n] = 0;

    long fd = sys_openat(path);
    if (fd < 0) return NULL;
    /* Copy the resolved path into a persistent buffer so the link map keeps a
     * valid name pointer. We stash it in a small static pool. */
    static char names[MAX_OBJS][PATH_MAX];
    static int  nnames = 0;
    const char *stored = name;
    if (nnames < MAX_OBJS) { kstrcpy(names[nnames], PATH_MAX, path); stored = names[nnames++]; }
    struct obj *o = load_from_fd((int)fd, stored);
    sys_close((int)fd);
    return o;
}

/* Walk a colon-separated path list, trying each directory. */
static struct obj *try_pathlist(const char *list, const char *name)
{
    if (!list) return NULL;
    const char *p = list;
    while (*p) {
        const char *start = p;
        while (*p && *p != ':') p++;
        struct obj *o = try_dir(start, (u64)(p - start), name);
        if (o) return o;
        if (*p == ':') p++;
    }
    return NULL;
}

static struct obj *load_needed(struct obj *importer, const char *name)
{
    struct obj *o = already_loaded(name);
    if (o) return o;

    /* If the name contains a slash it is a path; use it directly. */
    if (klast_slash(name) >= 0) return load_path(name);

    if ((o = try_pathlist(importer->runpath, name))) return o;
    if ((o = try_pathlist(g_env_libpath, name)))     return o;
    if (g_prog_dir[0] && (o = try_dir(g_prog_dir, kstrlen(g_prog_dir), name)))
        return o;
    if ((o = try_dir(".", 1, name))) return o;

    dstr("loader: cannot find shared object: "); dstr(name); dstr("\n");
    sys_exit(127);
}

/* ===========================================================================
 * 12. Running constructors: DT_INIT then each pointer in DT_INIT_ARRAY. glibc
 *     calls these as `void (*)(int argc, char **argv, char **envp)`, so we pass
 *     the program's argc/argv/envp too (harmless to a zero-arg constructor).
 *
 * NOTE (honesty): a real ld.so runs the *libraries'* constructors and leaves
 * the main executable's init to __libc_start_main. Our test programs are
 * freestanding and have no libc to do that, so this loader runs constructors
 * for *every* object, dependencies first, then the executable last.
 * ========================================================================= */
typedef void (*init_fn)(int, char **, char **);

static void run_init(struct obj *o, int argc, char **argv, char **envp)
{
    if (o->inited) return;
    o->inited = 1;
    if (o->init) {
        trace2("DT_INIT", o->init);
        ((init_fn)o->init)(argc, argv, envp);
    }
    u64 count = o->init_arraysz / sizeof(u64);
    for (u64 i = 0; i < count; i++) {
        init_fn f = (init_fn)o->init_array[i];
        if (f && (u64)f != ~0UL) f(argc, argv, envp);   /* skip 0/-1 fillers */
    }
}

/* ===========================================================================
 * 13. The hand-off: build a fresh initial stack and jump to the entry point.
 *
 * The kernel, on execve, hands a process a stack shaped exactly like this
 * (low address first — this is what rsp points at when _start runs):
 *
 *     [ argc ][ argv[0] .. argv[argc-1] ][ NULL ]
 *            [ envp[0] .. envp[n-1]     ][ NULL ]
 *            [ auxv: (type,val) pairs   ][ AT_NULL,0 ]
 *
 * We were exec'd as "loader prog args...", so we rebuild this block for `prog`:
 * drop our own argv[0], keep the rest of argv, inherit envp, and patch the
 * auxv (AT_PHDR/AT_PHENT/AT_PHNUM/AT_ENTRY/AT_BASE/AT_EXECFN) to describe
 * `prog`, not the loader. The string bytes themselves stay where the kernel put
 * them on the loader's original stack (still mapped), so we only copy pointers.
 * ========================================================================= */

/* The final register/stack transition, in inline asm because it cannot touch
 * the C stack after switching rsp. Contract at a fresh process entry:
 *   - rsp -> the argc we built (and rsp % 16 == 0),
 *   - rdx == 0 tells a glibc _start "there is no rtld_fini to register",
 *   - rbp == 0 marks the outermost frame for debuggers.
 * We force entry into rdi and the new sp into rsi so both are already in
 * registers before we clobber rsp — nothing reads the old stack afterwards. */
__attribute__((noreturn))
static void enter_program(u64 entry, u64 *sp)
{
    __asm__ volatile(
        "movq %%rsi, %%rsp\n\t"  /* switch to the program's stack            */
        "xorl %%edx, %%edx\n\t"  /* rdx = 0: no rtld_fini                     */
        "xorl %%ebp, %%ebp\n\t"  /* rbp = 0: end of the frame chain           */
        "jmp  *%%rdi\n\t"        /* jump to the entry point; never returns    */
        : : "D"(entry), "S"(sp) : "memory");
    __builtin_unreachable();
}

/* Count NULL-terminated pointer arrays / auxv. */
static int count_ptrs(char **v) { int n = 0; while (v[n]) n++; return n; }

__attribute__((noreturn))
static void handoff(struct obj *exe, int argc, char **argv, char **envp,
                    Elf64_auxv_t *auxv)
{
    /* The program's view: its name is argv[1] of the loader, so shift by one.*/
    int prog_argc = argc - 1;
    char **prog_argv = argv + 1;
    int nenv = count_ptrs(envp);
    int naux = 0;
    while (auxv[naux].a_type != AT_NULL) naux++;

    /* One 64 KiB anonymous stack for the child block. Real stacks auto-grow;
     * for our small programs a fixed page range is plenty and keeps the code
     * honest about *where* the initial data structure lives. */
    u64 stksz = 256 * 1024;
    long m = sys_mmap(0, stksz, PROT_READ | PROT_WRITE,
                      MAP_PRIVATE | MAP_ANONYMOUS, -1, 0);
    if (mmap_failed(m)) die2("stack mmap failed", (u64)-m);
    u64 top = (u64)m + stksz;

    /* Total 8-byte slots: argc + argv + NULL + envp + NULL + auxv + AT_NULL. */
    u64 slots = 1 + (u64)(prog_argc + 1) + (u64)(nenv + 1) + (u64)(naux + 1) * 2;
    u64 base = align_down(top - slots * 8, 16);   /* argc must be 16-aligned  */
    u64 *p = (u64 *)base;

    *p++ = (u64)prog_argc;
    for (int i = 0; i < prog_argc; i++) *p++ = (u64)prog_argv[i];
    *p++ = 0;                                     /* argv terminator          */
    for (int i = 0; i < nenv; i++)      *p++ = (u64)envp[i];
    *p++ = 0;                                     /* envp terminator          */

    /* Copy the auxv, overriding the entries that describe the executable. */
    for (int i = 0; i < naux; i++) {
        u64 t = auxv[i].a_type, v = auxv[i].a_val;
        switch (t) {
        case AT_PHDR:   v = (u64)exe->phdr; break;
        case AT_PHENT:  v = sizeof(Elf64_Phdr); break;
        case AT_PHNUM:  v = exe->phnum; break;
        case AT_ENTRY:  v = exe->entry; break;
        case AT_BASE:   v = 0; break;   /* interpreter base = us; irrelevant  */
        case AT_EXECFN: v = (u64)prog_argv[0]; break;
        case AT_PAGESZ: v = PAGE; break;
        default: break;                 /* forward AT_RANDOM, AT_SYSINFO_EHDR,
                                         * AT_HWCAP, AT_CLKTCK, ... unchanged  */
        }
        *p++ = t; *p++ = v;
    }
    *p++ = AT_NULL; *p++ = 0;

    trace2("entry", exe->entry);
    trace2("new sp", base);
    enter_program(exe->entry, (u64 *)base);
}

/* ===========================================================================
 * 14. loader_main — the C entry point, called from _start with the raw initial
 *     stack pointer. From it we recover argc/argv/envp/auxv the same way any
 *     _start does.
 * ========================================================================= */
__attribute__((noreturn))
void loader_main(u64 *sp)
{
    long argc = (long)sp[0];
    char **argv = (char **)&sp[1];
    char **envp = argv + argc + 1;

    /* Find the auxv: it follows the envp NULL terminator. */
    char **e = envp;
    while (*e) e++;
    Elf64_auxv_t *auxv = (Elf64_auxv_t *)(e + 1);

    /* Read a couple of environment variables ourselves (no getenv). */
    for (char **v = envp; *v; v++) {
        const char *s = *v;
        if (s[0]=='L'&&s[1]=='D'&&s[2]=='L'&&s[3]=='A'&&s[4]=='B'&&s[5]=='_') {
            if (kstreq(s, "LDLAB_DEBUG=1")) g_debug = 1;
            /* LDLAB_LIBRARY_PATH=... : remember the value after '='. */
            const char *k = "LDLAB_LIBRARY_PATH=";
            const char *a = s; const char *b = k;
            while (*b && *a == *b) { a++; b++; }
            if (*b == 0) g_env_libpath = a;
        }
    }

    if (argc < 2)
        die("usage: loader ./program [args...]   (set LDLAB_DEBUG=1 to trace)");

    const char *prog = argv[1];

    /* Remember the program's directory so we can find its sibling libraries. */
    long slash = klast_slash(prog);
    if (slash > 0) {
        u64 n = (u64)slash < PATH_MAX - 1 ? (u64)slash : PATH_MAX - 1;
        for (u64 i = 0; i < n; i++) g_prog_dir[i] = prog[i];
        g_prog_dir[n] = 0;
    }

    trace("loading main program");
    struct obj *exe = load_path(prog);

    /* Breadth-first: load every DT_NEEDED, appending to the link map. Because
     * new objects are appended to g_objs, this single loop naturally processes
     * transitively-needed libraries too. */
    for (int i = 0; i < g_nobjs; i++) {
        struct obj *o = &g_objs[i];
        for (int k = 0; k < o->nneeded; k++) {
            const char *nm = o->strtab + o->needed[k];
            if (g_debug) { dstr("[ld] DT_NEEDED "); dstr(nm); dstr("\n"); }
            load_needed(o, nm);
        }
    }

    /* Relocate every object now that all are mapped (so cross-object symbols
     * resolve), then lock down each RELRO region. */
    for (int i = 0; i < g_nobjs; i++) relocate_object(&g_objs[i]);
    for (int i = 0; i < g_nobjs; i++) apply_relro(&g_objs[i]);

    /* Run constructors dependencies-first: objects were appended in dependency
     * order (exe, then its needs), so we init from the last one back to the
     * first. */
    for (int i = g_nobjs - 1; i >= 0; i--)
        run_init(&g_objs[i], argc - 1, argv + 1, envp);

    /* Build the child stack and jump in. Control does not return. */
    handoff(exe, (int)argc, argv, envp, auxv);
}

/* ===========================================================================
 * 15. _start — the process entry point the kernel jumps to after execve. There
 *     is no libc and no `main` convention here; we grab the stack pointer (which
 *     points at argc) and hand it to loader_main. We align rsp to 16 first
 *     because loader_main makes `call`s and the ABI requires rsp % 16 == 0 at a
 *     call boundary. Written as module-level asm so no prologue mangles rsp.
 * ========================================================================= */
__asm__(
    ".text\n"
    ".globl _start\n"
    ".type _start,@function\n"
    "_start:\n"
    "   xorl  %ebp, %ebp\n"       /* clear frame pointer: bottom of the chain */
    "   movq  %rsp, %rdi\n"       /* arg0 = the raw stack pointer (&argc)      */
    "   andq  $-16, %rsp\n"       /* 16-byte align rsp for the upcoming call   */
    "   callq loader_main\n"      /* never returns                             */
    "   hlt\n"                    /* trap if it somehow does                   */
);
