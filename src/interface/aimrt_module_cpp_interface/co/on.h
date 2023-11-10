#pragma once

#ifdef AIMRT_USE_EXECUTOR

  #include <unifex/on.hpp>

namespace aimrt {
namespace co {

inline constexpr auto& On = unifex::on;

}  // namespace co
}  // namespace aimrt

#endif
