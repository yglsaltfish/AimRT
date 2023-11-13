#include "helloworld_module/helloworld_module.h"
#include "aimrt_module_cpp_interface/co/aimrt_context.h"
#include "aimrt_module_cpp_interface/co/schedule.h"
#include "aimrt_module_cpp_interface/co/sync_wait.h"

#include "yaml-cpp/yaml.h"

namespace aimrt::examples::example_helloworld::helloworld_module {

bool HelloWorldModule::Initialize(aimrt::CoreRef core) noexcept {
  // Save aimrt framework handle
  core_ = core;

  try {
    // Read cfg
    aimrt::ConfiguratorRef configurator = core_.GetConfigurator();
    if (configurator) {
      std::string file_path = std::string(configurator.GetConfigFilePath());
      if (!file_path.empty()) {
        YAML::Node cfg_node = YAML::LoadFile(file_path);
        for (const auto& itr : cfg_node) {
          std::string k = itr.first.as<std::string>();
          std::string v = itr.second.as<std::string>();
          AIMRT_HL_INFO(core_.GetLogger(), "cfg [{} : {}]", k, v);
        }
      }
    }

    // Get executor handle
    executor_ = core_.GetExecutorManager().GetExecutor("work_thread_pool");
    AIMRT_HL_CHECK_ERROR_THROW(core_.GetLogger(), executor_,
                               "Get executor 'work_thread_pool' failed.");

  } catch (const std::exception& e) {
    AIMRT_HL_ERROR(core_.GetLogger(), "Init failed, {}", e.what());
    return false;
  }

  AIMRT_HL_INFO(core_.GetLogger(), "Init succeeded.");

  return true;
}

bool HelloWorldModule::Start() noexcept {
  try {
    // Start main loop
    scope_.spawn(MainLoop());
  } catch (const std::exception& e) {
    AIMRT_HL_ERROR(core_.GetLogger(), "Start failed, {}", e.what());
    return false;
  }

  AIMRT_HL_INFO(core_.GetLogger(), "Start succeeded.");
  return true;
}

void HelloWorldModule::Shutdown() noexcept {
  try {
    // Wait all coroutine complete
    run_flag_ = false;
    aimrt::co::SyncWait(scope_.complete());
  } catch (const std::exception& e) {
    AIMRT_HL_ERROR(core_.GetLogger(), "Shutdown failed, {}", e.what());
    return;
  }

  AIMRT_HL_INFO(core_.GetLogger(), "Shutdown succeeded.");
}

aimrt::co::Task<void> HelloWorldModule::MainLoop() {
  try {
    AIMRT_HL_INFO(core_.GetLogger(), "Start MainLoop.");

    aimrt::co::AimRTScheduler work_thread_pool_scheduler(executor_);

    co_await aimrt::co::Schedule(work_thread_pool_scheduler);

    uint32_t count = 0;
    while (run_flag_) {
      // Sleep for some time
      co_await aimrt::co::ScheduleAfter(
          work_thread_pool_scheduler, std::chrono::milliseconds(1000));

      count++;
      AIMRT_HL_INFO(core_.GetLogger(),
                    "Loop count : {} -------------------------", count);

      // Start a new task coroutine
      scope_.spawn_on(work_thread_pool_scheduler, TestTask(count));
    }

    AIMRT_HL_INFO(core_.GetLogger(), "Exit MainLoop.");
  } catch (const std::exception& e) {
    AIMRT_HL_ERROR(core_.GetLogger(),
                   "Exit MainLoop with exception, {}", e.what());
  }

  co_return;
}

aimrt::co::Task<void> HelloWorldModule::TestTask(uint32_t count) {
  AIMRT_HL_INFO(core_.GetLogger(), "Start TestTask {}.", count);

  // Sleep for some time
  co_await aimrt::co::ScheduleAfter(
      aimrt::co::AimRTScheduler(executor_), std::chrono::milliseconds(2000));

  AIMRT_HL_INFO(core_.GetLogger(), "Exit TestTask {}.", count);

  co_return;
}

}  // namespace aimrt::examples::example_helloworld::helloworld_module
