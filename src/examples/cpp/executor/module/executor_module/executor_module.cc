#include "executor_module/executor_module.h"
#include "aimrt_module_cpp_interface/co/aimrt_context.h"
#include "aimrt_module_cpp_interface/co/on.h"
#include "aimrt_module_cpp_interface/co/schedule.h"
#include "aimrt_module_cpp_interface/co/sync_wait.h"

#include "yaml-cpp/yaml.h"

namespace aimrt::examples::cpp::executor::executor_module {

bool ExecutorModule::Initialize(aimrt::CoreRef core) noexcept {
  // Save aimrt framework handle
  core_ = core;

  AIMRT_INFO("Init succeeded.");

  return true;
}

bool ExecutorModule::Start() noexcept {
  try {
  } catch (const std::exception& e) {
    AIMRT_ERROR("Start failed, {}", e.what());
    return false;
  }

  AIMRT_INFO("Start succeeded.");
  return true;
}

void ExecutorModule::Shutdown() noexcept {
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

}  // namespace aimrt::examples::cpp::executor::executor_module
