/* ===========================================================================
 * compiler.h — the front end's public face: source text -> ObjFunction.
 * ===========================================================================
 *
 * The compiler is single-pass: it scans, parses (Pratt / precedence-climbing),
 * and emits bytecode in one walk over the source — there is no separate AST.
 * That keeps the whole front end small and is exactly how clox, Lua, and many
 * production script compilers work. The result is one ObjFunction (the implicit
 * top-level "<script>") whose chunk the VM then runs.
 * ===========================================================================
 */
#ifndef CLOXI_COMPILER_H
#define CLOXI_COMPILER_H

#include "object.h"

/* Compile `source` into the top-level function. Returns NULL on any compile
 * error (the compiler prints the diagnostics itself). The returned function is
 * freshly heap-allocated and reachable only via the caller until the VM roots
 * it — the caller pushes it immediately (see vm.c/interpret). */
ObjFunction *compile(const char *source);

/* GC hook: while the compiler is running, the functions it is BUILDING are not
 * yet on the VM stack, so the collector would free them. The GC calls this to
 * mark every function in the active compiler chain as a root. */
void markCompilerRoots(void);

#endif /* CLOXI_COMPILER_H */
