#pragma once

#include <memory>
#include <mutex>
#include <shared_mutex>
#include <string>
#include <unordered_map>

#include "aimrt_module_c_interface/channel/channel_handle_base.h"
#include "aimrt_module_cpp_interface/util/function.h"
#include "aimrt_module_cpp_interface/util/string.h"
#include "aimrt_module_cpp_interface/util/type_support.h"
#include "core/channel/channel_backend_manager.h"
#include "util/log_util.h"
#include "util/string_util.h"

namespace aimrt::runtime::core::channel {

class PublisherProxy {
 public:
  PublisherProxy(std::string_view pkg_path,
                 std::string_view module_name,
                 std::string_view topic_name,
                 ChannelBackendManager& channel_backend_manager)
      : pkg_path_(pkg_path),
        module_name_(module_name),
        topic_name_(topic_name),
        channel_backend_manager_(channel_backend_manager),
        base_(GenBase(this)) {}
  ~PublisherProxy() = default;

  PublisherProxy(const PublisherProxy&) = delete;
  PublisherProxy& operator=(const PublisherProxy&) = delete;

  const aimrt_channel_publisher_base_t* NativeHandle() const { return &base_; }

 private:
  bool RegisterPublishType(const aimrt_type_support_base_t* msg_type_support) {
    return channel_backend_manager_.RegisterPublishType(PublishTypeWrapper{
        .msg_type = aimrt::util::TypeSupportRef(msg_type_support).TypeName(),
        .pkg_path = pkg_path_,
        .module_name = module_name_,
        .topic_name = topic_name_,
        .msg_type_support = msg_type_support});
  }

  void Publish(std::string_view msg_type,
               aimrt::channel::ContextRef ctx_ref,
               const void* msg_ptr) {
    channel_backend_manager_.Publish(PublishWrapper{
        .msg_type = msg_type,
        .pkg_path = pkg_path_,
        .module_name = module_name_,
        .topic_name = topic_name_,
        .ctx_ref = ctx_ref,
        .msg_ptr = msg_ptr});
  }

  static aimrt_channel_publisher_base_t GenBase(void* impl) {
    return aimrt_channel_publisher_base_t{
        .register_publish_type = [](void* impl, const aimrt_type_support_base_t* msg_type_support) -> bool {
          return static_cast<PublisherProxy*>(impl)->RegisterPublishType(msg_type_support);
        },
        .publish = [](void* impl, aimrt_string_view_t msg_type, const aimrt_channel_context_base_t* ctx_ptr, const void* msg) {
          static_cast<PublisherProxy*>(impl)->Publish(
              aimrt::util::ToStdStringView(msg_type), aimrt::channel::ContextRef(ctx_ptr), msg);  //
        },
        .impl = impl};
  }

 private:
  const std::string_view pkg_path_;
  const std::string_view module_name_;
  const std::string topic_name_;

  ChannelBackendManager& channel_backend_manager_;

  const aimrt_channel_publisher_base_t base_;
};

class SubscriberProxy {
 public:
  SubscriberProxy(std::string_view pkg_path,
                  std::string_view module_name,
                  std::string_view topic_name,
                  ChannelBackendManager& channel_backend_manager)
      : pkg_path_(pkg_path),
        module_name_(module_name),
        topic_name_(topic_name),
        channel_backend_manager_(channel_backend_manager),
        base_(GenBase(this)) {}
  ~SubscriberProxy() = default;

  SubscriberProxy(const SubscriberProxy&) = delete;
  SubscriberProxy& operator=(const SubscriberProxy&) = delete;

  const aimrt_channel_subscriber_base_t* NativeHandle() const { return &base_; }

 private:
  bool Subscribe(const aimrt_type_support_base_t* msg_type_support,
                 aimrt::util::Function<aimrt_function_subscriber_callback_ops_t>&& callback) {
    return channel_backend_manager_.Subscribe(SubscribeWrapper{
        .msg_type = aimrt::util::TypeSupportRef(msg_type_support).TypeName(),
        .pkg_path = pkg_path_,
        .module_name = module_name_,
        .topic_name = topic_name_,
        .msg_type_support = msg_type_support,
        .callback = std::move(callback)});
  }

  static aimrt_channel_subscriber_base_t GenBase(void* impl) {
    return aimrt_channel_subscriber_base_t{
        .subscribe = [](void* impl, const aimrt_type_support_base_t* msg_type_support, aimrt_function_base_t* callback) -> bool {
          return static_cast<SubscriberProxy*>(impl)->Subscribe(
              msg_type_support, aimrt::util::Function<aimrt_function_subscriber_callback_ops_t>(callback));
        },
        .impl = impl};
  }

