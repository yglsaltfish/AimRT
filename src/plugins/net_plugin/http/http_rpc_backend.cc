#include "net_plugin/http/http_rpc_backend.h"

#include <regex>

#include "aimrt_module_cpp_interface/rpc/rpc_status.h"
#include "aimrt_module_cpp_interface/util/buffer.h"
#include "aimrt_module_cpp_interface/util/type_support.h"
#include "net_plugin/global.h"

namespace YAML {
template <>
struct convert<aimrt::plugins::net_plugin::HttpRpcBackend::Options> {
  using Options = aimrt::plugins::net_plugin::HttpRpcBackend::Options;

  static Node encode(const Options& rhs) {
    Node node;

    node["clients_options"] = YAML::Node();
    for (const auto& client_options : rhs.clients_options) {
      Node client_options_node;
      client_options_node["func_name"] = client_options.func_name;
      client_options_node["server_url"] = client_options.server_url;
      node["clients_options"].push_back(client_options_node);
    }

    node["servers_options"] = YAML::Node();
    for (const auto& server_options : rhs.servers_options) {
      Node server_options_node;
      server_options_node["func_name"] = server_options.func_name;
      node["servers_options"].push_back(server_options_node);
    }

    return node;
  }

  static bool decode(const Node& node, Options& rhs) {
    if (node["clients_options"] && node["clients_options"].IsSequence()) {
      for (auto& client_options_node : node["clients_options"]) {
        auto client_options = Options::ClientOptions{
            .func_name = client_options_node["func_name"].as<std::string>(),
            .server_url = client_options_node["server_url"].as<std::string>()};

        rhs.clients_options.emplace_back(std::move(client_options));
      }
    }

    if (node["servers_options"] && node["servers_options"].IsSequence()) {
      for (auto& server_options_node : node["servers_options"]) {
        auto server_options = Options::ServerOptions{
            .func_name = server_options_node["func_name"].as<std::string>()};

        rhs.servers_options.emplace_back(std::move(server_options));
      }
    }

    return true;
  }
};
}  // namespace YAML

