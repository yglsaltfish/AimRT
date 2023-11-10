
#pragma once

#include <map>
#include <memory>
#include <string_view>

#include "aimrt_module_c_interface/channel/channel_handle_base.h"
#include "aimrt_module_cpp_interface/util/function.h"

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
  Function<aimrt_function_subscriber_callback_ops_t> callback;
};

class ChannelRegistry {
 public:
  ChannelRegistry() = default;
  ~ChannelRegistry() = default;

  ChannelRegistry(const ChannelRegistry&) = delete;
  ChannelRegistry& operator=(const ChannelRegistry&) = delete;

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
  // 发布类型注册表: pkg_path:module_name:topic:msg_type:wrapper
  using PublishTypeMsgTypeMap = std::map<std::string_view, std::unique_ptr<PublishTypeWrapper>>;
  using PublishTypeTopicMap = std::map<std::string_view, PublishTypeMsgTypeMap>;
  using PublishTypeModuleMap = std::map<std::string_view, PublishTypeTopicMap>;
  using PublishTypePkgMap = std::map<std::string_view, PublishTypeModuleMap>;
  PublishTypePkgMap publish_type_wrapper_map_;

  // 订阅回调注册表: pkg_path:module_name:topic:msg_type:wrapper
  using SubscribeMsgTypeMap = std::map<std::string_view, std::unique_ptr<SubscribeWrapper>>;
  using SubscribeTopicMap = std::map<std::string_view, SubscribeMsgTypeMap>;
  using SubscribeModuleMap = std::map<std::string_view, SubscribeTopicMap>;
  using SubscribePkgMap = std::map<std::string_view, SubscribeModuleMap>;
  SubscribePkgMap subscribe_wrapper_map_;
};

}  // namespace aimrt::runtime::core::channel