#pragma once

#include <atomic>
#include <memory>
#include <string>

#include "core/channel/channel_backend_base.h"
#include "core/channel/channel_registry.h"
#include "util/log_util.h"

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
  ChannelBackendManager()
      : logger_ptr_(std::make_shared<common::util::LoggerWrapper>()) {}
  ~ChannelBackendManager() = default;

  ChannelBackendManager(const ChannelBackendManager&) = delete;
  ChannelBackendManager& operator=(const ChannelBackendManager&) = delete;

  void Initialize(ChannelRegistry* channel_registry_ptr);
  void Start();
  void Shutdown();

  void SetPubTopicsBackendsRules(
      const std::vector<std::pair<std::string, std::vector<std::string>>>& rules);
  void SetSubTopicsBackendsRules(
      const std::vector<std::pair<std::string, std::vector<std::string>>>& rules);

  void RegisterChannelBackend(ChannelBackendBase* channel_backend_ptr);

  bool RegisterPublishType(PublishTypeWrapper&& publish_type_wrapper);
  bool Subscribe(SubscribeWrapper&& subscribe_wrapper);
  void Publish(const PublishWrapper& publish_wrapper);

  State GetState() const { return state_.load(); }

  void SetLogger(const std::shared_ptr<common::util::LoggerWrapper>& logger_ptr) { logger_ptr_ = logger_ptr; }
  const common::util::LoggerWrapper& GetLogger() const { return *logger_ptr_; }

 private:
  std::vector<ChannelBackendBase*> GetBackendsByRules(
      std::string_view topic_name,
      const std::vector<std::pair<std::string, std::vector<std::string>>>& rules);

 private:
  std::atomic<State> state_ = State::PreInit;
  std::shared_ptr<common::util::LoggerWrapper> logger_ptr_;

  ChannelRegistry* channel_registry_ptr_;

  std::vector<ChannelBackendBase*> channel_backend_index_vec_;

  std::vector<std::pair<std::string, std::vector<std::string>>> pub_topics_backends_rules_;
  std::vector<std::pair<std::string, std::vector<std::string>>> sub_topics_backends_rules_;
  std::unordered_map<
      std::string,
      std::vector<ChannelBackendBase*>,
      aimrt::common::util::StringHash,
      std::equal_to<>>
      pub_topics_backend_index_map_;
};

}  // namespace aimrt::runtime::core::channel