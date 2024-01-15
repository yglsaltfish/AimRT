#include "ros2_plugin/ros2_adapter_rpc_server.h"
#include "aimrt_module_cpp_interface/util/type_support.h"
#include "ros2_plugin/global.h"

namespace aimrt::plugins::ros2_plugin {

Ros2AdapterServer::Ros2AdapterServer(
    const std::shared_ptr<rcl_node_t>& node_handle,
    const runtime::core::rpc::ServiceFuncWrapper& service_func_wrapper,
    const std::string& real_ros2_func_name, runtime::core::rpc::ContextManager* context_manager_ptr)
    : rclcpp::ServiceBase(node_handle),
      service_func_wrapper_(service_func_wrapper),
      real_ros2_func_name_(real_ros2_func_name),
      context_manager_ptr_(context_manager_ptr) {
  // rcl does the static memory allocation here
  service_handle_ = std::shared_ptr<rcl_service_t>(
      new rcl_service_t, [node_handle](rcl_service_t* service) {
        if (rcl_service_fini(service, node_handle.get()) != RCL_RET_OK) {
          RCLCPP_ERROR(
              rclcpp::get_node_logger(node_handle.get()).get_child("rclcpp"),
              "Error in destruction of rcl service handle: %s",
              rcl_get_error_string().str);
          rcl_reset_error();
        }
        delete service;
      });
  *service_handle_.get() = rcl_get_zero_initialized_service();

  rcl_service_options_t service_options = rcl_service_get_default_options();
  service_options.qos = rclcpp::ServicesQoS().get_rmw_qos_profile();
  rcl_ret_t ret =
      rcl_service_init(service_handle_.get(), node_handle.get(),
                       static_cast<const rosidl_service_type_support_t*>(
                           service_func_wrapper.custom_type_support_ptr),
                       real_ros2_func_name_.c_str(), &service_options);
  if (ret != RCL_RET_OK) {
    if (ret == RCL_RET_SERVICE_NAME_INVALID) {
      auto rcl_node_handle = get_rcl_node_handle();
      // this will throw on any validation problem
      rcl_reset_error();
      rclcpp::expand_topic_or_service_name(
          real_ros2_func_name_, rcl_node_get_name(rcl_node_handle),
          rcl_node_get_namespace(rcl_node_handle), true);
    }

    AIMRT_WARN("Create ros2 service failed, func name '{}', err info: {}",
               service_func_wrapper_.func_name, rcl_get_error_string().str);
    rcl_reset_error();

  } else {
    AIMRT_TRACE("Create ros2 service successfully, func name '{}'",
                service_func_wrapper_.func_name);
  }
}

std::shared_ptr<void> Ros2AdapterServer::create_request() {
  AIMRT_TRACE("Create ros2 req, func name '{}'", service_func_wrapper_.func_name);
  return aimrt::util::TypeSupportRef(service_func_wrapper_.req_type_support).CreateSharedPtr();
}

std::shared_ptr<rmw_request_id_t> Ros2AdapterServer::create_request_header() {
  AIMRT_TRACE("Create ros2 req header, func name '{}'",
              service_func_wrapper_.func_name);
  return std::make_shared<rmw_request_id_t>();
}

void Ros2AdapterServer::handle_request(
    std::shared_ptr<rmw_request_id_t> request_header,
    std::shared_ptr<void> request) {
  if (!run_flag.load()) return;

  AIMRT_TRACE("Handle ros2 req, func name '{}', seq num '{}'",
              service_func_wrapper_.func_name,
              request_header->sequence_number);

  // ctx 创建
  std::shared_ptr<runtime::core::rpc::ContextImpl> ctx_ptr(
      context_manager_ptr_->NewContext(),
      [context_manager_ptr{context_manager_ptr_}](runtime::core::rpc::ContextImpl* ptr) {
        context_manager_ptr->DeleteContext(ptr);
      });

  // service rsp 创建
  std::shared_ptr<void> service_rsp_ptr = aimrt::util::TypeSupportRef(service_func_wrapper_.rsp_type_support).CreateSharedPtr();

  aimrt::util::Function<aimrt_function_service_callback_ops_t> service_callback(
      [this, service_rsp_ptr, ctx_ptr, request, request_header](uint32_t code) {
        AIMRT_TRACE("Handle ros2 req completed, func name '{}', seq num '{}'",
                    service_func_wrapper_.func_name,
                    request_header->sequence_number);

        // 发送rsp
        rcl_ret_t ret = rcl_send_response(
            service_handle_.get(), request_header.get(), service_rsp_ptr.get());

        if (ret != RCL_RET_OK) {
          AIMRT_WARN("Send ros2 rsp failed, func name '{}', err info: {}",
                     service_func_wrapper_.func_name,
                     rcl_get_error_string().str);
          rcl_reset_error();
        }
      });
  service_func_wrapper_.service_func(ctx_ptr->NativeHandle(), request.get(),
                                     service_rsp_ptr.get(),
                                     service_callback.NativeHandle());
}

}  // namespace aimrt::plugins::ros2_plugin