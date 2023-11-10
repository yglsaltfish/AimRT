#include <gtest/gtest.h>

#include "core/executor/thread_executor.h"

namespace aimrt::runtime::core::executor {

TEST(THREAD_EXECUTOR_TEST, base) {
  YAML::Node options_node = YAML::Load(R"str(
thread_num: 1
)str");

  std::string_view name = "test_thread";

  ThreadExecutor thread_executor;
  thread_executor.Initialize(name, options_node);

  EXPECT_EQ(thread_executor.Type(), "thread");
  EXPECT_EQ(thread_executor.Name(), name);
  EXPECT_TRUE(thread_executor.ThreadSafe());

  thread_executor.Shutdown();
}

TEST(THREAD_EXECUTOR_TEST, base2) {
  YAML::Node options_node = YAML::Load(R"str(
thread_num: 2
)str");

  std::string_view name = "test_thread_2";

  ThreadExecutor thread_executor;
  thread_executor.Initialize(name, options_node);

  EXPECT_EQ(thread_executor.Type(), "thread");
  EXPECT_EQ(thread_executor.Name(), name);
  EXPECT_FALSE(thread_executor.ThreadSafe());

  thread_executor.Shutdown();
}

TEST(THREAD_EXECUTOR_TEST, execute) {
  YAML::Node options_node = YAML::Load(R"str(
    thread_num: 1
    thread_sched_policy: SCHED_OTHER
    thread_bind_cpu: [2]
    timeout_alarm_threshold_us: 1000
  )str");

  std::string_view name = "test_thread";

  ThreadExecutor thread_executor;
  thread_executor.Initialize(name, options_node);

  thread_executor.Start();

  EXPECT_FALSE(thread_executor.IsInCurrentExecutor());

  bool ret = false;
  thread_executor.Execute(
      [&]() { ret = thread_executor.IsInCurrentExecutor(); });

  std::this_thread::sleep_for(std::chrono::milliseconds(100));
  EXPECT_TRUE(ret);

  // ExecuteAfterNs
  ret = false;
  thread_executor.ExecuteAfterNs(1000 * 1000 * 5, [&]() { ret = true; });
  EXPECT_FALSE(ret);

  std::this_thread::sleep_for(std::chrono::milliseconds(100));
  EXPECT_TRUE(ret);

  // ExecuteAtNs
  ret = false;
  thread_executor.ExecuteAtNs(
      std::chrono::steady_clock::now().time_since_epoch().count() +
          1000 * 1000 * 5,
      [&]() { ret = true; });
  EXPECT_FALSE(ret);

  std::this_thread::sleep_for(std::chrono::milliseconds(100));
  EXPECT_TRUE(ret);

  thread_executor.Shutdown();
}

}  // namespace aimrt::runtime::core::executor