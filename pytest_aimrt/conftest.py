#!/usr/bin/env python3
# -*- coding: utf-8 -*-

"""
AimRT测试框架pytest配置文件

提供全局的pytest配置和fixtures。
"""

import sys
from pathlib import Path


sys.path.insert(0, str(Path(__file__).parent))

from pytest_aimrt.fixtures.aimrt_test import aimrt_test_runner, aimrt_test_runner_session

__all__ = ['aimrt_test_runner', 'aimrt_test_runner_session']
