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
    AIMRT_ASSERT(base_ptr_, "Reference is null.");
    base_ptr_->reserve(base_ptr_->impl, buffer_array, new_cap);
  }

  aimrt_buffer_t Allocate(aimrt_buffer_array_t* buffer_array, size_t size) {
    AIMRT_ASSERT(base_ptr_, "Reference is null.");
    return base_ptr_->allocate(base_ptr_->impl, buffer_array, size);
  }

  void Release(aimrt_buffer_array_t* buffer_array) {
    AIMRT_ASSERT(base_ptr_, "Reference is null.");
    base_ptr_->release(base_ptr_->impl, buffer_array);
  }

 private:
  const aimrt_buffer_array_allocator_t* base_ptr_ = nullptr;
};

class BufferArray {
 public:
  explicit BufferArray(
      const aimrt_buffer_array_allocator_t* allocator = SimpleBufferArrayAllocator::NativeHandle()) {
    base_ = aimrt_buffer_array_t{
        .data = nullptr,
        .len = 0,
        .capacity = 0,
        .allocator = allocator};
  }
  explicit BufferArray(BufferArrayAllocatorRef allocator) {
    base_ = aimrt_buffer_array_t{
        .data = nullptr,
        .len = 0,
        .capacity = 0,
        .allocator = allocator.NativeHandle()};
  }

  ~BufferArray() { base_.allocator->release(base_.allocator->impl, &base_); }

  BufferArray(const BufferArray&) = delete;
  BufferArray& operator=(const BufferArray&) = delete;

  const aimrt_buffer_array_t* NativeHandle() const { return &base_; }
  aimrt_buffer_array_t* NativeHandle() { return &base_; }

  size_t Size() const { return base_.len; }

  size_t Capacity() const { return base_.capacity; }

  aimrt_buffer_t* Data() const { return base_.data; }

  void Reserve(size_t new_cap) {
    base_.allocator->reserve(base_.allocator->impl, &base_, new_cap);
  }

  aimrt_buffer_t NewBuffer(size_t size) {
    return base_.allocator->allocate(base_.allocator->impl, &base_, size);
  }

  size_t BufferSize() const {
    size_t result = 0;
    for (size_t ii = 0; ii < base_.len; ++ii) {
      result += base_.data[ii].len;
    }
    return result;
  }

 private:
  aimrt_buffer_array_t base_;
};

}  // namespace aimrt::util
