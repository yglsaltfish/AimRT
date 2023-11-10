#pragma once

#include <atomic>
#include <map>
#include <memory>
#include <vector>

#include "aimrt_module_cpp_interface/executor/executor.h"
#include "core/logger/log_level_tool.h"
#include "core/logger/logger_proxy.h"
#include "core/util/module_detail_info.h"

#include "yaml-cpp/yaml.h"

namespace aimrt::runtime::core::logger {

class LoggerManager {
 public:
  struct Options {
    aimrt_log_level_t core_lvl = aimrt_log_level_t::AIMRT_LOG_LEVEL_TRACE;
    aimrt_log_level_t default_module_lvl =
        aimrt_log_level_t::AIMRT_LOG_LEVEL_TRACE;

    struct BackendOptions {
      std::string type;
      YAML::Node options;
    };
    std::vector<BackendOptions> backends_options;
  };

 public:
  LoggerManager() = default;
  ~LoggerManager() = default;

  LoggerManager(const LoggerManager&) = delete;
  LoggerManager& operator=(const LoggerManager&) = delete;

  void Initialize(YAML::Node options_node);
  void Start();
  void Shutdown();

  void SetLogExecutor(ExecutorRef log_executor);

  void RegisterLoggerBackend(
      std::unique_ptr<LoggerBackendBase>&& logger_backend_ptr);

  LoggerProxy& GetLoggerProxy(const util::ModuleDetailInfo& module_info);
  LoggerProxy& GetLoggerProxy(std::string_view logger_name);

 private:
  void RegisterConsoleLoggerBackend();
  void RegisterRotateFileLoggerBackend();

 private:
  enum class Status : uint32_t {
    PreInit,
    Init,
    Start,
    Shutdown,
  };

  Options options_;
  std::atomic<Status> status_ = Status::PreInit;

  ExecutorRef log_executor_;

  std::vector<std::unique_ptr<LoggerBackendBase> > logger_backend_ptr_vec_;
  std::vector<LoggerBackendBase*> used_logger_backend_ptr_vec_;

  std::map<std::string, std::unique_ptr<LoggerProxy> > logger_proxy_map_;
};

}  // namespace aimrt::runtime::core::logger
