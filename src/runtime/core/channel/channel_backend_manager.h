#pragma once

#include <atomic>
#include <memory>
#include <string>

#include "aimrt_module_cpp_interface/util/type_support.h"
#include "core/channel/channel_backend_base.h"
#include "core/channel/channel_registry.h"
#include "util/log_util.h"

namespace aimrt::runtime::core::channel {

struct FrameworkHookData {
  std::string_view topic_name;
  std::string_view msg_type;
  std::string_view pkg_path;
  std::string_view module_name;
  aimrt::util::TypeSupportRef type_support_ref;
};

using FrameworkHookFunc = std::function<void(const FrameworkHookData&, aimrt::channel::ContextRef, const void*)>;

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

  void RegisterPublishHook(FrameworkHookFunc&& hook);
  void RegisterSubscribeHook(FrameworkHookFunc&& hook);

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

  std::vector<FrameworkHookFunc> publish_hook_vec_;
  std::vector<FrameworkHookFunc> subscribe_hook_vec_;

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