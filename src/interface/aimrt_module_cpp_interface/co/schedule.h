#pragma once

#ifdef AIMRT_USE_EXECUTOR

  #include <exec/timed_scheduler.hpp>
  #include <stdexec/execution.hpp>

namespace aimrt::co {

inline constexpr auto& Schedule = stdexec::schedule;
inline constexpr auto& ScheduleAfter = exec::schedule_after;
inline constexpr auto& ScheduleAt = exec::schedule_at;

}  // namespace aimrt::co

#endif
