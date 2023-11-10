#pragma once

#include "aimrt_module_cpp_interface/executor/executor.h"
#include "core/logger/logger_backend_base.h"

namespace aimrt::runtime::core::logger {

class ConsoleLoggerBackend : public LoggerBackendBase {
 public:
  struct Options {
    bool print_color = true;
  };

 public:
  ConsoleLoggerBackend() = default;
  ~ConsoleLoggerBackend() override = default;

  std::string_view Name() const override { return "console"; }

  void Initialize(YAML::Node options_node) override;
  void Shutdown() override {}

  void SetLogExecutor(ExecutorRef log_executor) {
    log_executor_ = log_executor;
  }

  void Log(const LogDataWrapper& log_data_wrapper,
           const std::shared_ptr<std::string>& format_log_str_ptr) override;

 private:
  Options options_;
  ExecutorRef log_executor_;
};

}  // namespace aimrt::runtime::core::logger
