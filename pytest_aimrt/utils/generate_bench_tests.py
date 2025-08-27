#!/usr/bin/env python3
# -*- coding: utf-8 -*-

"""
基准场景用例生成器

功能：
- 扫描仓库 examples 中的 benchmark 启动脚本，自动配对 pub/sub（或 client/server），
- 生成可直接运行的 YAML 测试文件与一个汇总的 pytest 测试文件。

默认扫描目录：src/examples（含 plugins、cpp、py 等子树）
默认输出目录：pytest_aimrt/generated

使用示例：
    python -m pytest_aimrt.utils.generate_bench_tests \
        --scan-root src/examples \
        --out-dir pytest_aimrt/generated \
        --cwd build \
        --time-sec 60

生成后运行：
    pytest -m "examples and bench" pytest_aimrt/generated -v
"""

from __future__ import annotations

import argparse
import re
import sys
from dataclasses import dataclass
from pathlib import Path
from typing import Dict, List, Tuple


SUB_KEYS = ["sub", "subscriber", "server"]
PUB_KEYS = ["pub", "publisher", "client"]


@dataclass
class BenchPair:
    key: str
    sub_path: Path
    pub_path: Path
    family: str  # e.g. mqtt/pb_chn/ros2_chn/pb_rpc/ros2_rpc


def normalize_key(name: str) -> str:
    """从脚本文件名中抽取可配对的 key（去掉 sub/pub 等语义词）。"""
    base = name
    base = re.sub(r"^start_", "", base)
    base = re.sub(r"\.sh$", "", base)
    tokens = re.split(r"[_\-]+", base)
    filtered = [t for t in tokens if t.lower() not in (SUB_KEYS + PUB_KEYS)]
    return "_".join(filtered)


def guess_family(path: Path) -> str:
    p = str(path).lower()
    if "mqtt" in p:
        return "mqtt"
    if "ros2_chn" in p or "ros2" in p and "chn" in p:
        return "ros2_chn"
    if "ros2_rpc" in p:
        return "ros2_rpc"
    if "pb_chn" in p and "benchmark" in p:
        return "pb_chn"
    if "pb_rpc" in p and "benchmark" in p:
        return "pb_rpc"
    if "zenoh" in p:
        return "zenoh"
    if "iceoryx" in p:
        return "iceoryx"
    if "grpc" in p:
        return "grpc"
    return "examples"


def find_benchmark_scripts(scan_root: Path) -> Tuple[List[Path], List[Path]]:
    """返回 (subs, pubs) 两组脚本路径。"""
    subs: List[Path] = []
    pubs: List[Path] = []
    for p in scan_root.rglob("start_*bench*.*sh"):
        name = p.name.lower()
        if any(k in name for k in SUB_KEYS):
            subs.append(p)
        if any(k in name for k in PUB_KEYS):
            pubs.append(p)
    # 兼容 examples/plugins 模式，比如: start_examples_plugins_mqtt_plugin_pb_chn_benchmark_*
    for p in scan_root.rglob("start_examples_*benchmark*.*sh"):
        name = p.name.lower()
        if any(k in name for k in SUB_KEYS) and p not in subs:
            subs.append(p)
        if any(k in name for k in PUB_KEYS) and p not in pubs:
            pubs.append(p)
    return subs, pubs


def pair_scripts(subs: List[Path], pubs: List[Path]) -> List[BenchPair]:
    sub_map: Dict[str, Path] = {}
    for sp in subs:
        sub_map.setdefault(normalize_key(sp.name.lower()), sp)

    pairs: List[BenchPair] = []
    for pp in pubs:
        key = normalize_key(pp.name.lower())
        if key in sub_map:
            subp = sub_map[key]
            fam = guess_family(pp)
            pairs.append(BenchPair(key=key, sub_path=subp, pub_path=pp, family=fam))
    return pairs


