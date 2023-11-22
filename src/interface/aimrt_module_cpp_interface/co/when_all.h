#pragma once

#ifdef AIMRT_USE_EXECUTOR

  #include <unifex/when_all.hpp>

namespace aimrt::co {

inline constexpr auto& WhenAll = unifex::when_all;

}

#endif
