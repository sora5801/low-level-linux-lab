/* ===========================================================================
 * examples/start.c — the other half of the demo: the program's entry point.
 * ===========================================================================
 *
 * It defines `_start` (the ELF entry, where the kernel jumps after execve) and
 * references three symbols DEFINED IN msg.c. None of their addresses are known
 * when this file is compiled, so the compiler leaves relocations behind:
 *
 *   print_hello()   -> R_X86_64_PLT32  (a direct cross-object call)
 *   minild_hook     -> R_X86_64_32S    (absolute address of the pointer var)
 *   sys_exit()      -> R_X86_64_PLT32  (another cross-object call)
 *
 * minild resolves each undefined reference to msg.c's definition and patches
 * the bytes. The result is a single, self-contained static executable.
 *
 * Compile (Linux ABI, no PIC):
 *   clang --target=x86_64-pc-linux-gnu -ffreestanding -fno-pic \
 *         -fno-stack-protector -c examples/start.c -o start.o
 * ===========================================================================
 */

/* These live in msg.c — undefined here, so each use emits a relocation. */
extern void print_hello(void);
extern void sys_exit(int code) __attribute__((noreturn));
extern void (*minild_hook)(void);

/* _start — GLOBAL and the entry point. minild sets e_entry to its address.
 * There is no caller and no return address: we must leave via a syscall, which
 * sys_exit does. */
__attribute__((noreturn))
void _start(void) {
    print_hello();     /* direct cross-object call (PLT32 -> bound direct)    */
    minild_hook();     /* load the relocated pointer (32S) and call through it */
    sys_exit(0);       /* cross-object call; exit_group(0) -> $? == 0         */
}
