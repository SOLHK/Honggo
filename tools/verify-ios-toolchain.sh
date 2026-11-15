#!/usr/bin/env bash
set -euo pipefail
source_root=$(cd "$(dirname "${BASH_SOURCE[0]}")/.." && pwd)
work="$source_root/build/toolchain"
mkdir -p "$work"
# The CI selects this exact Xcode through DEVELOPER_DIR, not the runner default.
xcodebuild -version | tee "$work/xcode-version.txt"
xcrun --sdk iphoneos --show-sdk-version > "$work/device-sdk.txt"
xcrun --sdk iphonesimulator --show-sdk-version > "$work/simulator-sdk.txt"
xcrun simctl list --json > "$work/simulators.json"
python3 "$source_root/tools/simulator_target.py" "$work/simulators.json" --record "$work/selection.json"
python3 - "$work" <<'PY'
import json
import os
import re
import sys
from pathlib import Path

root = Path(sys.argv[1])
xcode = (root / "xcode-version.txt").read_text().strip()
device_sdk = (root / "device-sdk.txt").read_text().strip()
simulator_sdk = (root / "simulator-sdk.txt").read_text().strip()
if not re.search(r"^Xcode 27\.2$", xcode, re.M) or device_sdk != "27.2" or simulator_sdk != "27.2":
    raise SystemExit("Xcode 27.2 with both iPhoneOS and Simulator 27.2 SDKs is required; refusing substitution")
evidence = json.loads((root / "selection.json").read_text())
evidence.update(xcode=xcode, deviceSDK=device_sdk, simulatorSDK=simulator_sdk,
                developerDirectory=os.environ.get("DEVELOPER_DIR", ""),
                runnerImage=os.environ.get("ImageOS", ""), runnerImageVersion=os.environ.get("ImageVersion", ""),
                workflowRun=os.environ.get("GITHUB_RUN_ID", ""))
(root / "BUILD-ENVIRONMENT.json").write_text(json.dumps(evidence, indent=2) + "\n")
print(json.dumps(evidence, indent=2))
PY
