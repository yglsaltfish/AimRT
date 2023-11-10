#pragma once

#include "aimrt_module_c_interface/util/buffer_base.h"

#include "rclcpp/rclcpp.hpp"
#include "rclcpp/serialization.hpp"

namespace aimrt {

class Ros2RclSerializedMessageAdapter {
 public:
  explicit Ros2RclSerializedMessageAdapter(aimrt_buffer_array_t* buffer_array)
      : serialized_msg_(rcutils_uint8_array_t{
            .buffer = nullptr,
            .buffer_length = 0,
            .buffer_capacity = 0,
            .allocator = rcutils_allocator_t{
                .allocate = [](size_t size, void* state) -> void* {
                  aimrt_buffer_array_t* buffer_array = static_cast<aimrt_buffer_array_t*>(state);
                  const aimrt_buffer_array_allocator_t* allocator = buffer_array->allocator;
                  aimrt_buffer_t buffer = allocator->allocate(allocator->impl, buffer_array, size);
                  return buffer.data;
                },
                .deallocate = [](void* pointer, void* state) {  //
                  if (pointer == nullptr) [[unlikely]]
                    return;

                  aimrt_buffer_array_t* buffer_array = static_cast<aimrt_buffer_array_t*>(state);
                  for (size_t ii = 0; ii < buffer_array->len; ++ii) {
                    if (buffer_array->data[ii].data == pointer) {
                      buffer_array->data[ii].len = 0;
                      return;
                    }
                  }
                  fprintf(stderr, "Invalid pointer for deallocate. Warning by Ros2RclSerializedMessageAdapter.\n");
                },
                .reallocate = [](void* pointer, size_t size, void* state) -> void* {
                  aimrt_buffer_array_t* buffer_array = static_cast<aimrt_buffer_array_t*>(state);

                  if (pointer == nullptr) [[unlikely]] {
                    const aimrt_buffer_array_allocator_t* allocator = buffer_array->allocator;
                    aimrt_buffer_t buffer = allocator->allocate(allocator->impl, buffer_array, size);
                    return buffer.data;
                  }

                  for (size_t ii = 0; ii < buffer_array->len; ++ii) {
                    if (buffer_array->data[ii].data == pointer) {
                      buffer_array->data[ii].len = 0;

                      const aimrt_buffer_array_allocator_t* allocator = buffer_array->allocator;
                      aimrt_buffer_t buffer = allocator->allocate(allocator->impl, buffer_array, size);
                      return buffer.data;
                    }
                  }

                  fprintf(stderr, "Invalid pointer for deallocate. Warning by Ros2RclSerializedMessageAdapter.\n");

                  return nullptr;
                },
                .zero_allocate = [](size_t number_of_elements, size_t size_of_element, void* state) -> void* {
                  aimrt_buffer_array_t* buffer_array = static_cast<aimrt_buffer_array_t*>(state);
                  const aimrt_buffer_array_allocator_t* allocator = buffer_array->allocator;
                  aimrt_buffer_t buffer = allocator->allocate(allocator->impl, buffer_array, number_of_elements * size_of_element);
                  memset(buffer.data, 0, buffer.len);
                  return buffer.data;
                },
                .state = buffer_array}}) {}
  ~Ros2RclSerializedMessageAdapter() = default;

  Ros2RclSerializedMessageAdapter(const Ros2RclSerializedMessageAdapter&) = delete;
  Ros2RclSerializedMessageAdapter& operator=(const Ros2RclSerializedMessageAdapter&) = delete;

  rcl_serialized_message_t* GetRclSerializedMessage() { return &serialized_msg_; }

 private:
  rcl_serialized_message_t serialized_msg_;
};

}  // namespace aimrt
