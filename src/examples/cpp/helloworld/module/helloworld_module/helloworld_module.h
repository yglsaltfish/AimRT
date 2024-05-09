#pragma once

#include <atomic>

#include "aimrt_module_cpp_interface/co/async_scope.h"
#include "aimrt_module_cpp_interface/co/task.h"
#include "aimrt_module_cpp_interface/module_base.h"

namespace aimrt::examples::cpp::helloworld::helloworld_module {

class HelloWorldModule : public aimrt::ModuleBase {
 public:
  HelloWorldModule() = default;
  ~HelloWorldModule() override = default;

  ModuleInfo Info() const override {
    return ModuleInfo{.name = "HelloWorldModule"};
  }

  bool Initialize(aimrt::CoreRef core) override;

  bool Start() override;

  void Shutdown() override;

 private:
  auto GetLogger() { return core_.GetLogger(); }

  co::Task<void> MainLoop();

 private:
  aimrt::CoreRef core_;
  aimrt::executor::ExecutorRef work_executor_;

  co::AsyncScope scope_;
  std::atomic_bool run_flag_ = true;
};

}  // namespace aimrt::examples::cpp::helloworld::helloworld_module
