#!/usr/bin/env bash
set -euo pipefail

# Actual UIKit in a controlled host. Test the current code, the disabled feature,
# and the same assertions against the known-bad source as a negative control.
source_root=$(cd "$(dirname "${BASH_SOURCE[0]}")/.." && pwd)
work="$source_root/build/tabbar-tests"
sdk=$(xcrun --sdk iphonesimulator --show-sdk-path)
architecture=$(uname -m)
baseline_ref=65758841996f7e7ce8187c776032b04fd1b0e2c5
background_reset_baseline_ref=5d16b2411c262313b58585b5920e83599f2a0e99
mkdir -p "$work/current" "$work/legacy" "$work/background-baseline"

git -C "$source_root" show "$baseline_ref:src/TabBar.m" |
    sed 's/"HongguoGlass.h"/"Honggo.h"/' > "$work/legacy/TabBar.m"
git -C "$source_root" show "$background_reset_baseline_ref:src/TabBar.m" > "$work/background-baseline/TabBar.m"

build_variant() {
    local variant=$1
    local tabbar_source=$2
    local app="$work/$variant/HGTabBarTests.app"
    local bundle_id="org.honggo.tabbar.tests.$variant"
    mkdir -p "$app"
    # Historical TabBar implementations predate the title ownership accessor.
    # Keep their foreground behavior unchanged when linking current Controls.
    local title_compat=()
    if [ "$variant" != current ]; then
        printf '#import "Honggo.h"\nUIColor *HGOriginalTabTitleColor(UILabel *label) { return label.textColor; }\n' > "$work/$variant/TitleCompat.m"
        title_compat=("$work/$variant/TitleCompat.m")
    fi
    xcrun --sdk iphonesimulator clang \
        -target "$architecture-apple-ios15.0.1-simulator" -isysroot "$sdk" \
        -O0 -g -fobjc-arc -fblocks -Wall -Werror -I "$source_root/src" \
        -framework Foundation -framework CoreFoundation -framework UIKit -framework QuartzCore -framework CoreGraphics -lobjc \
        "$source_root/src/Ads.m" "$source_root/src/Honggo.m" \
        "$source_root/src/Settings.m" "$tabbar_source" \
        "$source_root/src/Controls.m" "$source_root/src/Diagnostics.m" \
        "$source_root/tests/ControlsIntegration.m" \
        "$source_root/tests/TabBarIntegration.m" "${title_compat[@]}" -o "$app/HGTabBarTests"
    python3 - "$app/Info.plist" "$bundle_id" "$source_root" <<'PY'
import plistlib
import sys
from pathlib import Path

sys.path.insert(0, sys.argv[3])
from tools.prepare_ipa import configure_native_glass

original_info = {
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
    # Start from the original host's old design, then consume the exact shared
    # metadata transformation used by the real pre-signing IPA preparation.
    "UIDesignRequiresCompatibility": True,
    "UIApplicationSceneManifest": {"UIApplicationSupportsMultipleScenes": False},
    "UISupportedInterfaceOrientations": ["UIInterfaceOrientationPortrait"],
}
prepared_info = configure_native_glass(original_info)
assert original_info["UIDesignRequiresCompatibility"] is True
assert prepared_info["UIDesignRequiresCompatibility"] is False
Path(sys.argv[1]).write_bytes(plistlib.dumps(prepared_info))
PY
    codesign --force --sign - "$app"
}

