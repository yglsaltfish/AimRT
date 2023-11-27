
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

    core_ptr_->SetGlobal();

    init_flag_ = true;

    core_ptr_->RegisterHookFunc(runtime::core::AimRTCore::State::PostInitLog,
                                [this] { SetPluginLogger(); });

    core_ptr_->RegisterHookFunc(runtime::core::AimRTCore::State::PreInitRpc,
                                [this] { RegisterLcmRpcBackend(); });

    core_ptr_->RegisterHookFunc(runtime::core::AimRTCore::State::PreInitChannel,
                                [this] { RegisterLcmChannelBackend(); });

    return true;
  } catch (const std::exception& e) {
    fprintf(stderr, "Initialize failed, %s\n", e.what());
  }

  return false;
}

void LcmPlugin::Shutdown() noexcept {
  if (core_ptr_) core_ptr_->UnSetGlobal();

  try {
    if (!init_flag_) return;
  } catch (const std::exception& e) {
    fprintf(stderr, "Shutdown failed, %s\n", e.what());
  }
}

void LcmPlugin::SetPluginLogger() {
  SetLogger(aimrt::logger::LoggerRef(core_ptr_->GetLoggerManager().GetLoggerProxy(Name()).NativeHandle()));
}

void LcmPlugin::RegisterLcmRpcBackend() {
}

void LcmPlugin::RegisterLcmChannelBackend() {
  std::unique_ptr<runtime::core::channel::ChannelBackendBase> lcm_channel_backend_ptr =
      std::make_unique<LcmChannelBackend>();

  auto get_executor_func = [this](const std::string_view& executor_name) -> aimrt::executor::ExecutorRef {
    auto ptr = core_ptr_->GetExecutorManager().GetExecutorManagerProxy(runtime::core::util::ModuleDetailInfo{}).GetExecutor(executor_name);
    return ptr ? executor::ExecutorRef(ptr->NativeHandle()) : executor::ExecutorRef();
  };

  static_cast<LcmChannelBackend*>(lcm_channel_backend_ptr.get())
      ->RegisterGetExecutorFunc(get_executor_func);

  core_ptr_->GetChannelManager().RegisterChannelBackend(
      std::move(lcm_channel_backend_ptr));
}

}  // namespace aimrt::plugins::lcm_plugin
