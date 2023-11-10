#include <cassert>
#include <cstdlib>
#include <cstring>

#include "core/util/buffer_array_allocator.h"

namespace aimrt::runtime::core::util {

void BufferArrayAllocator::Reserve(
    aimrt_buffer_array_t* buffer_array, size_t new_cap) {
  assert(buffer_array);

  aimrt_buffer_t* cur_data = buffer_array->data;

  buffer_array->data = new aimrt_buffer_t[new_cap];
  buffer_array->capacity = new_cap;

  if (cur_data) {
    memcpy(buffer_array->data, cur_data, buffer_array->len * sizeof(aimrt_buffer_t));
    delete[] cur_data;
  }
}

aimrt_buffer_t BufferArrayAllocator::Allocate(
    aimrt_buffer_array_t* buffer_array, size_t size) {
  assert(buffer_array);

  void* data = std::malloc(size);

  if (data == nullptr) [[unlikely]]
    return aimrt_buffer_t{data, 0};

  // 可以直接放在当前data中
  if (buffer_array->capacity > buffer_array->len) {
    return (buffer_array->data[buffer_array->len++] = aimrt_buffer_t{data, size});
  }

  // 当前data区已满，需要重新开辟空间
  static constexpr size_t kInitCapacitySzie = 2;
  size_t new_capacity = (buffer_array->capacity < kInitCapacitySzie)
                            ? kInitCapacitySzie
                            : (buffer_array->capacity << 1);
  Reserve(buffer_array, new_capacity);

  return (buffer_array->data[buffer_array->len++] = aimrt_buffer_t{data, size});
}

void BufferArrayAllocator::Release(aimrt_buffer_array_t* buffer_array) {
  assert(buffer_array);

  for (size_t ii = 0; ii < buffer_array->len; ++ii) {
    std::free(buffer_array->data[ii].data);
  }

  if (buffer_array->data) delete[] buffer_array->data;
}

const aimrt_buffer_array_allocator_t* BufferArrayAllocator::NativeHandle() {
  static constexpr aimrt_buffer_array_allocator_t buffer_array_allocator{
      .reserve = [](void* impl, aimrt_buffer_array_t* buffer_array, size_t new_cap) {  //
        Reserve(buffer_array, new_cap);
      },
      .allocate = [](void* impl, aimrt_buffer_array_t* buffer_array, size_t size) -> aimrt_buffer_t {
        return Allocate(buffer_array, size);
      },
      .release = [](void* impl, aimrt_buffer_array_t* buffer_array) {  //
        Release(buffer_array);
      },
      .impl = nullptr};

  return &buffer_array_allocator;
}
}  // namespace aimrt::runtime::core::util
