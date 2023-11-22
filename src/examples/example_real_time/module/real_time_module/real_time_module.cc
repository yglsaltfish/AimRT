#include "real_time_module/real_time_module.h"
#include "aimrt_module_cpp_interface/co/aimrt_context.h"
#include "aimrt_module_cpp_interface/co/schedule.h"
#include "aimrt_module_cpp_interface/co/sync_wait.h"

#include "yaml-cpp/yaml.h"

namespace aimrt::examples::example_real_time::real_time_module {

bool RealTimeModule::Initialize(aimrt::CoreRef core) noexcept {
  // Save aimrt framework handle
  core_ = core;

  try {
    // Read cfg
    const auto configurator = core_.GetConfigurator();
    if (configurator) {
      std::string file_path = std::string(configurator.GetConfigFilePath());
      if (!file_path.empty()) {
        YAML::Node cfg_node = YAML::LoadFile(file_path);
        for (const auto& itr : cfg_node) {
          std::string k = itr.first.as<std::string>();
          std::string v = itr.second.as<std::string>();
          AIMRT_INFO("cfg [{} : {}]", k, v);
        }
      }
    }

  } catch (const std::exception& e) {
    AIMRT_ERROR("Init failed, {}", e.what());
    return false;
  }

  AIMRT_INFO("Init succeeded.");

  return true;
}

bool RealTimeModule::Start() noexcept {
  try {
    // sched_fifo_thread
    auto sched_fifo_thread_executor =
        core_.GetExecutorManager().GetExecutor("sched_fifo_thread");
    AIMRT_CHECK_ERROR_THROW(sched_fifo_thread_executor,
                            "Get executor 'sched_fifo_thread' failed.");
    scope_.spawn_on(
        aimrt::co::AimRTScheduler(sched_fifo_thread_executor),
        WorkLoop(sched_fifo_thread_executor));

    // sched_other_thread
    auto sched_other_thread_executor =
        core_.GetExecutorManager().GetExecutor("sched_other_thread");
    AIMRT_CHECK_ERROR_THROW(sched_other_thread_executor,
                            "Get executor 'sched_other_thread' failed.");
    scope_.spawn_on(
        aimrt::co::AimRTScheduler(sched_other_thread_executor),
        WorkLoop(sched_other_thread_executor));

    // sched_rr_thread
    auto sched_rr_thread_executor =
        core_.GetExecutorManager().GetExecutor("sched_rr_thread");
    AIMRT_CHECK_ERROR_THROW(sched_rr_thread_executor,
                            "Get executor 'sched_rr_thread' failed.");
    scope_.spawn_on(
        aimrt::co::AimRTScheduler(sched_rr_thread_executor),
        WorkLoop(sched_rr_thread_executor));

  } catch (const std::exception& e) {
    AIMRT_ERROR("Start failed, {}", e.what());
    return false;
  }

  AIMRT_INFO("Start succeeded.");
  return true;
}

void RealTimeModule::Shutdown() noexcept {
  try {
    // Wait all coroutine complete
    run_flag_ = false;
    aimrt::co::SyncWait(scope_.complete());
  } catch (const std::exception& e) {
    AIMRT_ERROR("Shutdown failed, {}", e.what());
    return;
  }

  AIMRT_INFO("Shutdown succeeded.");
}

aimrt::co::Task<void> RealTimeModule::WorkLoop(aimrt::executor::ExecutorRef executor) {
  try {
    AIMRT_INFO("Start WorkLoop in {}.", executor.Name());

    // Get thread name
    char thread_name[16];
    pthread_getname_np(pthread_self(), thread_name, sizeof(thread_name));

    // Get thread sched param
    int policy;
    struct sched_param param;
    pthread_getschedparam(pthread_self(), &policy, &param);

    AIMRT_INFO("Executor name: {}, thread_name: {}, policy: {}, priority: {}",
               executor.Name(), thread_name, policy, param.sched_priority);

    uint32_t count = 0;
    while (run_flag_) {
      count++;

      // Sleep for some time
      auto start_tp = std::chrono::steady_clock::now();
      co_await aimrt::co::ScheduleAfter(
          aimrt::co::AimRTScheduler(executor), std::chrono::milliseconds(1000));
      auto end_tp = std::chrono::steady_clock::now();

      // Get cpuset used by the current thread
      cpu_set_t cur_cpuset;
      CPU_ZERO(&cur_cpuset);
      auto pthread_getaffinity_np_ret =
          pthread_getaffinity_np(pthread_self(), sizeof(cpu_set_t), &cur_cpuset);
      AIMRT_CHECK_ERROR_THROW(pthread_getaffinity_np_ret == 0,
                              "Call 'pthread_getaffinity_np' get error: {}",
                              pthread_getaffinity_np_ret);

      uint32_t cpu_size = std::thread::hardware_concurrency();
      std::string cur_cpuset_str;
      for (int ii = 0; ii < cpu_size; ii++) {
        if (CPU_ISSET(ii, &cur_cpuset)) {
          cur_cpuset_str += (std::to_string(ii) + ", ");
        }
      }
      cur_cpuset_str = cur_cpuset_str.substr(0, cur_cpuset_str.size() - 2);

      // Get cpu index used by the current code
      unsigned int current_cpu, current_node;
      int getcpu_ret = getcpu(&current_cpu, &current_node);
      AIMRT_CHECK_ERROR_THROW(getcpu_ret == 0, "Call 'getcpu' get error: {}", getcpu_ret);

      // Log
      AIMRT_INFO(
          "Loop count: {}, executor name: {}, sleep for {}, cpu: {}, node: {}, use cpu: '{}'",
          count, executor.Name(), end_tp - start_tp, current_cpu, current_node, cur_cpuset_str);
    }

    AIMRT_INFO("Exit WorkLoop.");
  } catch (const std::exception& e) {
    AIMRT_ERROR("Exit WorkLoop with exception, {}", e.what());
  }

  co_return;
}

}  // namespace aimrt::examples::example_real_time::real_time_module
