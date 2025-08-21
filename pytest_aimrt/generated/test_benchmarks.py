#!/usr/bin/env python3
# -*- coding: utf-8 -*-

import pytest
from pathlib import Path
from pytest_aimrt.fixtures.aimrt_test import AimRTTestRunner

CASES = [
    pytest.param('bench_benchmark.yaml', marks=[pytest.mark.aimrt, pytest.mark.bench, pytest.mark.examples, pytest.mark.pb_chn]),
    pytest.param('bench_examples_plugins_iceoryx_plugin_pb_chn_benchmark.yaml', marks=[pytest.mark.aimrt, pytest.mark.bench, pytest.mark.channel, pytest.mark.examples, pytest.mark.pb_chn]),
    pytest.param('bench_examples_plugins_zenoh_plugin_pb_chn_benchmark_with_shm.yaml', marks=[pytest.mark.aimrt, pytest.mark.bench, pytest.mark.channel, pytest.mark.examples, pytest.mark.pb_chn]),
    pytest.param('bench_examples_plugins_zenoh_plugin_pb_chn_benchmark.yaml', marks=[pytest.mark.aimrt, pytest.mark.bench, pytest.mark.channel, pytest.mark.examples, pytest.mark.pb_chn]),
    pytest.param('bench_examples_plugins_ros2_plugin_pb_chn_benchmark_besteffort.yaml', marks=[pytest.mark.aimrt, pytest.mark.bench, pytest.mark.channel, pytest.mark.examples, pytest.mark.ros2_chn]),
    pytest.param('bench_examples_plugins_ros2_plugin_ros2_chn_benchmark_reliable.yaml', marks=[pytest.mark.aimrt, pytest.mark.bench, pytest.mark.channel, pytest.mark.examples, pytest.mark.ros2_chn]),
    pytest.param('bench_examples_plugins_ros2_plugin_pb_rpc_benchmark_besteffort.yaml', marks=[pytest.mark.aimrt, pytest.mark.bench, pytest.mark.examples, pytest.mark.pb_rpc, pytest.mark.rpc]),
    pytest.param('bench_examples_plugins_ros2_plugin_ros2_chn_benchmark_besteffort.yaml', marks=[pytest.mark.aimrt, pytest.mark.bench, pytest.mark.channel, pytest.mark.examples, pytest.mark.ros2_chn]),
    pytest.param('bench_examples_plugins_ros2_plugin_pb_rpc_benchmark_reliable.yaml', marks=[pytest.mark.aimrt, pytest.mark.bench, pytest.mark.examples, pytest.mark.pb_rpc, pytest.mark.rpc]),
    pytest.param('bench_examples_plugins_ros2_plugin_pb_chn_benchmark_reliable.yaml', marks=[pytest.mark.aimrt, pytest.mark.bench, pytest.mark.channel, pytest.mark.examples, pytest.mark.ros2_chn]),
    pytest.param('bench_examples_plugins_ros2_plugin_ros2_rpc_benchmark_besteffort.yaml', marks=[pytest.mark.aimrt, pytest.mark.bench, pytest.mark.examples, pytest.mark.ros2_rpc, pytest.mark.rpc]),
    pytest.param('bench_examples_plugins_ros2_plugin_ros2_rpc_benchmark_reliable.yaml', marks=[pytest.mark.aimrt, pytest.mark.bench, pytest.mark.examples, pytest.mark.ros2_rpc, pytest.mark.rpc]),
    pytest.param('bench_examples_plugins_net_plugin_pb_chn_udp_benchmark.yaml', marks=[pytest.mark.aimrt, pytest.mark.bench, pytest.mark.channel, pytest.mark.examples, pytest.mark.pb_chn]),
    pytest.param('bench_examples_plugins_net_plugin_pb_chn_tcp_benchmark.yaml', marks=[pytest.mark.aimrt, pytest.mark.bench, pytest.mark.channel, pytest.mark.examples, pytest.mark.pb_chn]),
    pytest.param('bench_examples_plugins_net_plugin_pb_chn_http_benchmark.yaml', marks=[pytest.mark.aimrt, pytest.mark.bench, pytest.mark.channel, pytest.mark.examples, pytest.mark.pb_chn]),
    pytest.param('bench_examples_plugins_mqtt_plugin_pb_chn_benchmark_qos1.yaml', marks=[pytest.mark.aimrt, pytest.mark.bench, pytest.mark.channel, pytest.mark.examples, pytest.mark.mqtt, pytest.mark.qos1]),
    pytest.param('bench_examples_plugins_mqtt_plugin_pb_chn_benchmark_qos0.yaml', marks=[pytest.mark.aimrt, pytest.mark.bench, pytest.mark.channel, pytest.mark.examples, pytest.mark.mqtt, pytest.mark.qos0]),
    pytest.param('bench_examples_plugins_mqtt_plugin_pb_rpc_benchmark_qos1.yaml', marks=[pytest.mark.aimrt, pytest.mark.bench, pytest.mark.examples, pytest.mark.mqtt, pytest.mark.qos1, pytest.mark.rpc]),
    pytest.param('bench_examples_plugins_mqtt_plugin_pb_chn_benchmark_qos2.yaml', marks=[pytest.mark.aimrt, pytest.mark.bench, pytest.mark.channel, pytest.mark.examples, pytest.mark.mqtt, pytest.mark.qos2]),
    pytest.param('bench_examples_plugins_mqtt_plugin_pb_rpc_benchmark_qos2.yaml', marks=[pytest.mark.aimrt, pytest.mark.bench, pytest.mark.examples, pytest.mark.mqtt, pytest.mark.qos2, pytest.mark.rpc]),
    pytest.param('bench_examples_plugins_mqtt_plugin_pb_rpc_benchmark_qos0.yaml', marks=[pytest.mark.aimrt, pytest.mark.bench, pytest.mark.examples, pytest.mark.mqtt, pytest.mark.qos0, pytest.mark.rpc]),
]

@pytest.mark.parametrize('yaml_name', CASES)
def test_generated_benchmarks(yaml_name: str, aimrt_test_runner: AimRTTestRunner):
    yaml_path = Path(__file__).parent / yaml_name
    if not yaml_path.exists():
        pytest.skip(f'YAML not found: {yaml_path}')
    if not aimrt_test_runner.setup_from_yaml(str(yaml_path)):
        pytest.fail('Failed to setup test environment from YAML configuration')
    success = aimrt_test_runner.run_test()
    if not success:
        pytest.fail('benchmark test execution failed')