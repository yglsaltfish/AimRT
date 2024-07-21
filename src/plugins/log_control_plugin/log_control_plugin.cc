#include "log_control_plugin/log_control_plugin.h"

#include "aimrt_module_cpp_interface/rpc/rpc_handle.h"
#include "aimrt_module_protobuf_interface/util/protobuf_tools.h"
#include "core/aimrt_core.h"
#include "log_control_plugin/global.h"

namespace YAML {
template <>
struct convert<aimrt::plugins::log_control_plugin::LogControlPlugin::Options> {
  using Options = aimrt::plugins::log_control_plugin::LogControlPlugin::Options;

  static Node encode(const Options& rhs) {
    Node node;

    return node;
  }

  static bool decode(const Node& node, Options& rhs) {
    if (!node.IsMap()) return false;

    return true;
  }
};
}  // namespace YAML

namespace aimrt::plugins::log_control_plugin {

co::Task<aimrt::rpc::Status> DebugLogServerFilter(
    aimrt::rpc::ContextRef ctx, const void* req_ptr, void* rsp_ptr,
    const aimrt::rpc::CoRpcHandle& next) {
  AIMRT_INFO("Svr get new rpc call. req: {}",
             aimrt::Pb2CompactJson(*static_cast<const google::protobuf::Message*>(req_ptr)));
  const auto& status = co_await next(ctx, req_ptr, rsp_ptr);
  AIMRT_INFO("Svr handle rpc completed, status: {}, rsp: {}",
             status.ToString(),
             aimrt::Pb2CompactJson(*static_cast<const google::protobuf::Message*>(rsp_ptr)));
  co_return status;
}

bool LogControlPlugin::Initialize(runtime::core::AimRTCore* core_ptr) noexcept {
  try {
    core_ptr_ = core_ptr;

    YAML::Node plugin_options_node = core_ptr_->GetPluginManager().GetPluginOptionsNode(Name());

    if (plugin_options_node && !plugin_options_node.IsNull()) {
      options_ = plugin_options_node.as<Options>();
    }

    init_flag_ = true;

    core_ptr_->RegisterHookFunc(runtime::core::AimRTCore::State::PostInitLog,
                                [this] { SetPluginLogger(); });

    core_ptr_->RegisterHookFunc(runtime::core::AimRTCore::State::PreInitModules,
                                [this] { RegisterRpcService(); });

    plugin_options_node = options_;
    return true;
  } catch (const std::exception& e) {
    AIMRT_ERROR("Initialize failed, {}", e.what());
  }

  return false;
}

void LogControlPlugin::Shutdown() noexcept {
  try {
    if (!init_flag_) return;

  } catch (const std::exception& e) {
    AIMRT_ERROR("Shutdown failed, {}", e.what());
  }
}

void LogControlPlugin::SetPluginLogger() {
  SetLogger(aimrt::logger::LoggerRef(
      core_ptr_->GetLoggerManager().GetLoggerProxy().NativeHandle()));
}

void LogControlPlugin::RegisterRpcService() {
  // 注册rpc服务
  service_ptr_ = std::make_unique<LogControlServiceImpl>();
  service_ptr_->RegisterFilter(DebugLogServerFilter);

  service_ptr_->SetLoggerManager(&(core_ptr_->GetLoggerManager()));

  auto rpc_handle_ref = aimrt::rpc::RpcHandleRef(
      core_ptr_->GetRpcManager().GetRpcHandleProxy().NativeHandle());

  bool ret = rpc_handle_ref.RegisterService(service_ptr_.get());
  AIMRT_CHECK_ERROR(ret, "Register service failed.");
}

}  // namespace aimrt::plugins::log_control_plugin
