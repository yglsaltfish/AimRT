// Copyright (c) 2023, AgiBot Inc.
// All rights reserved.

#pragma once

#include "core/channel/channel_backend_base.h"
#include "zenoh_plugin/msg_handle_registry.h"
#include "zenoh_plugin/zenoh_manager.h"

namespace aimrt::plugins::zenoh_plugin {

class ZenohChannelBackend : public runtime::core::channel::ChannelBackendBase {
 public:
  // todo 可选择一些需要用户配置的后端选项用于通信时的配置，暂时使用zenoh默认配置
  struct Options {};

 public:
  ZenohChannelBackend(
      std::shared_ptr<ZenohManager>& zenoh_util_ptr,
      std::shared_ptr<MsgHandleRegistry> msg_handle_registry_ptr)
      : zenoh_manager_ptr_(zenoh_util_ptr),
        msg_handle_registry_ptr_(msg_handle_registry_ptr) {}

  ~ZenohChannelBackend() override = default;

  std::string_view Name() const override { return "zenoh"; }

  void Initialize(YAML::Node options_node) override;
  void Start() override;
  void Shutdown() override;

  void SetChannelRegistry(const runtime::core::channel::ChannelRegistry* channel_registry_ptr) override {
    channel_registry_ptr_ = channel_registry_ptr;
  }

  bool RegisterPublishType(
      const runtime::core::channel::PublishTypeWrapper& publish_type_wrapper) noexcept override;
  bool Subscribe(const runtime::core::channel::SubscribeWrapper& subscribe_wrapper) noexcept override;
  void Publish(runtime::core::channel::MsgWrapper& msg_wrapper) noexcept override;

 private:
  enum class State : uint32_t {
    PreInit,
    Init,
    Start,
    Shutdown,
  };

  Options options_;
  std::atomic<State> state_ = State::PreInit;

  const runtime::core::channel::ChannelRegistry* channel_registry_ptr_ = nullptr;

  std::shared_ptr<ZenohManager> zenoh_manager_ptr_;
  std::shared_ptr<MsgHandleRegistry> msg_handle_registry_ptr_;

  std::unordered_map<
      std::string,
      std::unique_ptr<std::vector<const runtime::core::channel::SubscribeWrapper*>>>
      subscribe_wrapper_map_;
};

}  // namespace aimrt::plugins::zenoh_plugin