/* ===========================================================================
 * asm/demo.c — the dict HASH + the incremental-rehash INDEX STEP, distilled.
 * ===========================================================================
 *
 * This is a self-contained extraction of the two purest-logic routines at the
 * core of the Redis clone's hash table (see ../dict.c for the full versions):
 *
 *   1. dict_hash            — MurmurHash2-64A: the avalanche mixer that turns a
 *                             key's bytes into a 64-bit hash. This is what makes
 *                             buckets fill evenly; a seed defeats hash-flooding.
 *   2. rehash_target_index  — the ONE arithmetic step at the heart of
 *                             incremental rehashing: given a key's hash and the
 *                             two tables' bucket masks, pick which table a NEW
 *                             insert targets and compute its bucket. During a
 *                             resize (rehashidx >= 0) inserts go to table 1 so
 *                             table 0 only ever shrinks; otherwise table 0.
 *
 * It is written FREESTANDING so clang emits clean teaching assembly:
 *   - NO #include, NO system headers.
 *   - Its own integer types.
 *   - The 8-byte block load is assembled byte-by-byte (no memcpy), which also
 *     makes the little-endian byte order explicit in the emitted code.
 *
 * Generate the three assembly views (each must exit 0 and be non-empty):
 *
 *   clang --target=x86_64-pc-linux-gnu -S -O0 -fno-asynchronous-unwind-tables \
 *         -fno-jump-tables -fno-omit-frame-pointer demo.c -o demo.O0.s
 *   clang --target=x86_64-pc-linux-gnu -S -O1 -fno-asynchronous-unwind-tables \
 *         -fno-jump-tables -fno-omit-frame-pointer demo.c -o demo.s
 *   clang --target=x86_64-pc-linux-gnu -S -O2 -fno-asynchronous-unwind-tables \
 *         -fno-jump-tables demo.c -o demo.O2.s
 *
 * demo.annotated.s is hand-written from demo.s (-O1). Things to watch for there:
 *   - the 64-bit magic multiplier `m` loaded with `movabsq` (it is too wide for
 *     an immediate operand), and `imulq` doing the mixing multiplies;
 *   - the xor-shift avalanche (`shrq` + `xorq`) that diffuses high bits down;
 *   - the tail `switch (len & 7)` compiled WITHOUT a jump table (we pass
 *     -fno-jump-tables) into a compare-and-branch ladder — a byte-by-byte fold;
 *   - rehash_target_index collapsing the `?:` table choice into a single
 *     branchless conditional move (`cmovns`/`cmovs`) plus one `andq` (the
 *     power-of-two modulo `hash & mask`).
 * ===========================================================================
 */

/* Freestanding integer types (LP64 on the x86-64 Linux target). */
typedef unsigned char      u8;
typedef unsigned long long u64;    /* the 64-bit hash state                     */
typedef unsigned long      usize;  /* a bucket index / size                     */

/* ---------------------------------------------------------------------------
 * dict_hash — MurmurHash2, 64-bit ("64A"), by Austin Appleby.
 *
 * SysV AMD64 ABI: key in %rdi, len in %rsi, seed in %rdx; result in %rax.
 * Reads the key 8 bytes at a time, mixing each block through multiply +
 * xor-shift, then folds the 1..7 trailing bytes and does a final avalanche so a
 * one-bit input change flips ~half the output bits.
 * --------------------------------------------------------------------------- */
u64 dict_hash(const u8 *key, u64 len, u64 seed)
{
    const u64 m = 0xc6a4a7935bd1e995ULL;   /* Murmur's mixing multiplier         */
    const int r = 47;                       /* Murmur's mixing shift              */
    u64 h = seed ^ (len * m);               /* seed the state with the length     */

    u64 nblocks = len / 8;                  /* number of full 8-byte blocks       */
    for (u64 i = 0; i < nblocks; i++) {
        const u8 *p = key + i * 8;
        /* Assemble one LITTLE-ENDIAN 64-bit word from 8 bytes. Doing it by hand
         * keeps the result identical on any endianness and makes the byte order
         * visible in the asm; on x86-64 the optimizer may fuse it into one load. */
        u64 k = (u64)p[0]        | ((u64)p[1] << 8)  | ((u64)p[2] << 16) |
                ((u64)p[3] << 24) | ((u64)p[4] << 32) | ((u64)p[5] << 40) |
                ((u64)p[6] << 48) | ((u64)p[7] << 56);
        k *= m;         /* scramble the block                                     */
        k ^= k >> r;    /* fold the high bits down (avalanche)                    */
        k *= m;         /* scramble again                                         */
        h ^= k;         /* mix the block into the running hash                    */
        h *= m;         /* diffuse                                                */
    }

    /* Tail: the final len%8 bytes. A fallthrough switch, one byte per case; with
     * -fno-jump-tables this becomes a compare-and-branch ladder. */
    const u8 *tail = key + nblocks * 8;
    switch (len & 7) {
    case 7: h ^= (u64)tail[6] << 48; /* fallthrough */
    case 6: h ^= (u64)tail[5] << 40; /* fallthrough */
    case 5: h ^= (u64)tail[4] << 32; /* fallthrough */
    case 4: h ^= (u64)tail[3] << 24; /* fallthrough */
    case 3: h ^= (u64)tail[2] << 16; /* fallthrough */
    case 2: h ^= (u64)tail[1] << 8;  /* fallthrough */
    case 1: h ^= (u64)tail[0];
            h *= m;                  /* mix the assembled tail                    */
    }

    /* Final avalanche: guarantee the last bytes affect every output bit. */
    h ^= h >> r;
    h *= m;
    h ^= h >> r;
    return h;
}

/* ---------------------------------------------------------------------------
 * rehash_target_index — the incremental-rehash INDEX STEP in isolation.
 *
 * SysV AMD64 ABI: hash in %rdi, mask0 in %rsi, mask1 in %rdx, rehashidx in %rcx;
 * result (the bucket index) in %rax.
 *
 * A dict's tables always have a power-of-two bucket count, so the bucket for a
 * hash is `hash & (size - 1)` == `hash & mask` — one AND instead of a modulo.
 * While a resize is in progress (rehashidx >= 0) new entries must go into the
 * SECOND table (mask1); otherwise the first (mask0). The `?:` compiles to a
 * branchless conditional move, so this whole "which bucket, in which table"
 * decision is two or three instructions with no branch to mispredict.
 * --------------------------------------------------------------------------- */
usize rehash_target_index(u64 hash, u64 mask0, u64 mask1, long rehashidx)
{
    u64 mask = (rehashidx >= 0) ? mask1 : mask0;   /* pick the target table's mask*/
    return (usize)(hash & mask);                    /* power-of-two modulo         */
}
