/* ===========================================================================
 * main.c — the command-line driver: a REPL and a file runner.
 * ===========================================================================
 *
 *   ./lvm              -> interactive REPL (read a line, interpret, repeat)
 *   ./lvm script.lox   -> compile and run a whole file
 *
 * All the interesting machinery lives elsewhere; this file only wires stdin/a
 * file to interpret() and maps the InterpretResult to a process exit code using
 * the BSD sysexits.h conventions (65 = data/format error, 70 = internal
 * software error), which is what test harnesses key off of.
 * ===========================================================================
 */
#include <stdio.h>
#include <stdlib.h>
#include <string.h>

#include "common.h"
#include "vm.h"

/* Interactive loop. Each line is compiled and run as its own program, so state
 * that must persist across lines (a `var`) has to be a GLOBAL — a local dies
 * with the line's function. That is the standard REPL limitation and a fine
 * demonstration of the global/local split. */
static void repl(void)
{
    char line[1024];
    for (;;) {
        printf("> ");
        /* fgets returns NULL on EOF (Ctrl-D / Ctrl-Z), which ends the REPL. */
        if (!fgets(line, sizeof(line), stdin)) {
            printf("\n");
            break;
        }
        interpret(line);
    }
}

/* ---------------------------------------------------------------------------
 * readFile — slurp an entire file into a heap buffer (NUL-terminated so the
 * scanner can treat it as a C string). We seek to the end to size the buffer in
 * one allocation rather than growing incrementally. Every failure path exits
 * with code 74 (I/O error) after a diagnostic — a CLI, unlike the VM core, can
 * afford to just die on a bad file.
 * --------------------------------------------------------------------------- */
static char *readFile(const char *path)
{
    FILE *file = fopen(path, "rb");
    if (file == NULL) {
        fprintf(stderr, "Could not open file \"%s\".\n", path);
        exit(74);
    }

    fseek(file, 0L, SEEK_END);
    long fileSize = ftell(file);      /* byte length of the file                 */
    rewind(file);

    char *buffer = (char *)malloc((size_t)fileSize + 1);   /* +1 for the NUL     */
    if (buffer == NULL) {
        fprintf(stderr, "Not enough memory to read \"%s\".\n", path);
        exit(74);
    }

    size_t bytesRead = fread(buffer, sizeof(char), (size_t)fileSize, file);
    if (bytesRead < (size_t)fileSize) {
        fprintf(stderr, "Could not read file \"%s\".\n", path);
        exit(74);
    }
    buffer[bytesRead] = '\0';         /* terminate so the lexer knows where to stop */

    fclose(file);
    return buffer;
}

static void runFile(const char *path)
{
    char           *source = readFile(path);
    InterpretResult result = interpret(source);
    free(source);   /* the compiler copied everything it needed into GC objects  */

    /* Map interpreter status to sysexits.h codes so `make test` and CI can tell
     * a compile error (65, EX_DATAERR) from a runtime error (70, EX_SOFTWARE). */
    if (result == INTERPRET_COMPILE_ERROR) exit(65);
    if (result == INTERPRET_RUNTIME_ERROR) exit(70);
}

int main(int argc, const char *argv[])
{
    initVM();

    if (argc == 1) {
        repl();
    } else if (argc == 2) {
        runFile(argv[1]);
    } else {
        fprintf(stderr, "Usage: lvm [path]\n");
        freeVM();
        exit(64);   /* EX_USAGE: the command line itself was wrong               */
    }

    freeVM();
    return 0;
}
