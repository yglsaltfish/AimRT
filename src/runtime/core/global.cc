#include "core/global.h"

#include <cassert>
#include <vector>

#include "core/aimrt_core.h"
#include "core/util/buffer_array_allocator.h"
#include "core/util/simple_logger.h"

namespace aimrt::runtime::core {

aimrt::logger::LoggerRef GetLogger() {
  AimRTCore* global_core_ptr = AimRTCore::GetGlobalAimRTCore();

  if (global_core_ptr) {
    auto state = global_core_ptr->GetLoggerManager().GetState();
    if (state == logger::LoggerManager::State::Init ||
        state == logger::LoggerManager::State::Start) {
      static aimrt::logger::LoggerRef core_logger_ref(
          global_core_ptr->GetLoggerManager().GetLoggerProxy("core").NativeHandle());

      return core_logger_ref;
    }
  }

  static util::SimpleLogger simple_logger;
  return aimrt::logger::LoggerRef(simple_logger.NativeHandle());
}

aimrt::util::BufferArrayAllocatorRef GetDefaultBufferArrayAllocator() {
  return aimrt::util::BufferArrayAllocatorRef(util::BufferArrayAllocator::NativeHandle());
}

}  // namespace aimrt::runtime::core
