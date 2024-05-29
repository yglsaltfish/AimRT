#pragma once

#include <cstring>

#include "core/logger/logger_backend_base.h"

namespace aimrt::runtime::core::logger {

class LogLevelTool {
 public:
  static std::string_view GetLogLevelName(aimrt_log_level_t lvl) {
    return lvl_name_array_[static_cast<uint32_t>(lvl)];
  }

  static aimrt_log_level_t GetLogLevelFromName(std::string_view lvl_name) {
    for (int ii = 0; ii <= aimrt_log_level_t::AIMRT_LOG_LEVEL_OFF; ++ii) {
#if defined(_WIN32)
      if (_stricmp(lvl_name.data(), lvl_name_array_[ii].data()) == 0)
        return static_cast<aimrt_log_level_t>(ii);
#else
      if (strcasecmp(lvl_name.data(), lvl_name_array_[ii].data()) == 0)
        return static_cast<aimrt_log_level_t>(ii);
#endif
    }
    return aimrt_log_level_t::AIMRT_LOG_LEVEL_OFF;
  }

 private:
  static constexpr std::string_view
      lvl_name_array_[aimrt_log_level_t::AIMRT_LOG_LEVEL_OFF + 1] = {
          "Trace", "Debug", "Info", "Warn", "Error", "Fatal", "Off"};
};
}  // namespace aimrt::runtime::core::logger
