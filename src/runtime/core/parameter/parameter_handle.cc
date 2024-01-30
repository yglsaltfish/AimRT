#include "core/parameter/parameter_handle.h"

namespace aimrt::runtime::core::parameter {

std::shared_ptr<const std::string> ParameterHandle::GetParameter(std::string_view key) {
  ParameterMap::const_accessor ac;
  bool find_ret = parameter_map_.find(ac, key);

  if (find_ret) {
    AIMRT_TRACE("Get parameter '{}'", key);
    std::lock_guard<std::mutex> lck(ac->second.mu);
    return ac->second.ptr;
  }

  AIMRT_TRACE("Can not get parameter '{}'", key);
  return std::shared_ptr<std::string>();
}

void ParameterHandle::SetParameter(
    std::string_view key, const std::shared_ptr<std::string>& value_ptr) {
  if (!value_ptr || value_ptr->empty()) [[unlikely]] {
    AIMRT_TRACE("Erase parameter '{}'", key);
    parameter_map_.erase(key);
    return;
  }

  ParameterMap::accessor ac;
  bool emplace_ret = parameter_map_.emplace(ac, key, value_ptr);

  if (emplace_ret) {
    AIMRT_TRACE("Set parameter '{}'", key);
    return;
  }

  AIMRT_TRACE("Update parameter '{}'", key);
  std::lock_guard<std::mutex> lck(ac->second.mu);
  ac->second.ptr = value_ptr;
}

std::vector<std::string> ParameterHandle::ListParameter() const {
  std::vector<std::string> result;
  result.reserve(parameter_map_.size());

  for (auto itr = parameter_map_.begin(); itr != parameter_map_.end(); ++itr) {
    result.emplace_back(itr->first);
  }

  return result;
}

}  // namespace aimrt::runtime::core::parameter
