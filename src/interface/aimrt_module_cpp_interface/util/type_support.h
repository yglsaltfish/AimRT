#pragma once

#include <cassert>
#include <memory>
#include <span>

#include "aimrt_module_c_interface/util/type_support_base.h"
#include "aimrt_module_cpp_interface/util/string.h"

namespace aimrt::util {

class TypeSupportRef {
 public:
  TypeSupportRef() = default;
  explicit TypeSupportRef(const aimrt_type_support_base_t* base_ptr)
      : base_ptr_(base_ptr) {}
  ~TypeSupportRef() = default;

  explicit operator bool() const { return (base_ptr_ != nullptr); }

  const aimrt_type_support_base_t* NativeHandle() const {
    return base_ptr_;
  }

  std::string_view TypeName() const {
    assert(base_ptr_);
    return ToStdStringView(base_ptr_->type_name(base_ptr_->impl));
  }

  void* Create() const {
    assert(base_ptr_);
    return base_ptr_->create(base_ptr_->impl);
  }

  void Destory(void* msg) const {
    assert(base_ptr_);
    base_ptr_->destory(base_ptr_->impl, msg);
  }

  std::shared_ptr<void> CreateSharedPtr() {
    assert(base_ptr_);
    return std::shared_ptr<void>(
        base_ptr_->create(base_ptr_->impl),
        [base_ptr{this->base_ptr_}](void* ptr) {
          base_ptr->destory(base_ptr->impl, ptr);
        });
  }

  void Copy(const void* from, void* to) const {
    assert(base_ptr_);
    base_ptr_->copy(base_ptr_->impl, from, to);
  }

  void Move(void* from, void* to) const {
    assert(base_ptr_);
    base_ptr_->move(base_ptr_->impl, from, to);
  }

  bool Serialize(
      std::string_view serialization_type,
      const void* msg,
      aimrt_buffer_array_t* buffer_array) const {
    assert(base_ptr_);
    return base_ptr_->serialize(
        base_ptr_->impl,
        ToAimRTStringView(serialization_type),
        msg,
        buffer_array);
  }

  bool Deserialize(
      std::string_view serialization_type,
      aimrt_buffer_array_view_t buffer_array_view,
      void* msg) const {
    assert(base_ptr_);
    return base_ptr_->deserialize(
        base_ptr_->impl,
        ToAimRTStringView(serialization_type),
        buffer_array_view,
        msg);
  }

  size_t SerializationTypesSupportedNum() const {
    assert(base_ptr_);
    return base_ptr_->serialization_types_supported_num(base_ptr_->impl);
  }

  const aimrt_string_view_t* SerializationTypesSupportedList() const {
    assert(base_ptr_);
    return base_ptr_->serialization_types_supported_list(base_ptr_->impl);
  }

  std::span<const aimrt_string_view_t> SerializationTypesSupportedListSpan() const {
    assert(base_ptr_);
    return std::span<const aimrt_string_view_t>(
        base_ptr_->serialization_types_supported_list(base_ptr_->impl),
        base_ptr_->serialization_types_supported_num(base_ptr_->impl));
  }

  const void* CustomTypeSupportPtr() const {
    assert(base_ptr_);
    return base_ptr_->custom_type_support_ptr(base_ptr_->impl);
  }

 private:
  const aimrt_type_support_base_t* base_ptr_ = nullptr;
};

}  // namespace aimrt::util
