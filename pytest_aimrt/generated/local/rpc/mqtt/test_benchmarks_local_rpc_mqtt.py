#!/usr/bin/env python3
# -*- coding: utf-8 -*-

import pytest
from pathlib import Path
from pytest_aimrt.fixtures.aimrt_test import AimRTTestRunner

CASES = [
    pytest.param('bench_examples_plugins_mqtt_plugin_pb_rpc_benchmark_qos0_local_bench_msg_size_16384_qos0.yaml', marks=[pytest.mark.aimrt, pytest.mark.bench, pytest.mark.examples, pytest.mark.localmark, pytest.mark.mqtt, pytest.mark.msg_size_16384, pytest.mark.qos0, pytest.mark.rpc]),
    pytest.param('bench_examples_plugins_mqtt_plugin_pb_rpc_benchmark_qos0_local_fixed_freq_msg_size_1024_qos0.yaml', marks=[pytest.mark.aimrt, pytest.mark.bench, pytest.mark.examples, pytest.mark.localmark, pytest.mark.mqtt, pytest.mark.msg_size_1024, pytest.mark.qos0, pytest.mark.rpc]),
    pytest.param('bench_examples_plugins_mqtt_plugin_pb_rpc_benchmark_qos0_local_fixed_freq_msg_size_4096_qos0.yaml', marks=[pytest.mark.aimrt, pytest.mark.bench, pytest.mark.examples, pytest.mark.localmark, pytest.mark.mqtt, pytest.mark.msg_size_4096, pytest.mark.qos0, pytest.mark.rpc]),
    pytest.param('bench_examples_plugins_mqtt_plugin_pb_rpc_benchmark_qos0_local_bench_msg_size_512_qos0.yaml', marks=[pytest.mark.aimrt, pytest.mark.bench, pytest.mark.examples, pytest.mark.localmark, pytest.mark.mqtt, pytest.mark.msg_size_512, pytest.mark.qos0, pytest.mark.rpc]),
    pytest.param('bench_examples_plugins_mqtt_plugin_pb_rpc_benchmark_qos0_local_bench_msg_size_2048_qos0.yaml', marks=[pytest.mark.aimrt, pytest.mark.bench, pytest.mark.examples, pytest.mark.localmark, pytest.mark.mqtt, pytest.mark.msg_size_2048, pytest.mark.qos0, pytest.mark.rpc]),
    pytest.param('bench_examples_plugins_mqtt_plugin_pb_rpc_benchmark_qos0_local_fixed_freq_msg_size_32768_qos0.yaml', marks=[pytest.mark.aimrt, pytest.mark.bench, pytest.mark.examples, pytest.mark.localmark, pytest.mark.mqtt, pytest.mark.msg_size_32768, pytest.mark.qos0, pytest.mark.rpc]),
    pytest.param('bench_examples_plugins_mqtt_plugin_pb_rpc_benchmark_qos0_local_fixed_freq_msg_size_512_qos0.yaml', marks=[pytest.mark.aimrt, pytest.mark.bench, pytest.mark.examples, pytest.mark.localmark, pytest.mark.mqtt, pytest.mark.msg_size_512, pytest.mark.qos0, pytest.mark.rpc]),
    pytest.param('bench_examples_plugins_mqtt_plugin_pb_rpc_benchmark_qos0_local_bench_msg_size_32768_qos0.yaml', marks=[pytest.mark.aimrt, pytest.mark.bench, pytest.mark.examples, pytest.mark.localmark, pytest.mark.mqtt, pytest.mark.msg_size_32768, pytest.mark.qos0, pytest.mark.rpc]),
    pytest.param('bench_examples_plugins_mqtt_plugin_pb_rpc_benchmark_qos0_local_bench_msg_size_65536_qos0.yaml', marks=[pytest.mark.aimrt, pytest.mark.bench, pytest.mark.examples, pytest.mark.localmark, pytest.mark.mqtt, pytest.mark.msg_size_65536, pytest.mark.qos0, pytest.mark.rpc]),
    pytest.param('bench_examples_plugins_mqtt_plugin_pb_rpc_benchmark_qos0_local_bench_parallel_number_none_qos0.yaml', marks=[pytest.mark.aimrt, pytest.mark.bench, pytest.mark.examples, pytest.mark.localmark, pytest.mark.mqtt, pytest.mark.qos0, pytest.mark.rpc]),
    pytest.param('bench_examples_plugins_mqtt_plugin_pb_rpc_benchmark_qos0_local_bench_msg_size_4096_qos0.yaml', marks=[pytest.mark.aimrt, pytest.mark.bench, pytest.mark.examples, pytest.mark.localmark, pytest.mark.mqtt, pytest.mark.msg_size_4096, pytest.mark.qos0, pytest.mark.rpc]),
    pytest.param('bench_examples_plugins_mqtt_plugin_pb_rpc_benchmark_qos0_local_fixed_freq_msg_size_16384_qos0.yaml', marks=[pytest.mark.aimrt, pytest.mark.bench, pytest.mark.examples, pytest.mark.localmark, pytest.mark.mqtt, pytest.mark.msg_size_16384, pytest.mark.qos0, pytest.mark.rpc]),
    pytest.param('bench_examples_plugins_mqtt_plugin_pb_rpc_benchmark_qos0_local_bench_msg_size_8192_qos0.yaml', marks=[pytest.mark.aimrt, pytest.mark.bench, pytest.mark.examples, pytest.mark.localmark, pytest.mark.mqtt, pytest.mark.msg_size_8192, pytest.mark.qos0, pytest.mark.rpc]),
    pytest.param('bench_examples_plugins_mqtt_plugin_pb_rpc_benchmark_qos0_local_fixed_freq_msg_size_2048_qos0.yaml', marks=[pytest.mark.aimrt, pytest.mark.bench, pytest.mark.examples, pytest.mark.localmark, pytest.mark.mqtt, pytest.mark.msg_size_2048, pytest.mark.qos0, pytest.mark.rpc]),
    pytest.param('bench_examples_plugins_mqtt_plugin_pb_rpc_benchmark_qos0_local_fixed_freq_msg_size_8192_qos0.yaml', marks=[pytest.mark.aimrt, pytest.mark.bench, pytest.mark.examples, pytest.mark.localmark, pytest.mark.mqtt, pytest.mark.msg_size_8192, pytest.mark.qos0, pytest.mark.rpc]),
    pytest.param('bench_examples_plugins_mqtt_plugin_pb_rpc_benchmark_qos0_local_fixed_freq_msg_size_65536_qos0.yaml', marks=[pytest.mark.aimrt, pytest.mark.bench, pytest.mark.examples, pytest.mark.localmark, pytest.mark.mqtt, pytest.mark.msg_size_65536, pytest.mark.qos0, pytest.mark.rpc]),
    pytest.param('bench_examples_plugins_mqtt_plugin_pb_rpc_benchmark_qos0_local_fixed_freq_parallel_number_none_qos0.yaml', marks=[pytest.mark.aimrt, pytest.mark.bench, pytest.mark.examples, pytest.mark.localmark, pytest.mark.mqtt, pytest.mark.qos0, pytest.mark.rpc]),
    pytest.param('bench_examples_plugins_mqtt_plugin_pb_rpc_benchmark_qos0_local_bench_msg_size_1024_qos0.yaml', marks=[pytest.mark.aimrt, pytest.mark.bench, pytest.mark.examples, pytest.mark.localmark, pytest.mark.mqtt, pytest.mark.msg_size_1024, pytest.mark.qos0, pytest.mark.rpc]),
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