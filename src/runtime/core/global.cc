#include "core/global.h"

#include <vector>

#include "core/util/buffer_array_allocator.h"
#include "core/util/simple_logger.h"

namespace aimrt::runtime::core {

// Logger
aimrt::logger::LoggerRef global_logger;

void SetLogger(aimrt::logger::LoggerRef logger) { global_logger = logger; }
aimrt::logger::LoggerRef GetLogger() {
  if (global_logger) return global_logger;

  static util::SimpleLogger simple_logger;
  global_logger = aimrt::logger::LoggerRef(simple_logger.NativeHandle());

  return global_logger;
}

// BufferArrayAllocatorRef
aimrt::util::BufferArrayAllocatorRef GetDefaultBufferArrayAllocator() {
  return aimrt::util::BufferArrayAllocatorRef(util::BufferArrayAllocator::NativeHandle());
}

// SignalHandle
std::vector<std::pair<std::set<int>, std::function<void(int)>>> global_signal_handle_vec;

void SignalHandler(int sig) {
  for (auto& itr : global_signal_handle_vec) {
    if (itr.first.find(sig) != itr.first.end()) itr.second(sig);
  }
}

void RegisterSignalHandle(
    const std::set<int>& signals, const std::function<void(int)>& signal_handle) {
  global_signal_handle_vec.emplace_back(signals, signal_handle);

  for (auto sig : signals) signal(sig, SignalHandler);
}

}  // namespace aimrt::runtime::core
