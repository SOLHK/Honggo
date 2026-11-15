#!/usr/bin/env bash
set -euo pipefail

# Exercise real production appearance adapters on controlled UIKit host fixtures.
source_root=$(cd "$(dirname "${BASH_SOURCE[0]}")/.." && pwd)
work="$source_root/build/appearance-tests"
app="$work/HGAppearanceTests.app"
bundle_id=org.honggo.appearance.tests
mkdir -p "$app"
if ! command -v xcrun >/dev/null 2>&1; then
    printf 'Appearance compatibility UIKit integration requires macOS with Xcode and an iOS simulator runtime.\n' >&2
    exit 1
fi
sdk=$(xcrun --sdk iphonesimulator --show-sdk-path)
architecture=$(uname -m)

xcrun --sdk iphonesimulator clang \
    -target "$architecture-apple-ios15.0.1-simulator" -isysroot "$sdk" \
    -O0 -g -fobjc-arc -fblocks -Wall -Werror -I "$source_root/src" \
    -framework Foundation -framework CoreFoundation -framework UIKit -framework CoreText -framework PhotosUI -framework ImageIO -framework UniformTypeIdentifiers -framework QuartzCore -framework CoreGraphics -lobjc \
    "$source_root"/src/*.m "$source_root/tests/AppearanceIntegration.m" \
    -o "$app/HGAppearanceTests" 2>&1 | tee "$work/build.log"

python3 - "$app/Info.plist" "$bundle_id" <<'PY'
import plistlib
import sys
from pathlib import Path

Path(sys.argv[1]).write_bytes(plistlib.dumps({
    "CFBundleIdentifier": sys.argv[2],
    "CFBundleName": "Honggo Appearance Compatibility Tests",
    "CFBundleExecutable": "HGAppearanceTests",
    "CFBundlePackageType": "APPL",
    "CFBundleVersion": "1",
    "CFBundleShortVersionString": "1.0",
    "CFBundleSupportedPlatforms": ["iPhoneSimulator"],
    "LSRequiresIPhoneOS": True,
    "MinimumOSVersion": "15.0",
    "UIDeviceFamily": [1, 2],
    "UILaunchScreen": {},
    "UIApplicationSceneManifest": {"UIApplicationSupportsMultipleScenes": False},
    "UISupportedInterfaceOrientations": ["UIInterfaceOrientationPortrait"],
}))
PY
codesign --force --sign - "$app"
xcrun simctl list --json > "$work/simulators.json"
selection=$(python3 "$source_root/tools/simulator_target.py" "$work/simulators.json" --record "$work/simulator-target.json")
read -r device_type runtime_id <<< "$selection"
device_id=$(xcrun simctl create "HonggoAppearance-$$" "$device_type" "$runtime_id")
cleanup() {
    xcrun simctl terminate "$device_id" "$bundle_id" >/dev/null 2>&1 || true
    xcrun simctl shutdown "$device_id" >/dev/null 2>&1 || true
    xcrun simctl delete "$device_id" >/dev/null 2>&1 || true
}
trap cleanup EXIT
xcrun simctl boot "$device_id"
xcrun simctl bootstatus "$device_id" -b
python3 "$source_root/tools/simulator_target.py" "$work/simulators.json" --record "$work/simulator-target.json" --verify-booted "$device_id"
xcrun simctl install "$device_id" "$app"
data_directory=$(xcrun simctl get_app_container "$device_id" "$bundle_id" data)
rm -f -- "$data_directory/Documents/appearance-results.json"
xcrun simctl launch "$device_id" "$bundle_id" 2>&1 | tee "$work/launch.log"
for ((attempt=0; attempt<45; attempt++)); do
    if [ -f "$data_directory/Documents/appearance-results.json" ]; then
        break
    fi
    sleep 1
done
xcrun simctl spawn "$device_id" log show --style compact --last 2m \
    --predicate 'process == "HGAppearanceTests"' > "$work/simulator.log" 2>&1 || true
if [ ! -f "$data_directory/Documents/appearance-results.json" ]; then
    cat "$work/simulator.log"
    printf 'Appearance compatibility integration app did not write a result within 45 seconds.\n' >&2
    exit 1
fi
cp "$data_directory/Documents/appearance-results.json" "$work/appearance-results.json"
if [ -f "$data_directory/Documents/dock-preview.png" ]; then
    cp "$data_directory/Documents/dock-preview.png" "$work/dock-preview.png"
fi
python3 - "$work/appearance-results.json" <<'PY'
import json
import sys

report = json.load(open(sys.argv[1]))
print(json.dumps(report, indent=2, ensure_ascii=False))
checks = report.get("checks", [])
if report.get("status") != "passed" or report.get("failed") != 0 or \
        report.get("total", 0) < 50 or len(checks) != report.get("total") or \
        any(check.get("passed") is not True for check in checks):
    raise SystemExit("Appearance compatibility UIKit integration checks failed")
PY
