#!/usr/bin/env bash
set -euo pipefail

# Actual UIKit in a controlled host. Test the current code, the disabled feature,
# and the same assertions against the known-bad source as a negative control.
source_root=$(cd "$(dirname "${BASH_SOURCE[0]}")/.." && pwd)
work="$source_root/build/tabbar-tests"
sdk=$(xcrun --sdk iphonesimulator --show-sdk-path)
architecture=$(uname -m)
baseline_ref=65758841996f7e7ce8187c776032b04fd1b0e2c5
mkdir -p "$work/current" "$work/legacy"

git -C "$source_root" show "$baseline_ref:src/TabBar.m" |
    sed 's/"HongguoGlass.h"/"Honggo.h"/' > "$work/legacy/TabBar.m"

build_variant() {
    local variant=$1
    local tabbar_source=$2
    local app="$work/$variant/HGTabBarTests.app"
    local bundle_id="org.honggo.tabbar.tests.$variant"
    mkdir -p "$app"
    xcrun --sdk iphonesimulator clang \
        -target "$architecture-apple-ios15.0.1-simulator" -isysroot "$sdk" \
        -O0 -g -fobjc-arc -fblocks -Wall -Werror -I "$source_root/src" \
        -framework Foundation -framework UIKit -framework QuartzCore -framework CoreGraphics -lobjc \
        "$source_root/src/Ads.m" "$source_root/src/Honggo.m" \
        "$source_root/src/Settings.m" "$tabbar_source" \
        "$source_root/tests/TabBarIntegration.m" -o "$app/HGTabBarTests"
    python3 - "$app/Info.plist" "$bundle_id" <<'PY'
import plistlib
import sys
from pathlib import Path

Path(sys.argv[1]).write_bytes(plistlib.dumps({
    "CFBundleIdentifier": sys.argv[2],
    "CFBundleName": "Honggo",
    "CFBundleExecutable": "HGTabBarTests",
    "CFBundlePackageType": "APPL",
    "CFBundleVersion": "1",
    "CFBundleShortVersionString": "1.0",
    "CFBundleSupportedPlatforms": ["iPhoneSimulator"],
    "LSRequiresIPhoneOS": True,
    "MinimumOSVersion": "15.0",
    "UIDeviceFamily": [1, 2],
    "UILaunchScreen": {},
    # The supported IPA opts into compatibility globally. Explicit glass must
    # still be validated under that condition; do not alter the host-wide flag.
    "UIDesignRequiresCompatibility": True,
    "UIApplicationSceneManifest": {"UIApplicationSupportsMultipleScenes": False},
    "UISupportedInterfaceOrientations": ["UIInterfaceOrientationPortrait"],
}))
PY
    codesign --force --sign - "$app"
}

build_variant current "$source_root/src/TabBar.m"
build_variant legacy "$work/legacy/TabBar.m"
# A separate test application changes only the global compatibility flag and
# bundle identity. Its --native-reference mode does not install Honggo hooks.
mkdir -p "$work/reference-modern/HGTabBarTests.app"
cp -R "$work/current/HGTabBarTests.app/." "$work/reference-modern/HGTabBarTests.app/"
python3 - "$work/reference-modern/HGTabBarTests.app/Info.plist" <<'PY'
import plistlib
import sys
from pathlib import Path

path = Path(sys.argv[1])
info = plistlib.loads(path.read_bytes())
info["CFBundleIdentifier"] = "org.honggo.tabbar.tests.reference-modern"
info["UIDesignRequiresCompatibility"] = False
path.write_bytes(plistlib.dumps(info))
PY
codesign --force --sign - "$work/reference-modern/HGTabBarTests.app"
xcrun simctl list --json > "$work/simulators.json"
selection=$(python3 - "$work/simulators.json" <<'PY'
import json
import re
import sys

inventory = json.load(open(sys.argv[1]))
devices = inventory["devices"]
ios = [(tuple(map(int, re.findall(r"\d+", runtime.split(".iOS-")[-1]))), runtime, entries)
       for runtime, entries in devices.items() if ".iOS-" in runtime]
for _, runtime, entries in sorted(ios, reverse=True):
    phones = [item for item in entries if item.get("isAvailable") and item["name"].startswith("iPhone")]
    if phones:
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
device_id=$(xcrun simctl create "HonggoTabBar-$$" "$device_type" "$runtime_id")
cleanup() {
    xcrun simctl shutdown "$device_id" >/dev/null 2>&1 || true
    xcrun simctl delete "$device_id" >/dev/null 2>&1 || true
}
trap cleanup EXIT
xcrun simctl boot "$device_id"
xcrun simctl bootstatus "$device_id" -b

run_variant() {
    local variant=$1
    local result_name=$2
    shift 2
    local bundle_id="org.honggo.tabbar.tests.$variant"
    local app="$work/$variant/HGTabBarTests.app"
    xcrun simctl terminate "$device_id" "$bundle_id" >/dev/null 2>&1 || true
    xcrun simctl install "$device_id" "$app"
    local data_directory
    data_directory=$(xcrun simctl get_app_container "$device_id" "$bundle_id" data)
    rm -f -- "$data_directory/Documents/$result_name" \
        "$data_directory/Documents/tabbar-capture-phase.txt" \
        "$data_directory/Documents/tabbar-capture-ack.txt"
    xcrun simctl launch "$device_id" "$bundle_id" "$@"
    local captured_phase=
    for ((attempt=0; attempt<120; attempt++)); do
        if [ -f "$data_directory/Documents/$result_name" ]; then
            break
        fi
        if [ -f "$data_directory/Documents/tabbar-capture-phase.txt" ]; then
            local phase
            phase=$(tr -d '\r\n' < "$data_directory/Documents/tabbar-capture-phase.txt")
            case "$phase" in
                tabbar-light|tabbar-dark|tabbar-scroll|tabbar-disabled|native-reference-light|native-reference-dark)
                    if [ "$phase" != "$captured_phase" ]; then
                        xcrun simctl io "$device_id" screenshot "$work/$variant/$phase-system-simulator.png"
                        printf '%s\n' "$phase" > "$data_directory/Documents/tabbar-capture-ack.txt"
                        captured_phase=$phase
                    fi
                    ;;
                *)
                    printf 'Unexpected screenshot phase: %s\n' "$phase" >&2
                    return 1
                    ;;
            esac
        fi
        sleep 1
    done
    xcrun simctl spawn "$device_id" log show --style compact --last 3m \
        --predicate 'process == "HGTabBarTests"' > "$work/$variant/simulator.log" 2>&1 || true
    if [ ! -f "$data_directory/Documents/$result_name" ]; then
        printf 'Tab-bar integration app (%s) did not finish within 120 polling iterations.\n' "$variant" >&2
        return 1
    fi
    cp "$data_directory/Documents/$result_name" "$work/$variant/$result_name"
    for file in "$data_directory/Documents/"*-simulator.png; do
        if [ -f "$file" ]; then
            cp "$file" "$work/$variant/"
        fi
    done
    xcrun simctl terminate "$device_id" "$bundle_id" >/dev/null 2>&1 || true
}

