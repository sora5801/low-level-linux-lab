// SPDX-License-Identifier: GPL-2.0
/* ===========================================================================
 * tinyfs.c — a tiny, in-memory VFS filesystem (a ramfs you can read).
 * ===========================================================================
 *
 * WHAT THIS IS
 * ------------
 * A complete, mountable Linux filesystem whose entire contents live in RAM.
 * It is deliberately modeled on the kernel's own `fs/ramfs/inode.c`, trimmed
 * and then re-commented so that every VFS contract is spelled out. Mount it,
 * create files and directories, write and read small files, make symlinks —
 * it all works — but nothing is ever written to a disk. When you unmount, the
 * data is gone. That is the point: it isolates the *VFS glue* from the messy
 * business of a real on-disk layout, so you can see clearly what a filesystem
 * MUST provide to the kernel and what the kernel provides for free.
 *
 * THE THREE OBJECTS THE VFS TALKS TO, AND WHO OWNS WHAT
 * ----------------------------------------------------
 *   struct file_system_type  — one per filesystem *driver*. We register it so
 *                              `mount -t tinyfs` can find us. It is the factory.
 *   struct super_block       — one per *mounted instance*. Holds s_op, the root
 *                              dentry, block size, magic, and our private
 *                              s_fs_info (mount options). The VFS creates it and
 *                              hands it to our fill_super to populate.
 *   struct inode             — one per file/dir/symlink. Holds i_op (namespace
 *                              ops), i_fop (I/O ops), i_mapping (the page cache
 *                              for this file's bytes), owner, mode, timestamps.
 *   struct dentry            — one per *name→inode* link in the tree. The VFS
 *                              owns the dcache; we only pin entries (dget) and
 *                              attach inodes to them (d_instantiate).
 *
 * WHERE DO THE FILE BYTES LIVE?
 * -----------------------------
 * In the *page cache*. Every inode has an `address_space` (i_mapping). For a
 * RAM filesystem we simply never evict those pages and never write them back:
 * the page cache *is* our storage. A disk filesystem would instead point the
 * address_space at a block device and supply writeback/readpage that do real
 * I/O. We hand almost all of the read/write path to the kernel's generic
 * page-cache helpers (generic_file_read_iter / write_iter, simple_write_begin
 * / _end); the only thing we customize is "these pages are pinned in RAM."
 *
 * TEACHING-CORE SCOPE (read this honestly)
 * ----------------------------------------
 * Covered end to end: register_filesystem, mount (mount_nodev + fill_super),
 * super_operations, directory inode_operations (create/lookup/mkdir/…),
 * regular-file I/O through the page cache, symlinks, and a `mode=` mount
 * option. NOT covered (a real/persistent FS would add all of this): an on-disk
 * superblock and inode table, block allocation and a free-space bitmap,
 * journaling/crash consistency, a writeback path (->writepages), extended
 * attributes, quotas, and NFS export. Those are 🟥-months of work each; see the
 * README "Going further" for the map.
 *
 * BUILD & RUN: kernel modules build ONLY on Linux, against configured kernel
 * headers, and must be tested in a throwaway VM (a bad FS module can wedge your
 * machine). See README.md for the exact QEMU/WSL recipe. This file will NOT
 * compile on the Windows host it ships from — that is expected; the assembly
 * deliverable in asm/ extracts the pure-logic core so the repo's "every C
 * project ships annotated asm" rule is still honored.
 *
 * KERNEL VERSION: written against the Linux 6.6 LTS VFS API generation, whose
 * fingerprints are: inode_operations callbacks take `struct mnt_idmap *` (6.3+),
 * ctime is set via inode_set_ctime_current() (6.6+), the page-cache read hook
 * is ->read_folio (6.5+), and splice uses filemap_splice_read (6.5+). The VFS
 * churns these signatures often; each is flagged at its use site below.
 * ===========================================================================
 */

