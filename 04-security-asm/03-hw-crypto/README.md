# Crypto with hardware instructions 🟧

**What it is.** One small program that computes the same cryptography three ways
and proves all three agree with the official NIST vectors:

- **AES-128 / AES-256 with AES-NI** — the CPU's `aesenc` / `aesenclast` do a full
  AES round each, and `aeskeygenassist` drives the key schedule.
- **SHA-256 with the SHA extensions** — `sha256rnds2` / `sha256msg1` /
  `sha256msg2` fold two rounds and four schedule words at a time.
- **A constant-time, table-free *software* AES** — the fallback for CPUs without
  AES-NI, written specifically to *not* leak the key through cache timing. This
  is the didactic heart of the project.

Which path runs is decided at **runtime by `cpuid`**. The point is not just speed;
it is the side-channel lesson: the "fast" textbook software AES uses secret-indexed
lookup tables and is broken by cache-timing attacks, whereas both AES-NI and the
constant-time software path here compute the S-box with **no secret-dependent
memory access at all.**

> **On your own machines only.** This is a learning lab. The "attack" it teaches
> about — cache-timing key recovery — is discussed so you can *defend* against it.
> Build and run this on hardware you own. The takeaway is a defensive coding
> discipline (constant-time crypto), not an exploit.

## What you'll learn

- **AES-NI**: `aesenc`, `aesenclast`, `aesdec`, `aesdeclast`, `aeskeygenassist`
  (SubWord+RotWord+Rcon in one op), and `aesimc` (InvMixColumns, to turn an
  encryption schedule into the decryption schedule for the equivalent inverse
  cipher). One instruction = one AES round, in hardware, in ~4 cycles.
- **SHA-NI**: how `sha256rnds2`/`msg1`/`msg2` collapse SHA-256's 64-round inner
  loop, and the state re-ordering ({A,B,E,F},{C,D,G,H}) and big-endian byte
  shuffles the instructions demand.
- **`cpuid`**: leaf `1` ECX (AES-NI bit 25, SSE4.1 bit 19, SSSE3 bit 9) and leaf
  `7` EBX (SHA bit 29), guarded by the max-leaf check. This is exactly how
  OpenSSL/BoringSSL/glibc pick an implementation at load time.
- **Constant-time coding**: multiplying in GF(2^8) branch-free, computing the AES
  S-box as `Affine(x^{-1})` with `x^{-1}=x^254` (square-and-multiply on a *public*
  exponent) instead of a table, and comparing secrets with no early-out
  (`ct_memeq`). The generated assembly shows `sete`/`neg`/`sbb`/`cmov` — no
  secret-dependent branches.
- **Side-channel awareness**: *why* `sbox[state ^ key]` leaks and how to avoid it.

## Build & run (Linux / WSL / macOS, x86-64)

```bash
make            # builds ./hwcrypto with per-file ISA flags
make test       # runs the NIST known-answer suite; exit code = # of failures
```

Per-file flags matter: only `aes_ni.c` is compiled with `-maes -msse4.1` and only
`sha256_ni.c` with `-msha -mssse3 -msse4.1`. The rest is baseline x86-64. The
program is safe to run even on a CPU that lacks these instructions, because it
only *calls* the hardware routines after `cpuid` confirms them — otherwise it
uses the constant-time software AES and software SHA-256.

Example run (on a CPU with AES-NI but no SHA extensions — note the honest skip):

```
=== CPU crypto feature detection (cpuid) ===
  AES-NI : yes
  SHA    : no
=== AES known-answer tests (FIPS-197) ===
  [PASS] CT-soft encrypt == NIST
  [PASS] AES-NI encrypt == NIST
  [PASS] AES-NI == CT-soft (encrypt)
  ...
=== summary: 0 check(s) failed ===
```

Regenerate the teaching assembly (works on any host; clang cross-targets Linux):

```bash
make asm
```

## How it works

- **`cpuid.h`** — `cpuid_raw()` issues the instruction (with the x86-64 `=b`
  caveat explained); `hwc_detect()` reads leaves 1 and 7 into a feature struct.
- **`aes.h` / `aes_ni.c`** — the AES-NI back end. `aeskeygenassist` + Intel's
  shift-and-XOR "assist" build the AES-128 and AES-256 schedules; `aesenc` ×
  (rounds−1) then `aesenclast` do the block; `aesimc` derives the decrypt schedule.
- **`aes_ct.c`** — the constant-time software AES. `gf_mul` is a branch-free
  Russian-peasant multiply (masks, not `if`); `gf_inv` raises to the 254th power
  by square-and-multiply on a public exponent; `sbox` = affine map of that
  inverse. No tables, no secret branches — the whole reason the file exists.
