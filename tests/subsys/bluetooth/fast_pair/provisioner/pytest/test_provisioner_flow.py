# Copyright (c) 2026 Nordic Semiconductor ASA
#
# SPDX-License-Identifier: LicenseRef-Nordic-5-Clause

from __future__ import annotations

import logging
from dataclasses import dataclass, field
from pathlib import Path

from twister_harness import DeviceAdapter
from twister_harness_ext.utils.common import reset_board
from twister_harness_ext.utils.helpers import run_command

logger = logging.getLogger(__name__)

PROVISIONER_SCENARIO = "fast_pair.provisioner.image"
APP_PROV_SCENARIO = "fast_pair.provisioner.app_prov"

PROVISIONER_DONE = r"FP_PROV_TEST: provisioner image done"
ZTEST_DONE = r"PROJECT EXECUTION (SUCCESSFUL|FAILED)"

FLASH_TIMEOUT = 180
READ_TIMEOUT = 30


@dataclass
class StepResult:
    name: str
    passed: bool
    detail: str = ""


@dataclass
class Pipeline:
    dut: DeviceAdapter
    results: list[StepResult] = field(default_factory=list)

    def _flash(self, build_dir: Path, recover: bool) -> None:
        command = ["west", "flash", "--skip-rebuild", "--no-reset", "-d", str(build_dir)]
        if self.dut.device_config.id:
            command += ["--dev-id", self.dut.device_config.id]
        if recover:
            command += ["--recover"]
        run_command(command, timeout=FLASH_TIMEOUT)

    def step(
        self, name: str, build_dir: Path, until: str, expect: list[str], recover: bool = False
    ) -> None:
        logger.info(f"=== STEP: {name} ({build_dir.name}) ===")
        try:
            self._flash(build_dir, recover)
            self.dut.clear_buffer()
            reset_board(self.dut.device_config.id)
            lines = self.dut.readlines_until(regex=until, print_output=True, timeout=READ_TIMEOUT)
            output = "\n".join(lines)
            missing = [e for e in expect if e not in output]
            passed = not missing
            detail = f"missing: {missing}" if missing else ""
        except Exception as exc:
            passed, detail = False, repr(exc)
        self.results.append(StepResult(name, passed, detail))

    def summary(self) -> bool:
        logger.info("=" * 60)
        logger.info("Fast Pair provisioner flow summary")
        for r in self.results:
            logger.info(f"  {'PASS' if r.passed else 'FAIL'}  {r.name}  {r.detail}")
        logger.info("=" * 60)
        return all(r.passed for r in self.results)


def _find_build_dir(required_build_dirs: list[str], scenario: str) -> Path:
    for build_dir in required_build_dirs:
        if Path(build_dir).name.endswith(scenario):
            return Path(build_dir)
    raise AssertionError(f"Required build '{scenario}' not found in {required_build_dirs}")


def test_provisioner_flow(unlaunched_dut: DeviceAdapter, required_build_dirs: list[str]):
    dut = unlaunched_dut
    app_unprov = Path(dut.device_config.build_dir)
    provisioner = _find_build_dir(required_build_dirs, PROVISIONER_SCENARIO)
    app_prov = _find_build_dir(required_build_dirs, APP_PROV_SCENARIO)

    # Open the UART before the first flash, so no boot output is lost.
    dut.start_reader()
    dut.connect()

    pipeline = Pipeline(dut)

    pipeline.step(
        "erase + unprovisioned app",
        app_unprov,
        ZTEST_DONE,
        expect=["FP_PROV_TEST: unprovisioned app image started", "PROJECT EXECUTION SUCCESSFUL"],
        recover=True,
    )
    pipeline.step(
        "provisioner (first run)",
        provisioner,
        PROVISIONER_DONE,
        expect=["FP_PROV_TEST: provisioner image started"],
    )
    pipeline.step(
        "provisioned app",
        app_prov,
        ZTEST_DONE,
        expect=["FP_PROV_TEST: provisioned app image started", "PROJECT EXECUTION SUCCESSFUL"],
    )
    pipeline.step(
        "provisioner (second run)",
        provisioner,
        PROVISIONER_DONE,
        expect=["FP_PROV_TEST: provisioner image started"],
    )

    assert pipeline.summary(), "Fast Pair provisioner flow failed, see the summary above"
