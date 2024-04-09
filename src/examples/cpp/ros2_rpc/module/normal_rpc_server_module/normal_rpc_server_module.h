#pragma once

#include "aimrt_module_cpp_interface/module_base.h"
#include "normal_rpc_server_module/service.h"

namespace aimrt::examples::cpp::ros2_rpc::normal_rpc_server_module {

class NormalRpcServerModule : public aimrt::ModuleBase {
 public:
  NormalRpcServerModule() = default;
  ~NormalRpcServerModule() override = default;

  ModuleInfo Info() const noexcept override {
    return ModuleInfo{.name = "NormalRpcServerModule"};
  }

  bool Initialize(aimrt::CoreRef core) noexcept override;

  bool Start() noexcept override;

  void Shutdown() noexcept override;

 private:
  aimrt::CoreRef core_;
  std::shared_ptr<RosTestRpcServiceImpl> service_ptr_;
};

}  // namespace aimrt::examples::cpp::ros2_rpc::normal_rpc_server_module
