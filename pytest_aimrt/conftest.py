#!/usr/bin/env python3
# -*- coding: utf-8 -*-

"""
AimRT测试框架pytest配置文件

提供全局的pytest配置和fixtures。
"""

import sys
from pathlib import Path

# 将pytest_aimrt添加到Python路径中
sys.path.insert(0, str(Path(__file__).parent))

# 导入fixtures
from pytest_aimrt.fixtures.aimrt_test import aimrt_test_runner, aimrt_test_runner_session

# 重新导出fixtures，使其在全局可用
__all__ = ['aimrt_test_runner', 'aimrt_test_runner_session']
