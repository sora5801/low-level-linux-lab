# A tiny in-kernel filesystem (tinyfs) 🟥

**What it is.** `tinyfs` is a complete, mountable Linux filesystem whose entire
contents live in RAM — a heavily re-commented cousin of the kernel's own
`fs/ramfs`. You register it, `mount -t tinyfs`, then create files and
directories, write and read small files, and make symlinks; it all works, but
nothing is ever written to a disk, and unmounting throws the data away. The
point of the exercise is to isolate the **VFS glue** — the exact set of
`file_system_type`, `super_operations`, `inode_operations`, `file_operations`,
and `address_space_operations` a filesystem must supply — from the messy
business of an on-disk layout, so you can see plainly what the kernel demands
and what it gives you for free through the `simple_*` and `generic_*` helpers.

This is a 🟥 **teaching core**, and it is honest about that: it demonstrates the
mount + namespace + page-cache-I/O path end to end, and the README below maps
exactly what a *persistent* filesystem would add on top.

## What you'll learn

- **Registering a filesystem**: `register_filesystem` / `unregister_filesystem`
  and the `struct file_system_type` factory, including `MODULE_ALIAS_FS` so
  `mount` can autoload the module.
- **The mount path for a device-less FS**: `mount_nodev` → your `fill_super`,
  building the superblock, the root inode, and the root dentry with
  `d_make_root`; and the teardown via `kill_litter_super`.
- **The four VFS operation tables** and how little you actually write:
  `super_operations` (`simple_statfs`, `generic_delete_inode`),
  directory `inode_operations` (hand-written `create`/`mkdir`/`symlink`/`mknod`
  over `simple_lookup`/`simple_link`/`simple_unlink`/`simple_rmdir`/`simple_rename`),
  file `inode_operations` (`simple_setattr`/`simple_getattr`), and
  `address_space_operations` wiring an inode's bytes to the **page cache**.
- **Page-cache file I/O for free**: `generic_file_read_iter` /
  `generic_file_write_iter` / `generic_file_mmap` driving `simple_write_begin` /
  `simple_write_end`, and why a RAM FS uses `noop_dirty_folio` + `noop_fsync`.
- **The RAM-FS invariants**: why pages are `mapping_set_unevictable` (there is no
  backing store to page them back from), and the `dget()` "pin the dentry in
  core" reference that makes the dcache double as directory storage.
- **Reference/link-count discipline**: `inc_nlink` for `.`/`..`, `d_instantiate`,
  `iput` on the error path, and `generic_delete_inode` as the line that actually
  frees a deleted file's RAM.
- **Assembly**: the dcache string hash and the inode-number allocator, and how
  clang compiles `* 11` with no multiply and "skip zero" with no branch.

## Build & run (Linux only; test in a VM)

Kernel modules build only on **Linux**, against configured kernel headers, and a
buggy filesystem module can hang the whole machine — so build and test inside a
**throwaway VM** (QEMU, or WSL2 with a custom kernel, or a disposable cloud box).
Never load an experimental FS module on a machine you care about.

```bash
# On the Linux build host (needs: make, clang or gcc, kernel headers matching
# `uname -r`, i.e. the linux-headers-$(uname -r) package or a built kernel tree):
cd 01-kernel/04-in-memory-fs
make                      # -> tinyfs.ko    (Kbuild: make -C $KDIR M=$PWD modules)
```

Then, **as root inside the VM**:

```bash
insmod ./tinyfs.ko
cat /proc/filesystems | grep tinyfs         # tinyfs now appears

mkdir -p /mnt/tiny
mount -t tinyfs none /mnt/tiny              # 'none' = no backing device
#   or with an option:  mount -t tinyfs -o mode=0777 none /mnt/tiny

echo "hello ram" > /mnt/tiny/file          # create + write (page cache)
cat /mnt/tiny/file                          # read it back:  hello ram
mkdir /mnt/tiny/sub                         # mkdir
ln -s file /mnt/tiny/link                   # symlink
ls -l /mnt/tiny                             # see it all
stat -f /mnt/tiny                           # f_type shows our magic (0x54494e59)
cat /proc/mounts | grep tinyfs              # show_options renders mode= if set

umount /mnt/tiny                            # all data is freed here
rmmod tinyfs                                # refused while still mounted (.owner)
```

