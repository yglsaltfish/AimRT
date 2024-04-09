#pragma once

#include <atomic>

#include "aimrt_module_cpp_interface/co/async_scope.h"
#include "aimrt_module_cpp_interface/co/task.h"
#include "aimrt_module_cpp_interface/module_base.h"

namespace aimrt::examples::cpp::executor::real_time_module {

class RealTimeModule : public aimrt::ModuleBase {
 public:
  RealTimeModule() = default;
  ~RealTimeModule() override = default;

  ModuleInfo Info() const noexcept override {
    return ModuleInfo{.name = "RealTimeModule"};
  }

  bool Initialize(aimrt::CoreRef aimrt_ptr) noexcept override;

  bool Start() noexcept override;

  void Shutdown() noexcept override;

 private:
  aimrt::logger::LoggerRef GetLogger() { return core_.GetLogger(); }

  co::Task<void> WorkLoop(aimrt::executor::ExecutorRef executor_ptr);

 private:
  aimrt::CoreRef core_;
  co::AsyncScope scope_;
  std::atomic_bool run_flag_ = true;
};

}  // namespace aimrt::examples::cpp::executor::real_time_module
