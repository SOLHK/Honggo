import unittest
from tools.simulator_target import select_target, verify_booted, version_tuple


class SimulatorTargetTests(unittest.TestCase):
    def setUp(self):
        self.runtime = "com.apple.CoreSimulator.SimRuntime.iOS-27-2"
        self.device = "com.apple.CoreSimulator.SimDeviceType.iPhone-17-Pro-Max"
        self.inventory = {
            "runtimes": [
                {"identifier": self.runtime, "version": "27.2", "buildversion": "fixture-build", "isAvailable": True},
                {"identifier": "com.apple.CoreSimulator.SimRuntime.iOS-27-3", "version": "27.3", "isAvailable": True},
            ],
            "devicetypes": [{"identifier": self.device, "name": "iPhone 17 Pro Max"}],
            "devices": {self.runtime: [{"name": "iPhone 17 Pro Max", "deviceTypeIdentifier": self.device,
                                       "isAvailable": True, "state": "Booted", "udid": "fixture-udid"}]},
        }

    def test_exact_os_wins_over_newest(self):
        target = select_target(self.inventory, "27.2")
        self.assertEqual(target["runtime"], self.runtime)
        self.assertEqual(target["deviceType"], self.device)

    def test_absent_os_fails_instead_of_using_newer(self):
        self.inventory["runtimes"].pop(0)
        with self.assertRaisesRegex(ValueError, "No OS fallback"):
            select_target(self.inventory, "27.2")

    def test_unavailable_os_fails(self):
        self.inventory["runtimes"][0]["isAvailable"] = False
        with self.assertRaises(ValueError):
            select_target(self.inventory, "27.2")

    def test_patch_release_is_not_silently_substituted(self):
        self.inventory["runtimes"][0]["version"] = "27.2.1"
        with self.assertRaises(ValueError):
            select_target(self.inventory, "27.2")

    def test_equivalent_zero_patch_is_accepted(self):
        self.assertEqual(version_tuple("27.2.0"), version_tuple("27.2"))

    def test_invalid_version_rejected(self):
        for invalid in ("latest", "27", "27.2-beta", "27.2; echo wrong", None):
            with self.assertRaises(ValueError):
                version_tuple(invalid)

    def test_runtime_can_create_device_without_existing_devices(self):
        self.inventory["devices"] = {}
        self.assertEqual(select_target(self.inventory, "27.2")["deviceType"], self.device)

    def test_supported_device_type_is_respected(self):
        self.inventory["devices"] = {}
        self.inventory["runtimes"][0]["supportedDeviceTypes"] = ["com.apple.invalid"]
        with self.assertRaises(ValueError):
            select_target(self.inventory, "27.2")

    def test_boot_verifies_actual_device_on_exact_runtime(self):
        evidence = verify_booted(self.inventory, select_target(self.inventory, "27.2"), "fixture-udid")
        self.assertTrue(evidence["verified"])
        self.assertEqual(evidence["runtimeVersion"], "27.2")

    def test_wrong_udid_cannot_pass(self):
        with self.assertRaises(ValueError):
            verify_booted(self.inventory, select_target(self.inventory, "27.2"), "wrong")

    def test_shutdown_device_cannot_pass(self):
        target = select_target(self.inventory, "27.2")
        self.inventory["devices"][self.runtime][0]["state"] = "Shutdown"
        with self.assertRaises(ValueError):
            verify_booted(self.inventory, target, "fixture-udid")

    def test_device_on_another_os_cannot_pass(self):
        target = select_target(self.inventory, "27.2")
        self.inventory["devices"]["com.apple.CoreSimulator.SimRuntime.iOS-27-3"] = self.inventory["devices"].pop(self.runtime)
        with self.assertRaises(ValueError):
            verify_booted(self.inventory, target, "fixture-udid")

    def test_missing_type_id_resolves_by_model(self):
        self.inventory["devices"][self.runtime][0].pop("deviceTypeIdentifier")
        self.assertEqual(select_target(self.inventory, "27.2")["deviceType"], self.device)

    def test_wrong_platform_rejected(self):
        self.inventory["runtimes"][0]["identifier"] = "com.apple.CoreSimulator.SimRuntime.tvOS-27-2"
        with self.assertRaises(ValueError):
            select_target(self.inventory, "27.2")


if __name__ == "__main__":
    unittest.main()
