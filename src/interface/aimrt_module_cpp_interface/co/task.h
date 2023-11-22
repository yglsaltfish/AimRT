#pragma once

#ifdef AIMRT_USE_EXECUTOR

  #include <unifex/task.hpp>

namespace aimrt::co {

template <typename T>
using Task = typename unifex::task<T>;

}

#endif
