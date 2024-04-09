#include "normal_rpc_server_module/normal_rpc_server_module.h"
#include "normal_rpc_server_module/filter.h"
#include "normal_rpc_server_module/global.h"

namespace aimrt::examples::cpp::protobuf_rpc::normal_rpc_server_module {

bool NormalRpcServerModule::Initialize(aimrt::CoreRef core) noexcept {
  core_ = core;

  SetLogger(core_.GetLogger());

  try {
    // 注册rpc服务
    service_ptr_ = std::make_shared<ExampleServiceImpl>();

    // Register filter
    service_ptr_->RegisterFilter(DebugLogServerFilter);
    service_ptr_->RegisterFilter(TimeCostLogServerFilter);

    bool ret = core_.GetRpcHandle().RegisterService(service_ptr_.get());
    AIMRT_CHECK_ERROR_THROW(ret, "Register service failed.");

    AIMRT_INFO("Register service succeeded.");

  } catch (const std::exception& e) {
    AIMRT_ERROR("Init failed, {}", e.what());
    return false;
  }

  AIMRT_INFO("Init succeeded.");

  return true;
}

bool NormalRpcServerModule::Start() noexcept { return true; }

void NormalRpcServerModule::Shutdown() noexcept {}

}  // namespace aimrt::examples::cpp::protobuf_rpc::normal_rpc_server_module
