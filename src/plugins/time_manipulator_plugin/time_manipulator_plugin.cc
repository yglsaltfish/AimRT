#include "time_manipulator_plugin/time_manipulator_plugin.h"
#include "time_manipulator_plugin/time_manipulator_executor.h"

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
        auto get_executor_func = [this](std::string_view executor_name) -> aimrt::executor::ExecutorRef {
          auto executor_manager =
              core_ptr_->GetExecutorManager().GetExecutorManagerProxy(runtime::core::util::ModuleDetailInfo{}).NativeHandle();

          return aimrt::executor::ExecutorRef(
              executor_manager->get_executor(
                  executor_manager->impl, aimrt::util::ToAimRTStringView(executor_name)));
        };

        auto ptr = std::make_unique<TimeManipulatorExecutor>();
        ptr->RegisterGetExecutorFunc(get_executor_func);
        return ptr;
      });
}

}  // namespace aimrt::plugins::time_manipulator_plugin
