"""Host-side regressions for configuration input, deadline analysis and verdicts.

SPDX-License-Identifier: MIT
"""

import contextlib
import copy
import io
from pathlib import Path
import tempfile
import unittest
from unittest.mock import patch

import framework
from run_test import result_passed


class ToolTests(unittest.TestCase):
    def setUp(self):
        self.config = {"policy": "PTL_POLICY_SKIP", "tasks": [{
            "name": "Sensor", "period": 100, "deadline": 100, "wcet": 10,
            "priority": 2, "policy": "PTL_POLICY_USE_GLOBAL",
        }]}

    def test_invalid_generated_identifiers_and_timing(self):
        for field, value in [("name", "bad-name"), ("name", 'bad"name'),
                             ("name", "NameMuchTooLong"), ("period", 0),
                             ("wcet", -1), ("deadline", 101),
                             ("deadline", 0), ("priority", 8)]:
            with self.subTest(field=field, value=value):
                config = copy.deepcopy(self.config)
                config["tasks"][0][field] = value
                with self.assertRaises(ValueError):
                    framework.validate_config(config)

    def test_duplicate_names_and_capacity(self):
        config = copy.deepcopy(self.config)
        config["tasks"] *= 2
        with self.assertRaises(ValueError):
            framework.validate_config(config)
        config["tasks"] *= 5
        with self.assertRaises(ValueError):
            framework.validate_config(config)

    def test_deadline_miss_even_below_full_utilization(self):
        self.config["tasks"][0].update(wcet=10, deadline=5)
        with contextlib.redirect_stdout(io.StringIO()):
            self.assertFalse(framework.perform_analysis(self.config))

    def test_completion_exactly_at_deadline(self):
        self.config["tasks"][0].update(wcet=5, deadline=5)
        with contextlib.redirect_stdout(io.StringIO()):
            self.assertTrue(framework.perform_analysis(self.config))

    def test_generation_without_launching_qemu(self):
        with tempfile.TemporaryDirectory() as directory:
            output = Path(directory) / "main_gen.c"
            with patch.object(framework, "GENERATED_FILE", str(output)):
                with contextlib.redirect_stdout(io.StringIO()):
                    framework.generate_code(self.config)
            source = output.read_text()
            self.assertIn("Job_Sensor", source)
            self.assertIn("PTL_POLICY_USE_GLOBAL", source)
            self.assertIn("PTL_Init(&c, t, 1)", source)

    def test_pass_cannot_mask_errors(self):
        self.assertTrue(result_passed("[PASS] Completed\n"))
        for error in ["[FAIL] Bad job", "[FAIL - Required <=10%]",
                      "[FATAL] Memory", "[ERROR] Init", "HardFault", "assertion failed"]:
            with self.subTest(error=error):
                self.assertFalse(result_passed(error + "\n[PASS] Completed\n"))
        self.assertFalse(result_passed("No verdict\n"))


if __name__ == "__main__":
    unittest.main()
