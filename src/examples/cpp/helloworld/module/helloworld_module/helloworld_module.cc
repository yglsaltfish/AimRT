#include "helloworld_module/helloworld_module.h"
#include "aimrt_module_cpp_interface/co/aimrt_context.h"
#include "aimrt_module_cpp_interface/co/inline_scheduler.h"
#include "aimrt_module_cpp_interface/co/on.h"
#include "aimrt_module_cpp_interface/co/schedule.h"
#include "aimrt_module_cpp_interface/co/sync_wait.h"

#include "yaml-cpp/yaml.h"

namespace aimrt::examples::cpp::helloworld::helloworld_module {

bool HelloWorldModule::Initialize(aimrt::CoreRef core) noexcept {
  // Save aimrt framework handle
  core_ = core;

  try {
    // Read cfg
    auto configurator = core_.GetConfigurator();
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
    work_executor_ = core_.GetExecutorManager().GetExecutor("work_thread_pool");
    AIMRT_HL_CHECK_ERROR_THROW(core_.GetLogger(),
                               work_executor_ && work_executor_.SupportTimerSchedule(),
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
    scope_.spawn(co::On(co::InlineScheduler(), MainLoop()));
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
    co::SyncWait(scope_.complete());
  } catch (const std::exception& e) {
    AIMRT_HL_ERROR(core_.GetLogger(), "Shutdown failed, {}", e.what());
    return;
  }

  AIMRT_HL_INFO(core_.GetLogger(), "Shutdown succeeded.");
}

co::Task<void> HelloWorldModule::MainLoop() {
  try {
    AIMRT_HL_INFO(core_.GetLogger(), "Start MainLoop.");

    co::AimRTScheduler work_scheduler(work_executor_);

    co_await co::Schedule(work_scheduler);

    uint32_t count = 0;
    while (run_flag_) {
      count++;
      AIMRT_HL_INFO(core_.GetLogger(),
                    "Loop count : {} -------------------------", count);

      co_await co::ScheduleAfter(work_scheduler, std::chrono::seconds(1));
    }

    AIMRT_HL_INFO(core_.GetLogger(), "Exit MainLoop.");
  } catch (const std::exception& e) {
    AIMRT_HL_ERROR(core_.GetLogger(),
                   "Exit MainLoop with exception, {}", e.what());
  }

  co_return;
}

}  // namespace aimrt::examples::cpp::helloworld::helloworld_module
