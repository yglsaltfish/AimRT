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
    stderr = process_info.stderr or ""

    # 自定义检查逻辑
    if "hello world foo" not in stdout:
        return CallbackResult(success=False, message="未找到预期日志")

    return CallbackResult(success=True, message="检测到有正确的日志输出")

def my_log_check_2(context: Dict[str, Any]) -> CallbackResult:
    """自定义日志检查回调"""
    process_info = context.get('process_info')

    if not process_info:
        return CallbackResult(success=False, message="缺少进程信息")
    stdout = process_info.stdout or ""

    if "warn" in stdout:
        return CallbackResult(success=False, message="检测到有警告日志输出")

    return CallbackResult(success=True, message="检测到没有警告日志输出")


class TestRPCExamples:
    """
    RPC examples test class demonstrating both programmatic and YAML-based testing.

    This class shows how to:
    - Test RPC communication between client and server
    - Use YAML configuration for test setup
    - Validate RPC request/response patterns
    - Handle process dependencies and timing
    """

    @pytest.mark.aimrt
    def test_rpc_from_yaml_config(self, aimrt_test_runner: AimRTTestRunner):
        """
        Test RPC communication using YAML configuration.

        This test demonstrates:
        - Loading test configuration from YAML file
        - Setting up test environment automatically
        - Managing process dependencies and timing
        - Validating RPC communication patterns
        """
        # Path to the YAML test configuration
        yaml_config_path = Path(__file__).parent / "test_rpc_ros2.yaml"

        if not yaml_config_path.exists():
            pytest.skip(f"YAML config file not found: {yaml_config_path}")

        # Setup test environment from YAML configuration
        if not aimrt_test_runner.setup_from_yaml(str(yaml_config_path)):
            pytest.fail("Failed to setup test environment from YAML configuration")


        aimrt_test_runner.register_function_callback(
            name="custom_log_check",
            trigger=CallbackTrigger.PROCESS_END,
            func=my_log_check,
        )

        aimrt_test_runner.register_function_callback(
            name="custom_log_check_2",
            trigger=CallbackTrigger.PROCESS_END,
            func=my_log_check_2,
        )

        # Get the test configuration
        test_config = aimrt_test_runner.get_test_config()
        if not test_config:
            pytest.fail("Test configuration not loaded")

        print(f"🚀 Running test: {test_config.name}")
        print(f"📝 Description: {test_config.description}")

        # Run the test using the YAML configuration
        success = aimrt_test_runner.run_test()

        if not success:
            pytest.fail("RPC test execution failed")

        # Get process status for verification
        process_status = aimrt_test_runner.get_process_status()
        print(f"📊 Final process status: {process_status}")

        # Validate that processes completed successfully
        execution_results = aimrt_test_runner.get_execution_results()
        failed_processes = []

        for script_name, process_info in execution_results.items():
            print(f"   {script_name}: {process_info.status}")
            if process_info.status not in ["completed", "running", "killed"]:
                failed_processes.append(script_name)

        if failed_processes:
            pytest.fail(f"Some processes failed: {failed_processes}")

        print("✅ YAML-based RPC test completed successfully")


if __name__ == "__main__":
    pytest.main([__file__, "-v"])