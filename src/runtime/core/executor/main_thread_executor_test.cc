#include <gtest/gtest.h>
#include <thread>

#include "core/executor/main_thread_executor.h"

namespace aimrt::runtime::core::executor {
class MainThreadExecutorTest : public ::testing::Test {
 protected:
  void SetUp() override {
  }

  void TearDown() override {
  }

 public:
  MainThreadExecutor main_thread_executor_;
};

TEST_F(MainThreadExecutorTest, execute) {
  YAML::Node options_node = YAML::Load(R"str(
    thread_sched_policy: SCHED_OTHER
    thread_bind_cpu: [0]
  )str");

  main_thread_executor_.Initialize(options_node);
  EXPECT_EQ(main_thread_executor_.Type(), "tbb_thread");
  EXPECT_EQ(main_thread_executor_.Name(), "aimrt_main");
  EXPECT_EQ(main_thread_executor_.ThreadSafe(), true);

  std::thread timer_thread([&]() {
    std::this_thread::sleep_for(std::chrono::milliseconds(1000));
    main_thread_executor_.Execute([&]() {
      main_thread_executor_.Shutdown();
    });
  });

  main_thread_executor_.Start();

  timer_thread.join();
}
}  // namespace aimrt::runtime::core::executor