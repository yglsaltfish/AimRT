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

  virtual std::string_view Type() const = 0;  // It should always return the same value

  virtual void Initialize(YAML::Node options_node) = 0;
  virtual void Start() = 0;
  virtual void Shutdown() = 0;

  virtual bool AllowDuplicates() const = 0;  // It should always return the same value

  /**
   * @brief Do the log
   * @note
   * 1. This method will only be called after 'Initialize' and before 'Shutdown'.
   * 2. Backend can define the actual behavior.
   * 3. 'format_log_str_ptr' is used to reuse the format result of the previous backend, It may be empty.
   *
   * @param log_data_wrapper Log data
   * @param format_log_str_ptr Reuse the format result of the previous backend
   */
  virtual void Log(const LogDataWrapper& log_data_wrapper,
                   const std::shared_ptr<std::string>& format_log_str_ptr) = 0;
};

}  // namespace aimrt::runtime::core::logger
