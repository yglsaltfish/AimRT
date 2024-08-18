#pragma once

#include <memory>
#include <string>

#include "aimrt_module_cpp_interface/rpc/rpc_context.h"
#include "aimrt_module_cpp_interface/rpc/rpc_status.h"
#include "aimrt_module_cpp_interface/util/type_support.h"

namespace aimrt::runtime::core::rpc {

struct FuncInfo {
  std::string func_name;
  std::string pkg_path;
  std::string module_name;

  const void* custom_type_support_ptr;
  aimrt::util::TypeSupportRef req_type_support_ref;
  aimrt::util::TypeSupportRef rsp_type_support_ref;
};

struct InvokeWrapper {
  const FuncInfo& info;

  const void* req_ptr;
  void* rsp_ptr;

  aimrt::rpc::ContextRef ctx_ref;

  std::function<void(aimrt::rpc::Status)> callback;

  std::unordered_map<
      std::string,
      std::shared_ptr<aimrt::util::BufferArrayView>,
      aimrt::common::util::StringHash,
      std::equal_to<>>
      req_serialization_cache;
  std::unordered_map<
      std::string,
      std::shared_ptr<aimrt::util::BufferArrayView>,
      aimrt::common::util::StringHash,
      std::equal_to<>>
      rsp_serialization_cache;

  std::shared_ptr<aimrt::util::BufferArrayView> SerializeReqWithCache(std::string_view serialization_type) {
    auto find_itr = req_serialization_cache.find(serialization_type);
    if (find_itr != req_serialization_cache.end())
      return find_itr->second;

    auto buffer_array_ptr = std::make_shared<aimrt::util::BufferArray>();
    bool ret = info.req_type_support_ref.Serialize(
        serialization_type,
        req_ptr,
        buffer_array_ptr->AllocatorNativeHandle(),
        buffer_array_ptr->BufferArrayNativeHandle());

    if (!ret) [[unlikely]]
      return {};

    auto buffer_array_view_ptr = std::shared_ptr<aimrt::util::BufferArrayView>(
        new aimrt::util::BufferArrayView(*buffer_array_ptr),
        [buffer_array_ptr](const auto* ptr) { delete ptr; });

    req_serialization_cache.emplace(serialization_type, buffer_array_view_ptr);

    return buffer_array_view_ptr;
  }

  std::shared_ptr<aimrt::util::BufferArrayView> SerializeRspWithCache(std::string_view serialization_type) {
    auto find_itr = rsp_serialization_cache.find(serialization_type);
    if (find_itr != rsp_serialization_cache.end())
      return find_itr->second;

    auto buffer_array_ptr = std::make_shared<aimrt::util::BufferArray>();
    bool ret = info.rsp_type_support_ref.Serialize(
        serialization_type,
        rsp_ptr,
        buffer_array_ptr->AllocatorNativeHandle(),
        buffer_array_ptr->BufferArrayNativeHandle());

    if (!ret) [[unlikely]]
      return {};

    auto buffer_array_view_ptr = std::shared_ptr<aimrt::util::BufferArrayView>(
        new aimrt::util::BufferArrayView(*buffer_array_ptr),
        [buffer_array_ptr](const auto* ptr) { delete ptr; });

    rsp_serialization_cache.emplace(serialization_type, buffer_array_view_ptr);

    return buffer_array_view_ptr;
  }
};

}  // namespace aimrt::runtime::core::rpc