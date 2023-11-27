#pragma once

#include <csignal>
#include <functional>
#include <set>

#include "aimrt_module_cpp_interface/logger/logger.h"
#include "aimrt_module_cpp_interface/util/buffer.h"

namespace aimrt::runtime::core {

aimrt::logger::LoggerRef GetLogger();

aimrt::util::BufferArrayAllocatorRef GetDefaultBufferArrayAllocator();

}  // namespace aimrt::runtime::core
