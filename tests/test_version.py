import os
import shutil
import subprocess
import sys
import tempfile
import unittest
from pathlib import Path
from unittest import mock
from contextlib import redirect_stdout, redirect_stderr
from io import StringIO

from tools import version as versions


class PluginVersionTest(unittest.TestCase):
    def setUp(self):
        temporary = tempfile.TemporaryDirectory()
        self.addCleanup(temporary.cleanup)
        self.root = Path(temporary.name)
        self.git("init", "-q")
        (self.root / "src").mkdir()
        (self.root / "src/Plugin.m").write_text("// initial plugin\n")

    def git(self, *arguments):
        return subprocess.check_output([
            "git", "-C", str(self.root), "-c", "user.name=Version fixture",
            "-c", "user.email=fixture@example.invalid", *arguments,
        ], stderr=subprocess.STDOUT, text=True).strip()

    def set_version(self, value):
        (self.root / versions.VERSION_HEADER).write_text(f'#define HG_PLUGIN_VERSION @"{value}"\n')

    def commit_baseline(self, value=None):
        if value is not None:
            self.set_version(value)
        self.git("add", ".")
        self.git("commit", "-qm", "baseline")
        return self.git("rev-parse", "HEAD")

    def test_rejects_host_versions_and_invalid_macho_versions(self):
        for value in ("7.4.3.33", "1.00.1", "v1.0.1", "1.0.1-test", "65536.0.0", "1.256.0", "1.0.256"):
            with self.subTest(value=value), self.assertRaises(ValueError):
                versions.version_tuple(value)
        self.assertEqual(versions.version_tuple("1.0.1")[:3], (1, 0, 1))

    def test_orders_test_revisions_then_stable_then_next_base(self):
        ordered = ["1.1.15-R1", "1.1.15-R2", "1.1.15-R10", "1.1.15", "1.1.16-R1"]
        self.assertEqual(sorted(reversed(ordered), key=versions.version_tuple), ordered)
        for value in ("1.1.15-R0", "1.1.15-R01", "1.1.15-r1", "1.1.15-R", "1.1.15-R-1",
                      "1.1.15-R1.1", "1.1.15-R1-extra", "1.1.15-R１", "1.1.15-R1\n"):
            with self.subTest(value=value), self.assertRaises(ValueError):
                versions.version_tuple(value)

    def test_requires_a_new_revision_for_test_plugin_changes(self):
        baseline = self.commit_baseline("1.1.15-R1")
        (self.root / "src/Plugin.m").write_text("// revised test build\n")
        with self.assertRaisesRegex(ValueError, "increase HG_PLUGIN_VERSION"):
            versions.check_increment(self.root, baseline)
        self.set_version("1.1.15-R2")
        self.assertEqual(versions.check_increment(self.root, baseline)[0], "1.1.15-R2")

    def test_allows_test_promotion_but_rejects_reverting_stable_to_test(self):
        baseline = self.commit_baseline("1.1.15-R2")
        self.set_version("1.1.15")
        self.assertEqual(versions.check_increment(self.root, baseline)[0], "1.1.15")
        baseline = self.commit_baseline()
        self.set_version("1.1.15-R3")
        with self.assertRaisesRegex(ValueError, "cannot decrease"):
            versions.check_increment(self.root, baseline)
        self.set_version("1.1.16-R1")
        self.assertEqual(versions.check_increment(self.root, baseline)[0], "1.1.16-R1")

    def test_release_channel_rejects_stable_on_test_and_tests_on_main(self):
        versions.check_release_channel("1.1.15-R1", "test")
        versions.check_release_channel("1.1.15", "main")
        with self.assertRaisesRegex(ValueError, "test release channel requires"):
            versions.check_release_channel("1.1.15", "test")
        with self.assertRaisesRegex(ValueError, "main release channel requires"):
            versions.check_release_channel("1.1.15-R1", "main")

    def test_cli_separates_artifact_and_numeric_macho_versions(self):
        for argument, expected in (("--artifact", "Honggo-1.1.15-R1.dylib"), ("--macho", "1.1.15")):
            output = StringIO()
            with self.subTest(argument=argument), mock.patch.object(versions, "read_version", return_value="1.1.15-R1"), \
                    mock.patch.object(sys, "argv", ["version.py", argument]), redirect_stdout(output):
                versions.main()
            self.assertEqual(output.getvalue().strip(), expected)
        self.assertEqual(versions.macho_version("65535.255.255-R2"), "65535.255.255")
        with mock.patch.object(versions, "read_version", return_value="1.1.15"), \
                mock.patch.object(sys, "argv", ["version.py", "--channel", "test"]), \
                redirect_stderr(StringIO()), self.assertRaises(SystemExit) as raised:
            versions.main()
        self.assertEqual(raised.exception.code, 1)

    @unittest.skipUnless(shutil.which("bash"), "build script fixture requires bash")
    def test_device_build_uses_numeric_macho_and_test_artifact_versions(self):
        self.set_version("1.1.15-R1")
        tools_dir = self.root / "tools"
        tools_dir.mkdir()
        for name in ("version.py", "build-ios.sh"):
            shutil.copyfile(versions.ROOT / "tools" / name, tools_dir / name)
        bin_dir = self.root / "bin"
        bin_dir.mkdir()
        wrappers = {
            "python3": '#!/usr/bin/env sh\nexec "$HG_TEST_PYTHON" "$@"\n',
            "xcrun": '#!/usr/bin/env sh\nprintf "%s\\n" "$@" >> "$HG_TEST_XCRUN_ARGS"\n'
                     'if [ "$3" = "--show-sdk-path" ]; then printf "/fixture-sdk\\n"; fi\n',
        }
        for name, content in wrappers.items():
            wrapper = bin_dir / name
            wrapper.write_text(content, encoding="utf-8", newline="\n")
            wrapper.chmod(0o755)
        arguments_file = self.root / "xcrun-arguments.txt"
        environment = os.environ.copy()
        environment.update(
            PATH=str(bin_dir) + os.pathsep + environment["PATH"],
            HG_TEST_PYTHON=Path(sys.executable).as_posix(),
            HG_TEST_XCRUN_ARGS=arguments_file.as_posix(),
        )
        subprocess.run([shutil.which("bash"), str(tools_dir / "build-ios.sh")],
                       env=environment, check=True, stdout=subprocess.PIPE, stderr=subprocess.PIPE)
        arguments = arguments_file.read_text(encoding="utf-8").splitlines()
        self.assertIn("-Wl,-current_version,1.1.15", arguments)
        self.assertNotIn("-Wl,-current_version,1.1.15-R1", arguments)
        self.assertTrue(any(value.endswith("/Honggo-1.1.15-R1.dylib") for value in arguments))

    def test_adopts_a_version_for_the_original_unversioned_plugin(self):
        baseline = self.commit_baseline()
        self.set_version("1.0.1")
        self.assertEqual(versions.check_increment(self.root, baseline)[0], "1.0.1")

    def test_requires_a_bump_when_plugin_source_changes(self):
        baseline = self.commit_baseline("1.0.1")
        (self.root / "src/Plugin.m").write_text("// updated plugin\n")
        with self.assertRaisesRegex(ValueError, "increase HG_PLUGIN_VERSION"):
            versions.check_increment(self.root, baseline)
        self.set_version("1.0.2")
        self.assertEqual(versions.check_increment(self.root, baseline)[0], "1.0.2")

    def test_requires_a_bump_when_device_build_changes(self):
        baseline = self.commit_baseline("1.0.1")
        (self.root / "tools").mkdir()
        (self.root / "tools/build-ios.sh").write_text("# updated build\n")
        self.git("add", "tools/build-ios.sh")
        with self.assertRaisesRegex(ValueError, "increase HG_PLUGIN_VERSION"):
            versions.check_increment(self.root, baseline)

    def test_requires_a_bump_when_ipa_preparation_changes(self):
        (self.root / "tools").mkdir()
        (self.root / "tools/prepare_ipa.py").write_text("# original packaging\n")
        baseline = self.commit_baseline("1.0.3")
        (self.root / "tools/prepare_ipa.py").write_text("# changed host design metadata\n")
        with self.assertRaisesRegex(ValueError, "increase HG_PLUGIN_VERSION"):
            versions.check_increment(self.root, baseline)
        self.set_version("1.0.4")
        self.assertEqual(versions.check_increment(self.root, baseline)[0], "1.0.4")

    def test_allows_verification_only_commits_to_reuse_the_release(self):
        baseline = self.commit_baseline("1.0.1")
        (self.root / "README.md").write_text("New verification evidence\n")
        self.git("add", "README.md")
        self.assertEqual(versions.check_increment(self.root, baseline)[0], "1.0.1")

    def test_rejects_version_regression_and_missing_history(self):
        baseline = self.commit_baseline("1.0.2")
        self.set_version("1.0.1")
        with self.assertRaisesRegex(ValueError, "cannot decrease"):
            versions.check_increment(self.root, baseline)
        with self.assertRaisesRegex(ValueError, "Cannot resolve version baseline"):
            versions.check_increment(self.root, "1" * 40)


if __name__ == "__main__":
    unittest.main()
