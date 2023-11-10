#pragma once

#include <concepts>
#include <memory>

#include "aimrt_module_cpp_interface/channel/channel_handle.h"
#include "aimrt_module_protobuf_interface/util/protobuf_type_support.h"

#ifdef AIMRT_USE_EXECUTOR
  #include "aimrt_module_cpp_interface/co/start_detached.h"
  #include "aimrt_module_cpp_interface/co/task.h"
#endif

namespace aimrt {
namespace channel {

template <std::derived_from<google::protobuf::Message> MsgType>
inline bool RegisterPublishType(PublisherRef publisher) {
  return publisher.RegisterPublishType(GetProtobufMessageTypeSupport<MsgType>());
}

template <std::derived_from<google::protobuf::Message> MsgType>
inline void Publish(PublisherRef publisher, aimrt::channel::ContextRef ctx_ref, const MsgType& msg) {
  static const std::string msg_type_name = "pb:" + MsgType().GetTypeName();

  if (ctx_ref) {
    if (ctx_ref.GetSerializationType().empty()) ctx_ref.SetSerializationType("pb");
    publisher.Publish(msg_type_name, ctx_ref, static_cast<const void*>(&msg));
    return;
  }

  auto ctx_ptr = publisher.GetContextManager().NewContextSharedPtr();
  ctx_ref = aimrt::channel::ContextRef(ctx_ptr);
  ctx_ref.SetSerializationType("pb");
  publisher.Publish(msg_type_name, ctx_ref, static_cast<const void*>(&msg));
}

template <std::derived_from<google::protobuf::Message> MsgType>
inline void Publish(PublisherRef publisher, const MsgType& msg) {
  static const std::string msg_type_name = "pb:" + MsgType().GetTypeName();
  Publish(publisher, aimrt::channel::ContextRef(), msg);
}

template <std::derived_from<google::protobuf::Message> MsgType>
inline bool Subscribe(
    SubscriberRef subscriber,
    Function<void(aimrt::channel::ContextRef ctx_ref,
                  const std::shared_ptr<const MsgType>&)>&& callback) {
  return subscriber.Subscribe(
      GetProtobufMessageTypeSupport<MsgType>(),
      [callback{std::move(callback)}](
          const aimrt_channel_context_base_t* ctx_ptr,
          const void* msg_ptr,
          aimrt_function_base_t* release_callback_base) {
        Function<aimrt_function_subscriber_release_callback_ops_t> release_callback(release_callback_base);
        std::shared_ptr<const MsgType> msg_shared_ptr =
            std::shared_ptr<const MsgType>(
                static_cast<const MsgType*>(msg_ptr),
                [release_callback{std::move(release_callback)}](const MsgType*) { release_callback(); });
        callback(aimrt::channel::ContextRef(ctx_ptr), msg_shared_ptr);
      });
}

template <std::derived_from<google::protobuf::Message> MsgType>
inline bool Subscribe(
    SubscriberRef subscriber,
    Function<void(const std::shared_ptr<const MsgType>&)>&& callback) {
  return subscriber.Subscribe(
      GetProtobufMessageTypeSupport<MsgType>(),
      [callback{std::move(callback)}](
          const aimrt_channel_context_base_t* ctx_ptr,
          const void* msg_ptr,
          aimrt_function_base_t* release_callback_base) {
        Function<aimrt_function_subscriber_release_callback_ops_t> release_callback(release_callback_base);
        std::shared_ptr<const MsgType> msg_shared_ptr =
            std::shared_ptr<const MsgType>(
                static_cast<const MsgType*>(msg_ptr),
                [release_callback{std::move(release_callback)}](const MsgType*) { release_callback(); });
        callback(msg_shared_ptr);
      });
}

#ifdef AIMRT_USE_EXECUTOR

template <std::derived_from<google::protobuf::Message> MsgType>
inline bool SubscribeCo(
    SubscriberRef subscriber,
    Function<co::Task<void>(aimrt::channel::ContextRef ctx_ref, const MsgType&)>&& callback) {
  return subscriber.Subscribe(
      GetProtobufMessageTypeSupport<MsgType>(),
      [callback{std::move(callback)}](
          const aimrt_channel_context_base_t* ctx_ptr,
          const void* msg_ptr,
          aimrt_function_base_t* release_callback_base) {
        co::StartDetached(
            callback(aimrt::channel::ContextRef(ctx_ptr), *(static_cast<const MsgType*>(msg_ptr))),
            Function<aimrt_function_subscriber_release_callback_ops_t>(release_callback_base));
      });
}

template <std::derived_from<google::protobuf::Message> MsgType>
inline bool SubscribeCo(SubscriberRef subscriber,
                        Function<co::Task<void>(const MsgType&)>&& callback) {
  return subscriber.Subscribe(
      GetProtobufMessageTypeSupport<MsgType>(),
      [callback{std::move(callback)}](
          const aimrt_channel_context_base_t* ctx_ptr,
          const void* msg_ptr,
          aimrt_function_base_t* release_callback_base) {
        co::StartDetached(
            callback(*(static_cast<const MsgType*>(msg_ptr))),
            Function<aimrt_function_subscriber_release_callback_ops_t>(release_callback_base));
      });
}

#endif

}  // namespace channel
}  // namespace aimrt
