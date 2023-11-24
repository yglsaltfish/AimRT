#pragma once

#include <cassert>
#include <chrono>
#include <cinttypes>
#include <memory>
#include <string_view>

#include "aimrt_module_c_interface/channel/channel_context_base.h"
#include "aimrt_module_cpp_interface/util/string.h"

namespace aimrt::channel {

using ContextSharedPtr = std::shared_ptr<const aimrt_channel_context_base_t>;

class ContextRef {
 public:
  ContextRef() = default;
  explicit ContextRef(const ContextSharedPtr& ctx)
      : base_ptr_(ctx.get()) {}
  explicit ContextRef(const aimrt_channel_context_base_t* base_ptr)
      : base_ptr_(base_ptr) {}
  ~ContextRef() = default;

  explicit operator bool() const { return (base_ptr_ != nullptr); }

  const aimrt_channel_context_base_t* NativeHandle() const {
    return base_ptr_;
  }

  // Timestamp
  std::chrono::steady_clock::time_point GetMsgTimestampNs() const {
    assert(base_ptr_ && base_ptr_->ops);
    return std::chrono::steady_clock::time_point(std::chrono::nanoseconds(
        base_ptr_->ops->get_msg_timestamp_ns(base_ptr_->impl)));
  }

  void SetMsgTimestampNs(
      const std::chrono::steady_clock::time_point& deadline) {
    assert(base_ptr_ && base_ptr_->ops);
    base_ptr_->ops->set_msg_timestamp_ns(
        base_ptr_->impl,
        static_cast<uint64_t>(
            std::chrono::duration_cast<std::chrono::nanoseconds>(
                deadline.time_since_epoch())
                .count()));
  }

  // Some frame fields
  std::string_view GetMetaValue(std::string_view key) const {
    assert(base_ptr_ && base_ptr_->ops);
    return aimrt::util::ToStdStringView(base_ptr_->ops->get_meta_val(base_ptr_->impl, aimrt::util::ToAimRTStringView(key)));
  }

  void SetMetaValue(std::string_view key, std::string_view val) {
    assert(base_ptr_ && base_ptr_->ops);
    base_ptr_->ops->set_meta_val(base_ptr_->impl, aimrt::util::ToAimRTStringView(key), aimrt::util::ToAimRTStringView(val));
  }

  std::string_view GetFromAddr() const {
    return GetMetaValue(AIMRT_CHANNEL_CONTEXT_KEY_FROM_ADDR);
  }

  std::string_view GetSerializationType() const {
    return GetMetaValue(AIMRT_CHANNEL_CONTEXT_KEY_SERIALIZATION_TYPE);
  }
  void SetSerializationType(std::string_view val) {
    SetMetaValue(AIMRT_CHANNEL_CONTEXT_KEY_SERIALIZATION_TYPE, val.data());
  }

 private:
  const aimrt_channel_context_base_t* base_ptr_;
};

}  // namespace aimrt::channel