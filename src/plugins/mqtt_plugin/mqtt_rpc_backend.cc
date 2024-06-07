#include "mqtt_plugin/mqtt_rpc_backend.h"

#include <regex>

#include "aimrt_module_cpp_interface/rpc/rpc_status.h"
#include "aimrt_module_cpp_interface/util/buffer.h"
#include "aimrt_module_cpp_interface/util/type_support.h"
#include "mqtt_plugin/global.h"
#include "util/buffer_util.h"
#include "util/url_encode.h"

namespace YAML {
template <>
struct convert<aimrt::plugins::mqtt_plugin::MqttRpcBackend::Options> {
  using Options = aimrt::plugins::mqtt_plugin::MqttRpcBackend::Options;

  static Node encode(const Options& rhs) {
    Node node;

    node["timeout_executor"] = rhs.timeout_executor;

    node["clients_options"] = YAML::Node();
    for (const auto& client_options : rhs.clients_options) {
      Node client_options_node;
      client_options_node["func_name"] = client_options.func_name;
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
    if (node["timeout_executor"])
      rhs.timeout_executor = node["timeout_executor"].as<std::string>();

    if (node["clients_options"] && node["clients_options"].IsSequence()) {
      for (auto& client_options_node : node["clients_options"]) {
        auto client_options = Options::ClientOptions{
            .func_name = client_options_node["func_name"].as<std::string>()};

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

namespace aimrt::plugins::mqtt_plugin {

void MqttRpcBackend::Initialize(YAML::Node options_node,
                                const runtime::core::rpc::RpcRegistry* rpc_registry_ptr,
                                runtime::core::rpc::ContextManager* context_manager_ptr) {
  AIMRT_CHECK_ERROR_THROW(
      std::atomic_exchange(&state_, State::Init) == State::PreInit,
      "Mqtt Rpc backend can only be initialized once.");

  if (options_node && !options_node.IsNull())
    options_ = options_node.as<Options>();

  rpc_registry_ptr_ = rpc_registry_ptr;
  context_manager_ptr_ = context_manager_ptr;

  if (!options_.timeout_executor.empty()) {
    AIMRT_CHECK_ERROR_THROW(
        get_executor_func_,
        "Get executor function is not set before initialize.");

    auto timeout_executor = get_executor_func_(options_.timeout_executor);

    AIMRT_CHECK_ERROR_THROW(
        timeout_executor,
        "Get timeout executor '{}' failed.", options_.timeout_executor);

    client_tool_.RegisterTimeoutExecutor(timeout_executor);
    client_tool_.RegisterTimeoutHandle([](MsgRecorder&& msg_recorder) {
      msg_recorder.client_invoke_wrapper_ptr->callback(AIMRT_RPC_STATUS_TIMEOUT);
    });

    AIMRT_TRACE("Mqtt rpc backend enable the timeout function, use '{}' as timeout executor.",
                options_.timeout_executor);
  } else {
    AIMRT_TRACE("Mqtt rpc backend does not enable the timeout function.");
  }

  options_node = options_;
}

void MqttRpcBackend::Start() {
  AIMRT_CHECK_ERROR_THROW(
      std::atomic_exchange(&state_, State::Start) == State::Init,
      "Function can only be called when state is 'Init'.");

  SubscribeMqttTopic();
}

void MqttRpcBackend::Shutdown() {
  if (std::atomic_exchange(&state_, State::Shutdown) == State::Shutdown)
    return;

  UnSubscribeMqttTopic();
}

bool MqttRpcBackend::RegisterServiceFunc(
    const runtime::core::rpc::ServiceFuncWrapper& service_func_wrapper) noexcept {
  if (state_.load() != State::Init) {
    AIMRT_ERROR("Service func can only be registered when state is 'Init'.");
    return false;
  }

  namespace util = aimrt::common::util;

  std::string mqtt_sub_topic = "aimrt_rpc_req/" + util::UrlEncode(GetRealFuncName(service_func_wrapper.func_name));

  std::string share_mqtt_sub_topic = "$share/aimrt/" + mqtt_sub_topic;

  sub_info_vec_.emplace_back(share_mqtt_sub_topic);

  msg_handle_registry_ptr_->RegisterMsgHandle(
      mqtt_sub_topic,
      [this, &service_func_wrapper](MQTTAsync_message* message) {
        try {
          std::shared_ptr<runtime::core::rpc::ContextImpl> ctx_ptr = context_manager_ptr_->NewContextSharedPtr();

          // 获取字段
          util::ConstBufferOperator buf_oper(static_cast<const char*>(message->payload), message->payloadlen);

          std::string serialization_type(buf_oper.GetString(util::BufferLenType::UINT8));
          ctx_ptr->SetMetaValue(AIMRT_RPC_CONTEXT_KEY_SERIALIZATION_TYPE, serialization_type);

          std::string mqtt_pub_topic(buf_oper.GetString(util::BufferLenType::UINT8));

          char req_id_buf[4];
          buf_oper.GetBuffer(req_id_buf, 4);

          // service req反序列化
          auto remaining_buf = buf_oper.GetRemainingBuffer();
          aimrt_buffer_view_t buffer_view{
              .data = remaining_buf.data(),
              .len = remaining_buf.size()};

          aimrt_buffer_array_view_t buffer_array_view{
              .data = &buffer_view,
              .len = 1};

          auto service_req_type_support_ref = aimrt::util::TypeSupportRef(service_func_wrapper.req_type_support);

          std::shared_ptr<void> service_req_ptr = service_req_type_support_ref.CreateSharedPtr();

          bool deserialize_ret = service_req_type_support_ref.Deserialize(
              serialization_type, buffer_array_view, service_req_ptr.get());

          if (!deserialize_ret) [[unlikely]] {
            AIMRT_ERROR("Mqtt req deserialize failed.");

            ReturnRspWithStatusCode(
                mqtt_pub_topic, serialization_type, req_id_buf, AIMRT_RPC_STATUS_SVR_DESERIALIZATION_FAILDE);

            return;
          }

          // service rsp创建
          auto service_rsp_type_support_ref = aimrt::util::TypeSupportRef(service_func_wrapper.rsp_type_support);

          std::shared_ptr<void> service_rsp_ptr = service_rsp_type_support_ref.CreateSharedPtr();

          // service rpc调用
          aimrt::util::Function<aimrt_function_service_callback_ops_t> service_callback(
              [this,
               &service_func_wrapper,
               ctx_ptr,
               service_req_ptr,
               service_rsp_ptr,
               serialization_type{std::move(serialization_type)},
               mqtt_pub_topic{std::move(mqtt_pub_topic)},
               req_id_buf](uint32_t code) {
                if (code) [[unlikely]] {
                  // 如果code不为suc，则没必要反序列化
                  ReturnRspWithStatusCode(
                      mqtt_pub_topic, serialization_type, req_id_buf, code);

                  return;
                }

                aimrt::util::BufferArray buffer_array;

                // service rsp序列化
                auto service_rsp_type_support_ref = aimrt::util::TypeSupportRef(service_func_wrapper.rsp_type_support);
                bool serialize_ret = service_rsp_type_support_ref.Serialize(
                    serialization_type, service_rsp_ptr.get(), buffer_array.AllocatorNativeHandle(), buffer_array.BufferArrayNativeHandle());

                // 序列化失败一般很少见，此处暂时不做处理
                assert(serialize_ret);

                auto buffer_array_data = buffer_array.Data();
                const size_t buffer_array_len = buffer_array.Size();
                size_t rsp_size = buffer_array.BufferSize();

                size_t mqtt_pkg_size = 1 + serialization_type.size() + 4 + 4 + rsp_size;

                if (mqtt_pkg_size > max_pkg_size_) [[unlikely]] {
                  AIMRT_WARN("Mqtt publish failed, pkg is too large, limit {}k, actual {}k",
                             max_pkg_size_ / 1024, mqtt_pkg_size / 1024);

                  ReturnRspWithStatusCode(
                      mqtt_pub_topic, serialization_type, req_id_buf, AIMRT_RPC_STATUS_SVR_UNKNOWN);
                  return;
                }

                std::vector<char> msg_buf_vec(mqtt_pkg_size);
                util::BufferOperator buf_oper(msg_buf_vec.data(), msg_buf_vec.size());

                buf_oper.SetString(serialization_type, util::BufferLenType::UINT8);
                buf_oper.SetBuffer(req_id_buf, sizeof(req_id_buf));
                buf_oper.SetUint32(0);

                for (size_t ii = 0; ii < buffer_array_len; ++ii) {
                  buf_oper.SetBuffer(
                      static_cast<const char*>(buffer_array_data[ii].data),
                      buffer_array_data[ii].len);
                }

                MQTTAsync_message pubmsg = MQTTAsync_message_initializer;
                pubmsg.payload = msg_buf_vec.data();
                pubmsg.payloadlen = msg_buf_vec.size();
                pubmsg.qos = 2;
                pubmsg.retained = 0;

                AIMRT_TRACE("Mqtt publish to '{}'", mqtt_pub_topic);
                int rc = MQTTAsync_sendMessage(client_, mqtt_pub_topic.data(), &pubmsg, NULL);
                AIMRT_CHECK_WARN(rc == MQTTASYNC_SUCCESS,
                                 "publish mqtt msg failed, topic: {}, code: {}",
                                 mqtt_pub_topic, rc);
              });

          service_func_wrapper.service_func(
              ctx_ptr->NativeHandle(),
              service_req_ptr.get(),
              service_rsp_ptr.get(),
              service_callback.NativeHandle());
        } catch (const std::exception& e) {
          AIMRT_WARN("Handle mqtt rpc msg failed, exception info: {}", e.what());
        }
      });

  return true;
}

bool MqttRpcBackend::RegisterClientFunc(
    const runtime::core::rpc::ClientFuncWrapper& client_func_wrapper) noexcept {
  if (state_.load() != State::Init) {
    AIMRT_ERROR("Client func can only be registered when state is 'Init'.");
    return false;
  }

  namespace util = aimrt::common::util;

  std::string mqtt_sub_topic =
      "aimrt_rpc_rsp/" +
      util::UrlEncode(client_id_) + "/" +
      util::UrlEncode(GetRealFuncName(client_func_wrapper.func_name));

  if (mqtt_sub_topic.size() > 255) {
    AIMRT_ERROR("Too long mqtt topic name: {}", mqtt_sub_topic);
    return false;
  }

  sub_info_vec_.emplace_back(mqtt_sub_topic);

  msg_handle_registry_ptr_->RegisterMsgHandle(
      mqtt_sub_topic,
      [this](MQTTAsync_message* message) {
        std::shared_ptr<runtime::core::rpc::ClientInvokeWrapper> client_invoke_wrapper_ptr;

        try {
          util::ConstBufferOperator buf_oper(static_cast<const char*>(message->payload), message->payloadlen);

          std::string serialization_type(buf_oper.GetString(util::BufferLenType::UINT8));
          uint32_t req_id = buf_oper.GetUint32();
          uint32_t code = buf_oper.GetUint32();

          auto msg_recorder = client_tool_.GetRecord(req_id);
          if (!msg_recorder) [[unlikely]] {
            // 未找到记录，说明此次调用已经超时了，走了超时处理后删掉了记录
            AIMRT_TRACE("Can not get req id {} from recorder.", req_id);
            return;
          }

          auto client_func_wrapper_ptr = msg_recorder->client_func_wrapper_ptr;
          client_invoke_wrapper_ptr = std::move(msg_recorder->client_invoke_wrapper_ptr);

          if (code) [[unlikely]] {
            client_invoke_wrapper_ptr->callback(code);
            return;
          }

          // client rsp 反序列化
          auto remaining_buf = buf_oper.GetRemainingBuffer();
          aimrt_buffer_view_t buffer_view{
              .data = remaining_buf.data(),
              .len = remaining_buf.size()};

          aimrt_buffer_array_view_t buffer_array_view{
              .data = &buffer_view,
              .len = 1};

          auto client_rsp_type_support_ref = aimrt::util::TypeSupportRef(client_func_wrapper_ptr->rsp_type_support);

          bool deserialize_ret = client_rsp_type_support_ref.Deserialize(
              serialization_type, buffer_array_view, client_invoke_wrapper_ptr->rsp_ptr);

          if (!deserialize_ret) {
            // 调用回调
            client_invoke_wrapper_ptr->callback(AIMRT_RPC_STATUS_CLI_DESERIALIZATION_FAILDE);
            return;
          }

          client_invoke_wrapper_ptr->callback(AIMRT_RPC_STATUS_OK);
          return;

        } catch (const std::exception& e) {
          AIMRT_WARN("Handle mqtt rpc msg failed, exception info: {}", e.what());
        }

        if (client_invoke_wrapper_ptr)
          client_invoke_wrapper_ptr->callback(AIMRT_RPC_STATUS_CLI_UNKNOWN);
      });

  return true;
}

bool MqttRpcBackend::TryInvoke(
    const std::shared_ptr<runtime::core::rpc::ClientInvokeWrapper>& client_invoke_wrapper_ptr) noexcept {
  assert(state_.load() == State::Start);

  namespace util = aimrt::common::util;

  std::string_view pkg_path = client_invoke_wrapper_ptr->pkg_path;
  std::string_view module_name = client_invoke_wrapper_ptr->module_name;
  std::string_view func_name = client_invoke_wrapper_ptr->func_name;

  // 检查ctx，当前只支持一个server url，且在plugin层配置
  std::string_view to_addr =
      client_invoke_wrapper_ptr->ctx_ref.GetMetaValue(AIMRT_RPC_CONTEXT_KEY_TO_ADDR);

  if (!to_addr.empty()) {
    auto pos = to_addr.find("://");
    if (pos == std::string_view::npos) return false;
    if (to_addr.substr(0, pos) != "mqtt") return false;
  }

  // 协议为mqtt，需要由mqtt后端处理。此行之后只能使用callback报错，不能返回false
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

  uint32_t cur_req_id = client_tool_.GetNewReqID();

  auto serialization_type =
      client_invoke_wrapper_ptr->ctx_ref.GetMetaValue(AIMRT_RPC_CONTEXT_KEY_SERIALIZATION_TYPE);

  assert(serialization_type.size() <= 255);

  // aimrt_rpc_rsp/client_id/uri
  std::string mqtt_sub_topic =
      "aimrt_rpc_rsp/" +
      util::UrlEncode(client_id_) + "/" +
      util::UrlEncode(GetRealFuncName(func_name));

  assert(mqtt_sub_topic.size() <= 255);

  // msg buf
  aimrt::util::BufferArray buffer_array;
  auto client_req_type_support_ref = aimrt::util::TypeSupportRef(client_func_wrapper_ptr->req_type_support);

  // client req序列化
  bool serialize_ret = client_req_type_support_ref.Serialize(
      serialization_type, client_invoke_wrapper_ptr->req_ptr, buffer_array.AllocatorNativeHandle(), buffer_array.BufferArrayNativeHandle());

  // 序列化失败一般很少见，此处暂时不做处理
  assert(serialize_ret);

  // 填mqtt包
  auto buffer_array_data = buffer_array.Data();
  const size_t buffer_array_len = buffer_array.Size();
  size_t req_size = buffer_array.BufferSize();

  size_t mqtt_pkg_size = 1 + serialization_type.size() +
                         1 + mqtt_sub_topic.size() +
                         4 +
                         req_size;

  if (mqtt_pkg_size > max_pkg_size_) [[unlikely]] {
    AIMRT_WARN("Mqtt publish failed, pkg is too large, limit {}k, actual {}k",
               max_pkg_size_ / 1024, mqtt_pkg_size / 1024);

    client_invoke_wrapper_ptr->callback(AIMRT_RPC_STATUS_CLI_SEND_REQ_FAILED);

    return true;
  }

  // 将回调等内容记录下来
  bool ret = client_tool_.Record(
      cur_req_id,
      client_invoke_wrapper_ptr->ctx_ref.Timeout(),
      MsgRecorder{
          .client_func_wrapper_ptr = client_func_wrapper_ptr,
          .client_invoke_wrapper_ptr = client_invoke_wrapper_ptr});

  if (!ret) [[unlikely]] {
    // 一般不太可能出现
    AIMRT_WARN("Failed to record msg.");
    client_invoke_wrapper_ptr->callback(AIMRT_RPC_STATUS_CLI_UNKNOWN);
    return true;
  }

  std::vector<char> msg_buf_vec(mqtt_pkg_size);

  util::BufferOperator buf_oper(msg_buf_vec.data(), msg_buf_vec.size());

  buf_oper.SetString(serialization_type, util::BufferLenType::UINT8);
  buf_oper.SetString(mqtt_sub_topic, util::BufferLenType::UINT8);
  buf_oper.SetUint32(cur_req_id);

  // data
  for (size_t ii = 0; ii < buffer_array_len; ++ii) {
    buf_oper.SetBuffer(
        static_cast<const char*>(buffer_array_data[ii].data),
        buffer_array_data[ii].len);
  }

  // 发送
  MQTTAsync_message pubmsg = MQTTAsync_message_initializer;
  pubmsg.payload = msg_buf_vec.data();
  pubmsg.payloadlen = msg_buf_vec.size();
  pubmsg.qos = 2;
  pubmsg.retained = 0;

  // topic: aimrt_rpc_req/uri
  std::string mqtt_pub_topic = "aimrt_rpc_req/" + util::UrlEncode(GetRealFuncName(func_name));

  AIMRT_TRACE("Mqtt publish to '{}'", mqtt_pub_topic);
  int rc = MQTTAsync_sendMessage(client_, mqtt_pub_topic.data(), &pubmsg, NULL);
  AIMRT_CHECK_WARN(rc == MQTTASYNC_SUCCESS,
                   "publish mqtt msg failed, topic: {}, code: {}",
                   mqtt_pub_topic, rc);

  return true;
}

void MqttRpcBackend::RegisterGetExecutorFunc(
    const std::function<aimrt::executor::ExecutorRef(std::string_view)>& get_executor_func) {
  AIMRT_CHECK_ERROR_THROW(
      state_.load() == State::PreInit,
      "Function can only be called when state is 'PreInit'.");
  get_executor_func_ = get_executor_func;
}

void MqttRpcBackend::SubscribeMqttTopic() {
  // todo:换成MQTTClient_subscribeMany
  for (auto sub_info : sub_info_vec_) {
    AIMRT_TRACE("Mqtt subscribe for '{}'", sub_info);
    int rc = MQTTAsync_subscribe(client_, sub_info.data(), 2, NULL);  // rpc qos默认都是2
    if (rc != MQTTASYNC_SUCCESS) {
      AIMRT_ERROR("Failed to subscribe mqtt, topic: {} return code: {}", sub_info, rc);
    }
  }
}

void MqttRpcBackend::UnSubscribeMqttTopic() {
  // todo:换成MQTTClient_unsubscribeMany
  for (auto sub_info : sub_info_vec_) {
    AIMRT_TRACE("Mqtt unsubscribe for '{}'", sub_info);
    MQTTAsync_unsubscribe(client_, sub_info.data(), NULL);
  }
}

void MqttRpcBackend::ReturnRspWithStatusCode(
    std::string_view mqtt_pub_topic,
    std::string_view serialization_type,
    const char* req_id_buf,
    uint32_t code) {
  namespace util = aimrt::common::util;

  std::vector<char> msg_buf_vec(1 + serialization_type.size() + 4 + 4);
  util::BufferOperator buf_oper(msg_buf_vec.data(), msg_buf_vec.size());

  buf_oper.SetString(serialization_type, util::BufferLenType::UINT8);
  buf_oper.SetBuffer(req_id_buf, 4);
  buf_oper.SetUint32(code);

  MQTTAsync_message pubmsg = MQTTAsync_message_initializer;
  pubmsg.payload = msg_buf_vec.data();
  pubmsg.payloadlen = msg_buf_vec.size();
  pubmsg.qos = 2;
  pubmsg.retained = 0;

  AIMRT_TRACE("Mqtt publish to '{}'", mqtt_pub_topic);
  int rc = MQTTAsync_sendMessage(client_, mqtt_pub_topic.data(), &pubmsg, NULL);
  AIMRT_CHECK_WARN(rc == MQTTASYNC_SUCCESS,
                   "publish mqtt msg failed, topic: {}, code: {}",
                   mqtt_pub_topic, rc);
}

}  // namespace aimrt::plugins::mqtt_plugin