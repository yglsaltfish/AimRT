#pragma once

#include <cassert>
#include <concepts>
#include <exception>
#include <memory>
#include <span>
#include <vector>

#include "aimrt_module_c_interface/parameter/parameter_handle_base.h"
#include "aimrt_module_cpp_interface/util/function.h"
#include "aimrt_module_cpp_interface/util/string.h"

namespace aimrt::parameter {

class ParameterView {
 public:
  explicit ParameterView(aimrt_parameter_view_t base) : base_(base) {}

  ParameterView() {
    base_.type = aimrt_parameter_type_t::AIMRT_PARAMETER_TYPE_NULL;
  }

  ParameterView(bool val) {
    base_.type = aimrt_parameter_type_t::AIMRT_PARAMETER_TYPE_BOOL;
    base_.data.b = val;
  }

  template <std::signed_integral T>
  ParameterView(T val) {
    base_.type = aimrt_parameter_type_t::AIMRT_PARAMETER_TYPE_INTEGER;
    base_.data.i = val;
  }

  template <std::unsigned_integral T>
  ParameterView(T val) {
    base_.type = aimrt_parameter_type_t::AIMRT_PARAMETER_TYPE_UNSIGNED_INTEGER;
    base_.data.u = val;
  }

  template <std::floating_point T>
  ParameterView(T val) {
    base_.type = aimrt_parameter_type_t::AIMRT_PARAMETER_TYPE_DOUBLE;
    base_.data.f = val;
  }

  ParameterView(std::string_view val) {
    base_.type = aimrt_parameter_type_t::AIMRT_PARAMETER_TYPE_STRING;
    base_.data.array.data = val.data();
    base_.data.array.type_size = 1;
    base_.data.array.len = val.size();
  }

  ParameterView(const std::string& val) {
    base_.type = aimrt_parameter_type_t::AIMRT_PARAMETER_TYPE_STRING;
    base_.data.array.data = val.c_str();
    base_.data.array.type_size = 1;
    base_.data.array.len = val.size();
  }

  ParameterView(void* buf, size_t len) {
    base_.type = aimrt_parameter_type_t::AIMRT_PARAMETER_TYPE_BYTE_ARRAY;
    base_.data.array.data = buf;
    base_.data.array.type_size = 1;
    base_.data.array.len = len;
  }

  ParameterView(std::span<bool> val) {
    base_.type = aimrt_parameter_type_t::AIMRT_PARAMETER_TYPE_BOOL_ARRAY;
    base_.data.array.data = val.data();
    base_.data.array.type_size = 1;
    base_.data.array.len = val.size();
  }

  ParameterView(const std::vector<bool>& val) {
    const size_t len = val.size();
    convert_buf_.resize(len);
    for (size_t ii = 0; ii < len; ++ii) {
      convert_buf_[ii] = (val[ii]) ? 1 : 0;
    }

    base_.type = aimrt_parameter_type_t::AIMRT_PARAMETER_TYPE_BOOL_ARRAY;
    base_.data.array.data = convert_buf_.data();
    base_.data.array.type_size = 1;
    base_.data.array.len = len;
  }

  template <std::signed_integral T>
  ParameterView(std::span<T> val) {
    base_.type = aimrt_parameter_type_t::AIMRT_PARAMETER_TYPE_INTEGER_ARRAY;
    base_.data.array.data = val.data();
    base_.data.array.type_size = sizeof(T);
    base_.data.array.len = val.size();
  }

  template <std::signed_integral T>
  ParameterView(const std::vector<T>& val) {
    base_.type = aimrt_parameter_type_t::AIMRT_PARAMETER_TYPE_INTEGER_ARRAY;
    base_.data.array.data = val.data();
    base_.data.array.type_size = sizeof(T);
    base_.data.array.len = val.size();
  }

  template <std::unsigned_integral T>
  ParameterView(std::span<T> val) {
    base_.type = aimrt_parameter_type_t::AIMRT_PARAMETER_TYPE_UNSIGNED_INTEGER_ARRAY;
    base_.data.array.data = val.data();
    base_.data.array.type_size = sizeof(T);
    base_.data.array.len = val.size();
  }

  template <std::unsigned_integral T>
  ParameterView(const std::vector<T>& val) {
    base_.type = aimrt_parameter_type_t::AIMRT_PARAMETER_TYPE_UNSIGNED_INTEGER_ARRAY;
    base_.data.array.data = val.data();
    base_.data.array.type_size = sizeof(T);
    base_.data.array.len = val.size();
  }

