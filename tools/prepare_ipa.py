#!/usr/bin/env python3
"""Prepare a local copy of the supported IPA for an IPA-only signer."""

import argparse
import copy
import plistlib
import shutil
import struct
import zipfile
from pathlib import Path


APP = "Payload/Eggplant.app"
EXECUTABLE = f"{APP}/Eggplant"
PLUGIN_NAME = "HongguoGlass.dylib"
PLUGIN_ENTRY = f"{APP}/Frameworks/{PLUGIN_NAME}"
LOAD_PATH = f"@executable_path/Frameworks/{PLUGIN_NAME}".encode("ascii")

MH_MAGIC_64 = 0xFEEDFACF
CPU_TYPE_ARM64 = 0x0100000C
MH_EXECUTE = 2
MH_DYLIB = 6
LC_SEGMENT_64 = 0x19
LC_ENCRYPTION_INFO_64 = 0x2C
LC_BUILD_VERSION = 0x32
LC_VERSION_MIN_IPHONEOS = 0x25
LC_LOAD_DYLIB = 0xC
LC_ID_DYLIB = 0xD
LOAD_DYLIB_COMMANDS = {LC_LOAD_DYLIB, 0x80000018, 0x8000001F, 0x20, 0x80000023}
ZERO_FILL_SECTION_TYPES = {1, 0xC, 0x12}
MACH_HEADER = struct.Struct("<8I")
MAX_LOAD_COMMAND_BYTES = 1024 * 1024
COPY_CHUNK = 1024 * 1024


def read_macho(source, file_size, file_type):
    header_bytes = source.read(MACH_HEADER.size)
    if len(header_bytes) != MACH_HEADER.size:
        raise ValueError("Truncated Mach-O header.")
    header = MACH_HEADER.unpack(header_bytes)
    magic, cpu, _, actual_type, count, command_bytes, _, _ = header
    if (magic, cpu, actual_type) != (MH_MAGIC_64, CPU_TYPE_ARM64, file_type):
        raise ValueError("Expected a thin 64-bit arm64 Mach-O of the required file type.")
    if command_bytes > min(file_size - MACH_HEADER.size, MAX_LOAD_COMMAND_BYTES):
        raise ValueError("Mach-O load commands exceed the supported bounds.")
    raw_commands = source.read(command_bytes)
    if len(raw_commands) != command_bytes:
        raise ValueError("Truncated Mach-O load commands.")

    offset = 0
    commands = []
    for _ in range(count):
        if offset + 8 > command_bytes:
            raise ValueError("Truncated Mach-O load command.")
        kind, size = struct.unpack_from("<II", raw_commands, offset)
        if size < 8 or size % 8 or offset + size > command_bytes:
            raise ValueError("Invalid Mach-O load command size.")
        commands.append((kind, raw_commands[offset:offset + size]))
        offset += size
    if offset != command_bytes:
        raise ValueError("Mach-O load command count and size disagree.")

    platforms = []
    for kind, command in commands:
        if kind == LC_BUILD_VERSION and len(command) >= 24:
            platforms.append(struct.unpack_from("<I", command, 8)[0])
        if kind == LC_VERSION_MIN_IPHONEOS and len(command) == 16:
            platforms.append(2)
    if platforms != [2]:
        raise ValueError("Expected an iOS device Mach-O.")
    return header, raw_commands, commands


def dylib_basename(command):
    if len(command) < 24:
        raise ValueError("Invalid Mach-O dylib command.")
    name_offset, = struct.unpack_from("<I", command, 8)
    name = command[name_offset:].split(b"\0", 1)
    if name_offset < 24 or len(name) != 2 or not name[0]:
        raise ValueError("Invalid Mach-O dylib path.")
    return name[0].rsplit(b"/", 1)[-1].lower()


def first_text_section(commands, file_size):
    for kind, command in commands:
        if kind != LC_SEGMENT_64 or command[8:24].rstrip(b"\0") != b"__TEXT":
            continue
        if len(command) < 72:
            break
        file_offset, file_bytes, _, _, section_count = struct.unpack_from("<QQIII", command, 40)
        if file_offset or not 0 < file_bytes <= file_size or len(command) != 72 + section_count * 80:
            break

        offsets = []
        for start in range(72, len(command), 80):
            size, offset = struct.unpack_from("<QI", command, start + 40)
            flags, = struct.unpack_from("<I", command, start + 64)
            if size and (flags & 0xFF) not in ZERO_FILL_SECTION_TYPES:
                if not offset or offset + size > file_bytes:
                    raise ValueError("Mach-O __TEXT section is outside its file segment.")
                offsets.append(offset)
        if offsets:
            return min(offsets)
    raise ValueError("Cannot establish load command padding from the Mach-O __TEXT sections.")


