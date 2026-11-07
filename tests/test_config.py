"""Run with ESPHome's Python environment: python tests/test_config.py."""

import importlib.util
from pathlib import Path
import unittest

from esphome import config_validation as cv
from esphome.core import CORE


SOURCE = Path(__file__).resolve().parents[1] / "components/hi_link_ld1125h/__init__.py"
spec = importlib.util.spec_from_file_location("hi_link_ld1125h", SOURCE)
component = importlib.util.module_from_spec(spec)
spec.loader.exec_module(component)


class ConfigurationTests(unittest.TestCase):
    def test_aligned_limits(self):
        for key, (_, minimum, maximum, _) in component.PARAMETERS.items():
            with self.subTest(key=key):
                component.validate_limits({key: {"min_value": minimum, "max_value": maximum}})
        component.validate_limits({"max_distance": {"min_value": 0.3, "max_value": 8.1}})

    def test_unaligned_limits(self):
        for key in component.PARAMETERS:
            for limit in ("min_value", "max_value"):
                config = {"min_value": 0, "max_value": 8}
                config[limit] = 0.05 if key == "max_distance" else 0.75
                with self.subTest(key=key, limit=limit):
                    with self.assertRaisesRegex(cv.Invalid, f"{limit} must be a multiple"):
                        component.validate_limits({key: config})

    def test_reversed_or_equal_limits(self):
        for minimum in (8, 9):
            with self.assertRaisesRegex(cv.Invalid, "greater than min_value"):
                component.validate_limits({"max_distance": {"min_value": minimum, "max_value": 8}})

    def test_restore_modes(self):
        for mode in (None, *component.switch.RESTORE_MODES):
            CORE.reset()
            switch = {"name": "Test Mode"}
            if mode is not None:
                switch["restore_mode"] = mode
            with self.subTest(mode=mode):
                config = component.CONFIG_SCHEMA({"test_mode": switch})
                self.assertEqual(str(config["test_mode"]["restore_mode"]), mode or "ALWAYS_OFF")


if __name__ == "__main__":
    unittest.main()
