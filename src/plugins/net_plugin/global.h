#pragma once

#include "aimrt_module_cpp_interface/logger/logger.h"
#include "aimrt_module_cpp_interface/util/buffer.h"

namespace aimrt::plugins::net_plugin {

void SetLogger(LoggerRef);
LoggerRef GetLogger();

BufferArrayAllocatorRef GetDefaultBufferArrayAllocator();

}  // namespace aimrt::plugins::net_plugin
