#!/usr/bin/env python3
# -*- coding: utf-8 -*-

"""
AimRT测试框架进程管理器

负责执行脚本、管理进程依赖关系、处理超时控制，并集成资源监控功能。
"""

import subprocess
import threading
import time
import os

import psutil
from typing import Dict, List, Optional, Any, Literal
from dataclasses import dataclass
from datetime import datetime
from pathlib import Path

from .config_manager import ScriptConfig
from .resource_monitor import ResourceMonitor, ProcessMonitorData
from .callback_manager import CallbackManager, CallbackTrigger


@dataclass
class ProcessInfo:
    """进程信息"""
    script_path: str
    pid: int
    process: subprocess.Popen
    start_time: datetime
    end_time: Optional[datetime] = None
    exit_code: Optional[int] = None
    status: Literal["running", "completed", "failed", "timeout", "killed"] = "running"
    stdout: str = ""
    stderr: str = ""
    monitor_data: Optional[ProcessMonitorData] = None
    terminated_by_framework: bool = False


class ProcessManager:
    """进程管理器"""

    def __init__(self, base_cwd: str = "", resource_monitor: Optional[ResourceMonitor] = None,
                 callback_manager: Optional[CallbackManager] = None):
        """
        初始化进程管理器

        Args:
            base_cwd: 基础工作目录
            resource_monitor: 资源监控器实例
            callback_manager: 回调管理器实例
        """
        self.base_cwd = Path(base_cwd) if base_cwd else Path.cwd()
        self.resource_monitor = resource_monitor or ResourceMonitor()
        self.callback_manager = callback_manager or CallbackManager()
        self._processes: Dict[str, ProcessInfo] = {}
        self._lock = threading.Lock()

    def execute_script(self, script_config: ScriptConfig, run_time: Optional[int] = None) -> ProcessInfo:
        """
        执行单个脚本

        Args:
            script_config: 脚本配置
            run_time: 运行时间（秒），如果为None则使用脚本配置中的运行时间

        Returns:
            ProcessInfo: 进程信息
        """
        script_run_time = run_time or script_config.time_sec

        # 准备执行环境
        env = os.environ.copy()
        env.update(script_config.environment)

        # 确定工作目录
        if script_config.cwd:
            cwd = self.base_cwd / script_config.cwd
        else:
            cwd = self.base_cwd

        # 构建命令
        cmd = [script_config.path] + script_config.args

        print(f"🚀 执行脚本: {script_config.path}")
        print(f"   工作目录: {cwd}")
        print(f"   运行时间: {script_run_time}秒")
        print(f"   环境变量: {script_config.environment}")

        try:
            # 启动进程
            process = subprocess.Popen(
                cmd,
                cwd=str(cwd),
                env=env,
                stdout=subprocess.PIPE,
                stderr=subprocess.PIPE,
                text=True,
                bufsize=1,
                universal_newlines=True
            )

                        # 创建进程信息
            process_info = ProcessInfo(
                script_path=script_config.path,
                pid=process.pid,
                process=process,
                start_time=datetime.now()
            )

            with self._lock:
                self._processes[script_config.path] = process_info

            # 开始资源监控
            monitor_enabled = any(script_config.monitor.values())
            print(f"📊 监控配置: {script_config.monitor}, 启用监控: {monitor_enabled}")
            if monitor_enabled:
                success = self.resource_monitor.start_monitoring(process.pid, script_config.path)
                if success:
                    print(f"✅ 资源监控已启动: {script_config.path} (PID: {process.pid})")
                else:
                    print(f"❌ 资源监控启动失败: {script_config.path} (PID: {process.pid})")

            print(f"✅ 脚本启动成功，PID: {process.pid}")

            # 跳过YAML回调注册，仅支持代码中注册的函数回调

            # 触发进程启动回调
            context = {
                'process_info': process_info,
                'script_config': script_config
            }
            self.callback_manager.execute_callbacks(CallbackTrigger.PROCESS_START, context)

            self.start_periodic_callbacks()

            # 如果配置了 shutdown_patterns，启动后台日志监视线程
            if script_config.shutdown_patterns:
                self._start_shutdown_pattern_watcher(process_info, script_config)

            # 启动运行时间控制线程
            if script_run_time > 0:
                self._start_run_time_control(process_info, script_run_time, script_config.kill_signal)

            return process_info

        except Exception as e:
            print(f"❌ 启动脚本失败: {e}")
            # 创建失败的进程信息
            process_info = ProcessInfo(
                script_path=script_config.path,
                pid=-1,
                process=None,
                start_time=datetime.now(),
                end_time=datetime.now(),
                status="failed",
                stderr=str(e)
            )
            return process_info

    def wait_for_process(self, script_path: str, timeout: Optional[int] = None) -> ProcessInfo:
        """
        等待进程完成

        Args:
            script_path: 脚本路径
            timeout: 超时时间（秒），None表示无超时等待

        Returns:
            ProcessInfo: 更新后的进程信息
        """
        with self._lock:
            process_info = self._processes.get(script_path)

        if not process_info or not process_info.process:
            print(f"❌ 未找到脚本进程: {script_path}")
            return process_info

        print(f"⏳ 等待脚本执行完成: {script_path}")

        try:
            # 等待进程完成
            if timeout is not None:
                stdout, stderr = process_info.process.communicate(timeout=timeout)
            else:
                stdout, stderr = process_info.process.communicate()

            # 更新进程信息
            process_info.end_time = datetime.now()
            process_info.exit_code = process_info.process.returncode
            process_info.stdout = stdout
            process_info.stderr = stderr

            # 若之前已被标记为 killed/timeout，则尊重既有状态，避免覆盖
            if process_info.status in ["killed", "timeout"]:
                label = "被终止" if process_info.status == "killed" else "执行超时"
                print(f"⏹️ 脚本{label}: {script_path}")
            else:
                if process_info.exit_code == 0:
                    process_info.status = "completed"
                    print(f"✅ 脚本执行成功: {script_path}")
                else:
                    process_info.status = "failed"
                    print(f"❌ 脚本执行失败: {script_path}, 退出码: {process_info.exit_code}")

        except subprocess.TimeoutExpired:
            print(f"⏰ 脚本执行超时: {script_path}")
            process_info.status = "timeout"
            process_info.end_time = datetime.now()

            # 终止进程
            self.kill_process(script_path)

        except Exception as e:
            print(f"❌ 等待脚本时发生错误: {e}")
            process_info.status = "failed"
            process_info.end_time = datetime.now()
            process_info.stderr = str(e)

        # 停止资源监控
        if process_info.pid > 0:
            print(f"🛑 停止资源监控: {script_path} (PID: {process_info.pid})")
            monitor_data = self.resource_monitor.stop_monitoring(process_info.pid)
            if monitor_data:
                process_info.monitor_data = monitor_data
                print(f"📊 收集到监控数据: {len(monitor_data.snapshots)} 个采样点")
            else:
                print(f"⚠️ 未收集到监控数据: {script_path}")

        # 触发进程结束回调
        context = {
            'process_info': process_info,
            'monitor_data': process_info.monitor_data
        }
        self.callback_manager.execute_callbacks(CallbackTrigger.PROCESS_END, context)

        return process_info

    def kill_process(self, script_path: str) -> bool:
        """
        强制终止进程及其所有子进程

        Args:
            script_path: 脚本路径

        Returns:
            bool: 成功终止返回True
        """
        with self._lock:
            process_info = self._processes.get(script_path)

        if not process_info or not process_info.process:
            return False

        try:
            print(f"🔪 强制终止进程及其子进程: {script_path} (PID: {process_info.pid})")

            try:
                parent_process = psutil.Process(process_info.pid)
                child_processes = parent_process.children(recursive=True)
                all_processes = [parent_process] + child_processes

            except psutil.NoSuchProcess:
                print(f"⚠️  进程 {process_info.pid} 已经不存在")
                process_info.status = "killed"
                process_info.end_time = datetime.now()
                process_info.terminated_by_framework = True
                return True

            for proc in reversed(all_processes):
                try:
                    proc.terminate()
                except psutil.NoSuchProcess:
                    continue

            _, alive = psutil.wait_procs(all_processes, timeout=5)

            if alive:
                print(f"⚠️  有 {len(alive)} 个进程未能温和终止，强制杀死...")
                for proc in alive:
                    try:
                        proc.kill()
                    except psutil.NoSuchProcess:
                        continue
                psutil.wait_procs(alive, timeout=3)
                process_info.status = "killed"
                process_info.end_time = datetime.now()
                process_info.terminated_by_framework = True

            else:
                process_info.status = "killed"
                process_info.end_time = datetime.now()
                process_info.terminated_by_framework = True


            return True

        except Exception as e:
            print(f"❌ 终止进程失败: {e}")
            return False

    def execute_scripts_with_dependencies(self, scripts: List[ScriptConfig]) -> Dict[str, ProcessInfo]:
        """
        根据依赖关系并行执行脚本组

        新的执行逻辑：
        - 每个脚本是独立的，不需要等待前一个完全结束
        - 只要依赖的进程启动了，就可以根据delay时间启动下一个
        - 支持真正的并行执行

        Args:
            scripts: 脚本配置列表

        Returns:
            Dict[str, ProcessInfo]: 所有进程的执行结果
        """
        print("🎯 开始并行执行脚本组")

        # 构建脚本映射（用于依赖检查）
        results = {}
        started_scripts = set()  # 已启动的脚本

        # 启动线程来处理脚本启动
        import threading

        def start_script_when_ready(script_config: ScriptConfig):
            """当依赖满足时启动脚本"""
            script_path = script_config.path

            # 等待依赖的脚本启动
            while True:
                dependencies_satisfied = True
                for dep_path in script_config.depends_on:
                    if dep_path not in started_scripts:
                        dependencies_satisfied = False
                        break

                if dependencies_satisfied:
                    break

                # 短暂等待后重新检查
                time.sleep(0.5)

            # 等待延迟时间
            if script_config.delay_sec > 0:
                print(f"⏱️ 等待 {script_config.delay_sec} 秒后启动: {script_path}")
                time.sleep(script_config.delay_sec)

            # 启动脚本
            print(f"🚀 启动脚本: {script_path}")
            process_info = self.execute_script(script_config)
            results[script_path] = process_info

            # 标记为已启动（即使启动失败也要标记，避免阻塞其他脚本）
            started_scripts.add(script_path)

        # 为每个脚本启动监控线程
        threads = []
        for script in scripts:
            thread = threading.Thread(
                target=start_script_when_ready,
                args=(script,),
                daemon=True
            )
            threads.append(thread)
            thread.start()

        # 等待所有脚本启动完成
        for thread in threads:
            thread.join()

        print(f"✅ 所有脚本已启动，共 {len(started_scripts)} 个")

        # 现在所有脚本都在运行，等待它们完成或超时
        print("⏳ 等待所有脚本执行完成...")

                # 等待所有进程完成
        for script in scripts:
            script_path = script.path
            if script_path in results and results[script_path].status not in ["failed"]:
                # 等待进程自然结束（由运行时间控制线程管理）
                process_info = self.wait_for_process(script_path, None)
                results[script_path] = process_info

                status_emoji = "✅" if process_info.status == "completed" else "❌"
                print(f"{status_emoji} 脚本 {script_path} 执行{process_info.status}")

        return results

    def register_callback(self, callback):
        """注册自定义回调"""
        self.callback_manager.register_callback(callback)

    def register_function_callback(self, name: str, trigger: CallbackTrigger,
                                 func, **kwargs):
        """注册函数回调"""
        return self.callback_manager.register_function_callback(name, trigger, func, **kwargs)

    def get_callback_results(self, callback_name: Optional[str] = None):
        """获取回调执行结果"""
        return self.callback_manager.get_callback_results(callback_name)

    def start_periodic_callbacks(self):
        """启动周期性回调"""
        def context_provider():
            # 提供当前所有进程的上下文
            with self._lock:
                processes = list(self._processes.values())

            return {
                'all_processes': processes,
                'active_processes': [p for p in processes if p.status == "running"]
            }

        self.callback_manager.start_periodic_callbacks(context_provider)

    def stop_periodic_callbacks(self):
        """停止周期性回调"""
        self.callback_manager.stop_periodic_callbacks()

    def _register_script_callbacks(self, script_config: ScriptConfig):
        """已禁用：YAML回调注册（仅保留函数回调API）。"""
        return

    def _start_run_time_control(self, process_info: ProcessInfo, run_time: int, kill_signal: int):
        """
        启动运行时间控制线程

        Args:
            process_info: 进程信息
            run_time: 运行时间（秒）
            kill_signal: 终止信号
        """
        def run_time_controller():
            """运行时间控制器"""
            try:
                # 等待指定的运行时间
                time.sleep(run_time)

                # 检查进程是否还在运行
                if process_info.process and process_info.process.poll() is None:
                    print(f"⏰ 脚本 {process_info.script_path} 运行时间到期 ({run_time}秒)，关闭进程")
                    self.kill_process(process_info.script_path)

            except Exception as e:
                print(f"❌ 运行时间控制线程异常: {e}")

        # 启动控制线程
        control_thread = threading.Thread(
            target=run_time_controller,
            daemon=True
        )
        control_thread.start()

    def _start_shutdown_pattern_watcher(self, process_info: ProcessInfo, script_config: ScriptConfig):
        patterns = [p.lower() for p in script_config.shutdown_patterns]

        def watcher():
            try:
                proc = process_info.process
                if not proc:
                    return
                import selectors
                sel = selectors.DefaultSelector()
                if proc.stdout:
                    sel.register(proc.stdout, selectors.EVENT_READ)
                if proc.stderr:
                    sel.register(proc.stderr, selectors.EVENT_READ)

                matched = False
                while proc.poll() is None and not matched:
                    for key, _ in sel.select(timeout=0.2):
                        data = key.fileobj.readline()
                        if not data:
                            continue
                        if key.fileobj is proc.stdout:
                            process_info.stdout += data
                        else:
                            process_info.stderr += data
                        low = data.lower()
                        for p in patterns:
                            if p in low:
                                matched = True
                                break
                        if matched:
                            break

                if matched and proc.poll() is None:
                    print(f"🛎️ 触发shutdown_patterns，优雅终止: {process_info.script_path}")
                    try:
                        # 先尝试温和终止
                        parent = psutil.Process(process_info.pid)
                        for ch in parent.children(recursive=True):
                            try:
                                ch.terminate()
                            except psutil.NoSuchProcess:
                                pass
                        parent.terminate()
                        psutil.wait_procs([parent], timeout=3)
                    except Exception:
                        pass
                    # 设置状态为completed（视为正常结束）
                    process_info.status = "completed"
                    process_info.end_time = datetime.now()
            except Exception as e:
                print(f"❌ shutdown watcher异常: {e}")

        t = threading.Thread(target=watcher, daemon=True, name=f"shutdown-watcher-{process_info.pid}")
        t.start()

    def get_process_info(self, script_path: str) -> Optional[ProcessInfo]:
        """获取进程信息"""
        with self._lock:
            return self._processes.get(script_path)

    def get_all_processes(self) -> Dict[str, ProcessInfo]:
        """获取所有进程信息"""
        with self._lock:
            return self._processes.copy()

    def is_process_running(self, script_path: str) -> bool:
        """检查进程是否在运行"""
        process_info = self.get_process_info(script_path)
        if not process_info or not process_info.process:
            return False

        try:
            # 检查进程状态
            return process_info.process.poll() is None
        except Exception:
            return False

    def cleanup(self):
        """清理所有进程"""
        print("🧹 清理所有进程...")

        with self._lock:
            script_paths = list(self._processes.keys())

        # 终止所有运行中的进程
        for script_path in script_paths:
            if self.is_process_running(script_path):
                self.kill_process(script_path)

        # 停止所有资源监控
        self.resource_monitor.stop_all_monitoring()

        print("✅ 进程清理完成")

    def generate_summary_report(self) -> Dict[str, Any]:
        """生成执行总结报告"""
        processes = self.get_all_processes()

        total_count = len(processes)
        completed_count = sum(1 for p in processes.values() if p.status == "completed")
        failed_count = sum(1 for p in processes.values() if p.status == "failed")
        timeout_count = sum(1 for p in processes.values() if p.status == "timeout")
        killed_count = sum(1 for p in processes.values() if p.status == "killed")

        # 计算总执行时间
        start_times = [p.start_time for p in processes.values()]
        end_times = [p.end_time for p in processes.values() if p.end_time]

        if start_times and end_times:
            total_duration = (max(end_times) - min(start_times)).total_seconds()
        else:
            total_duration = 0

        report = {
            "summary": {
                "total_processes": total_count,
                "completed": completed_count,
                "failed": failed_count,
                "timeout": timeout_count,
                "killed": killed_count,
                "success_rate": (completed_count / total_count * 100) if total_count > 0 else 0,
                "total_duration_seconds": total_duration
            },
            "processes": {}
        }

        # 添加每个进程的详细信息
        for script_path, process_info in processes.items():
            process_report = {
                "script_path": script_path,
                "pid": process_info.pid,
                "status": process_info.status,
                "exit_code": process_info.exit_code,
                "start_time": process_info.start_time.isoformat(),
                "end_time": process_info.end_time.isoformat() if process_info.end_time else None,
                "duration_seconds": (
                    (process_info.end_time - process_info.start_time).total_seconds()
                    if process_info.end_time else None
                ),
                "stdout_length": len(process_info.stdout),
                "stderr_length": len(process_info.stderr),
                "has_errors": bool(process_info.stderr.strip())
            }

            # 添加资源监控报告
            if process_info.monitor_data:
                process_report["resource_usage"] = self.resource_monitor.generate_report(
                    process_info.monitor_data
                )

            report["processes"][script_path] = process_report

        return report