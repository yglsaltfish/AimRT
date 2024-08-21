// Copyright (c) 2023, AgiBot Inc.
// All rights reserved

#pragma once

#include "rosidl_typesupport_cpp/message_type_support.hpp"
#include "rosidl_typesupport_introspection_cpp/identifier.hpp"
#include "rosidl_typesupport_introspection_cpp/message_introspection.hpp"

namespace aimrt::common::ros2_util {

/// @brief 将普通类型的typesupport提升为内省类型(包含高级操作)
/// @param type_supports 普通类型的typesupport
/// @return
inline const rosidl_message_type_support_t* ImproveToIntrospectionTypeSupport(
    const rosidl_message_type_support_t* type_supports) {
  return get_message_typesupport_handle(
      type_supports, rosidl_typesupport_introspection_cpp::typesupport_identifier);
}

/// @brief 获取基本idl类型的typesuppport
/// @tparam MsgType 泛型
/// @return
template <typename MsgType>
inline const rosidl_message_type_support_t* GetTypeSupport() {
  return rosidl_typesupport_cpp::get_message_type_support_handle<MsgType>();
}

/// @brief 获取内省类型的typesuppport(idl类型的typesupport的扩展)
/// @tparam MsgType 泛型
/// @return
template <typename MsgType>
inline const rosidl_message_type_support_t* GetIntrospectionTypeSupport() {
  return ImproveToIntrospectionTypeSupport(
      rosidl_typesupport_cpp::get_message_type_support_handle<MsgType>());
}

/// @brief 直接通过type support获取members info
/// @param ts IDL类型的typesupport
/// @return
inline const rosidl_typesupport_introspection_cpp::MessageMembers* GetRosMembersInfo(
    const rosidl_message_type_support_t* ts) {
  return reinterpret_cast<const rosidl_typesupport_introspection_cpp::MessageMembers*>(
      ImproveToIntrospectionTypeSupport(ts)->data);
}

}  // namespace aimrt::common::ros2_util