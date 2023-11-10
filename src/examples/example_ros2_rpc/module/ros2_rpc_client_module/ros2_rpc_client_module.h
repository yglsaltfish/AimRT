#pragma once

#include <atomic>
#include <memory>

#include "aimrt_module_cpp_interface/co/async_scope.h"
#include "aimrt_module_cpp_interface/co/task.h"
#include "aimrt_module_cpp_interface/module_base.h"

#include "RosTestRpc.aimrt_rpc.srv.h"

namespace aimrt::examples::example_ros2_rpc::ros2_rpc_client_module {

class Ros2RpcClientModule : public aimrt::ModuleBase {
 public:
  Ros2RpcClientModule() = default;
  ~Ros2RpcClientModule() override = default;

  ModuleInfo Info() const noexcept override {
    return ModuleInfo{.name = "Ros2RpcClientModule"};
  }

  bool Initialize(aimrt::CoreRef core) noexcept override;

  bool Start() noexcept override;

  void Shutdown() noexcept override;

 private:
  aimrt::LoggerRef GetLogger() { return core_.GetLogger(); }

  aimrt::co::Task<void> MainLoop();

 private:
  aimrt::CoreRef core_;
  aimrt::ExecutorRef executor_;

  aimrt::co::AsyncScope scope_;
  std::atomic_bool run_flag_ = true;

  double rpc_frq_;
  std::shared_ptr<example_ros2::srv::RosTestRpcProxy> proxy_;
};

}  // namespace aimrt::examples::example_ros2_rpc::ros2_rpc_client_module
