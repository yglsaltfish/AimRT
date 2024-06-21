#pragma once

#include <chrono>
#include <cinttypes>
#include <memory>
#include <string_view>
#include <unordered_map>

#include "aimrt_module_c_interface/rpc/rpc_context_base.h"
#include "aimrt_module_cpp_interface/util/string.h"
#include "util/exception.h"
#include "util/stl_tool.h"
#include "util/string_util.h"
#include "util/time_util.h"

namespace aimrt::rpc {

class Context {
 public:
  Context() : base_(aimrt_rpc_context_base_t{
                  .ops = GenOpsBase(),
                  .impl = this}) {}
  ~Context() = default;

  const aimrt_rpc_context_base_t* NativeHandle() const { return &base_; }

  // Timeout
  std::chrono::nanoseconds Timeout() const {
    return std::chrono::nanoseconds(timeout_ns_);
  }

  void SetTimeout(std::chrono::nanoseconds timeout) {
    timeout_ns_ = timeout.count();
  }

  // Some frame fields
  std::string_view GetMetaValue(std::string_view key) const {
    auto finditr = meta_data_map_.find(key);
    if (finditr != meta_data_map_.end()) return finditr->second;
    return "";
  }

  void SetMetaValue(std::string_view key, std::string_view val) {
    auto finditr = meta_data_map_.find(key);
    if (finditr != meta_data_map_.end()) {
      finditr->second = std::string(val);
      return;
    }

    meta_data_map_.emplace(key, val);
  }

  std::set<std::string_view> GetMetaKeys() const {
    std::set<std::string_view> result;
    for (const auto& it : meta_data_map_) result.emplace(it.first);
    return result;
  }

  std::string_view GetToAddr() const {
    return GetMetaValue(AIMRT_RPC_CONTEXT_KEY_TO_ADDR);
  }
  void SetToAddr(std::string_view val) {
    SetMetaValue(AIMRT_RPC_CONTEXT_KEY_TO_ADDR, val);
  }

  std::string_view GetSerializationType() const {
    return GetMetaValue(AIMRT_RPC_CONTEXT_KEY_SERIALIZATION_TYPE);
  }
  void SetSerializationType(std::string_view val) {
    SetMetaValue(AIMRT_RPC_CONTEXT_KEY_SERIALIZATION_TYPE, val);
  }

  std::string ToString() const {
    std::stringstream ss;
    ss << "timeout: " << timeout_ns_ / 1000000 << "ms, meta: {";
    bool flag = true;
    for (const auto& itr : meta_data_map_) {
      if (flag)
        flag = false;
      else
        ss << ",";

      ss << "{" << itr.first << "," << itr.second << "}";
    }

    ss << "}";

    return ss.str();
  }

 private:
  static const aimrt_rpc_context_base_ops_t* GenOpsBase() {
    static constexpr aimrt_rpc_context_base_ops_t ops{
        .get_timeout_ns = [](void* impl) -> uint64_t {
          return static_cast<Context*>(impl)->timeout_ns_;
        },
        .set_timeout_ns = [](void* impl, uint64_t timeout) {
          static_cast<Context*>(impl)->timeout_ns_ = timeout;  //
        },
        .get_meta_val = [](void* impl, aimrt_string_view_t key) -> aimrt_string_view_t {
          return aimrt::util::ToAimRTStringView(
              static_cast<Context*>(impl)->GetMetaValue(aimrt::util::ToStdStringView(key)));
        },
        .set_meta_val = [](void* impl, aimrt_string_view_t key, aimrt_string_view_t val) {
          static_cast<Context*>(impl)->SetMetaValue(
              aimrt::util::ToStdStringView(key), aimrt::util::ToStdStringView(val));  //
        },
        .get_meta_keys = [](void* impl) -> aimrt_string_view_array_t {
          const auto& meta_data_map = static_cast<Context*>(impl)->meta_data_map_;
          auto& meta_keys_vec = static_cast<Context*>(impl)->meta_keys_vec_;

          meta_keys_vec.clear();
          meta_keys_vec.reserve(meta_data_map.size());

          for (const auto& it : meta_data_map) {
            meta_keys_vec.emplace_back(aimrt::util::ToAimRTStringView(it.first));
          }

          return aimrt_string_view_array_t{
              .str_array = meta_keys_vec.data(),
              .len = meta_keys_vec.size()};
        }};

    return &ops;
  }

 private:
  uint64_t timeout_ns_ = 0;

