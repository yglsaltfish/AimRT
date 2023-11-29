#pragma once

#include <cassert>

#include "aimrt_module_c_interface/allocator/allocator_base.h"

namespace aimrt::allocator {

class AllocatorRef {
 public:
  AllocatorRef() = default;
  explicit AllocatorRef(const aimrt_allocator_base_t* base_ptr)
      : base_ptr_(base_ptr) {}
  ~AllocatorRef() = default;

  explicit operator bool() const { return (base_ptr_ != nullptr); }

  const aimrt_allocator_base_t* NativeHandle() const { return base_ptr_; }

  void* AllocateThreadLocalBuf(size_t buf_size) {
    assert(base_ptr_);
    return base_ptr_->allocate_thread_local_buf(base_ptr_->impl, buf_size);
  }

  void ReleaseThreadLocalBuf() {
    assert(base_ptr_);
    base_ptr_->release_thread_local_buf(base_ptr_->impl);
  }

 private:
  const aimrt_allocator_base_t* base_ptr_ = nullptr;
};

}  // namespace aimrt::allocator
