/* ===========================================================================
 * config.c — the service-file parser.
 * ===========================================================================
 *
 * Grammar (deliberately tiny — a supervisor's config should be boring):
 *
 *     # comments start with '#' and run to end of line
 *     [name]                         <- begins a service block
 *     exec = /path/to/prog arg1 arg2 <- required; the command line to run
 *     after = dep1 dep2              <- optional; start these services first
 *     restart = always|on-failure|no <- optional; default is on-failure
 *
 * Whitespace around '=' is ignored. `exec` is split on spaces into argv; we do
 * NOT implement shell quoting (a documented Stretch goal) — wrap complex
 * commands in `/bin/sh -c "..."` instead, which is what real init systems end
 * up doing anyway.
 *
 * This file DOES use libc string helpers (<string.h>, <ctype.h>) and stdio to
 * read the file, so unlike order.c it is not part of the header-free asm
 * deliverable. Reading a config into fixed buffers is exactly the kind of code
 * where off-by-ones bite, so every bound is checked explicitly.
 * ===========================================================================
 */
#include "config.h"

#include <stdio.h>     /* fopen, fgets, fclose                                 */
#include <string.h>    /* strlen, strcmp, memcpy, memset                       */
#include <ctype.h>     /* isspace                                              */

/* Trim leading and trailing ASCII whitespace in place, returning the new start.
 * Editing in place is safe: we only ever shrink the string within its own
 * buffer, never grow it, so no bound can be exceeded. */
static char *trim(char *s)
{
    while (*s && isspace((unsigned char)*s))
        s++;                                   /* skip leading blanks           */
    char *end = s + strlen(s);
    while (end > s && isspace((unsigned char)end[-1]))
        *--end = '\0';                         /* chop trailing blanks          */
    return s;
}

/* Split `value` on runs of whitespace into svc->argv[], copying the bytes into
 * svc->exec_buf so the pointers stay valid for the life of the struct. Returns
 * 0 on success, -1 if the command is empty or overflows a fixed bound. */
static int split_exec(struct sup_service *svc, const char *value)
{
    /* Copy the command line into the per-service backing buffer first, bounded
     * by SUP_MAX_EXEC (including the terminating NUL). We tokenize this COPY so
     * the argv pointers alias stable storage we own. */
    size_t n = strlen(value);
    if (n >= SUP_MAX_EXEC)
        return -1;                             /* command line too long         */
    memcpy(svc->exec_buf, value, n + 1);       /* +1 copies the NUL             */

    int argc = 0;
    char *p = svc->exec_buf;
    while (*p) {
        while (*p && isspace((unsigned char)*p))
            *p++ = '\0';                       /* turn separators into NULs     */
        if (!*p)
            break;                             /* trailing whitespace only      */
        if (argc >= SUP_MAX_ARGS)
            return -1;                         /* too many argv tokens          */
        svc->argv[argc++] = p;                 /* start of a token              */
        while (*p && !isspace((unsigned char)*p))
            p++;                               /* walk to the token's end       */
    }
    if (argc == 0)
        return -1;                             /* exec = (nothing) is invalid   */
    svc->argv[argc] = 0;                       /* execvp needs a NULL sentinel  */
    svc->argc = argc;
    return 0;
}

/* Store a space/comma-separated dependency list into svc->after_names[]. */
static int split_after(struct sup_service *svc, char *value)
{
    char *p = value;
    while (*p) {
        while (*p && (isspace((unsigned char)*p) || *p == ','))
            p++;                               /* skip separators               */
        if (!*p)
            break;
        char *start = p;
        while (*p && !isspace((unsigned char)*p) && *p != ',')
            p++;
        size_t len = (size_t)(p - start);
        if (len >= SUP_MAX_NAME)
            return -1;                         /* dependency name too long      */
        if (svc->n_after >= SUP_MAX_DEPS)
            return -1;                         /* too many dependencies         */
        memcpy(svc->after_names[svc->n_after], start, len);
        svc->after_names[svc->n_after][len] = '\0';
        svc->n_after++;
    }
    return 0;
}