run_variant current tabbar-results.json
run_variant current tabbar-disabled-results.json --glass-off
run_variant legacy tabbar-results.json --legacy-control
run_variant reference-modern tabbar-reference-results.json --native-reference

python3 - "$work" <<'PY'
import json
import sys
from pathlib import Path

root = Path(sys.argv[1])
reports = {}
for name, path in {
    "current": root / "current/tabbar-results.json",
    "disabled": root / "current/tabbar-disabled-results.json",
    "legacy": root / "legacy/tabbar-results.json",
    "reference-modern": root / "reference-modern/tabbar-reference-results.json",
}.items():
    report = json.loads(path.read_text())
    reports[name] = report
    print(name)
    print(json.dumps(report, indent=2, ensure_ascii=False))

failed = []
for name, minimum in (("current", 48), ("disabled", 7), ("reference-modern", 2)):
    report = reports[name]
    if report.get("status") != "passed" or report.get("failed") != 0 or report.get("total", 0) < minimum:
        failed.append(f"{name} UIKit integration checks failed")

for name, expected in {
    "current": ("tabbar-light", "tabbar-dark", "tabbar-scroll", "native-reference-light", "native-reference-dark"),
    "disabled": ("tabbar-disabled",),
    "legacy": ("tabbar-light", "tabbar-dark", "tabbar-scroll", "native-reference-light", "native-reference-dark"),
    "reference-modern": ("native-reference-light", "native-reference-dark"),
}.items():
    directory = root / ("current" if name == "disabled" else name)
    acknowledged = reports[name].get("systemScreenshotsAcknowledged", [])
    if list(expected) != acknowledged or any(
        not (directory / f"{phase}-system-simulator.png").is_file() for phase in expected
    ):
        failed.append(f"{name} did not provide all runloop-settled simctl screenshots")

if reports["current"].get("UIDesignRequiresCompatibility") is not True or \
        reports["reference-modern"].get("UIDesignRequiresCompatibility") is not False or \
        reports["reference-modern"].get("nativeReferenceOnly") is not True:
    failed.append("native compatibility comparison does not have the intended isolated test configuration")

# The old code has an effect view too. Requiring these behavior failures proves
# the regression suite is sensitive to the actual occlusion and viewport bugs.
legacy = reports["legacy"]
legacy_checks = {item["name"]: item["passed"] for item in legacy.get("checks", [])}
expected_failures = (
    "nested content and wrapper backgrounds no longer cover the glass",
    "known nested host backgrounds are suppressed",
    "the main scroll viewport extends behind the entire floating bar",
)
if legacy.get("failed", 0) < 3 or any(legacy_checks.get(name) is not False for name in expected_failures):
    failed.append("legacy negative control did not expose the known overlay and viewport failures")
summary = {
    "status": "failed" if failed else "passed",
    "scope": "UIKit simulator plus known-bad-source negative control; not the Hongguo IPA or a physical device",
    "currentTotal": reports["current"].get("total"),
    "currentFailed": reports["current"].get("failed"),
    "disabledTotal": reports["disabled"].get("total"),
    "disabledFailed": reports["disabled"].get("failed"),
    "legacyDetectedFailures": legacy.get("failed"),
    "nativeGlassAvailable": reports["current"].get("nativeGlassAvailable"),
    "referenceModernTotal": reports["reference-modern"].get("total"),
    "referenceModernFailed": reports["reference-modern"].get("failed"),
    "systemScreenshotPhases": {name: report.get("systemScreenshotsAcknowledged", [])
                               for name, report in reports.items()},
    "errors": failed,
}
(root / "tabbar-suite-results.json").write_text(json.dumps(summary, indent=2, ensure_ascii=False) + "\n")
print(json.dumps(summary, indent=2, ensure_ascii=False))
if failed:
    raise SystemExit("; ".join(failed))
PY
