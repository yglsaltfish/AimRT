#include "time_manipulator_plugin/global.h"
#include "core/util/simple_buffer_array_allocator.h"
#include "core/util/simple_logger.h"

namespace aimrt::plugins::time_manipulator_plugin {

aimrt::logger::LoggerRef global_logger;

void SetLogger(aimrt::logger::LoggerRef logger) { global_logger = logger; }
aimrt::logger::LoggerRef GetLogger() {
  if (global_logger) return global_logger;

  static runtime::core::util::SimpleLogger simple_logger;
  global_logger = aimrt::logger::LoggerRef(simple_logger.NativeHandle());

  return global_logger;
}

}  // namespace aimrt::plugins::time_manipulator_plugin
