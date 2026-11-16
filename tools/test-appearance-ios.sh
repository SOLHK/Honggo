#!/usr/bin/env bash
set -euo pipefail

# Exercise real production appearance adapters on controlled UIKit host fixtures.
source_root=$(cd "$(dirname "${BASH_SOURCE[0]}")/.." && pwd)
work="$source_root/build/appearance-tests"
app="$work/HGAppearanceTests.app"
bundle_id=org.honggo.appearance.tests
mkdir -p "$app"
rm -f -- "$work/appearance-results.json" "$work"/video-backdrop-*.json \
    "$work"/video-backdrop-*.png "$work"/video-backdrop-*.txt "$work"/*-preview.png
if ! command -v xcrun >/dev/null 2>&1; then
    printf 'Appearance compatibility UIKit integration requires macOS with Xcode and an iOS simulator runtime.\n' >&2
    exit 1
fi
sdk=$(xcrun --sdk iphonesimulator --show-sdk-path)
architecture=$(uname -m)

xcrun --sdk iphonesimulator clang \
    -target "$architecture-apple-ios15.0.1-simulator" -isysroot "$sdk" \
    -O0 -g -fobjc-arc -fblocks -Wall -Werror -I "$source_root/src" \
    -framework Foundation -framework CoreFoundation -framework UIKit -framework CoreText -framework PhotosUI -framework ImageIO -framework UniformTypeIdentifiers -framework QuartzCore -framework CoreGraphics -framework Metal -lobjc \
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
data_directory=
evidence_collected=0
collect_evidence() {
    if [ "$evidence_collected" -eq 1 ]; then return; fi
    evidence_collected=1
    python3 - "$device_id" "$work" "$data_directory" <<'PY'
from pathlib import Path
import shutil
import subprocess
import sys

device, work, data_directory = sys.argv[1:]
work = Path(work)
documents = Path(data_directory) / "Documents" if data_directory else None

def copy_evidence():
    if documents is None or not documents.is_dir():
        return
    patterns = ("appearance-results.json", "*-preview.png", "video-backdrop-*.json",
                "video-backdrop-*.png", "video-backdrop-*.txt")
    for pattern in patterns:
        for source in documents.glob(pattern):
            if source.is_file():
                try:
                    shutil.copyfile(source, work / source.name)
                except OSError as error:
                    print(f"Could not copy {source.name}: {error}", file=sys.stderr)

copy_evidence()
with (work / "simulator.log").open("w") as log:
    try:
        subprocess.run(["xcrun", "simctl", "spawn", device, "log", "show", "--style", "compact",
                        "--last", "5m", "--predicate", 'process == "HGAppearanceTests"'],
                       stdout=log, stderr=subprocess.STDOUT, check=False, timeout=20)
    except (subprocess.TimeoutExpired, OSError) as error:
        log.write(f"\nBounded simulator log collection failed: {error}\n")
# The app may finish writing its failure report while logs are being fetched.
copy_evidence()
PY
}
cleanup() {
    local result=$?
    trap - EXIT
    collect_evidence || true
    python3 - "$device_id" "$bundle_id" <<'PY' || true
import subprocess
import sys
device, bundle = sys.argv[1:]
for command, timeout in ((["terminate", device, bundle], 5), (["shutdown", device], 8), (["delete", device], 8)):
    try:
        subprocess.run(["xcrun", "simctl", *command], stdout=subprocess.DEVNULL,
                       stderr=subprocess.DEVNULL, timeout=timeout, check=False)
    except (subprocess.TimeoutExpired, OSError):
        pass
PY
    exit "$result"
}
trap cleanup EXIT
xcrun simctl boot "$device_id"
xcrun simctl bootstatus "$device_id" -b
python3 "$source_root/tools/simulator_target.py" "$work/simulators.json" --record "$work/simulator-target.json" --verify-booted "$device_id"
xcrun simctl install "$device_id" "$app"
data_directory=$(xcrun simctl get_app_container "$device_id" "$bundle_id" data)
rm -f -- "$data_directory/Documents/appearance-results.json" "$work/appearance-results.json"
xcrun simctl launch "$device_id" "$bundle_id" 2>&1 | tee "$work/launch.log"
run_deadline=$((SECONDS + 180))
capture_failed=0
while ((SECONDS < run_deadline)); do
    # The Metal regression requests live compositor images. UIGraphics/layer
    # snapshots can omit CAMetalLayer and cannot validate a video backdrop.
    for phase in red blue; do
        prefix="video-backdrop-${phase}"
        if [ -f "$data_directory/Documents/${prefix}-ready.json" ] && \
           [ ! -f "$data_directory/Documents/${prefix}-captured" ]; then
            remaining_run=$((run_deadline - SECONDS))
            if [ "$remaining_run" -le 0 ]; then break; fi
            # A stalled simctl client can leave a complete screenshot before
            # timing out. Validate those bytes too; never replace a failed
            # compositor image with a synthetic view/layer snapshot.
            if python3 - "$device_id" "$work/${prefix}-simulator.png" \
                "$data_directory/Documents/${prefix}-simulator.png" "$phase" \
                "$data_directory/Documents/video-backdrop-active-phase.txt" \
                "$data_directory/Documents/appearance-results.json" \
                "$work/${prefix}-transport.json" "$remaining_run" <<'PY'
from pathlib import Path
import json
import struct
import subprocess
import sys
import tempfile
import time
import zlib

def validate_png(path):
    data = path.read_bytes()
    if not data.startswith(b"\x89PNG\r\n\x1a\n"):
        raise ValueError("capture is not a PNG")
    offset, header, pixels = 8, False, False
    while offset + 12 <= len(data):
        length = struct.unpack_from(">I", data, offset)[0]
        end = offset + 12 + length
        if end > len(data):
            raise ValueError("capture contains a truncated PNG chunk")
        kind = data[offset + 4:offset + 8]
        chunk = data[offset + 8:offset + 8 + length]
        crc = struct.unpack_from(">I", data, offset + 8 + length)[0]
        if zlib.crc32(kind + chunk) & 0xFFFFFFFF != crc:
            raise ValueError("capture contains a corrupt PNG chunk")
        if not header:
            if kind != b"IHDR" or length != 13:
                raise ValueError("capture lacks a PNG image header")
            width, height = struct.unpack_from(">II", chunk)
            if not width or not height:
                raise ValueError("capture has empty dimensions")
            header = True
        if kind == b"IDAT" and length:
            pixels = True
        if kind == b"IEND":
            if length or end != len(data) or not pixels:
                raise ValueError("capture has an incomplete PNG image")
            return data
        offset = end
    raise ValueError("capture lacks the PNG end marker")

def capture():
    device, output, destination, phase, active, result, status, run_remaining = sys.argv[1:]
    output, destination, active, result, status = map(Path, (output, destination, active, result, status))
    output.unlink(missing_ok=True)
    destination.unlink(missing_ok=True)
    started = time.monotonic()
    budget = min(40, max(0, float(run_remaining)))
    deadline = started + budget
    report = {"phase": phase, "success": False, "budget_seconds": budget, "attempts": []}
    def current_phase():
        if result.exists() or not active.exists() or active.read_text().strip() != phase:
            raise RuntimeError("app is no longer displaying the requested Metal phase")
    try:
        for attempt in (1, 2):
            temporary = None
            entry = {"attempt": attempt}
            report["attempts"].append(entry)
            try:
                current_phase()
                remaining = deadline - time.monotonic()
                if remaining <= 0:
                    raise RuntimeError("phase capture exhausted its 40-second budget")
                with tempfile.NamedTemporaryFile(dir=output.parent,
                        prefix=f"{output.stem}.attempt-{attempt}-", suffix=".png") as placeholder:
                    temporary = Path(placeholder.name)
                try:
                    completed = subprocess.run(["xcrun", "simctl", "io", device, "screenshot", "--type=png", str(temporary)],
                                               check=False, capture_output=True, text=True, timeout=min(20, remaining))
                    entry["returncode"] = completed.returncode
                    entry["stderr"] = completed.stderr[-1200:]
                except subprocess.TimeoutExpired:
                    entry["timed_out"] = True
                # A valid complete PNG remains evidence even when simctl failed
                # to return promptly. Black pixels will still fail in the app.
                data = validate_png(temporary)
                current_phase()
                temporary.replace(output)
                staged = destination.with_name(destination.name + ".tmp")
                staged.write_bytes(data)
                staged.replace(destination)
                report["success"] = True
                entry["valid_png"] = True
                print(f"Metal capture {phase} completed on attempt {attempt} in {time.monotonic()-started:.3f}s", flush=True)
                return 0
            except (OSError, RuntimeError, ValueError) as error:
                entry["error"] = str(error)
                print(f"Metal capture {phase} attempt {attempt}/2 failed: {error}", file=sys.stderr, flush=True)
                if isinstance(error, RuntimeError):
                    break
            finally:
                if temporary is not None:
                    temporary.unlink(missing_ok=True)
        return 1
    finally:
        report["elapsed_seconds"] = round(time.monotonic()-started, 3)
        status.write_text(json.dumps(report, indent=2) + "\n")

raise SystemExit(capture())
PY
            then
                :
            else
                capture_failed=1
                printf 'Metal capture transport failed for %s; preserving app pixel failures and diagnostics.\n' "$phase" >&2
            fi
            # Acknowledge both outcomes so a transport failure cannot prevent
            # the app from recording its unchanged pixel assertions and report.
            touch "$data_directory/Documents/${prefix}-captured"
        fi
    done
    if [ -f "$data_directory/Documents/appearance-results.json" ]; then
        break
    fi
    sleep 1
done
collect_evidence
if [ ! -f "$work/appearance-results.json" ]; then
    cat "$work/simulator.log"
    printf 'Appearance compatibility integration app did not write a result within the bounded 180-second run budget.\n' >&2
    exit 1
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
if [ "$capture_failed" -ne 0 ]; then
    printf 'At least one required Metal compositor transport failed.\n' >&2
    exit 1
fi
