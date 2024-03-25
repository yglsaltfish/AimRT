#include "mqtt_plugin/mqtt_plugin.h"

#include "core/aimrt_core.h"
#include "mqtt_plugin/global.h"

namespace YAML {
template <>
struct convert<aimrt::plugins::mqtt_plugin::MqttPlugin::Options> {
  using Options = aimrt::plugins::mqtt_plugin::MqttPlugin::Options;

  static Node encode(const Options &rhs) {
    Node node;

    node["broker_addr"] = rhs.broker_addr;
    node["client_id"] = rhs.client_id;

    return node;
  }

  static bool decode(const Node &node, Options &rhs) {
    if (!node.IsMap()) return false;

    rhs.broker_addr = node["broker_addr"].as<std::string>();
    rhs.client_id = node["client_id"].as<std::string>();

    return true;
  }
};
}  // namespace YAML

namespace aimrt::plugins::mqtt_plugin {

bool MqttPlugin::Initialize(runtime::core::AimRTCore *core_ptr) noexcept {
  try {
    core_ptr_ = core_ptr;

    YAML::Node plugin_options_node = core_ptr_->GetPluginManager().GetPluginOptionsNode(Name());

    if (plugin_options_node && !plugin_options_node.IsNull()) {
      options_ = plugin_options_node.as<Options>();
    }

    init_flag_ = true;

    // 初始化mqtt
    int rc = MQTTClient_create(
        &client_, options_.broker_addr.c_str(), options_.client_id.c_str(), MQTTCLIENT_PERSISTENCE_NONE, NULL);
    AIMRT_CHECK_ERROR_THROW(rc == MQTTCLIENT_SUCCESS, "Failed to create mqtt client, return code: {}", rc);

    rc = MQTTClient_setCallbacks(
        client_,
        this,
        [](void *context, char *cause) {
          static_cast<MqttPlugin *>(context)->OnConnectLost(cause);
        },
        [](void *context, char *topicName, int topicLen, MQTTClient_message *message) -> int {
          return static_cast<MqttPlugin *>(context)->OnMsgRecv(topicName, topicLen, message);
        },
        NULL);
    AIMRT_CHECK_ERROR_THROW(rc == MQTTCLIENT_SUCCESS, "Failed to set callbacks for mqtt client, return code: {}", rc);

    MQTTClient_connectOptions conn_opts = MQTTClient_connectOptions_initializer;
    conn_opts.keepAliveInterval = 20;
    conn_opts.cleansession = 1;
    rc = MQTTClient_connect(client_, &conn_opts);
    AIMRT_CHECK_ERROR_THROW(rc == MQTTCLIENT_SUCCESS, "Failed to connect mqtt broker, return code: {}", rc);
    connect_flag_.store(true);

    reconnect_thread_ptr_ = std::make_shared<std::thread>([this]() {
      while (!stop_flag_.load()) {
        if (!connect_flag_.load()) {
          MQTTClient_connectOptions conn_opts = MQTTClient_connectOptions_initializer;
          conn_opts.keepAliveInterval = 20;
          conn_opts.cleansession = 1;

          AIMRT_TRACE("Try reconnect to mqtt broker.");
          int rc = MQTTClient_connect(client_, &conn_opts);
          if (rc == MQTTCLIENT_SUCCESS) {
            AIMRT_TRACE("Reconnect to mqtt broker success.");

            for (auto ptr : mqtt_channel_backend_ptr_vec_) {
              ptr->SubscribeMqttTopic();
            }

            for (auto ptr : mqtt_rpc_backend_ptr_vec_) {
              ptr->SubscribeMqttTopic();
            }

            connect_flag_.store(true);
          } else {
            AIMRT_WARN("Failed to connect mqtt broker, return code {}", rc);
          }
        }
        std::this_thread::sleep_for(std::chrono::milliseconds(3000));
      }
    });

    msg_handle_registry_ptr_ = std::make_shared<MsgHandleRegistry>();

    // 注册hook函数
    core_ptr_->RegisterHookFunc(runtime::core::AimRTCore::State::PostInitLog,
                                [this] { SetPluginLogger(); });

    core_ptr_->RegisterHookFunc(runtime::core::AimRTCore::State::PreInitRpc,
                                [this] { RegisterMqttRpcBackend(); });

    core_ptr_->RegisterHookFunc(runtime::core::AimRTCore::State::PreInitChannel,
                                [this] { RegisterMqttChannelBackend(); });

    plugin_options_node = options_;
    return true;
  } catch (const std::exception &e) {
    AIMRT_ERROR("Initialize failed, {}", e.what());
  }

  return false;
}

void MqttPlugin::Shutdown() noexcept {
  try {
    if (!init_flag_) return;

    stop_flag_ = true;

    reconnect_thread_ptr_->join();

    MQTTClient_disconnect(client_, 10000);

    MQTTClient_destroy(&client_);

  } catch (const std::exception &e) {
    AIMRT_ERROR("Shutdown failed, {}", e.what());
  }
}

void MqttPlugin::SetPluginLogger() {
  SetLogger(aimrt::logger::LoggerRef(core_ptr_->GetLoggerManager().GetLoggerProxy(Name()).NativeHandle()));
}

void MqttPlugin::RegisterMqttChannelBackend() {
  std::unique_ptr<runtime::core::channel::ChannelBackendBase> mqtt_channel_backend_ptr =
      std::make_unique<MqttChannelBackend>(client_, msg_handle_registry_ptr_);

  mqtt_channel_backend_ptr_vec_.emplace_back(
      static_cast<MqttChannelBackend *>(mqtt_channel_backend_ptr.get()));

  core_ptr_->GetChannelManager().RegisterChannelBackend(std::move(mqtt_channel_backend_ptr));
}

void MqttPlugin::RegisterMqttRpcBackend() {
  std::unique_ptr<runtime::core::rpc::RpcBackendBase> mqtt_rpc_backend_ptr =
      std::make_unique<MqttRpcBackend>(options_.client_id, client_, msg_handle_registry_ptr_);

  mqtt_rpc_backend_ptr_vec_.emplace_back(
      static_cast<MqttRpcBackend *>(mqtt_rpc_backend_ptr.get()));

  core_ptr_->GetRpcManager().RegisterRpcBackend(std::move(mqtt_rpc_backend_ptr));
}

void MqttPlugin::OnConnectLost(char *cause) {
  AIMRT_WARN("Lost connect to mqtt broker, cause {}", (cause == nullptr) ? "nil" : cause);
  connect_flag_.store(false);
}

int MqttPlugin::OnMsgRecv(char *topic, int topic_len, MQTTClient_message *message) {
  std::string_view topic_str = topic_len ? std::string_view(topic, topic_len) : std::string_view(topic);
  msg_handle_registry_ptr_->HandleServerMsg(topic_str, message);
  MQTTClient_freeMessage(&message);
  MQTTClient_free(topic);
  return 1;
}

}  // namespace aimrt::plugins::mqtt_plugin
