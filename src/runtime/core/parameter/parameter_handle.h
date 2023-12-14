#pragma once

#include <atomic>
#include <mutex>
#include <string>
#include <vector>

#include "aimrt_module_c_interface/parameter/parameter_handle_base.h"
#include "tbb/concurrent_hash_map.h"

namespace aimrt::runtime::core::parameter {

class Parameter {
 public:
  Parameter()
      : view_(aimrt_parameter_view_t{.type = aimrt_parameter_type_t::AIMRT_PARAMETER_TYPE_NULL}) {}
  ~Parameter() = default;

  Parameter(const Parameter&) = delete;
  Parameter& operator=(const Parameter&) = delete;

  bool SetData(aimrt_parameter_view_t parameter_view);

  aimrt_parameter_view_t GetView() const { return view_; }

  bool IsArray() const { return !(array_data_.empty()); }

 private:
  aimrt_parameter_view_t view_;
  std::vector<uint8_t> array_data_;
};

class ParameterHandle {
 public:
  ParameterHandle() = default;
  ~ParameterHandle() = default;

  ParameterHandle(const ParameterHandle&) = delete;
  ParameterHandle& operator=(const ParameterHandle&) = delete;

  std::shared_ptr<Parameter> GetParameter(std::string_view key);

  void SetParameter(std::string_view key, const std::shared_ptr<Parameter>& parameter_ptr);

 private:
  struct StringHashCompare {
    using is_transparent = void;

    bool equal(const std::string& lhs, const std::string& rhs) const { return lhs == rhs; }
    bool equal(const std::string& lhs, std::string_view rhs) const { return lhs == rhs; }
    bool equal(std::string_view lhs, const std::string& rhs) const { return lhs == rhs; }

    std::size_t hash(const std::string& k) const { return std::hash<std::string_view>{}(k); }
    std::size_t hash(std::string_view k) const { return std::hash<std::string_view>{}(k); }
  };

  // TODO: 编译器支持后可以换成std::atomic<std::shared_ptr<Parameter>
  struct ParameterWrap {
    explicit ParameterWrap(const std::shared_ptr<Parameter>& input_ptr)
        : ptr(input_ptr) {}
    std::shared_ptr<Parameter> ptr;
    mutable std::mutex mu;
  };
  using ParameterMap = tbb::concurrent_hash_map<std::string, ParameterWrap, StringHashCompare>;
  ParameterMap parameter_map_;
};

}  // namespace aimrt::runtime::core::parameter
