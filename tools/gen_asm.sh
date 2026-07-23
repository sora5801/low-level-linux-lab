#!/usr/bin/env bash
# =============================================================================
# gen_asm.sh — turn a C translation unit into *didactic* x86-64 assembly.
# =============================================================================
#
# WHY THIS EXISTS
# ---------------
# Every C project in this repo ships the assembly the compiler actually emits,
# so you can correlate the C you wrote with the machine code the CPU runs. That
# is the single most valuable habit in low-level work: "keep `objdump -d` open
# constantly; correlate C to asm; learn what the optimizer does."
#
# We deliberately target *Linux* (the System V AMD64 ABI) even when you run this
# on another OS, because every project here assumes that ABI:
#
#   - integer/pointer args:  rdi, rsi, rdx, rcx, r8, r9      (then the stack)
#   - return value:          rax  (rdx:rax for 128-bit)
#   - syscall number:        rax ; syscall args: rdi,rsi,rdx,r10,r8,r9
#   - callee-saved:          rbx, rbp, r12-r15  (a function must preserve these)
#   - caller-saved:          rax, rcx, rdx, rsi, rdi, r8-r11
#   - the "red zone":        128 bytes below rsp that leaf functions may use
#   - stack alignment:       rsp % 16 == 0 at the point of a `call`
#
# clang is a cross-compiler by construction (one binary, many target backends),
# so `--target=x86_64-pc-linux-gnu -S` gives us real Linux asm regardless of the
# host. `-S` stops after code generation, so we never need a Linux linker/libc
# to *see* the assembly — which is exactly what we want for teaching.
#
# USAGE
# -----
#   tools/gen_asm.sh <file.c> [extra clang flags...]
#
# It writes, next to an ./asm/ directory:
#   asm/<file>.s            raw compiler output at -O1 (frame pointers kept)
#   asm/<file>.O0.s         -O0 output: the most literal C-statement -> asm map
#   asm/<file>.O2.s         -O2 output: "what the optimizer really does"
#
# The heavily-commented, human-readable `asm/<file>.annotated.s` is written by
# hand (or by an agent) *from* these files — the point of annotating is to
# explain, in prose, every prologue, ABI move, syscall, and optimization. See
# CONVENTIONS.md for the annotation style.
#
# We keep three optimization levels on purpose:
#   -O0  shows the naive, spill-everything-to-the-stack mapping. Easiest to read
#        as "this C line became these instructions."
#   -O1  is the sweet spot we annotate: registers are used sensibly but the
#        control flow still mirrors the source.
#   -O2  shows vectorization, inlining, strength reduction, and the tricks that
#        make "why is it doing THAT?" the whole point of the exercise.
# =============================================================================

set -euo pipefail

if [ "$#" -lt 1 ]; then
    echo "usage: $0 <file.c> [extra clang flags...]" >&2
    exit 2
fi

SRC="$1"; shift || true
EXTRA=("$@")

# Prefer clang (multi-target); fall back to a Linux-targeting gcc if present.
CC="${CC:-clang}"
TARGET="${TARGET:-x86_64-pc-linux-gnu}"

base="$(basename "${SRC%.c}")"
dir="$(dirname "$SRC")"
out="$dir/asm"
mkdir -p "$out"

# Flags shared by every optimization level:
#   --target            : force the System V AMD64 (Linux) ABI + ELF asm syntax
#   -S                  : emit assembly, stop before assembling/linking
#   -fno-asynchronous-unwind-tables : drop the .cfi_* noise so the asm is readable
#   -fno-jump-tables    : make switch statements branch, not table-index (clearer)
#   -masm=att is default (AT&T syntax: `op src, dst`). Pass -masm=intel to flip.
COMMON=( --target="$TARGET" -S -fno-asynchronous-unwind-tables -fno-jump-tables )

echo ">> ${CC} ${TARGET}: $SRC"

# -O0: keep the frame pointer so the stack frame is visible and legible.
"$CC" "${COMMON[@]}" -O0 -fno-omit-frame-pointer "${EXTRA[@]}" "$SRC" -o "$out/$base.O0.s"

# -O1: the level we annotate by hand. Frame pointer kept for teaching clarity.
"$CC" "${COMMON[@]}" -O1 -fno-omit-frame-pointer "${EXTRA[@]}" "$SRC" -o "$out/$base.s"

# -O2: let the optimizer off the leash so the annotation can explain its tricks.
"$CC" "${COMMON[@]}" -O2 "${EXTRA[@]}" "$SRC" -o "$out/$base.O2.s"

echo "   wrote $out/$base.O0.s  (naive, literal mapping)"
echo "   wrote $out/$base.s     (-O1, the annotated baseline)"
echo "   wrote $out/$base.O2.s  (-O2, optimizer showcase)"
echo "   now hand-write $out/$base.annotated.s from $out/$base.s"
