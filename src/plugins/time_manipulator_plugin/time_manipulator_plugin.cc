#include "time_manipulator_plugin/time_manipulator_plugin.h"

#include "aimrt_module_cpp_interface/rpc/rpc_handle.h"
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

bool TimeManipulatorPlugin::Initialize(runtime::core::AimRTCore* core_ptr) noexcept {
  try {
    core_ptr_ = core_ptr;

    core_ptr_->SetGlobal();

    YAML::Node plugin_options_node = core_ptr_->GetPluginManager().GetPluginOptionsNode(Name());

    if (!plugin_options_node || plugin_options_node.IsNull())
      return true;

    options_ = plugin_options_node.as<Options>();

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
    fprintf(stderr, "Initialize failed, %s\n", e.what());
  }

  return false;
}

void TimeManipulatorPlugin::Shutdown() noexcept {
  if (core_ptr_) core_ptr_->UnSetGlobal();

  try {
    if (!init_flag_) return;

  } catch (const std::exception& e) {
    fprintf(stderr, "Shutdown failed, %s\n", e.what());
  }
}

void TimeManipulatorPlugin::SetPluginLogger() {
  SetLogger(aimrt::logger::LoggerRef(core_ptr_->GetLoggerManager().GetLoggerProxy(Name()).NativeHandle()));
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
  service_ptr_ = std::make_shared<TimeManipulatorServiceImpl>();

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
      core_ptr_->GetRpcManager()
          .GetRpcHandleProxy(aimrt::runtime::core::util::ModuleDetailInfo{})
          .NativeHandle());

  bool ret = rpc_handle_ref.RegisterService(service_ptr_.get());
  AIMRT_CHECK_ERROR(ret, "Register service failed.");
}

}  // namespace aimrt::plugins::time_manipulator_plugin
