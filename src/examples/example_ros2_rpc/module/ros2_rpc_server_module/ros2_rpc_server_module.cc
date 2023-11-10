#include "ros2_rpc_server_module/ros2_rpc_server_module.h"
#include "ros2_rpc_server_module/filter.h"
#include "ros2_rpc_server_module/global.h"

namespace aimrt::examples::example_ros2_rpc::ros2_rpc_server_module {

bool Ros2RpcServerModule::Initialize(aimrt::CoreRef core) noexcept {
  core_ = core;

  SetLogger(core_.GetLogger());

  try {
    // 注册rpc服务
    service_ptr_ = std::make_shared<RosTestRpcServiceImpl>();

    // Register filter
    service_ptr_->RegisterFilter(TimeCostLogServerFilter);

    bool ret = core_.GetRpcHandle().RegisterService(service_ptr_);
    AIMRT_CHECK_ERROR_THROW(ret, "Register service failed.");

    AIMRT_INFO("Register service succeeded.");

  } catch (const std::exception& e) {
    AIMRT_ERROR("Init failed, {}", e.what());
    return false;
  }

  AIMRT_INFO("Init succeeded.");

  return true;
}

bool Ros2RpcServerModule::Start() noexcept { return true; }

void Ros2RpcServerModule::Shutdown() noexcept {}

}  // namespace aimrt::examples::example_ros2_rpc::ros2_rpc_server_module
