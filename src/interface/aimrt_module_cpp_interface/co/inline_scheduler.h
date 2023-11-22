#pragma once

#ifdef AIMRT_USE_EXECUTOR

  #include <unifex/inline_scheduler.hpp>

namespace aimrt::co {

using InlineScheduler = unifex::inline_scheduler;

}

#endif
