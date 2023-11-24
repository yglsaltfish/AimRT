#pragma once

#ifdef AIMRT_USE_EXECUTOR

  #include <exec/task.hpp>

namespace aimrt::co {

template <typename T>
using Task = typename exec::task<T>;

}

#endif
