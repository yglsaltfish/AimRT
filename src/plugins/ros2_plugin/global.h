#pragma once

#include "aimrt_module_cpp_interface/logger/logger.h"
#include "aimrt_module_cpp_interface/util/buffer.h"

namespace aimrt::plugins::ros2_plugin {

void SetLogger(LoggerRef);
LoggerRef GetLogger();

}  // namespace aimrt::plugins::ros2_plugin