  template <std::floating_point T>
  ParameterView(std::span<T> val) {
    base_.type = aimrt_parameter_type_t::AIMRT_PARAMETER_TYPE_DOUBLE_ARRAY;
    base_.data.array.data = val.data();
    base_.data.array.type_size = sizeof(T);
    base_.data.array.len = val.size();
  }

  template <std::floating_point T>
  ParameterView(const std::vector<T>& val) {
    base_.type = aimrt_parameter_type_t::AIMRT_PARAMETER_TYPE_DOUBLE_ARRAY;
    base_.data.array.data = val.data();
    base_.data.array.type_size = sizeof(T);
    base_.data.array.len = val.size();
  }

  ParameterView(std::span<aimrt_string_view_t> val) {
    base_.type = aimrt_parameter_type_t::AIMRT_PARAMETER_TYPE_STRING_ARRAY;
    base_.data.array.data = val.data();
    base_.data.array.type_size = sizeof(aimrt_string_view_t);
    base_.data.array.len = val.size();
  }

  template <typename T>
    requires std::is_same_v<std::string_view, T> || std::is_same_v<std::string, T>
  ParameterView(std::span<T> val) {
    const size_t len = val.size();
    convert_buf_.resize(len * sizeof(aimrt_string_view_t));
    auto* real_val = reinterpret_cast<aimrt_string_view_t*>(convert_buf_.data());
    for (size_t ii = 0; ii < len; ++ii) {
      real_val[ii] = util::ToAimRTStringView(val[ii]);
    }

    base_.type = aimrt_parameter_type_t::AIMRT_PARAMETER_TYPE_STRING_ARRAY;
    base_.data.array.data = convert_buf_.data();
    base_.data.array.type_size = sizeof(aimrt_string_view_t);
    base_.data.array.len = len;
  }

  ParameterView(const std::vector<aimrt_string_view_t>& val) {
    base_.type = aimrt_parameter_type_t::AIMRT_PARAMETER_TYPE_STRING_ARRAY;
    base_.data.array.data = val.data();
    base_.data.array.type_size = sizeof(aimrt_string_view_t);
    base_.data.array.len = val.size();
  }

  template <typename T>
    requires std::is_same_v<std::string_view, T> || std::is_same_v<std::string, T>
  ParameterView(const std::vector<T>& val) {
    const size_t len = val.size();
    convert_buf_.resize(len * sizeof(aimrt_string_view_t));
    auto* real_val = reinterpret_cast<aimrt_string_view_t*>(convert_buf_.data());
    for (size_t ii = 0; ii < len; ++ii) {
      real_val[ii] = util::ToAimRTStringView(val[ii]);
    }

    base_.type = aimrt_parameter_type_t::AIMRT_PARAMETER_TYPE_STRING_ARRAY;
    base_.data.array.data = convert_buf_.data();
    base_.data.array.type_size = sizeof(aimrt_string_view_t);
    base_.data.array.len = len;
  }

  ~ParameterView() = default;

  ParameterView(const ParameterView&) = delete;
  ParameterView& operator=(const ParameterView&) = delete;

