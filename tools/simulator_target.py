#!/usr/bin/env python3
"""Select an exact iOS runtime; never silently replace the requested OS."""
from __future__ import annotations

import argparse
import json
import os
import re
import subprocess
import sys
from pathlib import Path
from typing import Any


def version_tuple(value: str) -> tuple[int, ...]:
    if not isinstance(value, str) or not re.fullmatch(r"\d+\.\d+(?:\.\d+)?", value):
        raise ValueError(f"Invalid iOS version: {value!r}")
    parts = tuple(map(int, value.split(".")))
    while len(parts) > 2 and parts[-1] == 0:
        parts = parts[:-1]
    return parts


def select_target(inventory: dict[str, Any], requested: str, preferred: str = "iPhone 17 Pro Max") -> dict[str, str]:
    wanted = version_tuple(requested)
    runtimes = []
    for runtime in inventory.get("runtimes", []):
        identifier = runtime.get("identifier", "")
        if not identifier.startswith("com.apple.CoreSimulator.SimRuntime.iOS-") or runtime.get("isAvailable") is not True:
            continue
        try:
            matches = version_tuple(runtime.get("version", "")) == wanted
        except ValueError:
            matches = False
        if matches:
            runtimes.append(runtime)
    if not runtimes:
        raise ValueError(f"Required iOS {requested} runtime is not installed/available. No OS fallback is permitted.")
    # Prefer the canonical runtime if alternate runtime packages coexist.
    canonical = "com.apple.CoreSimulator.SimRuntime.iOS-" + "-".join(map(str, wanted))
    runtime = next((item for item in runtimes if item["identifier"] == canonical), runtimes[0])
    types = {item["identifier"]: item for item in inventory.get("devicetypes", []) if item.get("identifier")}
    candidates: dict[str, str] = {}
    for device in inventory.get("devices", {}).get(runtime["identifier"], []):
        if device.get("isAvailable") is not True or not device.get("name", "").startswith("iPhone"):
            continue
        type_id = device.get("deviceTypeIdentifier")
        if not type_id:
            type_id = next((key for key, value in types.items() if value.get("name") == device["name"]), None)
        if type_id in types:
            candidates[type_id] = types[type_id].get("name", device["name"])
    if not candidates:
        # A runtime does not need a pre-created device. simctl create below is
        # authoritative for model compatibility and will fail rather than swap OS.
        supported = runtime.get("supportedDeviceTypes", [])
        supported_ids = {item.get("identifier") if isinstance(item, dict) else item for item in supported}
        candidates = {key: value.get("name", "") for key, value in types.items()
                      if value.get("name", "").startswith("iPhone") and (not supported_ids or key in supported_ids)}
    if not candidates:
        raise ValueError(f"No compatible iPhone device type is available for iOS {requested}")
    device_type = min(candidates, key=lambda key: (candidates[key] != preferred, "Pro Max" not in candidates[key], candidates[key]))
    return {"requestedOS": requested, "runtime": runtime["identifier"], "runtimeVersion": runtime["version"],
            "runtimeBuild": runtime.get("buildversion", "unknown"), "deviceType": device_type,
            "deviceName": candidates[device_type], "sourceCommit": os.environ.get("GITHUB_SHA", "local")}


def verify_booted(inventory: dict[str, Any], selection: dict[str, str], udid: str) -> dict[str, Any]:
    runtime = next((item for item in inventory.get("runtimes", []) if item.get("identifier") == selection["runtime"]), None)
    if not runtime or runtime.get("isAvailable") is not True or version_tuple(runtime["version"]) != version_tuple(selection["requestedOS"]):
        raise ValueError("The selected runtime is no longer the requested available iOS version")
    device = next((item for item in inventory.get("devices", {}).get(selection["runtime"], []) if item.get("udid") == udid), None)
    if not device or device.get("state") != "Booted":
        raise ValueError("The created test device is not booted on the required runtime")
    return {**selection, "deviceUDID": udid, "deviceState": device["state"], "verified": True}


def write_record(path: Path, value: dict[str, Any]) -> None:
    path.parent.mkdir(parents=True, exist_ok=True)
    path.write_text(json.dumps(value, indent=2, ensure_ascii=False) + "\n", encoding="utf-8")


def main() -> int:
    parser = argparse.ArgumentParser(description=__doc__)
    parser.add_argument("inventory", type=Path)
    parser.add_argument("--os", default=os.environ.get("HG_SIMULATOR_OS", "27.2"))
    parser.add_argument("--device", default=os.environ.get("HG_SIMULATOR_DEVICE", "iPhone 17 Pro Max"))
    parser.add_argument("--record", type=Path, required=True)
    parser.add_argument("--verify-booted", metavar="UDID")
    args = parser.parse_args()
    try:
        if args.verify_booted:
            selection = json.loads(args.record.read_text(encoding="utf-8"))
            output = subprocess.check_output(["xcrun", "simctl", "list", "--json"], text=True)
            evidence = verify_booted(json.loads(output), selection, args.verify_booted)
            write_record(args.record, evidence)
            print(f"Verified booted {evidence['deviceName']} on iOS {evidence['runtimeVersion']} ({evidence['runtimeBuild']})")
        else:
            evidence = select_target(json.loads(args.inventory.read_text(encoding="utf-8")), args.os, args.device)
            write_record(args.record, evidence)
            print(evidence["deviceType"], evidence["runtime"])
    except (OSError, ValueError, KeyError, TypeError, subprocess.CalledProcessError) as error:
        print(f"Simulator target error: {error}", file=sys.stderr)
        return 1
    return 0


if __name__ == "__main__":
    raise SystemExit(main())
