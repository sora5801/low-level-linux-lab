# RCU-protected hash table 🟥

**What it is.** A hash table living in the Linux kernel whose **readers take no
lock at all**. Lookups run inside `rcu_read_lock()`/`rcu_read_unlock()`, walk a
bucket chain with `hlist_for_each_entry_rcu()`, and read a value through
`rcu_dereference()` — no atomics, no cache-line ping-pong, no blocking. Writers
serialize on an ordinary spinlock, publish new nodes/values with
`rcu_assign_pointer()`, and defer every free to after a **grace period** with
`kfree_rcu()`, `call_rcu()`, and `synchronize_rcu()`. Two writer kthreads and
three reader kthreads then race on a deliberately tiny keyspace to prove the
discipline holds under real concurrency.

This is the pattern behind the kernel's dcache, the route cache, netfilter
conntrack, and countless read-mostly tables. It is tagged 🟥 because getting the
memory ordering and the reclaim lifecycle exactly right is genuinely hard, and
getting it *subtly* wrong produces a use-after-free that surfaces once a year on
one weakly-ordered machine.

> **Teaching-core honesty.** This is a complete, correct, load-it-and-watch-it
> demonstration of the RCU *mechanics* on a **fixed-size** table (256 buckets,
> single writer lock). It does **not** implement rhashtable-style online resizing
> / rehashing, per-bucket locks, or sleepable (SRCU) readers. Those are called
> out in **Going further**. The point is to make `rcu_assign_pointer` /
> `rcu_dereference` / grace periods legible, not to replace `linux/rhashtable.h`.

## What you'll learn

- **The RCU read side**: `rcu_read_lock` / `rcu_read_unlock`,
  `hlist_for_each_entry_rcu`, `rcu_dereference` — and the hard rule that you may
  **not sleep** inside a classic-RCU read section.
- **The RCU write side**: `rcu_assign_pointer` (store-release **publish**),
  `RCU_INIT_POINTER` (the barrier-free init for not-yet-published pointers),
  `hlist_add_head_rcu` / `hlist_del_rcu`, and `rcu_dereference_protected` for
  reads made safe by a held lock instead of a read section.
- **Deferred reclamation three ways**: `kfree_rcu` (defer a plain free),
  `call_rcu` (defer a custom callback), and `synchronize_rcu` (block until one
  grace period passes) — and *why each is the right tool where it is used*.
- **`rcu_barrier` at module unload** — the step whose omission is a textbook
  use-after-free when a `.ko` is removed while its `call_rcu` callbacks are still
  queued.
- **Publish/consume memory ordering**: *why* the barriers in
  `rcu_assign_pointer`/`rcu_dereference` exist and exactly which reordering they
  forbid.

## Build & run (Linux, **inside a QEMU/VM only**)

Kernel code oopses instead of segfaulting, so never load an experimental module
on your host. Use a throwaway VM.

```bash
# On a Linux box with kernel headers installed (linux-headers-$(uname -r)):
make                     # builds rcu_hashtable.ko via Kbuild

# Inside a disposable VM:
sudo insmod rcu_hashtable.ko     # starts 2 writer + 3 reader kthreads
sudo dmesg -w                    # watch: init line; kthreads race silently
sudo rmmod rcu_hashtable         # triggers ht_destroy + rcu_barrier
sudo dmesg | tail                # exit line with hit/miss/insert/delete counts
```

Expected `dmesg` shape (counts vary run to run):

```
rcu_hashtable: init: 256 buckets, 3 reader + 2 writer kthreads
rcu_hashtable: ht_destroy: freed 41 node(s) after one grace period
rcu_hashtable: exit: lookups hit=91833 miss=too-few-shown, inserts=..., deletes=...
```

The real test is **silence**: build your VM kernel with `CONFIG_PROVE_RCU=y`,
`CONFIG_DEBUG_OBJECTS_RCU_HEAD=y`, and `CONFIG_KASAN=y`, and the stress loop must
run indefinitely with **no** lockdep splat, no RCU-stall warning, and no KASAN
use-after-free. That silence is the proof the ordering is correct.

Regenerate the teaching assembly on any host (no Linux needed):

```bash
make asm                 # writes asm/demo.{O0.s,s,O2.s}
```

## How it works

### `rcu_hashtable.c` — the module (read top to bottom)

The file header is a full essay on publish/consume; the code then builds up:

- **`struct ht_value`** — the RCU-protected payload, separated from the node so
  that *updating* a key is a pointer swap (`rcu_assign_pointer`) with the old
  value defer-freed. Carries its own `rcu_head` for `kfree_rcu`.
- **`struct ht_node`** — one bucket entry: an `hlist_node` link, the key, a
  `struct ht_value __rcu *value` (the `__rcu` makes sparse *enforce* that you only
  dereference it with the RCU accessors), and an `rcu_head` for `call_rcu`.
- **`ht_bucket()`** — the pure-logic hash: `key * GOLDEN_RATIO_32`, an xor-shift
  avalanche, then `& HT_MASK`. No kernel API, no barriers — this is the routine
  extracted into `asm/demo.c`.
- **`ht_lookup()`** — the lock-free reader. `rcu_read_lock`,
  `hlist_for_each_entry_rcu`, `rcu_dereference(n->value)`, read, `rcu_read_unlock`.
- **`ht_insert()`** — writer. Allocates *before* taking the spinlock (you may not
  sleep holding it), then either `rcu_assign_pointer`s a replacement value
  (`kfree_rcu` the old one) or `RCU_INIT_POINTER` + `hlist_add_head_rcu` for a
  brand-new node.
