#include "zenoh_channel_backend.h"

namespace YAML {
template <>
struct convert<aimrt::plugins::zenoh_plugin::ZenohChannelBackend::Options> {
  using Options = aimrt::plugins::zenoh_plugin::ZenohChannelBackend::Options;

  static Node encode(const Options& rhs) {
    Node node;

    node["pub_topics_options"] = YAML::Node();
    for (const auto& pub_topic_options : rhs.pub_topics_options) {
      Node pub_topic_options_node;
      pub_topic_options_node["topic_name"] = pub_topic_options.topic_name;

      node["pub_topics_options"].push_back(pub_topic_options_node);
    }

    node["sub_topics_options"] = YAML::Node();
    for (const auto& sub_topic_options : rhs.sub_topics_options) {
      Node sub_topic_options_node;
      sub_topic_options_node["topic_name"] = sub_topic_options.topic_name;

      node["sub_topics_options"].push_back(sub_topic_options_node);
    }

    return node;
  }

  static bool decode(const Node& node, Options& rhs) {
    if (node["pub_topics_options"] && node["pub_topics_options"].IsSequence()) {
      for (auto& pub_topic_options_node : node["pub_topics_options"]) {
        auto pub_topic_options = Options::PubTopicOptions{
            .topic_name = pub_topic_options_node["topic_name"].as<std::string>(),
        };

        rhs.pub_topics_options.emplace_back(std::move(pub_topic_options));
      }
    }

    if (node["sub_topics_options"] && node["sub_topics_options"].IsSequence()) {
      for (auto& sub_topic_options_node : node["sub_topics_options"]) {
        auto sub_topic_options = Options::SubTopicOptions{
            .topic_name = sub_topic_options_node["topic_name"].as<std::string>(),
        };

        rhs.sub_topics_options.emplace_back(std::move(sub_topic_options));
      }
    }

    return true;
  }
};
}  // namespace YAML

namespace aimrt::plugins::zenoh_plugin {

// 后端配置文件初始化
void ZenohChannelBackend::Initialize(YAML::Node option_node) {
  AIMRT_CHECK_ERROR_THROW(
      std::atomic_exchange(&state_, State::Init) == State::PreInit,
      "Zenoh channel backend can only be initialized once.");
  /*
  ...
  */
}

void ZenohChannelBackend::Start() {
  AIMRT_CHECK_ERROR_THROW(
      std::atomic_exchange(&state_, State::Start) == State::Init,
      "Method can only be called when state is 'Init'.");
  /*
  ...
  */
}

// 释放资源
void ZenohChannelBackend::Shutdown() {
  if (std::atomic_exchange(&state_, State::Shutdown) == State::Shutdown)
    return;
  /*
  ...
  */
}
// 根据传入的发布者，解析其类型等信息并存放到map中便于查询和管理
bool ZenohChannelBackend::RegisterPublishType(
    const runtime::core::channel::PublishTypeWrapper& publish_type_wrapper) noexcept {
  if (state_.load() != State::Init) {
    AIMRT_ERROR("Method can only be called when state is 'Init'.");
    return false;
  }
  /*
  ...
  */
  return true;
}

// 根据传入的订阅者，解析器类型等信息并存放到map中便于查询和管理，除此之外注册与主题匹配的函数回调
bool ZenohChannelBackend::Subscribe(
    const runtime::core::channel::SubscribeWrapper& subscriber_wrapper) noexcept {
  if (state_.load() != State::Init) {
    AIMRT_ERROR("Msg can only be subscribed when state is 'Init'.");
    return false;
  }
  /*
  ...
  */
  return true;
}

// 将消息按指定格式序列化后调用底层api将数据发布
void ZenohChannelBackend::Publish(runtime::core::channel::MsgWrapper& msg_wrapper) noexcept {
  if (state_.load() != State::Start) [[unlikely]] {
    AIMRT_WARN("Method can only be called when state is 'Start'.");
    return;
  }
  /*
  ...
  */
}

}  // namespace aimrt::plugins::zenoh_plugin