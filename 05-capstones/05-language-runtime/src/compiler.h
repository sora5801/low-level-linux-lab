/* ===========================================================================
 * compiler.h — the single-pass front end: source text -> an ObjFunction.
 * ===========================================================================
 *
 * compile() runs the scanner and a Pratt (precedence-climbing) parser that emits
 * bytecode directly into the function being built — there is no AST. It returns
 * the top-level <script> as an ObjFunction (its bytecode is the whole program's
 * body), or NULL if a compile error was reported. Stand-in role: this is the
 * "front end" half of sibling 02-systems-tools/15-language-vm.
 */
#ifndef LUMEN_COMPILER_H
#define LUMEN_COMPILER_H

#include "object.h"

ObjFunction *compile(const char *source);

#endif /* LUMEN_COMPILER_H */
