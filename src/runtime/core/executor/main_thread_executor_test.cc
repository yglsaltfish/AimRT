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

  signal(SIGINT, [](int) {});

  std::thread raising_thread([this]() {
    std::this_thread::sleep_for(std::chrono::milliseconds(100));
    bool ret = false;
    this->main_thread_executor_.Execute([&]() { ret = true; });
    std::this_thread::sleep_for(std::chrono::milliseconds(100));
    EXPECT_TRUE(ret);

    ret = false;
    this->main_thread_executor_.ExecuteAfterNs(1000 * 1000 * 5, [&]() { ret = true; });
    EXPECT_FALSE(ret);
    std::this_thread::sleep_for(std::chrono::milliseconds(100));
    EXPECT_TRUE(ret);

    ret = false;
    this->main_thread_executor_.ExecuteAtNs(1000 * 1000 * 5, [&]() { ret = true; });
    EXPECT_FALSE(ret);
    std::this_thread::sleep_for(std::chrono::milliseconds(100));
    EXPECT_TRUE(ret);

    raise(SIGINT);
  });

  main_thread_executor_.RegisterSignalHandle(
      std::set<int>{SIGINT}, [this](auto, auto) {
        this->main_thread_executor_.Shutdown();
      });
  main_thread_executor_.Initialize(options_node);
  EXPECT_EQ(main_thread_executor_.Type(), "thread");
  EXPECT_EQ(main_thread_executor_.Name(), "main_thread");
  EXPECT_EQ(main_thread_executor_.ThreadSafe(), true);
  main_thread_executor_.Start();

  raising_thread.join();
}
}  // namespace aimrt::runtime::core::executor