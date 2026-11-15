import unittest
from tools.glass_expectations import encode_sdk_version, compatibility_opt_out_effective, expected_unsupported_reference


class GlassExpectationsTests(unittest.TestCase):
    def test_sdk_26_preserves_negative_compatibility_control(self):
        self.assertTrue(expected_unsupported_reference("reference-compat", 260500))

    def test_sdk_27_requires_visible_glass_despite_opt_out(self):
        self.assertFalse(expected_unsupported_reference("reference-compat", 270200))

    def test_other_modes_always_require_real_visible_glass(self):
        for mode in ("current", "startup", "reference-modern"):
            for sdk in (260500, 270200):
                self.assertFalse(expected_unsupported_reference(mode, sdk))

    def test_boundary_is_build_sdk_not_deployment_target(self):
        self.assertTrue(compatibility_opt_out_effective(269999))
        self.assertFalse(compatibility_opt_out_effective(270000))

    def test_missing_or_fake_evidence_is_rejected(self):
        for value in (None, True, False, "27.2", 27.2, 0, 27, -1, 1000000):
            with self.assertRaises(ValueError):
                compatibility_opt_out_effective(value)

    def test_sdk_matches_apple_compile_macro_encoding(self):
        self.assertEqual(encode_sdk_version("27.2"), 270200)
        self.assertEqual(encode_sdk_version("26.5.1"), 260501)

    def test_invalid_sdk_string_fails_closed(self):
        for value in (None, "latest", "27", "27.2 beta", "27.100", "27.2.100"):
            with self.assertRaises(ValueError):
                encode_sdk_version(value)

    def test_every_mode_requires_sdk_evidence(self):
        with self.assertRaises(ValueError):
            expected_unsupported_reference("current", None)


if __name__ == "__main__":
    unittest.main()