  template <typename T>
  T As() const {
    if constexpr (std::is_same_v<T, bool>) {
      if (base_.type == aimrt_parameter_type_t::AIMRT_PARAMETER_TYPE_BOOL)
        return base_.data.b;

      throw std::runtime_error("Can not convert parameter to bool");
    } else if constexpr (std::signed_integral<T>) {
      if (base_.type == aimrt_parameter_type_t::AIMRT_PARAMETER_TYPE_INTEGER)
        return static_cast<T>(base_.data.i);

      throw std::runtime_error("Can not convert parameter to int");
    } else if constexpr (std::unsigned_integral<T>) {
      if (base_.type == aimrt_parameter_type_t::AIMRT_PARAMETER_TYPE_UNSIGNED_INTEGER)
        return static_cast<T>(base_.data.u);

      throw std::runtime_error("Can not convert parameter to unsigned int");
    } else if constexpr (std::floating_point<T>) {
      if (base_.type == aimrt_parameter_type_t::AIMRT_PARAMETER_TYPE_DOUBLE)
        return static_cast<T>(base_.data.f);

      throw std::runtime_error("Can not convert parameter to floating point num");
    } else if constexpr (std::is_same_v<T, std::string_view> ||
                         std::is_same_v<T, std::string> ||
                         std::is_same_v<T, std::span<const char>>) {
      if (base_.type == aimrt_parameter_type_t::AIMRT_PARAMETER_TYPE_STRING &&
          base_.data.array.type_size == 1)
        return T(static_cast<const char*>(base_.data.array.data), base_.data.array.len);

      throw std::runtime_error("Can not convert parameter to string");
    } else if constexpr (std::is_same_v<T, std::vector<char>>) {
      if (base_.type == aimrt_parameter_type_t::AIMRT_PARAMETER_TYPE_STRING &&
          base_.data.array.type_size == 1) {
        std::vector<char> result(base_.data.array.len);
        memcpy(result.data(), base_.data.array.data, base_.data.array.len);
        return result;
      }

      throw std::runtime_error("Can not convert parameter to char array");
    } else if constexpr (std::is_same_v<T, std::span<const bool>>) {
      if ((base_.type == aimrt_parameter_type_t::AIMRT_PARAMETER_TYPE_BOOL_ARRAY ||
           base_.type == aimrt_parameter_type_t::AIMRT_PARAMETER_TYPE_BYTE_ARRAY ||
           base_.type == aimrt_parameter_type_t::AIMRT_PARAMETER_TYPE_INTEGER_ARRAY ||
           base_.type == aimrt_parameter_type_t::AIMRT_PARAMETER_TYPE_UNSIGNED_INTEGER_ARRAY) &&
          base_.data.array.type_size == 1) {
        return std::span<const bool>(
            static_cast<const bool*>(base_.data.array.data), base_.data.array.len);
      }

      throw std::runtime_error("Can not convert parameter to bool array");
    } else if constexpr (std::is_same_v<T, std::vector<bool>>) {
      if ((base_.type == aimrt_parameter_type_t::AIMRT_PARAMETER_TYPE_BOOL_ARRAY ||
           base_.type == aimrt_parameter_type_t::AIMRT_PARAMETER_TYPE_BYTE_ARRAY ||
           base_.type == aimrt_parameter_type_t::AIMRT_PARAMETER_TYPE_INTEGER_ARRAY ||
           base_.type == aimrt_parameter_type_t::AIMRT_PARAMETER_TYPE_UNSIGNED_INTEGER_ARRAY) &&
          base_.data.array.type_size == 1) {
        std::vector<bool> result(base_.data.array.len);
        for (size_t ii = 0; ii < base_.data.array.len; ++ii) {
          result[ii] = static_cast<const bool*>(base_.data.array.data)[ii];
        }

        return result;
      }

      throw std::runtime_error("Can not convert parameter to bool array");
    } else if constexpr (std::is_same_v<T, std::span<const uint8_t>>) {
      if ((base_.type == aimrt_parameter_type_t::AIMRT_PARAMETER_TYPE_BOOL_ARRAY ||
           base_.type == aimrt_parameter_type_t::AIMRT_PARAMETER_TYPE_BYTE_ARRAY ||
           base_.type == aimrt_parameter_type_t::AIMRT_PARAMETER_TYPE_UNSIGNED_INTEGER_ARRAY) &&
          base_.data.array.type_size == 1) {
        return std::span<const uint8_t>(
            static_cast<const uint8_t*>(base_.data.array.data), base_.data.array.len);
      }

      throw std::runtime_error("Can not convert parameter to uint8_t array");
    } else if constexpr (std::is_same_v<T, std::vector<uint8_t>>) {
      if ((base_.type == aimrt_parameter_type_t::AIMRT_PARAMETER_TYPE_BOOL_ARRAY ||
           base_.type == aimrt_parameter_type_t::AIMRT_PARAMETER_TYPE_BYTE_ARRAY ||
           base_.type == aimrt_parameter_type_t::AIMRT_PARAMETER_TYPE_UNSIGNED_INTEGER_ARRAY) &&
          base_.data.array.type_size == 1) {
        std::vector<uint8_t> result(base_.data.array.len);
        memcpy(result.data(), base_.data.array.data, base_.data.array.len);
        return result;
      }

      throw std::runtime_error("Can not convert parameter to uint8_t array");
    } else if constexpr (std::is_same_v<T, std::span<const uint16_t>>) {
      if (base_.type == aimrt_parameter_type_t::AIMRT_PARAMETER_TYPE_UNSIGNED_INTEGER_ARRAY &&
          base_.data.array.type_size == 2) {
        return std::span<const uint16_t>(
            static_cast<const uint16_t*>(base_.data.array.data), base_.data.array.len);
      }

      throw std::runtime_error("Can not convert parameter to uint16_t array");
    } else if constexpr (std::is_same_v<T, std::vector<uint16_t>>) {
      if (base_.type == aimrt_parameter_type_t::AIMRT_PARAMETER_TYPE_UNSIGNED_INTEGER_ARRAY &&
          base_.data.array.type_size == 2) {
        std::vector<uint16_t> result(base_.data.array.len);
        memcpy(result.data(), base_.data.array.data, base_.data.array.len << 1);
        return result;
      }

      throw std::runtime_error("Can not convert parameter to uint16_t array");
    } else if constexpr (std::is_same_v<T, std::span<const uint32_t>>) {
      if (base_.type == aimrt_parameter_type_t::AIMRT_PARAMETER_TYPE_UNSIGNED_INTEGER_ARRAY &&
          base_.data.array.type_size == 4) {
        return std::span<const uint32_t>(
            static_cast<const uint32_t*>(base_.data.array.data), base_.data.array.len);
      }

      throw std::runtime_error("Can not convert parameter to uint32_t array");
    } else if constexpr (std::is_same_v<T, std::vector<uint32_t>>) {
      if (base_.type == aimrt_parameter_type_t::AIMRT_PARAMETER_TYPE_UNSIGNED_INTEGER_ARRAY &&
          base_.data.array.type_size == 4) {
        std::vector<uint32_t> result(base_.data.array.len);
        memcpy(result.data(), base_.data.array.data, base_.data.array.len << 2);
        return result;
      }

      throw std::runtime_error("Can not convert parameter to uint32_t array");
    } else if constexpr (std::is_same_v<T, std::span<const uint64_t>>) {
      if (base_.type == aimrt_parameter_type_t::AIMRT_PARAMETER_TYPE_UNSIGNED_INTEGER_ARRAY &&
          base_.data.array.type_size == 8) {
        return std::span<const uint64_t>(
            static_cast<const uint64_t*>(base_.data.array.data), base_.data.array.len);
      }

      throw std::runtime_error("Can not convert parameter to uint64_t array");
    } else if constexpr (std::is_same_v<T, std::vector<uint64_t>>) {
      if (base_.type == aimrt_parameter_type_t::AIMRT_PARAMETER_TYPE_UNSIGNED_INTEGER_ARRAY &&
          base_.data.array.type_size == 8) {
        std::vector<uint64_t> result(base_.data.array.len);
        memcpy(result.data(), base_.data.array.data, base_.data.array.len << 3);
        return result;
      }

      throw std::runtime_error("Can not convert parameter to uint64_t array");
    } else if constexpr (std::is_same_v<T, std::span<const int8_t>>) {
      if ((base_.type == aimrt_parameter_type_t::AIMRT_PARAMETER_TYPE_BOOL_ARRAY ||
           base_.type == aimrt_parameter_type_t::AIMRT_PARAMETER_TYPE_BYTE_ARRAY ||
           base_.type == aimrt_parameter_type_t::AIMRT_PARAMETER_TYPE_INTEGER_ARRAY) &&
          base_.data.array.type_size == 1) {
        return std::span<const int8_t>(
            static_cast<const int8_t*>(base_.data.array.data), base_.data.array.len);
      }

      throw std::runtime_error("Can not convert parameter to int8_t array");
    } else if constexpr (std::is_same_v<T, std::vector<int8_t>>) {
      if ((base_.type == aimrt_parameter_type_t::AIMRT_PARAMETER_TYPE_BOOL_ARRAY ||
           base_.type == aimrt_parameter_type_t::AIMRT_PARAMETER_TYPE_BYTE_ARRAY ||
           base_.type == aimrt_parameter_type_t::AIMRT_PARAMETER_TYPE_INTEGER_ARRAY) &&
          base_.data.array.type_size == 1) {
        std::vector<int8_t> result(base_.data.array.len);
        memcpy(result.data(), base_.data.array.data, base_.data.array.len);
        return result;
      }

      throw std::runtime_error("Can not convert parameter to int8_t array");
    } else if constexpr (std::is_same_v<T, std::span<const int16_t>>) {
      if (base_.type == aimrt_parameter_type_t::AIMRT_PARAMETER_TYPE_INTEGER_ARRAY &&
          base_.data.array.type_size == 2) {
        return std::span<const int16_t>(
            static_cast<const int16_t*>(base_.data.array.data), base_.data.array.len);
      }

      throw std::runtime_error("Can not convert parameter to int16_t array");
    } else if constexpr (std::is_same_v<T, std::vector<int16_t>>) {
      if (base_.type == aimrt_parameter_type_t::AIMRT_PARAMETER_TYPE_INTEGER_ARRAY &&
          base_.data.array.type_size == 2) {
        std::vector<int16_t> result(base_.data.array.len);
        memcpy(result.data(), base_.data.array.data, base_.data.array.len << 1);
        return result;
      }

      throw std::runtime_error("Can not convert parameter to int16_t array");
    } else if constexpr (std::is_same_v<T, std::span<const int32_t>>) {
      if (base_.type == aimrt_parameter_type_t::AIMRT_PARAMETER_TYPE_INTEGER_ARRAY &&
          base_.data.array.type_size == 4) {
        return std::span<const int32_t>(
            static_cast<const int32_t*>(base_.data.array.data), base_.data.array.len);
      }

      throw std::runtime_error("Can not convert parameter to int32_t array");
    } else if constexpr (std::is_same_v<T, std::vector<int32_t>>) {
      if (base_.type == aimrt_parameter_type_t::AIMRT_PARAMETER_TYPE_INTEGER_ARRAY &&
          base_.data.array.type_size == 4) {
        std::vector<int32_t> result(base_.data.array.len);
        memcpy(result.data(), base_.data.array.data, base_.data.array.len << 2);
        return result;
      }

      throw std::runtime_error("Can not convert parameter to int32_t array");
    } else if constexpr (std::is_same_v<T, std::span<const int64_t>>) {
      if (base_.type == aimrt_parameter_type_t::AIMRT_PARAMETER_TYPE_INTEGER_ARRAY &&
          base_.data.array.type_size == 8) {
        return std::span<const int64_t>(
            static_cast<const int64_t*>(base_.data.array.data), base_.data.array.len);
      }

      throw std::runtime_error("Can not convert parameter to int64_t array");
    } else if constexpr (std::is_same_v<T, std::vector<int64_t>>) {
      if (base_.type == aimrt_parameter_type_t::AIMRT_PARAMETER_TYPE_INTEGER_ARRAY &&
          base_.data.array.type_size == 8) {
        std::vector<int64_t> result(base_.data.array.len);
        memcpy(result.data(), base_.data.array.data, base_.data.array.len << 3);
        return result;
      }

      throw std::runtime_error("Can not convert parameter to int64_t array");
    } else if constexpr (std::is_same_v<T, std::span<const float>>) {
      if (base_.type == aimrt_parameter_type_t::AIMRT_PARAMETER_TYPE_DOUBLE_ARRAY &&
          base_.data.array.type_size == 4) {
        return std::span<const float>(
            static_cast<const float*>(base_.data.array.data), base_.data.array.len);
      }

      throw std::runtime_error("Can not convert parameter to float array");
    } else if constexpr (std::is_same_v<T, std::vector<float>>) {
      if (base_.type == aimrt_parameter_type_t::AIMRT_PARAMETER_TYPE_DOUBLE_ARRAY &&
          base_.data.array.type_size == 4) {
        std::vector<float> result(base_.data.array.len);
        memcpy(result.data(), base_.data.array.data, base_.data.array.len << 2);
        return result;
      }

      throw std::runtime_error("Can not convert parameter to float array");
    } else if constexpr (std::is_same_v<T, std::span<const double>>) {
      if (base_.type == aimrt_parameter_type_t::AIMRT_PARAMETER_TYPE_DOUBLE_ARRAY &&
          base_.data.array.type_size == 8) {
        return std::span<const double>(
            static_cast<const double*>(base_.data.array.data), base_.data.array.len);
      }

      throw std::runtime_error("Can not convert parameter to double array");
    } else if constexpr (std::is_same_v<T, std::vector<double>>) {
      if (base_.type == aimrt_parameter_type_t::AIMRT_PARAMETER_TYPE_DOUBLE_ARRAY &&
          base_.data.array.type_size == 8) {
        std::vector<double> result(base_.data.array.len);
        memcpy(result.data(), base_.data.array.data, base_.data.array.len << 3);
        return result;
      }

      throw std::runtime_error("Can not convert parameter to double array");
    } else if constexpr (std::is_same_v<T, std::span<const aimrt_string_view_t>>) {
      if (base_.type == aimrt_parameter_type_t::AIMRT_PARAMETER_TYPE_STRING_ARRAY &&
          base_.data.array.type_size == sizeof(aimrt_string_view_t)) {
        return std::span<const aimrt_string_view_t>(
            static_cast<const aimrt_string_view_t*>(base_.data.array.data), base_.data.array.len);
      }

      throw std::runtime_error("Can not convert parameter to string array");
    } else if constexpr (std::is_same_v<T, std::vector<aimrt_string_view_t>>) {
      if (base_.type == aimrt_parameter_type_t::AIMRT_PARAMETER_TYPE_STRING_ARRAY &&
          base_.data.array.type_size == sizeof(aimrt_string_view_t)) {
        std::vector<aimrt_string_view_t> result(base_.data.array.len);
        memcpy(result.data(), base_.data.array.data, base_.data.array.len * sizeof(aimrt_string_view_t));
        return result;
      }

      throw std::runtime_error("Can not convert parameter to string array");
    } else if constexpr (std::is_same_v<T, std::vector<std::string_view>>) {
      if (base_.type == aimrt_parameter_type_t::AIMRT_PARAMETER_TYPE_STRING_ARRAY &&
          base_.data.array.type_size == sizeof(aimrt_string_view_t)) {
        std::vector<std::string_view> result(base_.data.array.len);
        for (size_t ii = 0; ii < base_.data.array.len; ++ii) {
          result[ii] = util::ToStdStringView(
              static_cast<const aimrt_string_view_t*>(base_.data.array.data)[ii]);
        }

        return result;
      }

      throw std::runtime_error("Can not convert parameter to string array");
    } else if constexpr (std::is_same_v<T, std::vector<std::string>>) {
      if (base_.type == aimrt_parameter_type_t::AIMRT_PARAMETER_TYPE_STRING_ARRAY &&
          base_.data.array.type_size == sizeof(aimrt_string_view_t)) {
        std::vector<std::string> result(base_.data.array.len);
        for (size_t ii = 0; ii < base_.data.array.len; ++ii) {
          result[ii] = util::ToStdString(
              static_cast<const aimrt_string_view_t*>(base_.data.array.data)[ii]);
        }

        return result;
      }

      throw std::runtime_error("Can not convert parameter to string array");
    } else {
      static_assert(sizeof(T) == 0, "Can not convert parameter to target type");
    }
  }