namespace aimrt::plugins::net_plugin {

void HttpRpcBackend::Initialize(YAML::Node options_node,
                                const runtime::core::rpc::RpcRegistry* rpc_registry_ptr) {
  AIMRT_CHECK_ERROR_THROW(
      std::atomic_exchange(&state_, State::Init) == State::PreInit,
      "Http Rpc backend can only be initialized once.");

  if (options_node && !options_node.IsNull())
    options_ = options_node.as<Options>();

  rpc_registry_ptr_ = rpc_registry_ptr;

  options_node = options_;
}

void HttpRpcBackend::Start() {
  AIMRT_CHECK_ERROR_THROW(
      std::atomic_exchange(&state_, State::Start) == State::Init,
      "Method can only be called when state is 'Init'.");
}

void HttpRpcBackend::Shutdown() {
  if (std::atomic_exchange(&state_, State::Shutdown) == State::Shutdown)
    return;
}

bool HttpRpcBackend::RegisterServiceFunc(
    const runtime::core::rpc::ServiceFuncWrapper& service_func_wrapper) noexcept {
  if (state_.load() != State::Init) {
    AIMRT_ERROR("Service func can only be registered when state is 'Init'.");
    return false;
  }

  namespace asio = boost::asio;
  namespace http = boost::beast::http;

  std::string pattern =
      std::string("/rpc") + std::string(GetRealFuncName(service_func_wrapper.func_name));

  runtime::common::net::AsioHttpServer::HttpHandle<http::dynamic_body> http_handle =
      [this, &service_func_wrapper](
          const http::request<http::dynamic_body>& req,
          http::response<http::dynamic_body>& rsp,
          std::chrono::nanoseconds timeout)
      -> asio::awaitable<runtime::common::net::AsioHttpServer::HttpHandleStatus> {
    // ctx 创建
    auto ctx_ptr = std::make_shared<aimrt::rpc::Context>();

    // 序列化类型
    std::string serialization_type;
    auto req_content_type_itr = req.find(http::field::content_type);
    AIMRT_CHECK_ERROR_THROW(req_content_type_itr != req.end(),
                            "Http req has no content type.");

    auto req_content_type_boost_sw = req_content_type_itr->value();
    std::string_view req_content_type(req_content_type_boost_sw.data(), req_content_type_boost_sw.size());
    if (req_content_type == "application/json" ||
        req_content_type == "application/json charset=utf-8") {
      serialization_type = "json";
      ctx_ptr->SetMetaValue(AIMRT_RPC_CONTEXT_KEY_SERIALIZATION_TYPE, serialization_type);
      rsp.set(http::field::content_type, "application/json");
    } else if (req_content_type == "application/protobuf") {
      serialization_type = "pb";
      ctx_ptr->SetMetaValue(AIMRT_RPC_CONTEXT_KEY_SERIALIZATION_TYPE, serialization_type);
      rsp.set(http::field::content_type, "application/protobuf");
    } else if (req_content_type == "application/ros2") {
      serialization_type = "ros2";
      ctx_ptr->SetMetaValue(AIMRT_RPC_CONTEXT_KEY_SERIALIZATION_TYPE, serialization_type);
      rsp.set(http::field::content_type, "application/ros2");
    } else {
      AIMRT_ERROR_THROW("Http req has invalid content type {}.", req_content_type);
    }

    // 超时
    ctx_ptr->SetTimeout(timeout);

    // service req反序列化
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

    auto service_req_type_support_ref = aimrt::util::TypeSupportRef(service_func_wrapper.req_type_support);

    std::shared_ptr<void> service_req_ptr = service_req_type_support_ref.CreateSharedPtr();

    bool deserialize_ret = service_req_type_support_ref.Deserialize(
        serialization_type, buffer_array_view, service_req_ptr.get());

    AIMRT_CHECK_ERROR_THROW(deserialize_ret, "Http req deserialize failed.");

    // service rsp创建
    auto service_rsp_type_support_ref = aimrt::util::TypeSupportRef(service_func_wrapper.rsp_type_support);

    std::shared_ptr<void> service_rsp_ptr = service_rsp_type_support_ref.CreateSharedPtr();

    // service rpc调用
    uint32_t ret_code = 0;
    auto sig_timer_ptr = std::make_shared<asio::steady_timer>(*io_ptr_, timeout);
    std::atomic_bool handle_flag = false;

    aimrt::rpc::ServiceCallback service_callback(
        [&service_func_wrapper,
         ctx_ptr,
         &req,
         &rsp,
         service_req_ptr,
         service_rsp_ptr,
         serialization_type{std::move(serialization_type)},
         sig_timer_ptr,
         &handle_flag,
         &ret_code](uint32_t code) {
          if (code) [[unlikely]] {
            ret_code = code;

          } else {
            aimrt::util::BufferArray buffer_array;

            auto service_rsp_type_support_ref = aimrt::util::TypeSupportRef(service_func_wrapper.rsp_type_support);

            // service rsp序列化
            bool serialize_ret = service_rsp_type_support_ref.Serialize(
                serialization_type, service_rsp_ptr.get(), buffer_array.AllocatorNativeHandle(), buffer_array.BufferArrayNativeHandle());

            if (!serialize_ret) [[unlikely]] {
              ret_code = AIMRT_RPC_STATUS_SVR_SERIALIZATION_FAILED;
            } else {
              // 填http rsp包，直接复制过去
              size_t rsp_size = buffer_array.BufferSize();
              auto rsp_beast_buf = rsp.body().prepare(rsp_size);

              auto data = buffer_array.BufferArrayNativeHandle()->data;
              auto buffer_array_pos = 0;
              size_t buffer_pos = 0;

              for (auto buf : boost::beast::buffers_range_ref(rsp_beast_buf)) {
                size_t cur_beast_buf_pos = 0;
                while (cur_beast_buf_pos < buf.size()) {
                  size_t cur_beast_buffer_size = buf.size() - cur_beast_buf_pos;
                  size_t cur_buffer_size = data[buffer_array_pos].len - buffer_pos;

                  size_t cur_copy_size = std::min(cur_beast_buffer_size, cur_buffer_size);

                  memcpy(static_cast<char*>(buf.data()) + cur_beast_buf_pos,
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
              rsp.body().commit(rsp_size);

              rsp.keep_alive(req.keep_alive());
              rsp.prepare_payload();
            }
          }

          handle_flag.store(true);

          // TODO: 这里可能会有提前cancel的风险。可能的解决方案：多cancel几次？
          sig_timer_ptr->cancel();
        });

    service_func_wrapper.service_func(
        ctx_ptr->NativeHandle(),
        service_req_ptr.get(),
        service_rsp_ptr.get(),
        service_callback.NativeHandle());

    bool finish_flag = true;
    if (!handle_flag.load()) {
      try {
        co_await sig_timer_ptr->async_wait(asio::use_awaitable);
        if (!handle_flag.load()) finish_flag = false;
      } catch (const std::exception& e) {
        AIMRT_TRACE("rpc cli session recv sig timer canceled, exception info: {}", e.what());
      }
    }

    AIMRT_CHECK_ERROR_THROW(finish_flag, "Local processing timeout.");
    AIMRT_CHECK_ERROR_THROW(ret_code == 0, "Handle rpc failed, code: {}.", ret_code);

    co_return runtime::common::net::AsioHttpServer::HttpHandleStatus::OK;
  };

  http_svr_ptr_->RegisterHttpHandleFunc<http::dynamic_body>(
      pattern, std::move(http_handle));
  AIMRT_INFO("Register http handle for rpc, uri '{}'", pattern);

  return true;
}

bool HttpRpcBackend::RegisterClientFunc(
    const runtime::core::rpc::ClientFuncWrapper& client_func_wrapper) noexcept {
  if (state_.load() != State::Init) {
    AIMRT_ERROR("Client func can only be registered when state is 'Init'.");
    return false;
  }

  return true;
}

bool HttpRpcBackend::TryInvoke(
    const std::shared_ptr<runtime::core::rpc::ClientInvokeWrapper>& client_invoke_wrapper_ptr) noexcept {
  assert(state_.load() == State::Start);

  namespace asio = boost::asio;
  namespace http = boost::beast::http;

  std::string_view pkg_path = client_invoke_wrapper_ptr->pkg_path;
  std::string_view module_name = client_invoke_wrapper_ptr->module_name;
  std::string_view func_name = client_invoke_wrapper_ptr->func_name;

  auto real_func_name = GetRealFuncName(func_name);

  // 检查ctx，to_addr优先级：ctx > server_url
  std::string_view to_addr =
      client_invoke_wrapper_ptr->ctx_ref.GetMetaValue(AIMRT_RPC_CONTEXT_KEY_TO_ADDR);

  if (to_addr.empty()) {
    for (auto& client_options : options_.clients_options) {
      try {
        if (std::regex_match(real_func_name.begin(), real_func_name.end(),
                             std::regex(client_options.func_name, std::regex::ECMAScript))) {
          to_addr = client_options.server_url;
          break;
        }
      } catch (const std::exception& e) {
        AIMRT_WARN("Regex get exception, expr: {}, string: {}, exception info: {}",
                   client_options.func_name, real_func_name, e.what());
      }
    }
  }

  if (to_addr.empty()) return false;

  auto pos = to_addr.find("://");
  if (pos == std::string_view::npos) return false;
  if (to_addr.substr(0, pos) != Name()) return false;

  // 协议为http，需要由http后端处理。之后只能使用callback报错，不能返回false
  auto url_op = aimrt::common::util::ParseUrl(to_addr);
  if (!url_op) {
    client_invoke_wrapper_ptr->callback(AIMRT_RPC_STATUS_CLI_INVALID_ADDR);
    return true;
  }

  std::string url_path(url_op->path);
  if (url_path.empty()) {
    url_path = "/rpc" + std::string(real_func_name);
  }

  const auto& client_func_wrapper_map = rpc_registry_ptr_->GetClientFuncWrapperMap();

  // 找注册的client方法
  auto get_client_func_wrapper_ptr_func = [&]() -> const runtime::core::rpc::ClientFuncWrapper* {
    auto find_lib_itr = client_func_wrapper_map.find(pkg_path);
    if (find_lib_itr == client_func_wrapper_map.end()) return nullptr;

    auto find_module_itr = find_lib_itr->second.find(module_name);
    if (find_module_itr == find_lib_itr->second.end()) return nullptr;

    auto find_func_itr = find_module_itr->second.find(func_name);
    if (find_func_itr == find_module_itr->second.end()) return nullptr;

    return find_func_itr->second.get();
  };
  const auto* client_func_wrapper_ptr = get_client_func_wrapper_ptr_func();

  asio::co_spawn(
      *io_ptr_,
      [http_cli_pool_ptr{http_cli_pool_ptr_},
       client_invoke_wrapper_ptr,
       client_func_wrapper_ptr,
       url_op{std::move(url_op)},
       url_path{std::move(url_path)}]() -> asio::awaitable<void> {
        try {
          runtime::common::net::AsioHttpClient::Options cli_options{
              .host = std::string(url_op->host),
              .service = std::string(url_op->service)};

          auto client_ptr = co_await http_cli_pool_ptr->GetClient(cli_options);

          http::request<http::dynamic_body> req{
              http::verb::post, url_path, 11};
          req.set(http::field::host, url_op->host);
          req.set(http::field::user_agent, "aimrt");

          auto timeout = client_invoke_wrapper_ptr->ctx_ref.Timeout();
          if (timeout <= std::chrono::nanoseconds(0))
            timeout = std::chrono::seconds(5);
          req.set(http::field::timeout,
                  std::to_string(std::chrono::duration_cast<std::chrono::seconds>(timeout).count()));

          std::string serialization_type(
              client_invoke_wrapper_ptr->ctx_ref.GetMetaValue(AIMRT_RPC_CONTEXT_KEY_SERIALIZATION_TYPE));
          if (serialization_type == "json") {
            req.set(http::field::content_type, "application/json");
          } else if (serialization_type == "pb") {
            req.set(http::field::content_type, "application/protobuf");
          } else if (serialization_type == "ros2") {
            req.set(http::field::content_type, "application/ros2");
          } else {
            client_invoke_wrapper_ptr->callback(AIMRT_RPC_STATUS_CLI_INVALID_SERIALIZATION_TYPE);
            co_return;
          }

          aimrt::util::BufferArray buffer_array;

          auto client_req_type_support_ref = aimrt::util::TypeSupportRef(client_func_wrapper_ptr->req_type_support);

          // client req序列化
          bool serialize_ret = client_req_type_support_ref.Serialize(
              serialization_type, client_invoke_wrapper_ptr->req_ptr, buffer_array.AllocatorNativeHandle(), buffer_array.BufferArrayNativeHandle());

          if (!serialize_ret) [[unlikely]] {
            client_invoke_wrapper_ptr->callback(AIMRT_RPC_STATUS_CLI_SERIALIZATION_FAILED);
            co_return;
          }

          // 填http req包，直接复制过去
          size_t req_size = buffer_array.BufferSize();
          auto req_beast_buf = req.body().prepare(req_size);

          auto data = buffer_array.BufferArrayNativeHandle()->data;
          auto buffer_array_pos = 0;
          size_t buffer_pos = 0;

          for (auto buf : boost::beast::buffers_range_ref(req_beast_buf)) {
            size_t cur_beast_buf_pos = 0;
            while (cur_beast_buf_pos < buf.size()) {
              size_t cur_beast_buffer_size = buf.size() - cur_beast_buf_pos;
              size_t cur_buffer_size = data[buffer_array_pos].len - buffer_pos;

              size_t cur_copy_size = std::min(cur_beast_buffer_size, cur_buffer_size);

              memcpy(static_cast<char*>(buf.data()) + cur_beast_buf_pos,
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
          req.body().commit(req_size);

          req.prepare_payload();
          req.keep_alive(true);

          auto rsp = co_await client_ptr->HttpSendRecvCo<http::dynamic_body, http::dynamic_body>(req, timeout);

          // 检查rsp header等参数（TODO）

          // client rsp 反序列化
          const auto& rsp_beast_buf = rsp.body().data();
          std::vector<aimrt_buffer_view_t> buffer_view_vec;

          for (auto const buf : boost::beast::buffers_range_ref(rsp_beast_buf)) {
            buffer_view_vec.emplace_back(aimrt_buffer_view_t{
                .data = buf.data(),
                .len = buf.size()});
          }

          aimrt_buffer_array_view_t buffer_array_view{
              .data = buffer_view_vec.data(),
              .len = buffer_view_vec.size()};

          auto client_rsp_type_support_ref = aimrt::util::TypeSupportRef(client_func_wrapper_ptr->rsp_type_support);

          bool deserialize_ret = client_rsp_type_support_ref.Deserialize(
              serialization_type, buffer_array_view, client_invoke_wrapper_ptr->rsp_ptr);

          if (!deserialize_ret) {
            // 调用回调
            client_invoke_wrapper_ptr->callback(AIMRT_RPC_STATUS_CLI_DESERIALIZATION_FAILED);
            co_return;
          }

          client_invoke_wrapper_ptr->callback(AIMRT_RPC_STATUS_OK);

          co_return;
        } catch (const std::exception& e) {
          AIMRT_WARN("Http call get exception, info: {}", e.what());
        }

        // TODO
        client_invoke_wrapper_ptr->callback(AIMRT_RPC_STATUS_CLI_UNKNOWN);
        co_return;
      },
      asio::detached);

  return true;
}

}  // namespace aimrt::plugins::net_plugin