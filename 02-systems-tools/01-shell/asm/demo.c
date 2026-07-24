/* ===========================================================================
 * asm/demo.c — SELF-CONTAINED extraction of the shell's most instructive
 *              pure-logic routine: glob matching with '*' backtracking.
 * ===========================================================================
 *
 * This file exists purely to generate teaching assembly. It has NO #includes and
 * declares nothing from the system, so a bare cross-compiler turns it into
 * x86-64 System V assembly with no libc or headers present (see the exact clang
 * commands in the README / Makefile, and the hand-annotated demo.annotated.s).
 *
 * WHAT IT IS
 * ----------
 * `glob_star` below is the heart of ../match.c with the bracket-expression case
 * removed, leaving the part worth reading in assembly: the two-pointer
 * backtracking that makes '*' cheap. The full matcher (with [a-z] classes) is in
 * ../match.c, whose assembly is committed as asm/match.s for comparison.
 *
 * THE TRICK (why '*' does not cause exponential blow-up)
 * ------------------------------------------------------
 * We scan the string left to right and keep ONE remembered star:
 *     star_pat = pattern position just after the last '*'
 *     star_str = where in the string that '*' currently begins matching
 * On any mismatch we do not recurse; we make the last star eat one more input
 * character (star_str++) and resume. star_str only moves forward, so the work is
 * bounded by |pat| * |str|. Watching star_pat/star_str live in registers, and
 * the mismatch path rewrite `pat = star_pat; str = ++star_str`, is the lesson.
 * ===========================================================================
 */

/* Bytes compared as UNSIGNED so high-bit (non-ASCII) filename bytes behave. */
typedef unsigned char uchar;

/* ---------------------------------------------------------------------------
 * glob_star — match `str` against glob `pat`, honoring only '*', '?', '\\' and
 *             literal characters (no bracket classes). Returns 1 on match.
 *
 * SysV ABI:  pat -> rdi (arg0),  str -> rsi (arg1),  result -> rax.
 * Pure leaf function: no calls, so it may use the 128-byte red zone and needs
 * no aligned stack. The optimizer keeps everything in registers.
 * --------------------------------------------------------------------------- */
int glob_star(const char *pat, const char *str)
{
    const char *star_pat = 0;   /* NULL until we have seen a '*' to back up to */
    const char *star_str = 0;

    while (*str != '\0') {
        uchar c = (uchar)*str;

        if (*pat == '?') {                  /* '?' : match any single char      */
            pat++;
            str++;
        } else if (*pat == '*') {           /* '*' : remember, match empty here */
            star_pat = ++pat;               /*   pattern resumes after the star */
            star_str = str;                 /*   star currently eats nothing yet */
        } else if (*pat == '\\' && pat[1] != '\0') {
            if ((uchar)pat[1] == c) {       /* '\\x' : literal x                */
                pat += 2;
                str++;
            } else if (star_pat) {          /*   mismatch -> backtrack to star  */
                pat = star_pat;
                str = ++star_str;
            } else {
                return 0;
            }
        } else if ((uchar)*pat == c) {      /* ordinary literal that matches    */
            pat++;
            str++;
        } else if (star_pat) {              /* mismatch, but a '*' can eat more  */
            pat = star_pat;
            str = ++star_str;               /*   let the star swallow one char  */
        } else {
            return 0;                       /* mismatch, nothing to fall back on */
        }
    }

    while (*pat == '*')                     /* trailing stars match empty tail  */
        pat++;
    return *pat == '\0';
}

/* ---------------------------------------------------------------------------
 * demo_run — a tiny self-contained exercise so the translation unit is complete
 * and glob_star cannot be optimized away when this file is compiled alone.
 *
 * It checks a handful of patterns whose answers are known, packing the results
 * into the low bits of the return value. Expected value: 0b1011 == 11.
 *   bit0: "*.c"   vs "main.c"   -> 1 (star eats "main")
 *   bit1: "a?c"   vs "abc"      -> 1 ('?' matches 'b')
 *   bit2: "a*d"   vs "abc"      -> 0 (no 'd' at the end)
 *   bit3: "*"     vs ""         -> 1 (star matches the empty string)
 * --------------------------------------------------------------------------- */
int demo_run(void)
{
    int r = 0;
    r |= glob_star("*.c", "main.c") << 0;
    r |= glob_star("a?c", "abc")    << 1;
    r |= glob_star("a*d", "abc")    << 2;
    r |= glob_star("*",   "")       << 3;
    return r;   /* expect 0b1011 == 11 */
}
