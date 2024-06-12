#include "ros2_plugin/ros2_rpc_backend.h"

#include <regex>

#include "ros2_plugin/global.h"
#include "ros2_plugin/ros2_name_encode.h"

namespace YAML {
template <>
struct convert<aimrt::plugins::ros2_plugin::Ros2RpcBackend::Options> {
  using Options = aimrt::plugins::ros2_plugin::Ros2RpcBackend::Options;

  static Node encode(const Options& rhs) {
    Node node;

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

  static bool decodeQos(const Node& node, Options::QosOptions& qos) {
    if (node) {
      if (node["history"]) {
        qos.history = node["history"].as<std::string>();
      }
      if (node["depth"]) {
        qos.depth = node["depth"].as<int>();
      }
      if (node["reliability"]) {
        qos.reliability = node["reliability"].as<std::string>();
      }
      if (node["durability"]) {
        qos.durability = node["durability"].as<std::string>();
      }
      if (node["lifespan"]) {
        qos.lifespan = node["lifespan"].as<int>();
      }
      if (node["deadline"]) {
        qos.deadline = node["deadline"].as<int>();
      }
      if (node["liveliness"]) {
        qos.liveliness = node["liveliness"].as<std::string>();
      }
      if (node["liveliness_lease_duration"]) {
        qos.liveliness_lease_duration = node["liveliness_lease_duration"].as<int>();
      }
    }
    return true;
  }

  static bool decode(const Node& node, Options& rhs) {
    if (node["clients_options"] && node["clients_options"].IsSequence()) {
      for (auto& client_options_node : node["clients_options"]) {
        auto client_options = Options::ClientOptions{
            .func_name = client_options_node["func_name"].as<std::string>()};

        if (client_options_node["qos"]) {
          decodeQos(client_options_node["qos"], client_options.qos);
        }
        rhs.clients_options.emplace_back(std::move(client_options));
      }
    }

    if (node["servers_options"] && node["servers_options"].IsSequence()) {
      for (auto& server_options_node : node["servers_options"]) {
        auto server_options = Options::ServerOptions{
            .func_name = server_options_node["func_name"].as<std::string>()};
        if (server_options_node["qos"]) {
          decodeQos(server_options_node["qos"], server_options.qos);
        }

        rhs.servers_options.emplace_back(std::move(server_options));
      }
    }

    return true;
  }
};
}  // namespace YAML

namespace aimrt::plugins::ros2_plugin {

void Ros2RpcBackend::Initialize(YAML::Node options_node,
                                const runtime::core::rpc::RpcRegistry* rpc_registry_ptr) {
  AIMRT_CHECK_ERROR_THROW(
      std::atomic_exchange(&state_, State::Init) == State::PreInit,
      "Ros2 Rpc backend can only be initialized once.");

  if (options_node && !options_node.IsNull())
    options_ = options_node.as<Options>();

  rpc_registry_ptr_ = rpc_registry_ptr;

  options_node = options_;
}

void Ros2RpcBackend::Start() {
  AIMRT_CHECK_ERROR_THROW(
      std::atomic_exchange(&state_, State::Start) == State::Init,
      "Function can only be called when state is 'Init'.");

  for (auto& itr : ros2_adapter_client_map_)
    itr.second->Start();

  for (auto& itr : ros2_adapter_server_map_)
    itr.second->Start();

  for (auto& itr : ros2_adapter_wrapper_client_map_)
    itr.second->Start();

  for (auto& itr : ros2_adapter_wrapper_server_map_)
    itr.second->Start();
}

void Ros2RpcBackend::Shutdown() {
  if (std::atomic_exchange(&state_, State::Shutdown) == State::Shutdown)
    return;

  for (auto& itr : ros2_adapter_client_map_)
    itr.second->Shutdown();

  for (auto& itr : ros2_adapter_server_map_)
    itr.second->Shutdown();

  for (auto& itr : ros2_adapter_wrapper_client_map_)
    itr.second->Shutdown();

  for (auto& itr : ros2_adapter_wrapper_server_map_)
    itr.second->Shutdown();

  ros2_adapter_client_map_.clear();
  ros2_adapter_server_map_.clear();
  ros2_adapter_wrapper_client_map_.clear();
  ros2_adapter_wrapper_server_map_.clear();

  ros2_node_ptr_.reset();
}

rclcpp::QoS Ros2RpcBackend::GetQos(const Options::QosOptions& qos_option) {
  rclcpp::QoS qos(qos_option.depth);

  if (qos_option.history == "keep_last") {
    qos.keep_last(qos_option.depth);
  } else if (qos_option.history == "keep_all") {
    qos.history(rclcpp::HistoryPolicy::KeepAll);
  } else {
    qos.history(rclcpp::HistoryPolicy::SystemDefault);
  }
  if (qos_option.reliability == "reliable") {
    qos.reliability(rclcpp::ReliabilityPolicy::Reliable);
  } else if (qos_option.reliability == "best_effort") {
    qos.reliability(rclcpp::ReliabilityPolicy::BestEffort);
  } else {
    qos.reliability(rclcpp::ReliabilityPolicy::SystemDefault);
  }

  if (qos_option.durability == "volatile") {
    qos.durability(rclcpp::DurabilityPolicy::Volatile);
  } else if (qos_option.durability == "transient_local") {
    qos.durability(rclcpp::DurabilityPolicy::TransientLocal);
  } else {
    qos.durability(rclcpp::DurabilityPolicy::SystemDefault);
  }

  if (qos_option.liveliness == "automatic") {
    qos.liveliness(rclcpp::LivelinessPolicy::Automatic);
  } else if (qos_option.liveliness == "manual_by_topic") {
    qos.liveliness(rclcpp::LivelinessPolicy::ManualByTopic);
  } else {
    qos.liveliness(rclcpp::LivelinessPolicy::SystemDefault);
  }

  if (qos_option.deadline != -1) {
    qos.deadline(rclcpp::Duration::from_nanoseconds(qos_option.deadline * 1000000));
  }

  if (qos_option.lifespan != -1) {
    qos.lifespan(rclcpp::Duration::from_nanoseconds(qos_option.lifespan * 1000000));
  }

  if (qos_option.liveliness_lease_duration != -1) {
    qos.liveliness_lease_duration(rclcpp::Duration::from_nanoseconds(qos_option.liveliness_lease_duration * 1000000));
  }
  return qos;
}

bool Ros2RpcBackend::RegisterServiceFunc(
    const runtime::core::rpc::ServiceFuncWrapper& service_func_wrapper) noexcept {
  if (state_.load() != State::Init) {
    AIMRT_ERROR("Service func can only be registered when state is 'Init'.");
    return false;
  }

  // 前缀是ros2类型的消息
  if (CheckRosFunc(service_func_wrapper.func_name)) {
    if (ros2_adapter_server_map_.find(service_func_wrapper.func_name) !=
        ros2_adapter_server_map_.end()) {
      // 重复注册
      AIMRT_WARN(
          "Service '{}' is registered repeatedly in ros2 rpc backend, module '{}', lib path '{}'",
          service_func_wrapper.func_name,
          service_func_wrapper.module_name,
          service_func_wrapper.pkg_path);
      return false;
    }

    auto ros2_func_name = GetRealRosFuncName(service_func_wrapper.func_name);

    // 读取配置中的QOS
    rclcpp::QoS qos = rclcpp::ServicesQoS();
    auto find_qos_option = std::find_if(options_.servers_options.begin(), options_.servers_options.end(), [&ros2_func_name](const Options::ServerOptions& service_option) {
      try {
        return std::regex_match(ros2_func_name.begin(), ros2_func_name.end(), std::regex(service_option.func_name, std::regex::ECMAScript));
      } catch (const std::exception& e) {
        AIMRT_WARN("Regex get exception, expr: {}, string: {}, exception info: {}",
                   service_option.func_name, ros2_func_name, e.what());
        return false;
      }
    });
    if (find_qos_option != options_.servers_options.end()) {
      qos = GetQos(find_qos_option->qos);
    }
    auto ros2_adapter_server_ptr = std::make_shared<Ros2AdapterServer>(
        ros2_node_ptr_->get_node_base_interface()->get_shared_rcl_node_handle(),
        service_func_wrapper,
        ros2_func_name,
        qos);
    ros2_node_ptr_->get_node_services_interface()->add_service(
        std::dynamic_pointer_cast<rclcpp::ServiceBase>(ros2_adapter_server_ptr),
        nullptr);

    ros2_adapter_server_map_.emplace(service_func_wrapper.func_name,
                                     ros2_adapter_server_ptr);

    AIMRT_INFO("Service '{}' is registered to ros2 rpc backend, ros2 func name is '{}'",
               service_func_wrapper.func_name, ros2_func_name);

    return true;
  }

  // 前缀不是ros2类型的消息
  if (ros2_adapter_wrapper_server_map_.find(service_func_wrapper.func_name) !=
      ros2_adapter_wrapper_server_map_.end()) {
    // 重复注册
    AIMRT_WARN(
        "Service '{}' is registered repeatedly in ros2 rpc backend, module '{}', lib path '{}'",
        service_func_wrapper.func_name,
        service_func_wrapper.module_name,
        service_func_wrapper.pkg_path);
    return false;
  }

  auto ros2_func_name = GetRealRosFuncName(Ros2NameEncode(service_func_wrapper.func_name));

  auto ros2_adapter_wrapper_server_ptr = std::make_shared<Ros2AdapterWrapperServer>(
      ros2_node_ptr_->get_node_base_interface()->get_shared_rcl_node_handle(),
      service_func_wrapper,
      ros2_func_name);
  ros2_node_ptr_->get_node_services_interface()->add_service(
      std::dynamic_pointer_cast<rclcpp::ServiceBase>(ros2_adapter_wrapper_server_ptr),
      nullptr);

  ros2_adapter_wrapper_server_map_.emplace(service_func_wrapper.func_name,
                                           ros2_adapter_wrapper_server_ptr);

  AIMRT_INFO("Service '{}' is registered to ros2 rpc backend, ros2 func name is '{}'",
             service_func_wrapper.func_name, ros2_func_name);

  return true;
}

bool Ros2RpcBackend::RegisterClientFunc(
    const runtime::core::rpc::ClientFuncWrapper& client_func_wrapper) noexcept {
  if (state_.load() != State::Init) {
    AIMRT_ERROR("Client func can only be registered when state is 'Init'.");
    return false;
  }

  // 前缀是ros2类型的消息
  if (CheckRosFunc(client_func_wrapper.func_name)) {
    if (ros2_adapter_client_map_.find(client_func_wrapper.func_name) !=
        ros2_adapter_client_map_.end()) {
      // 重复注册
      AIMRT_WARN(
          "Client '{}' is registered repeatedly in ros2 rpc backend, module '{}', lib path '{}'",
          client_func_wrapper.func_name, client_func_wrapper.module_name,
          client_func_wrapper.pkg_path);
      return false;
    }

    auto ros2_func_name = GetRealRosFuncName(client_func_wrapper.func_name);

    // 读取QOS的配置
    rclcpp::QoS qos(rclcpp::KeepLast(100));
    qos.reliable();                          // 可靠通信
    qos.lifespan(std::chrono::seconds(30));  // 生命周期为 30 秒
    auto find_qos_option = std::find_if(options_.clients_options.begin(), options_.clients_options.end(), [&ros2_func_name](const Options::ClientOptions& client_option) {
      try {
        return std::regex_match(ros2_func_name.begin(), ros2_func_name.end(), std::regex(client_option.func_name, std::regex::ECMAScript));
      } catch (const std::exception& e) {
        AIMRT_WARN("Regex get exception, expr: {}, string: {}, exception info: {}",
                   client_option.func_name, ros2_func_name, e.what());
        return false;
      }
    });
    if (find_qos_option != options_.clients_options.end()) {
      qos = GetQos(find_qos_option->qos);
    }

    auto ros2_adapter_client_ptr = std::make_shared<Ros2AdapterClient>(
        ros2_node_ptr_->get_node_base_interface().get(),
        ros2_node_ptr_->get_node_graph_interface(),
        client_func_wrapper,
        ros2_func_name,
        qos);
    ros2_node_ptr_->get_node_services_interface()->add_client(
        std::dynamic_pointer_cast<rclcpp::ClientBase>(ros2_adapter_client_ptr),
        nullptr);

    ros2_adapter_client_map_.emplace(client_func_wrapper.func_name,
                                     ros2_adapter_client_ptr);

    AIMRT_INFO("Client '{}' is registered to ros2 rpc backend, ros2 func name is '{}'",
               client_func_wrapper.func_name, ros2_func_name);

    return true;
  }

  // 前缀不是ros2类型的消息
  if (ros2_adapter_wrapper_client_map_.find(client_func_wrapper.func_name) !=
      ros2_adapter_wrapper_client_map_.end()) {
    // 重复注册
    AIMRT_WARN(
        "Client '{}' is registered repeatedly in ros2 rpc backend, module '{}', lib path '{}'",
        client_func_wrapper.func_name, client_func_wrapper.module_name,
        client_func_wrapper.pkg_path);
    return false;
  }

  auto ros2_func_name = GetRealRosFuncName(Ros2NameEncode(client_func_wrapper.func_name));

  auto ros2_adapter_wrapper_client_ptr = std::make_shared<Ros2AdapterWrapperClient>(
      ros2_node_ptr_->get_node_base_interface().get(),
      ros2_node_ptr_->get_node_graph_interface(),
      client_func_wrapper,
      ros2_func_name);
  ros2_node_ptr_->get_node_services_interface()->add_client(
      std::dynamic_pointer_cast<rclcpp::ClientBase>(ros2_adapter_wrapper_client_ptr),
      nullptr);

  ros2_adapter_wrapper_client_map_.emplace(client_func_wrapper.func_name,
                                           ros2_adapter_wrapper_client_ptr);

  AIMRT_INFO("Client '{}' is registered to ros2 rpc backend, ros2 func name is '{}'",
             client_func_wrapper.func_name, ros2_func_name);

  return true;
}

bool Ros2RpcBackend::TryInvoke(
    const std::shared_ptr<runtime::core::rpc::ClientInvokeWrapper>& client_invoke_wrapper_ptr) noexcept {
  assert(state_.load() == State::Start);

  // 检查ctx
  std::string_view to_addr =
      client_invoke_wrapper_ptr->ctx_ref.GetMetaValue(AIMRT_RPC_CONTEXT_KEY_TO_ADDR);

  if (!to_addr.empty()) {
    auto pos = to_addr.find("://");
    if (pos == std::string_view::npos) return false;
    if (to_addr.substr(0, pos) != "ros2") return false;
  }

  // 前缀是ros2类型的消息
  if (CheckRosFunc(client_invoke_wrapper_ptr->func_name)) {
    auto finditr = ros2_adapter_client_map_.find(client_invoke_wrapper_ptr->func_name);
    if (finditr == ros2_adapter_client_map_.end()) {
      AIMRT_TRACE(
          "Client '{}' unregistered in ros2 rpc backend, module '{}', lib path '{}'",
          client_invoke_wrapper_ptr->func_name,
          client_invoke_wrapper_ptr->module_name,
          client_invoke_wrapper_ptr->pkg_path);

      return false;
    }

    if (!(finditr->second->service_is_ready())) {
      AIMRT_TRACE("Ros2 service '{}' not ready, module '{}', lib path '{}'",
                  client_invoke_wrapper_ptr->func_name,
                  client_invoke_wrapper_ptr->module_name,
                  client_invoke_wrapper_ptr->pkg_path);

      return false;
    }

    finditr->second->Invoke(client_invoke_wrapper_ptr);

    return true;
  }

  // 前缀不是ros2类型的消息

  auto finditr = ros2_adapter_wrapper_client_map_.find(client_invoke_wrapper_ptr->func_name);
  if (finditr == ros2_adapter_wrapper_client_map_.end()) {
    AIMRT_TRACE(
        "Client '{}' unregistered in ros2 rpc backend, module '{}', lib path '{}'",
        client_invoke_wrapper_ptr->func_name,
        client_invoke_wrapper_ptr->module_name,
        client_invoke_wrapper_ptr->pkg_path);

    return false;
  }

  if (!(finditr->second->service_is_ready())) {
    AIMRT_TRACE("Ros2 service '{}' not ready, module '{}', lib path '{}'",
                client_invoke_wrapper_ptr->func_name,
                client_invoke_wrapper_ptr->module_name,
                client_invoke_wrapper_ptr->pkg_path);

    return false;
  }

  finditr->second->Invoke(client_invoke_wrapper_ptr);

  return true;
}

}  // namespace aimrt::plugins::ros2_plugin