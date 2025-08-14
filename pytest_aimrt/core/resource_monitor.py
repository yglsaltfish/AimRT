#!/usr/bin/env python3
# -*- coding: utf-8 -*-

"""
AimRT测试框架资源监控器

负责监控进程的CPU、内存、磁盘使用情况，并生成详细的监控报告。
"""

import psutil
import threading
import time
from typing import Dict, List, Optional, Any
from dataclasses import dataclass, field
from datetime import datetime
import json


@dataclass
class ResourceSnapshot:
    """资源使用快照"""
    timestamp: datetime
    cpu_percent: float
    memory_rss: int  # 物理内存 (bytes)
    memory_vms: int  # 虚拟内存 (bytes)
    memory_percent: float
    disk_read_bytes: int
    disk_write_bytes: int
    disk_read_count: int
    disk_write_count: int


@dataclass
class ProcessMonitorData:
    """进程监控数据"""
    pid: int
    name: str
    start_time: datetime
    end_time: Optional[datetime] = None
    snapshots: List[ResourceSnapshot] = field(default_factory=list)
    status: str = "running"
    exit_code: Optional[int] = None
    children: List[int] = field(default_factory=list)  # 子进程PID列表
    children_info: List[Dict[str, Any]] = field(default_factory=list)  # 子进程详细信息
    total_cpu_percent: float = 0.0  # 包括子进程的总CPU使用率
    total_memory_rss: int = 0  # 包括子进程的总内存使用量
    total_memory_percent: float = 0.0  # 包括子进程的总内存百分比


