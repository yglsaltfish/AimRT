#pragma once

#include "core/rpc/rpc_backend_base.h"
#include "core/util/rpc_client_tool.h"
#include "mqtt_plugin/msg_handle_registry.h"

namespace aimrt::plugins::mqtt_plugin {

class MqttRpcBackend : public runtime::core::rpc::RpcBackendBase {
 public:
  struct Options {
    std::string timeout_executor;

    struct ClientOptions {
      std::string func_name;
      std::string server_mqtt_id;
    };
    std::vector<ClientOptions> clients_options;

    struct ServerOptions {
      std::string func_name;
      bool allow_share{true};
    };
    std::vector<ServerOptions> servers_options;
  };

 public:
  MqttRpcBackend(
      std::string client_id,
      MQTTAsync& client,
      uint32_t max_pkg_size,
      std::shared_ptr<MsgHandleRegistry> msg_handle_registry_ptr)
      : client_id_(client_id),
        client_(client),
        max_pkg_size_(max_pkg_size),
        msg_handle_registry_ptr_(msg_handle_registry_ptr) {}

  ~MqttRpcBackend() override = default;

  std::string_view Name() const override { return "mqtt"; }

  void Initialize(YAML::Node options_node,
                  const runtime::core::rpc::RpcRegistry* rpc_registry_ptr) override;
  void Start() override;
  void Shutdown() override;

  bool RegisterServiceFunc(
      const runtime::core::rpc::ServiceFuncWrapper& service_func_wrapper) noexcept override;
  bool RegisterClientFunc(
      const runtime::core::rpc::ClientFuncWrapper& client_func_wrapper) noexcept override;
  bool TryInvoke(
      const std::shared_ptr<runtime::core::rpc::ClientInvokeWrapper>& client_invoke_wrapper_ptr) noexcept override;

  void RegisterGetExecutorFunc(const std::function<executor::ExecutorRef(std::string_view)>& get_executor_func);

  void SubscribeMqttTopic();
  void UnSubscribeMqttTopic();

 private:
  static std::string_view GetRealFuncName(std::string_view func_name) {
    if (func_name.substr(0, 5) == "ros2:") return func_name.substr(5);
    if (func_name.substr(0, 3) == "pb:") return func_name.substr(3);
    return func_name;
  }

  void ReturnRspWithStatusCode(
      std::string_view mqtt_pub_topic,
      std::string_view serialization_type,
      const char* req_id_buf,
      uint32_t code);

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

  std::function<executor::ExecutorRef(std::string_view)> get_executor_func_;

  std::string client_id_;
  MQTTAsync& client_;
  uint32_t max_pkg_size_;

  std::vector<std::string> sub_info_vec_;

  std::shared_ptr<MsgHandleRegistry> msg_handle_registry_ptr_;

  std::atomic_uint32_t req_id_ = 0;

  std::map<std::string_view, std::string_view> client_func_to_server_id_;

  struct MsgRecorder {
    const runtime::core::rpc::ClientFuncWrapper* client_func_wrapper_ptr;
    std::shared_ptr<runtime::core::rpc::ClientInvokeWrapper> client_invoke_wrapper_ptr;
  };
  aimrt::runtime::core::util::RpcClientTool<MsgRecorder> client_tool_;
};

}  // namespace aimrt::plugins::mqtt_plugin