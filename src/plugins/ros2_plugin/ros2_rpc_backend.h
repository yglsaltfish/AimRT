#pragma once

#include "core/rpc/rpc_backend_base.h"

#include "ros2_plugin/ros2_adapter_rpc_client.h"
#include "ros2_plugin/ros2_adapter_rpc_server.h"

#include "rclcpp/rclcpp.hpp"

namespace aimrt::plugins::ros2_plugin {

class Ros2RpcBackend : public runtime::core::rpc::RpcBackendBase {
 public:
  struct Options {
    struct ClientOptions {
      std::string func_name;
    };
    std::vector<ClientOptions> clients_options;

    struct ServerOptions {
      std::string func_name;
    };
    std::vector<ServerOptions> servers_options;
  };

 public:
  Ros2RpcBackend() = default;
  ~Ros2RpcBackend() override = default;

  std::string_view Name() const override { return "ros2"; }

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

  void SetNodePtr(const std::shared_ptr<rclcpp::Node>& ros2_node_ptr) {
    ros2_node_ptr_ = ros2_node_ptr;
  }

 private:
  static bool CheckRosFunc(std::string_view func_name) {
    return (func_name.substr(0, 5) == "ros2:");
  }

  std::string GetRealRosFuncName(std::string_view func_name) {
    return rclcpp::extend_name_with_sub_namespace(
        std::string(func_name.substr(5)), ros2_node_ptr_->get_sub_namespace());
  }

 private:
  enum class State : uint32_t {
    PreInit,
    Init,
    Start,
    Shutdown,
  };

  Options options_;
  std::atomic<State> state_ = State::PreInit;

  const runtime::core::rpc::RpcRegistry* rpc_registry_ptr_ = nullptr;
  runtime::core::rpc::ContextManager* context_manager_ptr_ = nullptr;

  std::shared_ptr<rclcpp::Node> ros2_node_ptr_;

  std::unordered_map<std::string_view, std::shared_ptr<Ros2AdapterServer>> ros2_adapter_server_map_;
  std::unordered_map<std::string_view, std::shared_ptr<Ros2AdapterClient>> ros2_adapter_client_map_;
};

}  // namespace aimrt::plugins::ros2_plugin
