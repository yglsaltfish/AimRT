#pragma once

#include <atomic>
#include <memory>

#include "aimrt_module_cpp_interface/co/async_scope.h"
#include "aimrt_module_cpp_interface/co/task.h"
#include "aimrt_module_cpp_interface/module_base.h"

#include "RosTestRpc.aimrt_rpc.srv.h"

namespace aimrt::examples::cpp::ros2_rpc::normal_rpc_client_module {

class NormalRpcClientModule : public aimrt::ModuleBase {
 public:
  NormalRpcClientModule() = default;
  ~NormalRpcClientModule() override = default;

  ModuleInfo Info() const noexcept override {
    return ModuleInfo{.name = "NormalRpcClientModule"};
  }

  bool Initialize(aimrt::CoreRef core) noexcept override;

  bool Start() noexcept override;

  void Shutdown() noexcept override;

 private:
  aimrt::logger::LoggerRef GetLogger() { return core_.GetLogger(); }

  co::Task<void> MainLoop();

 private:
  aimrt::CoreRef core_;
  aimrt::executor::ExecutorRef executor_;

  co::AsyncScope scope_;
  std::atomic_bool run_flag_ = true;

  double rpc_frq_;
  std::shared_ptr<example_ros2::srv::RosTestRpcProxy> proxy_;
};

}  // namespace aimrt::examples::cpp::ros2_rpc::normal_rpc_client_module
