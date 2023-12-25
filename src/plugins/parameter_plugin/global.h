#pragma once

#include "aimrt_module_cpp_interface/logger/logger.h"

namespace aimrt::plugins::parameter_plugin {

void SetLogger(aimrt::logger::LoggerRef);
aimrt::logger::LoggerRef GetLogger();

}  // namespace aimrt::plugins::parameter_plugin
