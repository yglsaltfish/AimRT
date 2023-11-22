#pragma once

#ifdef AIMRT_USE_EXECUTOR

  #include <unifex/async_manual_reset_event.hpp>

namespace aimrt::co {

using AsyncEvent = unifex::async_manual_reset_event;

}

#endif
