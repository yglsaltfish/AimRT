#pragma once

#include "aimrt_module_cpp_interface/module_base.h"
#include "ros2_rpc_server_module/service.h"

namespace aimrt::examples::example_ros2_rpc::ros2_rpc_server_module {

class Ros2RpcServerModule : public aimrt::ModuleBase {
 public:
  Ros2RpcServerModule() = default;
  ~Ros2RpcServerModule() override = default;

  ModuleInfo Info() const noexcept override {
    return ModuleInfo{.name = "Ros2RpcServerModule"};
  }

  bool Initialize(aimrt::CoreRef core) noexcept override;

  bool Start() noexcept override;

  void Shutdown() noexcept override;

 private:
  aimrt::CoreRef core_;
  std::shared_ptr<RosTestRpcServiceImpl> service_ptr_;
};

}  // namespace aimrt::examples::example_ros2_rpc::ros2_rpc_server_module
