#pragma once

#include <fstream>

#include "aimrt_module_cpp_interface/executor/executor.h"
#include "core/logger/logger_backend_base.h"

namespace aimrt::runtime::core::logger {

class RotateFileLoggerBackend : public LoggerBackendBase {
 public:
  struct Options {
    std::string path = "log";
    std::string filename = "app.log";
    uint32_t max_file_size_m = 16;
    uint32_t max_file_num = 0;
  };

 public:
  RotateFileLoggerBackend() = default;
  ~RotateFileLoggerBackend() override;

  std::string_view Name() const override { return "rotate_file"; }

  void Initialize(YAML::Node options_node) override;
  void Shutdown() override { run_flag_.store(false); }

  void SetLogExecutor(executor::ExecutorRef log_executor) {
    log_executor_ = log_executor;
  }

  void Log(const LogDataWrapper& log_data_wrapper,
           const std::shared_ptr<std::string>& format_log_str_ptr) override;

 private:
  bool OpenNewFile();
  void CleanLogFile();
  uint32_t GetNextIndex();

 private:
  Options options_;
  executor::ExecutorRef log_executor_;

  std::string base_file_name_;  // 基础文件路径
  std::ofstream ofs_;

  std::atomic_bool run_flag_ = false;
};

}  // namespace aimrt::runtime::core::logger
