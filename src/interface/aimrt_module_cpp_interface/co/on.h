#pragma once

#ifdef AIMRT_USE_EXECUTOR

  #include <unifex/on.hpp>

namespace aimrt::co {

inline constexpr auto& On = unifex::on;

}

#endif
