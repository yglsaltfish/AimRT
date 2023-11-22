#pragma once

#ifdef AIMRT_USE_EXECUTOR

  #include <unifex/async_mutex.hpp>

namespace aimrt::co {

using AsyncMutex = unifex::async_mutex;

}

#endif
