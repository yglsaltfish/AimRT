#include "core/channel/channel_backend_manager.h"
#include "core/global.h"
#include "util/stl_tool.h"

namespace aimrt::runtime::core::channel {

void ChannelBackendManager::Initialize(ChannelRegistry* channel_registry_ptr) {
  AIMRT_CHECK_ERROR_THROW(
      std::atomic_exchange(&state_, State::Init) == State::PreInit,
      "Channel backend manager can only be initialized once.");

  channel_registry_ptr_ = channel_registry_ptr;
}

void ChannelBackendManager::Start() {
  AIMRT_CHECK_ERROR_THROW(
      std::atomic_exchange(&state_, State::Start) == State::Init,
      "Function can only be called when state is 'Init'.");

  for (auto& backend : channel_backend_index_vec_) {
    AIMRT_TRACE("Start channel backend '{}'.", backend->Name());
    backend->Start();
  }
}

void ChannelBackendManager::Shutdown() {
  if (std::atomic_exchange(&state_, State::Shutdown) == State::Shutdown)
    return;

  for (auto& backend : channel_backend_index_vec_) {
    AIMRT_TRACE("Shutdown channel backend '{}'.", backend->Name());
    backend->Shutdown();
  }

  channel_backend_index_vec_.clear();
}

void ChannelBackendManager::RegisterChannelBackend(
    ChannelBackendBase* channel_backend_ptr) {
  AIMRT_CHECK_ERROR_THROW(
      state_.load() == State::PreInit,
      "Function can only be called when state is 'PreInit'.");

  channel_backend_index_vec_.emplace_back(channel_backend_ptr);
}

bool ChannelBackendManager::RegisterPublishType(
    PublishTypeWrapper&& publish_type_wrapper) {
  if (state_.load() != State::Init) {
    AIMRT_ERROR("Publish type can only be registered when state is 'Init'.");
    return false;
  }

  auto publish_type_wrapper_ptr =
      std::make_unique<PublishTypeWrapper>(std::move(publish_type_wrapper));
  const auto& publish_type_wrapper_ref = *publish_type_wrapper_ptr;

  if (!channel_registry_ptr_->RegisterPublishType(
          std::move(publish_type_wrapper_ptr)))
    return false;

  bool ret = true;
  for (auto& itr : channel_backend_index_vec_) {
    ret &= itr->RegisterPublishType(publish_type_wrapper_ref);
  }
  return ret;
}

bool ChannelBackendManager::Subscribe(SubscribeWrapper&& subscribe_wrapper) {
  if (state_.load() != State::Init) {
    AIMRT_ERROR("Msg can only be subscribed when state is 'Init'.");
    return false;
  }

  auto subscribe_wrapper_ptr =
      std::make_unique<SubscribeWrapper>(std::move(subscribe_wrapper));
  const auto& subscribe_wrapper_ref = *subscribe_wrapper_ptr;

  if (!channel_registry_ptr_->Subscribe(std::move(subscribe_wrapper_ptr)))
    return false;

  bool ret = true;
  for (auto& itr : channel_backend_index_vec_) {
    ret &= itr->Subscribe(subscribe_wrapper_ref);
  }
  return ret;
}

void ChannelBackendManager::Publish(const PublishWrapper& publish_wrapper) {
  assert(state_.load() == State::Start);

  const auto* publish_type_wrapper_ptr =
      channel_registry_ptr_->GetPublishTypeWrapper(
          publish_wrapper.pkg_path, publish_wrapper.module_name,
          publish_wrapper.topic_name, publish_wrapper.msg_type);

  if (publish_type_wrapper_ptr == nullptr) {
    AIMRT_WARN(
        "Publish type unregistered, pkg_path: {}, module_name: {}, topic_name: {}, msg_type: {}",
        publish_wrapper.pkg_path, publish_wrapper.module_name,
        publish_wrapper.topic_name, publish_wrapper.msg_type);
    return;
  }

  publish_wrapper.msg_type_support = publish_type_wrapper_ptr->msg_type_support;

  for (auto& itr : channel_backend_index_vec_) {
    AIMRT_TRACE("Publish msg '{}' to channel backend '{}'",
                publish_wrapper.msg_type, itr->Name());
    itr->Publish(publish_wrapper);
  }
}

}  // namespace aimrt::runtime::core::channel
