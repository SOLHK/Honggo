#!/usr/bin/env python3
"""Read Honggo's plugin version and enforce increases for plugin updates."""

import argparse
import re
import subprocess
import sys
from pathlib import Path


ROOT = Path(__file__).resolve().parents[1]
VERSION_HEADER = "src/HonggoVersion.h"
VERSION_PATTERN = re.compile(
    r"(0|[1-9][0-9]*)\.(0|[1-9][0-9]*)\.(0|[1-9][0-9]*)(?:-R([1-9][0-9]*))?"
)


def version_tuple(value):
    """Sort numbered test builds before the stable release of the same base."""
    match = VERSION_PATTERN.fullmatch(value)
    if not match:
        raise ValueError("Plugin version must be MAJOR.MINOR.PATCH[-Rn], with n >= 1 and no leading zeroes.")
    parts = tuple(map(int, match.groups()[:3]))
    if parts[0] > 65535 or parts[1] > 255 or parts[2] > 255:
        raise ValueError("Plugin version exceeds Mach-O current_version limits (65535.255.255).")
    revision = match.group(4)
    return (*parts, revision is None, int(revision) if revision else 0)


def macho_version(value):
    return ".".join(map(str, version_tuple(value)[:3]))


def check_release_channel(value, channel):
    stable = version_tuple(value)[3]
    if channel == "test" and stable:
        raise ValueError("The test release channel requires an -R1, -R2, ... suffix; stable releases belong on main.")
    if channel == "main" and not stable:
        raise ValueError("The main release channel requires the stable version without an R suffix.")


def version_from_header(text):
    matches = re.findall(r'^#define HG_PLUGIN_VERSION @"([^"\n]+)"\s*$', text, re.MULTILINE)
    if len(matches) != 1:
        raise ValueError(f"{VERSION_HEADER} must define one HG_PLUGIN_VERSION string.")
    version_tuple(matches[0])
    return matches[0]


def read_version(root=ROOT):
    return version_from_header((Path(root) / VERSION_HEADER).read_text(encoding="utf-8"))


def git(root, *arguments, check=True):
    return subprocess.run(
        ["git", "-C", str(root), *arguments], check=check,
        stdout=subprocess.PIPE, stderr=subprocess.PIPE, text=True,
    )


def check_increment(root=ROOT, base="HEAD^"):
    root = Path(root)
    current = read_version(root)
    # GitHub uses all zeroes when creating a branch; still compare its parent.
    if base == "0" * 40:
        base = "HEAD^"
    resolved = git(root, "rev-parse", "--verify", "--end-of-options", f"{base}^{{commit}}", check=False)
    if resolved.returncode:
        # A genuine root commit has no previous plugin version to compare.
        parents = git(root, "rev-list", "--parents", "-n", "1", "HEAD").stdout.split()
        if base == "HEAD^" and len(parents) == 1:
            return current, "first versioned commit"
        raise ValueError(f"Cannot resolve version baseline {base!r}; fetch its history before checking.")
    baseline = resolved.stdout.strip()
    old_header = git(root, "show", f"{baseline}:{VERSION_HEADER}", check=False)
    if old_header.returncode:
        # The initial Honggo release had no independent plugin version.
        entries = git(root, "ls-tree", "-r", "--name-only", baseline).stdout.splitlines()
        if VERSION_HEADER in entries:
            raise ValueError("Cannot read the baseline version header.")
        return current, "introduced plugin version for previously unversioned source"
    previous = version_from_header(old_header.stdout)
    if version_tuple(current) < version_tuple(previous):
        raise ValueError(f"Plugin version cannot decrease: {previous} -> {current}.")
    changed = git(root, "diff", "--name-only", baseline, "--").stdout.splitlines()
    plugin_changed = any(
        path.startswith("src/") or path in {"tools/build-ios.sh", "tools/prepare_ipa.py"}
        for path in changed
    )
    if plugin_changed and version_tuple(current) <= version_tuple(previous):
        raise ValueError(
            f"Plugin source or packaging changed but version is still {current}; "
            f"increase HG_PLUGIN_VERSION in {VERSION_HEADER}."
        )
    # Documentation, CI configuration and verification-only commits may rerun
    # the same release without inventing a new plugin build.
    return current, f"version check passed ({previous} -> {current})"


def main():
    parser = argparse.ArgumentParser(description=__doc__)
    parser.add_argument("--check", action="store_true", help="Require a version increase for plugin changes")
    parser.add_argument("--base", default="HEAD^", help="Commit before the update (default: HEAD^)")
    output = parser.add_mutually_exclusive_group()
    output.add_argument("--artifact", action="store_true", help="Print the versioned dylib filename")
    output.add_argument("--macho", action="store_true", help="Print the numeric Mach-O version without a test suffix")
    parser.add_argument("--channel", choices=("test", "main"), help="Validate the intended release channel")
    args = parser.parse_args()
    try:
        if args.check:
            version, explanation = check_increment(base=args.base)
            print(explanation, file=sys.stderr)
        else:
            version = read_version()
        if args.channel:
            check_release_channel(version, args.channel)
    except (OSError, ValueError, subprocess.CalledProcessError) as error:
        parser.exit(1, f"Version check failed: {error}\n")
    print(f"Honggo-{version}.dylib" if args.artifact else macho_version(version) if args.macho else version)


if __name__ == "__main__":
    main()
