#pragma once

#include <concepts>
#include <memory>

#include "aimrt_module_cpp_interface/channel/channel_handle.h"
#include "aimrt_module_protobuf_interface/util/protobuf_type_support.h"

#include "aimrt_module_cpp_interface/co/inline_scheduler.h"
#include "aimrt_module_cpp_interface/co/on.h"
#include "aimrt_module_cpp_interface/co/start_detached.h"
#include "aimrt_module_cpp_interface/co/task.h"
#include "aimrt_module_cpp_interface/co/then.h"

namespace aimrt::channel {

template <std::derived_from<google::protobuf::Message> MsgType>
inline bool RegisterPublishType(PublisherRef publisher) {
  return publisher.RegisterPublishType(GetProtobufMessageTypeSupport<MsgType>());
}

template <std::derived_from<google::protobuf::Message> MsgType>
inline void Publish(PublisherRef publisher, ContextRef ctx_ref, const MsgType& msg) {
  static const std::string msg_type_name = "pb:" + MsgType().GetTypeName();
  ctx_ref.SetSerializationType("pb");
  publisher.Publish(msg_type_name, ctx_ref, static_cast<const void*>(&msg));
}

template <std::derived_from<google::protobuf::Message> MsgType>
inline void Publish(PublisherRef publisher, const MsgType& msg) {
  aimrt::channel::Context ctx;
  Publish(publisher, ctx, msg);
}

template <std::derived_from<google::protobuf::Message> MsgType>
inline bool Subscribe(
    SubscriberRef subscriber,
    aimrt::util::Function<void(aimrt::channel::ContextRef ctx_ref,
                               const std::shared_ptr<const MsgType>&)>&& callback) {
  return subscriber.Subscribe(
      GetProtobufMessageTypeSupport<MsgType>(),
      [callback{std::move(callback)}](
          const aimrt_channel_context_base_t* ctx_ptr,
          const void* msg_ptr,
          aimrt_function_base_t* release_callback_base) {
        aimrt::util::Function<aimrt_function_subscriber_release_callback_ops_t> release_callback(release_callback_base);
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
    aimrt::util::Function<void(const std::shared_ptr<const MsgType>&)>&& callback) {
  return subscriber.Subscribe(
      GetProtobufMessageTypeSupport<MsgType>(),
      [callback{std::move(callback)}](
          const aimrt_channel_context_base_t* ctx_ptr,
          const void* msg_ptr,
          aimrt_function_base_t* release_callback_base) {
        aimrt::util::Function<aimrt_function_subscriber_release_callback_ops_t> release_callback(release_callback_base);
        std::shared_ptr<const MsgType> msg_shared_ptr =
            std::shared_ptr<const MsgType>(
                static_cast<const MsgType*>(msg_ptr),
                [release_callback{std::move(release_callback)}](const MsgType*) { release_callback(); });
        callback(msg_shared_ptr);
      });
}

template <std::derived_from<google::protobuf::Message> MsgType>
inline bool SubscribeCo(
    SubscriberRef subscriber,
    aimrt::util::Function<co::Task<void>(aimrt::channel::ContextRef ctx_ref, const MsgType&)>&& callback) {
  return subscriber.Subscribe(
      GetProtobufMessageTypeSupport<MsgType>(),
      [callback{std::move(callback)}](
          const aimrt_channel_context_base_t* ctx_ptr,
          const void* msg_ptr,
          aimrt_function_base_t* release_callback_base) {
        aimrt::co::StartDetached(
            aimrt::co::On(
                aimrt::co::InlineScheduler(),
                callback(aimrt::channel::ContextRef(ctx_ptr), *(static_cast<const MsgType*>(msg_ptr)))) |
            aimrt::co::Then(
                aimrt::util::Function<aimrt_function_subscriber_release_callback_ops_t>(release_callback_base)));
      });
}

template <std::derived_from<google::protobuf::Message> MsgType>
inline bool SubscribeCo(SubscriberRef subscriber,
                        aimrt::util::Function<co::Task<void>(const MsgType&)>&& callback) {
  return subscriber.Subscribe(
      GetProtobufMessageTypeSupport<MsgType>(),
      [callback{std::move(callback)}](
          const aimrt_channel_context_base_t* ctx_ptr,
          const void* msg_ptr,
          aimrt_function_base_t* release_callback_base) {
        aimrt::co::StartDetached(
            aimrt::co::On(
                aimrt::co::InlineScheduler(),
                callback(*(static_cast<const MsgType*>(msg_ptr)))) |
            aimrt::co::Then(
                aimrt::util::Function<aimrt_function_subscriber_release_callback_ops_t>(release_callback_base)));
      });
}

}  // namespace aimrt::channel
