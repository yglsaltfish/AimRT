#include <gmock/gmock.h>
#include <gtest/gtest.h>
#include <filesystem>
#include <fstream>
#include <source_location>
#include <thread>

#include "core/executor/executor_manager.h"
#include "core/logger/console_logger_backend.h"
#include "core/logger/logger_manager.h"
#include "core/util/module_detail_info.h"

namespace aimrt::runtime::core::logger {

class LoggerBackendMock : public LoggerBackendBase {
 public:
  MOCK_CONST_METHOD0(Type, std::string_view());
  MOCK_METHOD1(Initialize, void(YAML::Node));
  MOCK_METHOD0(Start, void());
  MOCK_METHOD0(Shutdown, void());
  MOCK_CONST_METHOD0(AllowDuplicates, bool());
  MOCK_METHOD1(Log, void(const LogDataWrapper&));
};

class LoggerManagerTest : public ::testing::Test {
 protected:
  void SetUp() override {
    YAML::Node thread_options_node = YAML::Load(R"str(
      executors:
      - name: work_thread_pool
        type: asio_thread
        options:
          thread_num: 1
    )str");

    executor_manager_.Initialize(thread_options_node);

    logger_manager_.RegisterGetExecutorFunc(
        [this](std::string_view name) {
          return executor_manager_.GetExecutor(name);
        });

    // register the mocked backend, register can only in PreInit state.
    logger_manager_.RegisterLoggerBackendGenFunc("mocked_backend", [this]() -> std::unique_ptr<LoggerBackendBase> {
      std::unique_ptr<LoggerBackendBase> mocked_backend_ptr = std::make_unique<LoggerBackendMock>();
      EXPECT_CALL(*(static_cast<LoggerBackendMock*>(mocked_backend_ptr.get())), Type)
          .WillRepeatedly(testing::Return("mocked_backend"));
      EXPECT_CALL(*(static_cast<LoggerBackendMock*>(mocked_backend_ptr.get())), Log)
          .WillRepeatedly([this](const LogDataWrapper& logger_wrapper) {  //
            this->log_res_ = std::string(logger_wrapper.log_data, logger_wrapper.log_data_size);
          });
      EXPECT_CALL(*(static_cast<LoggerBackendMock*>(mocked_backend_ptr.get())), Initialize)
          .WillRepeatedly([](YAML::Node) {});
      EXPECT_CALL(*(static_cast<LoggerBackendMock*>(mocked_backend_ptr.get())), Shutdown)
          .WillRepeatedly([]() {});

      return mocked_backend_ptr;
    });

    YAML::Node logger_manager_options_node = YAML::Load(R"str(
        log:
        core_lvl: INFO
        default_module_lvl: INFO
        backends:
          - type: console
            options:
              color: true
              log_executor_name: work_thread_pool
          - type: rotate_file
            options:
              path: ./
              filename: logger_manager_test.log
              max_file_size_m: 4
              max_file_num: 10
              log_executor_name: work_thread_pool
          - type: mocked_backend
      )str");
    logger_manager_.Initialize(logger_manager_options_node);
  }

  void TearDown() override {
    logger_manager_.Shutdown();
    executor_manager_.Shutdown();
  }

  LoggerManager logger_manager_;
  executor::ExecutorManager executor_manager_;
  std::string log_res_;
};

TEST_F(LoggerManagerTest, initialize_and_start_using_cfg_log_lvl) {
  const util::ModuleDetailInfo module_info = {
      .name = "logger_manager_test",
      .use_default_log_lvl = false,
  };

  auto h = logger_manager_.GetLoggerProxy(module_info).NativeHandle();
  ASSERT_NE(h, nullptr);
  EXPECT_EQ(h->get_log_level(h->impl), AIMRT_LOG_LEVEL_TRACE);

  logger_manager_.Start();
}

TEST_F(LoggerManagerTest, initialize_using_default_log_lvl) {
  const util::ModuleDetailInfo module_info = {
      .name = "logger_manager_test",
      .log_lvl = AIMRT_LOG_LEVEL_DEBUG,
      .use_default_log_lvl = true,
  };

  auto h = logger_manager_.GetLoggerProxy(module_info).NativeHandle();
  ASSERT_NE(h, nullptr);
  EXPECT_EQ(h->get_log_level(h->impl), AIMRT_LOG_LEVEL_INFO);
}

TEST_F(LoggerManagerTest, log_with_backends) {
  auto logger = logger_manager_.GetLoggerProxy("logger_manager_test").NativeHandle();
  ASSERT_NE(logger, nullptr);

  executor_manager_.Start();
  logger_manager_.Start();

  auto location = std::source_location::current();
  std::string log_str("logger_test");
  logger->log(logger->impl,
              AIMRT_LOG_LEVEL_INFO,
              location.line(),
              location.column(),
              location.file_name(),
              location.function_name(),
              log_str.c_str(),
              log_str.size());
  std::this_thread::sleep_for(std::chrono::milliseconds(100));  // wait the log thread

  // test the mocked_backend
  EXPECT_EQ(log_res_, log_str);
}
}  // namespace aimrt::runtime::core::logger