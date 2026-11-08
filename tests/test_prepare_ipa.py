import plistlib
import struct
import tempfile
import unittest
import zipfile
from pathlib import Path
from unittest import mock

from tools import prepare_ipa as prep


def dylib_command(kind, path):
    name = path + b"\0"
    size = (24 + len(name) + 7) & ~7
    return struct.pack("<6I", kind, size, 24, 0, 0, 0) + name.ljust(size - 24, b"\0")


def macho(file_type, commands, platform=2):
    platform_command = struct.pack("<6I", prep.LC_BUILD_VERSION, 24, platform, 0xF0000, 0x1A0000, 0)
    table = platform_command + b"".join(commands)
    header = prep.MACH_HEADER.pack(
        prep.MH_MAGIC_64, prep.CPU_TYPE_ARM64, 0, file_type, 1 + len(commands), len(table), 0, 0
    )
    return header + table


def host_binary(cryptid=0, section_offset=512, padding=0, duplicate_load=False):
    text = b"__TEXT"
    body = b"test code payload"
    segment = struct.pack("<II16sQQQQIIII", prep.LC_SEGMENT_64, 152, text,
                          0, 4096, 0, section_offset + len(body), 5, 5, 1, 0)
    section = struct.pack("<16s16sQQIIIIIIII", b"__text", text,
                          section_offset, len(body), section_offset, 0, 0, 0, 0, 0, 0, 0)
    encryption = struct.pack("<6I", prep.LC_ENCRYPTION_INFO_64, 24, section_offset, len(body), cryptid, 0)
    commands = [segment + section, encryption]
    if duplicate_load:
        commands.append(dylib_command(prep.LC_LOAD_DYLIB, b"@rpath/HongguoGlass.dylib"))
    prefix = macho(prep.MH_EXECUTE, commands)
    return prefix.ljust(section_offset, bytes([padding])) + body


class PrepareIPATest(unittest.TestCase):
    def setUp(self):
        scratch = tempfile.TemporaryDirectory()
        self.addCleanup(scratch.cleanup)
        root = Path(scratch.name)
        self.input = root / "original.ipa"
        self.plugin = root / prep.PLUGIN_NAME
        self.output = root / "unsigned.ipa"
        self.identity = dylib_command(prep.LC_ID_DYLIB, b"@rpath/HongguoGlass.dylib")
        self.plugin.write_bytes(macho(prep.MH_DYLIB, [self.identity]))

    def write_ipa(self, binary, version="7.4.3.33", embedded_plugin=False):
        with zipfile.ZipFile(self.input, "w", compression=zipfile.ZIP_DEFLATED) as archive:
            archive.comment = b"comment"
            archive.writestr(f"{prep.APP}/Info.plist", plistlib.dumps({
                "CFBundleIdentifier": "com.phoenix.video",
                "CFBundleShortVersionString": "7.4.3",
                "CFBundleVersion": version,
                "CFBundleExecutable": "Eggplant",
            }))
            archive.writestr(prep.EXECUTABLE, binary)
            archive.writestr(f"{prep.APP}/Frameworks/", b"")
            archive.writestr(f"{prep.APP}/PlugIns/Widget.appex/Widget", b"extension")
            if embedded_plugin:
                archive.writestr(prep.PLUGIN_ENTRY, self.plugin.read_bytes())

    def test_injects_one_load_command_and_preserves_zip_contents(self):
        binary = host_binary()
        self.write_ipa(binary)
        input_bytes = self.input.read_bytes()
        prep.prepare(self.input, self.plugin, self.output)

        self.assertEqual(self.input.read_bytes(), input_bytes)
        with zipfile.ZipFile(self.input) as original, zipfile.ZipFile(self.output) as output:
            self.assertEqual(output.namelist(), original.namelist() + [prep.PLUGIN_ENTRY])
            self.assertEqual(output.comment, original.comment)
            self.assertIsNone(output.testzip())
            for entry in original.namelist():
                if entry != prep.EXECUTABLE:
                    self.assertEqual(output.read(entry), original.read(entry))
            self.assertEqual(output.read(prep.PLUGIN_ENTRY), self.plugin.read_bytes())
            updated = output.read(prep.EXECUTABLE)
            self.assertEqual(len(updated), len(binary))
            old_header = prep.MACH_HEADER.unpack_from(binary)
            new_header = prep.MACH_HEADER.unpack_from(updated)
            offset = prep.MACH_HEADER.size + old_header[5]
            expected = dylib_command(prep.LC_LOAD_DYLIB, prep.LOAD_PATH)
            self.assertEqual(new_header[4:6], (old_header[4] + 1, old_header[5] + len(expected)))
            self.assertEqual(updated[prep.MACH_HEADER.size:offset], binary[prep.MACH_HEADER.size:offset])
            self.assertEqual(updated[offset:offset + len(expected)], expected)
            self.assertEqual(updated[offset + len(expected):], binary[offset + len(expected):])

    def test_rejects_encrypted_macho(self):
        self.write_ipa(host_binary(cryptid=1))
        with self.assertRaisesRegex(ValueError, "encrypted"):
            prep.prepare(self.input, self.plugin, self.output)
        self.assertFalse(self.output.exists())

    def test_rejects_insufficient_or_nonzero_padding(self):
        for binary, error in [
            (host_binary(section_offset=250), "Not enough"),
            (host_binary(padding=1), "not zero-filled"),
        ]:
            with self.subTest(error=error):
                self.write_ipa(binary)
                with self.assertRaisesRegex(ValueError, error):
                    prep.prepare(self.input, self.plugin, self.output)
                self.assertFalse(self.output.exists())

    def test_rejects_duplicate_binary_or_load_command(self):
        for binary, embedded, error in [
            (host_binary(), True, "already contains"),
            (host_binary(duplicate_load=True), False, "already has a load command"),
        ]:
            with self.subTest(error=error):
                self.write_ipa(binary, embedded_plugin=embedded)
                with self.assertRaisesRegex(ValueError, error):
                    prep.prepare(self.input, self.plugin, self.output)
                self.assertFalse(self.output.exists())

    def test_rejects_wrong_host_version_or_simulator_dylib(self):
        self.write_ipa(host_binary(), version="7.4.4")
        with self.assertRaisesRegex(ValueError, "Expected Hongguo"):
            prep.prepare(self.input, self.plugin, self.output)
        self.write_ipa(host_binary())
        self.plugin.write_bytes(macho(prep.MH_DYLIB, [self.identity], platform=7))
        with self.assertRaisesRegex(ValueError, "iOS device"):
            prep.prepare(self.input, self.plugin, self.output)
        self.assertFalse(self.output.exists())

    def test_never_overwrites_source_or_existing_output(self):
        self.write_ipa(host_binary())
        old = self.input.read_bytes()
        with self.assertRaisesRegex(ValueError, "separate"):
            prep.prepare(self.input, self.plugin, self.input)
        self.assertEqual(self.input.read_bytes(), old)
        self.output.write_bytes(b"existing")
        with self.assertRaises(FileExistsError):
            prep.prepare(self.input, self.plugin, self.output)
        self.assertEqual(self.output.read_bytes(), b"existing")

    def test_removes_partial_output_on_write_failure(self):
        self.write_ipa(host_binary())
        with mock.patch.object(prep.shutil, "copyfileobj", side_effect=OSError("disk full")):
            with self.assertRaisesRegex(OSError, "disk full"):
                prep.prepare(self.input, self.plugin, self.output)
        self.assertFalse(self.output.exists())


if __name__ == "__main__":
    unittest.main()