def yaml_for_pair(pair: BenchPair, cwd: str, time_sec: int) -> str:
    """生成单个 bench 的 YAML 文本。使用宽松的 shutdown_patterns。"""
    lower_name = pair.pub_path.name.lower()
    full_pub_path = str(pair.pub_path).lower()

    m = re.search(r"plugins_([a-z0-9]+)_plugin", lower_name)
    if not m:
        m = re.search(r"plugins/([a-z0-9]+)_plugin", full_pub_path)
    plugin_name = m.group(1) if m else (
        "ros2" if "ros2" in full_pub_path else (pair.family.split("_")[0] if pair.family else "examples")
    )

    is_rpc = ("rpc" in lower_name) or ("rpc" in pair.key) or ("rpc" in pair.family)
    type_str = "rpc" if is_rpc else "chn"

    variant = None
    low_key = pair.key.lower()
    if "ros2" in full_pub_path or "ros2" in pair.family or "ros2" in low_key:
        if ("besteffort" in low_key) or ("best_effort" in low_key) or ("besteffort" in lower_name):
            variant = "besteffort"
        elif ("reliable" in low_key) or ("reliable" in lower_name):
            variant = "reliable"
    if plugin_name == "mqtt" and variant is None:
        mq = re.search(r"qos([0-2])", low_key) or re.search(r"qos([0-2])", lower_name)
        if mq:
            variant = f"qos{mq.group(1)}"

    name_str = f"{type_str}-{plugin_name}"
    if variant:
        name_str = f"{name_str}-{variant}"

    # 追加 marks 到 name 中（去除通用与重复项）
    try:
        marks = compute_marks(pair)
        base_parts = {type_str, plugin_name}
        if variant:
            base_parts.add(variant)
        # 过滤掉通用标签与与基础组件重复的标签
        exclude = {"aimrt", "examples", "bench"}
        if type_str == "chn":
            exclude.add("channel")
        if type_str == "rpc":
            exclude.add("rpc")
        additional = [m for m in marks if m not in exclude and m not in base_parts]
        if additional:
            name_str = f"{name_str}-{'-'.join(additional)}"
    except Exception:
        pass

    return f"""
name: "{name_str}"
description: "Auto-generated benchmark test for {pair.key}"

config:
  execution_count: 1
  time_sec: {max(time_sec, 1)}
  cwd: "{cwd}"
  environment:
    LOG_LEVEL: "info"

input:
  scripts:
    - path: "./{pair.sub_path.name}"
      args: []
      depends_on: []
      delay_sec: 0
      cwd: ""
      time_sec: {time_sec}
      monitor:
        cpu: true
        memory: true
        disk: true
      environment:
        ROLE: "server"
      shutdown_patterns: ["Benchmark plan 0 completed"]

    - path: "./{pair.pub_path.name}"
      args: []
      depends_on: ["./{pair.sub_path.name}"]
      delay_sec: 3
      cwd: ""
      time_sec: {time_sec}
      monitor:
        cpu: true
        memory: true
        disk: true
      environment:
        ROLE: "client"
      shutdown_patterns: ["Bench completed."]
""".strip() + "\n"


def compute_marks(pair: BenchPair) -> List[str]:
    marks = ["aimrt", "examples", "bench"]
    fam = pair.family
    if fam:
        marks.append(fam)
    m = re.search(r"qos([0-2])", pair.key)
    if m:
        marks.append(f"qos{m.group(1)}")
    low_key = pair.key.lower()
    low_pub = pair.pub_path.name.lower()
    # cross/local 识别
    if re.search(r"x86[-_]?2[-_]?orin", low_key) or re.search(r"x86[-_]?2[-_]?orin", low_pub):
        marks.append("cross")
    if ("local" in low_key) or ("local" in low_pub):
        marks.append("localmark")
    # msg_size 提取
    msz = re.search(r"msg[-_]?size[-_]?(\d+)", low_key) or re.search(r"msg[-_]?size[-_]?(\d+)", low_pub)
    if msz:
        marks.append(f"msg_size_{msz.group(1)}")
    if "besteffort" in low_key or "best_effort" in low_key:
        marks.append("besteffort")
    if "reliable" in low_key:
        marks.append("reliable")
    if any(x in pair.key for x in ["chn", "channel"]):
        marks.append("channel")
    if "rpc" in pair.key:
        marks.append("rpc")
    return sorted(set(marks))


