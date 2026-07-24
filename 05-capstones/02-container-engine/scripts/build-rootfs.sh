#!/bin/sh
# ============================================================================
# build-rootfs.sh — assemble a minimal image rootfs for `ceng` to run.
# ============================================================================
#
# `ceng` needs a LOWERDIR: a read-only directory tree that looks like a Unix
# root (/bin/sh, coreutils, the mountpoints /proc /sys /dev). overlayfs stacks a
# per-container writable layer on top of it, so this tree is never modified at
# run time and can be shared by many containers at once.
#
# Two ways to build one, cheapest first:
#
#   busybox   — one static binary providing sh + ~300 applets via symlinks. Tiny
#               (~1-2 MiB), no network needed, perfect for the teaching demo.
#   debootstrap — a real Debian userland (needs root + network). Closer to what
#               you would actually ship; slower and larger.
#
# Usage:
#   scripts/build-rootfs.sh busybox   [DEST]   # default DEST = ./image
#   scripts/build-rootfs.sh debian    [DEST] [SUITE]  # e.g. debian ./image bookworm
#
# This is Linux-only (it builds a Linux root). Run it on the same host you run
# `ceng` on. It does NOT need to run as root for the busybox path.
# ============================================================================
set -eu

METHOD="${1:-busybox}"
DEST="${2:-./image}"

# The mountpoints ceng expects to exist in the image: a fresh /proc is mounted
# over /proc inside the container, pivot_root needs the tree to be a real root,
# and /dev + /tmp are conveniences software assumes.
make_skeleton() {
    mkdir -p "$DEST"/bin "$DEST"/proc "$DEST"/sys "$DEST"/dev \
             "$DEST"/etc "$DEST"/tmp "$DEST"/root
    # A couple of files so DNS + a sane shell prompt work inside the box. These
    # live in the read-only lower layer; the container can override them because
    # a write triggers overlayfs copy-up into its private upper layer.
    printf 'nameserver 1.1.1.1\n' > "$DEST"/etc/resolv.conf
    printf 'root:x:0:0:root:/root:/bin/sh\n' > "$DEST"/etc/passwd
    printf 'root:x:0:\n' > "$DEST"/etc/group
}

case "$METHOD" in
busybox)
    # Find a busybox binary. Prefer a STATIC one so the image needs no shared
    # libraries at all (nothing to copy, nothing to break after pivot_root).
    BB="$(command -v busybox || true)"
    if [ -z "$BB" ]; then
        echo "error: busybox not found. Install it, e.g.:" >&2
        echo "   Debian/Ubuntu:  sudo apt-get install busybox-static" >&2
        echo "   Alpine:         apk add busybox" >&2
        exit 1
    fi

    make_skeleton
    cp "$BB" "$DEST"/bin/busybox

    # busybox decides which applet to run from argv[0], so every command is just
    # a symlink to the one binary. `busybox --install -s` would do this too, but
    # we spell out a useful subset explicitly so the image stays small and the
    # intent is clear.
    for applet in sh ls cat echo pwd ps mount umount id uname sleep hostname \
                  mkdir rmdir rm cp mv ln touch chmod chown grep sed awk \
                  ping wget ip ifconfig route netstat cut head tail wc env; do
        ln -sf busybox "$DEST/bin/$applet"
    done

    echo "busybox rootfs ready at: $DEST"
    echo "run it:  sudo ../../ ... (see README)   ./ceng --image $DEST -- /bin/sh"
    ;;

debian)
    SUITE="${3:-bookworm}"
    if ! command -v debootstrap >/dev/null 2>&1; then
        echo "error: debootstrap not found (sudo apt-get install debootstrap)" >&2
        exit 1
    fi
    if [ "$(id -u)" -ne 0 ]; then
        echo "error: the debian path needs root (debootstrap creates device nodes)" >&2
        echo "       re-run:  sudo scripts/build-rootfs.sh debian $DEST $SUITE" >&2
        exit 1
    fi
    # --variant=minbase = the smallest usable Debian: dpkg + apt + a shell, no
    # daemons. This downloads packages, so it needs network.
    debootstrap --variant=minbase "$SUITE" "$DEST" \
        http://deb.debian.org/debian/
    echo "debian ($SUITE) rootfs ready at: $DEST"
    ;;

*)
    echo "usage: $0 {busybox|debian} [DEST] [SUITE]" >&2
    exit 2
    ;;
esac
