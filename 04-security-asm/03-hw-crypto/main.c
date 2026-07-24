/* ===========================================================================
 * main.c — detect CPU crypto features, then verify every back end against the
 *          official NIST vectors and cross-check the paths against each other.
 * ===========================================================================
 *
 * This is the test harness the CONVENTIONS ask for ("verify every
 * implementation against the official NIST test vectors"). It:
 *
 *   1. runs `cpuid` to see what the CPU supports (aes_ni.c / sha256_ni.c are
 *      only *called* when their instructions exist — otherwise it is SIGILL);
 *   2. encrypts/decrypts the FIPS-197 AES-128 and AES-256 known-answer vectors
 *      with the constant-time software path (always) and the AES-NI path (when
 *      present), and checks byte-for-byte;
 *   3. hashes the FIPS-180 SHA-256 vectors with the software and SHA-NI paths;
 *   4. cross-checks hardware == software (a mismatch would mean WE got the
 *      hardware permutations wrong);
 *   5. demonstrates the branch-free constant-time comparison from asm/demo.c.
 *
 * Exit status is the number of failed checks (0 = all good), so `make test`
 * can gate on it.
 * ===========================================================================
 */
#include <stdio.h>
#include <string.h>
#include <stdint.h>

#include "cpuid.h"
#include "aes.h"
#include "sha256.h"

/* ------------------------------ small helpers ---------------------------- */

/* Parse a hex string (no spaces) into `out`; returns the number of bytes. */
static size_t unhex(const char *hex, uint8_t *out)
{
    size_t n = 0;
    for (const char *p = hex; p[0] && p[1]; p += 2) {
        unsigned hi, lo;
        sscanf(p,     "%1x", &hi);
        sscanf(p + 1, "%1x", &lo);
        out[n++] = (uint8_t)((hi << 4) | lo);
    }
    return n;
}

static void print_hex(const uint8_t *b, size_t n)
{
    for (size_t i = 0; i < n; i++) printf("%02x", b[i]);
}

/* Global pass/fail tally; each check bumps one. */
static int g_fail = 0;

/* Compare `got` to `want`; print a one-line PASS/FAIL with the label. */
static void check(const char *label, const uint8_t *got, const uint8_t *want, size_t n)
{
    int ok = memcmp(got, want, n) == 0;
    if (!ok) g_fail++;
    printf("  [%s] %-34s ", ok ? "PASS" : "FAIL", label);
    if (!ok) {
        printf("\n        got  "); print_hex(got, n);
        printf("\n        want "); print_hex(want, n);
    }
    printf("\n");
}

/* =========================================================================
 * Constant-time comparison, mirrored from asm/demo.c so we can demonstrate the
 * branch-free behavior live. `ct_eq` returns 0xFFFFFFFF when equal, else 0 —
 * with NO early-out, so its timing does not leak *where* two buffers differ
 * (the property a MAC/tag check needs to resist timing oracles).
 * ========================================================================= */
static uint32_t ct_eq_u32(uint32_t x, uint32_t y)
{
    uint32_t q  = x ^ y;                 /* 0 iff equal                        */
    uint32_t nz = (q | (0u - q)) >> 31;  /* 1 if any bit differs, else 0       */
    return nz - 1u;                      /* 0xFFFFFFFF if equal, else 0        */
}
static uint32_t ct_memeq(const uint8_t *a, const uint8_t *b, size_t n)
{
    uint8_t diff = 0;
    for (size_t i = 0; i < n; i++) diff |= (uint8_t)(a[i] ^ b[i]); /* fold ALL bytes */
    return ct_eq_u32(diff, 0);
}

/* ------------------------------ AES testing ------------------------------ */

/* Run one AES known-answer test through whichever back end the fn pointers name.
 * `have_ni` selects whether the AES-NI columns run. */
