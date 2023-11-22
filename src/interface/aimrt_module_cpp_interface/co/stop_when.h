#pragma once

#ifdef AIMRT_USE_EXECUTOR

  #include <unifex/stop_when.hpp>

namespace aimrt::co {

inline constexpr auto& StopWhen = unifex::stop_when;

}

#endif
