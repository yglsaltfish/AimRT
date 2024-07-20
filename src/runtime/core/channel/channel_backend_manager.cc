#include "core/channel/channel_backend_manager.h"

#include <regex>
#include <vector>

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
      "Method can only be called when state is 'Init'.");

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

void ChannelBackendManager::SetPubTopicsBackendsRules(
    const std::vector<std::pair<std::string, std::vector<std::string>>>& rules) {
  AIMRT_CHECK_ERROR_THROW(
      state_.load() == State::PreInit,
      "Method can only be called when state is 'PreInit'.");

  pub_topics_backends_rules_ = rules;
}

void ChannelBackendManager::SetSubTopicsBackendsRules(
    const std::vector<std::pair<std::string, std::vector<std::string>>>& rules) {
  AIMRT_CHECK_ERROR_THROW(
      state_.load() == State::PreInit,
      "Method can only be called when state is 'PreInit'.");

  sub_topics_backends_rules_ = rules;
}

void ChannelBackendManager::RegisterChannelBackend(
    ChannelBackendBase* channel_backend_ptr) {
  AIMRT_CHECK_ERROR_THROW(
      state_.load() == State::PreInit,
      "Method can only be called when state is 'PreInit'.");

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

  if (!channel_registry_ptr_->RegisterPublishType(std::move(publish_type_wrapper_ptr)))
    return false;

  std::string_view topic_name = publish_type_wrapper_ref.topic_name;

  auto backend_itr = pub_topics_backend_index_map_.find(topic_name);
  if (backend_itr == pub_topics_backend_index_map_.end()) {
    auto backend_ptr_vec = GetBackendsByRules(topic_name, pub_topics_backends_rules_);
    auto emplace_ret = pub_topics_backend_index_map_.emplace(topic_name, std::move(backend_ptr_vec));
    backend_itr = emplace_ret.first;
  }

  bool ret = true;
  for (auto& itr : backend_itr->second) {
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

  std::string_view topic_name = subscribe_wrapper_ref.topic_name;

  auto backend_ptr_vec = GetBackendsByRules(topic_name, sub_topics_backends_rules_);

  bool ret = true;
  for (auto& itr : backend_ptr_vec) {
    ret &= itr->Subscribe(subscribe_wrapper_ref);
  }

  sub_topics_backend_index_map_.emplace(topic_name, std::move(backend_ptr_vec));

  return ret;
}

void ChannelBackendManager::Publish(const PublishWrapper& publish_wrapper) {
  if (state_.load() != State::Start) [[unlikely]] {
    AIMRT_WARN("Method can only be called when state is 'Start'.");
    return;
  }

  std::string_view topic_name = publish_wrapper.topic_name;

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

  auto find_itr = pub_topics_backend_index_map_.find(topic_name);

  if (find_itr == pub_topics_backend_index_map_.end()) [[unlikely]] {
    AIMRT_WARN("Topic '{}' has no backend.", topic_name);
    return;
  }

  for (auto& itr : find_itr->second) {
    AIMRT_TRACE("Publish msg '{}' to channel backend '{}'",
                publish_wrapper.msg_type, itr->Name());
    itr->Publish(publish_wrapper);
  }
}

std::vector<ChannelBackendBase*> ChannelBackendManager::GetBackendsByRules(
    std::string_view topic_name,
    const std::vector<std::pair<std::string, std::vector<std::string>>>& rules) {
  for (const auto& item : rules) {
    const auto& topic_regex = item.first;
    const auto& enable_backends = item.second;

    try {
      if (std::regex_match(topic_name.begin(), topic_name.end(), std::regex(topic_regex, std::regex::ECMAScript))) {
        std::vector<ChannelBackendBase*> backend_ptr_vec;

        for (const auto& backend_name : enable_backends) {
          auto itr = std::find_if(
              channel_backend_index_vec_.begin(), channel_backend_index_vec_.end(),
              [&backend_name](const ChannelBackendBase* backend_ptr) -> bool {
                return backend_ptr->Name() == backend_name;
              });

          if (itr == channel_backend_index_vec_.end()) [[unlikely]] {
            AIMRT_WARN("Can not find '{}' in backend list.", backend_name);
            continue;
          }

          backend_ptr_vec.emplace_back(*itr);
        }

        return backend_ptr_vec;
      }
    } catch (const std::exception& e) {
      AIMRT_WARN("Regex get exception, expr: {}, string: {}, exception info: {}",
                 topic_regex, topic_name, e.what());
    }
  }

  return {};
}

std::unordered_map<std::string_view, std::vector<std::string_view>>
ChannelBackendManager::GetPubTopicBackendInfo() const {
  std::unordered_map<std::string_view, std::vector<std::string_view>> result;
  for (auto& itr : pub_topics_backend_index_map_) {
    std::vector<std::string_view> backends_name;
    for (auto& item : itr.second)
      backends_name.emplace_back(item->Name());

    result.emplace(itr.first, std::move(backends_name));
  }

  return result;
}

std::unordered_map<std::string_view, std::vector<std::string_view>>
ChannelBackendManager::GetSubTopicBackendInfo() const {
  std::unordered_map<std::string_view, std::vector<std::string_view>> result;
  for (auto& itr : sub_topics_backend_index_map_) {
    std::vector<std::string_view> backends_name;
    for (auto& item : itr.second)
      backends_name.emplace_back(item->Name());

    result.emplace(itr.first, std::move(backends_name));
  }

  return result;
}

}  // namespace aimrt::runtime::core::channel
