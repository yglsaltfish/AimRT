## AimRT Test Framework Guide (pytest_aimrt)

### Overview
- A Pytest-based test framework supporting YAML-driven test cases, process orchestration, resource monitoring, remote execution, and callback validations.
- Main entry: `pytest_aimrt/fixtures/aimrt_test.py` provides the `AimRTTestRunner` fixture.

### Requirements
- Python 3.10+
- `python3` available on both local and remote hosts
- Optional: install `psutil` on remote hosts to enable remote resource monitoring

### Directory and Key Modules
- `pytest_aimrt/core/`
  - `config_manager.py`: Parse YAML
  - `process_manager.py`: Process launching, remote execution, resource monitoring
  - `callback_manager.py`: Callback registration/execution
  - `base_test.py`: High-level test flow
  - `report_generator.py`: Report generation (HTML/JSON)
  - `pytest_results.py`: Aggregate native pytest results
- `pytest_aimrt/fixtures/aimrt_test.py`: Pytest fixtures (recommended)

### Quick Start
1) Write a test (example)
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
        return CallbackResult(False, "Missing process info")
    if "Bench completed." not in (p.stdout or ""):
        return CallbackResult(False, "Expected log not found")
    return CallbackResult(True, "Detected expected log output")

class TestRPCExamples:
    @pytest.mark.aimrt
    def test_mqtt_qos2_config(self, aimrt_test_runner: AimRTTestRunner):
        yaml_config_path = Path(__file__).parent / "test_mqtt_qos2.yaml"
        assert yaml_config_path.exists()
        assert aimrt_test_runner.setup_from_yaml(str(yaml_config_path))

        # Register callback (controlled by enabled_callbacks allowlist)
        aimrt_test_runner.register_function_callback(
            name="log_check",
            trigger=CallbackTrigger.PROCESS_END,
            func=my_log_check,
        )

        assert aimrt_test_runner.run_test()
```

2) Write YAML (local/remote supported)
```yaml
# Local example (no 'remote' field → run locally)
name: "MQTT Channel qos2"
config:
  time_sec: 60

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
# Remote example (define hosts and set each script's 'remote' to a host profile name)
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
      # Presence of this field (even empty) enables global allowlist mode
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

3) Run
```bash
# Run all cases under this directory
pytest pytest_aimrt/examples/channel_remote -v

# Filter by keyword
pytest -k mqtt -q
```

### Runtime Model and Notes
- **Local/Remote**
  - Scripts without a `remote` field are executed locally.
  - Scripts with `remote: <host_profile_name>` run on remote hosts via SSH (Fabric).
  - Remote working directory: the framework creates `/tmp/aimrt/<random>` with `stdout.log`, `stderr.log`, `env.log`, etc.
- **Resource Monitoring**
  - Local: sample with `psutil`.
  - Remote: enabled only if remote has `psutil`; samples are written to `monitor.jsonl` and aggregated into the report.
- **Runtime Control**
  - `time_sec` defines the maximum run time; timeout triggers termination.
  - On `shutdown_patterns` match, terminate gracefully and mark as completed.
- **Callbacks (validation/extraction)**
  - Register via `aimrt_test_runner.register_function_callback(name, trigger, func, **kwargs)`.
  - Supported triggers: `PROCESS_START`, `PROCESS_END`, `PERIODIC` (periodic callbacks require `interval_sec`, etc.).
  - `func(context) -> CallbackResult`, where `context` includes `process_info`, `script_path`, etc.
- **Callback Allowlist (`enabled_callbacks`)**
  - If any script in YAML contains `enabled_callbacks` (even empty), allowlist mode is ON.
  - In allowlist mode, only callbacks listed in a script's `enabled_callbacks` are registered and effective for that script.
  - `enabled_callbacks: []` on a script means no callbacks allowed for that script.
  - If the field never appears, all registered callbacks are allowed by default.
- **I/O and Capturing**
  - The framework collects `stdout/stderr` and includes them in process results for callbacks and reports.

### Reports
- Output location: `test_reports/`
  - HTML: `test_reports/html/<test_name>_<time>_report.html`
  - JSON: `test_reports/json/<test_name>_<time>_report.json`
- The report includes overall statistics, per-script status, resource usage (if available), callback results, and a summary of native pytest results.