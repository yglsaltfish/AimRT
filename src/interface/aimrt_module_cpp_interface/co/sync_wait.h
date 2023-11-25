#pragma once

#include <stdexec/execution.hpp>

namespace aimrt::co {

inline constexpr auto& SyncWait = stdexec::sync_wait;

}
