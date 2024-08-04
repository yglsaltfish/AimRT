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

void TcpChannelBackend::Initialize(YAML::Node options_node) {
  AIMRT_CHECK_ERROR_THROW(
      std::atomic_exchange(&state_, State::Init) == State::PreInit,
      "Tcp channel backend can only be initialized once.");

  if (options_node && !options_node.IsNull())
    options_ = options_node.as<Options>();

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

  const auto& info = publish_type_wrapper.info;

  std::vector<std::string> server_url_list;

  auto find_option_itr = std::find_if(
      options_.pub_topics_options.begin(), options_.pub_topics_options.end(),
      [topic_name = info.topic_name](const Options::PubTopicOptions& pub_option) {
        try {
          return std::regex_match(topic_name.begin(), topic_name.end(), std::regex(pub_option.topic_name, std::regex::ECMAScript));
        } catch (const std::exception& e) {
          AIMRT_WARN("Regex get exception, expr: {}, string: {}, exception info: {}",
                     pub_option.topic_name, topic_name, e.what());
          return false;
        }
      });

  if (find_option_itr != options_.pub_topics_options.end()) {
    server_url_list = find_option_itr->server_url_list;
  }

  std::vector<boost::asio::ip::tcp::endpoint> server_ep_vec;
  for (const auto& publish_add : server_url_list) {
    auto v = util::SplitToVec<std::string>(publish_add, ":");
    boost::asio::ip::tcp::endpoint ep{
        boost::asio::ip::make_address(v[0].c_str()),
        static_cast<uint16_t>(atoi(v[1].c_str()))};

    server_ep_vec.emplace_back(ep);
  }

  pub_cfg_info_map_.emplace(
      info.topic_name,
      PubCfgInfo{
          .server_ep_vec = std::move(server_ep_vec)});

  // 检查path
  std::string pattern = std::string("/channel/") +
                        util::UrlEncode(info.topic_name) + "/" +
                        util::UrlEncode(info.msg_type);

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

  const auto& info = subscribe_wrapper.info;

  std::string pattern = std::string("/channel/") +
                        util::UrlEncode(info.topic_name) + "/" +
                        util::UrlEncode(info.msg_type);

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

  auto handle = [this, topic_name = info.topic_name, subscribe_wrapper_vec_ptr](
                    const std::shared_ptr<boost::asio::streambuf>& msg_buf_ptr) {
    auto ctx_ptr = std::make_shared<aimrt::channel::Context>(aimrt_channel_context_type_t::AIMRT_RPC_SUBSCRIBER_CONTEXT);

    // 解析msg buf
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

    ctx_ptr->SetMetaValue(AIMRT_CHANNEL_CONTEXT_TOPIC_NAME, topic_name);
    ctx_ptr->SetMetaValue(AIMRT_CHANNEL_CONTEXT_KEY_BACKEND, Name());

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
      if (msg_ptr_map.find(subscribe_wrapper_ptr->info.pkg_path) != msg_ptr_map.end())
        continue;

      auto subscribe_type_support_ref = subscribe_wrapper_ptr->info.msg_type_support_ref;

      // 创建消息
      std::shared_ptr<void> msg_ptr = subscribe_type_support_ref.CreateSharedPtr();

      // 消息反序列化
      bool deserialize_ret = subscribe_type_support_ref.Deserialize(
          serialization_type, buffer_array_view, msg_ptr.get());

      AIMRT_CHECK_ERROR_THROW(deserialize_ret, "Tcp msg deserialize failed.");

      msg_ptr_map.emplace(subscribe_wrapper_ptr->info.pkg_path, msg_ptr);
    }

    // 调用注册的subscribe方法
    for (auto subscribe_wrapper_ptr : *subscribe_wrapper_vec_ptr) {
      auto finditr = msg_ptr_map.find(subscribe_wrapper_ptr->info.pkg_path);
      std::shared_ptr<void> msg_ptr = finditr->second;

      // 创建 sub msg wrapper
      runtime::core::channel::MsgWrapper sub_msg_wrapper{
          .info = subscribe_wrapper_ptr->info,
          .msg_ptr = msg_ptr.get(),
          .ctx_ref = ctx_ptr};

      subscribe_wrapper_ptr->callback(sub_msg_wrapper, [msg_ptr, ctx_ptr]() {});
    }
  };

  msg_handle_registry_ptr_->RegisterMsgHandle(pattern, std::move(handle));

  AIMRT_INFO("Register tcp handle for channel, uri '{}'", pattern);

  return true;
}