build_variant current "$source_root/src/TabBar.m"
build_variant legacy "$work/legacy/TabBar.m"
build_variant background-baseline "$work/background-baseline/TabBar.m"
mkdir -p "$work/nested-baseline"
git -C "$source_root" show 78a17ad2e2f1b8ee458d0885a37fab67b5f2f09f:src/TabBar.m > "$work/nested-baseline/TabBar.m"
build_variant nested-baseline "$work/nested-baseline/TabBar.m"
mkdir -p "$work/profile-baseline"
git -C "$source_root" show 466599abf81b8c54b1863332593aebf6f2d9127d:src/TabBar.m > "$work/profile-baseline/TabBar.m"
build_variant profile-baseline "$work/profile-baseline/TabBar.m"
mkdir -p "$work/return-baseline"
git -C "$source_root" show aea27775ef9c0ef6ee49ab153d18641459801af9:src/TabBar.m > "$work/return-baseline/TabBar.m"
build_variant return-baseline "$work/return-baseline/TabBar.m"
# Reuse the compiled test executable. Reference modes install no Honggo hooks;
# the unprepared fallback runs only its compact behavior and compositor checks.
clone_variant() {
    local variant=$1
    local preparation=$2
    local app="$work/$variant/HGTabBarTests.app"
    mkdir -p "$app"
    cp -R "$work/current/HGTabBarTests.app/." "$app/"
    python3 - "$app/Info.plist" "$variant" "$preparation" "$source_root" <<'PY'
import plistlib
import sys
from pathlib import Path

sys.path.insert(0, sys.argv[4])
from tools.prepare_ipa import configure_native_glass

path = Path(sys.argv[1])
info = plistlib.loads(path.read_bytes())
info["CFBundleIdentifier"] = f"org.honggo.tabbar.tests.{sys.argv[2]}"
if sys.argv[3] == "prepared":
    info = configure_native_glass(info)
else:
    info["UIDesignRequiresCompatibility"] = True
path.write_bytes(plistlib.dumps(info))
PY
    codesign --force --sign - "$app"
}
clone_variant reference-modern prepared
clone_variant reference-compat unprepared
clone_variant fallback unprepared
clone_variant startup unprepared
clone_variant background-current prepared
clone_variant nested-current prepared
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
    local phase_count=7
    case "$result_name:$variant" in
        tabbar-background-reset-results.json:*|tabbar-nested-results.json:*) phase_count=0 ;;
        tabbar-disabled-results.json:*) phase_count=1 ;;
        tabbar-reference-results.json:*|tabbar-fallback-results.json:*) phase_count=2 ;;
        tabbar-results.json:legacy) phase_count=5 ;;
    esac
    # Each scene has a bounded 60-second capture budget, plus two minutes for
    # application startup. The full seven-scene host is bounded at 540 seconds.
    local run_timeout=$((phase_count * 60 + 120))
    local run_deadline=$((SECONDS + run_timeout))
    while ((SECONDS < run_deadline)); do
        if [ -f "$data_directory/Documents/$result_name" ]; then
            break
        fi
        if [ -f "$data_directory/Documents/tabbar-capture-phase.txt" ]; then
            local phase
            phase=$(tr -d '\r\n' < "$data_directory/Documents/tabbar-capture-phase.txt")
            case "$phase" in
                tabbar-light|tabbar-dark|tabbar-scroll|tabbar-disabled|tabbar-material-light|tabbar-material-dark|native-reference-light|native-reference-dark)
                    if [ "$phase" != "$captured_phase" ]; then
                        captured_phase=$phase
                        if python3 - "$device_id" "$work/$variant/$phase-system-simulator.png" "$phase" <<'PY'
import subprocess
import sys
import time

started = time.monotonic()
try:
    subprocess.run(["xcrun", "simctl", "io", sys.argv[1], "screenshot", sys.argv[2]],
                   check=True, timeout=55)
except (subprocess.CalledProcessError, subprocess.TimeoutExpired) as error:
    print(f"Capture failed for {sys.argv[3]}: {error}", file=sys.stderr)
    raise SystemExit(1)
print(f"Capture completed for {sys.argv[3]} in {time.monotonic() - started:.3f}s")
PY
                        then
                            local current_phase
                            current_phase=$(tr -d '\r\n' < "$data_directory/Documents/tabbar-capture-phase.txt")
                            if [ "$current_phase" != "$phase" ]; then
                                printf 'Capture phase changed before completion: %s -> %s\n' "$phase" "$current_phase" >&2
                                return 1
                            fi
                            if [ -f "$data_directory/Documents/$result_name" ]; then
                                printf 'Capture completed after the app stopped waiting: %s\n' "$phase" >&2
                                break
                            fi
                            # Acknowledge only the completed image for the still
                            # displayed phase. A failed capture receives no ACK.
                            printf '%s\n' "$phase" > "$data_directory/Documents/tabbar-capture-ack.txt"
                        fi
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
        printf 'Tab-bar integration app (%s) did not finish within its %s-second capture budget.\n' "$variant" "$run_timeout" >&2
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

