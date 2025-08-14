#!/usr/bin/env python3
# -*- coding: utf-8 -*-

"""
AimRT测试框架基础测试类

提供基于YAML配置的测试执行功能，集成资源监控和进程管理。
"""

import time
from typing import Dict, List, Optional, Any

from .config_manager import ConfigManager, TestConfig
from .resource_monitor import ResourceMonitor
from .process_manager import ProcessManager, ProcessInfo
from .callback_manager import CallbackManager, CallbackTrigger
from .report_generator import ReportGenerator
from .pytest_results import export_results as export_pytest_results


class BaseAimRTTest:
    """AimRT测试基础类"""

    def __init__(self):
        """初始化测试基础类"""
        self.config_manager = ConfigManager()
        self.resource_monitor = ResourceMonitor(sample_interval=1.0)
        self.callback_manager = CallbackManager()
        self.process_manager = None
        self.report_generator = ReportGenerator()
        self._test_config: Optional[TestConfig] = None
        self._execution_results: Dict[str, ProcessInfo] = {}
        self._reports: List[Dict[str, Any]] = []

    def setup_from_yaml(self, yaml_path: str) -> bool:
        """
        从YAML配置文件设置测试环境

        Args:
            yaml_path: YAML配置文件路径

        Returns:
            bool: 设置成功返回True
        """
        try:
            # 加载配置
            if not self.config_manager.load_config(yaml_path):
                print("❌ 加载YAML配置失败")
                return False

            # 验证配置
            if not self.config_manager.validate_config():
                print("❌ YAML配置验证失败")
                return False

            # 获取配置
            self._test_config = self.config_manager.get_config()
            if not self._test_config:
                print("❌ 获取测试配置失败")
                return False

            # 初始化进程管理器
            self.process_manager = ProcessManager(
                base_cwd=self._test_config.cwd,
                resource_monitor=self.resource_monitor,
                callback_manager=self.callback_manager
            )

            print(f"✅ 测试环境设置成功: {self._test_config.name}")
            return True

        except Exception as e:
            print(f"❌ 设置测试环境失败: {e}")
            return False

    def run_test(self) -> bool:
        """
        执行测试

        Returns:
            bool: 测试成功返回True
        """
        if not self._test_config or not self.process_manager:
            print("❌ 测试环境未初始化")
            return False

        print(f"\n🎯 开始执行测试: {self._test_config.name}")
        print(f"📝 描述: {self._test_config.description}")
        print(f"🔢 执行次数: {self._test_config.execution_count}")
        print(f"⏱️ 运行时间: {self._test_config.time_sec}秒")

        success_count = 0

        try:
            # 执行多次测试
            for execution in range(self._test_config.execution_count):
                if self._test_config.execution_count > 1:
                    print(f"\n📊 执行第 {execution + 1}/{self._test_config.execution_count} 次测试")

                # 执行脚本组
                results = self.process_manager.execute_scripts_with_dependencies(
                    self._test_config.scripts
                )

                self._execution_results.update(results)

                execution_success = True
                for script_path, process_info in results.items():
                    if process_info.status not in ["completed"]:
                        print(f"❌ 脚本执行失败: {script_path} (状态: {process_info.status})")
                        execution_success = False
                    else:
                        print(f"✅ 脚本执行成功: {script_path}")

                if execution_success:
                    success_count += 1
                    print(f"✅ 第 {execution + 1} 次测试执行成功")
                else:
                    print(f"❌ 第 {execution + 1} 次测试执行失败")

                if execution < self._test_config.execution_count - 1:
                    print("⏳ 等待2秒后开始下一次执行...")
                    time.sleep(2)

            # 生成报告
            self._generate_reports()

            # 计算成功率
            success_rate = (success_count / self._test_config.execution_count) * 100
            print("\n📈 测试完成统计:")
            print(f"   总执行次数: {self._test_config.execution_count}")
            print(f"   成功次数: {success_count}")
            print(f"   成功率: {success_rate:.1f}%")

            return success_count == self._test_config.execution_count

        except Exception as e:
            print(f"❌ 执行测试时发生错误: {e}")
            return False

    def get_test_config(self) -> Optional[TestConfig]:
        """获取测试配置"""
        return self._test_config

    def get_process_status(self) -> Dict[str, str]:
        """
        获取进程状态

        Returns:
            Dict[str, str]: 脚本路径到状态的映射
        """
        if not self.process_manager:
            return {}

        processes = self.process_manager.get_all_processes()
        return {path: info.status for path, info in processes.items()}

    def get_execution_results(self) -> Dict[str, ProcessInfo]:
        """获取执行结果"""
        return self._execution_results.copy()

    def get_reports(self) -> List[Dict[str, Any]]:
        """获取所有报告"""
        return self._reports

    def register_callback(self, callback):
        """注册自定义回调"""
        if self.process_manager:
            self.process_manager.register_callback(callback)
        else:
            self.callback_manager.register_callback(callback)

    def register_function_callback(self, name: str, trigger: CallbackTrigger, func, **kwargs):
        """注册函数回调（支持脚本级 enabled_callbacks 白名单）"""
        if self._test_config:
            # 判断是否开启白名单模式：只要任一脚本在YAML中出现了 enabled_callbacks 字段（即使为空列表），即开启
            whitelist_enabled = any(hasattr(sc, 'enabled_callbacks') and sc.enabled_callbacks is not None
                                    for sc in (self._test_config.scripts or []))

            if whitelist_enabled:
                # 收集每个脚本声明的 enabled_callbacks（可能为空，表示该脚本不允许任何回调）
                script_enabled_map = {}
                for sc in (self._test_config.scripts or []):
                    if sc.enabled_callbacks is None:
                        continue
                    for n in (sc.enabled_callbacks or []):
                        script_enabled_map.setdefault(n, set()).add(sc.path)

                # 若白名单开启但该回调未出现在任何脚本名单中，则不注册
                if name not in script_enabled_map:
                    print(f"⏭️ 跳过注册回调: {name} (白名单启用，未在任何脚本的enabled_callbacks中)")
                    return None
                # 注入目标脚本名单，执行阶段据此过滤
                kwargs = dict(kwargs or {})
                params = dict(kwargs.get('params', {}))
                params['target_scripts'] = sorted(list(script_enabled_map[name]))
                kwargs['params'] = params

        if self.process_manager:
            return self.process_manager.register_function_callback(name, trigger, func, **kwargs)
        else:
            return self.callback_manager.register_function_callback(name, trigger, func, **kwargs)

    def get_callback_results(self, callback_name: Optional[str] = None):
        """获取回调执行结果"""
        if self.process_manager:
            return self.process_manager.get_callback_results(callback_name)
        else:
            return self.callback_manager.get_callback_results(callback_name).copy()

    def _generate_reports(self):
        """生成测试报告"""
        if not self.process_manager:
            return

        print("\n📊 生成测试报告...")

        # generate summary report
        summary_report = self.process_manager.generate_summary_report()
        self._reports.append({
            "type": "summary",
            "timestamp": time.time(),
            "data": summary_report
        })

        self._print_resource_usage_report(summary_report)


        if self._test_config:
            callback_results = self.process_manager.get_callback_results() if self.process_manager else {}
            # 导出 pytest 结果
            try:
                pytest_results = export_pytest_results()
            except Exception:
                pytest_results = {"summary": {}, "tests": []}

            report_files = self.report_generator.generate_all_reports(
                self._test_config.name,
                self._execution_results,
                summary_report,
                callback_results,
                pytest_results
            )
            for format_type, filepath in report_files.items():
                print(f"📋 {format_type.upper()}报告: {filepath}")

        print("✅ 报告生成完成")

    def _print_resource_usage_report(self, summary_report: Dict[str, Any]):
        """打印资源使用情况报告"""
        print("\n📋 资源使用情况报告:")
        print("=" * 80)

        summary = summary_report["summary"]
        print(f"总进程数: {summary['total_processes']}")
        print(f"成功完成: {summary['completed']}")
        print(f"执行失败: {summary['failed']}")
        print(f"超时终止: {summary['timeout']}")
        print(f"强制终止: {summary['killed']}")
        print(f"成功率: {summary['success_rate']:.1f}%")
        print(f"总执行时间: {summary['total_duration_seconds']:.2f}秒")

        print("\n" + "─" * 80)
        print("进程详细资源使用情况:")
        print("─" * 80)

        # 打印每个进程的资源使用情况
        for script_path, process_data in summary_report["processes"].items():
            print(f"\n🔹 脚本: {script_path}")
            print(f"   状态: {process_data['status']}")
            print(f"   PID: {process_data['pid']}")
            print(f"   执行时间: {process_data['duration_seconds']:.2f}秒" if process_data['duration_seconds'] else "   执行时间: N/A")

            if "resource_usage" in process_data:
                resource = process_data["resource_usage"]

                # CPU使用情况
                cpu = resource.get("cpu", {})
                print(f"   💻 CPU使用率: 平均 {cpu.get('avg_percent', 0):.1f}%, "
                      f"最大 {cpu.get('max_percent', 0):.1f}%, "
                      f"最终 {cpu.get('final_percent', 0):.1f}%")

                # 内存使用情况
                memory = resource.get("memory", {})
                print(f"   🧠 内存使用: 平均 {memory.get('avg_rss_mb', 0):.1f}MB, "
                      f"最大 {memory.get('max_rss_mb', 0):.1f}MB, "
                      f"最终 {memory.get('final_rss_mb', 0):.1f}MB")
                print(f"   📊 内存占比: 平均 {memory.get('avg_percent', 0):.1f}%, "
                      f"最大 {memory.get('max_percent', 0):.1f}%, "
                      f"最终 {memory.get('final_percent', 0):.1f}%")

                # 磁盘I/O情况
                disk = resource.get("disk", {})
                print(f"   💾 磁盘读取: 总计 {disk.get('total_read_mb', 0):.2f}MB, "
                      f"平均 {disk.get('avg_read_mb_per_sec', 0):.2f}MB/s")
                print(f"   💾 磁盘写入: 总计 {disk.get('total_write_mb', 0):.2f}MB, "
                      f"平均 {disk.get('avg_write_mb_per_sec', 0):.2f}MB/s")
                print(f"   🔢 I/O次数: 読取 {disk.get('total_read_count', 0)}, "
                      f"写入 {disk.get('total_write_count', 0)}")
            else:
                print("   ⚠️ 无资源监控数据")

        print("\n" + "=" * 80)

    def cleanup(self):
        """清理测试环境"""
        print("\n🧹 清理测试环境...")

        if self.process_manager:
            self.process_manager.cleanup()

        # 停止所有资源监控
        self.resource_monitor.stop_all_monitoring()

        print("✅ 测试环境清理完成")

    def __enter__(self):
        """上下文管理器入口"""
        return self

    def __exit__(self, exc_type, exc_val, exc_tb):
        """上下文管理器出口"""
        self.cleanup()

    # pytest fixture支持
    def setup_method(self, method):
        """pytest方法级别的设置"""
        pass

    def teardown_method(self, method):
        """pytest方法级别的清理"""
        self.cleanup()

    def setup_class(self):
        """pytest类级别的设置"""
        pass

    def teardown_class(self):
        """pytest类级别的清理"""
        self.cleanup()