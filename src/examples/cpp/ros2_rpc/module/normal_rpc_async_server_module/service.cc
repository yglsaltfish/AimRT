#include "normal_rpc_async_server_module/service.h"

#include "normal_rpc_async_server_module/global.h"

namespace aimrt::examples::cpp::ros2_rpc::normal_rpc_async_server_module {

void RosTestRpcAsyncServiceImpl::RosTestRpc(
    aimrt::rpc::ContextRef ctx,
    const example_ros2::srv::RosTestRpc_Request& req,
    example_ros2::srv::RosTestRpc_Response& rsp,
    aimrt::util::Function<void(aimrt::rpc::Status)>&& callback) {
  rsp.code = 123;

  AIMRT_INFO("get new rpc call. req:\n{}\nreturn rsp:\n{}",
             example_ros2::srv::to_yaml(req), example_ros2::srv::to_yaml(rsp));

  callback(aimrt::rpc::Status());
}

}  // namespace aimrt::examples::cpp::ros2_rpc::normal_rpc_async_server_module
