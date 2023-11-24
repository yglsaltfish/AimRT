#pragma once

#ifdef AIMRT_USE_EXECUTOR

  #include <exec/inline_scheduler.hpp>

namespace aimrt::co {

using InlineScheduler = exec::inline_scheduler;

}

#endif
