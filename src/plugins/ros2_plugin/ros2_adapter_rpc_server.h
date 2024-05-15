#pragma once

#include "core/rpc/rpc_backend_base.h"

#include "rclcpp/service.hpp"

namespace aimrt::plugins::ros2_plugin {

class Ros2AdapterServer : public rclcpp::ServiceBase {
 public:
  Ros2AdapterServer(const std::shared_ptr<rcl_node_t>& node_handle,
                    const runtime::core::rpc::ServiceFuncWrapper& service_func_wrapper,
                    const std::string& real_ros2_func_name,
                    runtime::core::rpc::ContextManager* context_manager_ptr);
  ~Ros2AdapterServer() override = default;

  std::shared_ptr<void> create_request() override;
  std::shared_ptr<rmw_request_id_t> create_request_header() override;
  void handle_request(std::shared_ptr<rmw_request_id_t> request_header,
                      std::shared_ptr<void> request) override;

  void Start() { run_flag.store(true); }
  void Shutdown() { run_flag.store(false); }

 private:
  std::atomic_bool run_flag = false;
  const runtime::core::rpc::ServiceFuncWrapper& service_func_wrapper_;
  std::string real_ros2_func_name_;
  runtime::core::rpc::ContextManager* context_manager_ptr_ = nullptr;
};

class Ros2AdapterWrapperServer : public rclcpp::ServiceBase {
 public:
  Ros2AdapterWrapperServer(const std::shared_ptr<rcl_node_t>& node_handle,
                           const runtime::core::rpc::ServiceFuncWrapper& service_func_wrapper,
                           const std::string& real_ros2_func_name,
                           runtime::core::rpc::ContextManager* context_manager_ptr);
  ~Ros2AdapterWrapperServer() override = default;

  std::shared_ptr<void> create_request() override;
  std::shared_ptr<rmw_request_id_t> create_request_header() override;
  void handle_request(std::shared_ptr<rmw_request_id_t> request_header,
                      std::shared_ptr<void> request) override;

  void Start() { run_flag.store(true); }
  void Shutdown() { run_flag.store(false); }

 private:
  void ReturnRspWithStatusCode(std::shared_ptr<rmw_request_id_t> request_header, uint32_t code);

 private:
  std::atomic_bool run_flag = false;
  const runtime::core::rpc::ServiceFuncWrapper& service_func_wrapper_;
  std::string real_ros2_func_name_;
  runtime::core::rpc::ContextManager* context_manager_ptr_ = nullptr;
};

}  // namespace aimrt::plugins::ros2_plugin