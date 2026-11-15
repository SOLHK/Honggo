#!/usr/bin/env bash
set -euo pipefail
source_root=$(cd "$(dirname "${BASH_SOURCE[0]}")/.." && pwd)
work="$source_root/build/appearance-tests"
mkdir -p "$work"
xcrun clang -fobjc-arc -fblocks -Wall -Werror -I "$source_root/src" \
  -framework Foundation -framework CoreFoundation \
  "$source_root/src/AppearancePreferences.m" "$source_root/tests/AppearancePreferencesTests.m" \
  -o "$work/preferences"
"$work/preferences" 2>&1 | tee "$work/preferences.log"
