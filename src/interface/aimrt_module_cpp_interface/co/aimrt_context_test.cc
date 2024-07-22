#include "aimrt_module_cpp_interface/co/aimrt_context.h"
#include "aimrt_module_cpp_interface/executor/executor.h"
#include "gtest/gtest.h"

namespace aimrt::co {

class AimrtContextTest : public ::testing::Test {
 protected:
  void SetUp() override {
    mock_executor_base_support_timer_schedule = GenBaseSupprotTimerSchedule();
    mock_executor_ref = executor::ExecutorRef(&mock_executor_base_support_timer_schedule);
    aimrt_scheduler = AimRTScheduler(mock_executor_ref);
    mock_receiver.is_set_value_called = false;
  }
  static aimrt_executor_base_t GenBaseSupprotTimerSchedule() {
    return aimrt_executor_base_t{
        .is_support_timer_schedule = [](void* impl) { return true; },
        .execute = [](void* impl, aimrt_function_base_t* task) mutable { static_cast<aimrt::util::Function<aimrt_function_executor_task_ops_t>>(task)(); },
        .now = [](void* impl) mutable -> uint64_t { return uint64_t(10); },
        .execute_at_ns = [](void* impl, uint64_t tp, aimrt_function_base_t* task) { static_cast<aimrt::util::Function<aimrt_function_executor_task_ops_t>>(task)(); },
    };
  };

  class MockReceiver {
   public:
    void set_value() noexcept {
      is_set_value_called = true;
    }

    void set_error(std::exception_ptr e) noexcept {
      try {
        if (e) {
          std::rethrow_exception(e);
        }
      } catch (const std::exception& ex) {
        is_set_error_called = true;
      }
    }
    static bool is_set_value_called;
    static bool is_set_error_called;
  } mock_receiver;

  aimrt_executor_base_t mock_executor_base_support_timer_schedule;
  executor::ExecutorRef mock_executor_ref;
  AimRTScheduler aimrt_scheduler;
};
bool AimrtContextTest::MockReceiver::is_set_value_called = false;
bool AimrtContextTest::MockReceiver::is_set_error_called = false;

// 测试AimrtContext的Task
TEST_F(AimrtContextTest, Task) {
  auto task = aimrt_scheduler.schedule();
  EXPECT_FALSE(mock_receiver.is_set_value_called);
  auto operaton_state_test = task.connect(mock_receiver);
  operaton_state_test.start();
  EXPECT_TRUE(mock_receiver.is_set_value_called);
}
// 测试AimrtContext的SchedulerAfterTask
TEST_F(AimrtContextTest, SchedulerAfterTask) {
  auto scheduler_after_task = aimrt_scheduler.schedule_after(std::chrono::nanoseconds(10));
  EXPECT_FALSE(mock_receiver.is_set_value_called);
  auto operaton_state_test = scheduler_after_task.connect(mock_receiver);
  operaton_state_test.start();
  EXPECT_TRUE(mock_receiver.is_set_value_called);
}
// 测试AimrtContext的SchedulerAtTask
TEST_F(AimrtContextTest, SchedulerAtTask) {
  const auto& tp = std::chrono::system_clock::now();
  auto scheduler_at_task = aimrt_scheduler.schedule_at(tp);
  EXPECT_FALSE(mock_receiver.is_set_value_called);
  auto operaton_state_test = scheduler_at_task.connect(mock_receiver);
  operaton_state_test.start();
  EXPECT_TRUE(mock_receiver.is_set_value_called);
}

};  // namespace aimrt::co
