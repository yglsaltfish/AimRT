#pragma once

#include "RosTestRpc.aimrt_rpc.srv.h"

namespace aimrt::examples::example_ros2_rpc::ros2_rpc_server_module {

class RosTestRpcServiceImpl : public example_ros2::srv::RosTestRpcService {
 public:
  RosTestRpcServiceImpl() = default;
  ~RosTestRpcServiceImpl() override = default;

  aimrt::co::Task<aimrt::rpc::Status> RosTestRpc(
      aimrt::rpc::ContextRef ctx,
      const example_ros2::srv::RosTestRpc_Request& req,
      example_ros2::srv::RosTestRpc_Response& rsp) override;
};

}  // namespace aimrt::examples::example_ros2_rpc::ros2_rpc_server_module
