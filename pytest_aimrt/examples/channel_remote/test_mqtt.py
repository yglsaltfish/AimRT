#!/usr/bin/env python3
# -*- coding: utf-8 -*-

"""
AimRT RPC Examples Test Framework Demonstration

This module demonstrates how to use the pytest_aimrt testing framework
to test AimRT RPC examples using both programmatic and YAML-based configurations.

Requirements covered:
- 1.3: Support both C++ and Python AimRT examples
- 2.4: Capture and validate output logs for expected behavior
- 3.1: Support YAML-based test configuration
"""

import pytest
from pathlib import Path
from pytest_aimrt.fixtures.aimrt_test import AimRTTestRunner
from pytest_aimrt.core.callback_manager import CallbackTrigger, CallbackResult
from typing import Dict, Any

def rpc_service_check(context: Dict[str, Any]) -> CallbackResult:
    print(context)
    return CallbackResult(True, "rpc service check")

class TestRPCExamples:
    # @pytest.mark.aimrt
    # def test_mqtt_qos2_remote_config(self, aimrt_test_runner: AimRTTestRunner):
    #     yaml_config_path = Path(__file__).parent / "test_mqtt_qos2_remote.yaml"

    #     if not yaml_config_path.exists():
    #         pytest.skip(f"YAML config file not found: {yaml_config_path}")

    #     if not aimrt_test_runner.setup_from_yaml(str(yaml_config_path)):
    #         pytest.fail("Failed to setup test environment from YAML configuration")

    #     success = aimrt_test_runner.run_test()

    #     if not success:
    #         pytest.fail("channel test execution failed")

    # @pytest.mark.aimrt
    # def test_mqtt_qos2_config(self, aimrt_test_runner: AimRTTestRunner):
    #     yaml_config_path = Path(__file__).parent / "test_mqtt_qos2.yaml"

    #     if not yaml_config_path.exists():
    #         pytest.skip(f"YAML config file not found: {yaml_config_path}")

    #     if not aimrt_test_runner.setup_from_yaml(str(yaml_config_path)):
    #         pytest.fail("Failed to setup test environment from YAML configuration")

    #     success = aimrt_test_runner.run_test()

    #     if not success:
    #         pytest.fail("channel test execution failed")

    @pytest.mark.aimrt
    def test_mqtt_qos2_config(self, aimrt_test_runner: AimRTTestRunner):
        yaml_config_path = Path(__file__).parent / "bench_examples_plugins_ros2_plugin_pb_rpc_benchmark_besteffort.yaml"

        if not yaml_config_path.exists():
            pytest.skip(f"YAML config file not found: {yaml_config_path}")

        if not aimrt_test_runner.setup_from_yaml(str(yaml_config_path)):
            pytest.fail("Failed to setup test environment from YAML configuration")

        aimrt_test_runner.register_function_callback(
            name="rpc_service_check",
            trigger=CallbackTrigger.PROCESS_END,
            func=rpc_service_check,
        )

        success = aimrt_test_runner.run_test()

        if not success:
            pytest.fail("channel test execution failed")

if __name__ == "__main__":
    pytest.main([__file__, "-v"])