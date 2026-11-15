"""SDK-dependent UIKit design expectations, not plugin acceptance shortcuts.

Apple documentation and Frameworks Engineering clarification:
https://developer.apple.com/documentation/bundleresources/information-property-list/uidesignrequirescompatibility
https://developer.apple.com/forums/thread/838637

A test app built with an iOS 27+ SDK ignores the compatibility opt-out even
when its deployment target is older. The host executable's SDK, not the
injected dylib's SDK or just the running OS, determines this contract.
"""
from __future__ import annotations
import re


def encode_sdk_version(version: str) -> int:
    if not isinstance(version, str) or not re.fullmatch(r"[1-9][0-9]*\.[0-9]+(?:\.[0-9]+)?", version):
        raise ValueError("A concrete build SDK version is required")
    parts = list(map(int, version.split(".")))
    major, minor, patch = (parts + [0])[:3]
    if minor >= 100 or patch >= 100:
        raise ValueError("SDK version component out of range")
    return major * 10000 + minor * 100 + patch


def compatibility_opt_out_effective(build_sdk: int) -> bool:
    if type(build_sdk) is not int or not 150000 <= build_sdk < 1000000:
        raise ValueError("Missing or invalid compiled SDK evidence")
    return build_sdk < 270000


def expected_unsupported_reference(variant: str, build_sdk: int) -> bool:
    effective = compatibility_opt_out_effective(build_sdk)
    return variant == "reference-compat" and effective
