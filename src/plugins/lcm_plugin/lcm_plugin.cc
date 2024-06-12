
#include "lcm_plugin.h"
#include "lcm_channel_backend.h"

#include "core/aimrt_core.h"
#include "lcm_plugin/global.h"

namespace YAML {
template <>
struct convert<aimrt::plugins::lcm_plugin::LcmPlugin::Options> {
  using Options = aimrt::plugins::lcm_plugin::LcmPlugin::Options;

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

namespace aimrt::plugins::lcm_plugin {

bool LcmPlugin::Initialize(runtime::core::AimRTCore* core_ptr) noexcept {
  try {
    core_ptr_ = core_ptr;

    init_flag_ = true;

    core_ptr_->RegisterHookFunc(runtime::core::AimRTCore::State::PostInitLog,
                                [this] { SetPluginLogger(); });

    core_ptr_->RegisterHookFunc(runtime::core::AimRTCore::State::PreInitRpc,
                                [this] { RegisterLcmRpcBackend(); });

    core_ptr_->RegisterHookFunc(runtime::core::AimRTCore::State::PreInitChannel,
                                [this] { RegisterLcmChannelBackend(); });

    return true;
  } catch (const std::exception& e) {
    AIMRT_ERROR("Initialize failed, {}", e.what());
  }

  return false;
}

void LcmPlugin::Shutdown() noexcept {
  try {
    if (!init_flag_) return;
  } catch (const std::exception& e) {
    AIMRT_ERROR("Shutdown failed, {}", e.what());
  }
}

void LcmPlugin::SetPluginLogger() {
  SetLogger(aimrt::logger::LoggerRef(
      core_ptr_->GetLoggerManager().GetLoggerProxy().NativeHandle()));
}

void LcmPlugin::RegisterLcmRpcBackend() {
}

void LcmPlugin::RegisterLcmChannelBackend() {
  std::unique_ptr<runtime::core::channel::ChannelBackendBase> lcm_channel_backend_ptr =
      std::make_unique<LcmChannelBackend>();

  static_cast<LcmChannelBackend*>(lcm_channel_backend_ptr.get())
      ->RegisterGetExecutorFunc(
          [this](std::string_view executor_name) -> aimrt::executor::ExecutorRef {
            return core_ptr_->GetExecutorManager().GetExecutor(executor_name);
          });

  core_ptr_->GetChannelManager().RegisterChannelBackend(
      std::move(lcm_channel_backend_ptr));
}

}  // namespace aimrt::plugins::lcm_plugin
