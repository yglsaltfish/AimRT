#include "helloworld_module/helloworld_module.h"
#include "aimrt_module_cpp_interface/co/aimrt_context.h"
#include "aimrt_module_cpp_interface/co/inline_scheduler.h"
#include "aimrt_module_cpp_interface/co/on.h"
#include "aimrt_module_cpp_interface/co/schedule.h"
#include "aimrt_module_cpp_interface/co/sync_wait.h"

#include "yaml-cpp/yaml.h"

namespace aimrt::examples::cpp::helloworld::helloworld_module {

bool HelloWorldModule::Initialize(aimrt::CoreRef core) {
  // Save aimrt framework handle
  core_ = core;

  // Read cfg
  auto file_path = core_.GetConfigurator().GetConfigFilePath();
  if (!file_path.empty()) {
    YAML::Node cfg_node = YAML::LoadFile(file_path.data());
    for (const auto& itr : cfg_node) {
      std::string k = itr.first.as<std::string>();
      std::string v = itr.second.as<std::string>();
      AIMRT_INFO("cfg [{} : {}]", k, v);
    }
  }

  // Get executor handle
  work_executor_ = core_.GetExecutorManager().GetExecutor("work_thread_pool");
  AIMRT_CHECK_ERROR_THROW(work_executor_ && work_executor_.SupportTimerSchedule(),
                          "Get executor 'work_thread_pool' failed.");

  AIMRT_INFO("Init succeeded.");

  return true;
}

bool HelloWorldModule::Start() {
  scope_.spawn(co::On(co::InlineScheduler(), MainLoop()));

  AIMRT_INFO("Start succeeded.");
  return true;
}

void HelloWorldModule::Shutdown() {
  run_flag_ = false;
  co::SyncWait(scope_.complete());

  AIMRT_INFO("Shutdown succeeded.");
}

co::Task<void> HelloWorldModule::MainLoop() {
  try {
    AIMRT_INFO("Start MainLoop.");

    co::AimRTScheduler work_scheduler(work_executor_);

    co_await co::Schedule(work_scheduler);

    uint32_t count = 0;
    while (run_flag_) {
      count++;
      AIMRT_INFO("Loop count : {} -------------------------", count);

      co_await co::ScheduleAfter(work_scheduler, std::chrono::seconds(1));
    }

    AIMRT_INFO("Exit MainLoop.");
  } catch (const std::exception& e) {
    AIMRT_ERROR("Exit MainLoop with exception, {}", e.what());
  }

  co_return;
}

}  // namespace aimrt::examples::cpp::helloworld::helloworld_module
