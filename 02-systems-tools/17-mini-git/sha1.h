/* ===========================================================================
 * sha1.h — a tiny, SELF-CONTAINED SHA-1 implementation (public interface).
 * ===========================================================================
 *
 * WHY SHA-1 LIVES IN ITS OWN, HEADER-DECLARED, LIBC-FREE UNIT
 * ----------------------------------------------------------
 * git's object identity is a hash. The default is SHA-1 (a 160-bit / 20-byte
 * digest), computed NOT over the file bytes but over the string
 *
 *       "<type> <decimal-size>\0<content>"
 *
 * (see object.c). Everything git calls "content addressing" bottoms out in this
 * one function: same bytes in, same 20 bytes out, forever. That immutability is
 * what makes a commit a permanent snapshot — change one byte anywhere in the
 * tree and every enclosing hash changes.
 *
 * This translation unit is written to depend on NOTHING: it declares its own
 * fixed-width integer types below and #includes no system headers, so a bare
 * cross-compiler can turn sha1.c into x86-64 System V assembly with no libc or
 * kernel headers present. That is deliberate — it is one of this project's two
 * "real source file compiled to teaching assembly" deliverables (see asm/).
 *
 * SECURITY NOTE (read before reusing this anywhere real)
 * ------------------------------------------------------
 * SHA-1 is CRYPTOGRAPHICALLY BROKEN for collision resistance (the SHAttered
 * attack, 2017, produced two distinct PDFs with the same SHA-1). git mitigates
 * this in production with a hardened variant ("sha1dc") that detects the known
 * collision-attack byte patterns, and is migrating to SHA-256 as an alternate
 * object format. This file is the plain, textbook SHA-1 (RFC 3174) — perfect for
 * learning the round function and reading its assembly, NOT for security.
 * ===========================================================================
 */
#ifndef MYGIT_SHA1_H
#define MYGIT_SHA1_H

/* --- our own fixed-width types (NO <stdint.h>) ----------------------------
 * On every Linux x86-64 target these widths are exact: `int`/`unsigned` are 32
 * bits and `long long` is 64. We rely on the 32-bit wraparound of sha1_u32 for
 * SHA-1's modular addition, so the width MUST be exactly 32. */
typedef unsigned int       sha1_u32;   /* a 32-bit hash word (wraps mod 2^32) */
typedef unsigned long long sha1_u64;   /* the 64-bit message length, in bits  */
typedef unsigned long      sha1_size;  /* a byte count (size_t-shaped)        */

/* The streaming hash state. SHA-1 digests a message 512 bits (64 bytes) at a
 * time, so we buffer up to one partial block and remember the running 160-bit
 * state plus the total length (needed by the final padding step). */
typedef struct {
    sha1_u32      h[5];       /* the 160-bit running state, 5 * 32 bits        */
    sha1_u64      nbits;      /* total message length so far, in BITS          */
    unsigned char block[64];  /* partial 512-bit block being filled            */
    unsigned      blen;       /* bytes currently buffered in block[] (0..63)   */
} sha1_ctx;

/* init -> update* -> final is the standard streaming shape. `out` gets the raw
 * 20 digest bytes (big-endian), which is exactly what git stores inside trees. */
void sha1_init(sha1_ctx *c);
void sha1_update(sha1_ctx *c, const void *data, sha1_size len);
void sha1_final(sha1_ctx *c, unsigned char out[20]);

#endif /* MYGIT_SHA1_H */
