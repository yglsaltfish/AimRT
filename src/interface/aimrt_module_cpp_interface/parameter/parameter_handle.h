#pragma once

#include <cassert>
#include <memory>

#include "aimrt_module_c_interface/parameter/parameter_handle_base.h"

namespace aimrt::parameter {

class ParameterHandleRef {
 public:
  ParameterHandleRef() = default;
  explicit ParameterHandleRef(const aimrt_parameter_handle_base_t* base_ptr)
      : base_ptr_(base_ptr) {}
  ~ParameterHandleRef() = default;

  explicit operator bool() const { return (base_ptr_ != nullptr); }

  const aimrt_parameter_handle_base_t* NativeHandle() const {
    return base_ptr_;
  }

  // TODO

  template <typename T>
  void GetParameter() {
    assert(base_ptr_);
  }

  template <typename T>
  void SetParameter() {
    assert(base_ptr_);
  }

 private:
  const aimrt_parameter_handle_base_t* base_ptr_ = nullptr;
};

}  // namespace aimrt::parameter
