#pragma once

#include "core/logger/log_data_wrapper.h"

#include "yaml-cpp/yaml.h"

namespace aimrt::runtime::core::logger {

class LoggerBackendBase {
 public:
  LoggerBackendBase() = default;
  virtual ~LoggerBackendBase() = default;

  LoggerBackendBase(const LoggerBackendBase&) = delete;
  LoggerBackendBase& operator=(const LoggerBackendBase&) = delete;

  virtual std::string_view Name() const = 0;

  virtual void Initialize(YAML::Node options_node) = 0;
  virtual void Shutdown() = 0;

  // 可以复用前一个backend的format结果
  virtual void Log(const LogDataWrapper& log_data_wrapper,
                   const std::shared_ptr<std::string>& format_log_str_ptr) = 0;
};

}  // namespace aimrt::runtime::core::logger
