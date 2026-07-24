/* ===========================================================================
 * assemble.c — the symbol table and the TWO-PASS driver.
 * ===========================================================================
 *
 * This is where "two-pass assembly" — the headline feature that lets you refer
 * to a label BEFORE it is defined — actually happens.
 *
 * THE FORWARD-REFERENCE PROBLEM
 * -----------------------------
 *     jmp done      # done is 40 bytes further on — we don't know its address yet
 *     ...
 *     done:         # ... only learned HERE
 *
 * A single left-to-right pass cannot encode `jmp done`, because the rel32 it
 * needs is (address_of_done - address_after_jmp) and address_of_done is still
 * in the future. The classic fix is two passes:
 *
 *   PASS 1  (layout): walk every statement, add up instruction SIZES, and
 *           record each label's (section, offset). Emit no bytes. Afterwards
 *           every symbol's address within its section is known.
 *   PASS 2  (emit):   walk again, now knowing all addresses, and write the
 *           bytes. A forward `jmp done` resolves cleanly because `done` is
 *           already in the symbol table.
 *
 * We can compute exact sizes in pass 1 because masm uses FIXED-SIZE branch
 * encodings (every jmp/call is rel32, 5-6 bytes; never a 2-byte short form).
 * A production assembler instead does "branch relaxation": it starts by
 * assuming short branches and iterates passes until sizes stop changing, so it
 * can pick the shortest form. We trade that optimisation for a clean, single,
 * non-iterating layout pass — and say so in the README.
 * ===========================================================================
 */
#include "asm.h"
#include <stdio.h>
#include <string.h>

/* --- symbol table (linear scan; fine at teaching scale) -------------------- */

Symbol *sym_find(Assembler *A, const char *name)
{
    for (int i = 0; i < A->nsyms; i++)
        if (strcmp(A->syms[i].name, name) == 0) return &A->syms[i];
    return NULL;
}

/* Define a label: it becomes a DEFINED symbol at (section, value). A second
 * definition of the same name is an error — that is a real bug in the input. */
Symbol *sym_define(Assembler *A, const char *name, int section, uint64_t value, int line)
{
    Symbol *s = sym_find(A, name);
    if (s) {
        if (s->defined) {
            fprintf(stderr, "%s:%d: error: duplicate label '%s'\n",
                    A->srcname, line, name);
            A->errors++;
            return s;
        }
    } else {
        if (A->nsyms >= MAX_SYMS) {
            fprintf(stderr, "masm: too many symbols\n");
            A->errors++;
            return NULL;
        }
        s = &A->syms[A->nsyms++];
        memset(s, 0, sizeof *s);
        snprintf(s->name, MAXNAME, "%s", name);
        s->section = -1;
    }
    s->defined = 1;
    s->section = section;
    s->value   = value;
    return s;
}

/* Referenced-but-undefined name -> an UNDEFINED, GLOBAL symbol. ELF requires
 * undefined symbols to have global (or weak) binding so the linker will try to
 * satisfy them from another object. */
Symbol *sym_ref_undef(Assembler *A, const char *name)
{
    Symbol *s = sym_find(A, name);
    if (s) return s;
    if (A->nsyms >= MAX_SYMS) { fprintf(stderr, "masm: too many symbols\n"); A->errors++; return NULL; }
    s = &A->syms[A->nsyms++];
    memset(s, 0, sizeof *s);
    snprintf(s->name, MAXNAME, "%s", name);
    s->defined   = 0;
    s->is_global = 1;      /* undefined => must be global for the linker       */
    s->section   = -1;     /* SHN_UNDEF                                        */
    return s;
}

/* Remember a `.globl NAME`. We apply it to the actual symbol later, once all
 * labels (which may appear after the .globl) have been seen. */
void add_global(Assembler *A, const char *name)
{
    for (int i = 0; i < A->nglobals; i++)
        if (strcmp(A->globals[i], name) == 0) return;         /* already noted */
    if (A->nglobals >= MAX_SYMS) return;
    snprintf(A->globals[A->nglobals++], MAXNAME, "%s", name);
}

/* Record a relocation the linker must apply. */
void add_reloc(Assembler *A, uint64_t off, const char *sym, uint32_t type, int64_t add)
{
    if (A->nrelocs >= MAX_RELOCS) { fprintf(stderr, "masm: too many relocations\n"); A->errors++; return; }
    Reloc *r = &A->relocs[A->nrelocs++];
    r->offset = off;
    snprintf(r->sym, MAXNAME, "%s", sym);
    r->type   = type;
    r->addend = add;
}

/* After pass 1: turn every `.globl NAME` into a global binding. If NAME was
 * defined as a label, mark it global; if it was never defined, create it as an
 * undefined global (a promise the linker must satisfy). This MUST run before
 * pass 2, because a jump to a GLOBAL label emits a relocation (to allow link-
 * time interposition) rather than being resolved locally. */
static void apply_globals(Assembler *A)
{
    for (int i = 0; i < A->nglobals; i++) {
        Symbol *s = sym_find(A, A->globals[i]);
        if (s) s->is_global = 1;
        else   sym_ref_undef(A, A->globals[i]);   /* .globl of an undefined name */
    }
}

/* ---------------------------------------------------------------------------
 * The driver: run pass 1, apply globals, then run pass 2.
 * --------------------------------------------------------------------------- */
int assemble(Assembler *A, Stmt *stmts, int n)
{
    /* ---------- PASS 1: LAYOUT (emit == 0) -------------------------------- */
    A->cur = SEC_TEXT;                       /* default section before any .text */
    uint64_t off[NUM_SECS] = {0, 0};         /* running offset per section       */

    for (int i = 0; i < n; i++) {
        Stmt *s = &stmts[i];
        if (s->kind == STK_LABEL) {
            /* A label's address is "here": the current section and offset. */
            sym_define(A, s->label, A->cur, off[A->cur], s->line);
            continue;
        }
        int sz = encode_stmt(A, s, 0);       /* sizing only; also switches .cur  */
        s->section = A->cur;
        s->offset  = off[A->cur];
        s->size    = sz;
        off[A->cur] += (uint64_t)sz;
    }

    apply_globals(A);                        /* bindings known before we emit    */
    if (A->errors) return A->errors;         /* don't emit against a broken table */

    /* ---------- PASS 2: EMIT (emit == 1) --------------------------------- */
    A->cur = SEC_TEXT;
    /* buffers are still empty (pass 1 never wrote); assert that assumption.  */
    A->sec[SEC_TEXT].len = 0;
    A->sec[SEC_DATA].len = 0;

    for (int i = 0; i < n; i++) {
        Stmt *s = &stmts[i];
        if (s->kind == STK_LABEL) continue;  /* already placed in pass 1        */

        /* Defensive check: pass-2 position must match pass-1 layout exactly. */
        if (s->kind == STK_INSN && A->sec[SEC_TEXT].len != s->offset) {
            fprintf(stderr, "masm: internal error: layout drift at line %d "
                    "(expected %llu, at %llu)\n", s->line,
                    (unsigned long long)s->offset,
                    (unsigned long long)A->sec[SEC_TEXT].len);
            A->errors++;
        }
        encode_stmt(A, s, 1);
    }
    return A->errors;
}
