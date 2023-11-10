
#pragma once

#include "core/rpc/rpc_backend_base.h"

namespace aimrt::plugins::sm_plugin {

class SmRpcBackend : public runtime::core::rpc::RpcBackendBase {
 public:
  struct Options {};

 public:
  SmRpcBackend() = default;
  ~SmRpcBackend() override = default;

  std::string_view Name() const override { return "sm"; }

  void Initialize(YAML::Node options_node, const runtime::core::rpc::RpcRegistry* rpc_registry_ptr,
                  runtime::core::rpc::ContextManager* context_manager_ptr) override;
  void Start() override;
  void Shutdown() override;

  bool RegisterServiceFunc(
      const runtime::core::rpc::ServiceFuncWrapper& service_func_wrapper) noexcept override;
  bool RegisterClientFunc(
      const runtime::core::rpc::ClientFuncWrapper& client_func_wrapper) noexcept override;
  bool TryInvoke(
      const std::shared_ptr<runtime::core::rpc::ClientInvokeWrapper>& client_invoke_wrapper_ptr) noexcept override;

 private:
  enum class Status : uint32_t {
    PreInit,
    Init,
    Start,
    Shutdown,
  };

  Options options_;
  std::atomic<Status> status_ = Status::PreInit;
};

}  // namespace aimrt::plugins::sm_plugin
