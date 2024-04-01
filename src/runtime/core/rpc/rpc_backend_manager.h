#pragma once

#include <atomic>

#include "core/rpc/rpc_backend_base.h"
#include "util/log_util.h"

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
  RpcBackendManager()
      : logger_ptr_(std::make_shared<common::util::LoggerWrapper>()) {}
  ~RpcBackendManager() = default;

  RpcBackendManager(const RpcBackendManager&) = delete;
  RpcBackendManager& operator=(const RpcBackendManager&) = delete;

  void Initialize(RpcRegistry* rpc_registry_ptr);
  void Start();
  void Shutdown();

  void SetClientsBackendsRules(
      const std::vector<std::pair<std::string, std::vector<std::string>>>& rules);
  void SetServersBackendsRules(
      const std::vector<std::pair<std::string, std::vector<std::string>>>& rules);

  void RegisterRpcBackend(RpcBackendBase* rpc_backend_ptr);

  bool RegisterServiceFunc(ServiceFuncWrapper&& service_func_wrapper);
  bool RegisterClientFunc(ClientFuncWrapper&& client_func_wrapper);
  void Invoke(ClientInvokeWrapper&& client_invoke_wrapper);

  State GetState() const { return state_.load(); }

  void SetLogger(const std::shared_ptr<common::util::LoggerWrapper>& logger_ptr) { logger_ptr_ = logger_ptr; }
  const common::util::LoggerWrapper& GetLogger() const { return *logger_ptr_; }

 private:
  std::vector<RpcBackendBase*> GetBackendsByRules(
      std::string_view func_name,
      const std::vector<std::pair<std::string, std::vector<std::string>>>& rules);

 private:
  std::atomic<State> state_ = State::PreInit;
  std::shared_ptr<common::util::LoggerWrapper> logger_ptr_;

  RpcRegistry* rpc_registry_ptr_ = nullptr;

  std::vector<RpcBackendBase*> rpc_backend_index_vec_;
  std::unordered_map<std::string_view, RpcBackendBase*> rpc_backend_index_map_;

  std::vector<std::pair<std::string, std::vector<std::string>>> clients_backends_rules_;
  std::vector<std::pair<std::string, std::vector<std::string>>> servers_backends_rules_;
  std::unordered_map<
      std::string,
      std::vector<RpcBackendBase*>,
      aimrt::common::util::StringHash,
      std::equal_to<>>
      clients_backend_index_map_;
};
}  // namespace aimrt::runtime::core::rpc