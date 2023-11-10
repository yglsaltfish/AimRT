#pragma once

#ifdef AIMRT_USE_EXECUTOR

  #include <unifex/scheduler_concepts.hpp>

namespace aimrt {
namespace co {

inline constexpr auto& Schedule = unifex::schedule;
inline constexpr auto& ScheduleAfter = unifex::schedule_after;
inline constexpr auto& ScheduleAt = unifex::schedule_at;

}  // namespace co
}  // namespace aimrt

#endif
