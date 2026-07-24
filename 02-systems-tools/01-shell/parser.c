/* ===========================================================================
 * parser.c — turn the token stream into a `job` (a pipeline of processes).
 * ===========================================================================
 *
 * The grammar we accept is one pipeline per line, optionally backgrounded:
 *
 *     line     := pipeline [ '&' ]
 *     pipeline := command ( '|' command )*
 *     command  := ( WORD | redirection )+
 *     redirection := ('<' | '>' | '>>' | '2>') WORD
 *
 * Sequencing operators (';', '&&', '||') and subshells are out of scope and
 * noted in the README as the natural next step. Each WORD is glob-expanded here
 * (so one word can become several argv entries), which is why we build argv with
 * a strvec rather than a fixed array.
 * ===========================================================================
 */
#define _GNU_SOURCE
#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <unistd.h>      /* STDIN_FILENO, STDOUT_FILENO, STDERR_FILENO         */

#include "shell.h"

/* free_job — release every allocation a job owns: each process's argv strings
 * and array, its redirection list, the command text, and the structs. This is
 * the single place that knows the ownership graph, so it is the single place
 * that frees it. Called after a job completes or on a parse error. */
void free_job(job *j)
{
    if (!j)
        return;
    process *p = j->first_process;
    while (p) {
        process *pnext = p->next;
        if (p->argv) {
            for (int i = 0; p->argv[i]; i++)
                free(p->argv[i]);
            free(p->argv);
        }
        redir *r = p->redirs;
        while (r) {
            redir *rnext = r->next;
            free(r->file);
            free(r);
            r = rnext;
        }
        free(p);
        p = pnext;
    }
    free(j->command);
    free(j);
}

/* finish_process — package the accumulated argv (`args`) and redirection list
 * into a process struct and append it to the job. Ownership of the argv backing
 * array and of the redir list transfers to the new process; the caller's
 * accumulators are reset to empty afterward. Returns the process, or NULL if
 * there were no argv words (an empty pipeline stage, e.g. "cmd | | cmd"). */
static process *finish_process(job *j, process **tail, strvec *args, redir *rhead)
{
    if (args->len == 0)
        return NULL;                    /* caller reports the syntax error      */

    process *p = xmalloc(sizeof *p);
    memset(p, 0, sizeof *p);

    strvec_push(args, NULL);            /* NULL-terminate the argv vector        */
    p->argv   = args->items;            /* transfer the backing array            */
    p->redirs = rhead;

    /* Link at the tail so processes stay in left-to-right pipeline order. */
    if (*tail) (*tail)->next = p;
    else       j->first_process = p;
    *tail = p;

    strvec_init(args);                  /* fresh accumulator for the next stage  */
    return p;
}

/* ---------------------------------------------------------------------------
 * parse_line — see shell.h. On success returns an owned job; on an empty line
 * returns NULL with *had_error == 0; on a syntax/lex error returns NULL with
 * *had_error == 1 (a message was already printed).
 * --------------------------------------------------------------------------- */
job *parse_line(const char *line, int *had_error)
{
    *had_error = 0;

    job *j = xmalloc(sizeof *j);
    memset(j, 0, sizeof *j);
    j->stdin_fd  = STDIN_FILENO;
    j->stdout_fd = STDOUT_FILENO;
    j->stderr_fd = STDERR_FILENO;

    process *tail  = NULL;              /* last process appended                 */
    redir   *rhead = NULL, *rtail = NULL;  /* current stage's redirection list   */
    strvec   args;                     /* current stage's argv words            */
    strvec_init(&args);

    const char *cursor = line;
    token t;
    int   saw_amp   = 0;               /* have we consumed a trailing '&'?       */
    int   need_cmd  = 0;               /* a '|' was seen; a command must follow  */

/* On any error: print (already done), free partial state, return NULL. */
#define FAIL(msg) do {                                   \
        fprintf(stderr, "shell: syntax error: %s\n", msg);\
        strvec_free(&args);                              \
        while (rhead) { redir *rn = rhead->next; free(rhead->file); free(rhead); rhead = rn; } \
        free_job(j);                                     \
        *had_error = 1;                                  \
        return NULL;                                     \
    } while (0)

    for (;;) {
        if (lex_next(&cursor, &t) < 0)
            FAIL("bad token");         /* lexer already printed the specifics   */

        if (t.type == T_EOF)
            break;

        /* A trailing '&' must be the last thing on the line. Anything after it
         * would be a second job, which we do not support. */
        if (saw_amp)
            FAIL("unexpected token after '&'");

        switch (t.type) {
        case T_WORD:
            /* Glob-expand eligible words; a word that matches nothing (or was
             * quoted) contributes itself literally. */
            if (t.can_glob) {
                int n = glob_expand(t.text, &args);
                if (n == 0)
                    strvec_push(&args, xstrdup(t.text));
            } else {
                strvec_push(&args, xstrdup(t.text));
            }
            need_cmd = 0;              /* we now have a command word for this stage */
            break;

        case T_PIPE:
            /* End the current stage and start a new one. The stage must have a
             * command; "| foo" or "foo | | bar" is a syntax error. */
            if (!finish_process(j, &tail, &args, rhead))
                FAIL("empty command near '|'");
            rhead = rtail = NULL;      /* redirs belong to the finished stage    */
            need_cmd = 1;              /* the new stage still needs a command    */
            break;

        case T_LT:
        case T_GT:
        case T_GTGT:
        case T_2GT: {
            /* A redirection operator must be followed by a WORD (the filename).
             * We take the filename literally — globbing a redirect target that
             * matches multiple files is an "ambiguous redirect" error in real
             * shells; here we simply do not glob it. */
            ttype op = t.type;
            token f;
            if (lex_next(&cursor, &f) < 0)
                FAIL("bad token after redirection");
            if (f.type != T_WORD)
                FAIL("expected filename after redirection");

            redir *r = xmalloc(sizeof *r);
            r->type = (op == T_LT)   ? REDIR_IN
                    : (op == T_GT)   ? REDIR_OUT
                    : (op == T_GTGT) ? REDIR_APPEND
                                     : REDIR_ERR;
            r->file = xstrdup(f.text);
            r->next = NULL;
            if (rtail) rtail->next = r;
            else       rhead = r;
            rtail = r;
            break;
        }

        case T_AMP:
            /* Background the whole pipeline. Remember it and require EOF next. */
            saw_amp = 1;
            j->background = 1;
            break;

        default:
            FAIL("unexpected token");
        }
    }

    /* Close out the final stage. finish_process returns NULL when that stage
     * had no command word — which can mean several things we must distinguish. */
    if (!finish_process(j, &tail, &args, rhead)) {
        /* Redirection(s) but no command, e.g. "ls | > a" or a bare "> a". Note
         * that "ls >a" is FINE: there the redir attaches to the non-empty "ls"
         * stage, so we never reach here. */
        if (rhead)
            FAIL("redirection without a command");
        /* A trailing '|' with nothing after it, e.g. "foo |". */
        if (need_cmd)
            FAIL("empty command near '|'");
        /* Otherwise the line was truly empty (spaces, or a lone '&'): not an
         * error, just nothing to run. */
        if (j->first_process == NULL) {
            strvec_free(&args);
            free_job(j);
            return NULL;
        }
    }
    strvec_free(&args);                /* args was emptied by finish_process     */

    /* Keep the original text (minus any trailing '&') for the jobs table. */
    j->command = xstrdup(line);

    return j;

#undef FAIL
}
