#pragma once

#include "aimrt_module_cpp_interface/executor/executor.h"
#include "core/logger/logger_backend_base.h"

namespace aimrt::runtime::core::logger {

class ConsoleLoggerBackend : public LoggerBackendBase {
 public:
  struct Options {
    bool print_color = true;
    std::string log_executor_name = "";
  };

 public:
  ConsoleLoggerBackend() = default;
  ~ConsoleLoggerBackend() override = default;

  std::string_view Name() const override { return "console"; }

  void Initialize(YAML::Node options_node) override;
  void Shutdown() override { run_flag_.store(false); }

  void RegisterGetExecutorFunc(
      const std::function<aimrt::executor::ExecutorRef(std::string_view)>& get_executor_func) {
    get_executor_func_ = get_executor_func;
  }

  void Log(const LogDataWrapper& log_data_wrapper,
           const std::shared_ptr<std::string>& format_log_str_ptr) override;

 private:
  Options options_;
  std::function<aimrt::executor::ExecutorRef(std::string_view)> get_executor_func_;
  aimrt::executor::ExecutorRef log_executor_;
  std::atomic_bool run_flag_ = false;
};

}  // namespace aimrt::runtime::core::logger
