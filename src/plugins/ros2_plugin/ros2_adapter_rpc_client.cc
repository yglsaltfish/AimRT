#include "ros2_plugin/ros2_adapter_rpc_client.h"
#include "aimrt_module_cpp_interface/rpc/rpc_status.h"
#include "aimrt_module_cpp_interface/util/type_support.h"
#include "ros2_plugin/global.h"

namespace aimrt::plugins::ros2_plugin {

Ros2AdapterClient::Ros2AdapterClient(
    rclcpp::node_interfaces::NodeBaseInterface* node_base,
    rclcpp::node_interfaces::NodeGraphInterface::SharedPtr node_graph,
    const runtime::core::rpc::ClientFuncWrapper& client_func_wrapper,
    const std::string& real_ros2_func_name)
    : rclcpp::ClientBase(node_base, node_graph),
      client_func_wrapper_(client_func_wrapper),
      real_ros2_func_name_(real_ros2_func_name) {
  rclcpp::QoS qos(rclcpp::KeepLast(1000));
  qos.reliable();                          // 可靠通信
  qos.lifespan(std::chrono::seconds(30));  // 生命周期为 30 秒

  rcl_client_options_t client_options = rcl_client_get_default_options();
  client_options.qos = qos.get_rmw_qos_profile();

  rcl_ret_t ret = rcl_client_init(
      this->get_client_handle().get(), this->get_rcl_node_handle(),
      static_cast<const rosidl_service_type_support_t*>(
          client_func_wrapper_.custom_type_support_ptr),
      real_ros2_func_name_.c_str(), &client_options);
  if (ret != RCL_RET_OK) {
    if (ret == RCL_RET_SERVICE_NAME_INVALID) {
      auto rcl_node_handle = this->get_rcl_node_handle();
      // this will throw on any validation problem
      rcl_reset_error();
      rclcpp::expand_topic_or_service_name(
          real_ros2_func_name_, rcl_node_get_name(rcl_node_handle),
          rcl_node_get_namespace(rcl_node_handle), true);
    }

    AIMRT_WARN("Create ros2 client failed, func name '{}', err info: {}",
               client_func_wrapper_.func_name, rcl_get_error_string().str);
    rcl_reset_error();
  } else {
    AIMRT_TRACE("Create ros2 client successfully, func name '{}'",
                client_func_wrapper_.func_name);
  }
}

std::shared_ptr<void> Ros2AdapterClient::create_response() {
  AIMRT_TRACE("Create ros2 rsp, func name '{}'",
              client_func_wrapper_.func_name);
  return aimrt::util::TypeSupportRef(client_func_wrapper_.rsp_type_support).CreateSharedPtr();
}

std::shared_ptr<rmw_request_id_t> Ros2AdapterClient::create_request_header() {
  AIMRT_TRACE("Create ros2 req header, func name '{}'",
              client_func_wrapper_.func_name);
  return std::make_shared<rmw_request_id_t>();
}

void Ros2AdapterClient::handle_response(
    std::shared_ptr<rmw_request_id_t> request_header,
    std::shared_ptr<void> response) {
  if (!run_flag.load()) return;

  AIMRT_TRACE("Handle ros2 rsp, func name '{}', seq num '{}'",
              client_func_wrapper_.func_name, request_header->sequence_number);

  auto cb_wrapper = GetAndErasePendingRequest(request_header->sequence_number);
  if (!cb_wrapper) {
    return;
  }

  auto client_invoke_wrapper_ptr = cb_wrapper->client_invoke_wrapper_ptr;
  aimrt::util::TypeSupportRef(client_func_wrapper_.rsp_type_support)
      .Move(response.get(), client_invoke_wrapper_ptr->rsp_ptr);
  client_invoke_wrapper_ptr->callback(AIMRT_RPC_STATUS_OK);
}

void Ros2AdapterClient::Invoke(
    const std::shared_ptr<runtime::core::rpc::ClientInvokeWrapper>& client_invoke_wrapper_ptr) {
  AIMRT_TRACE("Invoke ros2 req, func name '{}'",
              client_invoke_wrapper_ptr->func_name);

  int64_t sequence_number;
  std::lock_guard<std::mutex> lock(pending_requests_mutex_);
  rcl_ret_t ret =
      rcl_send_request(get_client_handle().get(),
                       client_invoke_wrapper_ptr->req_ptr, &sequence_number);
  if (RCL_RET_OK != ret) {
    AIMRT_WARN("Ros2 client send req failed, func name '{}', err info: {}",
               client_invoke_wrapper_ptr->func_name,
               rcl_get_error_string().str);
    rcl_reset_error();
    client_invoke_wrapper_ptr->callback(AIMRT_RPC_STATUS_CLI_SEND_REQ_FAILED);
    return;
  }
  pending_requests_.try_emplace(
      sequence_number,
      CallBackWrapper{.client_invoke_wrapper_ptr = client_invoke_wrapper_ptr});
  return;
}

}  // namespace aimrt::plugins::ros2_plugin