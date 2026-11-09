#!/usr/bin/env bash
set -euo pipefail
root=$(cd "$(dirname "${BASH_SOURCE[0]}")/.." && pwd)
mkdir -p "$root/build/diagnostics-tests"
xcrun clang -fobjc-arc -fblocks -Wall -Werror -O1 -g -I "$root/src" \
    -framework Foundation "$root/src/Diagnostics.m" "$root/tests/DiagnosticsIntegration.m" \
    -o "$root/build/diagnostics-tests/diagnostics"
"$root/build/diagnostics-tests/diagnostics" 2>&1 | tee "$root/build/diagnostics-tests/results.log"
"$root/build/diagnostics-tests/diagnostics" --write-failure 2>&1 | tee "$root/build/diagnostics-tests/failure-results.log"