int sup_config_parse(const char *path, struct sup_service *svc, const char **err)
{
    *err = 0;

    FILE *f = fopen(path, "r");                /* text mode; fgets handles lines */
    if (!f) {
        *err = "cannot open config file";
        return -1;
    }

    int count = 0;                             /* services parsed so far         */
    int cur = -1;                              /* index of the block we are in   */
    char line[SUP_MAX_LINE];

    while (fgets(line, sizeof line, f)) {
        /* Strip a trailing newline and any inline comment. A '#' anywhere begins
         * a comment; there is no escaping (kept intentionally simple). */
        char *hash = strchr(line, '#');
        if (hash)
            *hash = '\0';
        char *s = trim(line);
        if (*s == '\0')
            continue;                          /* blank / comment-only line      */

        if (*s == '[') {
            /* ---- new service block: [name] ---- */
            char *close = strchr(s, ']');
            if (!close) { *err = "missing ']' in section header"; fclose(f); return -1; }
            *close = '\0';
            char *name = trim(s + 1);
            size_t len = strlen(name);
            if (len == 0 || len >= SUP_MAX_NAME) { *err = "bad service name"; fclose(f); return -1; }
            if (count >= SUP_MAX_SERVICES) { *err = "too many services"; fclose(f); return -1; }

            cur = count++;
            /* Zero the whole struct so every runtime field starts in a known
             * state (STOPPED, pid 0, restart_count 0, no argv). memset over the
             * fixed struct is cheaper and safer than field-by-field init. */
            memset(&svc[cur], 0, sizeof svc[cur]);
            memcpy(svc[cur].name, name, len + 1);
            svc[cur].restart = SUP_RESTART_ON_FAILURE;  /* sane default policy   */
            continue;
        }

        /* ---- key = value inside the current block ---- */
        if (cur < 0) { *err = "directive before any [service] block"; fclose(f); return -1; }
        char *eq = strchr(s, '=');
        if (!eq) { *err = "expected key = value"; fclose(f); return -1; }
        *eq = '\0';
        char *key = trim(s);
        char *val = trim(eq + 1);

        if (strcmp(key, "exec") == 0) {
            if (split_exec(&svc[cur], val) != 0) { *err = "bad exec line"; fclose(f); return -1; }
        } else if (strcmp(key, "after") == 0) {
            if (split_after(&svc[cur], val) != 0) { *err = "bad after line"; fclose(f); return -1; }
        } else if (strcmp(key, "restart") == 0) {
            if (strcmp(val, "always") == 0)          svc[cur].restart = SUP_RESTART_ALWAYS;
            else if (strcmp(val, "on-failure") == 0) svc[cur].restart = SUP_RESTART_ON_FAILURE;
            else if (strcmp(val, "no") == 0)         svc[cur].restart = SUP_RESTART_NO;
            else { *err = "restart must be always|on-failure|no"; fclose(f); return -1; }
        } else {
            *err = "unknown directive";
            fclose(f);
            return -1;
        }
    }
    fclose(f);

    /* Every service needs a command. Catch an empty [block] with no exec here
     * rather than failing mysteriously at fork() time. */
    for (int i = 0; i < count; i++) {
        if (svc[i].argc == 0) { *err = "a service has no exec line"; return -1; }
    }

    /* --- Resolve `after` names to indices ---------------------------------
     * We do this only after the whole file is read so a service may depend on
     * one defined later in the file. An unresolved name is fatal: we refuse to
     * boot rather than silently drop an ordering constraint. */
    for (int i = 0; i < count; i++) {
        for (int d = 0; d < svc[i].n_after; d++) {
            int found = -1;
            for (int j = 0; j < count; j++) {
                if (strcmp(svc[i].after_names[d], svc[j].name) == 0) { found = j; break; }
            }
            if (found < 0) { *err = "unknown service in 'after'"; return -1; }
            if (found == i) { *err = "a service cannot depend on itself"; return -1; }
            svc[i].after_idx[d] = found;
        }
    }

    return count;
}
