#pragma once

#ifdef AIMRT_USE_EXECUTOR

  #include <unifex/invoke.hpp>

namespace aimrt {
namespace co {

inline constexpr auto& CoInvoke = unifex::co_invoke;

}  // namespace co
}  // namespace aimrt

#endif
