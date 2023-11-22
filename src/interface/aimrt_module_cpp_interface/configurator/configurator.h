#pragma once

#include <cassert>
#include <string_view>

#include "aimrt_module_c_interface/configurator/configurator_base.h"
#include "aimrt_module_cpp_interface/util/string.h"

namespace aimrt::configurator {

class ConfiguratorRef {
 public:
  ConfiguratorRef() = default;
  explicit ConfiguratorRef(const aimrt_configurator_base_t* base_ptr)
      : base_ptr_(base_ptr) {}
  ~ConfiguratorRef() = default;

  explicit operator bool() const { return (base_ptr_ != nullptr); }

  const aimrt_configurator_base_t* NativeHandle() const { return base_ptr_; }

  /**
   * @brief Get the config file path
   *
   * @return Config file path
   */
  std::string_view GetConfigFilePath() const {
    assert(base_ptr_);
    return aimrt::util::ToStdStringView(base_ptr_->config_file_path(base_ptr_->impl));
  }

 private:
  const aimrt_configurator_base_t* base_ptr_ = nullptr;
};

}  // namespace aimrt::configurator