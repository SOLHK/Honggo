"""Exercise the actual tab-bar screenshot transport without a simulator.

Only the capture subprocess is mocked. The shell's embedded Python, live-phase
guards, PNG validation, temporary files, and final-file publication all execute.
"""

import ast
import contextlib
import io
from pathlib import Path
import re
import struct
import subprocess
import sys
import tempfile
import unittest
from unittest import mock
import zlib


def capture_program():
    script = Path(__file__).resolve().parents[1] / "tools" / "test-tabbar-ios.sh"
    candidates = []
    for source in re.findall(r"<<'PY'\n(.*?)\nPY(?:\n|$)", script.read_text(), re.S):
        tree = ast.parse(source, filename=str(script))
        functions = {node.name for node in tree.body if isinstance(node, ast.FunctionDef)}
        if {"capture", "require_current_phase", "validate_png"} <= functions:
            candidates.append(tree)
    if len(candidates) != 1:
        raise AssertionError("Expected exactly one embedded simulator capture program")
    return compile(candidates[0], str(script) + ":capture", "exec")


def valid_png():
    def chunk(kind, data):
        return (struct.pack(">I", len(data)) + kind + data +
                struct.pack(">I", zlib.crc32(kind + data) & 0xFFFFFFFF))

    return (b"\x89PNG\r\n\x1a\n" +
            chunk(b"IHDR", struct.pack(">IIBBBBB", 1, 1, 8, 6, 0, 0, 0)) +
            chunk(b"IDAT", zlib.compress(b"\0\xff\0\0\xff")) +
            chunk(b"IEND", b""))


class SimulatorCaptureTests(unittest.TestCase):
    @classmethod
    def setUpClass(cls):
        cls.program = capture_program()
        cls.png = valid_png()

    def run_scenario(self, scenario):
        with tempfile.TemporaryDirectory() as temporary:
            root = Path(temporary)
            destination = root / "phase-system-simulator.png"
            phase_file = root / "phase.txt"
            result_file = root / "results.json"
            phase_file.write_text("tabbar-material-dark")
            destination.write_bytes(b"stale capture from a prior invocation")
            calls = []

            def run(arguments, check, timeout):
                self.assertTrue(check)
                self.assertEqual(timeout, 50)
                self.assertEqual(arguments[:5], ["xcrun", "simctl", "io", "device", "screenshot"])
                self.assertFalse(destination.exists(), "Never expose a stale or partial final capture")
                target = Path(arguments[-1])
                self.assertNotEqual(target, destination)
                calls.append(target)
                self.assertLessEqual(len(calls), 2)
                self.assertEqual(len(set(calls)), len(calls), "Retry paths must be unique")
                attempt = len(calls)

                if scenario == "two_timeouts" or (attempt == 1 and scenario in {
                        "timeout_then_success", "phase_changed_after_failure", "finished_after_failure"}):
                    target.write_bytes(b"partial capture")
                    if scenario == "phase_changed_after_failure":
                        phase_file.write_text("other-phase")
                    if scenario == "finished_after_failure":
                        result_file.write_text("{}")
                    raise subprocess.TimeoutExpired(arguments, 50)
                if scenario == "exit_then_success" and attempt == 1:
                    raise subprocess.CalledProcessError(1, arguments)

                corrupt = scenario == "two_corrupt" or (scenario == "corrupt_then_success" and attempt == 1)
                target.write_bytes(b"corrupt PNG" if corrupt else self.png)
                if scenario == "phase_changed_after_success":
                    phase_file.write_text("other-phase")
                if scenario == "finished_after_success":
                    result_file.write_text("{}")
                return subprocess.CompletedProcess(arguments, 0)

            arguments = ["capture", "device", str(destination), "tabbar-material-dark",
                         str(phase_file), str(result_file)]
            with mock.patch.object(sys, "argv", arguments), \
                    mock.patch("subprocess.run", side_effect=run), \
                    contextlib.redirect_stdout(io.StringIO()), \
                    contextlib.redirect_stderr(io.StringIO()), \
                    self.assertRaises(SystemExit) as stopped:
                exec(self.program, {})

            success = scenario in {"success", "timeout_then_success", "exit_then_success", "corrupt_then_success"}
            self.assertEqual(stopped.exception.code, 0 if success else 1)
            self.assertEqual(destination.exists(), success)
            if success:
                self.assertEqual(destination.read_bytes(), self.png)
            self.assertEqual(list(root.glob("*.attempt-*.png")), [], "Temporary captures must be removed")
            expected_calls = 2 if scenario in {
                "timeout_then_success", "exit_then_success", "corrupt_then_success", "two_timeouts", "two_corrupt"
            } else 1
            self.assertEqual(len(calls), expected_calls)

    def test_first_capture_succeeds(self):
        self.run_scenario("success")

    def test_timeout_retries_with_a_unique_file(self):
        self.run_scenario("timeout_then_success")

    def test_transport_exit_retries(self):
        self.run_scenario("exit_then_success")

    def test_corrupt_capture_retries_before_publication(self):
        self.run_scenario("corrupt_then_success")

    def test_changed_phase_prevents_retry(self):
        self.run_scenario("phase_changed_after_failure")

    def test_finished_app_prevents_retry(self):
        self.run_scenario("finished_after_failure")

    def test_changed_phase_prevents_success_publication(self):
        self.run_scenario("phase_changed_after_success")

    def test_finished_app_prevents_success_publication(self):
        self.run_scenario("finished_after_success")

    def test_repeated_timeouts_stop_after_two_attempts(self):
        self.run_scenario("two_timeouts")

    def test_repeated_corrupt_captures_never_publish(self):
        self.run_scenario("two_corrupt")


if __name__ == "__main__":
    unittest.main()
