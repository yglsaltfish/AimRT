#!/usr/bin/env python3
# -*- coding: utf-8 -*-

import threading
from abc import ABC, abstractmethod
from typing import Dict, List, Optional, Any, Callable
from dataclasses import dataclass, field
from datetime import datetime
from enum import Enum


class CallbackTrigger(Enum):
    """回调触发时机"""
    PROCESS_START = "process_start"      # 进程启动时
    PROCESS_END = "process_end"          # 进程结束时
    PERIODIC = "periodic"                # 周期性检查
    ON_DEMAND = "on_demand"              # 按需调用
    RESOURCE_THRESHOLD = "resource_threshold"  # 资源阈值触发


@dataclass
class CallbackResult:
    """回调执行结果"""
    success: bool
    message: str
    data: Dict[str, Any] = field(default_factory=dict)
    timestamp: datetime = field(default_factory=datetime.now)
    warnings: List[str] = field(default_factory=list)
    errors: List[str] = field(default_factory=list)


@dataclass
class CallbackConfig:
    """回调配置"""
    name: str
    trigger: CallbackTrigger
    enabled: bool = True
    interval_sec: float = 1.0  # 周期性检查的间隔（秒）
    timeout_sec: float = 10.0  # 回调执行超时时间
    retry_count: int = 0       # 失败重试次数
    params: Dict[str, Any] = field(default_factory=dict)  # 自定义参数


class BaseCallback(ABC):
    """回调基础类"""

    def __init__(self, config: CallbackConfig):
        self.config = config
        self.results: List[CallbackResult] = []
        self._lock = threading.Lock()

    @abstractmethod
    def execute(self, context: Dict[str, Any]) -> CallbackResult:
        """
        执行回调逻辑

        Args:
            context: 执行上下文，包含进程信息、监控数据等

        Returns:
            CallbackResult: 执行结果
        """
        pass

    def get_results(self) -> List[CallbackResult]:
        """获取所有执行结果"""
        with self._lock:
            return self.results.copy()

    def add_result(self, result: CallbackResult):
        """添加执行结果"""
        with self._lock:
            self.results.append(result)

    def clear_results(self):
        """清空执行结果"""
        with self._lock:
            self.results.clear()

class ResourceThresholdCallback(BaseCallback):
    """资源阈值检测回调示例"""

    def execute(self, context: Dict[str, Any]) -> CallbackResult:
        """检查资源使用是否超过阈值"""
        try:
            monitor_data = context.get('monitor_data')
            if not monitor_data or not monitor_data.snapshots:
                return CallbackResult(
                    success=True,
                    message="无监控数据，跳过资源检查"
                )

            # 获取阈值配置
            cpu_threshold = self.config.params.get('cpu_threshold', 80.0)
            memory_threshold = self.config.params.get('memory_threshold', 80.0)

            warnings = []
            errors = []

            # 检查最新的资源快照
            latest_snapshot = monitor_data.snapshots[-1]

            if latest_snapshot.cpu_percent > cpu_threshold:
                warnings.append(f"CPU使用率过高: {latest_snapshot.cpu_percent:.1f}% > {cpu_threshold}%")

            if latest_snapshot.memory_percent > memory_threshold:
                warnings.append(f"内存使用率过高: {latest_snapshot.memory_percent:.1f}% > {memory_threshold}%")

            # 统计信息
            avg_cpu = sum(s.cpu_percent for s in monitor_data.snapshots) / len(monitor_data.snapshots)
            avg_memory = sum(s.memory_percent for s in monitor_data.snapshots) / len(monitor_data.snapshots)

            data = {
                'latest_cpu_percent': latest_snapshot.cpu_percent,
                'latest_memory_percent': latest_snapshot.memory_percent,
                'avg_cpu_percent': avg_cpu,
                'avg_memory_percent': avg_memory,
                'cpu_threshold': cpu_threshold,
                'memory_threshold': memory_threshold,
                'snapshot_count': len(monitor_data.snapshots)
            }

            return CallbackResult(
                success=len(errors) == 0,
                message=f"资源检查完成: CPU平均{avg_cpu:.1f}%, 内存平均{avg_memory:.1f}%",
                data=data,
                warnings=warnings,
                errors=errors
            )

        except Exception as e:
            return CallbackResult(
                success=False,
                message=f"资源检查异常: {e}",
                errors=[str(e)]
            )


class CustomFunctionCallback(BaseCallback):
    """自定义函数回调"""

    def __init__(self, config: CallbackConfig, func: Callable[[Dict[str, Any]], CallbackResult]):
        super().__init__(config)
        self.func = func

    def execute(self, context: Dict[str, Any]) -> CallbackResult:
        """执行自定义函数"""
        try:
            return self.func(context)
        except Exception as e:
            return CallbackResult(
                success=False,
                message=f"自定义函数执行异常: {e}",
                errors=[str(e)]
            )


