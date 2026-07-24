/* ===========================================================================
 * main.c — a tiny CLI around the decoder: a LINEAR-SWEEP disassembler.
 * ===========================================================================
 *
 * Linear sweep = "start at byte 0, decode one instruction, advance by its
 * length, repeat." It is what `objdump -d` does. Its weakness (the reason the
 * README's "going further" points at recursive descent) is that it cannot tell
 * code from data: if a jump table or an embedded constant sits in .text, linear
 * sweep will happily decode those bytes as instructions and desynchronize.
 *
 * Usage:
 *     disasm [--intel] [--att] [--base 0xADDR] HEXBYTES...
 *     disasm [--intel] [--att] [--base 0xADDR] -f FILE      # raw binary blob
 *     echo "48 89 e5" | disasm                              # hex on stdin
 *
 * HEXBYTES are whitespace/comma-separated pairs, e.g. "48 89 e5" or "4889e5".
 * We only read from files/args the user hands us; there is no network or exec.
 * ===========================================================================
 */
#include "disasm.h"
#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <ctype.h>

/* Parse a stream of hex text into a byte buffer. Accepts "48 89 e5", "4889e5",
 * commas, "0x" prefixes, and '#' line comments. Returns the byte count, or -1
 * on a malformed nibble. `out` must hold at least `cap` bytes. */
static long parse_hex(const char *text, uint8_t *out, size_t cap) {
    size_t n = 0;
    int hi = -1;                       /* the pending high nibble, or -1        */
    for (const char *s = text; *s; s++) {
        if (*s == '#') {               /* comment to end of line                */
            while (*s && *s != '\n') s++;
            if (!*s) break;
            continue;
        }
        if (*s=='0' && (s[1]=='x'||s[1]=='X')) { s++; continue; } /* skip 0x     */
        if (isspace((unsigned char)*s) || *s==',') continue;
        int v;
        if      (*s>='0'&&*s<='9') v = *s-'0';
        else if (*s>='a'&&*s<='f') v = *s-'a'+10;
        else if (*s>='A'&&*s<='F') v = *s-'A'+10;
        else return -1;                /* not a hex digit                       */
        if (hi < 0) hi = v;            /* first nibble of a byte                */
        else {
            if (n >= cap) return (long)n;
            out[n++] = (uint8_t)((hi << 4) | v);   /* combine nibble pair       */
            hi = -1;
        }
    }
    return (hi < 0) ? (long)n : -1;    /* an odd trailing nibble is an error    */
}

/* Read an entire file into a freshly-malloc'd buffer. Caller frees. */
static uint8_t *read_file(const char *path, size_t *len) {
    FILE *f = fopen(path, "rb");
    if (!f) { perror(path); return NULL; }
    if (fseek(f, 0, SEEK_END) != 0) { fclose(f); return NULL; }
    long sz = ftell(f);
    if (sz < 0) { fclose(f); return NULL; }
    rewind(f);
    uint8_t *buf = malloc((size_t)sz ? (size_t)sz : 1);
    if (!buf) { fclose(f); return NULL; }
    size_t got = fread(buf, 1, (size_t)sz, f);
    fclose(f);
    *len = got;
    return buf;
}

/* Emit one disassembly line in an objdump-ish layout:
 *   0000000000401000:  48 89 e5             mov    %rsp,%rbp   */
static void print_line(const Insn *in, Syntax syn) {
    char text[160];
    disasm_format(in, syn, text, sizeof text);

    printf("%016llx:  ", (unsigned long long)in->addr);

    /* raw bytes, padded so the mnemonic column lines up (max 15 bytes shown). */
    int shown = in->len > 15 ? 15 : in->len;
    for (int i = 0; i < shown; i++) printf("%02x ", in->raw[i]);
    for (int i = shown; i < 12; i++) printf("   ");   /* pad to a fixed column  */

    printf("%s\n", text);
}

int main(int argc, char **argv) {
    Syntax syn = SYN_ATT;              /* objdump's default, so easiest to diff  */
    uint64_t base = 0x401000;          /* a plausible .text load address         */
    const char *file = NULL;

    /* Collect non-flag args as hex text; handle the few flags. */
    char hexbuf[4096]; hexbuf[0] = 0;
    for (int i = 1; i < argc; i++) {
        if      (!strcmp(argv[i], "--intel")) syn = SYN_INTEL;
        else if (!strcmp(argv[i], "--att"))   syn = SYN_ATT;
        else if (!strcmp(argv[i], "--base") && i+1 < argc) base = strtoull(argv[++i], NULL, 0);
        else if (!strcmp(argv[i], "-f") && i+1 < argc)     file = argv[++i];
        else {
            strncat(hexbuf, argv[i], sizeof(hexbuf)-strlen(hexbuf)-2);
            strncat(hexbuf, " ",     sizeof(hexbuf)-strlen(hexbuf)-2);
        }
    }

    uint8_t  stackbuf[4096];
    uint8_t *code = stackbuf;
    size_t   n = 0;
    uint8_t *heap = NULL;

    if (file) {
        heap = read_file(file, &n);
        if (!heap) return 1;
        code = heap;
    } else {
        if (!hexbuf[0]) {              /* no args -> read hex from stdin         */
            size_t got = fread(hexbuf, 1, sizeof(hexbuf)-1, stdin);
            hexbuf[got] = 0;
        }
        long got = parse_hex(hexbuf, stackbuf, sizeof stackbuf);
        if (got < 0) { fprintf(stderr, "error: malformed hex input\n"); return 1; }
        n = (size_t)got;
    }

    /* The sweep: decode, print, advance by the decoded length, repeat. */
    size_t off = 0;
    while (off < n) {
        Insn in;
        int len = disasm_one(code + off, n - off, base + off, &in);
        print_line(&in, syn);
        if (len <= 0) len = 1;         /* never stall the sweep                  */
        off += (size_t)len;
    }

    free(heap);
    return 0;
}
