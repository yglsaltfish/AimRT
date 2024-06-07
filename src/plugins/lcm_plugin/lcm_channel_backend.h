

#pragma once

#include <memory>
#include <string>
#include <unordered_map>

#include "lcm_dispatcher.h"

#include "aimrt_module_cpp_interface/executor/executor.h"
#include "aimrt_module_cpp_interface/util/function.h"
#include "core/channel/channel_backend_base.h"

namespace aimrt::plugins::lcm_plugin {

class LcmChannelBackend : public runtime::core::channel::ChannelBackendBase {
 public:
  struct SubTopicOptions {
    std::string topic_name;                                  // subscriber topic name
    std::string executor;                                    // subscriber callback executor name
    std::string lcm_url;                                     // lcm url
    std::string lcm_dispatcher_executor;                     // lcm dispatcher executor name
    int32_t priority = std::numeric_limits<int32_t>::max();  // priority, number is bigger, priority is lower, must be >= 0
  };

  struct PubTopicOptions {
    std::string topic_name;                                  // subscriber topic name
    std::string lcm_url;                                     // lcm url
    int32_t priority = std::numeric_limits<int32_t>::max();  // priority, number is bigger, priority is lower, must be >= 0
  };

  struct Options {
    std::string sub_default_executor;
    std::list<SubTopicOptions> sub_topic_options;
    std::list<PubTopicOptions> pub_topic_options;
  };

  struct ModuleInfo {
    explicit ModuleInfo(const runtime::core::channel::SubscribeWrapper& wrapper)
        : module_name(wrapper.module_name),
          pkg_path(wrapper.pkg_path),
          subscribe_wrapper(wrapper) {}
    std::string module_name;                                            // module name
    std::string pkg_path;                                               // module package path
    const runtime::core::channel::SubscribeWrapper& subscribe_wrapper;  // subscribe wrapper
  };

  struct SubscriberInfo {
    std::string lcm_url;                                      // lcm url
    std::string topic_name;                                   // subscriber topic name
    std::string msg_type;                                     // subscriber message type
    executor::ExecutorRef executor;                           // subscriber callback executor ref
    std::string lcm_dispatcher_executor;                      // lcm dispatcher executor ref
    int32_t priority = std::numeric_limits<int32_t>::min();   // priority, number is bigger, priority is lower
    std::list<std::shared_ptr<ModuleInfo>> module_info_list;  // subscriber module info list
  };

  using SubscriberInfoPtr = std::shared_ptr<SubscriberInfo>;

 public:
  LcmChannelBackend() = default;
  ~LcmChannelBackend() override = default;

  std::string_view Name() const override { return "lcm"; }

  void Initialize(YAML::Node options_node,
                  const runtime::core::channel::ChannelRegistry* channel_registry_ptr) override;
  void Start() override;
  void Shutdown() override;

  bool RegisterPublishType(
      const runtime::core::channel::PublishTypeWrapper& publish_type_wrapper) noexcept override;
  bool Subscribe(const runtime::core::channel::SubscribeWrapper& subscribe_wrapper) noexcept override;
  void Publish(const runtime::core::channel::PublishWrapper& publish_wrapper) noexcept override;

  void RegisterGetExecutorFunc(const std::function<executor::ExecutorRef(std::string_view)>& get_executor_func);

 private:
  enum class State : uint32_t {
    PreInit,
    Init,
    Start,
    Shutdown,
  };

  Options options_;
  std::atomic<State> state_ = State::PreInit;

  executor::ExecutorRef sub_default_executor_ref_;                            // default executor
  std::function<executor::ExecutorRef(std::string_view)> get_executor_func_;  // can get executor by name

  std::unordered_map<uint64_t, LcmPtr> publisher_map_;
  std::unordered_map<uint64_t, SubscriberInfoPtr> subscriber_info_map_;
  std::unordered_map<uint64_t, std::pair<executor::ExecutorRef, LcmDispatcherPtr>> dispatcher_map_;
};

}  // namespace aimrt::plugins::lcm_plugin
