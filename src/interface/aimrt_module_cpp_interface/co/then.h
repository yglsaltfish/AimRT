#pragma once

#ifdef AIMRT_USE_EXECUTOR

  #include <stdexec/execution.hpp>

namespace aimrt::co {

inline constexpr auto& Then = stdexec::then;

}

#endif
