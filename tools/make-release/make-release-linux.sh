#!/usr/bin/env bash
set -euo pipefail

SCRIPT_DIR="$(cd "$(dirname "${BASH_SOURCE[0]}")" && pwd)"
# shellcheck source=_release-common.sh
source "$SCRIPT_DIR/_release-common.sh"

ROOT="$(rd_repo_root)"
cd "$ROOT"

VERSION="$(rd_version "$ROOT")"
[ -n "$VERSION" ] || { echo "ERROR: could not parse RETRODEBUGGER_VERSION_STRING" >&2; exit 1; }

case "$(uname -m)" in
  x86_64)        ARCH="x64" ;;
  aarch64|arm64) ARCH="arm64" ;;
  *)             ARCH="$(uname -m)" ;;
esac

RELEASE_NAME="RetroDebugger-v$VERSION"
ZIP_NAME="RetroDebugger-v$VERSION-linux-$ARCH.zip"

echo "==> [1/5] Staging $RELEASE_NAME (linux-$ARCH)"
STAGE="$(rd_make_stage "$VERSION")"
trap 'rm -rf "$STAGE"' EXIT
RELEASE_DIR="$STAGE/$RELEASE_NAME"
rd_copy_common "$ROOT" "$RELEASE_DIR"

# Linux-only: icons folder with the contents of assets/icons.
mkdir -p "$RELEASE_DIR/icons"
cp -R "$ROOT/assets/icons/." "$RELEASE_DIR/icons/"

echo "==> [2/5] Building RetroDebugger (clean)"
rm -rf "$ROOT/build"
echo "==> Running build-linux.sh with verbose logging"
set -x
chmod +x "$ROOT/build-linux.sh"
( cd "$ROOT" && ./build-linux.sh )
set +x

echo "==> [3/5] Copying binary"
BIN="$ROOT/build/retrodebugger"
[ -f "$BIN" ] || { echo "ERROR: built binary not found at $BIN" >&2; exit 1; }
cp "$BIN" "$RELEASE_DIR/"

echo "==> [4/5] Packaging $ZIP_NAME"
FINAL="$(rd_package_and_collect "$ROOT" "$STAGE" "$RELEASE_NAME" "$ZIP_NAME")"

echo "==> [5/5] Cleaning staging dir"
rm -rf "$STAGE"

echo "==> Release ready: $FINAL"
