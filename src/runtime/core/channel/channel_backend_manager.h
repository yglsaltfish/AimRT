#pragma once

#include <atomic>
#include <memory>
#include <string>

#include "core/channel/channel_backend_base.h"
#include "core/channel/channel_registry.h"

namespace aimrt::runtime::core::channel {

class ChannelBackendManager {
 public:
  enum class State : uint32_t {
    PreInit,
    Init,
    Start,
    Shutdown,
  };

 public:
  ChannelBackendManager() = default;
  ~ChannelBackendManager() = default;

  ChannelBackendManager(const ChannelBackendManager&) = delete;
  ChannelBackendManager& operator=(const ChannelBackendManager&) = delete;

  void Initialize(ChannelRegistry* channel_registry_ptr);
  void Start();
  void Shutdown();

  void RegisterChannelBackend(ChannelBackendBase* channel_backend_ptr);

  bool RegisterPublishType(PublishTypeWrapper&& publish_type_wrapper);
  bool Subscribe(SubscribeWrapper&& subscribe_wrapper);
  void Publish(const PublishWrapper& publish_wrapper);

  State GetState() const { return state_.load(); }

 private:
  std::atomic<State> state_ = State::PreInit;

  ChannelRegistry* channel_registry_ptr_;

  std::vector<ChannelBackendBase*> channel_backend_index_vec_;
};

}  // namespace aimrt::runtime::core::channel