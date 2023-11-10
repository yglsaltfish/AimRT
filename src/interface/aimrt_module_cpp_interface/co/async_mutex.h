#pragma once

#ifdef AIMRT_USE_EXECUTOR

  #include <unifex/async_mutex.hpp>

namespace aimrt {
namespace co {

using AsyncMutex = unifex::async_mutex;

}  // namespace co
}  // namespace aimrt

#endif