 private:
  const std::string_view pkg_path_;
  const std::string_view module_name_;
  const std::string topic_name_;
  ChannelBackendManager& channel_backend_manager_;

  const aimrt_channel_subscriber_base_t base_;
};

class ChannelHandleProxy {
 public:
  using PublisherProxyMap = std::unordered_map<
      std::string,
      std::unique_ptr<PublisherProxy>,
      aimrt::common::util::StringHash,
      std::equal_to<>>;

  using SubscriberProxyMap = std::unordered_map<
      std::string,
      std::unique_ptr<SubscriberProxy>,
      aimrt::common::util::StringHash,
      std::equal_to<>>;

  ChannelHandleProxy(std::string_view pkg_path,
                     std::string_view module_name,
                     aimrt::common::util::LoggerWrapper& logger,
                     ChannelBackendManager& channel_backend_manager,
                     std::atomic_bool& start_flag,
                     PublisherProxyMap& publisher_proxy_map,
                     SubscriberProxyMap& subscriber_proxy_map)
      : pkg_path_(pkg_path),
        module_name_(module_name),
        logger_(logger),
        channel_backend_manager_(channel_backend_manager),
        start_flag_(start_flag),
        publisher_proxy_map_(publisher_proxy_map),
        subscriber_proxy_map_(subscriber_proxy_map),
        base_(GenBase(this)) {}

  ~ChannelHandleProxy() = default;

  ChannelHandleProxy(const ChannelHandleProxy&) = delete;
  ChannelHandleProxy& operator=(const ChannelHandleProxy&) = delete;

  const aimrt_channel_handle_base_t* NativeHandle() const { return &base_; }

  const aimrt::common::util::LoggerWrapper& GetLogger() const { return logger_; }

 private:
  PublisherProxy* GetPublisher(std::string_view topic) {
    auto find_itr = publisher_proxy_map_.find(topic);
    if (find_itr != publisher_proxy_map_.end()) {
      return find_itr->second.get();
    }

    if (start_flag_.load()) {
      AIMRT_WARN("Can not get new publisher for topic '{}' after start.", topic);
      return nullptr;
    }

    auto emplace_ret = publisher_proxy_map_.emplace(
        topic,
        std::make_unique<PublisherProxy>(pkg_path_, module_name_, topic, channel_backend_manager_));

    return emplace_ret.first->second.get();
  }

  SubscriberProxy* GetSubscriber(std::string_view topic) {
    auto find_itr = subscriber_proxy_map_.find(topic);
    if (find_itr != subscriber_proxy_map_.end()) {
      return find_itr->second.get();
    }

    if (start_flag_.load()) {
      AIMRT_WARN("Can not get new subscriber for topic '{}' after start.", topic);
      return nullptr;
    }

    auto emplace_ret = subscriber_proxy_map_.emplace(
        topic,
        std::make_unique<SubscriberProxy>(pkg_path_, module_name_, topic, channel_backend_manager_));

    return emplace_ret.first->second.get();
  }

  static aimrt_channel_handle_base_t GenBase(void* impl) {
    return aimrt_channel_handle_base_t{
        .get_publisher = [](void* impl, aimrt_string_view_t topic) -> const aimrt_channel_publisher_base_t* {
          auto publisher_ptr = static_cast<ChannelHandleProxy*>(impl)->GetPublisher(aimrt::util::ToStdStringView(topic));
          return publisher_ptr ? publisher_ptr->NativeHandle() : nullptr;
        },
        .get_subscriber = [](void* impl, aimrt_string_view_t topic) -> const aimrt_channel_subscriber_base_t* {
          auto subscriber_ptr = static_cast<ChannelHandleProxy*>(impl)->GetSubscriber(aimrt::util::ToStdStringView(topic));
          return subscriber_ptr ? subscriber_ptr->NativeHandle() : nullptr;
        },
        .impl = impl};
  }

 private:
  const std::string_view pkg_path_;
  const std::string_view module_name_;

  aimrt::common::util::LoggerWrapper& logger_;

  ChannelBackendManager& channel_backend_manager_;

  std::atomic_bool& start_flag_;
  PublisherProxyMap& publisher_proxy_map_;
  SubscriberProxyMap& subscriber_proxy_map_;

  const aimrt_channel_handle_base_t base_;
};

}  // namespace aimrt::runtime::core::channel
