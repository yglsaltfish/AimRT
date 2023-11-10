#pragma once

#ifdef AIMRT_USE_EXECUTOR

  #include <unifex/when_all.hpp>

namespace aimrt {
namespace co {

inline constexpr auto& WhenAll = unifex::when_all;

}  // namespace co
}  // namespace aimrt

#endif
