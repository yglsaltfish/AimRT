#pragma once

#include <exec/task.hpp>

namespace aimrt::co {

template <typename T>
using Task = typename exec::task<T>;

}
