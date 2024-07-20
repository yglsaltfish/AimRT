#include "net_plugin/http/http_channel_backend.h"
#include "aimrt_module_cpp_interface/util/string.h"
#include "aimrt_module_cpp_interface/util/type_support.h"

#include "net_plugin/global.h"
#include "util/url_encode.h"

namespace YAML {
template <>
struct convert<aimrt::plugins::net_plugin::HttpChannelBackend::Options> {
  using Options = aimrt::plugins::net_plugin::HttpChannelBackend::Options;

  static Node encode(const Options& rhs) {
    Node node;

    node["pub_topics_options"] = YAML::Node();
    for (const auto& pub_topic_options : rhs.pub_topics_options) {
      Node pub_topic_options_node;
      pub_topic_options_node["topic_name"] = pub_topic_options.topic_name;
      pub_topic_options_node["server_url_list"] = pub_topic_options.server_url_list;
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
            .server_url_list = pub_topic_options_node["server_url_list"].as<std::vector<std::string>>()};

        rhs.pub_topics_options.emplace_back(std::move(pub_topic_options));
      }
    }

    if (node["sub_topics_options"] && node["sub_topics_options"].IsSequence()) {
      for (auto& sub_topic_options_node : node["sub_topics_options"]) {
        auto sub_topic_options = Options::SubTopicOptions{
            .topic_name = sub_topic_options_node["topic_name"].as<std::string>()};

        rhs.sub_topics_options.emplace_back(std::move(sub_topic_options));
      }
    }

    return true;
  }
};
}  // namespace YAML

namespace aimrt::plugins::net_plugin {

void HttpChannelBackend::Initialize(
    YAML::Node options_node,
    const runtime::core::channel::ChannelRegistry* channel_registry_ptr) {
  AIMRT_CHECK_ERROR_THROW(
      std::atomic_exchange(&state_, State::Init) == State::PreInit,
      "Http channel backend can only be initialized once.");

  if (options_node && !options_node.IsNull())
    options_ = options_node.as<Options>();

  channel_registry_ptr_ = channel_registry_ptr;

  options_node = options_;
}

void HttpChannelBackend::Start() {
  AIMRT_CHECK_ERROR_THROW(
      std::atomic_exchange(&state_, State::Start) == State::Init,
      "Method can only be called when state is 'Init'.");
}

void HttpChannelBackend::Shutdown() {
  if (std::atomic_exchange(&state_, State::Shutdown) == State::Shutdown)
    return;
}

bool HttpChannelBackend::RegisterPublishType(
    const runtime::core::channel::PublishTypeWrapper& publish_type_wrapper) noexcept {
  return true;
}

bool HttpChannelBackend::Subscribe(
    const runtime::core::channel::SubscribeWrapper& subscribe_wrapper) noexcept {
  if (state_.load() != State::Init) {
    AIMRT_ERROR("Msg can only be subscribed when state is 'Init'.");
    return false;
  }

  namespace asio = boost::asio;
  namespace http = boost::beast::http;
  namespace util = aimrt::common::util;

  std::string pattern = std::string("/channel/") +
                        util::UrlEncode(subscribe_wrapper.topic_name) + "/" +
                        util::UrlEncode(subscribe_wrapper.msg_type);

  auto find_itr = http_subscribe_wrapper_map_.find(pattern);
  if (find_itr != http_subscribe_wrapper_map_.end()) {
    find_itr->second->emplace_back(&subscribe_wrapper);
    return true;
  }

  auto emplace_ret = http_subscribe_wrapper_map_.emplace(
      pattern,
      std::make_unique<std::vector<const runtime::core::channel::SubscribeWrapper*>>(
          std::vector<const runtime::core::channel::SubscribeWrapper*>{&subscribe_wrapper}));

  auto subscribe_wrapper_vec_ptr = emplace_ret.first->second.get();

  runtime::common::net::AsioHttpServer::HttpHandle<http::dynamic_body> http_handle =
      [this, subscribe_wrapper_vec_ptr](
          const http::request<http::dynamic_body>& req,
          http::response<http::dynamic_body>& rsp,
          std::chrono::nanoseconds timeout)
      -> asio::awaitable<runtime::common::net::AsioHttpServer::HttpHandleStatus> {
    // 获取序列化类型
    std::string serialization_type;
    auto req_content_type_itr = req.find(http::field::content_type);
    AIMRT_CHECK_ERROR_THROW(req_content_type_itr != req.end(),
                            "Http req has no content type.");

    auto req_content_type_boost_sw = req_content_type_itr->value();
    std::string_view req_content_type(req_content_type_boost_sw.data(), req_content_type_boost_sw.size());
    if (req_content_type == "application/json" ||
        req_content_type == "application/json charset=utf-8") {
      serialization_type = "json";
      rsp.set(http::field::content_type, "application/json");
    } else if (req_content_type == "application/protobuf") {
      serialization_type = "pb";
      rsp.set(http::field::content_type, "application/protobuf");
    } else if (req_content_type == "application/ros2") {
      serialization_type = "ros2";
      rsp.set(http::field::content_type, "application/ros2");
    } else {
      AIMRT_ERROR_THROW("Http req has invalid content type {}.", req_content_type);
    }

    rsp.keep_alive(req.keep_alive());
    rsp.prepare_payload();

    // context
    auto ctx_ptr = std::make_shared<aimrt::channel::Context>();

    ctx_ptr->SetSerializationType(serialization_type);

    // 获取消息buf
    const auto& req_beast_buf = req.body().data();
    std::vector<aimrt_buffer_view_t> buffer_view_vec;

    for (auto const buf : boost::beast::buffers_range_ref(req_beast_buf)) {
      buffer_view_vec.emplace_back(aimrt_buffer_view_t{
          .data = const_cast<void*>(buf.data()),
          .len = buf.size()});
    }

    aimrt_buffer_array_view_t buffer_array_view{
        .data = buffer_view_vec.data(),
        .len = buffer_view_vec.size()};

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

      AIMRT_CHECK_ERROR_THROW(deserialize_ret, "Http req deserialize failed.");

      msg_ptr_map.emplace(subscribe_wrapper_ptr->pkg_path, msg_ptr);
    }

    // 调用注册的subscribe方法
    for (auto subscribe_wrapper_ptr : *subscribe_wrapper_vec_ptr) {
      auto finditr = msg_ptr_map.find(subscribe_wrapper_ptr->pkg_path);
      std::shared_ptr<void> msg_ptr = finditr->second;
      aimrt::channel::SubscriberReleaseCallback release_callback([msg_ptr, ctx_ptr]() {});
      subscribe_wrapper_ptr->callback(ctx_ptr->NativeHandle(), msg_ptr.get(), release_callback.NativeHandle());
    }

    co_return runtime::common::net::AsioHttpServer::HttpHandleStatus::OK;
  };

