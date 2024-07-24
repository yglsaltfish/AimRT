#include "net_plugin/tcp/tcp_channel_backend.h"

#include <regex>

#include "aimrt_module_cpp_interface/util/string.h"
#include "aimrt_module_cpp_interface/util/type_support.h"
#include "net_plugin/global.h"
#include "util/buffer_util.h"
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
      "Method can only be called when state is 'Init'.");
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

  std::string_view topic_name = subscribe_wrapper.topic_name;

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

  auto handle = [this, topic_name, subscribe_wrapper_vec_ptr](
                    const std::shared_ptr<boost::asio::streambuf>& msg_buf_ptr) {
    auto ctx_ptr = std::make_shared<aimrt::channel::Context>(aimrt_channel_context_type_t::AIMRT_RPC_SUBSCRIBER_CONTEXT);
    ctx_ptr->SetMetaValue(AIMRT_CHANNEL_CONTEXT_TOPIC_NAME, topic_name);
    ctx_ptr->SetMetaValue(AIMRT_CHANNEL_CONTEXT_KEY_BACKEND, Name());

    util::ConstBufferOperator buf_oper(
        static_cast<const char*>(msg_buf_ptr->data().data()),
        msg_buf_ptr->size());

    auto pattern = buf_oper.GetString(util::BufferLenType::UINT8);

    std::string serialization_type(buf_oper.GetString(util::BufferLenType::UINT8));
    ctx_ptr->SetSerializationType(serialization_type);

    // 获取context
    size_t ctx_num = buf_oper.GetUint8();
    for (size_t ii = 0; ii < ctx_num; ++ii) {
      auto key = buf_oper.GetString(util::BufferLenType::UINT16);
      auto val = buf_oper.GetString(util::BufferLenType::UINT16);
      ctx_ptr->SetMetaValue(key, val);
    }

    // 获取消息buf
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

      AIMRT_CHECK_ERROR_THROW(deserialize_ret, "Tcp msg deserialize failed.");

      msg_ptr_map.emplace(subscribe_wrapper_ptr->pkg_path, msg_ptr);
    }

    // 调用注册的subscribe方法
    for (auto subscribe_wrapper_ptr : *subscribe_wrapper_vec_ptr) {
      auto finditr = msg_ptr_map.find(subscribe_wrapper_ptr->pkg_path);
      std::shared_ptr<void> msg_ptr = finditr->second;
      subscribe_wrapper_ptr->callback(ctx_ptr, msg_ptr.get(), [msg_ptr, ctx_ptr]() {});
    }
  };

  msg_handle_registry_ptr_->RegisterMsgHandle(pattern, std::move(handle));

  AIMRT_INFO("Register tcp handle for channel, uri '{}'", pattern);

  return true;
}

void TcpChannelBackend::Publish(
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

  // context
  const auto& keys = publish_wrapper.ctx_ref.GetMetaKeys();
  if (keys.size() > 255) [[unlikely]] {
    AIMRT_WARN("Too much context meta, require less than 255, but actually {}.", keys.size());
    return;
  }

  std::vector<std::string_view> context_meta_kv;
  size_t context_meta_kv_size = 1;
  for (const auto& key : keys) {
    context_meta_kv_size += (2 + key.size());
    context_meta_kv.emplace_back(key);

    auto val = publish_wrapper.ctx_ref.GetMetaValue(key);
    context_meta_kv_size += (2 + val.size());
    context_meta_kv.emplace_back(val);
  }

  // 填内容，直接复制过去
  auto msg_buf_ptr = std::make_shared<boost::asio::streambuf>();

  auto buffer_array_data = buffer_array->Data();
  const size_t buffer_array_len = buffer_array->Size();
  size_t msg_size = buffer_array->BufferSize();

  uint32_t pkg_size = 1 + pattern.size() +
                      1 + serialization_type.size() +
                      context_meta_kv_size +
                      msg_size;

  auto buf = msg_buf_ptr->prepare(pkg_size);
  util::BufferOperator buf_oper(static_cast<char*>(buf.data()), buf.size());

  buf_oper.SetString(pattern, util::BufferLenType::UINT8);
  buf_oper.SetString(serialization_type, util::BufferLenType::UINT8);

  buf_oper.SetUint8(static_cast<uint8_t>(keys.size()));
  for (const auto& s : context_meta_kv) {
    buf_oper.SetString(s, util::BufferLenType::UINT16);
  }

  // data
  for (size_t ii = 0; ii < buffer_array_len; ++ii) {
    buf_oper.SetBuffer(
        static_cast<const char*>(buffer_array_data[ii].data),
        buffer_array_data[ii].len);
  }

  msg_buf_ptr->commit(pkg_size);

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
