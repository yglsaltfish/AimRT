#pragma once

#ifdef AIMRT_USE_EXECUTOR

  #include <unifex/sync_wait.hpp>

namespace aimrt {
namespace co {

inline constexpr auto& SyncWait = unifex::sync_wait;

}  // namespace co
}  // namespace aimrt

#endif
