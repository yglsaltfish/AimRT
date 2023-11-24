#include "core/global.h"
#include "core/util/buffer_array_allocator.h"
#include "core/util/simple_logger.h"

namespace aimrt::runtime::core {

aimrt::logger::LoggerRef global_logger;

void SetLogger(aimrt::logger::LoggerRef logger) { global_logger = logger; }
aimrt::logger::LoggerRef GetLogger() {
  if (global_logger) return global_logger;

  static util::SimpleLogger simple_logger;
  global_logger = aimrt::logger::LoggerRef(simple_logger.NativeHandle());

  return global_logger;
}

aimrt::util::BufferArrayAllocatorRef GetDefaultBufferArrayAllocator() {
  return aimrt::util::BufferArrayAllocatorRef(util::BufferArrayAllocator::NativeHandle());
}

}  // namespace aimrt::runtime::core
