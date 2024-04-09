#pragma once

#include "core/rpc/rpc_backend_base.h"

#include "mqtt_plugin/msg_handle_registry.h"

#include "tbb/concurrent_hash_map.h"

namespace aimrt::plugins::mqtt_plugin {

class MqttRpcBackend : public runtime::core::rpc::RpcBackendBase {
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
  MqttRpcBackend(
      std::string client_id,
      MQTTAsync& client,
      std::shared_ptr<MsgHandleRegistry> msg_handle_registry_ptr)
      : client_id_(client_id),
        client_(client),
        msg_handle_registry_ptr_(msg_handle_registry_ptr) {}

  ~MqttRpcBackend() override = default;

  std::string_view Name() const override { return "mqtt"; }

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

  void SubscribeMqttTopic();

 private:
  static std::string_view GetRealFuncName(std::string_view func_name) {
    if (func_name.substr(0, 5) == "ros2:") return func_name.substr(5);

    if (func_name.substr(0, 3) == "pb:") return func_name.substr(3);

    return func_name;
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

  std::string client_id_;
  MQTTAsync& client_;
  std::shared_ptr<MsgHandleRegistry> msg_handle_registry_ptr_;

  std::vector<std::string> sub_info_vec_;

  std::atomic_uint32_t req_id_ = 0;

  struct MsgRecorder {
    const runtime::core::rpc::ClientFuncWrapper* client_func_wrapper_ptr;
    std::shared_ptr<runtime::core::rpc::ClientInvokeWrapper> client_invoke_wrapper_ptr;
  };
  using ClientMsgRecorderMap = tbb::concurrent_hash_map<uint32_t, MsgRecorder>;
  ClientMsgRecorderMap client_msg_recorder_map_;
};

}  // namespace aimrt::plugins::mqtt_plugin