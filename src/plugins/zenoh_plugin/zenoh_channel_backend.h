#pragma once

#include "core/channel/channel_backend_base.h"
#include "zenoh_plugin/msg_handle_registry.h"

namespace aimrt::plugins::zenoh_plugin {

class ZenohChannelBackend : public runtime::core::channel::ChannelBackendBase {
 public:
  // 后端配置信息对应的option （topic name相同topic name双方才可以通信）
  struct Options {
    struct PubTopicOptions {
      std::string topic_name;
    };

    std::vector<PubTopicOptions> pub_topics_options;

    struct SubTopicOptions {
      std::string topic_name;
    };

    std::vector<SubTopicOptions> sub_topics_options;
  };

 public:
  ZenohChannelBackend(
      std::shared_ptr<z_owned_subscriber_t>& sub,
      std::shared_ptr<z_owned_publisher_t>& pub,
      std::shared_ptr<MsgHandleRegistry> msg_handle_registry_ptr)
      : sub_(sub),
        pub_(pub),
        msg_handle_registry_ptr_(msg_handle_registry_ptr) {}

  ~ZenohChannelBackend() override = default;

  std::string_view Name() const override { return "zenoh"; }

  void Initialize(YAML::Node options_node) override;
  void Start() override;
  void Shutdown() override;

  void SetChannelRegistry(const runtime::core::channel::ChannelRegistry* channel_registry_ptr) override {
    channel_registry_ptr_ = channel_registry_ptr;
  }

  bool RegisterPublishType(
      const runtime::core::channel::PublishTypeWrapper& publish_type_wrapper) noexcept override;
  bool Subscribe(const runtime::core::channel::SubscribeWrapper& subscribe_wrapper) noexcept override;
  void Publish(runtime::core::channel::MsgWrapper& msg_wrapper) noexcept override;

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

  // 通过这两个结构体指针实现调用zenoh的接口
  const std::shared_ptr<z_owned_subscriber_t>& sub_;
  const std::shared_ptr<z_owned_publisher_t>& pub_;

  std::shared_ptr<MsgHandleRegistry> msg_handle_registry_ptr_;

  struct ZenohSubInfo {
    std::string topic;
  };
  std::vector<ZenohSubInfo> sub_info_vec_;

  std::unordered_map<
      std::string,
      std::unique_ptr<std::vector<const runtime::core::channel::SubscribeWrapper*>>>
      subscribe_wrapper_map_;

  struct PubCfgInfo {
  };
  std::unordered_map<std::string_view, PubCfgInfo> pub_cfg_info_map_;
};

}  // namespace aimrt::plugins::zenoh_plugin