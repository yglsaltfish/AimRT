#pragma once

#include <memory>

#include "aimrt_module_cpp_interface/module_base.h"
#include "normal_rpc_server_module/service.h"

namespace aimrt::examples::cpp::protobuf_rpc::normal_rpc_server_module {

class NormalRpcServerModule : public aimrt::ModuleBase {
 public:
  NormalRpcServerModule() = default;
  ~NormalRpcServerModule() override = default;

  ModuleInfo Info() const override {
    return ModuleInfo{.name = "NormalRpcServerModule"};
  }

  bool Initialize(aimrt::CoreRef core) override;

  bool Start() override;

  void Shutdown() override;

 private:
  aimrt::CoreRef core_;
  std::shared_ptr<ExampleServiceImpl> service_ptr_;
};

}  // namespace aimrt::examples::cpp::protobuf_rpc::normal_rpc_server_module
