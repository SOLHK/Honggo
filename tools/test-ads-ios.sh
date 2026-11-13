#!/usr/bin/env bash
set -euo pipefail

# Execute the production ad installer in a UIKit app, with controlled
# host fixtures. A passing result is not Hongguo device validation.
source_root=$(cd "$(dirname "${BASH_SOURCE[0]}")/.." && pwd)
work="$source_root/build/ads-tests"
app="$work/HGAdsTests.app"
bundle_id=org.honggo.ads.tests
mkdir -p "$app"
if ! command -v xcrun >/dev/null 2>&1; then
    printf 'Ad filtering UIKit integration requires macOS with Xcode and an iOS simulator runtime.\n' >&2
    exit 1
fi
sdk=$(xcrun --sdk iphonesimulator --show-sdk-path)
architecture=$(uname -m)

xcrun --sdk iphonesimulator clang \
    -target "$architecture-apple-ios15.0.1-simulator" -isysroot "$sdk" \
    -O0 -g -fobjc-arc -fblocks -Wall -Werror -I "$source_root/src" \
    -framework Foundation -framework UIKit -framework QuartzCore -framework CoreGraphics -lobjc \
    "$source_root/src/Ads.m" "$source_root/src/Honggo.m" "$source_root/src/Settings.m" "$source_root/src/TabBar.m" "$source_root/src/Controls.m" "$source_root/src/Diagnostics.m" "$source_root/src/AdSceneDiagnostics.m" "$source_root/tests/AdsIntegration.m" \
    -o "$app/HGAdsTests" 2>&1 | tee "$work/build.log"

python3 - "$app/Info.plist" "$bundle_id" <<'PY'
import plistlib
import sys
from pathlib import Path

Path(sys.argv[1]).write_bytes(plistlib.dumps({
    "CFBundleIdentifier": sys.argv[2],
    "CFBundleName": "Honggo Ad Filtering Tests",
    "CFBundleExecutable": "HGAdsTests",
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
ios = [(tuple(map(int, re.findall(r"\d+", runtime.split(".iOS-")[-1]))), runtime, entries)
       for runtime, entries in inventory["devices"].items() if ".iOS-" in runtime]
for _, runtime, entries in sorted(ios, reverse=True):
    phones = [entry for entry in entries
              if entry.get("isAvailable") and entry["name"].startswith("iPhone")]
    if not phones:
        continue
    device_type = phones[0].get("deviceTypeIdentifier")
    if not device_type:
        device_type = next(item["identifier"] for item in inventory["devicetypes"]
                           if item["name"] == phones[0]["name"])
    print(device_type, runtime)
    break
else:
    raise SystemExit("No available iPhone simulator runtime is installed")
PY
)
read -r device_type runtime_id <<< "$selection"
device_id=$(xcrun simctl create "HonggoAds-$$" "$device_type" "$runtime_id")
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
rm -f -- "$data_directory/Documents/ads-results.json"
xcrun simctl launch "$device_id" "$bundle_id" 2>&1 | tee "$work/launch.log"
for ((attempt=0; attempt<45; attempt++)); do
    if [ -f "$data_directory/Documents/ads-results.json" ]; then
        break
    fi
    sleep 1
done
xcrun simctl spawn "$device_id" log show --style compact --last 2m \
    --predicate 'process == "HGAdsTests"' > "$work/simulator.log" 2>&1 || true
if [ ! -f "$data_directory/Documents/ads-results.json" ]; then
    cat "$work/simulator.log"
    printf 'Ad filtering integration app did not write a result within 45 seconds.\n' >&2
    exit 1
fi
cp "$data_directory/Documents/ads-results.json" "$work/ads-results.json"
python3 - "$work/ads-results.json" <<'PY'
import json
import sys

report = json.load(open(sys.argv[1]))
print(json.dumps(report, indent=2, ensure_ascii=False))
checks = report.get("checks", [])
if report.get("status") != "passed" or report.get("failed") != 0 or \
        report.get("total", 0) < 42 or len(checks) != report.get("total") or \
        any(check.get("passed") is not True for check in checks):
    raise SystemExit("Ad filtering UIKit integration checks failed")
PY

