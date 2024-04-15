
#pragma once

#include <memory>
#include <string_view>
#include <unordered_map>

#include "aimrt_module_c_interface/channel/channel_handle_base.h"
#include "aimrt_module_cpp_interface/util/function.h"
#include "util/log_util.h"

namespace aimrt::runtime::core::channel {

struct PublishTypeWrapper {
  std::string_view msg_type;
  std::string_view pkg_path;
  std::string_view module_name;
  std::string_view topic_name;
  const aimrt_type_support_base_t* msg_type_support = nullptr;
};

struct SubscribeWrapper {
  std::string_view msg_type;
  std::string_view pkg_path;
  std::string_view module_name;
  std::string_view topic_name;
  const aimrt_type_support_base_t* msg_type_support = nullptr;
  aimrt::util::Function<aimrt_function_subscriber_callback_ops_t> callback;
};

class ChannelRegistry {
 public:
  ChannelRegistry()
      : logger_ptr_(std::make_shared<aimrt::common::util::LoggerWrapper>()) {}
  ~ChannelRegistry() = default;

  ChannelRegistry(const ChannelRegistry&) = delete;
  ChannelRegistry& operator=(const ChannelRegistry&) = delete;

  void SetLogger(const std::shared_ptr<aimrt::common::util::LoggerWrapper>& logger_ptr) { logger_ptr_ = logger_ptr; }
  const aimrt::common::util::LoggerWrapper& GetLogger() const { return *logger_ptr_; }

  bool RegisterPublishType(
      std::unique_ptr<PublishTypeWrapper>&& publish_type_wrapper_ptr);
  bool Subscribe(std::unique_ptr<SubscribeWrapper>&& subscribe_wrapper_ptr);

  const auto& GetPublishTypeWrapperMap() const {
    return publish_type_wrapper_map_;
  }

  const auto& GetSubscribeWrapperMap() const { return subscribe_wrapper_map_; }

  const PublishTypeWrapper* GetPublishTypeWrapper(
      std::string_view pkg_path,
      std::string_view module_name,
      std::string_view topic_name,
      std::string_view msg_type) const;

 private:
  std::shared_ptr<aimrt::common::util::LoggerWrapper> logger_ptr_;

  // 发布类型注册表: pkg_path:module_name:topic:msg_type:wrapper
  using PublishTypeMsgTypeMap = std::unordered_map<std::string_view, std::unique_ptr<PublishTypeWrapper>>;
  using PublishTypeTopicMap = std::unordered_map<std::string_view, PublishTypeMsgTypeMap>;
  using PublishTypeModuleMap = std::unordered_map<std::string_view, PublishTypeTopicMap>;
  using PublishTypePkgMap = std::unordered_map<std::string_view, PublishTypeModuleMap>;
  PublishTypePkgMap publish_type_wrapper_map_;

  // 订阅回调注册表: pkg_path:module_name:topic:msg_type:wrapper
  using SubscribeMsgTypeMap = std::unordered_map<std::string_view, std::unique_ptr<SubscribeWrapper>>;
  using SubscribeTopicMap = std::unordered_map<std::string_view, SubscribeMsgTypeMap>;
  using SubscribeModuleMap = std::unordered_map<std::string_view, SubscribeTopicMap>;
  using SubscribePkgMap = std::unordered_map<std::string_view, SubscribeModuleMap>;
  SubscribePkgMap subscribe_wrapper_map_;
};

}  // namespace aimrt::runtime::core::channel