Regenerate the teaching assembly on **any** host (clang cross-targets Linux):

```bash
make asm        # writes asm/demo.{O0.s,s,O2.s}; annotated.s is hand-authored
```

## How it works (file by file)

### `tinyfs.c` — the whole filesystem, one file, bottom-up

1. **`tinyfs_get_inode()` — the inode factory.** The single choke point that
   mints every inode. It calls `new_inode()`, numbers it with `get_next_ino()`,
   sets owner/mode via `inode_init_owner(&nop_mnt_idmap, …)`, points the inode's
   `i_mapping->a_ops` at our page-cache ops, and — the key RAM-FS step — marks
   the mapping **unevictable** (`mapping_set_unevictable`) so the reclaimer can
   never throw away file data that has nowhere to be paged back from. It then
   switches on the file-type bits to install the right `i_op`/`i_fop` for a
   regular file, directory (with `inc_nlink` for its own `.`), symlink, or
   special device node.

2. **Directory `inode_operations`.** `tinyfs_mknod()` is the backend for
   `create`/`mkdir`/`mknod`: make an inode, `d_instantiate()` it onto the
   negative dentry the VFS looked up, and take an extra `dget()` to **pin the
   dentry (and its inode) in core** — because with no disk, the dcache *is* the
   directory. `tinyfs_mkdir()` additionally bumps the parent's link count for the
   new `..`; `tinyfs_symlink()` stores the target string through the page cache
   with `page_symlink()`. Everything else — `lookup`, `link`, `unlink`, `rmdir`,
   `rename` — is a kernel `simple_*` helper.

3. **File I/O tables.** `tinyfs_aops` (`simple_read_folio` / `simple_write_begin`
   / `simple_write_end` / `noop_dirty_folio`) plug the inode into the page cache;
   `tinyfs_file_operations` are all `generic_*`/`noop_*` — the page cache moves
   the bytes to and from userspace, so we never touch a user pointer directly.

4. **Superblock + mount.** `tinyfs_fill_super()` sets `s_magic`, `s_blocksize =
   PAGE_SIZE`, `s_op`, allocates the per-mount `tinyfs_fs_info` (parsing a
   `mode=` option with the `parser.h` `match_token` API), and builds the root
   with `d_make_root()`. `tinyfs_mount()` is just `mount_nodev(…, fill_super)`;
   `tinyfs_kill_sb()` frees our private info then calls `kill_litter_super()`,
   whose cascading `dput()`s release every pin and free all the RAM.

5. **Module glue.** `tinyfs_init`/`_exit` call `register_filesystem` /
   `unregister_filesystem`; `.owner = THIS_MODULE` blocks `rmmod` while any
   instance is mounted.

The file is deliberately ~50% comments: every VFS call names the subsystem it
touches, and every refcount/link-count/eviction rule states the invariant and
what breaks if you violate it.

### `asm/demo.c` — the extracted pure-logic core

Two routines tinyfs relies on, lifted out with no kernel headers so they compile
standalone: `tinyfs_name_hash()` (the dcache string hash) and
`tinyfs_next_ino()` (the reserved-value-skipping inode allocator).

### `Makefile` — Kbuild wrapper

`obj-m += tinyfs.o` plus the `make -C $(KDIR) M=$(PWD) modules` re-entry, and an
`asm` target holding the exact clang commands.

## Assembly notes

Kernel C **cannot be compiled standalone** to assembly on this (or any) host: it
`#include`s `<linux/fs.h>` and friends, which only exist inside a configured
kernel tree. So — following the repo convention — the assembly deliverable is
extracted into `asm/demo.c`, a self-contained file holding the project's two
most instructive **pure-logic** routines, with genuine clang output committed
alongside:

