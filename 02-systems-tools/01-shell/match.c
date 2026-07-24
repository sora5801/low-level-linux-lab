/* ===========================================================================
 * match.c — glob pattern matching, the iterative-with-backtracking way.
 * ===========================================================================
 *
 * WHY THIS FILE HAS NO #includes
 * ------------------------------
 * Everything here is pure computation over two C strings. There is no reason to
 * pull in <string.h> or anything else, and by staying header-free this file
 * cross-compiles to Linux SysV assembly on ANY host (see asm/match.s). That is a
 * requirement of the lab: real project source that is self-contained gets its
 * assembly committed alongside the extracted asm/demo.c.
 *
 * THE ALGORITHM (and why it is O(n*m), never exponential)
 * -------------------------------------------------------
 * The naive way to match `*` is recursion: "try matching the rest of the
 * pattern at every suffix of the string." That is the same trap that makes
 * pathological regexes blow up — it can explore exponentially many splits.
 *
 * Instead we scan both strings left to right with TWO saved "backtrack"
 * pointers that remember the single most recent `*`:
 *
 *     star_pat : the pattern position just AFTER the last '*' we saw
 *     star_str : the string  position where that '*' currently starts matching
 *
 * On a mismatch we don't recurse — we "back up" to the last star, let it swallow
 * one more input character (star_str++), and resume. Because star_str only ever
 * moves forward, the total work is bounded by |pat| * |str|. This is the classic
 * technique used by editors and shells; keep it in your toolbox.
 * ===========================================================================
 */

/* We type our own char-as-int so ranges like [a-z] compare UNSIGNED. If bytes
 * were treated as signed char, a byte >= 0x80 would compare as negative and
 * range tests would misbehave for non-ASCII filenames. */
typedef unsigned char uchar;

/* ---------------------------------------------------------------------------
 * match_bracket — evaluate a bracket expression "[...]" against one character.
 *
 *   p   : points AT the opening '['.
 *   c   : the candidate character (already unsigned).
 *   end : OUT — set to the position just past the closing ']' on success.
 *
 * Returns 1 if `c` is in the class, 0 if not, and -1 if the bracket is
 * malformed (no closing ']'), in which case the caller must treat the '[' as an
 * ordinary literal character and *end is left untouched.
 *
 * Grammar handled:
 *   - optional leading '!' or '^' negates the whole class
 *   - a ']' immediately after the (possibly negated) '[' is a literal ']'
 *   - "a-z" inside is an inclusive range; a '-' with no rhs is a literal '-'
 * --------------------------------------------------------------------------- */
static int match_bracket(const char *p, uchar c, const char **end)
{
    const char *q = p + 1;          /* step over the '['                       */

    int negate = 0;
    if (*q == '!' || *q == '^') {   /* [!...] and [^...] both mean "not in set" */
        negate = 1;
        q++;
    }

    /* A ']' as the very first class member is a literal ']', not the closer.
     * We track "is this the first member" so that rule applies exactly once. */
    int matched = 0;
    int first   = 1;

    while (*q != '\0' && (*q != ']' || first)) {
        first = 0;

        /* Range "x-y": only when the '-' is genuinely between two members and
         * is not the class terminator. Otherwise '-' is a literal character. */
        if (q[0] != '\0' && q[1] == '-' && q[2] != '\0' && q[2] != ']') {
            uchar lo = (uchar)q[0];
            uchar hi = (uchar)q[2];
            if (lo <= c && c <= hi)     /* inclusive range test (unsigned)     */
                matched = 1;
            q += 3;                     /* consumed "x-y"                      */
        } else {
            if ((uchar)*q == c)         /* single-character member             */
                matched = 1;
            q += 1;
        }
    }

    if (*q != ']')                  /* ran off the end: no closing bracket     */
        return -1;                  /*   -> caller treats '[' as a literal     */

    *end = q + 1;                   /* hand back the position after ']'        */
    return negate ? !matched : matched;
}

/* ---------------------------------------------------------------------------
 * wildcard_match — the public entry point. See match.h for semantics.
 * --------------------------------------------------------------------------- */
int wildcard_match(const char *pat, const char *str)
{
    /* The single-star backtrack state. NULL means "no '*' seen yet, so there is
     * nowhere to back up to" — a mismatch in that state is a hard failure. */
    const char *star_pat = 0;
    const char *star_str = 0;

    /* Advance through the STRING. Every branch either consumes input, consumes
     * pattern, or backtracks to the last star; nothing loops in place. */
    while (*str != '\0') {
        uchar c = (uchar)*str;

        if (*pat == '?') {
            /* '?' matches this one character unconditionally. */
            pat++;
            str++;
        } else if (*pat == '[') {
            /* Bracket expression. If it is malformed, fall through to treating
             * '[' as a literal by using the generic literal path below. */
            const char *after;
            int r = match_bracket(pat, c, &after);
            if (r == 1) {                 /* class matched: consume both        */
                pat = after;
                str++;
            } else if (r == 0) {          /* class present but did not match    */
                if (star_pat) {           /*   backtrack into the last '*'      */
                    pat = star_pat;
                    str = ++star_str;
                } else {
                    return 0;
                }
            } else {                      /* r == -1: '[' is a literal char     */
                if (c == '[') {
                    pat++;
                    str++;
                } else if (star_pat) {
                    pat = star_pat;
                    str = ++star_str;
                } else {
                    return 0;
                }
            }
        } else if (*pat == '*') {
            /* A '*'. It matches the empty string first: remember where the
             * pattern continues and where the string is now, then keep going
             * WITHOUT consuming input. Consecutive '*'s collapse naturally
             * because the next iteration sees another '*' and just re-saves. */
            star_pat = ++pat;             /* pattern resumes after the star     */
            star_str = str;               /* the star currently eats [str..str) */
        } else if (*pat == '\\' && pat[1] != '\0') {
            /* Backslash escapes the next character into a literal. */
            if ((uchar)pat[1] == c) {
                pat += 2;
                str++;
            } else if (star_pat) {
                pat = star_pat;
                str = ++star_str;
            } else {
                return 0;
            }
        } else if ((uchar)*pat == c) {
            /* Ordinary literal character that matches. */
            pat++;
            str++;
        } else if (star_pat) {
            /* Literal mismatch, but a '*' is on the hook: let it swallow one
             * more character of the string and try again from just after it.
             * star_str only moves forward, which is what bounds the work. */
            pat = star_pat;
            str = ++star_str;
        } else {
            /* Mismatch with no star to fall back on: this name cannot match. */
            return 0;
        }
    }

    /* The string is fully consumed. The pattern matches iff whatever remains is
     * only stars (each of which can match the empty tail). */
    while (*pat == '*')
        pat++;
    return *pat == '\0';
}
