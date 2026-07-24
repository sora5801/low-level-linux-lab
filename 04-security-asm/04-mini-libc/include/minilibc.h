/* ===========================================================================
 * minilibc.h — the entire public surface of our tiny, from-scratch libc.
 * ===========================================================================
 *
 * This ONE header is what a program links against when you build it with
 * `-nostdlib` and our `libminilibc.a`. There is no <stdio.h>, no <string.h>,
 * no <unistd.h> underneath — everything a hosted C program takes for granted
 * (types, varargs, printf, malloc, the errno variable, argv/envp) is declared
 * here and implemented in ../src by *us*, on top of raw Linux syscalls.
 *
 * musl is the reference to read: every routine here has a fuller, meaner
 * cousin in musl's src/ tree, and the comments point you at it.
 *
 * PLATFORM: x86-64 Linux, LP64 (long and pointers are 64-bit). The syscall
 * numbers, O_* flags, and errno values below are the x86-64 Linux ABI — they
 * are stable kernel contract, which is exactly why we can hardcode them.
 * ===========================================================================
 */
#ifndef MINILIBC_H
#define MINILIBC_H

/* ---------------------------------------------------------------------------
 * 1. Fundamental types.
 *
 * A hosted toolchain gets these from <stddef.h>/<sys/types.h>. We are
 * freestanding, so we spell them out. On x86-64 Linux (LP64):
 *   - `unsigned long` and every pointer are 64-bit  -> size_t / uintptr_t
 *   - `long` is 64-bit                              -> ssize_t / ptrdiff_t / off_t
 * A syscall return, a pointer, and a byte count therefore all fit in a long
 * with no truncation — the reason kernel-facing code uses long everywhere.
 * --------------------------------------------------------------------------- */
typedef unsigned long  size_t;    /* count of bytes; result of sizeof         */
typedef long           ssize_t;   /* signed size: read/write return, -1 on err */
typedef long           off_t;     /* file offset (bytes)                        */
typedef unsigned long  uintptr_t; /* an integer wide enough to hold a pointer   */
typedef long           intptr_t;

#ifndef NULL
#define NULL ((void *)0)
#endif

/* ---------------------------------------------------------------------------
 * 2. Variadic-argument plumbing (what <stdarg.h> normally hands you).
 *
 * We do NOT reimplement the calling convention by hand — that is genuinely
 * compiler-internal knowledge (where the 7th integer arg and the vector regs
 * spill on the SysV ABI). Instead we expose the compiler's own builtins, which
 * every C compiler provides even in freestanding mode. printf() uses these.
 * --------------------------------------------------------------------------- */
typedef __builtin_va_list va_list;
#define va_start(ap, last) __builtin_va_start(ap, last)
#define va_arg(ap, type)   __builtin_va_arg(ap, type)
#define va_end(ap)         __builtin_va_end(ap)
#define va_copy(d, s)      __builtin_va_copy(d, s)

/* ---------------------------------------------------------------------------
 * 3. Linux x86-64 syscall numbers (subset we wrap). From the kernel's
 *    arch/x86/entry/syscalls/syscall_64.tbl — stable ABI, safe to hardcode.
 * --------------------------------------------------------------------------- */
#define SYS_read         0
#define SYS_write        1
#define SYS_open         2
#define SYS_close        3
#define SYS_mmap         9
#define SYS_munmap      11
#define SYS_brk         12
#define SYS_exit        60   /* exits ONE thread                               */
#define SYS_exit_group 231   /* exits the whole thread group (the process)     */

/* ---------------------------------------------------------------------------
 * 4. Flags & constants used by the wrappers (x86-64 Linux values).
 * --------------------------------------------------------------------------- */
/* open(2) flags. Note these are OCTAL in the kernel headers; we keep octal. */
#define O_RDONLY   00
#define O_WRONLY   01
#define O_RDWR     02
#define O_CREAT   0100
#define O_TRUNC  01000
#define O_APPEND 02000

/* mmap(2) protection bits and mapping flags. */
#define PROT_NONE   0x0
#define PROT_READ   0x1
#define PROT_WRITE  0x2
#define PROT_EXEC   0x4
#define MAP_SHARED    0x01
#define MAP_PRIVATE   0x02
#define MAP_FIXED     0x10
#define MAP_ANONYMOUS 0x20
#define MAP_FAILED  ((void *)-1)   /* mmap's error sentinel (not NULL!)        */

/* ---------------------------------------------------------------------------
 * 5. errno — the classic "last error" channel.
 *
 * The kernel returns errors as a NEGATIVE return value in rax (in the range
 * [-4095, -1]); libc's job is to translate that into the (-1, errno) contract
 * C programmers expect. In a REAL libc `errno` is thread-local (each thread
 * needs its own, or two threads racing on one syscall would clobber it). We
 * use a plain global for clarity and note the TLS gap in the README.
 * --------------------------------------------------------------------------- */
