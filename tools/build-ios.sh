#!/usr/bin/env bash
set -euo pipefail

if [ "$#" -ne 1 ]; then
    printf 'Usage: bash tools/build-ios.sh OUTPUT.dylib\n' >&2
    exit 2
fi

output=$1
source_root=$(cd "$(dirname "${BASH_SOURCE[0]}")/.." && pwd)
sdk=$(xcrun --sdk iphoneos --show-sdk-path)
mkdir -p -- "$(dirname -- "$output")"

xcrun --sdk iphoneos clang \
    -target arm64-apple-ios15.0.1 -isysroot "$sdk" \
    -O2 -fobjc-arc -fblocks -fvisibility=hidden -Wall -Werror \
    -dynamiclib -Wl,-install_name,@rpath/HongguoGlass.dylib -Wl,-no_adhoc_codesign \
    -framework Foundation -framework UIKit -framework QuartzCore -framework CoreGraphics -lobjc \
    "$source_root/src/Ads.m" "$source_root/src/HongguoGlass.m" \
    "$source_root/src/Settings.m" "$source_root/src/TabBar.m" -o "$output"

xcrun otool -hv "$output"
xcrun otool -L "$output"
xcrun vtool -show-build "$output"
