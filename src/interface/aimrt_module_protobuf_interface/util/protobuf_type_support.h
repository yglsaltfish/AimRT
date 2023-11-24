#pragma once

#include <cstring>
#include <string>
#include <vector>

#include "aimrt_module_c_interface/util/type_support_base.h"
#include "aimrt_module_cpp_interface/util/string.h"
#include "aimrt_module_protobuf_interface/util/protobuf_zero_copy_stream.h"

#include <google/protobuf/message.h>
#include <google/protobuf/stubs/stringpiece.h>
#include <google/protobuf/util/json_util.h>

namespace aimrt {

template <std::derived_from<::google::protobuf::Message> MsgType>
const aimrt_type_support_base_t* GetProtobufMessageTypeSupport() {
  static const aimrt_string_view_t kChannelProtobufSerializationTypesSupportedList[] = {
      aimrt::util::ToAimRTStringView("pb"),
      aimrt::util::ToAimRTStringView("json")};

  static const std::string msg_type_name = "pb:" + MsgType().GetTypeName();

  static const aimrt_type_support_base_t ts{
      .type_name = aimrt::util::ToAimRTStringView(msg_type_name),
      .create = []() -> void* { return new MsgType(); },
      .destory = [](void* msg) { delete static_cast<MsgType*>(msg); },
      .copy = [](const void* from, void* to) {
        *static_cast<MsgType*>(to) = *static_cast<const MsgType*>(from);  //
      },
      .move = [](void* from, void* to) {
        *static_cast<MsgType*>(to) = std::move(*static_cast<MsgType*>(from));  //
      },
      .serialize = [](aimrt_string_view_t serialization_type, const void* msg, aimrt_buffer_array_t* buffer_array) -> bool {
        try {
          const MsgType& msg_ref = *static_cast<const MsgType*>(msg);

          if (aimrt::util::ToStdStringView(serialization_type) == "pb") {
            BufferArrayZeroCopyOutputStream os(buffer_array);
            if (!msg_ref.SerializeToZeroCopyStream(&os)) return false;
            os.CommitLastBuf();
            return true;
          }

          if (aimrt::util::ToStdStringView(serialization_type) == "json") {
            // todo：使用zerocopy
            ::google::protobuf::util::JsonPrintOptions op;
            op.always_print_primitive_fields = true;
            op.preserve_proto_field_names = true;
            std::string str;
            auto status = ::google::protobuf::util::MessageToJsonString(msg_ref, &str, op);
            if (!status.ok()) return false;

            const aimrt_buffer_array_allocator_t* allocator = buffer_array->allocator;
            auto buffer = allocator->allocate(allocator->impl, buffer_array, str.size());
            if (buffer.data == nullptr || buffer.len < str.size()) return false;
            memcpy(buffer.data, str.c_str(), str.size());
            return true;
          }

        } catch (const std::exception& e) {
        }
        return false;
      },
      .deserialize = [](aimrt_string_view_t serialization_type, aimrt_buffer_array_view_t buffer_array_view, void* msg) -> bool {
        try {
          if (aimrt::util::ToStdStringView(serialization_type) == "pb") {
            BufferArrayZeroCopyInputStream is(buffer_array_view);
            if (!static_cast<MsgType*>(msg)->ParseFromZeroCopyStream(&is))
              return false;
            return true;
          }

          if (aimrt::util::ToStdStringView(serialization_type) == "json") {
            // todo：使用zerocopy
            if (buffer_array_view.len == 1) {
              auto status = ::google::protobuf::util::JsonStringToMessage(
                  ::google::protobuf::StringPiece(
                      static_cast<const char*>(buffer_array_view.data[0].data),
                      buffer_array_view.data[0].len),
                  static_cast<MsgType*>(msg),
                  ::google::protobuf::util::JsonParseOptions());
              return status.ok();
            }

            if (buffer_array_view.len > 1) {
              size_t total_size = 0;
              for (size_t ii = 0; ii < buffer_array_view.len; ++ii) {
                total_size += buffer_array_view.data[ii].len;
              }
              std::vector<char> buffer_vec(total_size);
              char* buffer = buffer_vec.data();
              size_t cur_size = 0;
              for (size_t ii = 0; ii < buffer_array_view.len; ++ii) {
                memcpy(buffer + cur_size, buffer_array_view.data[ii].data,
                       buffer_array_view.data[ii].len);
                cur_size += buffer_array_view.data[ii].len;
              }
              auto status = ::google::protobuf::util::JsonStringToMessage(
                  ::google::protobuf::StringPiece(
                      static_cast<const char*>(buffer), total_size),
                  static_cast<MsgType*>(msg),
                  ::google::protobuf::util::JsonParseOptions());
              return status.ok();
            }
            return false;
          }
        } catch (const std::exception& e) {
        }
        return false;
      },
      .serialization_types_supported_num = sizeof(kChannelProtobufSerializationTypesSupportedList) / sizeof(kChannelProtobufSerializationTypesSupportedList[0]),
      .serialization_types_supported_list = kChannelProtobufSerializationTypesSupportedList,
      .custom_type_support_ptr = nullptr};
  return &ts;
}

}  // namespace aimrt
