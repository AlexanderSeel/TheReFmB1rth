#!/usr/bin/env sh
set -eu

UPSTREAM_REPO="https://github.com/hugelton/Felucca.git"
UPSTREAM_COMMIT="3dd2b0852bc310a2c1bf00c2d443ac19202c2140"
DEST="${1:-.upstream/felucca}"

mkdir -p "$(dirname "$DEST")"
if [ ! -d "$DEST/.git" ]; then
  git clone --filter=blob:none --no-checkout "$UPSTREAM_REPO" "$DEST"
fi

git -C "$DEST" fetch --depth 1 origin "$UPSTREAM_COMMIT"
git -C "$DEST" checkout --detach "$UPSTREAM_COMMIT"
ACTUAL="$(git -C "$DEST" rev-parse HEAD)"
[ "$ACTUAL" = "$UPSTREAM_COMMIT" ] || { echo "Felucca pin mismatch: $ACTUAL" >&2; exit 2; }

echo "Pinned Felucca baseline ready at $DEST ($ACTUAL)"
