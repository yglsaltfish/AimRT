#pragma once

#include <atomic>
#include <functional>
#include <memory>
#include <string>
#include <unordered_map>
#include <vector>

#include "aimrt_module_c_interface/rpc/rpc_handle_base.h"
#include "aimrt_module_cpp_interface/executor/executor.h"
#include "core/rpc/rpc_backend_manager.h"
#include "core/rpc/rpc_handle_proxy.h"
#include "core/util/module_detail_info.h"
#include "util/log_util.h"

namespace aimrt::runtime::core::rpc {

class RpcManager {
 public:
  struct Options {
    struct BackendOptions {
      std::string type;
      YAML::Node options;
    };
    std::vector<BackendOptions> backends_options;

    struct ClientOptions {
      std::string func_name;
      std::vector<std::string> enable_backends;
    };
    std::vector<ClientOptions> clients_options;

    struct ServerOptions {
      std::string func_name;
      std::vector<std::string> enable_backends;
    };
    std::vector<ServerOptions> servers_options;
  };

  enum class State : uint32_t {
    PreInit,
    Init,
    Start,
    Shutdown,
  };

 public:
  RpcManager()
      : logger_ptr_(std::make_shared<aimrt::common::util::LoggerWrapper>()) {}
  ~RpcManager() = default;

  RpcManager(const RpcManager&) = delete;
  RpcManager& operator=(const RpcManager&) = delete;

  void Initialize(YAML::Node options_node);
  void Start();
  void Shutdown();

  void RegisterRpcBackend(std::unique_ptr<RpcBackendBase>&& rpc_backend_ptr);

  void RegisterGetExecutorFunc(
      const std::function<aimrt::executor::ExecutorRef(std::string_view)>& get_executor_func);

  const RpcHandleProxy& GetRpcHandleProxy(const util::ModuleDetailInfo& module_info);
  const RpcHandleProxy& GetRpcHandleProxy(std::string_view module_name = "core") {
    return GetRpcHandleProxy(
        util::ModuleDetailInfo{.name = std::string(module_name), .pkg_path = "core"});
  }

  // 信息查询类接口
  const RpcRegistry* GetRpcRegistry() const;
  const std::vector<std::string>& GetRpcBackendNameList() const;

  State GetState() const { return state_.load(); }

  std::list<std::pair<std::string, std::string>> GenInitializationReport() const;

  void SetLogger(const std::shared_ptr<aimrt::common::util::LoggerWrapper>& logger_ptr) { logger_ptr_ = logger_ptr; }
  const aimrt::common::util::LoggerWrapper& GetLogger() const { return *logger_ptr_; }

 private:
  void RegisterLocalRpcBackend();

 private:
  Options options_;
  std::atomic<State> state_ = State::PreInit;
  std::shared_ptr<aimrt::common::util::LoggerWrapper> logger_ptr_;

  std::function<aimrt::executor::ExecutorRef(std::string_view)> get_executor_func_;

  std::unique_ptr<RpcRegistry> rpc_registry_ptr_;

  std::vector<std::unique_ptr<RpcBackendBase>> rpc_backend_vec_;

  RpcBackendManager rpc_backend_manager_;

  std::unordered_map<std::string, std::unique_ptr<RpcHandleProxy>> rpc_handle_proxy_map_;

  // 信息查询类变量
  std::vector<std::string> rpc_backend_name_vec_;
};

}  // namespace aimrt::runtime::core::rpc
