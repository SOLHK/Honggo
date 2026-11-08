import plistlib
import struct
import tempfile
import unittest
import zipfile
from pathlib import Path
from unittest import mock

from tools import prepare_ipa as prep


EXTENSION_INFO = f"{prep.APP}/PlugIns/Widget.appex/Info.plist"


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


def host_binary(cryptid=0, section_offset=512, padding=0, duplicate_load=False, duplicate_name=b"Honggo.dylib"):
    text = b"__TEXT"
    body = b"test code payload"
    segment = struct.pack("<II16sQQQQIIII", prep.LC_SEGMENT_64, 152, text,
                          0, 4096, 0, section_offset + len(body), 5, 5, 1, 0)
    section = struct.pack("<16s16sQQIIIIIIII", b"__text", text,
                          section_offset, len(body), section_offset, 0, 0, 0, 0, 0, 0, 0)
    encryption = struct.pack("<6I", prep.LC_ENCRYPTION_INFO_64, 24, section_offset, len(body), cryptid, 0)
    commands = [segment + section, encryption]
    if duplicate_load:
        commands.append(dylib_command(prep.LC_LOAD_DYLIB, b"@rpath/" + duplicate_name))
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
        self.identity = dylib_command(prep.LC_ID_DYLIB, b"@rpath/Honggo.dylib")
        self.plugin.write_bytes(macho(prep.MH_DYLIB, [self.identity]))

    def write_ipa(self, binary, version="7.4.3.33", embedded_plugin=False,
                  info_extra=None, info_format=plistlib.FMT_XML):
        info = {
            "CFBundleIdentifier": "com.phoenix.video",
            "CFBundleShortVersionString": "7.4.3",
            "CFBundleVersion": version,
            "CFBundleExecutable": "Eggplant",
            "CFBundleDisplayName": "红果短剧",
            "CFBundleURLTypes": [{"CFBundleURLSchemes": ["phoenix", "phoenix-test"]}],
            "UIApplicationSceneManifest": {"UIApplicationSupportsMultipleScenes": False},
            "FixtureData": b"unchanged plist data",
            "FixtureNumber": 12.5,
        }
        if info_extra:
            info.update(info_extra)
        with zipfile.ZipFile(self.input, "w", compression=zipfile.ZIP_DEFLATED) as archive:
            archive.comment = b"comment"
            info_entry = zipfile.ZipInfo(prep.INFO_PLIST, (2025, 6, 9, 12, 34, 56))
            info_entry.create_system = 3
            info_entry.external_attr = 0o100644 << 16
            info_entry.comment = b"main app metadata"
            info_entry.compress_type = zipfile.ZIP_DEFLATED
            archive.writestr(info_entry, plistlib.dumps(info, fmt=info_format, sort_keys=False))
            archive.writestr(prep.EXECUTABLE, binary)
            archive.writestr(f"{prep.APP}/Frameworks/", b"")
            archive.writestr(f"{prep.APP}/PlugIns/Widget.appex/Widget", b"extension")
            archive.writestr(EXTENSION_INFO, plistlib.dumps({
                "CFBundleIdentifier": "com.phoenix.video.Widget",
                "CFBundleVersion": version,
                "UIDesignRequiresCompatibility": True,
                "NSExtension": {"NSExtensionPointIdentifier": "com.apple.widgetkit-extension"},
            }, fmt=plistlib.FMT_BINARY))
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
                if entry not in (prep.EXECUTABLE, prep.INFO_PLIST):
                    self.assertEqual(output.read(entry), original.read(entry))
                for attribute in ("date_time", "compress_type", "comment", "external_attr", "create_system"):
                    self.assertEqual(getattr(output.getinfo(entry), attribute),
                                     getattr(original.getinfo(entry), attribute))
            original_info = plistlib.loads(original.read(prep.INFO_PLIST))
            output_info = plistlib.loads(output.read(prep.INFO_PLIST))
            self.assertIs(output_info.pop("UIDesignRequiresCompatibility"), False)
            original_info.pop("UIDesignRequiresCompatibility", None)
            self.assertEqual(output_info, original_info)
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

    def test_changes_only_main_design_flag_and_preserves_source_and_extensions(self):
        for format_ in (plistlib.FMT_XML, plistlib.FMT_BINARY):
            for extra in ({}, {"UIDesignRequiresCompatibility": True}, {"UIDesignRequiresCompatibility": False}):
                with self.subTest(format=format_, existing_flag=extra):
                    self.write_ipa(host_binary(), info_extra=extra, info_format=format_)
                    original_bytes = self.input.read_bytes()
                    prep.prepare(self.input, self.plugin, self.output)
                    self.assertEqual(self.input.read_bytes(), original_bytes)
                    with zipfile.ZipFile(self.input) as original, zipfile.ZipFile(self.output) as output:
                        before = plistlib.loads(original.read(prep.INFO_PLIST))
                        after_bytes = output.read(prep.INFO_PLIST)
                        after = plistlib.loads(after_bytes)
                        self.assertIs(after.pop("UIDesignRequiresCompatibility"), False)
                        before.pop("UIDesignRequiresCompatibility", None)
                        self.assertEqual(after, before)
                        self.assertEqual(after_bytes.startswith(b"bplist00"), format_ == plistlib.FMT_BINARY)
                        if extra.get("UIDesignRequiresCompatibility") is False:
                            self.assertEqual(after_bytes, original.read(prep.INFO_PLIST))
                        self.assertEqual(output.read(EXTENSION_INFO), original.read(EXTENSION_INFO))
                        self.assertIs(plistlib.loads(output.read(EXTENSION_INFO))["UIDesignRequiresCompatibility"], True)
                    self.output.unlink()

    def test_native_design_configuration_returns_copy_without_mutating_metadata(self):
        for extra in ({}, {"UIDesignRequiresCompatibility": True}, {"UIDesignRequiresCompatibility": False}):
            with self.subTest(existing_flag=extra):
                original = {
                    "CFBundleExecutable": "SimulatorFixture",
                    "UIApplicationSceneManifest": {"UIApplicationSupportsMultipleScenes": False},
                    **extra,
                }
                before = plistlib.dumps(original)
                configured = prep.configure_native_glass(original)
                self.assertIsNot(configured, original)
                self.assertEqual(plistlib.dumps(original), before)
                self.assertIs(configured.pop("UIDesignRequiresCompatibility"), False)
                expected = plistlib.loads(before)
                expected.pop("UIDesignRequiresCompatibility", None)
                self.assertEqual(configured, expected)

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

    def test_versioned_input_is_embedded_with_stable_honggo_install_name(self):
        versioned = self.plugin.with_name("Honggo-1.0.1.dylib")
        self.plugin.rename(versioned)
        self.write_ipa(host_binary())
        prep.prepare(self.input, versioned, self.output)
        with zipfile.ZipFile(self.output) as output:
            self.assertIn("Payload/Eggplant.app/Frameworks/Honggo.dylib", output.namelist())
            self.assertEqual(output.read(prep.PLUGIN_ENTRY), versioned.read_bytes())

    def test_rejects_legacy_and_versioned_duplicate_loads(self):
        for name in (b"HongguoGlass.dylib", b"Honggo-1.0.0.dylib", b"HONGGO.DYLIB"):
            with self.subTest(name=name):
                self.write_ipa(host_binary(duplicate_load=True, duplicate_name=name))
                with self.assertRaisesRegex(ValueError, "already has a load command"):
                    prep.prepare(self.input, self.plugin, self.output)
                self.assertFalse(self.output.exists())

    def test_rejects_legacy_and_versioned_embedded_plugins(self):
        for name in ("HongguoGlass.dylib", "Honggo-1.0.0.dylib", "HONGGO.DYLIB"):
            with self.subTest(name=name):
                self.write_ipa(host_binary())
                with zipfile.ZipFile(self.input, "a") as archive:
                    archive.writestr(f"{prep.APP}/Frameworks/{name}", self.plugin.read_bytes())
                with self.assertRaisesRegex(ValueError, "already contains"):
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

    def test_rejects_unverified_host_before_configuring_native_design(self):
        for key, value in (
            ("CFBundleIdentifier", "com.example.different"),
            ("CFBundleShortVersionString", "7.4.4"),
            ("CFBundleVersion", "7.4.3.34"),
            ("CFBundleExecutable", "DifferentHost"),
        ):
            with self.subTest(key=key):
                self.write_ipa(host_binary(), info_extra={key: value})
                original = self.input.read_bytes()
                with mock.patch.object(prep, "configure_native_glass") as configure:
                    with self.assertRaisesRegex(ValueError, "Expected Hongguo"):
                        prep.prepare(self.input, self.plugin, self.output)
                    configure.assert_not_called()
                self.assertEqual(self.input.read_bytes(), original)
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
        original = self.input.read_bytes()
        with mock.patch.object(prep.shutil, "copyfileobj", side_effect=OSError("disk full")):
            with self.assertRaisesRegex(OSError, "disk full"):
                prep.prepare(self.input, self.plugin, self.output)
        self.assertFalse(self.output.exists())
        self.assertEqual(self.input.read_bytes(), original)


if __name__ == "__main__":
    unittest.main()