run_variant nested-current tabbar-nested-results.json --nested-only
python3 - "$work/nested-current/tabbar-nested-results.json" <<'PY'
import json, sys
report = json.load(open(sys.argv[1]))
assert report['failed'] == 0, report
PY
run_variant current tabbar-results.json
run_variant current tabbar-disabled-results.json --glass-off
run_variant legacy tabbar-results.json --legacy-control
run_variant reference-modern tabbar-reference-results.json --native-reference
run_variant reference-compat tabbar-reference-results.json --native-reference
run_variant fallback tabbar-fallback-results.json --compat-fallback
run_variant background-current tabbar-background-reset-results.json --background-reset-only
run_variant background-baseline tabbar-background-reset-results.json --background-reset-only
run_variant nested-baseline tabbar-nested-results.json --nested-only
run_variant profile-baseline tabbar-nested-results.json --nested-only
run_variant return-baseline tabbar-nested-results.json --nested-only
run_variant startup tabbar-results.json --startup-override

python3 - "$work" <<'PY'
import json, sys
from pathlib import Path
root = Path(sys.argv[1])
current = json.loads((root / 'nested-current/tabbar-nested-results.json').read_text())
assert current['failed'] == 0, current
baseline = json.loads((root / 'return-baseline/tabbar-nested-results.json').read_text())
name = 'nested scrolling does not install viewport observers'
matches = [x for x in baseline['checks'] if x['name'] == name]
assert len(matches) == 2 and all(x['passed'] is False for x in matches), baseline
print('Nested viewport ownership checks passed; previous source installs observers as expected.')

PY

python3 - "$work" <<'PY'
import json
import statistics
import struct
import sys
import zlib
from pathlib import Path

root = Path(sys.argv[1])
reports = {}
for name, path in {
    "current": root / "current/tabbar-results.json",
    "startup": root / "startup/tabbar-results.json",
    "disabled": root / "current/tabbar-disabled-results.json",
    "legacy": root / "legacy/tabbar-results.json",
    "reference-modern": root / "reference-modern/tabbar-reference-results.json",
    "reference-compat": root / "reference-compat/tabbar-reference-results.json",
    "fallback": root / "fallback/tabbar-fallback-results.json",
    "background-current": root / "background-current/tabbar-background-reset-results.json",
    "background-baseline": root / "background-baseline/tabbar-background-reset-results.json",
}.items():
    report = json.loads(path.read_text())
    reports[name] = report
    print(name)
    print(json.dumps(report, indent=2, ensure_ascii=False))

failed = []
for name, minimum in (("current", 59), ("startup", 61), ("disabled", 7), ("reference-modern", 2),
                      ("reference-compat", 2), ("fallback", 10), ("background-current", 14)):
    report = reports[name]
    if report.get("status") != "passed" or report.get("failed") != 0 or report.get("total", 0) < minimum:
        failed.append(f"{name} UIKit integration checks failed")

for name, expected in {
    "current": ("tabbar-light", "tabbar-dark", "tabbar-scroll", "tabbar-material-light", "tabbar-material-dark",
                "native-reference-light", "native-reference-dark"),
    "startup": ("tabbar-light", "tabbar-dark", "tabbar-scroll", "tabbar-material-light", "tabbar-material-dark",
                "native-reference-light", "native-reference-dark"),
    "disabled": ("tabbar-disabled",),
    "legacy": ("tabbar-light", "tabbar-dark", "tabbar-scroll", "native-reference-light", "native-reference-dark"),
    "reference-modern": ("native-reference-light", "native-reference-dark"),
    "reference-compat": ("native-reference-light", "native-reference-dark"),
    "fallback": ("tabbar-material-light", "tabbar-material-dark"),
    "background-current": (),
    "background-baseline": (),
}.items():
    directory = root / ("current" if name == "disabled" else name)
    acknowledged = reports[name].get("systemScreenshotsAcknowledged", [])
    if list(expected) != acknowledged or any(
        not (directory / f"{phase}-system-simulator.png").is_file() for phase in expected
    ):
        failed.append(f"{name} did not provide all runloop-settled simctl screenshots")