#include <linux/module.h>       /* module_init/exit, MODULE_* macros           */
#include <linux/fs.h>           /* the whole VFS: inode, super_block, dentry…   */
#include <linux/pagemap.h>      /* page cache: PAGE_SIZE, mapping_set_*         */
#include <linux/highmem.h>      /* kmap for high memory (page_symlink path)     */
#include <linux/init.h>         /* __init / __exit section annotations          */
#include <linux/string.h>       /* strlen, strsep                              */
#include <linux/slab.h>         /* kzalloc/kfree for our per-mount info         */
#include <linux/parser.h>       /* match_token/match_octal for mount options    */
#include <linux/seq_file.h>     /* seq_printf for show_options                  */
#include <linux/mnt_idmap.h>    /* nop_mnt_idmap — the identity id-mapping      */
#include <linux/statfs.h>       /* (via simple_statfs)                          */

/* Our on-the-wire superblock magic. `statfs(2)` reports it in f_type so tools
 * like `stat -f` can identify the filesystem. Real filesystems put this same
 * constant in <uapi/linux/magic.h>; we pick our own so we collide with nobody.
 * The bytes spell "TINY" in ASCII. */
#define TINYFS_MAGIC        0x54494e59
/* Default permission bits for the root directory (rwxr-xr-x). Overridable at
 * mount time with `-o mode=0777`, parsed below. */
#define TINYFS_DEFAULT_MODE 0755

/* ---------------------------------------------------------------------------
 * Per-mount private data. One of these hangs off super_block->s_fs_info for the
 * lifetime of the mount. Here it only remembers the root mode chosen at mount
 * time, but it is the natural home for any future superblock-wide state (a size
 * limit, a block bitmap pointer, statistics…). We allocate it in fill_super and
 * free it in kill_sb — that pairing is the whole ownership story.
 * ------------------------------------------------------------------------- */
struct tinyfs_fs_info {
	umode_t mode;           /* root-directory permission bits                */
};

/* Forward declarations. get_inode() (defined first, because fill_super needs
 * it) references these operation tables, which are defined lower down. C needs
 * to see the names before the bodies; the `static const` definitions later
 * fill them in. */
static const struct inode_operations tinyfs_dir_inode_operations;
static const struct inode_operations tinyfs_file_inode_operations;
static const struct file_operations  tinyfs_file_operations;
static const struct address_space_operations tinyfs_aops;

/* ===========================================================================
 * SECTION 1 — inode factory
 * ===========================================================================
 * Every file, directory and symlink in the tree is an `inode`. This function
 * is the single choke point that mints one, wires it to the correct operation
 * tables based on its type, and attaches a page-cache address_space. Both
 * fill_super (for the root dir) and mknod (for user-created files) call it.
 */
