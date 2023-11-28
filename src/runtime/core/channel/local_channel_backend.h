#pragma once

#include <atomic>

#include "aimrt_module_c_interface/channel/channel_handle_base.h"
#include "aimrt_module_cpp_interface/executor/executor.h"
#include "aimrt_module_cpp_interface/util/function.h"
#include "core/channel/channel_backend_base.h"

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
  LocalChannelBackend() = default;
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

 private:
  const SubscribeWrapper* GetTplSubscribeWrapper(
      std::string_view subscribe_pkg_path,
      std::string_view topic_name,
      std::string_view msg_type) const;

  std::tuple<const SubscribeWrapper*, std::string_view> GetSubscribeSerializationType(
      const aimrt_type_support_base_t* publish_msg_type_support_ptr,
      std::string_view subscribe_pkg_path,
      std::string_view topic_name,
      std::string_view msg_type) const;

 private:
  Options options_;
  std::atomic<State> state_ = State::PreInit;

  const ChannelRegistry* channel_registry_ptr_ = nullptr;
  ContextManager* context_manager_ptr_ = nullptr;

  // 订阅回调索引表: msg_type:topic:lib_path:module_name
  using SubscribeIndexMap = std::unordered_map<
      std::string_view,  // msg_type
      std::unordered_map<
          std::string_view,  // topic
          std::unordered_map<
              std::string_view,               // lib_path
              std::set<std::string_view>>>>;  // module_name
  SubscribeIndexMap subscribe_index_map_;

  std::function<executor::ExecutorRef(std::string_view)> get_executor_func_;
  executor::ExecutorRef subscribe_executor_ref_;
};

}  // namespace aimrt::runtime::core::channel
