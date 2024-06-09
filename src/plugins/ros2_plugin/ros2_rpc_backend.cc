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

  static bool decode(const Node& node, Options& rhs) {
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

    auto ros2_adapter_server_ptr = std::make_shared<Ros2AdapterServer>(
        ros2_node_ptr_->get_node_base_interface()->get_shared_rcl_node_handle(),
        service_func_wrapper,
        ros2_func_name);
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

    auto ros2_adapter_client_ptr = std::make_shared<Ros2AdapterClient>(
        ros2_node_ptr_->get_node_base_interface().get(),
        ros2_node_ptr_->get_node_graph_interface(),
        client_func_wrapper,
        ros2_func_name);
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