static struct inode *tinyfs_get_inode(struct super_block *sb,
				      const struct inode *dir,
				      umode_t mode, dev_t dev)
{
	/* new_inode() allocates from the inode slab cache, bumps the sb's inode
	 * list, and returns an inode with i_count == 1 (one reference — the one
	 * we are about to hand out). On failure we return NULL and the caller
	 * turns that into -ENOSPC. */
	struct inode *inode = new_inode(sb);

	if (!inode)
		return NULL;

	/* i_ino must be unique within this superblock. get_next_ino() is the
	 * kernel's lightweight per-CPU counter for pseudo-filesystems that have
	 * no real inode table. The exact allocator logic is extracted into
	 * asm/demo.c and annotated — 0 is reserved (means "no inode"), so it
	 * skips a wraparound to zero. */
	inode->i_ino = get_next_ino();

	/* Set owner uid/gid and mode. `nop_mnt_idmap` is the identity mapping:
	 * uids on the mount equal uids in the caller's namespace. A ramfs-class
	 * FS does not support idmapped mounts, so we hardcode the no-op map here
	 * even though our callers are handed a real idmap. If `dir` is non-NULL,
	 * setgid/inheritance rules from the parent dir are applied here too. */
	inode_init_owner(&nop_mnt_idmap, inode, dir, mode);

	/* Point this inode's page cache at OUR address_space ops. This is what
	 * makes reads/writes land in RAM pages that we control. Set it before the
	 * type switch because symlinks use the same mapping to store their target
	 * string. */
	inode->i_mapping->a_ops = &tinyfs_aops;

	/* CRITICAL RAM-FS INVARIANT #1: pages allocated for these files must
	 * never be reclaimed by the memory-management subsystem, because there is
	 * no backing store to page them back in from — evicting a page would
	 * silently destroy file data. mapping_set_unevictable() moves the pages
	 * onto the unevictable LRU so the reclaimer skips them. GFP_HIGHUSER lets
	 * the pages come from high memory (they are only ever accessed via the
	 * page cache, never as kernel-linear addresses). Forget the unevictable
	 * flag and your "file" can lose its contents under memory pressure. */
	mapping_set_gfp_mask(inode->i_mapping, GFP_HIGHUSER);
	mapping_set_unevictable(inode->i_mapping);

	/* Timestamps. inode_set_ctime_current() both sets i_ctime to "now" AND
	 * returns that timestamp; we fan it out to atime and mtime so all three
	 * start equal. Since 6.6 ctime has no public field — it MUST go through
	 * this accessor (the kernel steals low bits of the ctime for a query
	 * counter), which is why we cannot just write inode->i_ctime = …. */
	inode->i_atime = inode->i_mtime = inode_set_ctime_current(inode);

	/* Dispatch on the file-type bits (the high nibble of the mode). Each type
	 * needs a different pair of operation tables. */
	switch (mode & S_IFMT) {
	default:
		/* Device nodes, FIFOs, sockets: init_special_inode() installs the
		 * right i_fop from the kernel's device tables using `dev`. We get
		 * this for free by supporting mknod of special files. */
		init_special_inode(inode, mode, dev);
		break;
	case S_IFREG:
		/* A regular file: i_op handles metadata (setattr/getattr), i_fop
		 * handles byte I/O (read/write/mmap through the page cache). */
		inode->i_op  = &tinyfs_file_inode_operations;
		inode->i_fop = &tinyfs_file_operations;
		break;
	case S_IFDIR:
		/* A directory: i_op is our namespace table (create/lookup/…), and
		 * i_fop is the kernel's simple_dir_operations, which serves
		 * readdir() straight out of the dcache — perfect for a RAM FS whose
		 * directory *is* its set of pinned child dentries. */
		inode->i_op  = &tinyfs_dir_inode_operations;
		inode->i_fop = &simple_dir_operations;

		/* A freshly-created directory has link count 2, not 1: one for its
		 * name in the parent, one for its own "." entry. new_inode() gave
		 * us 1; bump to 2. (mkdir separately bumps the PARENT's count for
		 * the new "..".) Get this wrong and rmdir/unlink accounting breaks,
		 * leaking or prematurely freeing the inode. */
		inc_nlink(inode);
		break;
	case S_IFLNK:
		/* A symlink stores its target as file data in the page cache. The
		 * generic page_symlink_inode_operations knows how to read it back
		 * via ->get_link. inode_nohighmem() forces its one page into
		 * low memory so the readlink path can access it without kmap. */
		inode->i_op = &page_symlink_inode_operations;
		inode_nohighmem(inode);
		break;
	}

	return inode;
}

/* ===========================================================================
 * SECTION 2 — directory inode_operations (the namespace: create/lookup/…)
 * ===========================================================================
 * These are invoked by the VFS with the parent directory's i_rwsem held for
 * WRITE (for mutating ops like create/mkdir/unlink) — so we never take any lock
 * ourselves here; the VFS already serialized us against concurrent namespace
 * changes in the same directory. That single fact is why this code looks
 * lock-free: it is not, the caller holds the lock.
 */

