#include "net_plugin/tcp/tcp_channel_backend.h"

#include <regex>

#include "aimrt_module_cpp_interface/util/string.h"
#include "aimrt_module_cpp_interface/util/type_support.h"
#include "net_plugin/global.h"
#include "util/url_encode.h"

namespace YAML {
template <>
struct convert<aimrt::plugins::net_plugin::TcpChannelBackend::Options> {
  using Options = aimrt::plugins::net_plugin::TcpChannelBackend::Options;

  static Node encode(const Options& rhs) {
    Node node;

    node["pub_topics_options"] = YAML::Node();
    for (const auto& pub_topic_options : rhs.pub_topics_options) {
      Node pub_topic_options_node;
      pub_topic_options_node["topic_name"] = pub_topic_options.topic_name;
      pub_topic_options_node["server_url_list"] = pub_topic_options.server_url_list;
      node["pub_topics_options"].push_back(pub_topic_options_node);
    }

    return node;
  }

  static bool decode(const Node& node, Options& rhs) {
    if (node["pub_topics_options"] && node["pub_topics_options"].IsSequence()) {
      for (auto& pub_topic_options_node : node["pub_topics_options"]) {
        auto pub_topic_options = Options::PubTopicOptions{
            .topic_name = pub_topic_options_node["topic_name"].as<std::string>(),
            .server_url_list = pub_topic_options_node["server_url_list"].as<std::vector<std::string>>()};

        rhs.pub_topics_options.emplace_back(std::move(pub_topic_options));
      }
    }

    return true;
  }
};
}  // namespace YAML

namespace aimrt::plugins::net_plugin {

void TcpChannelBackend::Initialize(
    YAML::Node options_node,
    const runtime::core::channel::ChannelRegistry* channel_registry_ptr) {
  AIMRT_CHECK_ERROR_THROW(
      std::atomic_exchange(&state_, State::Init) == State::PreInit,
      "Tcp channel backend can only be initialized once.");

  if (options_node && !options_node.IsNull())
    options_ = options_node.as<Options>();

  channel_registry_ptr_ = channel_registry_ptr;

  options_node = options_;
}

void TcpChannelBackend::Start() {
  AIMRT_CHECK_ERROR_THROW(
      std::atomic_exchange(&state_, State::Start) == State::Init,
      "Function can only be called when state is 'Init'.");
}

void TcpChannelBackend::Shutdown() {
  if (std::atomic_exchange(&state_, State::Shutdown) == State::Shutdown)
    return;
}

bool TcpChannelBackend::RegisterPublishType(
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

  return true;
}

bool TcpChannelBackend::Subscribe(
    const runtime::core::channel::SubscribeWrapper& subscribe_wrapper) noexcept {
  if (state_.load() != State::Init) {
    AIMRT_ERROR("Msg can only be subscribed when state is 'Init'.");
    return false;
  }

  namespace util = aimrt::common::util;

  std::string pattern = std::string("/channel/") +
                        util::UrlEncode(subscribe_wrapper.topic_name) + "/" +
                        util::UrlEncode(subscribe_wrapper.msg_type);

  auto find_itr = tcp_subscribe_wrapper_map_.find(pattern);
  if (find_itr != tcp_subscribe_wrapper_map_.end()) {
    find_itr->second->emplace_back(&subscribe_wrapper);
    return true;
  }

  auto emplace_ret = tcp_subscribe_wrapper_map_.emplace(
      pattern,
      std::make_unique<std::vector<const runtime::core::channel::SubscribeWrapper*>>(
          std::vector<const runtime::core::channel::SubscribeWrapper*>{&subscribe_wrapper}));

  auto subscribe_wrapper_vec_ptr = emplace_ret.first->second.get();

  auto handle = [subscribe_wrapper_vec_ptr](
                    const std::shared_ptr<boost::asio::streambuf>& msg_buf_ptr) {
    // 1 byte uri len : n byte uri : 1 byte serialization type len : m byte
    // serialization type : buf.len-2-n-m byte data

    const void* buf_data = msg_buf_ptr->data().data();
    size_t buf_size = msg_buf_ptr->size();
    uint8_t uri_size = static_cast<const uint8_t*>(buf_data)[0];

    // 获取序列化类型
    uint8_t serialization_type_size = static_cast<const uint8_t*>(buf_data)[1 + uri_size];
    std::string serialization_type =
        std::string(static_cast<const char*>(buf_data) + 2 + uri_size, serialization_type_size);

    // context
    auto ctx_ptr = std::make_shared<aimrt::channel::Context>();

    ctx_ptr->SetSerializationType(serialization_type);

    // 获取消息buf
    uint32_t offset = 2 + uri_size + serialization_type_size;
    const uint8_t* msg_buf = static_cast<const uint8_t*>(buf_data) + offset;
    uint32_t msg_buf_size = buf_size - offset;

    aimrt_buffer_view_t buffer_view{
        .data = msg_buf,
        .len = msg_buf_size};

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

      AIMRT_CHECK_ERROR_THROW(deserialize_ret, "Tcp msg deserialize failed.");

      msg_ptr_map.emplace(subscribe_wrapper_ptr->pkg_path, msg_ptr);
    }

    // 调用注册的subscribe方法
    for (auto subscribe_wrapper_ptr : *subscribe_wrapper_vec_ptr) {
      auto finditr = msg_ptr_map.find(subscribe_wrapper_ptr->pkg_path);
      std::shared_ptr<void> msg_ptr = finditr->second;
      aimrt::util::Function<aimrt_function_subscriber_release_callback_ops_t> release_callback(
          [msg_ptr, ctx_ptr]() {});
      subscribe_wrapper_ptr->callback(ctx_ptr->NativeHandle(), msg_ptr.get(), release_callback.NativeHandle());
    }
  };

