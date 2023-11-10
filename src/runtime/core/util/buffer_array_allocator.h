#pragma once

#include "aimrt_module_c_interface/util/buffer_base.h"

namespace aimrt::runtime::core::util {

class BufferArrayAllocator {
 public:
  static void Reserve(aimrt_buffer_array_t* buffer_array, size_t new_cap);

  static aimrt_buffer_t Allocate(aimrt_buffer_array_t* buffer_array, size_t size);

  static void Release(aimrt_buffer_array_t* buffer_array);

  static const aimrt_buffer_array_allocator_t* NativeHandle();
};

}  // namespace aimrt::runtime::core::util
