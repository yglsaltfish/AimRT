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

def my_log_check(context: Dict[str, Any]) -> CallbackResult:
    """自定义日志检查回调"""
    process_info = context.get('process_info')

    if not process_info:
        return CallbackResult(success=False, message="缺少进程信息")

    # 检查日志
    stdout = process_info.stdout or ""
    _stderr = process_info.stderr or ""

    # 自定义检查逻辑
    if "Bench completed." not in stdout:
        return CallbackResult(success=False, message="未找到预期日志")

    return CallbackResult(success=True, message="检测到有正确的日志输出")



class TestRPCExamples:
    @pytest.mark.aimrt
    def test_mqtt_qos2_remote_config(self, aimrt_test_runner: AimRTTestRunner):
        yaml_config_path = Path(__file__).parent / "test_mqtt_qos2_remote.yaml"

        if not yaml_config_path.exists():
            pytest.skip(f"YAML config file not found: {yaml_config_path}")

        if not aimrt_test_runner.setup_from_yaml(str(yaml_config_path)):
            pytest.fail("Failed to setup test environment from YAML configuration")

        aimrt_test_runner.register_function_callback(
            name="log_check",
            trigger=CallbackTrigger.PROCESS_END,
            func=my_log_check,
        )

        success = aimrt_test_runner.run_test()

        if not success:
            pytest.fail("channel test execution failed")

    @pytest.mark.aimrt
    def test_mqtt_qos2_config(self, aimrt_test_runner: AimRTTestRunner):
        yaml_config_path = Path(__file__).parent / "test_mqtt_qos2.yaml"

        if not yaml_config_path.exists():
            pytest.skip(f"YAML config file not found: {yaml_config_path}")

        if not aimrt_test_runner.setup_from_yaml(str(yaml_config_path)):
            pytest.fail("Failed to setup test environment from YAML configuration")

        aimrt_test_runner.register_function_callback(
            name="log_check",
            trigger=CallbackTrigger.PROCESS_END,
            func=my_log_check,
        )

        success = aimrt_test_runner.run_test()

        if not success:
            pytest.fail("channel test execution failed")

if __name__ == "__main__":
    pytest.main([__file__, "-v"])