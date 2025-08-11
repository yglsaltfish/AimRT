#!/usr/bin/env python3
# -*- coding: utf-8 -*-

"""
AimRT测试框架超时处理工具

提供超时异常和相关工具函数。
"""

import signal
import functools
from typing import Any, Callable


class TimeoutError(Exception):
    """超时异常"""
    def __init__(self, message: str = "操作超时"):
        super().__init__(message)
        self.message = message


def timeout_handler(signum, frame):
    """超时信号处理器"""
    raise TimeoutError("操作超时")


def with_timeout(seconds: int):
    """
    超时装饰器

    Args:
        seconds: 超时时间（秒）

    Usage:
        @with_timeout(30)
        def long_running_function():
            # 你的代码
            pass
    """
    def decorator(func: Callable) -> Callable:
        @functools.wraps(func)
        def wrapper(*args, **kwargs) -> Any:
            # 设置超时信号
            old_handler = signal.signal(signal.SIGALRM, timeout_handler)
            signal.alarm(seconds)

            try:
                result = func(*args, **kwargs)
                return result
            finally:
                # 清除超时信号
                signal.alarm(0)
                signal.signal(signal.SIGALRM, old_handler)

        return wrapper
    return decorator