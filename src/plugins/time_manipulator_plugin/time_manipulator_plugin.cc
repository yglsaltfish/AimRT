#include "time_manipulator_plugin/time_manipulator_plugin.h"

#include "aimrt_module_cpp_interface/rpc/rpc_handle.h"
#include "aimrt_module_protobuf_interface/util/protobuf_tools.h"
#include "core/aimrt_core.h"
#include "time_manipulator_plugin/global.h"

namespace YAML {
template <>
struct convert<aimrt::plugins::time_manipulator_plugin::TimeManipulatorPlugin::Options> {
  using Options = aimrt::plugins::time_manipulator_plugin::TimeManipulatorPlugin::Options;

  static Node encode(const Options& rhs) {
    Node node;

    node["enable_rpc"] = rhs.enable_rpc;

    return node;
  }

  static bool decode(const Node& node, Options& rhs) {
    if (!node.IsMap()) return false;

    rhs.enable_rpc = node["enable_rpc"].as<bool>();

    return true;
  }
};
}  // namespace YAML

namespace aimrt::plugins::time_manipulator_plugin {

co::Task<aimrt::rpc::Status> DebugLogServerFilter(
    aimrt::rpc::ContextRef ctx, const void* req_ptr, void* rsp_ptr,
    const aimrt::rpc::RpcHandle& next) {
  AIMRT_INFO("Svr get new rpc call. req: {}",
             aimrt::Pb2CompactJson(*static_cast<const google::protobuf::Message*>(req_ptr)));
  const auto& status = co_await next(ctx, req_ptr, rsp_ptr);
  AIMRT_INFO("Svr handle rpc completed, status: {}, rsp: {}",
             status.ToString(),
             aimrt::Pb2CompactJson(*static_cast<const google::protobuf::Message*>(rsp_ptr)));
  co_return status;
}

bool TimeManipulatorPlugin::Initialize(runtime::core::AimRTCore* core_ptr) noexcept {
  try {
    core_ptr_ = core_ptr;

    YAML::Node plugin_options_node = core_ptr_->GetPluginManager().GetPluginOptionsNode(Name());

    if (plugin_options_node && !plugin_options_node.IsNull()) {
      options_ = plugin_options_node.as<Options>();
    }

    init_flag_ = true;

    core_ptr_->RegisterHookFunc(runtime::core::AimRTCore::State::PostInitLog,
                                [this] { SetPluginLogger(); });

    core_ptr_->RegisterHookFunc(runtime::core::AimRTCore::State::PreInitExecutor,
                                [this] { RegisterTimeManipulatorExecutor(); });

    if (options_.enable_rpc) {
      core_ptr_->RegisterHookFunc(runtime::core::AimRTCore::State::PreInitModules,
                                  [this] { RegisterRpcService(); });
    }

    plugin_options_node = options_;
    return true;
  } catch (const std::exception& e) {
    AIMRT_ERROR("Initialize failed, {}", e.what());
  }

  return false;
}

void TimeManipulatorPlugin::Shutdown() noexcept {
  try {
    if (!init_flag_) return;

  } catch (const std::exception& e) {
    AIMRT_ERROR("Shutdown failed, {}", e.what());
  }
}

void TimeManipulatorPlugin::SetPluginLogger() {
  SetLogger(aimrt::logger::LoggerRef(
      core_ptr_->GetLoggerManager().GetLoggerProxy().NativeHandle()));
}

void TimeManipulatorPlugin::RegisterTimeManipulatorExecutor() {
  core_ptr_->GetExecutorManager().RegisterExecutorGenFunc(
      "time_manipulator",
      [this]() -> std::unique_ptr<aimrt::runtime::core::executor::ExecutorBase> {
        auto ptr = std::make_unique<TimeManipulatorExecutor>();
        ptr->RegisterGetExecutorFunc(
            [this](std::string_view executor_name) -> aimrt::executor::ExecutorRef {
              return core_ptr_->GetExecutorManager().GetExecutor(executor_name);
            });
        return ptr;
      });
}

void TimeManipulatorPlugin::RegisterRpcService() {
  // 注册rpc服务
  service_ptr_ = std::make_unique<TimeManipulatorServiceImpl>();
  service_ptr_->RegisterFilter(DebugLogServerFilter);

  auto& executor_vec = core_ptr_->GetExecutorManager().GetAllExecutors();

  for (auto& itr : executor_vec) {
    if (itr->Type() == "time_manipulator") {
      auto ptr = dynamic_cast<TimeManipulatorExecutor*>(itr.get());
      if (ptr) {
        service_ptr_->RegisterTimeManipulatorExecutor(ptr);
      } else {
        AIMRT_ERROR("Invalid executor pointer");
      }
    }
  }

  auto rpc_handle_ref = aimrt::rpc::RpcHandleRef(
      core_ptr_->GetRpcManager().GetRpcHandleProxy().NativeHandle());

  bool ret = rpc_handle_ref.RegisterService(service_ptr_.get());
  AIMRT_CHECK_ERROR(ret, "Register service failed.");
}

}  // namespace aimrt::plugins::time_manipulator_plugin
