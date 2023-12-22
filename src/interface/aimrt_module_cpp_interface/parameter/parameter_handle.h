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

  ParameterView(const void* buf, size_t len) {
    base_.type = aimrt_parameter_type_t::AIMRT_PARAMETER_TYPE_BYTE_ARRAY;
    base_.data.array.data = buf;
    base_.data.array.type_size = 1;
    base_.data.array.len = len;
  }

  ParameterView(std::span<const bool> val) {
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
  ParameterView(std::span<const T> val) {
    if constexpr (sizeof(T) == 1) {
      base_.type = aimrt_parameter_type_t::AIMRT_PARAMETER_TYPE_BYTE_ARRAY;
      base_.data.array.data = val.data();
      base_.data.array.type_size = 1;
      base_.data.array.len = val.size();
    } else if constexpr (sizeof(T) == sizeof(int64_t)) {
      base_.type = aimrt_parameter_type_t::AIMRT_PARAMETER_TYPE_INTEGER_ARRAY;
      base_.data.array.data = val.data();
      base_.data.array.type_size = sizeof(int64_t);
      base_.data.array.len = val.size();
    } else {
      const size_t len = val.size();
      convert_buf_.resize(len * sizeof(int64_t));
      int64_t* int64_data = reinterpret_cast<int64_t*>(convert_buf_.data());
      for (size_t ii = 0; ii < len; ++ii) {
        int64_data[ii] = val[ii];
      }

      base_.type = aimrt_parameter_type_t::AIMRT_PARAMETER_TYPE_INTEGER_ARRAY;
      base_.data.array.data = int64_data;
      base_.data.array.type_size = sizeof(int64_t);
      base_.data.array.len = len;
    }
  }

  template <std::signed_integral T>
  ParameterView(const std::vector<T>& val)
      : ParameterView(std::span<const T>(val.data(), val.size())) {}

  template <std::unsigned_integral T>
  ParameterView(std::span<const T> val) {
    if constexpr (sizeof(T) == 1) {
      base_.type = aimrt_parameter_type_t::AIMRT_PARAMETER_TYPE_BYTE_ARRAY;
      base_.data.array.data = val.data();
      base_.data.array.type_size = 1;
      base_.data.array.len = val.size();
    } else if constexpr (sizeof(T) == sizeof(uint64_t)) {
      base_.type = aimrt_parameter_type_t::AIMRT_PARAMETER_TYPE_UNSIGNED_INTEGER_ARRAY;
      base_.data.array.data = val.data();
      base_.data.array.type_size = sizeof(uint64_t);
      base_.data.array.len = val.size();
    } else {
      const size_t len = val.size();
      convert_buf_.resize(len * sizeof(uint64_t));
      uint64_t* uint64_data = reinterpret_cast<uint64_t*>(convert_buf_.data());
      for (size_t ii = 0; ii < len; ++ii) {
        uint64_data[ii] = val[ii];
      }

      base_.type = aimrt_parameter_type_t::AIMRT_PARAMETER_TYPE_UNSIGNED_INTEGER_ARRAY;
      base_.data.array.data = uint64_data;
      base_.data.array.type_size = sizeof(uint64_t);
      base_.data.array.len = len;
    }
  }

  template <std::unsigned_integral T>
  ParameterView(const std::vector<T>& val)
      : ParameterView(std::span<const T>(val.data(), val.size())) {}

  template <std::floating_point T>
  ParameterView(std::span<const T> val) {
    base_.type = aimrt_parameter_type_t::AIMRT_PARAMETER_TYPE_DOUBLE_ARRAY;

    if constexpr (sizeof(T) == sizeof(double)) {
      base_.data.array.data = val.data();
      base_.data.array.type_size = sizeof(double);
      base_.data.array.len = val.size();
    } else {
      const size_t len = val.size();
      convert_buf_.resize(len * sizeof(double));
      double* double_data = reinterpret_cast<double*>(convert_buf_.data());
      for (size_t ii = 0; ii < len; ++ii) {
        double_data[ii] = val[ii];
      }

      base_.data.array.data = double_data;
      base_.data.array.type_size = sizeof(double);
      base_.data.array.len = len;
    }
  }

  template <std::floating_point T>
  ParameterView(const std::vector<T>& val)
      : ParameterView(std::span<const T>(val.data(), val.size())) {}

  ParameterView(std::span<const aimrt_string_view_t> val) {
    base_.type = aimrt_parameter_type_t::AIMRT_PARAMETER_TYPE_STRING_ARRAY;
    base_.data.array.data = val.data();
    base_.data.array.type_size = sizeof(aimrt_string_view_t);
    base_.data.array.len = val.size();
  }

  template <typename T>
    requires std::is_same_v<std::string_view, T> || std::is_same_v<std::string, T>
  ParameterView(std::span<const T> val) {
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

  ParameterView(const std::vector<aimrt_string_view_t>& val)
      : ParameterView(std::span<const aimrt_string_view_t>(val.data(), val.size())) {}

  template <typename T>
    requires std::is_same_v<std::string_view, T> || std::is_same_v<std::string, T>
  ParameterView(const std::vector<T>& val)
      : ParameterView(std::span<const T>(val.data(), val.size())) {}

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
                         std::is_same_v<T, std::string>) {
      if (base_.type == aimrt_parameter_type_t::AIMRT_PARAMETER_TYPE_STRING &&
          base_.data.array.type_size == 1)
        return T(static_cast<const char*>(base_.data.array.data), base_.data.array.len);

      throw std::runtime_error("Can not convert parameter to string");
    } else if constexpr (std::is_same_v<T, std::pair<const void*, size_t>>) {
      if (base_.type == aimrt_parameter_type_t::AIMRT_PARAMETER_TYPE_BYTE_ARRAY &&
          base_.data.array.type_size == 1)
        return T(base_.data.array.data, base_.data.array.len);

      throw std::runtime_error("Can not convert parameter to byte array");
    } else if constexpr (std::is_same_v<T, std::span<const char>> ||
                         std::is_same_v<T, std::span<const int8_t>> ||
                         std::is_same_v<T, std::span<const uint8_t>>) {
      if (base_.type == aimrt_parameter_type_t::AIMRT_PARAMETER_TYPE_BYTE_ARRAY &&
          base_.data.array.type_size == 1)
        return T(static_cast<T::element_type*>(base_.data.array.data), base_.data.array.len);

      throw std::runtime_error("Can not convert parameter to byte array");
    } else if constexpr (std::is_same_v<T, std::vector<char>> ||
                         std::is_same_v<T, std::vector<int8_t>> ||
                         std::is_same_v<T, std::vector<uint8_t>>) {
      if (base_.type == aimrt_parameter_type_t::AIMRT_PARAMETER_TYPE_BYTE_ARRAY &&
          base_.data.array.type_size == 1) {
        T result(base_.data.array.len);
        memcpy(result.data(), base_.data.array.data, base_.data.array.len);
        return result;
      }

      throw std::runtime_error("Can not convert parameter to byte array");
    } else if constexpr (std::is_same_v<T, std::span<const bool>>) {
      if (base_.type == aimrt_parameter_type_t::AIMRT_PARAMETER_TYPE_BOOL_ARRAY &&
          base_.data.array.type_size == 1) {
        return std::span<const bool>(
            static_cast<const bool*>(base_.data.array.data), base_.data.array.len);
      }

      throw std::runtime_error("Can not convert parameter to bool array");
    } else if constexpr (std::is_same_v<T, std::vector<bool>>) {
      if (base_.type == aimrt_parameter_type_t::AIMRT_PARAMETER_TYPE_BOOL_ARRAY &&
          base_.data.array.type_size == 1) {
        std::vector<bool> result(base_.data.array.len);
        for (size_t ii = 0; ii < base_.data.array.len; ++ii) {
          result[ii] = static_cast<const bool*>(base_.data.array.data)[ii];
        }

        return result;
      }

      throw std::runtime_error("Can not convert parameter to bool array");
    } else if constexpr (std::is_same_v<T, std::span<const int64_t>>) {
      if (base_.type == aimrt_parameter_type_t::AIMRT_PARAMETER_TYPE_INTEGER_ARRAY &&
          base_.data.array.type_size == sizeof(int64_t)) {
        return std::span<const int64_t>(
            static_cast<const int64_t*>(base_.data.array.data), base_.data.array.len);
      }

      throw std::runtime_error("Can not convert parameter to int64_t span");
    } else if constexpr (std::is_same_v<T, std::vector<int16_t>> ||
                         std::is_same_v<T, std::vector<int32_t>> ||
                         std::is_same_v<T, std::vector<int64_t>>) {
      if (base_.type == aimrt_parameter_type_t::AIMRT_PARAMETER_TYPE_INTEGER_ARRAY &&
          base_.data.array.type_size == sizeof(int64_t)) {
        T result(base_.data.array.len);
        const int64_t* int64_data = reinterpret_cast<const int64_t*>(base_.data.array.data);
        for (size_t ii = 0; ii < base_.data.array.len; ++ii) {
          result[ii] = static_cast<const T::value_type>(int64_data[ii]);
        }

        return result;
      }

      throw std::runtime_error("Can not convert parameter to int vector");
    } else if constexpr (std::is_same_v<T, std::span<const uint64_t>>) {
      if (base_.type == aimrt_parameter_type_t::AIMRT_PARAMETER_TYPE_UNSIGNED_INTEGER_ARRAY &&
          base_.data.array.type_size == sizeof(uint64_t)) {
        return std::span<const uint64_t>(
            static_cast<const uint64_t*>(base_.data.array.data), base_.data.array.len);
      }

      throw std::runtime_error("Can not convert parameter to uint64_t span");
    } else if constexpr (std::is_same_v<T, std::vector<uint16_t>> ||
                         std::is_same_v<T, std::vector<uint32_t>> ||
                         std::is_same_v<T, std::vector<uint64_t>>) {
      if (base_.type == aimrt_parameter_type_t::AIMRT_PARAMETER_TYPE_UNSIGNED_INTEGER_ARRAY &&
          base_.data.array.type_size == sizeof(uint64_t)) {
        T result(base_.data.array.len);
        const uint64_t* uint64_data = reinterpret_cast<const uint64_t*>(base_.data.array.data);
        for (size_t ii = 0; ii < base_.data.array.len; ++ii) {
          result[ii] = static_cast<const T::value_type>(uint64_data[ii]);
        }

        return result;
      }

      throw std::runtime_error("Can not convert parameter to uint vector");
    } else if constexpr (std::is_same_v<T, std::span<const double>>) {
      if (base_.type == aimrt_parameter_type_t::AIMRT_PARAMETER_TYPE_DOUBLE_ARRAY &&
          base_.data.array.type_size == sizeof(double)) {
        return std::span<const double>(
            static_cast<const double*>(base_.data.array.data), base_.data.array.len);
      }

      throw std::runtime_error("Can not convert parameter to double span");
    } else if constexpr (std::is_same_v<T, std::vector<float>> ||
                         std::is_same_v<T, std::vector<double>>) {
      if (base_.type == aimrt_parameter_type_t::AIMRT_PARAMETER_TYPE_DOUBLE_ARRAY &&
          base_.data.array.type_size == sizeof(double)) {
        T result(base_.data.array.len);
        const double* double_data = reinterpret_cast<const double*>(base_.data.array.data);
        for (size_t ii = 0; ii < base_.data.array.len; ++ii) {
          result[ii] = static_cast<const T::value_type>(double_data[ii]);
        }

        return result;
      }

      throw std::runtime_error("Can not convert parameter to float point vector");
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
