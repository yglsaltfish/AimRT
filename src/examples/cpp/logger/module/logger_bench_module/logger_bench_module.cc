// Copyright (c) 2023, AgiBot Inc.
// All rights reserved.

#include "logger_bench_module/logger_bench_module.h"

#include "yaml-cpp/yaml.h"

namespace aimrt::examples::cpp::logger::logger_bench_module {

bool LoggerBenchModule::Initialize(aimrt::CoreRef core) {
  // Save aimrt framework handle
  core_ = core;

  // Read cfg
  auto file_path = core_.GetConfigurator().GetConfigFilePath();
  if (!file_path.empty()) {
    YAML::Node cfg_node = YAML::LoadFile(file_path.data());
    log_size_ = cfg_node["log_size"].as<size_t>();
    bench_num_ = cfg_node["bench_num"].as<size_t>();
  }

  auto work_executor = core_.GetExecutorManager().GetExecutor("work_executor");
  AIMRT_CHECK_ERROR_THROW(work_executor, "Get executor 'work_thread_pool' failed.");

  AIMRT_INFO("Init succeeded.");

  return true;
}

bool LoggerBenchModule::Start() {
  AIMRT_INFO("Start succeeded.");

  auto work_executor = core_.GetExecutorManager().GetExecutor("work_executor");

  std::string log_data;
  log_data.reserve(log_size_);
  for (size_t ii = 0; ii < log_size_; ++ii) {
    log_data += std::to_string(ii % 10);
  }

  work_executor.Execute([this, log_data{std::move(log_data)}]() {
    using namespace std::chrono;

    auto start_time_point = steady_clock::now();

    for (size_t ii = 0; ii < bench_num_; ++ii) {
      AIMRT_INFO("{}", log_data);
    }

    auto end_time_point = steady_clock::now();

    auto total_duration = end_time_point - start_time_point;

    AIMRT_INFO("Printed {} logs ( {} bytes per log ) within {} microseconds, with an average of {} nanoseconds per log",
               bench_num_,
               log_size_,
               duration_cast<microseconds>(total_duration).count(),
               duration_cast<nanoseconds>(total_duration).count() / bench_num_);

    log_loop_stop_sig_.set_value();
  });

  return true;
}

void LoggerBenchModule::Shutdown() {
  log_loop_stop_sig_.get_future().wait();

  AIMRT_INFO("Shutdown succeeded.");
}

}  // namespace aimrt::examples::cpp::logger::logger_bench_module
