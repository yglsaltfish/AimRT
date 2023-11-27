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
  MOCK_CONST_METHOD0(Name, std::string_view());
  MOCK_METHOD1(Initialize, void(YAML::Node));
  MOCK_METHOD0(Shutdown, void());
  MOCK_METHOD2(Log, void(const LogDataWrapper&, const std::shared_ptr<std::string>&));
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

    util::ModuleDetailInfo detail_info = {
        .name = "logger_test",
    };
    logger_manager_.SetLogExecutor(
        aimrt::executor::ExecutorRef{executor_manager_.GetExecutorManagerProxy(detail_info).GetExecutor("work_thread_pool")->NativeHandle()});

    // register the mocked backend, register can only in PreInit state.
    std::unique_ptr<LoggerBackendBase> mocked_backend_ptr = std::make_unique<LoggerBackendMock>();
    EXPECT_CALL(*(static_cast<LoggerBackendMock*>(mocked_backend_ptr.get())), Name)
        .WillRepeatedly(testing::Return("mocked_backend"));
    EXPECT_CALL(*(static_cast<LoggerBackendMock*>(mocked_backend_ptr.get())), Log)
        .WillRepeatedly([this](const LogDataWrapper& logger_wrapper, const std::shared_ptr<std::string>&) {  //
          this->log_res_ = std::string(logger_wrapper.log_data, logger_wrapper.log_data_size);
        });
    EXPECT_CALL(*(static_cast<LoggerBackendMock*>(mocked_backend_ptr.get())), Initialize)
        .WillRepeatedly([](YAML::Node) {});
    EXPECT_CALL(*(static_cast<LoggerBackendMock*>(mocked_backend_ptr.get())), Shutdown)
        .WillRepeatedly([]() {});

    logger_manager_.RegisterLoggerBackend(std::move(mocked_backend_ptr));

    YAML::Node logger_manager_options_node = YAML::Load(R"str(
        log: # log配置
        core_lvl: TRACE # 内核日志等级，可选项：Trace/Debug/Info/Warn/Error/Fatal/Off，不区分大小写
        default_module_lvl: INFO # 模块默认日志等级
        backends: # 日志backends
          - type: console # 控制台日志
            options:
              color: true # 是否彩色打印
          - type: rotate_file # 文件日志
            options:
              path: ./ # 日志文件路径
              filename: logger_manager_test.log # 日志文件名称
              max_file_size_m: 4 # 日志文件最大尺寸，单位m
              max_file_num: 10 # 最大日志文件数量，0代表无限
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

  ASSERT_NE(logger_manager_.GetLoggerProxy(module_info).NativeHandle(), nullptr);
  EXPECT_EQ(logger_manager_.GetLoggerProxy(module_info).GetLogLevel(), AIMRT_LOG_LEVEL_TRACE);

  logger_manager_.Start();
}

TEST_F(LoggerManagerTest, initialize_using_default_log_lvl) {
  const util::ModuleDetailInfo module_info = {
      .name = "logger_manager_test",
      .log_lvl = AIMRT_LOG_LEVEL_DEBUG,
      .use_default_log_lvl = true,
  };

  ASSERT_NE(logger_manager_.GetLoggerProxy(module_info).NativeHandle(), nullptr);
  EXPECT_EQ(logger_manager_.GetLoggerProxy(module_info).GetLogLevel(), AIMRT_LOG_LEVEL_INFO);
}

TEST_F(LoggerManagerTest, log_with_backends) {
  const util::ModuleDetailInfo module_info = {
      .name = "logger_manager_test",
  };
  auto& logger = logger_manager_.GetLoggerProxy(module_info);
  ASSERT_NE(logger.NativeHandle(), nullptr);

  executor_manager_.Start();
  logger_manager_.Start();

  auto location = std::source_location::current();
  std::string log_str("logger_test");
  logger.Log(AIMRT_LOG_LEVEL_INFO,
             location.line(),
             location.column(),
             location.file_name(),
             location.function_name(),
             log_str.c_str(),
             log_str.size());
  std::this_thread::sleep_for(std::chrono::milliseconds(100));  // wait the log thread

  const std::filesystem::path log_file_path = "./logger_manager_test.log";
  auto file_status = std::filesystem::status(log_file_path);

  auto is_exist = std::filesystem::exists(file_status);
  EXPECT_EQ(is_exist, true);

  if (is_exist) {
    std::remove(log_file_path.c_str());
  }

  // test the mocked_backend
  EXPECT_EQ(log_res_, log_str);
}
}  // namespace aimrt::runtime::core::logger