  msg_handle_registry_ptr_->RegisterMsgHandle(pattern, std::move(handle));

  AIMRT_INFO("Register tcp handle for channel, uri '{}'", pattern);

  return true;
}

void TcpChannelBackend::Publish(
    const runtime::core::channel::PublishWrapper& publish_wrapper) noexcept {
  assert(state_.load() == State::Start);

  namespace util = aimrt::common::util;

  std::string_view msg_type = publish_wrapper.msg_type;
  std::string_view pkg_path = publish_wrapper.pkg_path;
  std::string_view module_name = publish_wrapper.module_name;
  std::string_view topic_name = publish_wrapper.topic_name;

  const std::vector<std::string>* server_url_list = nullptr;

  for (auto& pub_topic_options : options_.pub_topics_options) {
    try {
      if (std::regex_match(topic_name.begin(), topic_name.end(),
                           std::regex(pub_topic_options.topic_name, std::regex::ECMAScript))) {
        server_url_list = &(pub_topic_options.server_url_list);
        break;
      }
    } catch (const std::exception& e) {
      AIMRT_WARN("Regex get exception, expr: {}, string: {}, exception info: {}",
                 pub_topic_options.topic_name, topic_name, e.what());
    }
  }

  if (server_url_list == nullptr || server_url_list->empty()) return;

  // 确定path
  std::string pattern = std::string("/channel/") +
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
  auto msg_buf_ptr = std::make_shared<boost::asio::streambuf>();
  size_t msg_size = buffer_array->BufferSize();

  uint32_t head_size = 1 + pattern.size() + 1 + serialization_type.size();
  auto buf = msg_buf_ptr->prepare(head_size + msg_size);
  uint8_t* msg_buf = static_cast<uint8_t*>(buf.data());

  msg_buf[0] = static_cast<uint8_t>(pattern.size());
  memcpy(msg_buf + 1, pattern.c_str(), pattern.size());

  msg_buf[1 + pattern.size()] = static_cast<uint8_t>(serialization_type.size());
  memcpy(msg_buf + 2 + pattern.size(),
         serialization_type.c_str(), serialization_type.size());

  auto buffer_array_data = buffer_array->BufferArrayNativeHandle()->data;
  const size_t buffer_array_len = buffer_array->BufferArrayNativeHandle()->len;
  size_t cur_pos = 0;
  for (size_t ii = 0; ii < buffer_array_len; ++ii) {
    memcpy(msg_buf + head_size + cur_pos, buffer_array_data[ii].data, buffer_array_data[ii].len);
    cur_pos += buffer_array_data[ii].len;
  }

  msg_buf_ptr->commit(head_size + msg_size);

  for (const auto& publish_add : *server_url_list) {
    boost::asio::co_spawn(
        *io_ptr_,
        [this, publish_add, msg_buf_ptr]() -> boost::asio::awaitable<void> {
          auto v = aimrt::common::util::SplitToVec<std::string>(publish_add, ":");
          runtime::common::net::AsioTcpClient::Options client_options{
              .svr_ep = {boost::asio::ip::make_address(v[0].c_str()),
                         static_cast<uint16_t>(atoi(v[1].c_str()))}};

          auto cli = co_await tcp_cli_pool_ptr_->GetClient(client_options);

          cli->SendMsg(msg_buf_ptr);
        },
        boost::asio::detached);
  }

  return;
}

}  // namespace aimrt::plugins::net_plugin