/* mknod — the common backend for create/mkdir/mknod. Make an inode of the
 * requested type and splice it onto the dentry the VFS negatively looked up. */
static int tinyfs_mknod(struct mnt_idmap *idmap, struct inode *dir,
			struct dentry *dentry, umode_t mode, dev_t dev)
{
	struct inode *inode = tinyfs_get_inode(dir->i_sb, dir, mode, dev);
	int error = -ENOSPC;    /* the only way we can "fail": out of memory      */

	if (inode) {
		/* d_instantiate() attaches `inode` to `dentry`, turning a negative
		 * dentry (a name known to have no inode) into a positive one. After
		 * this, a lookup of that name finds this inode. */
		d_instantiate(dentry, inode);

		/* CRITICAL REFCOUNT INVARIANT #2: take an *extra* reference on the
		 * dentry so it (and therefore the inode it pins) is never evicted
		 * from the dcache while the file "exists". A disk FS can let a
		 * dentry be pruned and rebuilt from disk on the next lookup; we have
		 * no disk, so the dcache IS our directory storage. This dget() is
		 * released by simple_unlink()/simple_rmdir() when the name is
		 * removed. Drop this and your files vanish under dcache pressure. */
		dget(dentry);
		error = 0;

		/* Creating an entry modifies the parent directory: bump its mtime
		 * and ctime to now. */
		dir->i_mtime = inode_set_ctime_current(dir);
	}
	return error;
}

/* create(2) target: a regular file. Fold in S_IFREG and defer to mknod. `excl`
 * (from O_EXCL) is already enforced by the VFS before we are called, so we
 * ignore it. */
static int tinyfs_create(struct mnt_idmap *idmap, struct inode *dir,
			 struct dentry *dentry, umode_t mode, bool excl)
{
	return tinyfs_mknod(idmap, dir, dentry, mode | S_IFREG, 0);
}

/* mkdir(2): make a directory, then bump the PARENT's link count by one to
 * account for the new subdirectory's ".." entry pointing back at us. (The child
 * got its own +1 for "." inside tinyfs_get_inode's S_IFDIR case.) */
static int tinyfs_mkdir(struct mnt_idmap *idmap, struct inode *dir,
			struct dentry *dentry, umode_t mode)
{
	int ret = tinyfs_mknod(idmap, dir, dentry, mode | S_IFDIR, 0);

	if (!ret)
		inc_nlink(dir);
	return ret;
}

/* symlink(2): create an S_IFLNK inode and store the target path as its file
 * data via the page cache. */
static int tinyfs_symlink(struct mnt_idmap *idmap, struct inode *dir,
			  struct dentry *dentry, const char *symname)
{
	struct inode *inode;
	int error = -ENOSPC;

	inode = tinyfs_get_inode(dir->i_sb, dir, S_IFLNK | S_IRWXUGO, 0);
	if (inode) {
		/* +1 to copy the trailing NUL so readlink round-trips exactly. */
		int len = strlen(symname) + 1;

		/* page_symlink() writes the target into the inode's first page
		 * using our write_begin/write_end aops — i.e. the symlink body is
		 * stored the same way a regular file's first bytes would be. */
		error = page_symlink(inode, symname, len);
		if (!error) {
			d_instantiate(dentry, inode);
			dget(dentry);   /* same pin-in-core rule as mknod */
			dir->i_mtime = inode_set_ctime_current(dir);
		} else {
			/* Undo new_inode()'s reference: iput() drops i_count to 0,
			 * which (via our generic_delete_inode) frees the inode
			 * immediately. The classic "allocated then failed" unwind. */
			iput(inode);
		}
	}
	return error;
}

