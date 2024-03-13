#include "mqtt_plugin/mqtt_rpc_backend.h"

#include <regex>

#include "aimrt_module_cpp_interface/rpc/rpc_status.h"
#include "aimrt_module_cpp_interface/util/buffer.h"
#include "aimrt_module_cpp_interface/util/type_support.h"
#include "mqtt_plugin/global.h"
#include "util/url_encode.h"

namespace YAML {
template <>
struct convert<aimrt::plugins::mqtt_plugin::MqttRpcBackend::Options> {
  using Options = aimrt::plugins::mqtt_plugin::MqttRpcBackend::Options;

  static Node encode(const Options& rhs) {
    Node node;

    node["clients_options"] = YAML::Node();
    for (const auto& client_options : rhs.clients_options) {
      Node client_options_node;
      client_options_node["func_name"] = client_options.func_name;
      client_options_node["enable"] = client_options.enable;
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
            .enable = client_options_node["enable"].as<bool>()};

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

  // todo:换成MQTTClient_unsubscribeMany
  for (auto sub_info : sub_info_vec_) {
    AIMRT_TRACE("Mqtt unsubscribe for '{}'", sub_info);
    MQTTClient_unsubscribe(client_, sub_info.data());
  }
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

  msg_handle_registry_ptr_->RegisterRpcHandle(
      mqtt_sub_topic,
      [this, &service_func_wrapper](MQTTClient_message* message) {
        try {
          // ctx 创建
          std::shared_ptr<runtime::core::rpc::ContextImpl> ctx_ptr = context_manager_ptr_->NewContextSharedPtr();

          // 获取属性
          // 1 byte serialization_type size | n0 byte serialization_type |
          // 1 byte mqtt_pub_topic size | n1 byte mqtt_pub_topic |
          // 4 byte req_id_data |
          // buf.len-1-n0-1-n1-4 byte data

          const void* buf_data = message->payload;
          size_t buf_size = message->payloadlen;

          size_t cur_pos = 0;

          // serialization_type
          AIMRT_CHECK_ERROR_THROW(buf_size - cur_pos > 1, "Invalid msg, buf size: {}", buf_size);
          uint8_t serialization_type_size = static_cast<const uint8_t*>(buf_data)[cur_pos];
          cur_pos += 1;

          AIMRT_CHECK_ERROR_THROW(buf_size - cur_pos > serialization_type_size,
                                  "Invalid msg, buf size: {}, serialization type size: {}",
                                  buf_size, serialization_type_size);
          std::string serialization_type = std::string(static_cast<const char*>(buf_data) + cur_pos, serialization_type_size);
          cur_pos += serialization_type_size;
          ctx_ptr->SetMetaValue(AIMRT_RPC_CONTEXT_KEY_SERIALIZATION_TYPE, serialization_type);

          // mqtt_pub_topic
          AIMRT_CHECK_ERROR_THROW(buf_size - cur_pos > 1, "Invalid msg, buf size: {}", buf_size);
          uint8_t mqtt_pub_topic_size = static_cast<const uint8_t*>(buf_data)[cur_pos];
          cur_pos += 1;

          AIMRT_CHECK_ERROR_THROW(buf_size - cur_pos > mqtt_pub_topic_size,
                                  "Invalid msg, buf size: {}, mqtt pub topic size: {}",
                                  buf_size, mqtt_pub_topic_size);
          std::string mqtt_pub_topic = std::string(static_cast<const char*>(buf_data) + cur_pos, mqtt_pub_topic_size);
          cur_pos += mqtt_pub_topic_size;

          // req_id
          AIMRT_CHECK_ERROR_THROW(buf_size - cur_pos > 4, "Invalid msg, buf size: {}", buf_size);
          uint8_t req_id_buf[4];
          memcpy(req_id_buf, static_cast<const char*>(buf_data) + cur_pos, 4);
          cur_pos += 4;

          // service req反序列化
          aimrt_buffer_view_t buffer_view{.data = static_cast<const char*>(buf_data) + cur_pos,
                                          .len = buf_size - cur_pos};

          aimrt_buffer_array_view_t buffer_array_view{
              .data = &buffer_view,
              .len = 1};

          auto service_req_type_support_ref = aimrt::util::TypeSupportRef(service_func_wrapper.req_type_support);

          std::shared_ptr<void> service_req_ptr = service_req_type_support_ref.CreateSharedPtr();

          bool deserialize_ret = service_req_type_support_ref.Deserialize(
              serialization_type, buffer_array_view, service_req_ptr.get());

          AIMRT_CHECK_ERROR_THROW(deserialize_ret, "Mqtt req deserialize failed.");

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
                aimrt::util::BufferArray buffer_array;

                if (code) [[unlikely]] {
                  // 如果code不为suc，则没必要反序列化
                } else {
                  // service rsp序列化
                  auto service_rsp_type_support_ref = aimrt::util::TypeSupportRef(service_func_wrapper.rsp_type_support);

                  // service rsp序列化
                  bool serialize_ret = service_rsp_type_support_ref.Serialize(
                      serialization_type, service_rsp_ptr.get(), buffer_array.NativeHandle());

                  // 序列化失败一般很少见，此处暂时不做处理
                  assert(serialize_ret);
                }

                // 1 byte serialization_type size | n0 byte serialization_type |
                // 4 byte req_id_data |
                // buf.len-1-n0-4 byte data

                MQTTClient_message pubmsg = MQTTClient_message_initializer;

                auto buffer_array_data = buffer_array.Data();
                const size_t buffer_array_len = buffer_array.Size();
                size_t rsp_size = buffer_array.BufferSize();

                std::vector<uint8_t> msg_buf_vec(1 + serialization_type.size() + 4 + rsp_size);

                size_t cur_pos = 0;

                // 1 byte serialization_type size | n0 byte serialization_type
                size_t serialization_type_size = serialization_type.size();
                msg_buf_vec[cur_pos] = static_cast<uint8_t>(serialization_type_size);
                cur_pos += 1;
                memcpy(msg_buf_vec.data() + cur_pos, serialization_type.data(), serialization_type_size);
                cur_pos += serialization_type_size;

                // 4 byte req_id_data
                memcpy(msg_buf_vec.data() + cur_pos, req_id_buf, sizeof(req_id_buf));
                cur_pos += sizeof(req_id_buf);

                // data
                for (size_t ii = 0; ii < buffer_array_len; ++ii) {
                  memcpy(msg_buf_vec.data() + cur_pos, buffer_array_data[ii].data, buffer_array_data[ii].len);
                  cur_pos += buffer_array_data[ii].len;
                }

                // 发送
                pubmsg.payload = msg_buf_vec.data();
                pubmsg.payloadlen = msg_buf_vec.size();
                pubmsg.qos = 2;
                pubmsg.retained = 0;

                AIMRT_TRACE("Mqtt publish to '{}'", mqtt_pub_topic);
                int rc = MQTTClient_publishMessage(client_, mqtt_pub_topic.data(), &pubmsg, NULL);
                AIMRT_CHECK_WARN(rc == MQTTCLIENT_SUCCESS,
                                 "Publist mqtt msg failed, topic: {}, code: {}",
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

  sub_info_vec_.emplace_back(mqtt_sub_topic);

  msg_handle_registry_ptr_->RegisterRpcHandle(
      mqtt_sub_topic,
      [this](MQTTClient_message* message) {
        std::shared_ptr<runtime::core::rpc::ClientInvokeWrapper> client_invoke_wrapper_ptr;

        try {
          // 1 byte serialization_type size | n0 byte serialization_type |
          // 4 byte req_id_data |
          // buf.len-1-n0-4 byte data

          const void* buf_data = message->payload;
          size_t buf_size = message->payloadlen;

          size_t cur_pos = 0;

          // serialization_type
          AIMRT_CHECK_ERROR_THROW(buf_size - cur_pos > 1, "Invalid msg, buf size: {}", buf_size);
          uint8_t serialization_type_size = static_cast<const uint8_t*>(buf_data)[cur_pos];
          cur_pos += 1;

          AIMRT_CHECK_ERROR_THROW(buf_size - cur_pos > serialization_type_size,
                                  "Invalid msg, buf size: {}, serialization type size: {}",
                                  buf_size, serialization_type_size);
          std::string serialization_type = std::string(static_cast<const char*>(buf_data) + cur_pos, serialization_type_size);
          cur_pos += serialization_type_size;

          // req_id
          AIMRT_CHECK_ERROR_THROW(buf_size - cur_pos > 4, "Invalid msg, buf size: {}", buf_size);
          uint32_t req_id;
          memcpy(&req_id, static_cast<const char*>(buf_data) + cur_pos, 4);
          cur_pos += 4;

          // todo，从用户属性中获取server端返回code

          // 从client_msg_recorder_map_找到req
          ClientMsgRecorderMap::const_accessor ac;
          bool find_ret = client_msg_recorder_map_.find(ac, req_id);
          AIMRT_CHECK_ERROR_THROW(find_ret, "Invalid req id '{}' from mqtt msg", req_id);

          auto client_func_wrapper_ptr = ac->second.client_func_wrapper_ptr;
          client_invoke_wrapper_ptr = std::move(ac->second.client_invoke_wrapper_ptr);
          client_msg_recorder_map_.erase(ac);

          // client rsp 反序列化
          aimrt_buffer_view_t buffer_view{.data = static_cast<const char*>(buf_data) + cur_pos,
                                          .len = buf_size - cur_pos};

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

  auto real_func_name = GetRealFuncName(func_name);

  // 检查ctx，当前只支持一个server url，且在plugin层配置
  std::string_view to_addr =
      client_invoke_wrapper_ptr->ctx_ref.GetMetaValue(AIMRT_RPC_CONTEXT_KEY_TO_ADDR);

  if (to_addr.empty()) {
    for (auto& client_options : options_.clients_options) {
      try {
        if (std::regex_match(real_func_name.begin(), real_func_name.end(),
                             std::regex(client_options.func_name, std::regex::ECMAScript))) {
          if (client_options.enable) to_addr = "mqtt://";
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
  if (to_addr.substr(0, pos) != "mqtt") return false;

  // 协议为mqtt，需要由mqtt后端处理。之后只能使用callback报错，不能返回false
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

  // 将回调等内容放到并发hash表中
  uint32_t cur_req_id = ++req_id_;

  ClientMsgRecorderMap::const_accessor ac;
  bool emplace_ret = client_msg_recorder_map_.emplace(
      ac,
      cur_req_id,
      MsgRecorder{
          .client_func_wrapper_ptr = client_func_wrapper_ptr,
          .client_invoke_wrapper_ptr = client_invoke_wrapper_ptr});

  if (!emplace_ret) [[unlikely]] {
    // 一般不太可能出现，除非req_id_积压了一轮了
    client_invoke_wrapper_ptr->callback(AIMRT_RPC_STATUS_CLI_UNKNOWN);
    return true;
  }

  // 发布到mqtt
  // 1 byte serialization_type size | n0 byte serialization_type |
  // 1 byte mqtt_pub_topic size | n1 byte mqtt_pub_topic |
  // 4 byte req_id_data |
  // buf.len-1-n0-1-n1-4 byte data

  MQTTClient_message pubmsg = MQTTClient_message_initializer;

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
      serialization_type, client_invoke_wrapper_ptr->req_ptr, buffer_array.NativeHandle());

  // 序列化失败一般很少见，此处暂时不做处理
  assert(serialize_ret);

  // 填mqtt包
  auto buffer_array_data = buffer_array.Data();
  const size_t buffer_array_len = buffer_array.Size();
  size_t req_size = buffer_array.BufferSize();

  std::vector<uint8_t> msg_buf_vec(
      1 + serialization_type.size() +
      1 + mqtt_sub_topic.size() +
      4 +
      req_size);

  size_t cur_pos = 0;

  // 1 byte serialization_type size | n0 byte serialization_type
  size_t serialization_type_size = serialization_type.size();
  msg_buf_vec[cur_pos] = static_cast<uint8_t>(serialization_type_size);
  cur_pos += 1;
  memcpy(msg_buf_vec.data() + cur_pos, serialization_type.data(), serialization_type_size);
  cur_pos += serialization_type_size;

  // 1 byte mqtt_pub_topic size | n1 byte mqtt_pub_topic
  size_t mqtt_sub_topic_size = mqtt_sub_topic.size();
  msg_buf_vec[cur_pos] = static_cast<uint8_t>(mqtt_sub_topic_size);
  cur_pos += 1;
  memcpy(msg_buf_vec.data() + cur_pos, mqtt_sub_topic.data(), mqtt_sub_topic_size);
  cur_pos += mqtt_sub_topic_size;

  // 4 byte req_id_data
  static_assert(sizeof(cur_req_id) == 4);
  memcpy(msg_buf_vec.data() + cur_pos, &cur_req_id, sizeof(cur_req_id));
  cur_pos += sizeof(cur_req_id);

  // data
  for (size_t ii = 0; ii < buffer_array_len; ++ii) {
    memcpy(msg_buf_vec.data() + cur_pos, buffer_array_data[ii].data, buffer_array_data[ii].len);
    cur_pos += buffer_array_data[ii].len;
  }

  // 发送
  pubmsg.payload = msg_buf_vec.data();
  pubmsg.payloadlen = msg_buf_vec.size();
  pubmsg.qos = 2;
  pubmsg.retained = 0;

  // topic: aimrt_rpc_req/uri
  std::string mqtt_pub_topic = "aimrt_rpc_req/" + util::UrlEncode(GetRealFuncName(func_name));

  AIMRT_TRACE("Mqtt publish to '{}'", mqtt_pub_topic);
  int rc = MQTTClient_publishMessage(client_, mqtt_pub_topic.data(), &pubmsg, NULL);
  AIMRT_CHECK_WARN(rc == MQTTCLIENT_SUCCESS,
                   "Publist mqtt msg failed, topic: {}, code: {}",
                   mqtt_pub_topic, rc);

  return true;
}

void MqttRpcBackend::SubscribeMqttTopic() {
  for (auto sub_info : sub_info_vec_) {
    // rpc qos默认都是2
    // todo:换成MQTTClient_subscribeMany
    AIMRT_TRACE("Mqtt subscribe for '{}'", sub_info);
    int rc = MQTTClient_subscribe(client_, sub_info.data(), 2);
    if (rc != MQTTCLIENT_SUCCESS) {
      AIMRT_ERROR("Failed to subscribe mqtt, topic: {} return code: {}", sub_info, rc);
    }
  }
}

}  // namespace aimrt::plugins::mqtt_plugin