## AimRT 测试框架使用指南（pytest_aimrt）

### 概述
- 基于 Pytest 的测试框架，支持 YAML 配置驱动用例、进程编排、资源监控、远程执行与回调校验。
- 主要入口：`pytest_aimrt/fixtures/aimrt_test.py` 提供 `AimRTTestRunner` fixture。

### 环境要求
- Python 3.10+
- 本机和远端均需可用 `python3`
- 可选：远端安装 `psutil` 才能启用远程资源监控

### 目录与主要模块
- `pytest_aimrt/core/`
  - `config_manager.py`：解析 YAML
  - `process_manager.py`：进程启动、远程执行、资源监控
  - `callback_manager.py`：回调注册/执行
  - `base_test.py`：高层测试流程
  - `report_generator.py`：报告生成（HTML/JSON）
  - `pytest_results.py`：聚合 pytest 原生结果
- `pytest_aimrt/fixtures/aimrt_test.py`：Pytest fixtures（推荐使用）

### 快速开始
1) 写测试（示例）
```python
# pytest_aimrt/examples/channel_remote/test_mqtt.py
import pytest
from pathlib import Path
from typing import Dict, Any
from pytest_aimrt.fixtures.aimrt_test import AimRTTestRunner
from pytest_aimrt.core.callback_manager import CallbackTrigger, CallbackResult

def my_log_check(context: Dict[str, Any]) -> CallbackResult:
    p = context.get('process_info')
    if not p:
        return CallbackResult(False, "缺少进程信息")
    if "Bench completed." not in (p.stdout or ""):
        return CallbackResult(False, "未找到预期日志")
    return CallbackResult(True, "检测到有正确的日志输出")

class TestRPCExamples:
    @pytest.mark.aimrt
    def test_mqtt_qos2_config(self, aimrt_test_runner: AimRTTestRunner):
        yaml_config_path = Path(__file__).parent / "test_mqtt_qos2.yaml"
        assert yaml_config_path.exists()
        assert aimrt_test_runner.setup_from_yaml(str(yaml_config_path))

        # 注册回调（受 enabled_callbacks 白名单控制）
        aimrt_test_runner.register_function_callback(
            name="log_check",
            trigger=CallbackTrigger.PROCESS_END,
            func=my_log_check,
        )

        assert aimrt_test_runner.run_test()
```

2) 写 YAML（本地/远程均可）
```yaml
# 本地示例（无 remote 字段 → 本地执行）
name: "MQTT Channel qos2"
config:
  time_sec: 60
  cwd: "build"
input:
  scripts:
    - path: "./start_sub.sh"
      time_sec: 30
      monitor: { cpu: true, memory: true, disk: true }
      shutdown_patterns: ["ready"]
    - path: "./start_pub.sh"
      depends_on: ["./start_sub.sh"]
      delay_sec: 2
      time_sec: 30
      shutdown_patterns: ["Bench completed."]
```

```yaml
# 远程示例（有 hosts + 每个脚本的 remote = 主机档案名）
name: "MQTT Channel qos2 remote"
config:
  time_sec: 90
hosts:
  x86:
    host: "192.168.111.241"
    ssh_user: "agi"
    ssh_password: "1"
    ssh_port: 61513
    remote_cwd: "/agibot/data/ygl/build"
  orin:
    host: "192.168.111.241"
    ssh_user: "agi"
    ssh_password: "1"
    ssh_port: 61059
    remote_cwd: "/agibot/data/ygl/build"
input:
  scripts:
    - remote: orin
      path: "./start_sub.sh"
      time_sec: 60
      monitor: { cpu: true, memory: true, disk: true }
      shutdown_patterns: ["Benchmark plan 2 completed"]
      # 若出现该字段（可为空），即开启全局白名单模式
      # enabled_callbacks: []
    - remote: x86
      path: "./start_pub.sh"
      depends_on: ["./start_sub.sh"]
      delay_sec: 3
      time_sec: 60
      shutdown_patterns: ["Bench completed."]
      # enabled_callbacks:
      #   - log_check
```

3) 运行
```bash
# 运行该目录下所有用例
pytest pytest_aimrt/examples/channel_remote -v

# 按关键字过滤
pytest -k mqtt -q
```

### 运行机制与要点
- **本地/远程**
  - 未配置 `remote` 字段的脚本，一律作为本地执行。
  - 配置了 `remote: <host_profile_name>` 的脚本，将通过 SSH（Fabric）在远端执行。
  - 远端目录结构：框架会在 `/tmp/aimrt/<随机>` 下生成 `stdout.log`、`stderr.log`、`env.log` 等文件。
- **资源监控**
  - 本地：直接使用 `psutil` 采样。
  - 远程：仅当远端安装 `psutil` 时启用，采样写入 `monitor.jsonl` 并汇总到报告。
- **运行时控制**
  - `time_sec` 控制脚本最长运行时间，超时将发起终止流程。
  - `shutdown_patterns` 命中后优雅终止并视为 completed。
- **回调（校验/提取信息）**
  - 使用 `aimrt_test_runner.register_function_callback(name, trigger, func, **kwargs)` 注册。
  - 支持触发器：`PROCESS_START`、`PROCESS_END`、`PERIODIC`（周期回调需传 `interval_sec` 等参数）。
  - `func(context) -> CallbackResult`，其中 `context` 会包含 `process_info`、`script_path` 等。
- **回调白名单（enabled_callbacks）**
  - 只要 YAML 中任意脚本出现了 `enabled_callbacks` 字段（即使为空），就开启“白名单模式”。
  - 白名单开启后，仅出现在某脚本 `enabled_callbacks` 中的回调名才会被注册，并且只对列出的脚本生效。
  - 如果某脚本写 `enabled_callbacks: []`，表示该脚本不允许任何回调。
  - 若完全不出现该字段，默认放行所有已注册回调。
- **输入输出与捕获**
  - 框架会收集 `stdout/stderr` 并合并到进程结果，用于回调和报告。

### 报告
- 生成位置：`test_reports/`
  - HTML：`test_reports/html/<测试名>_<时间>_report.html`
  - JSON：`test_reports/json/<测试名>_<时间>_report.json`
- 报告内容包括整体统计、各脚本状态、资源使用（如可用）、回调结果和 pytest 原生结果摘要。