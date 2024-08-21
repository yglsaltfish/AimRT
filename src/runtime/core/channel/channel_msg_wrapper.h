// Copyright (c) 2023, AgiBot Inc.
// All rights reserved

#pragma once

#include <string>
#include <unordered_map>
#include <unordered_set>

#include "aimrt_module_cpp_interface/channel/channel_context.h"
#include "aimrt_module_cpp_interface/util/type_support.h"

namespace aimrt::runtime::core::channel {

/**
 * @brief Topic base info
 *
 */
struct TopicInfo {
  std::string msg_type;
  std::string topic_name;
  std::string pkg_path;
  std::string module_name;

  aimrt::util::TypeSupportRef msg_type_support_ref;
};

/**
 * @brief Sub/Pub msg info with cache
 *
 */
struct MsgWrapper {
  /// Topic base info
  const TopicInfo& info;

  /// pointer to msg, may be nullptr
  const void* msg_ptr = nullptr;

  /// ref to ctx, may be empty
  aimrt::channel::ContextRef ctx_ref;

  /// serialization cache
  std::unordered_map<
      std::string,
      std::shared_ptr<aimrt::util::BufferArrayView>,
      aimrt::common::util::StringHash,
      std::equal_to<>>
      serialization_cache;

  /// msg cache
  std::shared_ptr<void> msg_cache_ptr;

  bool CheckMsg() {
    if (msg_ptr != nullptr)
      return true;

    if (serialization_cache.empty()) [[unlikely]]
      return false;

    if (serialization_cache.size() == 1) {
      msg_cache_ptr = info.msg_type_support_ref.CreateSharedPtr();

      const auto& serialization_type = serialization_cache.begin()->first;
      auto buffer_array_view_ptr = serialization_cache.begin()->second->NativeHandle();

      bool deserialize_ret = info.msg_type_support_ref.Deserialize(
          serialization_type, *buffer_array_view_ptr, msg_cache_ptr.get());

      if (!deserialize_ret) [[unlikely]] {
        msg_cache_ptr.reset();
        return false;
      }

      msg_ptr = msg_cache_ptr.get();
      return true;
    }

    auto serialization_type_span = info.msg_type_support_ref.SerializationTypesSupportedListSpan();

    for (auto item : serialization_type_span) {
      auto serialization_type = aimrt::util::ToStdStringView(item);

      auto finditr = serialization_cache.find(serialization_type);
      if (finditr == serialization_cache.end()) [[unlikely]]
        continue;

      msg_cache_ptr = info.msg_type_support_ref.CreateSharedPtr();

      auto buffer_array_view_ptr = finditr->second->NativeHandle();

      bool deserialize_ret = info.msg_type_support_ref.Deserialize(
          serialization_type, *buffer_array_view_ptr, msg_cache_ptr.get());

      if (!deserialize_ret) [[unlikely]] {
        msg_cache_ptr.reset();
        return false;
      }

      msg_ptr = msg_cache_ptr.get();
      return true;
    }

    return false;
  }

  std::shared_ptr<aimrt::util::BufferArrayView> SerializeMsgWithCache(std::string_view serialization_type) {
    auto finditr = serialization_cache.find(serialization_type);
    if (finditr != serialization_cache.end())
      return finditr->second;

    if (!CheckMsg()) [[unlikely]]
      return {};

    auto buffer_array_ptr = std::make_unique<aimrt::util::BufferArray>();
    bool ret = info.msg_type_support_ref.Serialize(
        serialization_type,
        msg_ptr,
        buffer_array_ptr->AllocatorNativeHandle(),
        buffer_array_ptr->BufferArrayNativeHandle());

    if (!ret) [[unlikely]]
      return {};

    auto ptr = buffer_array_ptr.get();
    auto buffer_array_view_ptr = std::shared_ptr<aimrt::util::BufferArrayView>(
        new aimrt::util::BufferArrayView(*ptr),
        [buffer_array_ptr{std::move(buffer_array_ptr)}](const auto* ptr) { delete ptr; });

    serialization_cache.emplace(serialization_type, buffer_array_view_ptr);

    return buffer_array_view_ptr;
  }
};

}  // namespace aimrt::runtime::core::channel
