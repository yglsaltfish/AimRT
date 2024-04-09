#pragma once

#include <atomic>

#include "aimrt_module_cpp_interface/co/async_scope.h"
#include "aimrt_module_cpp_interface/co/task.h"
#include "aimrt_module_cpp_interface/module_base.h"

namespace aimrt::examples::cpp::parameter::parameter_module {

class ParameterModule : public aimrt::ModuleBase {
 public:
  ParameterModule() = default;
  ~ParameterModule() override = default;

  ModuleInfo Info() const noexcept override {
    return ModuleInfo{.name = "ParameterModule"};
  }

  bool Initialize(aimrt::CoreRef core) noexcept override;

  bool Start() noexcept override;

  void Shutdown() noexcept override;

 private:
  aimrt::logger::LoggerRef GetLogger() { return core_.GetLogger(); }

  co::Task<void> SetParameterLoop();
  co::Task<void> GetParameterLoop();

 private:
  aimrt::CoreRef core_;
  aimrt::executor::ExecutorRef work_executor_;
  aimrt::parameter::ParameterHandleRef parameter_handle_;

  co::AsyncScope scope_;
  std::atomic_bool run_flag_ = true;
};

}  // namespace aimrt::examples::cpp::parameter::parameter_module
