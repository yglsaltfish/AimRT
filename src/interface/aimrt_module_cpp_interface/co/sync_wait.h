#pragma once

#ifdef AIMRT_USE_EXECUTOR

  #include <unifex/sync_wait.hpp>

namespace aimrt::co {

inline constexpr auto& SyncWait = unifex::sync_wait;

}

#endif
