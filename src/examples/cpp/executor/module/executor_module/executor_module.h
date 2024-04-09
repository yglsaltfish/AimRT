#pragma once

#include <atomic>

#include "aimrt_module_cpp_interface/co/async_scope.h"
#include "aimrt_module_cpp_interface/co/task.h"
#include "aimrt_module_cpp_interface/module_base.h"

namespace aimrt::examples::cpp::executor::executor_module {

class ExecutorModule : public aimrt::ModuleBase {
 public:
  ExecutorModule() = default;
  ~ExecutorModule() override = default;

  ModuleInfo Info() const noexcept override {
    return ModuleInfo{.name = "ExecutorModule"};
  }

  bool Initialize(aimrt::CoreRef aimrt_ptr) noexcept override;

  bool Start() noexcept override;

  void Shutdown() noexcept override;

 private:
  aimrt::logger::LoggerRef GetLogger() { return core_.GetLogger(); }

 private:
  aimrt::CoreRef core_;
  co::AsyncScope scope_;
  std::atomic_bool run_flag_ = true;
};

}  // namespace aimrt::examples::cpp::executor::executor_module
