#pragma once

#include <atomic>
#include <unordered_set>

#include "core/rpc/rpc_backend_base.h"
#include "core/util/rpc_client_tool.h"
#include "util/log_util.h"

namespace aimrt::runtime::core::rpc {

class LocalRpcBackend : public RpcBackendBase {
 public:
  struct Options {
    std::string timeout_executor;
  };

  enum class State : uint32_t {
    PreInit,
    Init,
    Start,
    Shutdown,
  };

 public:
  LocalRpcBackend()
      : logger_ptr_(std::make_shared<aimrt::common::util::LoggerWrapper>()) {}
  ~LocalRpcBackend() override = default;

  std::string_view Name() const override { return "local"; }

  void Initialize(YAML::Node options_node,
                  const RpcRegistry* rpc_registry_ptr) override;
  void Start() override;
  void Shutdown() override;

  bool RegisterServiceFunc(
      const ServiceFuncWrapper& service_func_wrapper) noexcept override;
  bool RegisterClientFunc(
      const ClientFuncWrapper& client_func_wrapper) noexcept override;
  bool TryInvoke(
      const std::shared_ptr<ClientInvokeWrapper>& client_invoke_wrapper_ptr) noexcept override;

  void RegisterGetExecutorFunc(const std::function<executor::ExecutorRef(std::string_view)>& get_executor_func);

  State GetState() const { return state_.load(); }

  void SetLogger(const std::shared_ptr<aimrt::common::util::LoggerWrapper>& logger_ptr) { logger_ptr_ = logger_ptr; }
  const aimrt::common::util::LoggerWrapper& GetLogger() const { return *logger_ptr_; }

 private:
  Options options_;
  std::atomic<State> state_ = State::PreInit;
  std::shared_ptr<aimrt::common::util::LoggerWrapper> logger_ptr_;

  const RpcRegistry* rpc_registry_ptr_ = nullptr;

  std::function<executor::ExecutorRef(std::string_view)> get_executor_func_;

  std::atomic_uint32_t req_id_ = 0;

  aimrt::runtime::core::util::RpcClientTool<
      std::shared_ptr<runtime::core::rpc::ClientInvokeWrapper>>
      client_tool_;

  using ServiceFuncIndexMap =
      std::unordered_map<
          std::string_view,  // func_name
          std::unordered_map<
              std::string_view,  // lib_path
              std::unordered_set<
                  std::string_view>>>;  // module_name
  ServiceFuncIndexMap service_func_register_index_;
};

}  // namespace aimrt::runtime::core::rpc