void TcpChannelBackend::Publish(runtime::core::channel::MsgWrapper& msg_wrapper) noexcept {
  if (state_.load() != State::Start) [[unlikely]] {
    AIMRT_WARN("Method can only be called when state is 'Start'.");
    return;
  }

  namespace util = aimrt::common::util;

  const auto& info = msg_wrapper.info;

  auto find_itr = pub_cfg_info_map_.find(info.topic_name);
  if (find_itr == pub_cfg_info_map_.end() || find_itr->second.server_ep_vec.empty()) {
    AIMRT_WARN("Server url list is empty for topic '{}'", info.topic_name);
    return;
  }

  const auto& server_ep_vec = find_itr->second.server_ep_vec;

  // 确定数据序列化类型，先找ctx，ctx中未配置则找支持的第一种序列化类型
  auto publish_type_support_ref = info.msg_type_support_ref;

  auto serialization_type = msg_wrapper.ctx_ref.GetSerializationType();
  if (serialization_type.empty()) {
    serialization_type = aimrt::util::ToStdString(publish_type_support_ref.SerializationTypesSupportedList()[0]);
  }

  // msg序列化
  auto buffer_array_view_ptr = msg_wrapper.SerializeMsgWithCache(serialization_type);
  if (!buffer_array_view_ptr) [[unlikely]] {
    AIMRT_ERROR(
        "Msg serialization failed, serialization_type {}, pkg_path: {}, module_name: {}, topic_name: {}, msg_type: {}",
        serialization_type, info.pkg_path, info.module_name, info.topic_name, info.msg_type);
    return;
  }

  // context
  const auto& keys = msg_wrapper.ctx_ref.GetMetaKeys();
  if (keys.size() > 255) [[unlikely]] {
    AIMRT_WARN("Too much context meta, require less than 255, but actually {}.", keys.size());
    return;
  }

  std::vector<std::string_view> context_meta_kv;
  size_t context_meta_kv_size = 1;
  for (const auto& key : keys) {
    context_meta_kv_size += (2 + key.size());
    context_meta_kv.emplace_back(key);

    auto val = msg_wrapper.ctx_ref.GetMetaValue(key);
    context_meta_kv_size += (2 + val.size());
    context_meta_kv.emplace_back(val);
  }

  // 确定path
  std::string pattern = std::string("/channel/") +
                        util::UrlEncode(info.topic_name) + "/" +
                        util::UrlEncode(info.msg_type);

  // 填内容，直接复制过去
  auto msg_buf_ptr = std::make_shared<boost::asio::streambuf>();

  auto buffer_array_data = buffer_array_view_ptr->Data();
  const size_t buffer_array_len = buffer_array_view_ptr->Size();
  size_t msg_size = buffer_array_view_ptr->BufferSize();

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

  for (const auto& server_ep : server_ep_vec) {
    boost::asio::co_spawn(
        *io_ptr_,
        [this, server_ep, msg_buf_ptr]() -> boost::asio::awaitable<void> {
          runtime::common::net::AsioTcpClient::Options client_options{
              .svr_ep = server_ep};

          auto cli = co_await tcp_cli_pool_ptr_->GetClient(client_options);

          cli->SendMsg(msg_buf_ptr);
        },
        boost::asio::detached);
  }

  return;
}

}  // namespace aimrt::plugins::net_plugin
