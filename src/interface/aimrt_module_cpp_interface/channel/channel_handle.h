#pragma once

#include <string_view>

#include "aimrt_module_c_interface/channel/channel_handle_base.h"
#include "aimrt_module_cpp_interface/channel/channel_context.h"
#include "aimrt_module_cpp_interface/util/function.h"
#include "aimrt_module_cpp_interface/util/string.h"
#include "util/exception.h"

namespace aimrt::channel {

class ContextManagerRef {
 public:
  ContextManagerRef() = default;
  explicit ContextManagerRef(const aimrt_channel_context_manager_base_t* base_ptr)
      : base_ptr_(base_ptr) {}
  ~ContextManagerRef() = default;

  explicit operator bool() const { return (base_ptr_ != nullptr); }

  const aimrt_channel_context_manager_base_t* NativeHandle() const {
    return base_ptr_;
  }

  /**
   * @brief Create context shared ptr
   *
   * @return ContextSharedPtr
   */
  ContextSharedPtr NewContextSharedPtr() const {
    AIMRT_ASSERT(base_ptr_, "Reference is null.");
    return std::shared_ptr<const aimrt_channel_context_base_t>(
        base_ptr_->new_context(base_ptr_->impl),
        [base_ptr_ = base_ptr_](const aimrt_channel_context_base_t* ptr) {
          base_ptr_->delete_context(base_ptr_->impl, ptr);
        });
  }

 private:
  const aimrt_channel_context_manager_base_t* base_ptr_ = nullptr;
};

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
  void Publish(std::string_view msg_type,
               aimrt::channel::ContextRef ctx_ref,
               const void* msg_ptr) {
    AIMRT_ASSERT(base_ptr_, "Reference is null.");
    base_ptr_->publish(base_ptr_->impl, aimrt::util::ToAimRTStringView(msg_type), ctx_ref.NativeHandle(), msg_ptr);
  }

  ContextManagerRef GetContextManager() {
    AIMRT_ASSERT(base_ptr_, "Reference is null.");
    return ContextManagerRef(base_ptr_->get_context_manager(base_ptr_->impl));
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
      aimrt::util::Function<aimrt_function_subscriber_callback_ops_t>&& callback) {
    AIMRT_ASSERT(base_ptr_, "Reference is null.");
    return base_ptr_->subscribe(base_ptr_->impl, msg_type_support, callback.NativeHandle());
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

  PublisherRef GetPublisher(std::string_view topic) const {
    AIMRT_ASSERT(base_ptr_, "Reference is null.");
    return PublisherRef(
        base_ptr_->get_publisher(base_ptr_->impl, aimrt::util::ToAimRTStringView(topic)));
  }

  SubscriberRef GetSubscriber(std::string_view topic) const {
    AIMRT_ASSERT(base_ptr_, "Reference is null.");
    return SubscriberRef(
        base_ptr_->get_subscriber(base_ptr_->impl, aimrt::util::ToAimRTStringView(topic)));
  }

  ContextManagerRef GetContextManager() const {
    AIMRT_ASSERT(base_ptr_, "Reference is null.");
    return ContextManagerRef(base_ptr_->get_context_manager(base_ptr_->impl));
  }

 private:
  const aimrt_channel_handle_base_t* base_ptr_ = nullptr;
};

}  // namespace aimrt::channel
