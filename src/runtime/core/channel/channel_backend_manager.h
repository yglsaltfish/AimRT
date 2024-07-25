#pragma once

#include <atomic>
#include <memory>
#include <string>

#include "aimrt_module_cpp_interface/util/type_support.h"
#include "core/channel/channel_backend_base.h"
#include "core/channel/channel_framework_async_filter.h"
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
      : logger_ptr_(std::make_shared<aimrt::common::util::LoggerWrapper>()) {}
  ~ChannelBackendManager() = default;

  ChannelBackendManager(const ChannelBackendManager&) = delete;
  ChannelBackendManager& operator=(const ChannelBackendManager&) = delete;

  void Initialize(ChannelRegistry* channel_registry_ptr);
  void Start();
  void Shutdown();

  void RegisterPublishFilter(FrameworkAsyncChannelFilter&& filter);
  void RegisterSubscribeFilter(FrameworkAsyncChannelFilter&& filter);

  void SetPubTopicsBackendsRules(
      const std::vector<std::pair<std::string, std::vector<std::string>>>& rules);
  void SetSubTopicsBackendsRules(
      const std::vector<std::pair<std::string, std::vector<std::string>>>& rules);

  void RegisterChannelBackend(ChannelBackendBase* channel_backend_ptr);

  bool RegisterPublishType(PublishTypeWrapper&& publish_type_wrapper);
  bool Subscribe(SubscribeWrapper&& subscribe_wrapper);
  void Publish(const PublishWrapper& publish_wrapper);

  State GetState() const { return state_.load(); }

  void SetLogger(const std::shared_ptr<aimrt::common::util::LoggerWrapper>& logger_ptr) { logger_ptr_ = logger_ptr; }
  const aimrt::common::util::LoggerWrapper& GetLogger() const { return *logger_ptr_; }

  std::unordered_map<std::string_view, std::vector<std::string_view>> GetPubTopicBackendInfo() const;
  std::unordered_map<std::string_view, std::vector<std::string_view>> GetSubTopicBackendInfo() const;

 private:
  std::vector<ChannelBackendBase*> GetBackendsByRules(
      std::string_view topic_name,
      const std::vector<std::pair<std::string, std::vector<std::string>>>& rules);

 private:
  std::atomic<State> state_ = State::PreInit;
  std::shared_ptr<aimrt::common::util::LoggerWrapper> logger_ptr_;

  FrameworkAsyncFilterManager publish_filter_manager_;
  FrameworkAsyncFilterManager subscribe_filter_manager_;

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

  std::unordered_map<
      std::string,
      std::vector<ChannelBackendBase*>>
      sub_topics_backend_index_map_;
};

}  // namespace aimrt::runtime::core::channel