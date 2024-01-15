#pragma once

#include <atomic>
#include <unordered_set>

#include "aimrt_module_c_interface/channel/channel_handle_base.h"
#include "aimrt_module_cpp_interface/executor/executor.h"
#include "aimrt_module_cpp_interface/util/function.h"
#include "aimrt_module_cpp_interface/util/type_support.h"
#include "core/channel/channel_backend_base.h"
#include "util/log_util.h"

namespace aimrt::runtime::core::channel {

class LocalChannelBackend : public ChannelBackendBase {
 public:
  struct Options {
    bool subscriber_use_inline_executor = true;
    std::string subscriber_executor;
  };

  enum class State : uint32_t {
    PreInit,
    Init,
    Start,
    Shutdown,
  };

 public:
  LocalChannelBackend()
      : logger_ptr_(std::make_shared<common::util::LoggerWrapper>()) {}
  ~LocalChannelBackend() override = default;

  std::string_view Name() const override { return "local"; }

  void Initialize(YAML::Node options_node,
                  const ChannelRegistry* channel_registry_ptr,
                  ContextManager* context_manager_ptr) override;
  void Start() override;
  void Shutdown() override;

  bool RegisterPublishType(
      const PublishTypeWrapper& publish_type_wrapper) noexcept override;
  bool Subscribe(const SubscribeWrapper& subscribe_wrapper) noexcept override;
  void Publish(const PublishWrapper& publish_wrapper) noexcept override;

  void RegisterGetExecutorFunc(
      const std::function<executor::ExecutorRef(std::string_view)>& get_executor_func);

  State GetState() const { return state_.load(); }

  void SetLogger(const std::shared_ptr<common::util::LoggerWrapper>& logger_ptr) { logger_ptr_ = logger_ptr; }
  const common::util::LoggerWrapper& GetLogger() const { return *logger_ptr_; }

 private:
  const SubscribeWrapper* GetTplSubscribeWrapper(
      std::string_view subscribe_pkg_path,
      std::string_view topic_name,
      std::string_view msg_type) const;

  std::tuple<const SubscribeWrapper*, std::string_view> GetSubscribeSerializationType(
      aimrt::util::TypeSupportRef publish_msg_type_support_ref,
      std::string_view subscribe_pkg_path,
      std::string_view topic_name,
      std::string_view msg_type) const;

 private:
  Options options_;
  std::atomic<State> state_ = State::PreInit;
  std::shared_ptr<common::util::LoggerWrapper> logger_ptr_;

  const ChannelRegistry* channel_registry_ptr_ = nullptr;
  ContextManager* context_manager_ptr_ = nullptr;

  // 订阅回调索引表: msg_type:topic:lib_path:module_name
  using SubscribeIndexMap = std::unordered_map<
      std::string_view,  // msg_type
      std::unordered_map<
          std::string_view,  // topic
          std::unordered_map<
              std::string_view,  // lib_path
              std::unordered_set<
                  std::string_view>>>>;  // module_name
  SubscribeIndexMap subscribe_index_map_;

  std::function<executor::ExecutorRef(std::string_view)> get_executor_func_;
  executor::ExecutorRef subscribe_executor_ref_;
};

}  // namespace aimrt::runtime::core::channel