- [`asm/demo.O0.s`](asm/demo.O0.s) — `-O0`: the naive mapping. `partial_name_hash`
  and `end_name_hash` are real `callq`s, the `* 11` is a real `imulq $11`, and
  every value is spilled to the stack. Easiest to trace statement by statement.
- [`asm/demo.s`](asm/demo.s) — `-O1`: the annotated baseline.
- [`asm/demo.O2.s`](asm/demo.O2.s) — `-O2`: the hash loop is **unrolled two
  characters per iteration** with a scalar remainder tail.
- [`asm/demo.annotated.s`](asm/demo.annotated.s) — the `-O1` output with a
  comment on essentially every instruction, plus a header block on the SysV
  AMD64 ABI (args in `rdi, rsi, …`; return in `rax`; callee-saved `rbx, rbp,
  r12–r15`) and the leaf-function prologue/epilogue.

Two optimizations the annotation dwells on, because they are the whole reason to
read assembly:

- **Multiply by 11 with zero multiply instructions.** `partial_name_hash`'s
  `* 11` becomes two `leaq`s — `leaq (%rsi,%rsi,4), %rax` (×5) then
  `leaq (%rsi,%rax,2), %rax` (×11 = 1 + 2·5). Constant multiplies almost always
  become `lea`/shift/add chains; spotting that is core reverse-engineering.
- **Skip-zero on wraparound with zero branches.** `tinyfs_next_ino`'s
  `if (res == 0) res++;` compiles to `cmpl $1, %ecx` (carry is set exactly when
  the incremented value wrapped to 0) followed by `adcl $1, %eax` — add-with-
  carry folds both increments into straight-line code. Compare with `-O0`, where
  the same guard is a real `jne` branch.

## Going further (the `Stretch:` from the list, and what production adds)

The list's stretch goal: **persist to a backing file, or prototype in FUSE
first.** Concretely, the gap between this teaching core and a real filesystem:

- **On-disk layout.** A real FS defines a disk superblock and inode table,
  reads/writes them through `mount_bdev` and `buffer_head`/`bio` I/O, and
  supplies real `->read_folio`/`->writepages` that move blocks instead of
  zero-filling. `tinyfs` skips all of this by living in the page cache.
- **Block/space allocation.** A free-space bitmap or extent allocator, plus
  honest `statfs` block counts (ours reports zeros — there is no fixed size).
- **Crash consistency.** Journaling or copy-on-write so a power loss mid-write
  leaves a mountable filesystem. RAM has no crash to be consistent across.
- **The rest of the surface.** Extended attributes, ACLs, quotas, `fiemap`, NFS
  export ops, `tmpfile`, and idmapped-mount support.

Two good on-ramps: (1) **prototype in FUSE** (`libfuse`) entirely in userspace —
same `lookup`/`create`/`read`/`write` mental model, no VM required, no way to
crash the kernel — then port the design down; or (2) extend `tinyfs` to
**persist**: on unmount, walk the tree and serialize inodes + page contents to a
host file, and reload them in `fill_super`. That single step forces you to
confront everything a real FS does that a RAM FS gets to ignore.

## References

- Linux source: `fs/ramfs/inode.c` (the model for this code), `fs/libfs.c` (all
  the `simple_*` helpers), `fs/inode.c` (`get_next_ino`, `new_inode`),
  `include/linux/dcache.h` (`partial_name_hash`/`end_name_hash`).
- *Documentation/filesystems/vfs.rst* — the canonical description of the four
  operation tables and their contracts.
- `man 2 mount`, `man 2 statfs`, `man 8 mount` (the `-t`/`-o` interface).
- *Linux Kernel Development* (Love) ch. "The Virtual Filesystem"; *Understanding
  the Linux Kernel* (Bovet & Cesati) ch. "The VFS".
- FUSE: the `libfuse` `example/` directory (`hello.c`, `passthrough.c`) for the
  userspace prototype path.
