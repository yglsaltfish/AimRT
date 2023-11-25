#pragma once

#include <csignal>
#include <functional>
#include <set>

#include "aimrt_module_cpp_interface/logger/logger.h"
#include "aimrt_module_cpp_interface/util/buffer.h"

namespace aimrt::runtime::core {

void SetLogger(aimrt::logger::LoggerRef);
aimrt::logger::LoggerRef GetLogger();

aimrt::util::BufferArrayAllocatorRef GetDefaultBufferArrayAllocator();

void RegisterSignalHandle(
    const std::set<int>& signals, const std::function<void(int)>& signal_handle);

}  // namespace aimrt::runtime::core
