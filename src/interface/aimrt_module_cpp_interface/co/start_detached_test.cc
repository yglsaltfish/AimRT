#include "gtest/gtest.h"

#ifdef AIMRT_USE_EXECUTOR

  #include "aimrt_module_cpp_interface/co/start_detached.h"
  #include "aimrt_module_cpp_interface/co/task.h"

namespace aimrt {
namespace co {

TEST(StartDetached, base) {
  int n = 0;
  auto work = []() -> Task<int> { co_return 42; };

  StartDetached(work(), [&n](int ret) { n = ret; });

  EXPECT_EQ(n, 42);
}

}  // namespace co
}  // namespace aimrt

#endif