if reports["current"].get("UIDesignRequiresCompatibility") is not False or \
        reports["startup"].get("UIDesignRequiresCompatibility") is not True or \
        reports["startup"].get("startupGlassOverride") is not True or \
        reports["disabled"].get("UIDesignRequiresCompatibility") is not False or \
        reports["legacy"].get("UIDesignRequiresCompatibility") is not False or \
        reports["reference-modern"].get("UIDesignRequiresCompatibility") is not False or \
        reports["reference-modern"].get("nativeReferenceOnly") is not True or \
        reports["reference-compat"].get("UIDesignRequiresCompatibility") is not True or \
        reports["reference-compat"].get("nativeReferenceOnly") is not True or \
        reports["fallback"].get("UIDesignRequiresCompatibility") is not True or \
        reports["fallback"].get("compatibilityFallbackOnly") is not True:
    failed.append("prepared, compatibility-negative, and fallback modes do not have their intended isolated metadata")

# Run exactly the same immediate-state assertions against the candidate and
# frozen 1.0.5 source. No image capture or layout may conceal a setter-time leak.
reset_names = (
    "background image hidden reset is suppressed before its setter returns",
    "light background hidden reset is suppressed before its setter returns",
    "rebuilt image hidden reset is suppressed before its setter returns",
    "rebuilt light hidden reset is suppressed before its setter returns",
)
reset_reports = {name: reports[name] for name in ("background-current", "background-baseline")}
reset_checks = {name: {item["name"]: item["passed"] for item in report.get("checks", [])}
                for name, report in reset_reports.items()}
if list(reset_checks["background-current"]) != list(reset_checks["background-baseline"]):
    failed.append("background-reset candidate and 1.0.5 control did not execute identical assertions")
for name, report in reset_reports.items():
    if report.get("backgroundResetOnly") is not True or report.get("UIDesignRequiresCompatibility") is not False or \
            report.get("drawCaptureResults") != [] or report.get("systemCaptureEvents") != [] or \
            report.get("total") != 14:
        failed.append(f"{name} did not honor the 14-check synchronous no-capture mode")
    fixture = "background reset fixture starts with live glass and attached suppressed backgrounds"
    if reset_checks[name].get(fixture) is not True:
        failed.append(f"{name} did not establish the suppressed-background precondition")
    snapshot = next((item for item in report.get("snapshots", [])
                     if item.get("phase") == "background-reset-setter-returns"), {})
    if any(snapshot.get(key) != 0 for key in ("barLayoutDelta", "hostLayoutDelta", "appearanceDelta")):
        failed.append(f"{name} relied on a layout or appearance callback during background setters")
if reset_reports["background-baseline"].get("status") != "failed" or \
        reset_reports["background-baseline"].get("failed", 0) < len(reset_names) or any(
            reset_checks["background-baseline"].get(name) is not False or
            reset_checks["background-current"].get(name) is not True for name in reset_names):
    failed.append("1.0.5 background-reset negative control did not expose all four immediate setter leaks")

# Decode simctl's 8-bit RGB/RGBA PNGs using only the standard library. Compare
# pixels from the system compositor, not drawViewHierarchy or object properties.
def read_system_png(path):
    data = path.read_bytes()
    if data[:8] != b"\x89PNG\r\n\x1a\n":
        raise ValueError("not a PNG image")
    offset, compressed, header = 8, [], None
    while offset < len(data):
        size = struct.unpack_from(">I", data, offset)[0]
        kind = data[offset + 4:offset + 8]
        payload = data[offset + 8:offset + 8 + size]
        offset += size + 12
        if kind == b"IHDR":
            header = struct.unpack(">IIBBBBB", payload)
        elif kind == b"IDAT":
            compressed.append(payload)
        elif kind == b"IEND":
            break
    if not header:
        raise ValueError("missing PNG header")
    width, height, depth, color, compression, filtering, interlace = header
    if depth != 8 or color not in (2, 6) or (compression, filtering, interlace) != (0, 0, 0):
        raise ValueError(f"unsupported system PNG encoding: {header}")
    channels = 4 if color == 6 else 3
    stride = width * channels
    raw = zlib.decompress(b"".join(compressed))
    if len(raw) != height * (stride + 1):
        raise ValueError("unexpected decoded PNG size")
    rows, previous = [], bytes(stride)
    for y in range(height):
        start = y * (stride + 1)
        method = raw[start]
        row = bytearray(raw[start + 1:start + 1 + stride])
        if method == 1:
            for x in range(channels, stride):
                row[x] = (row[x] + row[x - channels]) & 255
        elif method == 2:
            row = bytearray((value + previous[x]) & 255 for x, value in enumerate(row))
        elif method in (3, 4):
            for x in range(stride):
                left = row[x - channels] if x >= channels else 0
                above = previous[x]
                upper_left = previous[x - channels] if x >= channels else 0
                if method == 3:
                    prediction = (left + above) // 2
                else:
                    p = left + above - upper_left
                    a, b, c = abs(p - left), abs(p - above), abs(p - upper_left)
                    prediction = left if a <= b and a <= c else above if b <= c else upper_left
                row[x] = (row[x] + prediction) & 255
        elif method != 0:
            raise ValueError(f"unknown PNG row filter {method}")
        previous = bytes(row)
        rows.append(previous)
    return width, height, channels, rows


