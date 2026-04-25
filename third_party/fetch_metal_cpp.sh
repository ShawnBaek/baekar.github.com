#!/usr/bin/env bash
# Downloads Apple's metal-cpp headers into third_party/metal-cpp/.
# These are header-only and add no runtime dependency.
set -euo pipefail

HERE="$(cd "$(dirname "$0")" && pwd)"
DEST="$HERE/metal-cpp"
URL="https://developer.apple.com/metal/cpp/files/metal-cpp_macOS14.2_iOS17.2.zip"

if [ -f "$DEST/Metal/Metal.hpp" ]; then
    echo "metal-cpp already present at $DEST"
    exit 0
fi

TMP="$(mktemp -d)"
echo "Downloading metal-cpp..."
curl -fsSL "$URL" -o "$TMP/metal-cpp.zip"
unzip -q "$TMP/metal-cpp.zip" -d "$TMP"
# Apple's archive root is metal-cpp/.
cp -R "$TMP/metal-cpp/." "$DEST/"
rm -rf "$TMP"
echo "metal-cpp installed at $DEST"