/*
 * The directory operations table. Note how MUCH we get from the kernel's
 * simple_* helpers — they implement the generic dcache-backed behavior that is
 * identical for every trivial in-memory filesystem:
 *   - simple_lookup : return a negative dentry (nothing exists until created)
 *   - simple_link   : hard-link (bump nlink, share the inode, extra dget)
 *   - simple_unlink : drop the name (dput the pin, decrement nlink)
 *   - simple_rmdir  : like unlink but checks the dir is empty first
 *   - simple_rename : re-parent a dentry within the dcache
 * We only hand-write the three that must MINT an inode (create/mkdir/symlink)
 * plus mknod. Everything a persistent FS would have to implement itself, the
 * VFS gives a RAM FS for free.
 */
static const struct inode_operations tinyfs_dir_inode_operations = {
	.create		= tinyfs_create,
	.lookup		= simple_lookup,
	.link		= simple_link,
	.unlink		= simple_unlink,
	.symlink	= tinyfs_symlink,
	.mkdir		= tinyfs_mkdir,
	.rmdir		= simple_rmdir,
	.mknod		= tinyfs_mknod,
	.rename		= simple_rename,
};

/* ===========================================================================
 * SECTION 3 — regular-file I/O tables (the page-cache fast path)
 * ===========================================================================
 * This is where "files in RAM" actually happens, and it is almost entirely the
 * kernel's generic machinery. We plug our address_space into the page cache and
 * let generic_file_{read,write}_iter drive it.
 */

/* address_space_operations: the contract between an inode's byte range and the
 * page cache. For a RAM FS:
 *   read_folio   : called on a cache miss to fill a page. simple_read_folio
 *                  just zero-fills and marks it uptodate — a "miss" on a RAM FS
 *                  means "this page was never written", i.e. a hole, which reads
 *                  as zeros. There is nothing to read *from*.
 *   write_begin  : allocate/find the target page before a write copies user
 *                  bytes into it (simple_write_begin).
 *   write_end    : mark the page uptodate and grow i_size (simple_write_end).
 *   dirty_folio  : noop_dirty_folio — we NEVER write back, so "dirty" is
 *                  meaningless; making it a no-op prevents the page from ever
 *                  being queued to a (nonexistent) backing device.
 * A disk FS would supply real read_folio/writepages that do block I/O here. */
static const struct address_space_operations tinyfs_aops = {
	.read_folio	= simple_read_folio,
	.write_begin	= simple_write_begin,
	.write_end	= simple_write_end,
	.dirty_folio	= noop_dirty_folio,
};

/* file_operations for regular files. Every entry is a generic kernel routine —
 * the page cache does all the work of moving bytes between userspace and pages:
 *   read_iter/write_iter : the modern iov_iter-based read/write. They call our
 *                          aops (write_begin/write_end) under the hood and
 *                          handle the copy_to/from_user for us, so we never
 *                          touch user pointers directly.
 *   mmap                 : map file pages into a process — trivial for a RAM FS
 *                          because the page-cache page IS the file page.
 *   fsync = noop_fsync   : "flush to stable storage" is a no-op; there is none.
 *   splice_*             : zero-copy pipe<->file paths, page-cache backed.
 *   llseek               : generic seek within i_size. */
static const struct file_operations tinyfs_file_operations = {
	.read_iter	= generic_file_read_iter,
	.write_iter	= generic_file_write_iter,
	.mmap		= generic_file_mmap,
	.fsync		= noop_fsync,
	.splice_read	= filemap_splice_read,
	.splice_write	= iter_file_splice_write,
	.llseek		= generic_file_llseek,
};

/* inode_operations for regular files: only metadata, no namespace ops (a file
 * is a leaf, you cannot lookup/create *inside* it). simple_setattr applies
 * chmod/chown/truncate to the in-core inode; simple_getattr fills stat(2). */
static const struct inode_operations tinyfs_file_inode_operations = {
	.setattr	= simple_setattr,
	.getattr	= simple_getattr,
};

/* ===========================================================================
 * SECTION 4 — super_operations and mount options
 * ===========================================================================
 */