- **`sha256.h` / `sha256.c`** — the padding + length-encoding driver, shared by
  both transforms via a function pointer.
- **`sha256_soft.c`** — the plain 32-bit reference transform (fallback + oracle).
- **`sha256_ni.c`** — the SHA-extension transform (canonical Gulley/Walton form),
  heavily commented on the state-layout and endianness shuffles.
- **`main.c`** — detects features, runs the FIPS-197 (AES) and FIPS-180 (SHA-256)
  known-answer vectors through every available path, cross-checks hardware ==
  software, and demonstrates the constant-time comparison.

### The defense lesson (blue-team half)

The classic fast software AES precomputes `Te0..Te3` / `Td0..Td3` T-tables and
does `out = Te0[b0] ^ Te1[b1] ^ ...` where the indices are **secret** (key ⊕
plaintext). Which cache line each lookup touches is therefore a function of the
key, and a co-resident attacker can recover it via **PRIME+PROBE / FLUSH+RELOAD**
cache-timing (Bernstein 2005; Osvik–Shamir–Tromer 2006). Two defenses, both shown
here:

1. **Use the hardware instruction.** AES-NI/SHA-NI compute the S-box in silicon
   with fixed latency and *no* table in the cache to probe. Prefer it when `cpuid`
   says it exists.
2. **When you can't, be constant-time.** `aes_ct.c` computes the S-box
   arithmetically and never indexes memory with a secret or branches on one. It
   is slower — that is the price — but its timing is independent of the key. The
   same discipline appears in `ct_memeq` (compare MAC/tags without an early return
   that would leak how many bytes matched).

## Assembly notes

Two hand-annotated files, both generated from genuine clang output:

- **`asm/aes_round.annotated.s`** — the AES-NI round the brief asks us to
  annotate. It walks `pxor` (whitening) → nine `aesenc` → `aesenclast` → store,
  with the **real opcode bytes** from `objdump` (e.g. `aesenc` = `66 0F 38 DC /r`)
  and the detail that the round-key displacement encodes as `disp8` until it
  exceeds a signed byte (offset `0x80`), then becomes `disp32`.
- **`asm/demo.annotated.s`** — the constant-time comparison/select from
  `asm/demo.c`. At `-O1` clang turns the `ct_eq` idiom into `cmp; sete; neg` and
  `ct_memeq` into an OR-fold plus `sbb` — **branchless**. The only conditional
  jump is on the *public* length. Compare `demo.O0.s` (literal) and `demo.O2.s`
  (unrolled/vectorized, still branch-free on the data).

Regenerate the raw `.s` with `make asm`; the `*.annotated.s` files are authored
and not overwritten. (Regenerating `aes_round.*` on a host without Linux headers
needs a one-line stub `stdlib.h` on `-isystem`; see the Makefile comment.)

## Going further

- **AES-GCM with `pclmulqdq`.** This project stops at the raw block cipher (ECB of
  one block, for known-answer testing). Real code never exposes raw ECB — it uses
  an authenticated mode. The GHASH of GCM is a carryless multiply in GF(2^128),
  which `pclmulqdq` accelerates the way AES-NI accelerates the cipher.
- **Bitsliced constant-time AES.** Our arithmetic S-box is simple and obviously
  constant-time but slow. A bitsliced AES (BearSSL `aes_ct`, Käsper–Schwabe)
  processes several blocks in parallel across the bits of a word and is both
  constant-time *and* fast — the technique used when AES-NI is unavailable.
- **VAES / AVX-512.** `vaesenc` on 256-/512-bit registers does 2–4 AES blocks per
  instruction; the wide SHA story is `sha512` on newer parts.
- **Verify constant-timeness.** Run `aes_ct.c` under `valgrind --tool=cachegrind`
  or `dudect` and confirm the timing distribution does not depend on the key.

## References

- FIPS-197 (AES) and FIPS-180-4 (SHA-2) — the standards and their known-answer
  vectors used verbatim in `main.c`.
- Shay Gueron, *Intel Advanced Encryption Standard (AES) New Instructions Set* —
  the canonical AES-NI key-expansion and cipher code.
- S. Gulley, V. Gopal, et al., *Intel SHA Extensions* white paper; Jeffrey
  Walton's public-domain `sha256_process_x86` — the SHA-NI structure here.
- D. J. Bernstein, *Cache-timing attacks on AES* (2005); Osvik, Shamir, Tromer,
  *Cache Attacks and Countermeasures* (2006) — the threat `aes_ct.c` defends.
- Intel SDM Vol. 2 — `CPUID`, `AESENC`, `SHA256RNDS2` instruction references.
- BearSSL `aes_ct.c` / `aes_ct64.c` (Thomas Pornin) — a production constant-time AES.
