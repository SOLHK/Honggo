#!/usr/bin/env bash
set -euo pipefail

# A real UIKit simulator app with a controlled SSSettingViewController analogue.
# This checks integration behavior, not the hierarchy or signing of the real IPA.
source_root=$(cd "$(dirname "${BASH_SOURCE[0]}")/.." && pwd)
work="$source_root/build/settings-tests"
app="$work/HGSettingsTests.app"
bundle_id=org.honggo.settings.tests
mkdir -p "$app"
sdk=$(xcrun --sdk iphonesimulator --show-sdk-path)
architecture=$(uname -m)

xcrun --sdk iphonesimulator clang \
    -target "$architecture-apple-ios15.0.1-simulator" -isysroot "$sdk" \
    -O0 -g -fobjc-arc -fblocks -Wall -Werror -I "$source_root/src" \
    -framework Foundation -framework UIKit -framework QuartzCore -framework CoreGraphics -lobjc \
    "$source_root/src/Ads.m" "$source_root/src/Honggo.m" \
    "$source_root/src/Settings.m" "$source_root/src/TabBar.m" \
    "$source_root/src/Controls.m" "$source_root/src/Diagnostics.m" \
    "$source_root/src/AdSceneDiagnostics.m" \
    "$source_root/tests/SettingsIntegration.m" -o "$app/HGSettingsTests"

python3 - "$app/Info.plist" "$bundle_id" <<'PY'
import plistlib
import sys
from pathlib import Path

Path(sys.argv[1]).write_bytes(plistlib.dumps({
    "CFBundleIdentifier": sys.argv[2],
    "CFBundleName": "Honggo Settings Tests",
    "CFBundleExecutable": "HGSettingsTests",
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
selection=$(python3 - "$work/simulators.json" <<'PY'
import json
import re
import sys

inventory = json.load(open(sys.argv[1]))
devices = inventory["devices"]
ios = [(tuple(map(int, re.findall(r"\d+", runtime.split(".iOS-")[-1]))), entries)
       for runtime, entries in devices.items() if ".iOS-" in runtime]
for _, entries in sorted(ios, reverse=True):
    phones = [item for item in entries if item.get("isAvailable") and item["name"].startswith("iPhone")]
    if phones:
        device_type = phones[0].get("deviceTypeIdentifier")
        if not device_type:
            device_type = next(item["identifier"] for item in inventory["devicetypes"]
                               if item["name"] == phones[0]["name"])
        runtime = next(key for key, value in devices.items() if value == entries)
        print(device_type, runtime)
        break
else:
    raise SystemExit("No available iPhone simulator runtime is installed")
PY
)
read -r device_type runtime_id <<< "$selection"
device_id=$(xcrun simctl create "HonggoSettings-$$" "$device_type" "$runtime_id")
cleanup() {
    xcrun simctl terminate "$device_id" "$bundle_id" >/dev/null 2>&1 || true
    xcrun simctl shutdown "$device_id" >/dev/null 2>&1 || true
    xcrun simctl delete "$device_id" >/dev/null 2>&1 || true
}
trap cleanup EXIT
xcrun simctl boot "$device_id"
xcrun simctl bootstatus "$device_id" -b
xcrun simctl install "$device_id" "$app"
data_directory=$(xcrun simctl get_app_container "$device_id" "$bundle_id" data)
rm -f -- "$data_directory/Documents/settings-results.json"
xcrun simctl launch "$device_id" "$bundle_id"
for ((attempt=0; attempt<45; attempt++)); do
    if [ -f "$data_directory/Documents/settings-results.json" ]; then
        break
    fi
    sleep 1
done
xcrun simctl spawn "$device_id" log show --style compact --last 2m \
    --predicate 'process == "HGSettingsTests"' > "$work/simulator.log" 2>&1 || true
if [ ! -f "$data_directory/Documents/settings-results.json" ]; then
    printf 'Settings integration app did not write its result within 45 seconds.\n' >&2
    exit 1
fi
cp "$data_directory/Documents/settings-results.json" "$work/settings-results.json"
for name in settings-menu-simulator.png settings-options-simulator.png; do
    if [ -f "$data_directory/Documents/$name" ]; then
        cp "$data_directory/Documents/$name" "$work/$name"
    fi
done
python3 - "$work/settings-results.json" <<'PY'
import json
import sys

report = json.load(open(sys.argv[1]))
print(json.dumps(report, indent=2, ensure_ascii=False))
if report.get("status") != "passed" or report.get("failed") != 0 or report.get("total", 0) < 21:
    raise SystemExit("Settings UIKit integration checks failed")
PY
