#include "core/parameter/parameter_handle.h"
#include "core/global.h"

namespace aimrt::runtime::core::parameter {

bool Parameter::SetData(aimrt_parameter_view_t input_view) {
  view_.type = input_view.type;

  if (view_.type == aimrt_parameter_type_t::AIMRT_PARAMETER_TYPE_NULL ||
      view_.type == aimrt_parameter_type_t::AIMRT_PARAMETER_TYPE_BOOL ||
      view_.type == aimrt_parameter_type_t::AIMRT_PARAMETER_TYPE_INTEGER ||
      view_.type == aimrt_parameter_type_t::AIMRT_PARAMETER_TYPE_UNSIGNED_INTEGER ||
      view_.type == aimrt_parameter_type_t::AIMRT_PARAMETER_TYPE_DOUBLE) {
    view_.data = input_view.data;
    return true;
  }

  if (view_.type == aimrt_parameter_type_t::AIMRT_PARAMETER_TYPE_STRING ||
      view_.type == aimrt_parameter_type_t::AIMRT_PARAMETER_TYPE_BYTE_ARRAY ||
      view_.type == aimrt_parameter_type_t::AIMRT_PARAMETER_TYPE_BOOL_ARRAY) {
    if (input_view.data.array.type_size != 1) [[unlikely]] {
      AIMRT_ERROR("Invalid parameter view type: {}, type_size: {}",
                  static_cast<size_t>(view_.type), input_view.data.array.type_size);
      return false;
    }

    view_.data.array.len = input_view.data.array.len;
    view_.data.array.type_size = 1;

    array_data_ = std::vector<uint8_t>(view_.data.array.len);
    view_.data.array.data = array_data_.data();

    memcpy(array_data_.data(), input_view.data.array.data, view_.data.array.len);
    return true;
  }

  if (view_.type == aimrt_parameter_type_t::AIMRT_PARAMETER_TYPE_INTEGER_ARRAY ||
      view_.type == aimrt_parameter_type_t::AIMRT_PARAMETER_TYPE_UNSIGNED_INTEGER_ARRAY ||
      view_.type == aimrt_parameter_type_t::AIMRT_PARAMETER_TYPE_DOUBLE_ARRAY) {
    if (input_view.data.array.type_size != 1 &&
        input_view.data.array.type_size != 2 &&
        input_view.data.array.type_size != 4 &&
        input_view.data.array.type_size != 8) [[unlikely]] {
      AIMRT_ERROR("Invalid parameter view type: {}, type_size: {}",
                  static_cast<size_t>(view_.type), input_view.data.array.type_size);
      return false;
    }

    view_.data.array.len = input_view.data.array.len;
    view_.data.array.type_size = input_view.data.array.type_size;

    array_data_ = std::vector<uint8_t>(view_.data.array.len * view_.data.array.type_size);
    view_.data.array.data = array_data_.data();

    memcpy(array_data_.data(), input_view.data.array.data, array_data_.size());
    return true;
  }

  if (view_.type == aimrt_parameter_type_t::AIMRT_PARAMETER_TYPE_STRING_ARRAY) {
    if (input_view.data.array.type_size != sizeof(aimrt_string_view_t)) [[unlikely]] {
      AIMRT_ERROR("Invalid parameter view type: {}, type_size: {}",
                  static_cast<size_t>(view_.type), input_view.data.array.type_size);
      return false;
    }

    view_.data.array.len = input_view.data.array.len;
    view_.data.array.type_size = input_view.data.array.type_size;

    const size_t array_data_len = view_.data.array.len * sizeof(aimrt_string_view_t);
    size_t buf_len = array_data_len;
    const auto* input_string_view_array = static_cast<const aimrt_string_view_t*>(input_view.data.array.data);
    for (size_t ii = 0; ii < view_.data.array.len; ++ii) {
      buf_len += input_string_view_array[ii].len;
    }

    array_data_ = std::vector<uint8_t>(buf_len);
    view_.data.array.data = array_data_.data();

    auto* string_view_array = reinterpret_cast<aimrt_string_view_t*>(array_data_.data());
    char* cur_pos = reinterpret_cast<char*>(array_data_.data()) + array_data_len;
    for (size_t ii = 0; ii < view_.data.array.len; ++ii) {
      string_view_array[ii].str = cur_pos;
      string_view_array[ii].len = input_string_view_array[ii].len;
      memcpy(cur_pos, input_string_view_array[ii].str, string_view_array[ii].len);
      cur_pos += string_view_array[ii].len;
    }

    return true;
  }

  AIMRT_ERROR("Invalid parameter view type: {}", static_cast<size_t>(view_.type));
  return false;
}

std::shared_ptr<Parameter> ParameterHandle::GetParameter(std::string_view key) {
  ParameterMap::const_accessor ac;
  bool find_ret = parameter_map_.find(ac, key);

  if (find_ret) {
    std::lock_guard<std::mutex> lck(ac->second.mu);
    return ac->second.ptr;
  }

  static auto ptr = std::make_shared<Parameter>();
  return ptr;
}

void ParameterHandle::SetParameter(
    std::string_view key, const std::shared_ptr<Parameter>& parameter_ptr) {
  ParameterMap::accessor ac;
  bool emplace_ret = parameter_map_.emplace(ac, key, parameter_ptr);

  if (emplace_ret) return;

  std::lock_guard<std::mutex> lck(ac->second.mu);
  ac->second.ptr = parameter_ptr;
}

}  // namespace aimrt::runtime::core::parameter
