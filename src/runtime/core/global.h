#pragma once

#include "aimrt_module_cpp_interface/logger/logger.h"
#include "aimrt_module_cpp_interface/util/buffer.h"

namespace aimrt::runtime::core {

void SetLogger(LoggerRef);
LoggerRef GetLogger();

BufferArrayAllocatorRef GetDefaultBufferArrayAllocator();

}  // namespace aimrt::runtime::core
