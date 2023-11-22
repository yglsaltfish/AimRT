#pragma once

#ifdef AIMRT_USE_EXECUTOR

  #include <unifex/async_scope.hpp>

namespace aimrt::co {

using AsyncScope = unifex::async_scope;

}

#endif
