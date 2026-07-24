/* ===========================================================================
 * util.c — process helpers: secure randomness and base64 key encoding.
 * ===========================================================================
 * Kept apart from the crypto core because these touch the OS (getrandom) and are
 * pure I/O convenience, not part of the protocol.
 * =========================================================================== */

#include "wg.h"

#include <errno.h>
#include <sys/random.h>   /* getrandom(2)                                       */

/* ---------------------------------------------------------------------------
 * rng_bytes — fill a buffer with kernel CSPRNG bytes via getrandom(2).
 *
 * getrandom(buf, len, flags) [syscall 318 on x86-64] draws from the same pool as
 * /dev/urandom but without needing an open fd, and (with flags=0) BLOCKS until
 * the pool is initialized at early boot — after that it never blocks. It can
 * return fewer bytes than asked (a short read, e.g. if interrupted), so we loop;
 * EINTR (interrupted by a signal) is retried, any other error is fatal because
 * proceeding with predictable "random" ephemeral keys would break all security.
 * --------------------------------------------------------------------------- */
int rng_bytes(void *buf, usize n)
{
    u8 *p = (u8 *)buf;
    usize got = 0;
    while (got < n) {
        ssize_t r = getrandom(p + got, n - got, 0);
        if (r < 0) {
            if (errno == EINTR) continue;   /* signal arrived mid-call; retry     */
            return -1;                      /* ENOSYS on ancient kernels, etc.    */
        }
        got += (usize)r;
    }
    return 0;
}

/* ---------------------------------------------------------------------------
 * base64 — the WireGuard key encoding (standard alphabet, '=' padding). 32-byte
 * keys become 44-char strings. Small and self-contained; no OpenSSL dependency.
 * --------------------------------------------------------------------------- */
static const char B64[] =
    "ABCDEFGHIJKLMNOPQRSTUVWXYZabcdefghijklmnopqrstuvwxyz0123456789+/";

void base64_encode(char *out, const u8 *in, usize n)
{
    usize o = 0, i = 0;
    /* Consume 3 input bytes -> 4 output chars at a time. */
    while (i + 3 <= n) {
        u32 v = ((u32)in[i] << 16) | ((u32)in[i + 1] << 8) | in[i + 2];
        out[o++] = B64[(v >> 18) & 63];
        out[o++] = B64[(v >> 12) & 63];
        out[o++] = B64[(v >> 6) & 63];
        out[o++] = B64[v & 63];
        i += 3;
    }
    /* Tail: 1 or 2 leftover bytes get padded with '='. */
    if (n - i == 1) {
        u32 v = (u32)in[i] << 16;
        out[o++] = B64[(v >> 18) & 63];
        out[o++] = B64[(v >> 12) & 63];
        out[o++] = '=';
        out[o++] = '=';
    } else if (n - i == 2) {
        u32 v = ((u32)in[i] << 16) | ((u32)in[i + 1] << 8);
        out[o++] = B64[(v >> 18) & 63];
        out[o++] = B64[(v >> 12) & 63];
        out[o++] = B64[(v >> 6) & 63];
        out[o++] = '=';
    }
    out[o] = '\0';
}

/* Map a base64 character back to its 6-bit value, or -1 if not a base64 char. */
static int b64val(char c)
{
    if (c >= 'A' && c <= 'Z') return c - 'A';
    if (c >= 'a' && c <= 'z') return c - 'a' + 26;
    if (c >= '0' && c <= '9') return c - '0' + 52;
    if (c == '+') return 62;
    if (c == '/') return 63;
    return -1;
}

int base64_decode(u8 *out, usize out_len, const char *in)
{
    usize o = 0;
    u32 acc = 0;
    int bits = 0;
    for (const char *p = in; *p && *p != '='; p++) {
        int v = b64val(*p);
        if (v < 0) return -1;               /* stray character -> malformed       */
        acc = (acc << 6) | (u32)v;
        bits += 6;
        if (bits >= 8) {                    /* enough bits for a whole byte       */
            bits -= 8;
            if (o >= out_len) return -1;    /* input encodes more than we expect  */
            out[o++] = (u8)((acc >> bits) & 0xff);
        }
    }
    return (o == out_len) ? 0 : -1;         /* must fill EXACTLY out_len bytes     */
}