def compare_material_pixels(directory, snapshot, *, host_material=False, expect_unsupported=False):
    phase = snapshot["phase"]
    width, height, channels, rows = read_system_png(directory / f"{phase}-system-simulator.png")
    window_width, window_height = snapshot["windowSizePoints"]
    scale_x, scale_y = width / window_width, height / window_height
    host_blur = host_material and snapshot["expectedEffectClass"] == "UIBlurEffect"
    keys = (("plugin", "blurReference", "nilClear") if host_blur else ("plugin", "nilClear")) \
        if host_material else ("regular", "clear", "regularClearAfter", "nilClear")
    material_key = "plugin" if host_material else "regularClearAfter"
    frames = {key: snapshot[key]["framePoints"] for key in keys}
    baseline = frames["nilClear"]
    if any(frame[0] != baseline[0] or frame[2:] != baseline[2:] for frame in frames.values()):
        raise ValueError("reference capsules do not have identical horizontal geometry")

    def pixel_at(key, local_x, local_y):
        frame_x, frame_y, _, _ = frames[key]
        pixel_x = int((frame_x + local_x) * scale_x)
        pixel_y = int((frame_y + local_y) * scale_y)
        if not (0 <= pixel_x < width and 0 <= pixel_y < height):
            raise ValueError("reference sample falls outside the system screenshot")
        return tuple(rows[pixel_y][pixel_x * channels:pixel_x * channels + 3])

    def rgb_mae(first, second):
        return sum(abs(a - b) for pixel, original in zip(first, second)
                   for a, b in zip(pixel, original)) / (3 * len(first))

    samples = {key: [] for key in keys}
    # Avoid labels (centered vertically) and rounded corners. All four views
    # share the same full-height vertical stripe background at each x position.
    bands = snapshot["pixelComparisonSampleBandsPoints"]
    for start_y, end_y in bands:
        for local_y in range(start_y, end_y):
            for local_x in range(32, int(baseline[2]) - 32):
                for key in keys:
                    samples[key].append(pixel_at(key, local_x + 0.5, local_y + 0.5))
    if not samples["nilClear"]:
        raise ValueError("reference sample region is empty")
    metrics = {}
    for key, pixels in samples.items():
        differences = [tuple(abs(a - b) for a, b in zip(pixel, original))
                       for pixel, original in zip(pixels, samples["nilClear"])]
        metrics[key] = {
            "meanAbsoluteRGBDifferenceFromNil": sum(map(sum, differences)) / (3 * len(differences)),
            "fractionWithChannelDifferenceAtLeast8": sum(max(item) >= 8 for item in differences) / len(differences),
            "maxChannelStandardDeviation": max(statistics.pstdev(pixel[channel] for pixel in pixels)
                                               for channel in range(3)),
            "sampleCount": len(pixels),
        }
    material = metrics[material_key]
    # Deliberately conservative evidence gates: a transparent empty shell has
    # no interior difference; a solid fill erases the stripe color variation.
    # These checks do not claim subjective visual quality or physical-device QA.
    visible_material = (material["meanAbsoluteRGBDifferenceFromNil"] > 1.5 and
                        material["fractionWithChannelDifferenceAtLeast8"] > 0.05 and
                        material["maxChannelStandardDeviation"] > 8)
    valid_controls = (snapshot["nilClear"]["effectClass"] == "none" and
                      snapshot["nilClear"]["backgroundClear"] and
                      snapshot["nilClear"]["contentBackgroundClear"] and
                      snapshot["nilClear"]["viewClass"] == "UIVisualEffectView" and
                      snapshot[material_key]["effectClass"] != "none" and
                      snapshot[material_key]["backgroundClear"] and
                      snapshot[material_key]["contentBackgroundClear"])
    unsupported_evidence = None
    blur_comparison = None
    corner_comparison = None
    if host_material:
        boolean_keys = [
            "effectMatchesExpectedClass", "hostInstalledSurface", "materialBelowHostContent",
            "originalContentParent", "originalButtonParent", "originalControlsVisible"]
        if host_blur:
            boolean_keys.extend(("blurReferenceMatchesExpectedClass", "blurReferenceMatchesForeground"))
        valid_controls = valid_controls and all(snapshot.get(key) is True and
            snapshot.get("semanticBooleanJSONTypes", {}).get(key) == "boolean" for key in boolean_keys)
        valid_controls = valid_controls and snapshot["foregroundStyle"] == (2 if phase.endswith("-dark") else 1)
        expected = "visible-host-blur" if host_blur else "visible-host-native-glass"
        if host_blur:
            valid_controls = valid_controls and snapshot["blurReference"]["viewClass"] == "UIVisualEffectView" and \
                snapshot["blurReference"]["backgroundClear"] and snapshot["blurReference"]["contentBackgroundClear"]
            reference = samples["blurReference"]
            best_solid = tuple(statistics.median(pixel[channel] for pixel in reference) for channel in range(3))
            error_to_reference = rgb_mae(samples["plugin"], reference)
            separation_from_nil = rgb_mae(reference, samples["nilClear"])
            separation_from_solid = rgb_mae(reference, [best_solid] * len(reference))
            # The coordinate-wise median is the best possible constant under
            # L1 error. Neither a nil surface nor ANY solid fill can satisfy
            # being twice as close to this independent UIKit reference.
            visible_material = error_to_reference <= 2 and \
                error_to_reference + 0.5 < 0.5 * min(separation_from_nil, separation_from_solid)
            blur_comparison = {
                "actualToSystemMaterialMAE": error_to_reference,
                "systemMaterialToNilMAE": separation_from_nil,
                "systemMaterialToBestSolidMAE": separation_from_solid,
                "bestSolidRGB": best_solid,
                "maximumReferenceError": 2,
                "quantizationMargin": 0.5,
                "requiredRelativeCloseness": 0.5,
                "matchesIndependentSystemMaterial": visible_material,
            }
            corner_comparison = []
            for name, right, bottom in (("top-left", False, False), ("top-right", True, False),
                                        ("bottom-left", False, True), ("bottom-right", True, True)):
                positions = [(baseline[2] - x if right else x, baseline[3] - y if bottom else y)
                             for y in (0.5, 1.5) for x in (0.5, 1.5)]
                corners = {key: [pixel_at(key, x, y) for x, y in positions] for key in keys}
                actual_error = rgb_mae(corners["plugin"], corners["nilClear"])
                reference_error = rgb_mae(corners["blurReference"], corners["nilClear"])
                corner_comparison.append({"corner": name, "actualToNilMAE": actual_error,
                    "referenceToNilMAE": reference_error, "passed": actual_error <= 2 and reference_error <= 2})
            passes = valid_controls and visible_material and all(corner["passed"] for corner in corner_comparison)
        else:
            passes = valid_controls and visible_material
    elif expect_unsupported:
        # Retain the failed 1.0.3 experiment as a strict prerequisite negative
        # control. In compatibility mode, clearing the fallback fill must be
        # observed to produce an empty shell, not counted as usable glass.
        defaults_are_solid = all(metrics[key]["maxChannelStandardDeviation"] < 1 and
                                 snapshot[key]["surfaceColors"]["background"]["resolvedAlpha"] > 0.99
                                 for key in ("regular", "clear"))
        cleared_is_empty = (material["meanAbsoluteRGBDifferenceFromNil"] <= 0.5 and
                            material["fractionWithChannelDifferenceAtLeast8"] <= 0.001)
        unsupported_evidence = {"originalRegularAndClearAreSolid": defaults_are_solid,
                                "clearAfterMatchesNilControl": cleared_is_empty}
        expected = "unsupported-native-clears-to-empty-shell"
        passes = valid_controls and defaults_are_solid and cleared_is_empty and not visible_material
    else:
        expected = "visible-public-native-glass"
        passes = valid_controls and visible_material
    return {
        "phase": phase,
        "UIDesignRequiresCompatibility": snapshot["UIDesignRequiresCompatibility"],
        "systemScreenshot": f"{phase}-system-simulator.png",
        "sampleBandsPoints": bands,
        "horizontalMarginPoints": 32,
        "metrics": metrics,
        "expectedOutcome": expected,
        "unsupportedNativeEvidence": unsupported_evidence,
        "systemBlurComparison": blur_comparison,
        "cornerPixels": corner_comparison,
        "validMaterialAndNilControls": valid_controls,
        "hasVisibleMaterialOverTransparentControl": visible_material,
        "passed": passes,
    }


