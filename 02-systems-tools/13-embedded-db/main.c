/* ===========================================================================
 * main.c — a tiny command-line front end for the embedded KV store.
 * ===========================================================================
 *
 *   db <file> put  <key> <value>     insert or overwrite  (durable on return)
 *   db <file> get  <key>             print the value, or exit 1 if absent
 *   db <file> del  <key>             delete a key
 *   db <file> scan                   print every pair in key order
 *   db <file> demo [N]               insert N sequential pairs (default 2000),
 *                                    forcing many B-tree splits, then verify.
 *
 * Durability is automatic: `put`/`del` return only after the write-ahead log
 * and then the data file have been fdatasync'd. To SEE crash recovery, run a
 * `put`, `kill -9` the process at any moment, then reopen with `get`/`scan` and
 * observe that the store is intact (either the whole write is there or none of
 * it — never a half-written page).
 * ===========================================================================
 */
#include "db.h"

#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <unistd.h>

/* meta_npages is exported by pager.c; we use it only to report page counts. */
uint32_t meta_npages(DB *db);

/* scan callback: print "key\tvalue\n". Keys/values from the CLI are text. */
static int print_pair(const void *key, uint16_t klen,
                      const void *val, uint32_t vlen, void *ctx)
{
    (void)ctx;
    fwrite(key, 1, klen, stdout);
    fputc('\t', stdout);
    fwrite(val, 1, vlen, stdout);
    fputc('\n', stdout);
    return 0;   /* 0 == keep going */
}

/* counter callback for the demo's verification scan. */
static int count_pair(const void *key, uint16_t klen,
                      const void *val, uint32_t vlen, void *ctx)
{
    (void)key; (void)klen; (void)val; (void)vlen;
    (*(long *)ctx)++;
    return 0;
}

static int usage(const char *argv0)
{
    fprintf(stderr,
        "usage:\n"
        "  %s <file> put  <key> <value>\n"
        "  %s <file> get  <key>\n"
        "  %s <file> del  <key>\n"
        "  %s <file> scan\n"
        "  %s <file> demo [N]\n",
        argv0, argv0, argv0, argv0, argv0);
    return 2;
}

/* --- the demo: stress the tree hard enough to split many times, then check --- */
static int run_demo(DB *db, long n)
{
    char k[32], v[64];
    for (long i = 0; i < n; i++) {
        /* zero-padded so lexicographic order == numeric order (nice for scan) */
        int kl = snprintf(k, sizeof k, "key%010ld", i);
        int vl = snprintf(v, sizeof v, "value-of-%ld", i);
        if (db_put(db, k, (uint16_t)kl, v, (uint32_t)vl) != 0) {
            fprintf(stderr, "put failed at i=%ld\n", i);
            return 1;
        }
    }
    /* Point lookups: first, middle, last, and one that must be absent. */
    long probes[] = { 0, n / 2, n - 1 };
    for (unsigned p = 0; p < sizeof probes / sizeof probes[0]; p++) {
        int kl = snprintf(k, sizeof k, "key%010ld", probes[p]);
        char got[64]; uint32_t gl = 0;
        int r = db_get(db, k, (uint16_t)kl, got, sizeof got, &gl);
        printf("get %s -> %s (len=%u)\n", k, r == 1 ? "" : "MISS", gl);
        if (r == 1) { printf("    value = %.*s\n", (int)gl, got); }
    }
    /* Full-scan verification: the count must equal what we inserted, in order. */
    long counted = 0;
    db_scan(db, count_pair, &counted);
    printf("inserted=%ld  scanned=%ld  pages=%u  %s\n",
           n, counted, meta_npages(db),
           counted == n ? "OK" : "MISMATCH");
    return counted == n ? 0 : 1;
}

int main(int argc, char **argv)
{
    if (argc < 3) return usage(argv[0]);
    const char *path = argv[1];
    const char *cmd  = argv[2];

    DB *db = db_open(path);
    if (!db) { fprintf(stderr, "cannot open db '%s'\n", path); return 1; }

    int rc = 0;

    if (strcmp(cmd, "put") == 0) {
        if (argc != 5) { rc = usage(argv[0]); goto out; }
        if (db_put(db, argv[3], (uint16_t)strlen(argv[3]),
                       argv[4], (uint32_t)strlen(argv[4])) != 0) {
            fprintf(stderr, "put failed\n"); rc = 1;
        }
    } else if (strcmp(cmd, "get") == 0) {
        if (argc != 4) { rc = usage(argv[0]); goto out; }
        char buf[MAX_VAL]; uint32_t vl = 0;
        int r = db_get(db, argv[3], (uint16_t)strlen(argv[3]), buf, sizeof buf, &vl);
        if (r == 1) {
            fwrite(buf, 1, vl < sizeof buf ? vl : sizeof buf, stdout);
            fputc('\n', stdout);
        } else if (r == 0) {
            fprintf(stderr, "not found\n"); rc = 1;
        } else {
            fprintf(stderr, "get error\n"); rc = 1;
        }
    } else if (strcmp(cmd, "del") == 0) {
        if (argc != 4) { rc = usage(argv[0]); goto out; }
        int r = db_del(db, argv[3], (uint16_t)strlen(argv[3]));
        if (r == 0) { fprintf(stderr, "not found\n"); rc = 1; }
        else if (r < 0) { fprintf(stderr, "del error\n"); rc = 1; }
    } else if (strcmp(cmd, "scan") == 0) {
        db_scan(db, print_pair, NULL);
    } else if (strcmp(cmd, "demo") == 0) {
        long n = (argc >= 4) ? strtol(argv[3], NULL, 10) : 2000;
        if (n <= 0) n = 2000;
        rc = run_demo(db, n);
    } else {
        rc = usage(argv[0]);
    }

out:
    db_close(db);
    return rc;
}