class ResourceMonitor:
    """资源监控器"""

    def __init__(self, sample_interval: float = 1.0):
        """
        初始化资源监控器

        Args:
            sample_interval: 采样间隔（秒）
        """
        self.sample_interval = sample_interval
        self._monitors: Dict[int, ProcessMonitorData] = {}
        self._monitor_threads: Dict[int, threading.Thread] = {}
        self._stop_events: Dict[int, threading.Event] = {}
        self._lock = threading.Lock()
        # 缓存psutil.Process对象，避免每次采样首次cpu_percent恒为0的问题
        self._proc_cache: Dict[int, psutil.Process] = {}
        # 标记已完成cpu_percent预热的pid（首次调用返回0，需要预热一次）
        self._cpu_primed: set[int] = set()

    def start_monitoring(self, pid: int, name: str) -> bool:
        """
        开始监控指定进程

        Args:
            pid: 进程ID
            name: 进程名称

        Returns:
            bool: 成功开始监控返回True
        """
        try:
            # 检查进程是否存在
            process = psutil.Process(pid)
            if not process.is_running():
                print(f"❌ 进程 {pid} ({name}) 不在运行")
                return False

            with self._lock:
                if pid in self._monitors:
                    print(f"⚠️ 进程 {pid} ({name}) 已在监控中")
                    return True

                # 缓存并预热cpu_percent，下一轮采样即可获得非零值
                self._proc_cache[pid] = process
                try:
                    process.cpu_percent(None)
                except Exception:
                    pass
                self._cpu_primed.add(pid)

                # 创建监控数据
                monitor_data = ProcessMonitorData(
                    pid=pid,
                    name=name,
                    start_time=datetime.now()
                )
                self._monitors[pid] = monitor_data

                # 创建停止事件
                stop_event = threading.Event()
                self._stop_events[pid] = stop_event

                # 启动监控线程
                monitor_thread = threading.Thread(
                    target=self._monitor_process,
                    args=(pid, stop_event),
                    daemon=True
                )
                self._monitor_threads[pid] = monitor_thread
                monitor_thread.start()

                print(f"🔍 开始监控进程 {pid} ({name})")
                return True

        except psutil.NoSuchProcess:
            print(f"❌ 进程 {pid} 不存在")
            return False
        except Exception as e:
            print(f"❌ 启动监控失败: {e}")
            return False

    def stop_monitoring(self, pid: int) -> Optional[ProcessMonitorData]:
        """
        停止监控指定进程

        Args:
            pid: 进程ID

        Returns:
            ProcessMonitorData: 监控数据，如果进程未被监控则返回None
        """
        with self._lock:
            if pid not in self._monitors:
                return None

            # 停止监控线程
            if pid in self._stop_events:
                self._stop_events[pid].set()

            if pid in self._monitor_threads:
                self._monitor_threads[pid].join(timeout=2.0)
                del self._monitor_threads[pid]

            if pid in self._stop_events:
                del self._stop_events[pid]

            # 获取监控数据
            monitor_data = self._monitors.pop(pid)
            monitor_data.end_time = datetime.now()

            # 获取最终状态
            try:
                process = psutil.Process(pid)
                if process.is_running():
                    monitor_data.status = "running"
                else:
                    monitor_data.status = "terminated"
                    monitor_data.exit_code = process.returncode
            except psutil.NoSuchProcess:
                monitor_data.status = "terminated"

            print(f"⏹️ 停止监控进程 {pid} ({monitor_data.name})")
            return monitor_data

    def stop_all_monitoring(self) -> Dict[int, ProcessMonitorData]:
        """
        停止所有监控

        Returns:
            Dict[int, ProcessMonitorData]: 所有监控数据
        """
        all_data = {}
        pids = list(self._monitors.keys())

        for pid in pids:
            data = self.stop_monitoring(pid)
            if data:
                all_data[pid] = data

        return all_data

    def _monitor_process(self, pid: int, stop_event: threading.Event):
        """监控进程及其子进程的资源使用情况"""
        try:
            # 复用已缓存的psutil.Process对象，避免每轮新建导致cpu_percent恒为0
            process = self._proc_cache.get(pid)
            if process is None:
                process = psutil.Process(pid)
                self._proc_cache[pid] = process

            while not stop_event.is_set():
                try:
                    # 检查进程是否还在运行
                    if not process.is_running():
                        break

                    # 获取进程及其所有子进程
                    all_processes = self._get_process_tree(pid)

                    # 计算总资源使用情况
                    total_cpu_percent = 0.0
                    total_memory_rss = 0
                    total_memory_percent = 0.0
                    total_disk_read_bytes = 0
                    total_disk_write_bytes = 0
                    total_disk_read_count = 0
                    total_disk_write_count = 0

                    # 获取主进程资源（带预热逻辑）
                    if pid in self._cpu_primed:
                        cpu_percent = process.cpu_percent(None)
                    else:
                        process.cpu_percent(None)
                        self._cpu_primed.add(pid)
                        cpu_percent = 0.0
                    memory_info = process.memory_info()
                    memory_percent = process.memory_percent()

                    total_cpu_percent += cpu_percent
                    total_memory_rss += memory_info.rss
                    total_memory_percent += memory_percent

                    # 获取主进程磁盘I/O
                    try:
                        io_counters = process.io_counters()
                        disk_read_bytes = io_counters.read_bytes
                        disk_write_bytes = io_counters.write_bytes
                        disk_read_count = io_counters.read_count
                        disk_write_count = io_counters.write_count
                        total_disk_read_bytes += disk_read_bytes
                        total_disk_write_bytes += disk_write_bytes
                        total_disk_read_count += disk_read_count
                        total_disk_write_count += disk_write_count
                    except (psutil.AccessDenied, AttributeError):
                        disk_read_bytes = 0
                        disk_write_bytes = 0
                        disk_read_count = 0
                        disk_write_count = 0

                    # 获取子进程资源
                    child_pids = []
                    child_cpu_map: Dict[int, float] = {}
                    for child_pid in all_processes:
                        if child_pid != pid:  # 跳过主进程
                            try:
                                child_process = self._proc_cache.get(child_pid)
                                if child_process is None:
                                    child_process = psutil.Process(child_pid)
                                    self._proc_cache[child_pid] = child_process
                                if child_process.is_running():
                                    child_pids.append(child_pid)

                                    # 子进程CPU使用率（带预热）
                                    if child_pid in self._cpu_primed:
                                        child_cpu = child_process.cpu_percent(None)
                                    else:
                                        child_process.cpu_percent(None)
                                        self._cpu_primed.add(child_pid)
                                        child_cpu = 0.0
                                    total_cpu_percent += child_cpu
                                    child_cpu_map[child_pid] = child_cpu

                                    # 子进程内存使用情况
                                    child_memory = child_process.memory_info()
                                    child_memory_percent = child_process.memory_percent()
                                    total_memory_rss += child_memory.rss
                                    total_memory_percent += child_memory_percent

                                    # 子进程磁盘I/O
                                    try:
                                        child_io = child_process.io_counters()
                                        total_disk_read_bytes += child_io.read_bytes
                                        total_disk_write_bytes += child_io.write_bytes
                                        total_disk_read_count += child_io.read_count
                                        total_disk_write_count += child_io.write_count
                                    except (psutil.AccessDenied, AttributeError):
                                        pass

                            except (psutil.NoSuchProcess, psutil.AccessDenied):
                                continue

                    # 创建快照（包含总资源使用情况）
                    snapshot = ResourceSnapshot(
                        timestamp=datetime.now(),
                        cpu_percent=total_cpu_percent,  # 使用总CPU使用率
                        memory_rss=total_memory_rss,    # 使用总内存使用量
                        memory_vms=memory_info.vms,     # 主进程虚拟内存
                        memory_percent=total_memory_percent,  # 使用总内存百分比
                        disk_read_bytes=total_disk_read_bytes,
                        disk_write_bytes=total_disk_write_bytes,
                        disk_read_count=total_disk_read_count,
                        disk_write_count=total_disk_write_count
                    )


                    # 更新监控数据
                    with self._lock:
                        if pid in self._monitors:
                            monitor_data = self._monitors[pid]
                            monitor_data.snapshots.append(snapshot)
                            monitor_data.children = []
                            monitor_data.children_info = []
                            monitor_data.total_cpu_percent = total_cpu_percent
                            monitor_data.total_memory_rss = total_memory_rss
                            monitor_data.total_memory_percent = total_memory_percent

                    # 等待下次采样
                    stop_event.wait(self.sample_interval)

                except psutil.NoSuchProcess:
                    break
                except psutil.AccessDenied:
                    print(f"⚠️ 无权限访问进程 {pid} 的资源信息")
                    break
                except Exception as e:
                    print(f"⚠️ 监控进程 {pid} 时出错: {e}")
                    time.sleep(self.sample_interval)

        except Exception as e:
            print(f"❌ 监控线程异常: {e}")

    def _get_process_tree(self, pid: int) -> List[int]:
        """
        获取进程及其所有子进程的PID列表

        Args:
            pid: 主进程PID

        Returns:
            List[int]: 包含主进程和所有子进程的PID列表
        """
        try:
            # 直接返回PID本身以及其递归子进程PID列表
            all_pids = [pid]  # 包含主进程

            # 递归获取所有子进程
            def get_children(parent_pid: int):
                try:
                    parent = psutil.Process(parent_pid)
                    children = parent.children(recursive=True)
                    for child in children:
                        all_pids.append(child.pid)
                except (psutil.NoSuchProcess, psutil.AccessDenied):
                    pass

            get_children(pid)
            return all_pids

        except (psutil.NoSuchProcess, psutil.AccessDenied):
            return [pid]

    def get_monitor_data(self, pid: int) -> Optional[ProcessMonitorData]:
        """获取指定进程的监控数据"""
        with self._lock:
            return self._monitors.get(pid)

    def get_all_monitor_data(self) -> Dict[int, ProcessMonitorData]:
        """获取所有监控数据"""
        with self._lock:
            return self._monitors.copy()

    def get_children_info(self, pid: int) -> List[Dict[str, Any]]:
        """
        获取指定进程的子进程详细信息

        Args:
            pid: 主进程PID

        Returns:
            List[Dict[str, Any]]: 子进程信息列表
        """
        with self._lock:
            monitor_data = self._monitors.get(pid)
            if not monitor_data:
                return []

        # 返回保存的子进程详细信息
        return monitor_data.children_info

    def generate_report(self, monitor_data: ProcessMonitorData) -> Dict[str, Any]:
        """
        生成监控报告

        Args:
            monitor_data: 监控数据

        Returns:
            Dict[str, Any]: 详细的监控报告
        """
        if not monitor_data.snapshots:
            return {
                "pid": monitor_data.pid,
                "name": monitor_data.name,
                "status": "no_data",
                "message": "没有收集到监控数据"
            }

        snapshots = monitor_data.snapshots

        # 计算统计信息
        cpu_values = [s.cpu_percent for s in snapshots]
        memory_rss_values = [s.memory_rss for s in snapshots]
        memory_percent_values = [s.memory_percent for s in snapshots]

        # 计算磁盘I/O增量（第一个快照作为基准）
        if len(snapshots) > 1:
            disk_read_delta = snapshots[-1].disk_read_bytes - snapshots[0].disk_read_bytes
            disk_write_delta = snapshots[-1].disk_write_bytes - snapshots[0].disk_write_bytes
            disk_read_count_delta = snapshots[-1].disk_read_count - snapshots[0].disk_read_count
            disk_write_count_delta = snapshots[-1].disk_write_count - snapshots[0].disk_write_count
        else:
            disk_read_delta = 0
            disk_write_delta = 0
            disk_read_count_delta = 0
            disk_write_count_delta = 0

        # 计算运行时间
        end_time = monitor_data.end_time or datetime.now()
        duration = (end_time - monitor_data.start_time).total_seconds()

        report = {
            "pid": monitor_data.pid,
            "name": monitor_data.name,
            "status": monitor_data.status,
            "exit_code": monitor_data.exit_code,
            "duration_seconds": duration,
            "start_time": monitor_data.start_time.isoformat(),
            "end_time": end_time.isoformat(),
            "sample_count": len(snapshots),
            "cpu": {
                "min_percent": min(cpu_values),
                "max_percent": max(cpu_values),
                "avg_percent": sum(cpu_values) / len(cpu_values),
                "final_percent": cpu_values[-1],
                "total_percent": monitor_data.total_cpu_percent  # 包括子进程的总CPU使用率
            },
            "memory": {
                "min_rss_mb": min(memory_rss_values) / 1024 / 1024,
                "max_rss_mb": max(memory_rss_values) / 1024 / 1024,
                "avg_rss_mb": sum(memory_rss_values) / len(memory_rss_values) / 1024 / 1024,
                "final_rss_mb": memory_rss_values[-1] / 1024 / 1024,
                "min_percent": min(memory_percent_values),
                "max_percent": max(memory_percent_values),
                "avg_percent": sum(memory_percent_values) / len(memory_percent_values),
                "final_percent": memory_percent_values[-1],
                "total_rss_mb": monitor_data.total_memory_rss / 1024 / 1024,  # 包括子进程的总内存
                "total_percent": monitor_data.total_memory_percent  # 包括子进程的总内存百分比
            },
            "disk": {
                "total_read_mb": disk_read_delta / 1024 / 1024,
                "total_write_mb": disk_write_delta / 1024 / 1024,
                "total_read_count": disk_read_count_delta,
                "total_write_count": disk_write_count_delta,
                "avg_read_mb_per_sec": (disk_read_delta / 1024 / 1024) / duration if duration > 0 else 0,
                "avg_write_mb_per_sec": (disk_write_delta / 1024 / 1024) / duration if duration > 0 else 0
            }
        }

        return report

    def export_detailed_data(self, monitor_data: ProcessMonitorData, format: str = "json") -> str:
        """
        导出详细的监控数据

        Args:
            monitor_data: 监控数据
            format: 导出格式 ("json" 或 "csv")

        Returns:
            str: 导出的数据字符串
        """
        if format == "json":
            data = {
                "process_info": {
                    "pid": monitor_data.pid,
                    "name": monitor_data.name,
                    "start_time": monitor_data.start_time.isoformat(),
                    "end_time": monitor_data.end_time.isoformat() if monitor_data.end_time else None,
                    "status": monitor_data.status,
                    "exit_code": monitor_data.exit_code
                },
                "snapshots": [
                    {
                        "timestamp": s.timestamp.isoformat(),
                        "cpu_percent": s.cpu_percent,
                        "memory_rss_mb": s.memory_rss / 1024 / 1024,
                        "memory_vms_mb": s.memory_vms / 1024 / 1024,
                        "memory_percent": s.memory_percent,
                        "disk_read_mb": s.disk_read_bytes / 1024 / 1024,
                        "disk_write_mb": s.disk_write_bytes / 1024 / 1024,
                        "disk_read_count": s.disk_read_count,
                        "disk_write_count": s.disk_write_count
                    }
                    for s in monitor_data.snapshots
                ]
            }
            return json.dumps(data, indent=2, ensure_ascii=False)

        elif format == "csv":
            lines = [
                "timestamp,cpu_percent,memory_rss_mb,memory_vms_mb,memory_percent,disk_read_mb,disk_write_mb,disk_read_count,disk_write_count"
            ]
            for s in monitor_data.snapshots:
                lines.append(
                    f"{s.timestamp.isoformat()},"
                    f"{s.cpu_percent},"
                    f"{s.memory_rss / 1024 / 1024:.2f},"
                    f"{s.memory_vms / 1024 / 1024:.2f},"
                    f"{s.memory_percent:.2f},"
                    f"{s.disk_read_bytes / 1024 / 1024:.2f},"
                    f"{s.disk_write_bytes / 1024 / 1024:.2f},"
                    f"{s.disk_read_count},"
                    f"{s.disk_write_count}"
                )
            return "\n".join(lines)

        else:
            raise ValueError(f"不支持的导出格式: {format}")