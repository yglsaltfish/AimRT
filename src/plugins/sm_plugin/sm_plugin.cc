
#include "sm_plugin.h"
#include "sm_channel_backend.h"
#include "sm_rpc_backend.h"

#include "core/aimrt_core.h"
#include "sm_plugin/global.h"

namespace YAML {
template <>
struct convert<aimrt::plugins::sm_plugin::SmPlugin::Options> {
  using Options = aimrt::plugins::sm_plugin::SmPlugin::Options;

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

namespace aimrt::plugins::sm_plugin {

bool SmPlugin::Initialize(runtime::core::AimRTCore* core_ptr) noexcept {
  try {
    core_ptr_ = core_ptr;

    init_flag_ = true;

    core_ptr_->RegisterHookFunc(runtime::core::AimRTCore::HookPoint::PostInitLog,
                                [this] { SetPluginLogger(); });

    core_ptr_->RegisterHookFunc(runtime::core::AimRTCore::HookPoint::PreInitRpc,
                                [this] { RegisterSmRpcBackend(); });

    core_ptr_->RegisterHookFunc(runtime::core::AimRTCore::HookPoint::PreInitChannel,
                                [this] { RegisterSmChannelBackend(); });

    return true;
  } catch (const std::exception& e) {
    fprintf(stderr, "Initialize failed, %s\n", e.what());
  }

  return false;
}

void SmPlugin::Shutdown() noexcept {
  try {
    init_flag_ = false;
  } catch (const std::exception& e) {
    fprintf(stderr, "Shutdown failed, %s\n", e.what());
  }
}

void SmPlugin::SetPluginLogger() {
  SetLogger(LoggerRef(core_ptr_->GetLoggerManager().GetLoggerProxy(Name()).NativeHandle()));
}

void SmPlugin::RegisterSmRpcBackend() {
  std::unique_ptr<runtime::core::rpc::RpcBackendBase> sm_rpc_backend_ptr =
      std::make_unique<SmRpcBackend>();

  // static_cast<SmRpcBackend*>(sm_rpc_backend_ptr.get())
  //     ->SetNodePtr(sm_node_ptr_);

  core_ptr_->GetRpcManager().RegisterRpcBackend(
      std::move(sm_rpc_backend_ptr));
}

void SmPlugin::RegisterSmChannelBackend() {
  std::unique_ptr<runtime::core::channel::ChannelBackendBase> sm_channel_backend_ptr =
      std::make_unique<SmChannelBackend>();

  auto get_executor_func = [this](const std::string_view& executor_name) -> aimrt::ExecutorRef {
    auto ptr = core_ptr_->GetExecutorManager().GetExecutorManagerProxy(runtime::core::util::ModuleDetailInfo{}).GetExecutor(executor_name);
    return ptr ? ExecutorRef(ptr->NativeHandle()) : ExecutorRef();
  };

  static_cast<SmChannelBackend*>(sm_channel_backend_ptr.get())
      ->RegisterGetExecutorFunc(get_executor_func);

  core_ptr_->GetChannelManager().RegisterChannelBackend(
      std::move(sm_channel_backend_ptr));
}

}  // namespace aimrt::plugins::sm_plugin
