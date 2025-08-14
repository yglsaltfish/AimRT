#!/usr/bin/env python3
# -*- coding: utf-8 -*-

"""
AimRT测试框架配置管理器

负责加载和解析YAML测试配置文件，提供测试执行所需的配置信息。
"""

import yaml
from typing import Dict, List, Optional, Any
from dataclasses import dataclass, field
from pathlib import Path

@dataclass
class HostProfile:
    """远端主机档案"""
    name: str = ""
    host: str = ""
    ssh_user: str = ""
    ssh_port: int = 22
    ssh_password: str = ""
    remote_cwd: str = ""

@dataclass
class ScriptConfig:
    """脚本配置类"""
    path: str
    args: List[str] = field(default_factory=list)
    depends_on: List[str] = field(default_factory=list)
    delay_sec: int = 0
    time_sec: int = 60  # 运行时间（秒）
    kill_signal: int = 15  # 结束信号 (15=SIGTERM, 2=SIGINT)
    monitor: Dict[str, bool] = field(default_factory=lambda: {"cpu": True, "memory": True, "disk": True})
    environment: Dict[str, str] = field(default_factory=dict)
    cwd: str = ""
    shutdown_patterns: List[str] = field(default_factory=list)  # 匹配即请求优雅退出并记为completed
    enabled_callbacks: Optional[List[str]] = None  # 若字段在YAML中出现则为列表（可为空），否则为None 表示未启用白名单
    remote_env: Dict[str, str] = field(default_factory=dict)
    host_profile: Optional[HostProfile] = None


@dataclass
class TestConfig:
    """测试配置类"""
    name: str
    description: str = ""
    execution_count: int = 1
    time_sec: int = 60  # 总运行时间（秒）、
    cwd: str = ""
    environment: Dict[str, str] = field(default_factory=dict)
    scripts: List[ScriptConfig] = field(default_factory=list)


class ConfigManager:
    """配置管理器"""

    def __init__(self):
        self._config: Optional[TestConfig] = None

    def load_config(self, config_path: str) -> bool:
        """
        加载YAML配置文件

        Args:
            config_path: 配置文件路径

        Returns:
            bool: 加载成功返回True，失败返回False
        """
        try:
            config_file = Path(config_path)
            if not config_file.exists():
                print(f"❌ 配置文件不存在: {config_path}")
                return False

            with open(config_file, 'r', encoding='utf-8') as f:
                data = yaml.safe_load(f)

            if not data:
                print("❌ 配置文件为空")
                return False

            self._config = self._parse_config(data)
            print(f"✅ 成功加载配置: {self._config.name}")
            return True

        except yaml.YAMLError as e:
            print(f"❌ YAML解析错误: {e}")
            return False
        except Exception as e:
            print(f"❌ 加载配置文件失败: {e}")
            return False

    def _parse_config(self, data: Dict[str, Any]) -> TestConfig:
        """解析配置数据"""
        config_data = data.get('config', {})

        test_config = TestConfig(
            name=data.get('name', 'Unknown Test'),
            description=data.get('description', ''),
            execution_count=config_data.get('execution_count', 1),
            time_sec=config_data.get('time_sec', 60),
            cwd=config_data.get('cwd', ''),
            environment=config_data.get('environment', {})
        )

        input_data = data.get('input', {})
        scripts_data = input_data.get('scripts', [])

        hosts_profiles_raw = data.get('hosts', {}) or {}
        hosts_profiles: Dict[str, HostProfile] = {}
        for name, prof in hosts_profiles_raw.items():
            if not isinstance(prof, dict):
                continue
            hosts_profiles[name] = HostProfile(
                name=name,
                host=prof.get('host', ''),
                ssh_user=prof.get('ssh_user', ''),
                ssh_port=int(prof.get('ssh_port', 22) or 22),
                ssh_password=prof.get('ssh_password', ''),
                remote_cwd=prof.get('remote_cwd', ''),
            )

        for script_data in scripts_data:
            script_remote = script_data.get('remote', '')
            host_prof = hosts_profiles.get(script_remote) if script_remote else None


            merged_remote_env: Dict[str, str] = {}
            prof_env = getattr(host_prof, 'remote_env', {}) if host_prof else {}
            if isinstance(prof_env, dict):
                merged_remote_env.update(prof_env)
            scr_env = script_data.get('remote_env', {}) or {}
            if isinstance(scr_env, dict):
                merged_remote_env.update(scr_env)

            if 'enabled_callbacks' in script_data:
                enabled_callbacks = script_data.get('enabled_callbacks') or []
            else:
                enabled_callbacks = None

            script_config = ScriptConfig(
                path=script_data.get('path', ''),
                args=script_data.get('args', []),
                depends_on=script_data.get('depends_on', []),
                delay_sec=script_data.get('delay_sec', 0),
                time_sec=script_data.get('time_sec', 60),
                kill_signal=script_data.get('kill_signal', 15),
                monitor=script_data.get('monitor', {"cpu": True, "memory": True, "disk": True}),
                environment=script_data.get('environment', {}),
                cwd=script_data.get('cwd', config_data.get('cwd', '')),
                shutdown_patterns=script_data.get('shutdown_patterns', []),
                enabled_callbacks=enabled_callbacks,
                remote_env=merged_remote_env,
                host_profile=host_prof
            )
            test_config.scripts.append(script_config)

        return test_config

    def get_config(self) -> Optional[TestConfig]:
        """获取当前配置"""
        return self._config

    def validate_config(self) -> bool:
        """验证配置的有效性"""
        if not self._config:
            return False

        for script in self._config.scripts:
            if not script.path:
                print("❌ 脚本路径为空")
                return False

        script_paths = {script.path for script in self._config.scripts}
        for script in self._config.scripts:
            for dep in script.depends_on:
                if dep not in script_paths:
                    print(f"❌ 脚本 {script.path} 依赖的脚本 {dep} 不存在")
                    return False

        return True

    def get_execution_order(self) -> List[List[str]]:
        """
        根据依赖关系计算脚本执行顺序

        Returns:
            List[List[str]]: 每个子列表包含可以并行执行的脚本路径
        """
        if not self._config:
            return []

        scripts = {script.path: script for script in self._config.scripts}
        resolved = set()
        order = []

        while len(resolved) < len(scripts):
            ready = []
            for path, script in scripts.items():
                if path not in resolved:
                    if all(dep in resolved for dep in script.depends_on):
                        ready.append(path)

            if not ready:
                remaining = set(scripts.keys()) - resolved
                raise ValueError(f"检测到循环依赖: {remaining}")

            order.append(ready)
            resolved.update(ready)

        return order