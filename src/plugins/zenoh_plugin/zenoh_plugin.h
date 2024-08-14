#pragma once

#include <atomic>
#include <memory>

#include "zenoh.h"

#include "aimrt_core_plugin_interface/aimrt_core_plugin_base.h"
#include "zenoh_plugin/msg_handle_registry.h"
#include "zenoh_plugin/zenoh_channel_backend.h"

namespace aimrt::plugins::zenoh_plugin {
class ZenohPlugin : public AimRTCorePluginBase {
 public:
  // 这个是plgin配置文件的选项 (keyexpr 资源标识符，相匹配的资源标识符会进行通信)
  struct Options {
    std::string keyexpr;
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

  // 链接丢失后调用重联函数
  void OnConnectLost(const char *cause);

  // 收到消息后调用注册好的回调函数进行处理
  int OnMsgRecv(char *topic, int topic_len, z_loaned_sample_t *sample);

 private:
  runtime::core::AimRTCore *core_ptr_ = nullptr;

  // 通过这两个结构体指针实现调用zenoh的接口
  std::shared_ptr<z_owned_subscriber_t> sub_;
  std::shared_ptr<z_owned_publisher_t> pub_;

  Options options_;

  bool init_flag_ = false;

  std::atomic_bool stop_flag_ = false;

  std::shared_ptr<MsgHandleRegistry> msg_handle_registry_ptr_;

  std::vector<std::function<void()>> reconnect_hook_;
};

}  // namespace aimrt::plugins::zenoh_plugin
