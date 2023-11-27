#include "ros2_plugin/ros2_rpc_backend.h"
#include "ros2_plugin/global.h"

namespace YAML {
template <>
struct convert<aimrt::plugins::ros2_plugin::Ros2RpcBackend::Options> {
  using Options = aimrt::plugins::ros2_plugin::Ros2RpcBackend::Options;

  static Node encode(const Options& rhs) {
    Node node;

    return node;
  }

  static bool decode(const Node& node, Options& rhs) {
    return true;
  }
};
}  // namespace YAML

namespace aimrt::plugins::ros2_plugin {

void Ros2RpcBackend::Initialize(YAML::Node options_node,
                                const runtime::core::rpc::RpcRegistry* rpc_registry_ptr,
                                runtime::core::rpc::ContextManager* context_manager_ptr) {
  AIMRT_CHECK_ERROR_THROW(
      std::atomic_exchange(&state_, State::Init) == State::PreInit,
      "Ros2 Rpc backend can only be initialized once.");

  if (options_node && !options_node.IsNull())
    options_ = options_node.as<Options>();

  rpc_registry_ptr_ = rpc_registry_ptr;
  context_manager_ptr_ = context_manager_ptr;

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
}

void Ros2RpcBackend::Shutdown() {
  if (std::atomic_exchange(&state_, State::Shutdown) == State::Shutdown)
    return;

  for (auto& itr : ros2_adapter_client_map_)
    itr.second->Shutdown();

  for (auto& itr : ros2_adapter_server_map_)
    itr.second->Shutdown();

  ros2_adapter_client_map_.clear();
  ros2_adapter_server_map_.clear();

  ros2_node_ptr_.reset();
}

bool Ros2RpcBackend::RegisterServiceFunc(
    const runtime::core::rpc::ServiceFuncWrapper& service_func_wrapper) noexcept {
  if (state_.load() != State::Init) {
    AIMRT_ERROR("Service func can only be registered when state is 'Init'.");
    return false;
  }

  // 只管前缀是ros2类型的消息
  if (!CheckRosFunc(service_func_wrapper.func_name)) return true;

  if (ros2_adapter_server_map_.find(service_func_wrapper.func_name) !=
      ros2_adapter_server_map_.end()) {
    // 重复注册
    AIMRT_WARN(
        "Service '{}' is registered repeatedly in ros2 rpc backend, module '{}', lib path '{}'",
        service_func_wrapper.func_name, service_func_wrapper.module_name,
        service_func_wrapper.pkg_path);
    return false;
  }

  auto ros2_adapter_server_ptr = std::make_shared<Ros2AdapterServer>(
      ros2_node_ptr_->get_node_base_interface()->get_shared_rcl_node_handle(),
      service_func_wrapper, GetRealRosFuncName(service_func_wrapper.func_name),
      context_manager_ptr_);
  ros2_node_ptr_->get_node_services_interface()->add_service(
      std::dynamic_pointer_cast<rclcpp::ServiceBase>(ros2_adapter_server_ptr),
      nullptr);

  ros2_adapter_server_map_.emplace(service_func_wrapper.func_name,
                                   ros2_adapter_server_ptr);

  return true;
}

bool Ros2RpcBackend::RegisterClientFunc(
    const runtime::core::rpc::ClientFuncWrapper& client_func_wrapper) noexcept {
  if (state_.load() != State::Init) {
    AIMRT_ERROR("Client func can only be registered when state is 'Init'.");
    return false;
  }

  // 只管前缀是ros2类型的消息
  if (!CheckRosFunc(client_func_wrapper.func_name)) return true;

  if (ros2_adapter_client_map_.find(client_func_wrapper.func_name) !=
      ros2_adapter_client_map_.end()) {
    // 重复注册
    AIMRT_WARN(
        "Client '{}' is registered repeatedly in ros2 rpc backend, module '{}', lib path '{}'",
        client_func_wrapper.func_name, client_func_wrapper.module_name,
        client_func_wrapper.pkg_path);
    return false;
  }

  auto ros2_adapter_client_ptr = std::make_shared<Ros2AdapterClient>(
      ros2_node_ptr_->get_node_base_interface().get(),
      ros2_node_ptr_->get_node_graph_interface(), client_func_wrapper,
      GetRealRosFuncName(client_func_wrapper.func_name));
  ros2_node_ptr_->get_node_services_interface()->add_client(
      std::dynamic_pointer_cast<rclcpp::ClientBase>(ros2_adapter_client_ptr),
      nullptr);

  ros2_adapter_client_map_.emplace(client_func_wrapper.func_name,
                                   ros2_adapter_client_ptr);

  return true;
}

bool Ros2RpcBackend::TryInvoke(
    const std::shared_ptr<runtime::core::rpc::ClientInvokeWrapper>& client_invoke_wrapper_ptr) noexcept {
  assert(state_.load() == State::Start);

  // 只管前缀是ros2类型的消息
  if (!CheckRosFunc(client_invoke_wrapper_ptr->func_name)) return false;

  auto finditr =
      ros2_adapter_client_map_.find(client_invoke_wrapper_ptr->func_name);
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

}  // namespace aimrt::plugins::ros2_plugin