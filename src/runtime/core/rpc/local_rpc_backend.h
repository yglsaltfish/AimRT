#pragma once

#include <atomic>

#include "core/rpc/rpc_backend_base.h"

namespace aimrt::runtime::core::rpc {

class LocalRpcBackend : public RpcBackendBase {
 public:
  struct Options {};

 public:
  LocalRpcBackend() = default;
  ~LocalRpcBackend() override = default;

  std::string_view Name() const override { return "local"; }

  void Initialize(YAML::Node options_node,
                  const RpcRegistry* rpc_registry_ptr,
                  ContextManager* context_manager_ptr) override;
  void Start() override;
  void Shutdown() override;

  bool RegisterServiceFunc(
      const ServiceFuncWrapper& service_func_wrapper) noexcept override;
  bool RegisterClientFunc(
      const ClientFuncWrapper& client_func_wrapper) noexcept override;
  bool TryInvoke(
      const std::shared_ptr<ClientInvokeWrapper>& client_invoke_wrapper_ptr) noexcept override;

 private:
  enum class Status : uint32_t {
    PreInit,
    Init,
    Start,
    Shutdown,
  };

  Options options_;
  std::atomic<Status> status_ = Status::PreInit;

  const RpcRegistry* rpc_registry_ptr_ = nullptr;
  ContextManager* context_manager_ptr_ = nullptr;

  using ServiceFuncIndexMap =
      std::map<std::string_view,                         // func_name
               std::map<std::string_view,                // lib_path
                        std::set<std::string_view> > >;  // module_name
  ServiceFuncIndexMap service_func_register_index_;
};

}  // namespace aimrt::runtime::core::rpc
