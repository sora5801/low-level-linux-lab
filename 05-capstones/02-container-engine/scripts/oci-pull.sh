#!/bin/sh
# ============================================================================
# oci-pull.sh — a minimal OCI/Docker image puller: fetch + extract layers.
# ============================================================================
#
# A container "image" on a registry is not one file — it is a small JSON
# MANIFEST that lists content-addressed BLOBS (a config blob + one gzipped tar
# per filesystem LAYER). Pulling an image is therefore:
#
#   1. get a bearer TOKEN scoped to "pull this repository"
#   2. GET the manifest; if it is a multi-arch INDEX, pick the amd64/linux entry
#      and GET that architecture's manifest
#   3. for each layer digest, GET the blob and untar it, IN ORDER, over the tree
#
# The result is a flattened rootfs suitable as `ceng`'s overlay lowerdir. This is
# exactly what `docker pull` / `containerd` do; we implement the happy path so
# the wire format is legible. It is the "(minimal) OCI puller" the capstone spec
# calls for.
#
# NEEDS: curl, jq, tar (gzip). Talks to Docker Hub by default (no login needed
# for public images). This is a network client — run it on a machine with egress.
#
# Usage:
#   scripts/oci-pull.sh alpine:3.19            ./image
#   scripts/oci-pull.sh library/busybox:latest ./image
#   REGISTRY=registry-1.docker.io scripts/oci-pull.sh <repo>:<tag> <dest>
#
# HONEST SCOPE (documented, not hidden):
#   * gzip layers only (the common case); zstd/estargz layers are not handled.
#   * whiteouts (.wh. files that DELETE a lower file) get a BEST-EFFORT pass;
#     opaque-dir whiteouts and hardlink edge cases are not fully modeled.
#   * no signature/cosign verification, no layer digest re-verification, no
#     manifest caching. A real client does all three.
# ============================================================================
set -eu

REF="${1:?usage: oci-pull.sh <repo>:<tag> <dest>}"
DEST="${2:-./image}"
REGISTRY="${REGISTRY:-registry-1.docker.io}"
AUTH="${AUTH:-auth.docker.io}"
AUTH_SERVICE="${AUTH_SERVICE:-registry.docker.io}"

for tool in curl jq tar; do
    command -v "$tool" >/dev/null 2>&1 || { echo "error: need '$tool'" >&2; exit 1; }
done

# --- parse "<repo>:<tag>", defaulting the tag and the official-image prefix ---
REPO="${REF%%:*}"
TAG="${REF#*:}"
[ "$TAG" = "$REF" ] && TAG="latest"            # no ':' present -> default tag
case "$REPO" in
    */*) : ;;                                  # already has a namespace
    *)   REPO="library/$REPO" ;;               # official image -> library/<name>
esac

echo ">> pulling $REPO:$TAG from $REGISTRY into $DEST"

# --- (1) bearer token scoped to pull this repo -------------------------------
# Public repos still require an anonymous token; the registry hands one out for
# the asked-for scope. We pass it as "Authorization: Bearer" on every request.
TOKEN=$(curl -fsSL \
    "https://${AUTH}/token?service=${AUTH_SERVICE}&scope=repository:${REPO}:pull" \
    | jq -r '.token')
[ -n "$TOKEN" ] && [ "$TOKEN" != "null" ] || { echo "error: no token" >&2; exit 1; }

# All the manifest media types we are willing to accept, in one reusable set of
# -H flags: Docker v2 + OCI, both single-manifest and multi-arch index.
ACCEPT='-H Accept:application/vnd.docker.distribution.manifest.v2+json
        -H Accept:application/vnd.docker.distribution.manifest.list.v2+json
        -H Accept:application/vnd.oci.image.manifest.v1+json
        -H Accept:application/vnd.oci.image.index.v1+json'

manifest_get() {  # $1 = reference (tag or digest)
    # shellcheck disable=SC2086  # we WANT $ACCEPT to word-split into -H flags
    curl -fsSL -H "Authorization: Bearer $TOKEN" $ACCEPT \
        "https://${REGISTRY}/v2/${REPO}/manifests/$1"
}

# --- (2) fetch the manifest; resolve an index down to amd64/linux ------------
MANIFEST=$(manifest_get "$TAG")
MEDIATYPE=$(printf '%s' "$MANIFEST" | jq -r '.mediaType // empty')

case "$MEDIATYPE" in
    *manifest.list*|*image.index*)
        echo ">> multi-arch index: selecting linux/amd64"
        DIGEST=$(printf '%s' "$MANIFEST" | jq -r '
            .manifests[]
            | select(.platform.os=="linux" and .platform.architecture=="amd64")
            | .digest' | head -n1)
        [ -n "$DIGEST" ] || { echo "error: no linux/amd64 in index" >&2; exit 1; }
        MANIFEST=$(manifest_get "$DIGEST")
        ;;
esac

# --- (3) download + extract each layer, in order -----------------------------
mkdir -p "$DEST"
TMP=$(mktemp -d)
trap 'rm -rf "$TMP"' EXIT

# .layers[].digest is the ordered list of filesystem layers, base first.
LAYERS=$(printf '%s' "$MANIFEST" | jq -r '.layers[].digest')
[ -n "$LAYERS" ] || { echo "error: manifest has no layers" >&2; exit 1; }

i=0
for DIGEST in $LAYERS; do
    i=$((i + 1))
    echo ">> layer $i: $DIGEST"
    # A blob GET may 307-redirect to a CDN; -L follows it. The blob is a gzipped
    # tar (media type ...layer.v1.tar+gzip).
    curl -fsSL -H "Authorization: Bearer $TOKEN" \
        "https://${REGISTRY}/v2/${REPO}/blobs/${DIGEST}" -o "$TMP/layer.tar.gz"

    # Extract over the growing tree. --no-same-owner keeps ownership sane when
    # unpacking as a normal user (real IDs get remapped by the user namespace at
    # run time anyway). Overlay-style whiteouts are handled in the pass below.
    tar -xzf "$TMP/layer.tar.gz" -C "$DEST" --no-same-owner 2>/dev/null || \
        tar -xzf "$TMP/layer.tar.gz" -C "$DEST"

    # BEST-EFFORT whiteouts: a file named ".wh.<name>" in a layer means "<name>
    # was deleted in this layer". Remove both the marker and its target. (Opaque
    # ".wh..wh..opq" directory markers are only partially handled — see scope.)
    find "$DEST" -name '.wh.*' 2>/dev/null | while IFS= read -r wh; do
        dir=$(dirname "$wh")
        base=$(basename "$wh")
        target="${base#.wh.}"
        rm -rf "$dir/$target" "$wh"
    done
done

echo ">> done. lowerdir ready: $DEST"
echo "   run:  ./ceng --image $DEST -- /bin/sh"
