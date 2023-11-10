#include "net_plugin/global.h"
#include "core/util/buffer_array_allocator.h"
#include "core/util/simple_logger.h"

namespace aimrt::plugins::net_plugin {

LoggerRef global_logger;

void SetLogger(LoggerRef logger) { global_logger = logger; }
LoggerRef GetLogger() {
  if (global_logger) return global_logger;

  static runtime::core::util::SimpleLogger simple_logger;
  global_logger = LoggerRef(simple_logger.NativeHandle());

  return global_logger;
}

BufferArrayAllocatorRef GetDefaultBufferArrayAllocator() {
  return BufferArrayAllocatorRef(runtime::core::util::BufferArrayAllocator::NativeHandle());
}

}  // namespace aimrt::plugins::net_plugin
