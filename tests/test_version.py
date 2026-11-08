import subprocess
import tempfile
import unittest
from pathlib import Path

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
        self.assertEqual(versions.version_tuple("1.0.1"), (1, 0, 1))

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
