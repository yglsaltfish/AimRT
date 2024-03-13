#pragma once

#include <set>

#include "core/channel/channel_backend_base.h"

#include "mqtt_plugin/msg_handle_registry.h"

namespace aimrt::plugins::mqtt_plugin {

class MqttChannelBackend : public runtime::core::channel::ChannelBackendBase {
 public:
  struct Options {
    struct PubTopicOptions {
      std::string topic_name;
      int qos = 2;
      bool enable;
    };

    std::vector<PubTopicOptions> pub_topics_options;

    struct SubTopicOptions {
      std::string topic_name;
      int qos = 2;
    };

    std::vector<SubTopicOptions> sub_topics_options;
  };

 public:
  MqttChannelBackend(
      MQTTClient& client,
      std::shared_ptr<MsgHandleRegistry> msg_handle_registry_ptr)
      : client_(client),
        msg_handle_registry_ptr_(msg_handle_registry_ptr) {}

  ~MqttChannelBackend() override = default;

  std::string_view Name() const override { return "mqtt"; }

  void Initialize(YAML::Node options_node,
                  const runtime::core::channel::ChannelRegistry* channel_registry_ptr,
                  runtime::core::channel::ContextManager* context_manager_ptr) override;
  void Start() override;
  void Shutdown() override;

  bool RegisterPublishType(
      const runtime::core::channel::PublishTypeWrapper& publish_type_wrapper) noexcept override;
  bool Subscribe(const runtime::core::channel::SubscribeWrapper& subscribe_wrapper) noexcept override;
  void Publish(const runtime::core::channel::PublishWrapper& publish_wrapper) noexcept override;

  void SubscribeMqttTopic();

 private:
  enum class State : uint32_t {
    PreInit,
    Init,
    Start,
    Shutdown,
  };

  Options options_;
  std::atomic<State> state_ = State::PreInit;

  const runtime::core::channel::ChannelRegistry* channel_registry_ptr_ = nullptr;
  runtime::core::channel::ContextManager* context_manager_ptr_ = nullptr;

  MQTTClient& client_;
  std::shared_ptr<MsgHandleRegistry> msg_handle_registry_ptr_;

  struct MqttSubInfo {
    std::string topic;
    int qos;
  };
  std::vector<MqttSubInfo> sub_info_vec_;

  std::unordered_map<std::string,
                     std::unique_ptr<std::vector<const runtime::core::channel::SubscribeWrapper*>>>
      subscribe_wrapper_map_;
};

}  // namespace aimrt::plugins::mqtt_plugin