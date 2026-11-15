#!/usr/bin/env bash
set -euo pipefail

source_root=$(cd "$(dirname "${BASH_SOURCE[0]}")/.." && pwd)
work="$source_root/build/preferences-tests"
mkdir -p "$work"
xcrun clang -fobjc-arc -fblocks -Wall -Werror -O1 -g -I "$source_root/src" \
    -framework Foundation -framework CoreFoundation \
    "$source_root/src/Preferences.m" "$source_root/tests/PreferencesIntegration.m" \
    -o "$work/preferences"
"$work/preferences" 2>&1 | tee "$work/results.log"
