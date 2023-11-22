#pragma once

#ifdef AIMRT_USE_EXECUTOR

  #include <unifex/invoke.hpp>

namespace aimrt::co {

inline constexpr auto& CoInvoke = unifex::co_invoke;

}

#endif