  aimrt_parameter_type_t Type() const {
    return base_.type;
  }

  aimrt_parameter_view_t NativeHandle() const {
    return base_;
  }

 private:
  aimrt_parameter_view_t base_;
  std::vector<uint8_t> convert_buf_;
};

class ParameterViewHolder {
 public:
  explicit ParameterViewHolder(const aimrt_parameter_view_holder_t& base)
      : parameter_view_(base.parameter_view),
        release_callback_(base.release_callback) {}
  ~ParameterViewHolder() {
    if (release_callback_) release_callback_();
  }

  ParameterViewHolder(const ParameterViewHolder&) = delete;
  ParameterViewHolder& operator=(const ParameterViewHolder&) = delete;

  template <typename T>
  T As() const {
    return parameter_view_.As<T>();
  }

  const ParameterView& Get() const {
    return parameter_view_;
  }

 private:
  const ParameterView parameter_view_;
  aimrt::util::Function<aimrt_function_parameter_ref_release_callback_ops_t> release_callback_;
};

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

  ParameterViewHolder GetParameter(std::string_view key) {
    assert(base_ptr_);

    return ParameterViewHolder(
        base_ptr_->get_parameter(base_ptr_->impl, util::ToAimRTStringView(key)));
  }

  bool SetParameter(std::string_view key, const ParameterView& val) {
    assert(base_ptr_);

    return base_ptr_->set_parameter(
        base_ptr_->impl, util::ToAimRTStringView(key), val.NativeHandle());
  }

  bool ClearParameter(std::string_view key) {
    return SetParameter(key, ParameterView());
  }

 private:
  const aimrt_parameter_handle_base_t* base_ptr_ = nullptr;
};

}  // namespace aimrt::parameter