pixel_comparisons = {}
for name in ("current", "startup", "reference-modern", "reference-compat"):
    comparisons = []
    for phase in ("native-reference-light", "native-reference-dark"):
        try:
            snapshot = next(item for item in reports[name]["snapshots"] if item["phase"] == phase)
            comparison = compare_material_pixels(root / name, snapshot, expect_unsupported=name == "reference-compat")
        except (OSError, ValueError, KeyError, TypeError, StopIteration, struct.error, zlib.error) as error:
            comparison = {"phase": phase, "passed": False, "error": str(error)}
        comparisons.append(comparison)
        if not comparison["passed"]:
            failed.append(f"{name} {phase}: native material does not satisfy its positive/negative compositor expectation")
    pixel_comparisons[name] = comparisons
    result = {"status": "passed" if all(item["passed"] for item in comparisons) else "failed",
              "scope": "simctl screenshot interior pixels against the same-background nil-effect control",
              "comparisons": comparisons}
    (root / name / "native-pixels-results.json").write_text(json.dumps(result, indent=2) + "\n")
    print(name + " native compositor pixels")
    print(json.dumps(result, indent=2))

host_pixel_comparisons = {}
for name in ("current", "startup", "fallback"):
    comparisons = []
    for phase in ("tabbar-material-light", "tabbar-material-dark"):
        try:
            snapshot = next(item for item in reports[name]["snapshots"] if item["phase"] == phase)
            comparison = compare_material_pixels(root / name, snapshot, host_material=True)
        except (OSError, ValueError, KeyError, TypeError, StopIteration, struct.error, zlib.error) as error:
            comparison = {"phase": phase, "passed": False, "error": str(error)}
        comparisons.append(comparison)
        if not comparison["passed"]:
            failed.append(f"{name} {phase}: actual host material is not visibly distinct from a transparent or solid surface")
    host_pixel_comparisons[name] = comparisons
    result = {"status": "passed" if all(item["passed"] for item in comparisons) else "failed",
              "scope": "actual Honggo backdrop installed through SSTabBar hooks, beside a nil-effect control",
              "comparisons": comparisons}
    (root / name / "host-material-pixels-results.json").write_text(json.dumps(result, indent=2) + "\n")
    print(name + " actual host compositor pixels")
    print(json.dumps(result, indent=2))

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
    "referenceCompatibilityTotal": reports["reference-compat"].get("total"),
    "referenceCompatibilityFailed": reports["reference-compat"].get("failed"),
    "fallbackTotal": reports["fallback"].get("total"),
    "fallbackFailed": reports["fallback"].get("failed"),
    "backgroundResetCurrentTotal": reports["background-current"].get("total"),
    "backgroundResetCurrentFailed": reports["background-current"].get("failed"),
    "backgroundResetBaselineRef": "5d16b2411c262313b58585b5920e83599f2a0e99",
    "backgroundResetBaselineDetectedFailures": reports["background-baseline"].get("failed"),
    "backgroundResetSetterChecks": {name: {variant: checks.get(name)
        for variant, checks in reset_checks.items()} for name in reset_names},
    "nativePixelComparisons": pixel_comparisons,
    "hostPixelComparisons": host_pixel_comparisons,
    "systemScreenshotPhases": {name: report.get("systemScreenshotsAcknowledged", [])
                               for name, report in reports.items()},
    "errors": failed,
}
(root / "tabbar-suite-results.json").write_text(json.dumps(summary, indent=2, ensure_ascii=False) + "\n")
print(json.dumps(summary, indent=2, ensure_ascii=False))
if failed:
    raise SystemExit("; ".join(failed))
PY
