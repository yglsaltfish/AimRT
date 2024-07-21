#pragma once

#include <string_view>

#include "aimrt_module_c_interface/channel/channel_handle_base.h"
#include "aimrt_module_cpp_interface/channel/channel_context.h"
#include "aimrt_module_cpp_interface/util/function.h"
#include "aimrt_module_cpp_interface/util/string.h"
#include "util/exception.h"

namespace aimrt::channel {

using SubscriberReleaseCallback = aimrt::util::Function<aimrt_function_subscriber_release_callback_ops_t>;
using SubscriberCallback = aimrt::util::Function<aimrt_function_subscriber_callback_ops_t>;

class PublisherRef {
 public:
  PublisherRef() = default;
  explicit PublisherRef(const aimrt_channel_publisher_base_t* base_ptr)
      : base_ptr_(base_ptr) {}
  ~PublisherRef() = default;

  explicit operator bool() const { return (base_ptr_ != nullptr); }

  const aimrt_channel_publisher_base_t* NativeHandle() const {
    return base_ptr_;
  }

  /**
   * @brief Register a type to be published
   *
   * @param msg_type_support
   * @return Register result
   */
  bool RegisterPublishType(const aimrt_type_support_base_t* msg_type_support) {
    AIMRT_ASSERT(base_ptr_, "Reference is null.");
    return base_ptr_->register_publish_type(base_ptr_->impl, msg_type_support);
  }

  /**
   * @brief Publish a msg
   *
   * @param msg_type
   * @param msg_ptr
   */
  void Publish(std::string_view msg_type, ContextRef ctx_ref, const void* msg_ptr) {
    AIMRT_ASSERT(base_ptr_, "Reference is null.");
    base_ptr_->publish(base_ptr_->impl, aimrt::util::ToAimRTStringView(msg_type), ctx_ref.NativeHandle(), msg_ptr);
  }

  /**
   * @brief Get the topic for current publisher
   *
   * @return Topic for current publisher
   */
  std::string_view GetTopic() const {
    AIMRT_ASSERT(base_ptr_, "Reference is null.");
    return aimrt::util::ToStdStringView(base_ptr_->get_topic(base_ptr_->impl));
  }

 private:
  const aimrt_channel_publisher_base_t* base_ptr_ = nullptr;
};

class SubscriberRef {
 public:
  SubscriberRef() = default;
  explicit SubscriberRef(const aimrt_channel_subscriber_base_t* base_ptr)
      : base_ptr_(base_ptr) {}
  ~SubscriberRef() = default;

  explicit operator bool() const { return (base_ptr_ != nullptr); }

  const aimrt_channel_subscriber_base_t* NativeHandle() const {
    return base_ptr_;
  }

  /**
   * @brief Subscribe to a certain type
   *
   * @param msg_type_support
   * @param callback
   * @return Subscribe result
   */
  bool Subscribe(
      const aimrt_type_support_base_t* msg_type_support,
      SubscriberCallback&& callback) {
    AIMRT_ASSERT(base_ptr_, "Reference is null.");
    return base_ptr_->subscribe(base_ptr_->impl, msg_type_support, callback.NativeHandle());
  }

  /**
   * @brief Get the topic for current subscriber
   *
   * @return Topic for current subscriber
   */
  std::string_view GetTopic() const {
    AIMRT_ASSERT(base_ptr_, "Reference is null.");
    return aimrt::util::ToStdStringView(base_ptr_->get_topic(base_ptr_->impl));
  }

 private:
  const aimrt_channel_subscriber_base_t* base_ptr_ = nullptr;
};

class ChannelHandleRef {
 public:
  ChannelHandleRef() = default;
  explicit ChannelHandleRef(const aimrt_channel_handle_base_t* base_ptr)
      : base_ptr_(base_ptr) {}
  ~ChannelHandleRef() = default;

  explicit operator bool() const { return (base_ptr_ != nullptr); }

  const aimrt_channel_handle_base_t* NativeHandle() const { return base_ptr_; }

  PublisherRef GetPublisher(std::string_view topic) {
    AIMRT_ASSERT(base_ptr_, "Reference is null.");
    return PublisherRef(
        base_ptr_->get_publisher(base_ptr_->impl, aimrt::util::ToAimRTStringView(topic)));
  }

  SubscriberRef GetSubscriber(std::string_view topic) {
    AIMRT_ASSERT(base_ptr_, "Reference is null.");
    return SubscriberRef(
        base_ptr_->get_subscriber(base_ptr_->impl, aimrt::util::ToAimRTStringView(topic)));
  }

 private:
  const aimrt_channel_handle_base_t* base_ptr_ = nullptr;
};

class PublisherProxyBase {
 public:
  using HookFunc = std::function<void(std::string_view, ContextRef, const void*)>;

 public:
  explicit PublisherProxyBase(PublisherRef publisher, std::string_view msg_type_name)
      : publisher_(publisher), msg_type_name_(msg_type_name) {}
  virtual ~PublisherProxyBase() = default;

  PublisherProxyBase(const PublisherProxyBase&) = delete;
  PublisherProxyBase& operator=(const PublisherProxyBase&) = delete;

  template <typename... Args>
    requires std::constructible_from<HookFunc, Args...>
  void RegisterHook(Args&&... args) {
    publish_hook_vec.emplace_back(std::forward<Args>(args)...);
  }

  std::shared_ptr<Context> NewContextSharedPtr() const {
    return default_ctx_ptr_
               ? std::make_shared<Context>(*default_ctx_ptr_)
               : std::make_shared<Context>();
  }

  void SetDefaultContextSharedPtr(const std::shared_ptr<Context>& ctx_ptr) {
    default_ctx_ptr_ = ctx_ptr;
  }

  std::shared_ptr<Context> GetDefaultContextSharedPtr() const {
    return default_ctx_ptr_;
  }

  std::string_view GetTopic() const {
    return publisher_.GetTopic();
  }

 protected:
  void PublishImpl(ContextRef ctx_ref, const void* msg_ptr) {
    for (const auto& item : publish_hook_vec)
      item(msg_type_name_, ctx_ref, msg_ptr);

    publisher_.Publish(msg_type_name_, ctx_ref, msg_ptr);
  }

 protected:
  PublisherRef publisher_;
  const std::string msg_type_name_;
  std::shared_ptr<Context> default_ctx_ptr_;
  std::vector<HookFunc> publish_hook_vec;
};

template <typename>
class PublisherProxy;

class SubscriberProxyBase {
 public:
  using HookFunc = std::function<void(std::string_view, ContextRef, const void*)>;

 public:
  explicit SubscriberProxyBase(SubscriberRef subscriber, std::string_view msg_type_name)
      : subscriber_(subscriber), msg_type_name_(msg_type_name) {}
  virtual ~SubscriberProxyBase() = default;

  SubscriberProxyBase(const SubscriberProxyBase&) = delete;
  SubscriberProxyBase& operator=(const SubscriberProxyBase&) = delete;

  template <typename... Args>
    requires std::constructible_from<HookFunc, Args...>
  void RegisterHook(Args&&... args) {
    subscribe_hook_vec.emplace_back(std::forward<Args>(args)...);
  }

  std::string_view GetTopic() const {
    return subscriber_.GetTopic();
  }

 protected:
  SubscriberRef subscriber_;
  const std::string msg_type_name_;
  std::vector<HookFunc> subscribe_hook_vec;
};

template <typename>
class SubscriberProxy;

}  // namespace aimrt::channel
