/* ===========================================================================
 * main.c — the CLI: run a .lum file, start a REPL, or fire an add-on demo.
 * ===========================================================================
 *
 *   lumen                 -> interactive REPL
 *   lumen path.lum        -> compile and run a program file
 *   lumen --jit-demo [n]  -> JIT-compile sum(1..n) to native x86-64 and run it
 *   lumen --coro-demo     -> run two cooperative green threads
 *
 * Exit codes follow the sysexits(3) convention used by the sibling VM: 65 for a
 * compile error, 70 for a runtime error, 74 for I/O, 64 for bad usage.
 */
#include <stdint.h>
#include <stdio.h>
#include <stdlib.h>
#include <string.h>

#include "jit.h"
#include "sched.h"
#include "vm.h"

/* Slurp an entire file into a NUL-terminated heap buffer (caller frees). */
static char *readFile(const char *path)
{
    FILE *file = fopen(path, "rb");
    if (file == NULL) {
        fprintf(stderr, "lumen: could not open '%s'\n", path);
        exit(74);
    }
    fseek(file, 0L, SEEK_END);
    long size = ftell(file);
    rewind(file);
    if (size < 0) {
        fprintf(stderr, "lumen: could not determine size of '%s'\n", path);
        fclose(file);
        exit(74);
    }

    char *buffer = (char *)malloc((size_t)size + 1);
    if (buffer == NULL) {
        fprintf(stderr, "lumen: not enough memory to read '%s'\n", path);
        fclose(file);
        exit(74);
    }
    size_t bytes = fread(buffer, 1, (size_t)size, file);
    if (bytes < (size_t)size) {
        fprintf(stderr, "lumen: could not read '%s'\n", path);
        free(buffer);
        fclose(file);
        exit(74);
    }
    buffer[bytes] = '\0';
    fclose(file);
    return buffer;
}

static void runFile(const char *path)
{
    char           *source = readFile(path);
    InterpretResult result = interpret(source);
    free(source);
    if (result == INTERPRET_COMPILE_ERROR) exit(65);
    if (result == INTERPRET_RUNTIME_ERROR) exit(70);
}

/* A line-at-a-time REPL. Each line is compiled and run independently. */
static void repl(void)
{
    char line[1024];
    printf("lumen REPL — type an expression statement or declaration; Ctrl-D to exit\n");
    for (;;) {
        printf("> ");
        if (!fgets(line, sizeof(line), stdin)) { printf("\n"); break; }
        interpret(line);
    }
}

int main(int argc, char *argv[])
{
    /* Add-on demos don't need the VM; handle them before initVM(). */
    if (argc >= 2 && strcmp(argv[1], "--jit-demo") == 0) {
        int64_t n = (argc >= 3) ? (int64_t)strtoll(argv[2], NULL, 10) : 100;
        return jitDemo(n);
    }
    if (argc >= 2 && strcmp(argv[1], "--coro-demo") == 0) {
        return coroDemo();
    }

    initVM();
    if (argc == 1) {
        repl();
    } else if (argc == 2) {
        runFile(argv[1]);
    } else {
        fprintf(stderr, "usage: lumen [path.lum | --jit-demo [n] | --coro-demo]\n");
        freeVM();
        exit(64);
    }
    freeVM();
    return 0;
}
