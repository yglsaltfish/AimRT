#include "logger_bench_module/logger_bench_module.h"

#include "yaml-cpp/yaml.h"

namespace aimrt::examples::cpp::logger::logger_bench_module {

bool LoggerBenchModule::Initialize(aimrt::CoreRef core) {
  // Save aimrt framework handle
  core_ = core;

  auto work_executor = core_.GetExecutorManager().GetExecutor("work_executor");
  AIMRT_CHECK_ERROR_THROW(work_executor, "Get executor 'work_thread_pool' failed.");

  AIMRT_INFO("Init succeeded.");

  return true;
}

bool LoggerBenchModule::Start() {
  AIMRT_INFO("Start succeeded.");

  static constexpr size_t log_bench_num = 10 * 1000;

  auto work_executor = core_.GetExecutorManager().GetExecutor("work_executor");

  work_executor.Execute([this]() {
    using namespace std::chrono;

    auto start_time_point = steady_clock::now();

    for (size_t ii = 0; ii < log_bench_num; ++ii) {
      AIMRT_INFO("This is a test bench log. This is a test bench log. This is a test bench log.");
    }

    auto end_time_point = steady_clock::now();

    auto total_duration = end_time_point - start_time_point;

    AIMRT_INFO("Printed {} logs within {} microseconds, with an average of {} nanoseconds per log",
               log_bench_num,
               duration_cast<microseconds>(total_duration).count(),
               duration_cast<nanoseconds>(total_duration).count() / log_bench_num);

    log_loop_stop_sig_.set_value();
  });

  return true;
}

void LoggerBenchModule::Shutdown() {
  log_loop_stop_sig_.get_future().wait();

  AIMRT_INFO("Shutdown succeeded.");
}

}  // namespace aimrt::examples::cpp::logger::logger_bench_module