  std::unordered_map<
      std::string,
      std::string,
      aimrt::common::util::StringHash,
      std::equal_to<>>
      meta_data_map_;

  std::vector<aimrt_string_view_t> meta_keys_vec_;

  const aimrt_rpc_context_base_t base_;
};

class ContextRef {
 public:
  ContextRef() = default;
  ContextRef(const Context& ctx)
      : base_ptr_(ctx.NativeHandle()) {}
  ContextRef(const Context* ctx_ptr)
      : base_ptr_(ctx_ptr ? ctx_ptr->NativeHandle() : nullptr) {}
  ContextRef(const std::shared_ptr<Context>& ctx_ptr)
      : base_ptr_(ctx_ptr ? ctx_ptr->NativeHandle() : nullptr) {}
  explicit ContextRef(const aimrt_rpc_context_base_t* base_ptr)
      : base_ptr_(base_ptr) {}
  ~ContextRef() = default;

  explicit operator bool() const { return (base_ptr_ != nullptr); }

  const aimrt_rpc_context_base_t* NativeHandle() const {
    return base_ptr_;
  }

  // Timeout manager
  std::chrono::nanoseconds Timeout() const {
    AIMRT_ASSERT(base_ptr_ && base_ptr_->ops, "Reference is null.");
    return std::chrono::nanoseconds(base_ptr_->ops->get_timeout_ns(base_ptr_->impl));
  }

  void SetTimeout(std::chrono::nanoseconds timeout) {
    AIMRT_ASSERT(base_ptr_ && base_ptr_->ops, "Reference is null.");
    base_ptr_->ops->set_timeout_ns(base_ptr_->impl, timeout.count());
  }

  // Some frame fields
  std::string_view GetMetaValue(std::string_view key) const {
    AIMRT_ASSERT(base_ptr_ && base_ptr_->ops, "Reference is null.");
    return aimrt::util::ToStdStringView(
        base_ptr_->ops->get_meta_val(base_ptr_->impl, aimrt::util::ToAimRTStringView(key)));
  }

  void SetMetaValue(std::string_view key, std::string_view val) {
    AIMRT_ASSERT(base_ptr_ && base_ptr_->ops, "Reference is null.");
    base_ptr_->ops->set_meta_val(
        base_ptr_->impl, aimrt::util::ToAimRTStringView(key), aimrt::util::ToAimRTStringView(val));
  }

  std::set<std::string_view> GetMetaKeys() const {
    AIMRT_ASSERT(base_ptr_ && base_ptr_->ops, "Reference is null.");
    aimrt_string_view_array_t keys = base_ptr_->ops->get_meta_keys(base_ptr_->impl);

    std::set<std::string_view> result;
    for (size_t ii = 0; ii < keys.len; ++ii) {
      result.emplace(aimrt::util::ToStdStringView(keys.str_array[ii]));
    }

    return result;
  }

  std::string_view GetToAddr() const {
    return GetMetaValue(AIMRT_RPC_CONTEXT_KEY_TO_ADDR);
  }
  void SetToAddr(std::string_view val) {
    SetMetaValue(AIMRT_RPC_CONTEXT_KEY_TO_ADDR, val.data());
  }

  std::string_view GetSerializationType() const {
    return GetMetaValue(AIMRT_RPC_CONTEXT_KEY_SERIALIZATION_TYPE);
  }
  void SetSerializationType(std::string_view val) {
    SetMetaValue(AIMRT_RPC_CONTEXT_KEY_SERIALIZATION_TYPE, val.data());
  }

  std::string ToString() const {
    AIMRT_ASSERT(base_ptr_ && base_ptr_->ops, "Reference is null.");

    std::stringstream ss;

    auto timeout = base_ptr_->ops->get_timeout_ns(base_ptr_->impl);
    ss << "timeout: " << timeout / 1000000 << "ms, meta: {";

    aimrt_string_view_array_t keys = base_ptr_->ops->get_meta_keys(base_ptr_->impl);
    for (size_t ii = 0; ii < keys.len; ++ii) {
      if (ii != 0) ss << ",";
      auto val = base_ptr_->ops->get_meta_val(base_ptr_->impl, keys.str_array[ii]);
      ss << "{" << aimrt::util::ToStdStringView(keys.str_array[ii])
         << "," << aimrt::util::ToStdStringView(val) << "}";
    }

    ss << "}";

    return ss.str();
  }

 private:
  const aimrt_rpc_context_base_t* base_ptr_;
};

}  // namespace aimrt::rpc