#pragma once

#ifdef AIMRT_EXECUTOR_USE_STDEXEC

  #include <stdexec/execution.hpp>

namespace aimrt::co {

inline constexpr auto& On = stdexec::on;

}

#else

  #include <unifex/on.hpp>

namespace aimrt::co {

inline constexpr auto& On = unifex::on;

}  // namespace aimrt::co

#endif
