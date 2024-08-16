#pragma once

#include <atomic>
#include <memory>

#include "zenoh.h"

#include "aimrt_core_plugin_interface/aimrt_core_plugin_base.h"
#include "zenoh_plugin/msg_handle_registry.h"
#include "zenoh_plugin/util.h"
#include "zenoh_plugin/zenoh_channel_backend.h"

namespace aimrt::plugins::zenoh_plugin {
class ZenohPlugin : public AimRTCorePluginBase {
 public:
  // 这个是plgin配置文件的选项 (keyexpr 资源标识符，相匹配的资源标识符会进行通信)
  struct Options {
    std::string keyexpr;
    std::string role;
  };

 public:
  ZenohPlugin() = default;
  ~ZenohPlugin() override = default;

  std::string_view Name() const noexcept override { return "zenoh_plugin"; }

  // 读取配置文件 初始化zenoh相关资源
  bool Initialize(runtime::core::AimRTCore *core_ptr) noexcept override;

  // 用于链接关闭时的释放工作
  void Shutdown() noexcept override;

 private:
  // 注册logger,可以使用相关宏定义
  void SetPluginLogger();

  // 注册一个zenoh channel后端，并提供重链机制（注册一个重链连的回调）
  void RegisterZenohChannelBackend();

  // todo rpc后端
  void RegisterZenohRpcBackend() {}

 private:
  runtime::core::AimRTCore *core_ptr_ = nullptr;

  std::shared_ptr<ZenohUtil> zenoh_util_ptr_ = std::make_shared<ZenohUtil>();

  Options options_;

  bool init_flag_ = false;

  std::atomic_bool stop_flag_ = false;

  std::shared_ptr<MsgHandleRegistry> msg_handle_registry_ptr_;

  std::vector<std::function<void()>> reconnect_hook_;
};

}  // namespace aimrt::plugins::zenoh_plugin
