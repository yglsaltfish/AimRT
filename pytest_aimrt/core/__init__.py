#!/usr/bin/env python3
# -*- coding: utf-8 -*-

"""
AimRT测试框架核心模块

提供测试配置管理、进程管理、资源监控、自定义回调和报告生成功能。
"""

from .config_manager import ConfigManager, TestConfig, ScriptConfig
from .resource_monitor import ResourceMonitor, ResourceSnapshot, ProcessMonitorData
from .process_manager import ProcessManager, ProcessInfo
from .callback_manager import (
    CallbackManager, BaseCallback, CallbackResult, CallbackConfig, CallbackTrigger,
    CustomFunctionCallback
)
from .report_generator import ReportGenerator
from .base_test import BaseAimRTTest

__all__ = [
    'ConfigManager', 'TestConfig', 'ScriptConfig',
    'ResourceMonitor', 'ResourceSnapshot', 'ProcessMonitorData',
    'ProcessManager', 'ProcessInfo',
    'CallbackManager', 'BaseCallback', 'CallbackResult', 'CallbackConfig', 'CallbackTrigger',
    'CustomFunctionCallback',
    'ReportGenerator',
    'BaseAimRTTest'
]