class CallbackManager:
    """回调管理器"""

    def __init__(self):
        self._callbacks: Dict[str, BaseCallback] = {}
        self._periodic_threads: Dict[str, threading.Thread] = {}
        self._stop_events: Dict[str, threading.Event] = {}
        self._lock = threading.Lock()

    def register_callback(self, callback: BaseCallback):
        """注册回调"""
        with self._lock:
            self._callbacks[callback.config.name] = callback
            print(f"✅ 注册回调: {callback.config.name} ({callback.config.trigger.value})")

    def register_function_callback(self, name: str, trigger: CallbackTrigger,
                                 func: Callable[[Dict[str, Any]], CallbackResult],
                                 **kwargs) -> BaseCallback:
        """注册函数回调"""
        config = CallbackConfig(name=name, trigger=trigger, **kwargs)
        callback = CustomFunctionCallback(config, func)
        self.register_callback(callback)
        return callback

    def unregister_callback(self, name: str):
        """取消注册回调"""
        with self._lock:
            if name in self._callbacks:
                # 停止周期性任务
                if name in self._periodic_threads:
                    self._stop_events[name].set()
                    self._periodic_threads[name].join(timeout=1)
                    del self._periodic_threads[name]
                    del self._stop_events[name]

                del self._callbacks[name]
                print(f"🗑️ 取消注册回调: {name}")

    def start_periodic_callbacks(self, context_provider: Callable[[], Dict[str, Any]]):
        """启动周期性回调"""
        with self._lock:
            for name, callback in self._callbacks.items():
                if (callback.config.trigger in (CallbackTrigger.PERIODIC, CallbackTrigger.RESOURCE_THRESHOLD) and
                    callback.config.enabled and
                    name not in self._periodic_threads):

                    stop_event = threading.Event()
                    self._stop_events[name] = stop_event

                    thread = threading.Thread(
                        target=self._periodic_callback_worker,
                        args=(callback, context_provider, stop_event),
                        daemon=True,
                        name=f"callback-{name}"
                    )
                    self._periodic_threads[name] = thread
                    thread.start()
                    print(f"🔄 启动周期性回调: {name}")

    def stop_periodic_callbacks(self):
        """停止所有周期性回调"""
        with self._lock:
            for name, stop_event in self._stop_events.items():
                stop_event.set()

            for name, thread in self._periodic_threads.items():
                thread.join(timeout=1)
                print(f"⏹️ 停止周期性回调: {name}")

            self._periodic_threads.clear()
            self._stop_events.clear()

    def execute_callbacks(self, trigger: CallbackTrigger, context: Dict[str, Any]) -> Dict[str, CallbackResult]:
        """执行指定触发时机的回调"""
        results = {}

        with self._lock:
            callbacks_to_execute = [
                (name, callback) for name, callback in self._callbacks.items()
                if callback.config.trigger == trigger and callback.config.enabled
            ]

        for name, callback in callbacks_to_execute:
            try:
                print(f"🔍 执行回调: {name}")
                # 在上下文中补充通用字段，便于回调与报告聚合
                enriched_context = dict(context)
                pinfo = context.get('process_info')
                if pinfo:
                    enriched_context.setdefault('script_path', getattr(pinfo, 'script_path', None))
                    enriched_context.setdefault('pid', getattr(pinfo, 'pid', None))
                    # 若回调指定了目标脚本名单，则仅在这些脚本上触发
                    try:
                        target_scripts = callback.config.params.get('target_scripts', [])
                    except Exception:
                        target_scripts = []
                    if target_scripts:
                        current_script = getattr(pinfo, 'script_path', None)
                        if current_script not in set(target_scripts):
                            continue
                result = callback.execute(enriched_context)
                # 回调可能未返回结果，做防御
                if result is None:
                    result = CallbackResult(
                        success=False,
                        message="回调未返回结果",
                        errors=["callback returned None"]
                    )
                # 将脚本名写入结果data，方便报告按脚本聚合
                if pinfo:
                    try:
                        result.data.setdefault('script_path', getattr(pinfo, 'script_path', None))
                    except Exception:
                        # 如果data不可写或非dict，降级赋一个新dict
                        result.data = {'script_path': getattr(pinfo, 'script_path', None)}
                callback.add_result(result)
                results[name] = result

                if result.success:
                    print(f"✅ 回调成功: {name} - {result.message}")
                else:
                    # 支持软失败打印，降低噪声
                    soft_fail = callback.config.params.get('soft_fail', False)
                    if soft_fail:
                        print(f"⚠️ 回调软失败: {name} - {result.message}")
                    else:
                        print(f"❌ 回调失败: {name} - {result.message}")

                if result.warnings:
                    for warning in result.warnings:
                        print(f"⚠️ 警告: {warning}")

            except Exception as e:
                error_result = CallbackResult(
                    success=False,
                    message=f"回调执行异常: {e}",
                    errors=[str(e)]
                )
                callback.add_result(error_result)
                results[name] = error_result
                print(f"❌ 回调异常: {name} - {e}")

        return results

    def get_callback_results(self, callback_name: Optional[str] = None) -> Dict[str, List[CallbackResult]]:
        """获取回调执行结果"""
        results = {}

        with self._lock:
            if callback_name:
                if callback_name in self._callbacks:
                    results[callback_name] = self._callbacks[callback_name].get_results()
            else:
                for name, callback in self._callbacks.items():
                    results[name] = callback.get_results()

        return results

    def clear_callback_results(self, callback_name: Optional[str] = None):
        """清空回调执行结果"""
        with self._lock:
            if callback_name:
                if callback_name in self._callbacks:
                    self._callbacks[callback_name].clear_results()
            else:
                for callback in self._callbacks.values():
                    callback.clear_results()

    def _periodic_callback_worker(self, callback: BaseCallback,
                                context_provider: Callable[[], Dict[str, Any]],
                                stop_event: threading.Event):
        """周期性回调工作线程"""
        while not stop_event.wait(callback.config.interval_sec):
            try:
                context = context_provider()
                result = callback.execute(context)
                callback.add_result(result)

                if not result.success:
                    print(f"⚠️ 周期性回调失败: {callback.config.name} - {result.message}")

            except Exception as e:
                error_result = CallbackResult(
                    success=False,
                    message=f"周期性回调异常: {e}",
                    errors=[str(e)]
                )
                callback.add_result(error_result)
                print(f"❌ 周期性回调异常: {callback.config.name} - {e}")

    def get_registered_callbacks(self) -> Dict[str, CallbackConfig]:
        """获取已注册的回调配置"""
        with self._lock:
            return {name: callback.config for name, callback in self._callbacks.items()}