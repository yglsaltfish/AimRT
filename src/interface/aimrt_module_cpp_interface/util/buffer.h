#pragma once

#include "aimrt_module_c_interface/util/buffer_base.h"
#include "aimrt_module_cpp_interface/util/simple_buffer_array_allocator.h"
#include "util/exception.h"

namespace aimrt::util {

class BufferArrayAllocatorRef {
 public:
  BufferArrayAllocatorRef() = default;
  explicit BufferArrayAllocatorRef(const aimrt_buffer_array_allocator_t* base_ptr)
      : base_ptr_(base_ptr) {}
  ~BufferArrayAllocatorRef() = default;

  explicit operator bool() const { return (base_ptr_ != nullptr); }

  const aimrt_buffer_array_allocator_t* NativeHandle() const {
    return base_ptr_;
  }

  void Reserve(aimrt_buffer_array_t* buffer_array, size_t new_cap) {
    base_ptr_->reserve(base_ptr_->impl, buffer_array, new_cap);
  }

  aimrt_buffer_t Allocate(aimrt_buffer_array_t* buffer_array, size_t size) {
    return base_ptr_->allocate(base_ptr_->impl, buffer_array, size);
  }

  void Release(aimrt_buffer_array_t* buffer_array) {
    base_ptr_->release(base_ptr_->impl, buffer_array);
  }

 private:
  const aimrt_buffer_array_allocator_t* base_ptr_ = nullptr;
};

class BufferArray {
 public:
  explicit BufferArray(
      const aimrt_buffer_array_allocator_t* allocator = SimpleBufferArrayAllocator::NativeHandle())
      : buffer_array_(aimrt_buffer_array_t{.data = nullptr, .len = 0, .capacity = 0}),
        allocator_ptr_(allocator) {}

  explicit BufferArray(BufferArrayAllocatorRef allocator)
      : buffer_array_(aimrt_buffer_array_t{.data = nullptr, .len = 0, .capacity = 0}),
        allocator_ptr_(allocator.NativeHandle()) {}

  ~BufferArray() { allocator_ptr_->release(allocator_ptr_->impl, &buffer_array_); }

  BufferArray(const BufferArray&) = delete;
  BufferArray& operator=(const BufferArray&) = delete;

  const aimrt_buffer_array_t* BufferArrayNativeHandle() const { return &buffer_array_; }
  aimrt_buffer_array_t* BufferArrayNativeHandle() { return &buffer_array_; }

  const aimrt_buffer_array_allocator_t* AllocatorNativeHandle() const { return allocator_ptr_; }

  size_t Size() const { return buffer_array_.len; }

  size_t Capacity() const { return buffer_array_.capacity; }

  aimrt_buffer_t* Data() const { return buffer_array_.data; }

  void Reserve(size_t new_cap) {
    allocator_ptr_->reserve(allocator_ptr_->impl, &buffer_array_, new_cap);
  }

  aimrt_buffer_t NewBuffer(size_t size) {
    return allocator_ptr_->allocate(allocator_ptr_->impl, &buffer_array_, size);
  }

  size_t BufferSize() const {
    size_t result = 0;
    for (size_t ii = 0; ii < buffer_array_.len; ++ii) {
      result += buffer_array_.data[ii].len;
    }
    return result;
  }

 private:
  aimrt_buffer_array_t buffer_array_;
  const aimrt_buffer_array_allocator_t* allocator_ptr_;
};

}  // namespace aimrt::util
