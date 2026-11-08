#!/usr/bin/env bash
set -euo pipefail

if [ "$#" -gt 1 ]; then
    printf 'Usage: bash tools/build-ios.sh [DIRECTORY/Honggo-VERSION.dylib]\n' >&2
    exit 2
fi

source_root=$(cd "$(dirname "${BASH_SOURCE[0]}")/.." && pwd)
version=$(python3 "$source_root/tools/version.py")
filename="Honggo-$version.dylib"
output=${1:-"$source_root/build/$filename"}
if [ "$(basename -- "$output")" != "$filename" ]; then
    printf 'The output filename must be %s, matching the plugin version.\n' "$filename" >&2
    exit 2
fi
sdk=$(xcrun --sdk iphoneos --show-sdk-path)
mkdir -p -- "$(dirname -- "$output")"

xcrun --sdk iphoneos clang \
    -target arm64-apple-ios15.0.1 -isysroot "$sdk" \
    -O2 -fobjc-arc -fblocks -fvisibility=hidden -Wall -Werror \
    -dynamiclib -Wl,-install_name,@rpath/Honggo.dylib -Wl,-no_adhoc_codesign \
    "-Wl,-current_version,$version" -Wl,-compatibility_version,1.0.0 \
    -framework Foundation -framework UIKit -framework QuartzCore -framework CoreGraphics -lobjc \
    "$source_root/src/Ads.m" "$source_root/src/Honggo.m" \
    "$source_root/src/Settings.m" "$source_root/src/TabBar.m" -o "$output"

xcrun otool -hv "$output"
xcrun otool -L "$output"
xcrun vtool -show-build "$output"
