#include "mqtt_plugin/mqtt_channel_backend.h"

#include <regex>

#include "aimrt_module_cpp_interface/util/string.h"
#include "aimrt_module_cpp_interface/util/type_support.h"
#include "mqtt_plugin/global.h"
#include "util/buffer_util.h"
#include "util/url_encode.h"

namespace YAML {
template <>
struct convert<aimrt::plugins::mqtt_plugin::MqttChannelBackend::Options> {
  using Options = aimrt::plugins::mqtt_plugin::MqttChannelBackend::Options;

  static Node encode(const Options& rhs) {
    Node node;

    node["pub_topics_options"] = YAML::Node();
    for (const auto& pub_topic_options : rhs.pub_topics_options) {
      Node pub_topic_options_node;
      pub_topic_options_node["topic_name"] = pub_topic_options.topic_name;
      pub_topic_options_node["qos"] = pub_topic_options.qos;
      node["pub_topics_options"].push_back(pub_topic_options_node);
    }

    node["sub_topics_options"] = YAML::Node();
    for (const auto& sub_topic_options : rhs.sub_topics_options) {
      Node sub_topic_options_node;
      sub_topic_options_node["topic_name"] = sub_topic_options.topic_name;
      sub_topic_options_node["qos"] = sub_topic_options.qos;
      node["sub_topics_options"].push_back(sub_topic_options_node);
    }

    return node;
  }

  static bool decode(const Node& node, Options& rhs) {
    if (node["pub_topics_options"] && node["pub_topics_options"].IsSequence()) {
      for (auto& pub_topic_options_node : node["pub_topics_options"]) {
        int qos = 2;

        if (pub_topic_options_node["qos"]) qos = pub_topic_options_node["qos"].as<int>();
        if (qos < 0 || qos > 2)
          throw aimrt::common::util::AimRTException("Invalid Mqtt qos: " + std::to_string(qos));

        auto pub_topic_options = Options::PubTopicOptions{
            .topic_name = pub_topic_options_node["topic_name"].as<std::string>(),
            .qos = qos};

        rhs.pub_topics_options.emplace_back(std::move(pub_topic_options));
      }
    }

    if (node["sub_topics_options"] && node["sub_topics_options"].IsSequence()) {
      for (auto& sub_topic_options_node : node["sub_topics_options"]) {
        int qos = 2;

        if (sub_topic_options_node["qos"]) qos = sub_topic_options_node["qos"].as<int>();
        if (qos < 0 || qos > 2)
          throw aimrt::common::util::AimRTException("Invalid Mqtt qos: " + std::to_string(qos));

        auto sub_topic_options = Options::SubTopicOptions{
            .topic_name = sub_topic_options_node["topic_name"].as<std::string>(),
            .qos = qos};

        rhs.sub_topics_options.emplace_back(std::move(sub_topic_options));
      }
    }

    return true;
  }
};
}  // namespace YAML

