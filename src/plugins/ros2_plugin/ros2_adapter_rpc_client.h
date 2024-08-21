// Copyright (c) 2023, AgiBot Inc.
// All rights reserved.

#pragma once

#include <mutex>
#include <unordered_map>

#include "core/rpc/rpc_backend_base.h"

#include "rclcpp/client.hpp"

namespace aimrt::plugins::ros2_plugin {

class Ros2AdapterClient : public rclcpp::ClientBase {
 public:
  Ros2AdapterClient(
      rclcpp::node_interfaces::NodeBaseInterface* node_base,
      rclcpp::node_interfaces::NodeGraphInterface::SharedPtr node_graph,
      const runtime::core::rpc::ClientFuncWrapper& client_func_wrapper,
      const std::string& real_ros2_func_name,
      const rclcpp::QoS& qos);
  ~Ros2AdapterClient() override = default;

  std::shared_ptr<void> create_response() override;
  std::shared_ptr<rmw_request_id_t> create_request_header() override;
  void handle_response(std::shared_ptr<rmw_request_id_t> request_header,
                       std::shared_ptr<void> response) override;

  void Invoke(
      const std::shared_ptr<runtime::core::rpc::InvokeWrapper>& client_invoke_wrapper_ptr);

  void Start() { run_flag.store(true); }
  void Shutdown() { run_flag.store(false); }

 private:
  struct CallBackWrapper {
    std::shared_ptr<runtime::core::rpc::InvokeWrapper> client_invoke_wrapper_ptr;
  };

  std::optional<CallBackWrapper> GetAndErasePendingRequest(int64_t request_number) {
    std::lock_guard<std::mutex> lock(pending_requests_mutex_);
    auto finditr = pending_requests_.find(request_number);
    if (finditr == pending_requests_.end()) return std::nullopt;

    auto value = std::move(finditr->second);
    pending_requests_.erase(finditr);
    return value;
  }

 private:
  std::atomic_bool run_flag = false;
  const runtime::core::rpc::ClientFuncWrapper& client_func_wrapper_;
  std::string real_ros2_func_name_;

  std::unordered_map<int64_t, CallBackWrapper> pending_requests_;
  std::mutex pending_requests_mutex_;
};

class Ros2AdapterWrapperClient : public rclcpp::ClientBase {
 public:
  Ros2AdapterWrapperClient(
      rclcpp::node_interfaces::NodeBaseInterface* node_base,
      rclcpp::node_interfaces::NodeGraphInterface::SharedPtr node_graph,
      const runtime::core::rpc::ClientFuncWrapper& client_func_wrapper,
      const std::string& real_ros2_func_name);
  ~Ros2AdapterWrapperClient() override = default;

  std::shared_ptr<void> create_response() override;
  std::shared_ptr<rmw_request_id_t> create_request_header() override;
  void handle_response(std::shared_ptr<rmw_request_id_t> request_header,
                       std::shared_ptr<void> response) override;

  void Invoke(
      const std::shared_ptr<runtime::core::rpc::InvokeWrapper>& client_invoke_wrapper_ptr);

  void Start() { run_flag.store(true); }
  void Shutdown() { run_flag.store(false); }

 private:
  struct CallBackWrapper {
    std::shared_ptr<runtime::core::rpc::InvokeWrapper> client_invoke_wrapper_ptr;
  };

  std::optional<CallBackWrapper> GetAndErasePendingRequest(int64_t request_number) {
    std::lock_guard<std::mutex> lock(pending_requests_mutex_);
    auto finditr = pending_requests_.find(request_number);
    if (finditr == pending_requests_.end()) return std::nullopt;

    auto value = std::move(finditr->second);
    pending_requests_.erase(finditr);
    return value;
  }

 private:
  std::atomic_bool run_flag = false;
  const runtime::core::rpc::ClientFuncWrapper& client_func_wrapper_;
  std::string real_ros2_func_name_;

  std::unordered_map<int64_t, CallBackWrapper> pending_requests_;
  std::mutex pending_requests_mutex_;
};

}  // namespace aimrt::plugins::ros2_plugin