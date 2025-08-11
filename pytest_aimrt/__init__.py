#!/usr/bin/env python3
# -*- coding: utf-8 -*-

"""
AimRT 测试框架

基于pytest的AimRT测试框架，支持YAML配置、资源监控和进程管理。
"""

__version__ = "0.1.0"
__author__ = "AimRT Team"

from .core import (
    BaseAimRTTest,
    ConfigManager,
    TestConfig,
    ScriptConfig,
    ResourceMonitor,
    ProcessManager,
    CallbackManager,
    BaseCallback,
    CallbackResult,
    CallbackTrigger
)

from .fixtures.aimrt_test import AimRTTestRunner
from .utils import TimeoutError, with_timeout

__all__ = [
    'BaseAimRTTest',
    'ConfigManager',
    'TestConfig',
    'ScriptConfig',
    'ResourceMonitor',
    'ProcessManager',
    'CallbackManager',
    'BaseCallback',
    'CallbackResult',
    'CallbackTrigger',
    'AimRTTestRunner',
    'TimeoutError',
    'with_timeout'
]