- **`ht_delete()`** — writer. `hlist_del_rcu` under the lock, then
  `call_rcu(&node->rcu, ht_node_reclaim)` to free after a grace period.
- **`ht_node_reclaim()`** — the `call_rcu` callback; frees value + node once no
  reader can hold them. Its address lives in module `.text`, which is exactly
  why unload must `rcu_barrier`.
- **`ht_destroy()`** — teardown. Unlinks every node, records them in a pointer
  array (crucially **not** by reusing the RCU list linkage — see the code comment
  on why that would corrupt an in-flight reader), does **one** `synchronize_rcu`,
  then frees. Called while readers are still live, so the barrier is real.
- **init / exit** — start kthreads; on exit, stop **writers**, `ht_destroy`
  (readers still racing → `synchronize_rcu` matters), stop readers, then
  `rcu_barrier`.

### Why the barriers matter (the one thing to remember)

Linking an initialized object is two stores: `newv->data = 42;` then
`rcu_assign_pointer(n->value, newv);`. Nothing automatically forces the first to
be globally visible before the second. `rcu_assign_pointer` is a **store-release**
(the *publish* barrier): any CPU that later observes the new pointer is
guaranteed to also see the initialized `data`. `rcu_dereference` on the reader is
the **consume** side: it orders the load of `*pointer` after the load of
`pointer`, so a reader that sees the node sees the finished object, never a
half-built one. Use a plain load and it works for years, then corrupts memory
once, under load, on a Power box. That is the whole reason RCU has its own
accessors.

## Assembly notes

Kernel C cannot be compiled to assembly on this host — `rcu_hashtable.c`
`#include`s `<linux/rcupdate.h>` and friends, which only exist in a configured
kernel tree. Per the repo convention, the project's most instructive **pure
logic** — the key→bucket hash — is extracted into a self-contained
[`asm/demo.c`](asm/demo.c) (it declares its own `u32`, includes nothing) and
*that* is compiled to real assembly with the exact commands in `make asm`.

See [`asm/demo.annotated.s`](asm/demo.annotated.s) for the line-by-line
walkthrough. Highlights the generated code teaches:

- `ht_bucket` is **`imul` (scatter) → `mov`/`shr`/`xor` (avalanche) → `movzbl`
  (mask)**: branch-free, memory-free, division-free. The power-of-two table size
  is what turns the index into a one-cycle `and`/`movzbl` instead of a `div`.
- A real optimizer lesson in `ht_bucket_mask`: clang rewrites `key * 0x61C88647
  & 0xFF` into `key * 71`, because masking the *output* to 8 bits means only the
  low 8 bits of the constant (`0x47` = 71) can affect the result.
- `main` constant-folds the whole `ht_bucket(0..7)` loop to the literal `164`.
- **The RCU point the asm makes by omission**: there is not a single memory
  barrier in any of these functions, because hashing touches no shared memory.
  RCU's publish/consume ordering wraps the *bucket and node loads* that consume
  this index in `rcu_hashtable.c` — never the arithmetic. Hashing is math; RCU
  safety is about the loads and stores that come after you have the bucket.

Compare the three levels: [`asm/demo.O0.s`](asm/demo.O0.s) (naive, everything
spilled, the `main` loop actually emitted), [`asm/demo.s`](asm/demo.s) (`-O1`,
the annotated baseline), [`asm/demo.O2.s`](asm/demo.O2.s) (`-O2`, frame pointers
gone, pure compute + `ret`).

## Going further (the `Stretch:` from the list)

- **Resizable table.** The fixed 256 buckets are this core's honest limit. The
  production answer is `include/linux/rhashtable.h`: RCU-safe **online rehashing**
  where a lookup may have to search *both* the old and new bucket arrays during a
  resize. Implementing even a two-table hand-off with RCU is the natural next
  step.
- **Per-bucket locks.** One writer spinlock serializes *all* writers. Real tables
  put a lock (or a `bit_spinlock`) per bucket so writers to different buckets
  proceed in parallel; readers still take nothing.
- **Sleepable readers (SRCU).** Classic RCU forbids sleeping in the read section.
  If a reader must sleep (e.g. call into something that blocks), `srcu_read_lock`
  / `synchronize_srcu` lift that restriction at the cost of an explicit SRCU
  domain and heavier read-side markers.
- **Measure it.** Add a `perf`/`rcutorture`-style harness and compare read
  throughput vs. a plain `spinlock`- or `rwlock`-guarded table as CPUs scale —
  the read-side scalability is the entire reason RCU exists.

## References

- `Documentation/RCU/` in the kernel tree — especially `whatisRCU.rst`,
  `rcu_dereference.rst`, and `listRCU.rst`. Ground truth.
- `include/linux/rcupdate.h`, `include/linux/rculist.h` — the accessors and
  `hlist_*_rcu` helpers this module uses.
- `include/linux/rhashtable.h` + `lib/rhashtable.c` — the real, resizable,
  production RCU hash table.
- Paul McKenney, *"What is RCU, Fundamentally?"* (LWN) and *Is Parallel
  Programming Hard, And, If So, What Can You Do About It?* — the definitive
  treatment of grace periods and publish/subscribe.
- `man 9 rcu_read_lock`, `man 9 call_rcu`, `man 9 synchronize_rcu` (kernel-doc).
