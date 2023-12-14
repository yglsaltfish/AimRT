#pragma once

#include <memory>
#include <mutex>
#include <shared_mutex>
#include <string>
#include <unordered_map>

#include "aimrt_module_c_interface/channel/channel_handle_base.h"
#include "aimrt_module_cpp_interface/util/function.h"
#include "aimrt_module_cpp_interface/util/string.h"
#include "core/channel/channel_backend_manager.h"
#include "core/channel/context_manager.h"
#include "util/string_util.h"

#include "tbb/concurrent_hash_map.h"

namespace aimrt::runtime::core::channel {

class ChannelContextManagerProxy {
 public:
  ChannelContextManagerProxy(ContextManager& context_manager)
      : base_(GenBase(&context_manager)) {}

  ~ChannelContextManagerProxy() = default;

  ChannelContextManagerProxy(const ChannelContextManagerProxy&) = delete;
  ChannelContextManagerProxy& operator=(const ChannelContextManagerProxy&) = delete;

  const aimrt_channel_context_manager_base_t* NativeHandle() const { return &base_; }

 private:
  static aimrt_channel_context_manager_base_t GenBase(void* impl) {
    return aimrt_channel_context_manager_base_t{
        .new_context = [](void* impl) -> const aimrt_channel_context_base_t* {
          return static_cast<ContextManager*>(impl)->NewContext()->NativeHandle();
        },
        .delete_context = [](void* impl, const aimrt_channel_context_base_t* ctx) {
          static_cast<ContextManager*>(impl)->DeleteContext(static_cast<ContextImpl*>(ctx->impl));  //
        },
        .impl = impl};
  }

 private:
  const aimrt_channel_context_manager_base_t base_;
};

class PublisherProxy {
 public:
  PublisherProxy(std::string_view pkg_path,
                 std::string_view module_name,
                 std::string_view topic_name,
                 ChannelBackendManager& channel_backend_manager,
                 ContextManager& context_manager)
      : pkg_path_(pkg_path),
        module_name_(module_name),
        topic_name_(topic_name),
        channel_backend_manager_(channel_backend_manager),
        context_manager_proxy_(context_manager),
        base_(GenBase(this)) {}
  ~PublisherProxy() = default;

  PublisherProxy(const PublisherProxy&) = delete;
  PublisherProxy& operator=(const PublisherProxy&) = delete;

  const aimrt_channel_publisher_base_t* NativeHandle() const { return &base_; }

