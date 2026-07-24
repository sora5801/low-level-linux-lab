/* ===========================================================================
 * keygen.c — THE SOLUTION. Prints a valid serial for any username.
 * ===========================================================================
 *
 * A keygen is the "proof" that you reversed the crackme: you recovered the
 * username -> serial function and re-implemented it. Because the serial is a
 * pure function of the name (see serial.h), the keygen is embarrassingly short
 * — it just runs the transform forward and formats the result. That asymmetry
 * (hard to reverse, trivial to re-run) is the whole point of a keygen-me.
 *
 * In a real engagement you would NOT have serial.h; you would reconstruct
 * key_from_name from the disassembly. docs/writeup.md walks that reconstruction
 * (the FNV constants, the `rolq $7`, the fmix64 finalizer) so you can see how
 * this file would be *derived* rather than shared. Here we share the header so
 * the crackme and its solution provably agree.
 *
 * Build (Linux/WSL):   clang -O2 keygen.c -o keygen
 * Use:                 ./keygen alice        ->  e.g. 1A2B-3C4D-5E6F-7089
 *                      ./crackme alice "$(./keygen alice)"   ->  Correct!
 * ===========================================================================
 */

#include <stdio.h>      /* printf                                              */
#include <stdint.h>     /* uint64_t                                            */
#include "serial.h"     /* the shared transform we are "solving"               */

int main(int argc, char **argv)
{
    if (argc != 2) {
        fprintf(stderr, "usage: %s <username>\n", argv[0]);
        return 1;
    }

    /* Forward-run the exact transform the crackme checks against... */
    uint64_t key = key_from_name(argv[1]);

    /* ...and format it the same way the crackme formats `expected`. */
    char serial[20];
    format_serial(key, serial);

    /* Print just the serial + newline so it composes in shell pipelines:
     *     ./crackme alice "$(./keygen alice)"                                  */
    printf("%s\n", serial);
    return 0;
}