static void test_aes_kat(const char *name, int key_bits, int have_ni,
                         const char *key_hex, const char *pt_hex, const char *ct_hex)
{
    uint8_t key[32], pt[16], ct[16], out[16], back[16];
    unhex(key_hex, key);
    unhex(pt_hex, pt);
    unhex(ct_hex, ct);

    printf("%s (key=%d bits)\n", name, key_bits);

    /* --- constant-time software path (always available) --- */
    aes_key sk;
    aes_ct_expand(&sk, key, key_bits);
    aes_ct_encrypt(&sk, pt, out);
    check("CT-soft encrypt == NIST", out, ct, 16);
    aes_ct_decrypt(&sk, ct, back);
    check("CT-soft decrypt == plaintext", back, pt, 16);

    /* --- AES-NI path (only if the CPU has it) --- */
    if (have_ni) {
        aes_key hk;
        uint8_t nout[16], nback[16];
        aes_ni_expand(&hk, key, key_bits);
        aes_ni_encrypt(&hk, pt, nout);
        check("AES-NI encrypt == NIST", nout, ct, 16);
        aes_ni_decrypt(&hk, ct, nback);
        check("AES-NI decrypt == plaintext", nback, pt, 16);
        /* Cross-check: the two independent implementations must AGREE. */
        check("AES-NI == CT-soft (encrypt)", nout, out, 16);
    } else {
        printf("  [skip] AES-NI not present on this CPU\n");
    }
    printf("\n");
}

/* ----------------------------- SHA-256 testing --------------------------- */

static void test_sha_kat(const char *msg, size_t len, int have_ni, const char *digest_hex)
{
    uint8_t want[32], got[32], nih[32];
    unhex(digest_hex, want);

    printf("SHA-256(\"%.*s\"%s)\n", (int)(len > 16 ? 16 : len), msg, len > 16 ? "..." : "");

    sha256_soft((const uint8_t *)msg, len, got);
    check("software == NIST", got, want, 32);

    if (have_ni) {
        sha256_ni((const uint8_t *)msg, len, nih);
        check("SHA-NI == NIST", nih, want, 32);
        check("SHA-NI == software", nih, got, 32);
    } else {
        printf("  [skip] SHA-NI not present on this CPU\n");
    }
    printf("\n");
}

int main(void)
{
    hwc_features f = hwc_detect();

    printf("=== CPU crypto feature detection (cpuid) ===\n");
    printf("  AES-NI : %s\n", f.aesni ? "yes" : "no");
    printf("  SSSE3  : %s\n", f.ssse3 ? "yes" : "no");
    printf("  SSE4.1 : %s\n", f.sse41 ? "yes" : "no");
    printf("  SHA    : %s\n", f.sha   ? "yes" : "no");
    printf("\n");

    /* SHA-NI needs SSSE3 (pshufb/palignr) and SSE4.1 (pblendw) alongside the SHA
     * instructions themselves; guard on all three before touching that path. */
    int have_sha_path = f.sha && f.ssse3 && f.sse41;

    printf("=== AES known-answer tests (FIPS-197) ===\n");
    /* FIPS-197 Appendix C.1 (AES-128) and C.3 (AES-256): the canonical vectors. */
    test_aes_kat("AES-128", 128, f.aesni,
        "000102030405060708090a0b0c0d0e0f",
        "00112233445566778899aabbccddeeff",
        "69c4e0d86a7b0430d8cdb78070b4c55a");
    test_aes_kat("AES-256", 256, f.aesni,
        "000102030405060708090a0b0c0d0e0f101112131415161718191a1b1c1d1e1f",
        "00112233445566778899aabbccddeeff",
        "8ea2b7ca516745bfeafc49904b496089");

    printf("=== SHA-256 known-answer tests (FIPS-180) ===\n");
    /* NIST example vectors: "abc", the empty string, and the 56-byte message. */
    test_sha_kat("abc", 3, have_sha_path,
        "ba7816bf8f01cfea414140de5dae2223b00361a396177a9cb410ff61f20015ad");
    test_sha_kat("", 0, have_sha_path,
        "e3b0c44298fc1c149afbf4c8996fb92427ae41e4649b934ca495991b7852b855");
    test_sha_kat("abcdbcdecdefdefgefghfghighijhijkijkljklmklmnlmnomnopnopq", 56, have_sha_path,
        "248d6a61d20638b8e5c026930c3e6039a33ce45964ff2167f6ecedd419db06c1");

    printf("=== constant-time comparison demo (asm/demo.c) ===\n");
    {
        uint8_t a[16], b[16];
        memset(a, 0xA5, sizeof a);
        memcpy(b, a, sizeof b);
        printf("  equal buffers   -> ct_memeq = 0x%08x (expect 0xffffffff)\n", ct_memeq(a, b, 16));
        b[7] ^= 0x01;  /* flip one bit deep in the buffer */
        printf("  1-bit different -> ct_memeq = 0x%08x (expect 0x00000000)\n", ct_memeq(a, b, 16));
        printf("  (no early return: the loop touches all 16 bytes either way)\n\n");
    }

    printf("=== summary: %d check(s) failed ===\n", g_fail);
    return g_fail;   /* 0 on full success — `make test` gates on this */
}