def write_test_hub(py_path: Path, yaml_files: List[Path], marks_map: Dict[str, List[str]]):
    """写入一个聚合测试文件，参数化加载所有生成的 YAML。"""
    rel_yaml = [yf.name for yf in yaml_files]
    # 将每个用例的 marks 以 param(marks=...) 形式注入
    lines = []
    lines.append("#!/usr/bin/env python3")
    lines.append("# -*- coding: utf-8 -*-")
    lines.append("")
    lines.append("import pytest")
    lines.append("from pathlib import Path")
    lines.append("from pytest_aimrt.fixtures.aimrt_test import AimRTTestRunner")
    lines.append("")
    lines.append("CASES = [")
    for name in rel_yaml:
        marks = marks_map.get(name, ["aimrt", "examples", "bench"])
        mark_expr = ", ".join([f"pytest.mark.{m}" for m in marks])
        lines.append(f"    pytest.param('{name}', marks=[{mark_expr}]),")
    lines.append("]")
    lines.append("")
    lines.append("@pytest.mark.parametrize('yaml_name', CASES)")
    lines.append("def test_generated_benchmarks(yaml_name: str, aimrt_test_runner: AimRTTestRunner):")
    lines.append("    yaml_path = Path(__file__).parent / yaml_name")
    lines.append("    if not yaml_path.exists():")
    lines.append("        pytest.skip(f'YAML not found: {yaml_path}')")
    lines.append("    if not aimrt_test_runner.setup_from_yaml(str(yaml_path)):")
    lines.append("        pytest.fail('Failed to setup test environment from YAML configuration')")
    lines.append("    success = aimrt_test_runner.run_test()")
    lines.append("    if not success:")
    lines.append("        pytest.fail('benchmark test execution failed')")
    py_path.write_text("\n".join(lines), encoding="utf-8")


def main(argv: List[str] | None = None) -> int:
    ap = argparse.ArgumentParser()
    ap.add_argument("--scan-root", type=str, default="src/examples", help="扫描 examples 根目录")
    ap.add_argument("--out-dir", type=str, default="pytest_aimrt/generated", help="输出目录")
    ap.add_argument("--cwd", type=str, default="build", help="YAML 中 config.cwd")
    ap.add_argument("--time-sec", type=int, default=60, help="每个脚本运行时间")
    args = ap.parse_args(argv)

    scan_root = Path(args.scan_root).resolve()
    out_dir = Path(args.out_dir).resolve()
    out_dir.mkdir(parents=True, exist_ok=True)

    subs, pubs = find_benchmark_scripts(scan_root)
    pairs = pair_scripts(subs, pubs)

    if not pairs:
        print("⚠️ 未发现 benchmark 脚本配对，检查扫描路径或命名约定。")
        return 1

    yaml_paths: List[Path] = []
    marks_map: Dict[str, List[str]] = {}
    for pair in pairs:
        lower_name = pair.pub_path.name.lower()
        is_rpc = ("rpc" in lower_name) or ("rpc" in pair.key) or ("rpc" in pair.family)
        # 暂时跳过所有 RPC 用例的生成
        if is_rpc:
            continue

        yaml_txt = yaml_for_pair(pair, cwd=args.cwd, time_sec=args.time_sec)
        yaml_name = f"bench_{pair.key}.yaml"
        yaml_path = out_dir / yaml_name
        yaml_path.write_text(yaml_txt, encoding="utf-8")
        yaml_paths.append(yaml_path)
        marks_map[yaml_name] = compute_marks(pair)

    test_py = out_dir / "test_benchmarks.py"
    write_test_hub(test_py, yaml_paths, marks_map)

    print(f"✅ 生成 {len(yaml_paths)} 个 YAML 与 1 个 pytest 测试: {test_py}")
    return 0


if __name__ == "__main__":
    sys.exit(main())