def patched_header(source, file_size):
    header, raw_commands, commands = read_macho(source, file_size, MH_EXECUTE)
    encryption = [command for kind, command in commands if kind == LC_ENCRYPTION_INFO_64]
    if len(encryption) != 1 or len(encryption[0]) != 24:
        raise ValueError("Cannot verify the host Mach-O encryption state.")
    if struct.unpack_from("<I", encryption[0], 16)[0] != 0:
        raise ValueError("Host IPA is encrypted (cryptid is nonzero).")

    for kind, command in commands:
        if kind in LOAD_DYLIB_COMMANDS and dylib_basename(command) == PLUGIN_NAME.lower().encode("ascii"):
            raise ValueError("The host already has a load command for HongguoGlass.dylib.")

    name = LOAD_PATH + b"\0"
    command_size = (24 + len(name) + 7) & ~7
    load = struct.pack("<6I", LC_LOAD_DYLIB, command_size, 24, 0, 0, 0)
    load += name.ljust(command_size - 24, b"\0")
    header_end = MACH_HEADER.size + len(raw_commands)
    if header_end + command_size > first_text_section(commands, file_size):
        raise ValueError("Not enough Mach-O header space for the load command.")
    if source.read(command_size) != bytes(command_size):
        raise ValueError("The required Mach-O header padding is not zero-filled.")

    magic, cpu, subtype, file_type, count, size, flags, reserved = header
    updated = MACH_HEADER.pack(magic, cpu, subtype, file_type, count + 1, size + command_size, flags, reserved)
    return updated + raw_commands + load


def prepare(original, plugin, destination):
    if original.resolve() == destination.resolve():
        raise ValueError("Choose an output path separate from the source IPA.")

    with zipfile.ZipFile(original) as archive, plugin.open("rb") as library:
        _, _, commands = read_macho(library, plugin.stat().st_size, MH_DYLIB)
        identities = [dylib_basename(command) for kind, command in commands if kind == LC_ID_DYLIB]
        if identities != [PLUGIN_NAME.lower().encode("ascii")]:
            raise ValueError("Expected the HongguoGlass.dylib install name.")
        library.seek(0)

        entries = archive.infolist()
        names = {entry.filename.casefold() for entry in entries}
        if len(names) != len(entries):
            raise ValueError("IPA contains duplicate or case-colliding ZIP entry names.")
        if PLUGIN_ENTRY.casefold() in names:
            raise ValueError("IPA already contains HongguoGlass.dylib.")
        info = plistlib.loads(archive.read(f"{APP}/Info.plist"))
        expected = {
            "CFBundleIdentifier": "com.phoenix.video",
            "CFBundleShortVersionString": "7.4.3",
            "CFBundleVersion": "7.4.3.33",
            "CFBundleExecutable": "Eggplant",
        }
        if not isinstance(info, dict) or any(info.get(key) != value for key, value in expected.items()):
            raise ValueError("Expected Hongguo 7.4.3 (7.4.3.33), bundle com.phoenix.video.")
        with archive.open(EXECUTABLE) as source:
            patch = patched_header(source, archive.getinfo(EXECUTABLE).file_size)

        output = destination.open("xb")
        completed = False
        try:
            with output, zipfile.ZipFile(output, "w", compression=zipfile.ZIP_DEFLATED) as result:
                result.comment = archive.comment
                for entry in entries:
                    with archive.open(entry) as source, result.open(copy.copy(entry), "w") as target:
                        if entry.filename == EXECUTABLE:
                            source.read(len(patch))
                            target.write(patch)
                        shutil.copyfileobj(source, target, COPY_CHUNK)

                plugin_info = zipfile.ZipInfo(PLUGIN_ENTRY)
                plugin_info.create_system = 3
                plugin_info.external_attr = 0o100755 << 16
                plugin_info.compress_type = zipfile.ZIP_DEFLATED
                with result.open(plugin_info, "w") as target:
                    shutil.copyfileobj(library, target, COPY_CHUNK)
            completed = True
        finally:
            if not completed:
                destination.unlink()


def main():
    parser = argparse.ArgumentParser(description=__doc__)
    parser.add_argument("original", type=Path, help="Original local Hongguo IPA")
    parser.add_argument("plugin", type=Path, help="HongguoGlass.dylib built from this project")
    parser.add_argument("destination", type=Path, help="New IPA path, to be re-signed")
    args = parser.parse_args()
    try:
        prepare(args.original, args.plugin, args.destination)
    except (OSError, ValueError, KeyError, RuntimeError, zipfile.BadZipFile) as error:
        parser.exit(1, f"Prepare failed: {error}\n")
    print(f"Prepared {args.destination}. Re-sign the application and its embedded code before installation.")


if __name__ == "__main__":
    main()