extern int errno;

/* Errno value subset we reference by name (asm-generic/errno-base.h). */
#define EPERM    1
#define ENOENT   2
#define EINTR    4   /* syscall interrupted by a signal — caller should retry  */
#define EIO      5
#define EBADF    9
#define EAGAIN  11
#define ENOMEM  12   /* out of memory — malloc's failure path                  */
#define EFAULT  14   /* bad address passed to the kernel                        */
#define EINVAL  22
#define EMFILE  24

/* ---------------------------------------------------------------------------
 * 6. Syscall wrappers (../src/syscall.c). Each sets errno and returns -1 (or
 *    MAP_FAILED) on error, matching POSIX so real code ports unchanged.
 * --------------------------------------------------------------------------- */
ssize_t read(int fd, void *buf, size_t count);
ssize_t write(int fd, const void *buf, size_t count);
int     open(const char *path, int flags, int mode);
int     close(int fd);
void   *mmap(void *addr, size_t len, int prot, int flags, int fd, off_t off);
int     munmap(void *addr, size_t len);

/* Raw brk(2): returns the resulting program break, or the *old* break on
 * failure (kernel semantics, no errno). sbrk() is the friendlier delta form
 * malloc actually uses; it returns (void*)-1 and sets ENOMEM on failure. */
void   *sys_brk(void *addr);
void   *sbrk(intptr_t increment);

/* Process exit. exit_group terminates every thread; _exit just the caller. */
__attribute__((noreturn)) void _exit(int status);
__attribute__((noreturn)) void exit(int status);

/* Loop over write(2) until the whole buffer is out or an error sticks; the
 * kernel may satisfy a write partially, so a single write() is never enough. */
ssize_t write_all(int fd, const void *buf, size_t count);

/* ---------------------------------------------------------------------------
 * 7. Program-startup state, published by __libc_start_main (../src/crt.c).
 * --------------------------------------------------------------------------- */
extern char **environ;   /* the envp array, NULL-terminated                    */

/* getauxval — read one value out of the ELF auxiliary vector the kernel left
 * on the stack. This is how a real libc discovers the page size, and where it
 * finds AT_RANDOM: 16 kernel-provided random bytes used to seed the stack
 * canary. That link (auxv -> canary) is the security lesson of this project. */
unsigned long getauxval(unsigned long type);

/* auxv entry types (elf.h). a_type == AT_NULL (0) terminates the array. */
#define AT_NULL    0
#define AT_PHDR    3    /* address of the program headers                      */
#define AT_PAGESZ  6    /* system page size (usually 4096)                     */
#define AT_ENTRY   9    /* the ELF entry point                                 */
#define AT_UID    11    /* real user id                                        */
#define AT_EUID   12    /* effective user id                                   */
#define AT_GID    13
#define AT_EGID   14
#define AT_SECURE 23    /* 1 if setuid/setgid — libc hardens itself if so      */
#define AT_RANDOM 25    /* ptr to 16 random bytes: the stack-canary seed       */

/* ---------------------------------------------------------------------------
 * 8. <string.h> (../src/string.c) — the byte-pushing primitives everything
 *    else is built on. memcpy/memset are also what the compiler *itself*
 *    emits calls to for struct copies and array zeroing, so we must provide
 *    them even if a program never names them.
 * --------------------------------------------------------------------------- */
size_t strlen(const char *s);
char  *strcpy(char *dst, const char *src);
int    strcmp(const char *a, const char *b);
void  *memcpy(void *dst, const void *src, size_t n);   /* src/dst MUST NOT overlap */
void  *memset(void *dst, int c, size_t n);

/* ---------------------------------------------------------------------------
 * 9. Heap (../src/malloc.c) — a first-fit free list grown with sbrk/brk.
 * --------------------------------------------------------------------------- */
void  *malloc(size_t size);
void   free(void *ptr);
void  *calloc(size_t nmemb, size_t size);   /* zeroed; overflow-checked         */
void  *realloc(void *ptr, size_t size);

/* ---------------------------------------------------------------------------
 * 10. Formatted output (../src/printf.c) — varargs, buffered, one write(2).
 *     Supports %d %i %u %x %X %c %s %p %% and an 'l' length modifier.
 * --------------------------------------------------------------------------- */
int printf(const char *fmt, ...)                __attribute__((format(printf, 1, 2)));
int dprintf(int fd, const char *fmt, ...)       __attribute__((format(printf, 2, 3)));
int vdprintf(int fd, const char *fmt, va_list ap);

#endif /* MINILIBC_H */
