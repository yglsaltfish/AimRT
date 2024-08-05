#pragma once

#include "aimrt_module_cpp_interface/channel/channel_context.h"
#include "aimrt_module_cpp_interface/util/type_support.h"

namespace aimrt::runtime::core::channel {

struct TopicInfo {
  std::string_view msg_type;
  std::string_view topic_name;
  std::string_view pkg_path;
  std::string_view module_name;

  aimrt::util::TypeSupportRef msg_type_support_ref;
};

struct MsgWrapper {
  const TopicInfo& info;

  const void* msg_ptr;

  aimrt::channel::ContextRef ctx_ref;

  std::unordered_map<
      std::string,
      std::shared_ptr<aimrt::util::BufferArrayView>,
      aimrt::common::util::StringHash,
      std::equal_to<>>
      serialization_cache;

  std::shared_ptr<aimrt::util::BufferArrayView> SerializeMsgWithCache(std::string_view serialization_type) {
    auto find_itr = serialization_cache.find(serialization_type);
    if (find_itr != serialization_cache.end())
      return find_itr->second;

    auto buffer_array_ptr = std::make_shared<aimrt::util::BufferArray>();
    bool ret = info.msg_type_support_ref.Serialize(
        serialization_type,
        msg_ptr,
        buffer_array_ptr->AllocatorNativeHandle(),
        buffer_array_ptr->BufferArrayNativeHandle());

    if (!ret) [[unlikely]]
      return {};

    auto buffer_array_view_ptr = std::shared_ptr<aimrt::util::BufferArrayView>(
        new aimrt::util::BufferArrayView(*buffer_array_ptr),
        [buffer_array_ptr](const auto* ptr) { delete ptr; });

    serialization_cache.emplace(serialization_type, buffer_array_view_ptr);

    return buffer_array_view_ptr;
  }
};

}  // namespace aimrt::runtime::core::channel
