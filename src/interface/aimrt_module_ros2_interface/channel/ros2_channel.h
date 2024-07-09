#pragma once

#include <concepts>
#include <memory>

#include "aimrt_module_cpp_interface/channel/channel_handle.h"
#include "aimrt_module_ros2_interface/util/ros2_type_support.h"

#include "aimrt_module_cpp_interface/co/inline_scheduler.h"
#include "aimrt_module_cpp_interface/co/on.h"
#include "aimrt_module_cpp_interface/co/start_detached.h"
#include "aimrt_module_cpp_interface/co/task.h"
#include "aimrt_module_cpp_interface/co/then.h"

namespace aimrt::channel {

template <Ros2MsgType MsgType>
inline bool RegisterPublishType(PublisherRef publisher) {
  return publisher.RegisterPublishType(GetRos2MessageTypeSupport<MsgType>());
}

template <Ros2MsgType MsgType>
inline void Publish(PublisherRef publisher, ContextRef ctx_ref, const MsgType& msg) {
  static const std::string msg_type_name =
      std::string("ros2:") + rosidl_generator_traits::name<MsgType>();

  if (ctx_ref) {
    if (ctx_ref.GetSerializationType().empty()) ctx_ref.SetSerializationType("ros2");
    publisher.Publish(msg_type_name, ctx_ref, static_cast<const void*>(&msg));
    return;
  }

  Context ctx;
  ctx.SetSerializationType("ros2");
  publisher.Publish(msg_type_name, ctx, static_cast<const void*>(&msg));
}

template <Ros2MsgType MsgType>
inline void Publish(PublisherRef publisher, const MsgType& msg) {
  Publish(publisher, ContextRef(), msg);
}

template <Ros2MsgType MsgType>
inline bool Subscribe(
    SubscriberRef subscriber,
    std::function<void(ContextRef, const std::shared_ptr<const MsgType>&)>&& callback) {
  return subscriber.Subscribe(
      GetRos2MessageTypeSupport<MsgType>(),
      [callback{std::move(callback)}](
          const aimrt_channel_context_base_t* ctx_ptr,
          const void* msg_ptr,
          aimrt_function_base_t* release_callback_base) {
        SubscriberReleaseCallback release_callback(release_callback_base);
        std::shared_ptr<const MsgType> msg_shared_ptr =
            std::shared_ptr<const MsgType>(
                static_cast<const MsgType*>(msg_ptr),
                [release_callback{std::move(release_callback)}](const MsgType*) { release_callback(); });
        callback(ContextRef(ctx_ptr), msg_shared_ptr);
      });
}

template <Ros2MsgType MsgType>
inline bool Subscribe(
    SubscriberRef subscriber,
    std::function<void(const std::shared_ptr<const MsgType>&)>&& callback) {
  return subscriber.Subscribe(
      GetRos2MessageTypeSupport<MsgType>(),
      [callback{std::move(callback)}](
          const aimrt_channel_context_base_t* ctx_ptr,
          const void* msg_ptr,
          aimrt_function_base_t* release_callback_base) {
        SubscriberReleaseCallback release_callback(release_callback_base);
        std::shared_ptr<const MsgType> msg_shared_ptr =
            std::shared_ptr<const MsgType>(
                static_cast<const MsgType*>(msg_ptr),
                [release_callback{std::move(release_callback)}](const MsgType*) { release_callback(); });
        callback(msg_shared_ptr);
      });
}

template <Ros2MsgType MsgType>
inline bool SubscribeCo(
    SubscriberRef subscriber,
    std::function<co::Task<void>(ContextRef, const MsgType&)>&& callback) {
  return subscriber.Subscribe(
      GetRos2MessageTypeSupport<MsgType>(),
      [callback{std::move(callback)}](
          const aimrt_channel_context_base_t* ctx_ptr,
          const void* msg_ptr,
          aimrt_function_base_t* release_callback_base) {
        aimrt::co::StartDetached(
            aimrt::co::On(
                aimrt::co::InlineScheduler(),
                callback(ContextRef(ctx_ptr), *(static_cast<const MsgType*>(msg_ptr)))) |
            aimrt::co::Then(
                SubscriberReleaseCallback(release_callback_base)));
      });
}

template <Ros2MsgType MsgType>
inline bool SubscribeCo(
    SubscriberRef subscriber,
    std::function<co::Task<void>(const MsgType&)>&& callback) {
  return subscriber.Subscribe(
      GetRos2MessageTypeSupport<MsgType>(),
      [callback{std::move(callback)}](
          const aimrt_channel_context_base_t* ctx_ptr,
          const void* msg_ptr,
          aimrt_function_base_t* release_callback_base) {
        aimrt::co::StartDetached(
            aimrt::co::On(
                aimrt::co::InlineScheduler(),
                callback(*(static_cast<const MsgType*>(msg_ptr)))) |
            aimrt::co::Then(
                SubscriberReleaseCallback(release_callback_base)));
      });
}

template <Ros2MsgType MsgType>
class PublisherProxy<MsgType> : public PublisherProxyBase {
 public:
  explicit PublisherProxy(PublisherRef publisher)
      : PublisherProxyBase(publisher) {}
  ~PublisherProxy() = default;

  static bool RegisterPublishType(PublisherRef publisher) {
    return publisher.RegisterPublishType(GetRos2MessageTypeSupport<MsgType>());
  }

  bool RegisterPublishType() const {
    return RegisterPublishType(publisher_);
  }

  void Publish(ContextRef ctx_ref, const MsgType& msg) const {
    static const std::string msg_type_name =
        std::string("ros2:") + rosidl_generator_traits::name<MsgType>();

    if (ctx_ref) {
      if (ctx_ref.GetSerializationType().empty()) ctx_ref.SetSerializationType("ros2");
      PublisherProxyBase::Publish(msg_type_name, ctx_ref, static_cast<const void*>(&msg));
      return;
    }

    auto ctx_ptr = NewContextSharedPtr();
    ctx_ptr->SetSerializationType("ros2");
    PublisherProxyBase::Publish(msg_type_name, ctx_ptr, static_cast<const void*>(&msg));
  }

  void Publish(const MsgType& msg) const {
    Publish(ContextRef(), msg);
  }
};

template <Ros2MsgType MsgType>
class SubscriberProxy<MsgType> : public SubscriberProxyBase {
 public:
  explicit SubscriberProxy(SubscriberRef subscriber)
      : SubscriberProxyBase(subscriber) {}
  ~SubscriberProxy() = default;

  bool Subscribe(
      std::function<void(ContextRef, const std::shared_ptr<const MsgType>&)>&& callback) const {
    return Subscribe(subscriber_, std::move(callback));
  }

  bool Subscribe(
      std::function<void(const std::shared_ptr<const MsgType>&)>&& callback) const {
    return Subscribe(subscriber_, std::move(callback));
  }

  bool SubscribeCo(
      std::function<co::Task<void>(ContextRef, const MsgType&)>&& callback) const {
    return SubscribeCo(subscriber_, std::move(callback));
  }

  bool SubscribeCo(std::function<co::Task<void>(const MsgType&)>&& callback) const {
    return SubscribeCo(subscriber_, std::move(callback));
  }
};

}  // namespace aimrt::channel