#include <gtest/gtest.h>

#include "core/executor/executor_manager.h"
#include "core/util/module_detail_info.h"

namespace aimrt::runtime::core::executor {

class ExecutorManagerTest : public ::testing::Test {
 protected:
  void SetUp() override {
  }

  void TearDown() override {
    executor_manager_.Shutdown();
  }

  ExecutorManager executor_manager_;
};

TEST_F(ExecutorManagerTest, initialize1) {
  YAML::Node options_node = YAML::Load(R"str(

)str");

  executor_manager_.Initialize(options_node);
}

TEST_F(ExecutorManagerTest, initialize2) {
  YAML::Node options_node = YAML::Load(R"str(
    executors:
    - name: work_thread_pool
      type: asio_thread
      options:
        thread_num: 2
  )str");

  executor_manager_.Initialize(options_node);

  util::ModuleDetailInfo detail_info = {
      .name = "test",
  };

  auto executor_manager = executor_manager_.GetExecutorManagerProxy(detail_info).NativeHandle();

  auto executor_ptr = executor_manager->get_executor(
      executor_manager->impl, aimrt::util::ToAimRTStringView("work_thread_pool"));

  ASSERT_NE(executor_manager_.GetExecutorManagerProxy(detail_info).NativeHandle(), nullptr);
  EXPECT_EQ(aimrt::util::ToStdStringView(executor_ptr->type(executor_ptr->impl)), "asio_thread");
  EXPECT_EQ(aimrt::util::ToStdStringView(executor_ptr->name(executor_ptr->impl)), "work_thread_pool");
}

TEST_F(ExecutorManagerTest, start) {
  YAML::Node options_node = YAML::Load(R"str(

)str");

  executor_manager_.Initialize(options_node);

  executor_manager_.Start();
}

TEST_F(ExecutorManagerTest, test_throw) {
  executor_manager_.Shutdown();

  YAML::Node thread_options_node = YAML::Load(R"str(

  )str");

  EXPECT_THROW(executor_manager_.Initialize(thread_options_node), std::exception);
}
}  // namespace aimrt::runtime::core::executor