/* super_operations: per-mount lifecycle hooks.
 *   statfs      = simple_statfs : reports our magic + zeroed block counts for
 *                 `df`/`statfs(2)`. A RAM FS has no fixed size, so 0 is honest.
 *   drop_inode  = generic_delete_inode : when the LAST reference to an inode
 *                 goes away, delete it immediately rather than keeping it cached
 *                 (the default keeps clean inodes around to re-read from disk —
 *                 pointless for us, and it would leak memory since our "disk" is
 *                 the inode itself). This is THE line that makes deleted files
 *                 actually free their RAM.
 *   show_options: render our mount options back for /proc/mounts. */
static int tinyfs_show_options(struct seq_file *m, struct dentry *root)
{
	struct tinyfs_fs_info *fsi = root->d_sb->s_fs_info;

	/* Only print non-default options, matching how the kernel's own
	 * filesystems keep /proc/mounts terse. */
	if (fsi->mode != TINYFS_DEFAULT_MODE)
		seq_printf(m, ",mode=%o", fsi->mode);
	return 0;
}

static const struct super_operations tinyfs_super_ops = {
	.statfs		= simple_statfs,
	.drop_inode	= generic_delete_inode,
	.show_options	= tinyfs_show_options,
};

/* Mount-option grammar. We accept exactly one option, `mode=<octal>`, to set
 * the root directory's permission bits. The parser.h API matches a string
 * against this table and extracts the argument. */
enum { Opt_mode, Opt_err };
static const match_table_t tinyfs_tokens = {
	{ Opt_mode, "mode=%o" },        /* %o => octal argument captured into args */
	{ Opt_err,  NULL },
};

/* Parse the raw comma-separated option string the mount(2) syscall handed us in
 * `void *data`. This is the classic (pre-fs_context) parsing path; it is more
 * transparent for teaching than the newer fs_parser state machine. */
static int tinyfs_parse_options(char *data, struct tinyfs_fs_info *fsi)
{
	substring_t args[MAX_OPT_ARGS];
	char *p;

	/* strsep() destructively walks the "a=1,b=2" string, returning each
	 * comma-separated token and advancing `data`. Empty tokens (e.g. from a
	 * leading comma) are skipped. */
	while ((p = strsep(&data, ",")) != NULL) {
		int token, option;

		if (!*p)
			continue;

		token = match_token(p, tinyfs_tokens, args);
		switch (token) {
		case Opt_mode:
			/* Convert the captured octal substring to an int. */
			if (match_octal(&args[0], &option))
				return -EINVAL;
			/* Keep only the permission bits (07777): a mount option
			 * has no business setting the file-TYPE bits. */
			fsi->mode = option & S_IALLUGO;
			break;
		default:
			/* Unknown option: refuse the mount rather than silently
			 * ignore it, so typos are caught loudly. */
			return -EINVAL;
		}
	}
	return 0;
}

/* ===========================================================================
 * SECTION 5 — mount / unmount plumbing
 * ===========================================================================
 */

/* fill_super: the VFS has allocated a blank super_block and calls us to
 * furnish it and build the root inode+dentry. Returning 0 means "this mount is
 * ready"; any negative errno aborts the mount and the VFS calls our kill_sb to
 * clean up (which frees s_fs_info). */
