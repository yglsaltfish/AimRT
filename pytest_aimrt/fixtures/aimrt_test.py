#!/usr/bin/env python3
# -*- coding: utf-8 -*-

"""
AimRT测试框架Pytest Fixtures

提供pytest兼容的测试fixtures，避免继承基类导致的pytest收集问题。
"""

import pytest
from typing import Dict

from ..core.base_test import BaseAimRTTest
from ..core.pytest_results import record_report, reset_results


class AimRTTestRunner:
    """AimRT测试运行器，封装BaseAimRTTest功能供pytest使用"""

    def __init__(self):
        self._base_test = BaseAimRTTest()
        self._initialized = False

    def setup_from_yaml(self, yaml_path: str) -> bool:
        """从YAML配置文件设置测试环境"""
        result = self._base_test.setup_from_yaml(yaml_path)
        if result:
            self._initialized = True
        return result

    def run_test(self) -> bool:
        """执行测试"""
        if not self._initialized:
            raise RuntimeError("测试环境未初始化，请先调用setup_from_yaml")
        return self._base_test.run_test()

    def get_test_config(self):
        """获取测试配置"""
        return self._base_test.get_test_config()

    def get_process_status(self) -> Dict[str, str]:
        """获取进程状态"""
        return self._base_test.get_process_status()

    def get_execution_results(self):
        """获取执行结果"""
        return self._base_test.get_execution_results()

    def get_reports(self):
        """获取所有报告"""
        return self._base_test.get_reports()

    def register_callback(self, callback):
        """注册自定义回调"""
        return self._base_test.register_callback(callback)

    def register_function_callback(self, name: str, trigger, func, **kwargs):
        """注册函数回调"""
        return self._base_test.register_function_callback(name, trigger, func, **kwargs)

    def get_callback_results(self, callback_name=None):
        """获取回调执行结果"""
        return self._base_test.get_callback_results(callback_name)

    def cleanup(self):
        """清理测试环境"""
        self._base_test.cleanup()
        self._initialized = False


@pytest.fixture(scope="function")
def aimrt_test_runner():
    """
    AimRT测试运行器fixture

    提供AimRT测试功能，自动处理清理工作。

    Usage:
        def test_my_aimrt_feature(aimrt_test_runner):
            yaml_path = "path/to/test_config.yaml"
            assert aimrt_test_runner.setup_from_yaml(yaml_path)
            assert aimrt_test_runner.run_test()
    """
    runner = AimRTTestRunner()

    try:
        yield runner
    finally:
        # 自动清理
        runner.cleanup()


@pytest.fixture(scope="session")
def aimrt_test_runner_session():
    """
    会话级别的AimRT测试运行器fixture

    在整个测试会话中重用同一个实例，适用于需要跨多个测试的场景。
    注意：需要手动管理清理。
    """
    runner = AimRTTestRunner()

    try:
        yield runner
    finally:
        runner.cleanup()


def pytest_configure(config):
    """Pytest配置钩子"""
    # 注册自定义标记
    config.addinivalue_line(
        "markers", "aimrt: mark test as AimRT framework test"
    )
    config.addinivalue_line(
        "markers", "slow: mark test as slow running"
    )
    config.addinivalue_line(
        "markers", "integration: mark test as integration test"
    )

    # 重置一次结果收集
    try:
        reset_results()
    except Exception:
        pass


def pytest_runtest_logreport(report):
    """收集每个用例阶段的测试结果（聚合为最终报告）。"""
    try:
        # 仅记录 call 阶段，若 setup/teardown 失败也会以 error 形式覆盖
        longrepr = None
        if hasattr(report, 'longrepr') and report.longrepr:
            try:
                longrepr = str(report.longrepr)
            except Exception:
                longrepr = None
        keywords = sorted([str(k) for k, v in getattr(report, 'keywords', {}).items() if v])
        record_report(
            nodeid=report.nodeid,
            outcome=str(report.outcome),
            duration=float(getattr(report, 'duration', 0.0) or 0.0),
            when=str(report.when),
            longrepr=longrepr,
            keywords=keywords,
        )
    except Exception:
        pass