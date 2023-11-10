#pragma once

#ifdef AIMRT_USE_EXECUTOR

  #include <unifex/inline_scheduler.hpp>

namespace aimrt {
namespace co {

using InlineScheduler = unifex::inline_scheduler;

}  // namespace co
}  // namespace aimrt

#endif