  http_svr_ptr_->RegisterHttpHandleFunc<http::dynamic_body>(
      pattern, std::move(http_handle));

  AIMRT_INFO("Register http handle for channel, uri '{}'", pattern);

  return true;
}

void HttpChannelBackend::Publish(
    const runtime::core::channel::PublishWrapper& publish_wrapper) noexcept {
  if (state_.load() != State::Start) [[unlikely]] {
    AIMRT_WARN("Method can only be called when state is 'Start'.");
    return;
  }

  namespace asio = boost::asio;
  namespace http = boost::beast::http;
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

  // http req
  auto req_ptr = std::make_shared<http::request<http::dynamic_body>>(
      http::verb::post, pattern, 11);
  req_ptr->set(http::field::user_agent, "aimrt");

  auto publish_type_support_ref = aimrt::util::TypeSupportRef(publish_wrapper.msg_type_support);

  // 确定数据序列化类型，先找ctx，ctx中未配置则找支持的第一种序列化类型
  std::string serialization_type(publish_wrapper.ctx_ref.GetSerializationType());
  if (serialization_type.empty() && publish_type_support_ref.SerializationTypesSupportedNum() > 0) {
    serialization_type = aimrt::util::ToStdString(publish_type_support_ref.SerializationTypesSupportedList()[0]);
  }

  if (serialization_type == "json") {
    req_ptr->set(http::field::content_type, "application/json");
  } else if (serialization_type == "pb") {
    req_ptr->set(http::field::content_type, "application/protobuf");
  } else if (serialization_type == "ros2") {
    req_ptr->set(http::field::content_type, "application/ros2");
  } else {
    AIMRT_WARN("Unsupport serialization type '{}'", serialization_type);
    return;
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

  // 填http req包，直接复制过去
  size_t msg_size = buffer_array->BufferSize();
  auto req_beast_buf = req_ptr->body().prepare(msg_size);

  auto data = buffer_array->BufferArrayNativeHandle()->data;
  auto buffer_array_pos = 0;
  size_t buffer_pos = 0;

  for (auto buf : boost::beast::buffers_range_ref(req_beast_buf)) {
    size_t cur_beast_buf_pos = 0;
    while (cur_beast_buf_pos < buf.size()) {
      size_t cur_beast_buffer_size = buf.size() - cur_beast_buf_pos;
      size_t cur_buffer_size = data[buffer_array_pos].len - buffer_pos;

      size_t cur_copy_size = std::min(cur_beast_buffer_size, cur_buffer_size);

      memcpy(buf.data(),
             static_cast<char*>(data[buffer_array_pos].data) + buffer_pos,
             cur_copy_size);

      buffer_pos += cur_copy_size;
      if (buffer_pos == data[buffer_array_pos].len) {
        ++buffer_array_pos;
        buffer_pos = 0;
      }

      cur_beast_buf_pos += cur_copy_size;
    }
  }
  req_ptr->body().commit(msg_size);

  req_ptr->keep_alive(true);

  for (const auto& publish_add : *server_url_list) {
    asio::co_spawn(
        *io_ptr_,
        [this, &publish_add, req_ptr]() -> asio::awaitable<void> {
          auto url_op = aimrt::common::util::ParseUrl(publish_add);
          runtime::common::net::AsioHttpClient::Options cli_options{
              .host = std::string(url_op->host),
              .service = std::string(url_op->service)};

          auto client_ptr = co_await http_cli_pool_ptr_->GetClient(cli_options);

          // todo:
          // 解决多地址发送时req设置host时的线程安全问题，除最后一个直接用指针，前几个都用值拷贝
          // host以及其他header字段使用配置进行设置，不要写死
          req_ptr->set(http::field::host, url_op->host);
          req_ptr->prepare_payload();

          auto rsp = co_await client_ptr->HttpSendRecvCo<http::dynamic_body, http::dynamic_body>(*req_ptr);

          if (rsp.result() != http::status::ok) {
            AIMRT_WARN("http channel publish get error: {} {}",
                       rsp.result_int(),
                       std::string(rsp.reason().data(), rsp.reason().size()));
          }
        },
        asio::detached);
  }

  return;
}

}  // namespace aimrt::plugins::net_plugin