static int tinyfs_fill_super(struct super_block *sb, void *data, int silent)
{
	struct tinyfs_fs_info *fsi;
	struct inode *root_inode;
	int err;

	/* Allocate our per-mount info and hang it off the superblock. kzalloc
	 * zeroes it; s_fs_info now owns this allocation and kill_sb frees it. */
	fsi = kzalloc(sizeof(*fsi), GFP_KERNEL);
	if (!fsi)
		return -ENOMEM;
	fsi->mode = TINYFS_DEFAULT_MODE;
	sb->s_fs_info = fsi;

	/* Apply any `-o mode=…`. On error we return; the VFS will invoke
	 * kill_sb, which frees fsi — so there is no leak on this path. */
	err = tinyfs_parse_options(data, fsi);
	if (err)
		return err;

	/* Fill in the superblock's mandatory fields:
	 *   s_maxbytes   : largest file offset — the generic file-size limit.
	 *   s_blocksize  : we say "one page" because our unit of storage is a
	 *                  page-cache page. s_blocksize_bits is its log2.
	 *   s_magic      : identifies the FS to statfs.
	 *   s_op         : our super_operations.
	 *   s_time_gran  : timestamp granularity in ns — 1 means nanosecond. */
	sb->s_maxbytes		= MAX_LFS_FILESIZE;
	sb->s_blocksize		= PAGE_SIZE;
	sb->s_blocksize_bits	= PAGE_SHIFT;
	sb->s_magic		= TINYFS_MAGIC;
	sb->s_op		= &tinyfs_super_ops;
	sb->s_time_gran		= 1;

	/* Build the root inode (a directory) and wrap it in the root dentry.
	 * d_make_root() creates the special "/" dentry for this mount; on failure
	 * it has ALREADY iput() the inode for us, so we just report -ENOMEM. */
	root_inode = tinyfs_get_inode(sb, NULL, S_IFDIR | fsi->mode, 0);
	sb->s_root = d_make_root(root_inode);
	if (!sb->s_root)
		return -ENOMEM;

	return 0;
}

/* mount: the file_system_type entry point. mount_nodev() is the helper for
 * filesystems with NO backing block device (RAM, pseudo, network): it allocates
 * an anonymous super_block and drives our fill_super. Contrast mount_bdev (disk
 * FS) and mount_single (one shared instance like sysfs). */
static struct dentry *tinyfs_mount(struct file_system_type *fs_type,
				   int flags, const char *dev_name, void *data)
{
	return mount_nodev(fs_type, flags, data, tinyfs_fill_super);
}

/* kill_sb: tear down a mounted instance. Order matters: free OUR private data
 * first, then let kill_litter_super() do the generic teardown — it drops the
 * root dentry, which cascades dput()s through the tree, releasing every dentry
 * pin we took in mknod and thus every inode, freeing all the RAM. */
static void tinyfs_kill_sb(struct super_block *sb)
{
	kfree(sb->s_fs_info);
	kill_litter_super(sb);
}

/* The filesystem *driver* descriptor. register_filesystem() links this into the
 * kernel's global list so the mount(2) path can match "-t tinyfs" to us.
 *   .owner   = THIS_MODULE : refcount the module while any tinyfs is mounted,
 *              so `rmmod` is refused until the last unmount — the invariant that
 *              stops the code being freed out from under a live filesystem. */
static struct file_system_type tinyfs_type = {
	.owner		= THIS_MODULE,
	.name		= "tinyfs",
	.mount		= tinyfs_mount,
	.kill_sb	= tinyfs_kill_sb,
};
/* Lets `mount -t tinyfs` autoload this module on demand. */
MODULE_ALIAS_FS("tinyfs");

/* ===========================================================================
 * SECTION 6 — module lifecycle
 * ===========================================================================
 */
static int __init tinyfs_init(void)
{
	/* register_filesystem() makes "tinyfs" a valid `-t` type. After this,
	 * userspace can mount us. It can fail (e.g. duplicate name) — propagate
	 * the errno so insmod reports it and the module does not load half-alive. */
	return register_filesystem(&tinyfs_type);
}

static void __exit tinyfs_exit(void)
{
	/* Must succeed: the VFS guarantees no tinyfs is mounted at this point,
	 * because .owner held a module reference for each live mount and rmmod is
	 * blocked while any reference is outstanding. */
	unregister_filesystem(&tinyfs_type);
}

module_init(tinyfs_init);
module_exit(tinyfs_exit);

MODULE_LICENSE("GPL");
MODULE_AUTHOR("low-level-linux-lab");
MODULE_DESCRIPTION("tinyfs: a heavily-commented in-memory VFS filesystem (a readable ramfs)");
