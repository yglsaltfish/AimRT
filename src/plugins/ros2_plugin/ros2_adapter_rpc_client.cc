#include "ros2_plugin/ros2_adapter_rpc_client.h"
#include "aimrt_module_cpp_interface/rpc/rpc_status.h"
#include "aimrt_module_cpp_interface/util/buffer.h"
#include "aimrt_module_cpp_interface/util/type_support.h"
#include "ros2_plugin/global.h"

#include "ros2_plugin_proto/srv/ros_rpc_wrapper.hpp"

namespace aimrt::plugins::ros2_plugin {

Ros2AdapterClient::Ros2AdapterClient(
    rclcpp::node_interfaces::NodeBaseInterface* node_base,
    rclcpp::node_interfaces::NodeGraphInterface::SharedPtr node_graph,
    const runtime::core::rpc::ClientFuncWrapper& client_func_wrapper,
    const std::string& real_ros2_func_name,
    const rclcpp::QoS& qos)
    : rclcpp::ClientBase(node_base, node_graph),
      client_func_wrapper_(client_func_wrapper),
      real_ros2_func_name_(real_ros2_func_name) {
  rcl_client_options_t client_options = rcl_client_get_default_options();
  client_options.qos = qos.get_rmw_qos_profile();

  rcl_ret_t ret = rcl_client_init(
      this->get_client_handle().get(),
      this->get_rcl_node_handle(),
      static_cast<const rosidl_service_type_support_t*>(client_func_wrapper_.custom_type_support_ptr),
      real_ros2_func_name_.c_str(),
      &client_options);
  if (ret != RCL_RET_OK) {
    if (ret == RCL_RET_SERVICE_NAME_INVALID) {
      auto rcl_node_handle = this->get_rcl_node_handle();
      // this will throw on any validation problem
      rcl_reset_error();
      rclcpp::expand_topic_or_service_name(
          real_ros2_func_name_,
          rcl_node_get_name(rcl_node_handle),
          rcl_node_get_namespace(rcl_node_handle),
          true);
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
  AIMRT_TRACE("Create ros2 rsp, func name '{}'", client_func_wrapper_.func_name);
  return aimrt::util::TypeSupportRef(client_func_wrapper_.rsp_type_support).CreateSharedPtr();
}

std::shared_ptr<rmw_request_id_t> Ros2AdapterClient::create_request_header() {
  AIMRT_TRACE("Create ros2 req header, func name '{}'", client_func_wrapper_.func_name);
  return std::make_shared<rmw_request_id_t>();
}

void Ros2AdapterClient::handle_response(
    std::shared_ptr<rmw_request_id_t> request_header, std::shared_ptr<void> response) {
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
  rcl_ret_t ret = rcl_send_request(
      get_client_handle().get(), client_invoke_wrapper_ptr->req_ptr, &sequence_number);

  if (RCL_RET_OK != ret) {
    AIMRT_WARN("Ros2 client send req failed, func name '{}', err info: {}",
               client_invoke_wrapper_ptr->func_name, rcl_get_error_string().str);
    rcl_reset_error();
    client_invoke_wrapper_ptr->callback(AIMRT_RPC_STATUS_CLI_SEND_REQ_FAILED);
    return;
  }
  pending_requests_.try_emplace(
      sequence_number,
      CallBackWrapper{.client_invoke_wrapper_ptr = client_invoke_wrapper_ptr});
}

Ros2AdapterWrapperClient::Ros2AdapterWrapperClient(
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
      this->get_client_handle().get(),
      this->get_rcl_node_handle(),
      rosidl_typesupport_cpp::get_service_type_support_handle<ros2_plugin_proto::srv::RosRpcWrapper>(),
      real_ros2_func_name_.c_str(),
      &client_options);
  if (ret != RCL_RET_OK) {
    if (ret == RCL_RET_SERVICE_NAME_INVALID) {
      auto rcl_node_handle = this->get_rcl_node_handle();
      // this will throw on any validation problem
      rcl_reset_error();
      rclcpp::expand_topic_or_service_name(
          real_ros2_func_name_,
          rcl_node_get_name(rcl_node_handle),
          rcl_node_get_namespace(rcl_node_handle),
          true);
    }

    AIMRT_WARN("Create ros2 client failed, func name '{}', err info: {}",
               client_func_wrapper_.func_name, rcl_get_error_string().str);
    rcl_reset_error();
  } else {
    AIMRT_TRACE("Create ros2 client successfully, func name '{}'",
                client_func_wrapper_.func_name);
  }
}

std::shared_ptr<void> Ros2AdapterWrapperClient::create_response() {
  AIMRT_TRACE("Create ros2 rsp, func name '{}'", client_func_wrapper_.func_name);
  return std::make_shared<ros2_plugin_proto::srv::RosRpcWrapper::Response>();
}

std::shared_ptr<rmw_request_id_t> Ros2AdapterWrapperClient::create_request_header() {
  AIMRT_TRACE("Create ros2 req header, func name '{}'", client_func_wrapper_.func_name);
  return std::make_shared<rmw_request_id_t>();
}

void Ros2AdapterWrapperClient::handle_response(
    std::shared_ptr<rmw_request_id_t> request_header, std::shared_ptr<void> response) {
  if (!run_flag.load()) return;

  AIMRT_TRACE("Handle ros2 rsp, func name '{}', seq num '{}'",
              client_func_wrapper_.func_name, request_header->sequence_number);

  auto cb_wrapper = GetAndErasePendingRequest(request_header->sequence_number);
  if (!cb_wrapper) {
    return;
  }

  auto client_invoke_wrapper_ptr = cb_wrapper->client_invoke_wrapper_ptr;

  // client rsp 创建、反序列化
  auto& wrapper_rsp = *(static_cast<ros2_plugin_proto::srv::RosRpcWrapper::Response*>(response.get()));

  if (wrapper_rsp.code) {
    client_invoke_wrapper_ptr->callback(wrapper_rsp.code);
    return;
  }

  aimrt_buffer_view_t buffer_view{
      .data = wrapper_rsp.data.data(),
      .len = wrapper_rsp.data.size()};

  aimrt_buffer_array_view_t buffer_array_view{
      .data = &buffer_view,
      .len = 1};

  auto client_rsp_type_support_ref = aimrt::util::TypeSupportRef(client_func_wrapper_.rsp_type_support);

  bool deserialize_ret = client_rsp_type_support_ref.Deserialize(
      wrapper_rsp.serialization_type, buffer_array_view, client_invoke_wrapper_ptr->rsp_ptr);

  if (!deserialize_ret) {
    // 调用回调
    client_invoke_wrapper_ptr->callback(AIMRT_RPC_STATUS_CLI_DESERIALIZATION_FAILED);
    return;
  }

  client_invoke_wrapper_ptr->callback(AIMRT_RPC_STATUS_OK);
}

void Ros2AdapterWrapperClient::Invoke(
    const std::shared_ptr<runtime::core::rpc::ClientInvokeWrapper>& client_invoke_wrapper_ptr) {
  AIMRT_TRACE("Invoke ros2 req, func name '{}'", client_invoke_wrapper_ptr->func_name);

  // 序列化 client req
  auto serialization_type =
      client_invoke_wrapper_ptr->ctx_ref.GetMetaValue(AIMRT_RPC_CONTEXT_KEY_SERIALIZATION_TYPE);

  // msg buf
  aimrt::util::BufferArray buffer_array;
  auto client_req_type_support_ref = aimrt::util::TypeSupportRef(client_func_wrapper_.req_type_support);

  // client req序列化
  bool serialize_ret = client_req_type_support_ref.Serialize(
      serialization_type, client_invoke_wrapper_ptr->req_ptr, buffer_array.AllocatorNativeHandle(), buffer_array.BufferArrayNativeHandle());

  // 序列化失败一般很少见，此处暂时不做处理
  assert(serialize_ret);

  // 填wrapper_req
  auto buffer_array_data = buffer_array.Data();
  const size_t buffer_array_len = buffer_array.Size();
  size_t req_size = buffer_array.BufferSize();

  ros2_plugin_proto::srv::RosRpcWrapper::Request wrapper_req;
  wrapper_req.serialization_type = serialization_type;
  wrapper_req.data.resize(req_size);

  auto cur_pos = wrapper_req.data.data();
  for (size_t ii = 0; ii < buffer_array_len; ++ii) {
    memcpy(cur_pos, buffer_array_data[ii].data, buffer_array_data[ii].len);
    cur_pos += buffer_array_data[ii].len;
  }

  int64_t sequence_number;
  std::lock_guard<std::mutex> lock(pending_requests_mutex_);
  rcl_ret_t ret = rcl_send_request(
      get_client_handle().get(), &wrapper_req, &sequence_number);

  if (RCL_RET_OK != ret) {
    AIMRT_WARN("Ros2 client send req failed, func name '{}', err info: {}",
               client_invoke_wrapper_ptr->func_name, rcl_get_error_string().str);
    rcl_reset_error();
    client_invoke_wrapper_ptr->callback(AIMRT_RPC_STATUS_CLI_SEND_REQ_FAILED);
    return;
  }
  pending_requests_.try_emplace(
      sequence_number,
      CallBackWrapper{.client_invoke_wrapper_ptr = client_invoke_wrapper_ptr});
}

}  // namespace aimrt::plugins::ros2_plugin