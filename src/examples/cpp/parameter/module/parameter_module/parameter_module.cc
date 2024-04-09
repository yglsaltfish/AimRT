#include "parameter_module/parameter_module.h"
#include "aimrt_module_cpp_interface/co/aimrt_context.h"
#include "aimrt_module_cpp_interface/co/inline_scheduler.h"
#include "aimrt_module_cpp_interface/co/on.h"
#include "aimrt_module_cpp_interface/co/schedule.h"
#include "aimrt_module_cpp_interface/co/sync_wait.h"

#include "yaml-cpp/yaml.h"

namespace aimrt::examples::cpp::parameter::parameter_module {

bool ParameterModule::Initialize(aimrt::CoreRef core) noexcept {
  // Save aimrt framework handle
  core_ = core;

  try {
    // Get executor handle
    work_executor_ = core_.GetExecutorManager().GetExecutor("work_thread_pool");
    AIMRT_CHECK_ERROR_THROW(
        work_executor_ && work_executor_.SupportTimerSchedule(),
        "Get executor 'work_thread_pool' failed.");

    parameter_handle_ = core_.GetParameterHandle();
    AIMRT_CHECK_ERROR_THROW(parameter_handle_, "Get parameter failed.");

  } catch (const std::exception& e) {
    AIMRT_ERROR("Init failed, {}", e.what());
    return false;
  }

  AIMRT_INFO("Init succeeded.");

  return true;
}

bool ParameterModule::Start() noexcept {
  try {
    scope_.spawn(co::On(co::AimRTScheduler(work_executor_), SetParameterLoop()));
    scope_.spawn(co::On(co::AimRTScheduler(work_executor_), GetParameterLoop()));
  } catch (const std::exception& e) {
    AIMRT_ERROR("Start failed, {}", e.what());
    return false;
  }

  AIMRT_INFO("Start succeeded.");
  return true;
}

void ParameterModule::Shutdown() noexcept {
  try {
    // Wait all coroutine complete
    run_flag_ = false;
    co::SyncWait(scope_.complete());
  } catch (const std::exception& e) {
    AIMRT_ERROR("Shutdown failed, {}", e.what());
    return;
  }

  AIMRT_INFO("Shutdown succeeded.");
}

co::Task<void> ParameterModule::SetParameterLoop() {
  try {
    AIMRT_INFO("Start SetParameterLoop.");

    co::AimRTScheduler work_scheduler(work_executor_);

    uint32_t count = 0;
    while (run_flag_) {
      count++;
      AIMRT_INFO("SetParameterLoop count : {} -------------------------", count);

      std::string key = "key-" + std::to_string(count);
      std::string val = "val-" + std::to_string(count);
      parameter_handle_.SetParameter(key, val);
      AIMRT_INFO("Set parameter, key: '{}', val: '{}'", key, val);

      co_await co::ScheduleAfter(
          work_scheduler, std::chrono::milliseconds(1000));
    }

    AIMRT_INFO("Exit SetParameterLoop.");
  } catch (const std::exception& e) {
    AIMRT_ERROR("Exit SetParameterLoop with exception, {}", e.what());
  }

  co_return;
}

co::Task<void> ParameterModule::GetParameterLoop() {
  try {
    AIMRT_INFO("Start GetParameterLoop.");

    co::AimRTScheduler work_scheduler(work_executor_);

    uint32_t count = 0;
    while (run_flag_) {
      count++;
      AIMRT_INFO("GetParameterLoop count : {} -------------------------", count);

      std::string key = "key-" + std::to_string(count);
      auto val = parameter_handle_.GetParameter(key);
      AIMRT_INFO("Get parameter, key: '{}', val: '{}'", key, val);

      co_await co::ScheduleAfter(
          work_scheduler,
          std::chrono::milliseconds(1000));
    }

    AIMRT_INFO("Exit GetParameterLoop.");
  } catch (const std::exception& e) {
    AIMRT_ERROR("Exit GetParameterLoop with exception, {}", e.what());
  }

  co_return;
}

}  // namespace aimrt::examples::cpp::parameter::parameter_module
