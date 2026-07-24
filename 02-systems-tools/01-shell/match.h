/* ===========================================================================
 * match.h — the shell's glob pattern matcher (interface).
 * ===========================================================================
 *
 * This is deliberately a *pure-logic* module: it takes a shell glob PATTERN and
 * a candidate FILENAME and answers "does this name match?". It performs NO I/O
 * and NO allocation, so it compiles with zero system headers — which is exactly
 * why the accompanying match.c is the translation unit we turn into teaching
 * assembly (see asm/). The directory-walking half of globbing lives in
 * expand.c, which calls the function declared here once per directory entry.
 *
 * Supported metacharacters (POSIX glob subset, per `man 7 glob`):
 *   *        matches any run of zero or more characters
 *   ?        matches exactly one character
 *   [abc]    matches one character from the set
 *   [a-z]    matches one character in the (inclusive) range
 *   [!abc]   / [^abc]   negated set
 *   \x       matches the literal character x (escapes a metacharacter)
 * ===========================================================================
 */
#ifndef SHELL_MATCH_H
#define SHELL_MATCH_H

/* Returns 1 if `str` matches the glob `pat`, 0 otherwise. Both are ordinary
 * NUL-terminated C strings. Pure function: no globals, no syscalls, no malloc,
 * so it is safe to call in the hottest inner loop of directory expansion. */
int wildcard_match(const char *pat, const char *str);

#endif /* SHELL_MATCH_H */
