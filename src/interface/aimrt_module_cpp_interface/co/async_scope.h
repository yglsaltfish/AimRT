#pragma once

#ifdef AIMRT_USE_EXECUTOR

  #include <unifex/async_scope.hpp>

namespace aimrt {
namespace co {

using AsyncScope = unifex::async_scope;

}  // namespace co
}  // namespace aimrt

#endif
