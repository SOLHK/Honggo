#!/usr/bin/env bash
set -euo pipefail
root=$(cd "$(dirname "${BASH_SOURCE[0]}")/.." && pwd)
build="$root/build/diagnostics-tests"
mkdir -p "$build"
binary="$build/diagnostics"
xcrun clang -fobjc-arc -fblocks -Wall -Werror -O1 -g -I "$root/src" \
    -framework Foundation "$root/src/Diagnostics.m" "$root/tests/DiagnosticsIntegration.m" -o "$binary"

fixture=$(mktemp -d "$build/run.XXXXXXXX")
stalled=
cleanup() {
    if [[ -n "$stalled" ]]; then kill -KILL "$stalled" 2>/dev/null || true; fi
    chmod u+w "$fixture/failure" 2>/dev/null || true
    rm -rf -- "$fixture"
}
trap cleanup EXIT
mkdir -p "$fixture/events" "$fixture/history" "$fixture/recovery" "$fixture/capacity" "$fixture/retention" "$fixture/failure/Exports"
"$binary" --events "$fixture/events" 2>&1 | tee "$build/events.log"
"$binary" --normal "$fixture/history" 2>&1 | tee "$build/normal.log"
"$binary" --stall "$fixture/history" >"$build/stall.log" 2>&1 &
stalled=$!
found=no
for ((i=0; i<100; i++)); do
    if grep -R -q --include='*.ndjson' '"watchdog.severe"' "$fixture/history"; then
        found=yes
        break
    fi
    sleep 0.2
done
test "$found" = yes
kill -KILL "$stalled"
wait "$stalled" 2>/dev/null || test "$?" -eq 137
stalled=
# Model an interrupted append without invalidating the earlier complete lines.
severe_file=$(grep -R -l --include='*.ndjson' '"watchdog.severe"' "$fixture/history")
printf '%s' '{"event":' >> "$severe_file"
"$binary" --restart "$fixture/history" 2>&1 | tee "$build/restart.log"
"$binary" --recover "$fixture/recovery" 2>&1 | tee "$build/recovery.log"
"$binary" --capacity "$fixture/capacity" 2>&1 | tee "$build/capacity.log"
"$binary" --retention "$fixture/retention" 2>&1 | tee "$build/retention.log"
if [[ $(id -u) != 0 ]]; then
    chmod 500 "$fixture/failure"
    "$binary" --write-failure "$fixture/failure" 2>&1 | tee "$build/failure.log"
fi
