#!/usr/bin/env python3
# -*- coding: utf-8 -*-

import pytest
from pathlib import Path
from pytest_aimrt.fixtures.aimrt_test import AimRTTestRunner

CASES = [
    pytest.param('mqtt/bench_examples_plugins_mqtt_plugin_pb_rpc_benchmark_qos0_x86_2_orin_fixed_freq_msg_size_1024_qos0.yaml', marks=[pytest.mark.aimrt, pytest.mark.examples, pytest.mark.bench]),
    pytest.param('ros2/bench_examples_plugins_ros2_plugin_ros2_rpc_benchmark_besteffort_x86_2_orin_bench_msg_size_2048_besteffort.yaml', marks=[pytest.mark.aimrt, pytest.mark.examples, pytest.mark.bench]),
    pytest.param('mqtt/bench_examples_plugins_mqtt_plugin_pb_rpc_benchmark_qos0_x86_2_orin_fixed_freq_msg_size_16384_qos0.yaml', marks=[pytest.mark.aimrt, pytest.mark.examples, pytest.mark.bench]),
    pytest.param('ros2/bench_examples_plugins_ros2_plugin_ros2_rpc_benchmark_reliable_x86_2_orin_bench_msg_size_65536_reliable.yaml', marks=[pytest.mark.aimrt, pytest.mark.examples, pytest.mark.bench]),
    pytest.param('ros2/bench_examples_plugins_ros2_plugin_ros2_rpc_benchmark_reliable_x86_2_orin_fixed_freq_msg_size_4096_reliable.yaml', marks=[pytest.mark.aimrt, pytest.mark.examples, pytest.mark.bench]),
    pytest.param('ros2/bench_examples_plugins_ros2_plugin_ros2_rpc_benchmark_besteffort_x86_2_orin_fixed_freq_parallel_number_none_besteffort.yaml', marks=[pytest.mark.aimrt, pytest.mark.examples, pytest.mark.bench]),
    pytest.param('mqtt/bench_examples_plugins_mqtt_plugin_pb_rpc_benchmark_qos0_x86_2_orin_bench_msg_size_32768_qos0.yaml', marks=[pytest.mark.aimrt, pytest.mark.examples, pytest.mark.bench]),
    pytest.param('ros2/bench_examples_plugins_ros2_plugin_ros2_rpc_benchmark_reliable_x86_2_orin_bench_msg_size_4096_reliable.yaml', marks=[pytest.mark.aimrt, pytest.mark.examples, pytest.mark.bench]),
    pytest.param('ros2/bench_examples_plugins_ros2_plugin_ros2_rpc_benchmark_reliable_x86_2_orin_bench_parallel_number_none_reliable.yaml', marks=[pytest.mark.aimrt, pytest.mark.examples, pytest.mark.bench]),
    pytest.param('mqtt/bench_examples_plugins_mqtt_plugin_pb_rpc_benchmark_qos0_x86_2_orin_bench_msg_size_4096_qos0.yaml', marks=[pytest.mark.aimrt, pytest.mark.examples, pytest.mark.bench]),
    pytest.param('mqtt/bench_examples_plugins_mqtt_plugin_pb_rpc_benchmark_qos0_x86_2_orin_fixed_freq_msg_size_65536_qos0.yaml', marks=[pytest.mark.aimrt, pytest.mark.examples, pytest.mark.bench]),
    pytest.param('mqtt/bench_examples_plugins_mqtt_plugin_pb_rpc_benchmark_qos0_x86_2_orin_fixed_freq_msg_size_8192_qos0.yaml', marks=[pytest.mark.aimrt, pytest.mark.examples, pytest.mark.bench]),
    pytest.param('ros2/bench_examples_plugins_ros2_plugin_ros2_rpc_benchmark_besteffort_x86_2_orin_fixed_freq_msg_size_65536_besteffort.yaml', marks=[pytest.mark.aimrt, pytest.mark.examples, pytest.mark.bench]),
    pytest.param('ros2/bench_examples_plugins_ros2_plugin_ros2_rpc_benchmark_reliable_x86_2_orin_fixed_freq_msg_size_16384_reliable.yaml', marks=[pytest.mark.aimrt, pytest.mark.examples, pytest.mark.bench]),
    pytest.param('mqtt/bench_examples_plugins_mqtt_plugin_pb_rpc_benchmark_qos0_x86_2_orin_bench_msg_size_65536_qos0.yaml', marks=[pytest.mark.aimrt, pytest.mark.examples, pytest.mark.bench]),
    pytest.param('ros2/bench_examples_plugins_ros2_plugin_ros2_rpc_benchmark_besteffort_x86_2_orin_bench_parallel_number_none_besteffort.yaml', marks=[pytest.mark.aimrt, pytest.mark.examples, pytest.mark.bench]),
    pytest.param('mqtt/bench_examples_plugins_mqtt_plugin_pb_rpc_benchmark_qos0_x86_2_orin_bench_msg_size_8192_qos0.yaml', marks=[pytest.mark.aimrt, pytest.mark.examples, pytest.mark.bench]),
    pytest.param('mqtt/bench_examples_plugins_mqtt_plugin_pb_rpc_benchmark_qos0_x86_2_orin_fixed_freq_msg_size_32768_qos0.yaml', marks=[pytest.mark.aimrt, pytest.mark.examples, pytest.mark.bench]),
    pytest.param('ros2/bench_examples_plugins_ros2_plugin_ros2_rpc_benchmark_besteffort_x86_2_orin_bench_msg_size_8192_besteffort.yaml', marks=[pytest.mark.aimrt, pytest.mark.examples, pytest.mark.bench]),
    pytest.param('ros2/bench_examples_plugins_ros2_plugin_ros2_rpc_benchmark_reliable_x86_2_orin_fixed_freq_parallel_number_none_reliable.yaml', marks=[pytest.mark.aimrt, pytest.mark.examples, pytest.mark.bench]),
    pytest.param('ros2/bench_examples_plugins_ros2_plugin_ros2_rpc_benchmark_reliable_x86_2_orin_fixed_freq_msg_size_32768_reliable.yaml', marks=[pytest.mark.aimrt, pytest.mark.examples, pytest.mark.bench]),
    pytest.param('ros2/bench_examples_plugins_ros2_plugin_ros2_rpc_benchmark_besteffort_x86_2_orin_bench_msg_size_512_besteffort.yaml', marks=[pytest.mark.aimrt, pytest.mark.examples, pytest.mark.bench]),
    pytest.param('ros2/bench_examples_plugins_ros2_plugin_ros2_rpc_benchmark_besteffort_x86_2_orin_fixed_freq_msg_size_1024_besteffort.yaml', marks=[pytest.mark.aimrt, pytest.mark.examples, pytest.mark.bench]),
    pytest.param('ros2/bench_examples_plugins_ros2_plugin_ros2_rpc_benchmark_besteffort_x86_2_orin_bench_msg_size_4096_besteffort.yaml', marks=[pytest.mark.aimrt, pytest.mark.examples, pytest.mark.bench]),
    pytest.param('ros2/bench_examples_plugins_ros2_plugin_ros2_rpc_benchmark_reliable_x86_2_orin_fixed_freq_msg_size_1024_reliable.yaml', marks=[pytest.mark.aimrt, pytest.mark.examples, pytest.mark.bench]),
    pytest.param('mqtt/bench_examples_plugins_mqtt_plugin_pb_rpc_benchmark_qos0_x86_2_orin_fixed_freq_msg_size_2048_qos0.yaml', marks=[pytest.mark.aimrt, pytest.mark.examples, pytest.mark.bench]),
    pytest.param('ros2/bench_examples_plugins_ros2_plugin_ros2_rpc_benchmark_besteffort_x86_2_orin_fixed_freq_msg_size_16384_besteffort.yaml', marks=[pytest.mark.aimrt, pytest.mark.examples, pytest.mark.bench]),
    pytest.param('ros2/bench_examples_plugins_ros2_plugin_ros2_rpc_benchmark_besteffort_x86_2_orin_fixed_freq_msg_size_512_besteffort.yaml', marks=[pytest.mark.aimrt, pytest.mark.examples, pytest.mark.bench]),
    pytest.param('mqtt/bench_examples_plugins_mqtt_plugin_pb_rpc_benchmark_qos0_x86_2_orin_bench_msg_size_1024_qos0.yaml', marks=[pytest.mark.aimrt, pytest.mark.examples, pytest.mark.bench]),
    pytest.param('ros2/bench_examples_plugins_ros2_plugin_ros2_rpc_benchmark_reliable_x86_2_orin_bench_msg_size_8192_reliable.yaml', marks=[pytest.mark.aimrt, pytest.mark.examples, pytest.mark.bench]),
    pytest.param('ros2/bench_examples_plugins_ros2_plugin_ros2_rpc_benchmark_besteffort_x86_2_orin_bench_msg_size_1024_besteffort.yaml', marks=[pytest.mark.aimrt, pytest.mark.examples, pytest.mark.bench]),
    pytest.param('ros2/bench_examples_plugins_ros2_plugin_ros2_rpc_benchmark_besteffort_x86_2_orin_bench_msg_size_16384_besteffort.yaml', marks=[pytest.mark.aimrt, pytest.mark.examples, pytest.mark.bench]),
    pytest.param('ros2/bench_examples_plugins_ros2_plugin_ros2_rpc_benchmark_reliable_x86_2_orin_fixed_freq_msg_size_8192_reliable.yaml', marks=[pytest.mark.aimrt, pytest.mark.examples, pytest.mark.bench]),
    pytest.param('mqtt/bench_examples_plugins_mqtt_plugin_pb_rpc_benchmark_qos0_x86_2_orin_fixed_freq_msg_size_512_qos0.yaml', marks=[pytest.mark.aimrt, pytest.mark.examples, pytest.mark.bench]),
    pytest.param('mqtt/bench_examples_plugins_mqtt_plugin_pb_rpc_benchmark_qos0_x86_2_orin_bench_msg_size_16384_qos0.yaml', marks=[pytest.mark.aimrt, pytest.mark.examples, pytest.mark.bench]),
    pytest.param('ros2/bench_examples_plugins_ros2_plugin_ros2_rpc_benchmark_besteffort_x86_2_orin_fixed_freq_msg_size_8192_besteffort.yaml', marks=[pytest.mark.aimrt, pytest.mark.examples, pytest.mark.bench]),
    pytest.param('ros2/bench_examples_plugins_ros2_plugin_ros2_rpc_benchmark_reliable_x86_2_orin_bench_msg_size_32768_reliable.yaml', marks=[pytest.mark.aimrt, pytest.mark.examples, pytest.mark.bench]),
    pytest.param('mqtt/bench_examples_plugins_mqtt_plugin_pb_rpc_benchmark_qos0_x86_2_orin_bench_msg_size_2048_qos0.yaml', marks=[pytest.mark.aimrt, pytest.mark.examples, pytest.mark.bench]),
    pytest.param('ros2/bench_examples_plugins_ros2_plugin_ros2_rpc_benchmark_besteffort_x86_2_orin_fixed_freq_msg_size_4096_besteffort.yaml', marks=[pytest.mark.aimrt, pytest.mark.examples, pytest.mark.bench]),
    pytest.param('ros2/bench_examples_plugins_ros2_plugin_ros2_rpc_benchmark_besteffort_x86_2_orin_fixed_freq_msg_size_32768_besteffort.yaml', marks=[pytest.mark.aimrt, pytest.mark.examples, pytest.mark.bench]),
    pytest.param('ros2/bench_examples_plugins_ros2_plugin_ros2_rpc_benchmark_besteffort_x86_2_orin_bench_msg_size_32768_besteffort.yaml', marks=[pytest.mark.aimrt, pytest.mark.examples, pytest.mark.bench]),
    pytest.param('ros2/bench_examples_plugins_ros2_plugin_ros2_rpc_benchmark_reliable_x86_2_orin_fixed_freq_msg_size_512_reliable.yaml', marks=[pytest.mark.aimrt, pytest.mark.examples, pytest.mark.bench]),
    pytest.param('ros2/bench_examples_plugins_ros2_plugin_ros2_rpc_benchmark_reliable_x86_2_orin_fixed_freq_msg_size_2048_reliable.yaml', marks=[pytest.mark.aimrt, pytest.mark.examples, pytest.mark.bench]),
    pytest.param('ros2/bench_examples_plugins_ros2_plugin_ros2_rpc_benchmark_reliable_x86_2_orin_bench_msg_size_2048_reliable.yaml', marks=[pytest.mark.aimrt, pytest.mark.examples, pytest.mark.bench]),
    pytest.param('ros2/bench_examples_plugins_ros2_plugin_ros2_rpc_benchmark_reliable_x86_2_orin_bench_msg_size_512_reliable.yaml', marks=[pytest.mark.aimrt, pytest.mark.examples, pytest.mark.bench]),
    pytest.param('mqtt/bench_examples_plugins_mqtt_plugin_pb_rpc_benchmark_qos0_x86_2_orin_fixed_freq_parallel_number_none_qos0.yaml', marks=[pytest.mark.aimrt, pytest.mark.examples, pytest.mark.bench]),
    pytest.param('ros2/bench_examples_plugins_ros2_plugin_ros2_rpc_benchmark_reliable_x86_2_orin_fixed_freq_msg_size_65536_reliable.yaml', marks=[pytest.mark.aimrt, pytest.mark.examples, pytest.mark.bench]),
    pytest.param('mqtt/bench_examples_plugins_mqtt_plugin_pb_rpc_benchmark_qos0_x86_2_orin_fixed_freq_msg_size_4096_qos0.yaml', marks=[pytest.mark.aimrt, pytest.mark.examples, pytest.mark.bench]),
    pytest.param('mqtt/bench_examples_plugins_mqtt_plugin_pb_rpc_benchmark_qos0_x86_2_orin_bench_parallel_number_none_qos0.yaml', marks=[pytest.mark.aimrt, pytest.mark.examples, pytest.mark.bench]),
    pytest.param('ros2/bench_examples_plugins_ros2_plugin_ros2_rpc_benchmark_reliable_x86_2_orin_bench_msg_size_1024_reliable.yaml', marks=[pytest.mark.aimrt, pytest.mark.examples, pytest.mark.bench]),
    pytest.param('mqtt/bench_examples_plugins_mqtt_plugin_pb_rpc_benchmark_qos0_x86_2_orin_bench_msg_size_512_qos0.yaml', marks=[pytest.mark.aimrt, pytest.mark.examples, pytest.mark.bench]),
    pytest.param('ros2/bench_examples_plugins_ros2_plugin_ros2_rpc_benchmark_reliable_x86_2_orin_bench_msg_size_16384_reliable.yaml', marks=[pytest.mark.aimrt, pytest.mark.examples, pytest.mark.bench]),
    pytest.param('ros2/bench_examples_plugins_ros2_plugin_ros2_rpc_benchmark_besteffort_x86_2_orin_fixed_freq_msg_size_2048_besteffort.yaml', marks=[pytest.mark.aimrt, pytest.mark.examples, pytest.mark.bench]),
    pytest.param('ros2/bench_examples_plugins_ros2_plugin_ros2_rpc_benchmark_besteffort_x86_2_orin_bench_msg_size_65536_besteffort.yaml', marks=[pytest.mark.aimrt, pytest.mark.examples, pytest.mark.bench]),
]

@pytest.mark.parametrize('yaml_name', CASES)
def test_generated_benchmarks(yaml_name: str, aimrt_test_runner: AimRTTestRunner):
    yaml_path = (Path(__file__).parent / yaml_name).resolve()
    if not yaml_path.exists():
        pytest.skip(f'YAML not found: {yaml_path}')
    if not aimrt_test_runner.setup_from_yaml(str(yaml_path)):
        pytest.fail('Failed to setup test environment from YAML configuration')
    success = aimrt_test_runner.run_test()
    if not success:
        pytest.fail('benchmark test execution failed')