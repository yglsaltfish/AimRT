#pragma once

#include <atomic>

#include "core/rpc/rpc_backend_base.h"

namespace aimrt::runtime::core::rpc {

class RpcBackendManager {
 public:
  enum class State : uint32_t {
    PreInit,
    Init,
    Start,
    Shutdown,
  };

 public:
  RpcBackendManager() = default;
  ~RpcBackendManager() = default;

  RpcBackendManager(const RpcBackendManager&) = delete;
  RpcBackendManager& operator=(const RpcBackendManager&) = delete;

  void Initialize(RpcRegistry* rpc_registry_ptr);
  void Start();
  void Shutdown();

  void RegisterRpcBackend(RpcBackendBase* rpc_backend_ptr);

  bool RegisterServiceFunc(ServiceFuncWrapper&& service_func_wrapper);
  bool RegisterClientFunc(ClientFuncWrapper&& client_func_wrapper);
  void Invoke(ClientInvokeWrapper&& client_invoke_wrapper);

  State GetState() const { return state_.load(); }

 private:
  std::atomic<State> state_ = State::PreInit;

  RpcRegistry* rpc_registry_ptr_ = nullptr;

  std::vector<RpcBackendBase*> rpc_backend_index_vec_;
  std::map<std::string_view, RpcBackendBase*> rpc_backend_index_map_;
};
}  // namespace aimrt::runtime::core::rpc