namespace aimrt::plugins::mqtt_plugin {

void MqttChannelBackend::Initialize(
    YAML::Node options_node,
    const runtime::core::channel::ChannelRegistry* channel_registry_ptr) {
  AIMRT_CHECK_ERROR_THROW(
      std::atomic_exchange(&state_, State::Init) == State::PreInit,
      "Mqtt channel backend can only be initialized once.");

  if (options_node && !options_node.IsNull())
    options_ = options_node.as<Options>();

  channel_registry_ptr_ = channel_registry_ptr;

  options_node = options_;
}

void MqttChannelBackend::Start() {
  AIMRT_CHECK_ERROR_THROW(
      std::atomic_exchange(&state_, State::Start) == State::Init,
      "Method can only be called when state is 'Init'.");

  SubscribeMqttTopic();
}

void MqttChannelBackend::Shutdown() {
  if (std::atomic_exchange(&state_, State::Shutdown) == State::Shutdown)
    return;

  // todo:换成MQTTClient_unsubscribeMany
  for (auto sub_info : sub_info_vec_) {
    MQTTAsync_unsubscribe(client_, sub_info.topic.data(), NULL);
  }
}

bool MqttChannelBackend::RegisterPublishType(
    const runtime::core::channel::PublishTypeWrapper& publish_type_wrapper) noexcept {
  namespace util = aimrt::common::util;

  // 检查path
  std::string pattern = std::string("/channel/") +
                        util::UrlEncode(publish_type_wrapper.topic_name) + "/" +
                        util::UrlEncode(publish_type_wrapper.msg_type);

  if (pattern.size() > 255) {
    AIMRT_ERROR("Too long uri: {}", pattern);
    return false;
  }

  AIMRT_INFO("Register publish type to mqtt channel, url: {}", pattern);

  return true;
}

bool MqttChannelBackend::Subscribe(
    const runtime::core::channel::SubscribeWrapper& subscribe_wrapper) noexcept {
  if (state_.load() != State::Init) {
    AIMRT_ERROR("Msg can only be subscribed when state is 'Init'.");
    return false;
  }

  namespace util = aimrt::common::util;

  std::string_view topic_name = subscribe_wrapper.topic_name;

  int qos = 2;

  for (auto& sub_topic_options : options_.sub_topics_options) {
    try {
      if (std::regex_match(topic_name.begin(), topic_name.end(),
                           std::regex(sub_topic_options.topic_name, std::regex::ECMAScript))) {
        qos = sub_topic_options.qos;
        break;
      }
    } catch (const std::exception& e) {
      AIMRT_WARN("Regex get exception, expr: {}, string: {}, exception info: {}",
                 sub_topic_options.topic_name, topic_name, e.what());
    }
  }

  std::string pattern = std::string("/channel/") +
                        util::UrlEncode(subscribe_wrapper.topic_name) + "/" +
                        util::UrlEncode(subscribe_wrapper.msg_type);

  auto find_itr = subscribe_wrapper_map_.find(pattern);
  if (find_itr != subscribe_wrapper_map_.end()) {
    find_itr->second->emplace_back(&subscribe_wrapper);
    return true;
  }

  auto emplace_ret = subscribe_wrapper_map_.emplace(
      pattern,
      std::make_unique<std::vector<const runtime::core::channel::SubscribeWrapper*>>(
          std::vector<const runtime::core::channel::SubscribeWrapper*>{&subscribe_wrapper}));

  auto subscribe_wrapper_vec_ptr = emplace_ret.first->second.get();

  auto handle = [this, subscribe_wrapper_vec_ptr](MQTTAsync_message* message) {
    try {
      auto ctx_ptr = std::make_shared<aimrt::channel::Context>();

      util::ConstBufferOperator buf_oper(static_cast<const char*>(message->payload), message->payloadlen);

      std::string serialization_type(buf_oper.GetString(util::BufferLenType::UINT8));
      ctx_ptr->SetSerializationType(serialization_type);

      auto remaining_buf = buf_oper.GetRemainingBuffer();
      aimrt_buffer_view_t buffer_view{
          .data = remaining_buf.data(),
          .len = remaining_buf.size()};

      aimrt_buffer_array_view_t buffer_array_view{
          .data = &buffer_view,
          .len = 1};

      // 每个lib统一一次性发布。lib_name:msg_ptr
      std::unordered_map<std::string_view, std::shared_ptr<void>> msg_ptr_map;
      for (auto subscribe_wrapper_ptr : *subscribe_wrapper_vec_ptr) {
        if (msg_ptr_map.find(subscribe_wrapper_ptr->pkg_path) != msg_ptr_map.end())
          continue;

        auto subscribe_type_support_ref = aimrt::util::TypeSupportRef(subscribe_wrapper_ptr->msg_type_support);

        // 创建消息
        std::shared_ptr<void> msg_ptr = subscribe_type_support_ref.CreateSharedPtr();

        // 消息反序列化
        bool deserialize_ret = subscribe_type_support_ref.Deserialize(
            serialization_type, buffer_array_view, msg_ptr.get());

        AIMRT_CHECK_ERROR_THROW(deserialize_ret, "Mqtt msg deserialize failed.");

        msg_ptr_map.emplace(subscribe_wrapper_ptr->pkg_path, msg_ptr);
      }

      // 调用注册的subscribe方法
      for (auto subscribe_wrapper_ptr : *subscribe_wrapper_vec_ptr) {
        auto finditr = msg_ptr_map.find(subscribe_wrapper_ptr->pkg_path);
        std::shared_ptr<void> msg_ptr = finditr->second;
        aimrt::channel::SubscriberReleaseCallback release_callback([msg_ptr, ctx_ptr]() {});
        subscribe_wrapper_ptr->callback(ctx_ptr->NativeHandle(), msg_ptr.get(), release_callback.NativeHandle());
      }
    } catch (const std::exception& e) {
      AIMRT_WARN("Handle mqtt rpc msg failed, exception info: {}", e.what());
    }
  };

  msg_handle_registry_ptr_->RegisterMsgHandle(pattern, std::move(handle));

  sub_info_vec_.emplace_back(MqttSubInfo{pattern, qos});

  AIMRT_INFO("Register mqtt handle for channel, uri '{}'", pattern);

  return true;
}

void MqttChannelBackend::Publish(
    const runtime::core::channel::PublishWrapper& publish_wrapper) noexcept {
  if (state_.load() != State::Start) [[unlikely]] {
    AIMRT_WARN("Method can only be called when state is 'Start'.");
    return;
  }

  namespace util = aimrt::common::util;

  std::string_view msg_type = publish_wrapper.msg_type;
  std::string_view pkg_path = publish_wrapper.pkg_path;
  std::string_view module_name = publish_wrapper.module_name;
  std::string_view topic_name = publish_wrapper.topic_name;

  int qos = 2;

  for (auto& pub_topic_options : options_.pub_topics_options) {
    try {
      if (std::regex_match(topic_name.begin(), topic_name.end(),
                           std::regex(pub_topic_options.topic_name, std::regex::ECMAScript))) {
        qos = pub_topic_options.qos;
        break;
      }
    } catch (const std::exception& e) {
      AIMRT_WARN("Regex get exception, expr: {}, string: {}, exception info: {}",
                 pub_topic_options.topic_name, topic_name, e.what());
    }
  }

  // 确定path
  std::string mqtt_pub_topic = std::string("/channel/") +
                               util::UrlEncode(publish_wrapper.topic_name) + "/" +
                               util::UrlEncode(publish_wrapper.msg_type);

  auto publish_type_support_ref = aimrt::util::TypeSupportRef(publish_wrapper.msg_type_support);

  // 确定数据序列化类型，先找ctx，ctx中未配置则找支持的第一种序列化类型
  std::string serialization_type(publish_wrapper.ctx_ref.GetSerializationType());
  if (serialization_type.empty() && publish_type_support_ref.SerializationTypesSupportedNum() > 0) {
    serialization_type = aimrt::util::ToStdString(publish_type_support_ref.SerializationTypesSupportedList()[0]);
  }

  // msg序列化
  std::shared_ptr<aimrt::util::BufferArray> buffer_array;

  auto find_serialization_cache_itr = publish_wrapper.serialization_cache.find(serialization_type);
  if (find_serialization_cache_itr == publish_wrapper.serialization_cache.end()) {
    // 没有缓存，序列化一次后放入缓存中
    buffer_array = std::make_shared<aimrt::util::BufferArray>();
    bool serialize_ret = publish_type_support_ref.Serialize(
        serialization_type, publish_wrapper.msg_ptr, buffer_array->AllocatorNativeHandle(), buffer_array->BufferArrayNativeHandle());

    if (!serialize_ret) {
      AIMRT_ERROR(
          "Msg serialization failed in local channel, serialization_type {}, pkg_path: {}, module_name: {}, topic_name: {}, msg_type: {}",
          serialization_type, pkg_path, module_name, topic_name, msg_type);
      return;
    }

    publish_wrapper.serialization_cache.emplace(serialization_type, buffer_array);
  } else {
    // 有缓存
    buffer_array = find_serialization_cache_itr->second;
  }

  // 填内容，直接复制过去
  auto buffer_array_data = buffer_array->Data();
  const size_t buffer_array_len = buffer_array->Size();
  size_t msg_size = buffer_array->BufferSize();

  size_t mqtt_pkg_size = 1 + serialization_type.size() + msg_size;

  if (mqtt_pkg_size > max_pkg_size_) [[unlikely]] {
    AIMRT_WARN("Mqtt publish failed, pkg is too large, limit {}k, actual {}k",
               max_pkg_size_ / 1024, mqtt_pkg_size / 1024);
    return;
  }

  std::vector<char> msg_buf_vec(mqtt_pkg_size);

  util::BufferOperator buf_oper(msg_buf_vec.data(), msg_buf_vec.size());

  buf_oper.SetString(serialization_type, util::BufferLenType::UINT8);

  for (size_t ii = 0; ii < buffer_array_len; ++ii) {
    buf_oper.SetBuffer(
        static_cast<const char*>(buffer_array_data[ii].data),
        buffer_array_data[ii].len);
  }

  MQTTAsync_message pubmsg = MQTTAsync_message_initializer;
  pubmsg.payload = msg_buf_vec.data();
  pubmsg.payloadlen = msg_buf_vec.size();
  pubmsg.qos = qos;
  pubmsg.retained = 0;

  AIMRT_TRACE("Mqtt publish to '{}'", mqtt_pub_topic);
  int rc = MQTTAsync_sendMessage(client_, mqtt_pub_topic.data(), &pubmsg, NULL);
  AIMRT_CHECK_WARN(rc == MQTTASYNC_SUCCESS,
                   "publish mqtt msg failed, topic: {}, code: {}",
                   mqtt_pub_topic, rc);

  return;
}

void MqttChannelBackend::SubscribeMqttTopic() {
  for (auto sub_info : sub_info_vec_) {
    // todo:换成MQTTClient_subscribeMany
    int rc = MQTTAsync_subscribe(client_, sub_info.topic.data(), sub_info.qos, NULL);
    if (rc != MQTTASYNC_SUCCESS) {
      AIMRT_ERROR("Failed to subscribe mqtt, topic: {} return code: {}", sub_info.topic, rc);
    }
  }
}

}  // namespace aimrt::plugins::mqtt_plugin