 private:
  bool RegisterPublishType(const aimrt_type_support_base_t* msg_type_support) {
    return channel_backend_manager_.RegisterPublishType(PublishTypeWrapper{
        .msg_type = aimrt::util::ToStdStringView(msg_type_support->type_name),
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
        .publish = [](void* impl,
                      aimrt_string_view_t msg_type,
                      const aimrt_channel_context_base_t* ctx_ptr,
                      const void* msg) {
          static_cast<PublisherProxy*>(impl)->Publish(
              aimrt::util::ToStdStringView(msg_type),
              aimrt::channel::ContextRef(ctx_ptr),
              msg);  //
        },
        .get_context_manager = [](void* impl) -> const aimrt_channel_context_manager_base_t* {
          return static_cast<PublisherProxy*>(impl)->context_manager_proxy_.NativeHandle();
        },
        .impl = impl};
  }

 private:
  const std::string_view pkg_path_;
  const std::string_view module_name_;
  const std::string topic_name_;
  ChannelBackendManager& channel_backend_manager_;
  ChannelContextManagerProxy context_manager_proxy_;

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
        .msg_type = aimrt::util::ToStdStringView(msg_type_support->type_name),
        .pkg_path = pkg_path_,
        .module_name = module_name_,
        .topic_name = topic_name_,
        .msg_type_support = msg_type_support,
        .callback = std::move(callback)});
  }

  static aimrt_channel_subscriber_base_t GenBase(void* impl) {
    return aimrt_channel_subscriber_base_t{
        .subscribe = [](void* impl,
                        const aimrt_type_support_base_t* msg_type_support,
                        aimrt_function_base_t* callback) -> bool {
          return static_cast<SubscriberProxy*>(impl)->Subscribe(
              msg_type_support,
              aimrt::util::Function<aimrt_function_subscriber_callback_ops_t>(callback));
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
  struct StringHashCompare {
    using is_transparent = void;

    bool equal(const std::string& lhs, const std::string& rhs) const { return lhs == rhs; }
    bool equal(const std::string& lhs, std::string_view rhs) const { return lhs == rhs; }
    bool equal(std::string_view lhs, const std::string& rhs) const { return lhs == rhs; }

    std::size_t hash(const std::string& k) const { return std::hash<std::string_view>{}(k); }
    std::size_t hash(std::string_view k) const { return std::hash<std::string_view>{}(k); }
  };

  using PublisherProxyMap = tbb::concurrent_hash_map<
      std::string,
      std::unique_ptr<PublisherProxy>,
      StringHashCompare>;

  using SubscriberProxyMap = tbb::concurrent_hash_map<
      std::string,
      std::unique_ptr<SubscriberProxy>,
      StringHashCompare>;

  ChannelHandleProxy(std::string_view pkg_path,
                     std::string_view module_name,
                     ChannelBackendManager& channel_backend_manager,
                     ContextManager& context_manager,
                     PublisherProxyMap& publisher_proxy_map,
                     SubscriberProxyMap& subscriber_proxy_map)
      : pkg_path_(pkg_path),
        module_name_(module_name),
        channel_backend_manager_(channel_backend_manager),
        context_manager_(context_manager),
        context_manager_proxy_(context_manager_),
        publisher_proxy_map_(publisher_proxy_map),
        subscriber_proxy_map_(subscriber_proxy_map),
        base_(GenBase(this)) {}

  ~ChannelHandleProxy() = default;

  ChannelHandleProxy(const ChannelHandleProxy&) = delete;
  ChannelHandleProxy& operator=(const ChannelHandleProxy&) = delete;

  const aimrt_channel_handle_base_t* NativeHandle() const { return &base_; }

 private:
  PublisherProxy& GetPublisher(std::string_view topic) {
    PublisherProxyMap::const_accessor ac;
    bool find_ret = publisher_proxy_map_.find(ac, topic);
    if (find_ret) return *(ac->second);

    publisher_proxy_map_.emplace(
        ac,
        topic,
        std::make_unique<PublisherProxy>(
            pkg_path_, module_name_, topic, channel_backend_manager_, context_manager_));

    return *(ac->second);
  }

  SubscriberProxy& GetSubscriber(std::string_view topic) {
    SubscriberProxyMap::const_accessor ac;
    bool find_ret = subscriber_proxy_map_.find(ac, topic);
    if (find_ret) return *(ac->second);

    subscriber_proxy_map_.emplace(
        ac,
        topic,
        std::make_unique<SubscriberProxy>(
            pkg_path_, module_name_, topic, channel_backend_manager_));

    return *(ac->second);
  }

  static aimrt_channel_handle_base_t GenBase(void* impl) {
    return aimrt_channel_handle_base_t{
        .get_publisher = [](void* impl, aimrt_string_view_t topic)
            -> const aimrt_channel_publisher_base_t* {
          return static_cast<ChannelHandleProxy*>(impl)
              ->GetPublisher(aimrt::util::ToStdStringView(topic))
              .NativeHandle();
        },
        .get_subscriber = [](void* impl, aimrt_string_view_t topic)
            -> const aimrt_channel_subscriber_base_t* {
          return static_cast<ChannelHandleProxy*>(impl)
              ->GetSubscriber(aimrt::util::ToStdStringView(topic))
              .NativeHandle();
        },
        .get_context_manager = [](void* impl) -> const aimrt_channel_context_manager_base_t* {
          return static_cast<ChannelHandleProxy*>(impl)->context_manager_proxy_.NativeHandle();
        },
        .impl = impl};
  }

 private:
  const std::string_view pkg_path_;
  const std::string_view module_name_;
  ChannelBackendManager& channel_backend_manager_;
  ContextManager& context_manager_;
  ChannelContextManagerProxy context_manager_proxy_;
  PublisherProxyMap& publisher_proxy_map_;
  SubscriberProxyMap& subscriber_proxy_map_;

  const aimrt_channel_handle_base_t base_;
};

}  // namespace aimrt::runtime::core::channel
