#pragma once

#include "aimrt_module_c_interface/logger/logger_base.h"
#include "util/log_util.h"

namespace aimrt::runtime::core::util {

class SimpleLogger {
 public:
  SimpleLogger() : base_(GenBase(this)) {}
  ~SimpleLogger() = default;

  const aimrt_logger_base_t* NativeHandle() const { return &base_; }

 private:
  static aimrt_logger_base_t GenBase(void* impl) {
    return aimrt_logger_base_t{
        .get_log_level = [](void* impl) -> aimrt_log_level_t {
          return static_cast<aimrt_log_level_t>(common::util::SimpleLogger::GetLogLevel());
        },
        .log = [](void* impl,
                  aimrt_log_level_t lvl,
                  uint32_t line,
                  uint32_t column,
                  const char* file_name,
                  const char* function_name,
                  const char* log_data,
                  size_t log_data_size) {
          common::util::SimpleLogger::Log(
              static_cast<uint32_t>(lvl),
              line,
              column,
              file_name,
              function_name,
              log_data,
              log_data_size);  //
        },
        .impl = impl};
  }

 private:
  const aimrt_logger_base_t base_;
};

}  // namespace aimrt::runtime::core::util
