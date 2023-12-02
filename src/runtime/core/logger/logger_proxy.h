#pragma once

#include <memory>
#include <thread>

#include "aimrt_module_c_interface/logger/logger_base.h"
#include "core/logger/logger_backend_base.h"

namespace aimrt::runtime::core::logger {

class LoggerProxy {
 public:
  LoggerProxy(
      std::string_view module_name, aimrt_log_level_t lvl,
      const std::vector<LoggerBackendBase*>& used_logger_backend_ptr_vec)
      : module_name_(module_name),
        lvl_(lvl),
        used_logger_backend_ptr_vec_(used_logger_backend_ptr_vec),
        base_(GenBase(this)) {}

  ~LoggerProxy() = default;

  LoggerProxy(const LoggerProxy&) = delete;
  LoggerProxy& operator=(const LoggerProxy&) = delete;

  const aimrt_logger_base_t* NativeHandle() const { return &base_; }

 private:
  aimrt_log_level_t GetLogLevel() const { return lvl_; }

  void Log(aimrt_log_level_t lvl,
           uint32_t line,
           uint32_t column,
           const char* file_name,
           const char* function_name,
           const char* log_data,
           size_t log_data_size) const {
    if (lvl >= lvl_) {
      size_t tid;
#if defined(_WIN32)
      tid = std::hash<std::thread::id>{}(std::this_thread::get_id());
#else
      tid = gettid();
#endif

      auto log_data_wrapper = LogDataWrapper{
          .module_name = module_name_,
          .thread_id = tid,
          .t = std::chrono::system_clock::now(),
          .lvl = lvl,
          .line = line,
          .column = column,
          .file_name = file_name,
          .function_name = function_name,
          .log_data = log_data,
          .log_data_size = log_data_size};

      auto format_log_str_ptr = std::make_shared<std::string>();
      for (auto& logger_backend_ptr : used_logger_backend_ptr_vec_) {
        logger_backend_ptr->Log(log_data_wrapper, format_log_str_ptr);
      }
    }
  }

  static aimrt_logger_base_t GenBase(void* impl) {
    return aimrt_logger_base_t{
        .get_log_level = [](void* impl) -> aimrt_log_level_t {
          return static_cast<LoggerProxy*>(impl)->GetLogLevel();
        },
        .log = [](void* impl,
                  aimrt_log_level_t lvl,
                  uint32_t line,
                  uint32_t column,
                  const char* file_name,
                  const char* function_name,
                  const char* log_data,
                  size_t log_data_size) {
          static_cast<LoggerProxy*>(impl)->Log(
              lvl,
              line,
              column,
              file_name,
              function_name,
              log_data,
              log_data_size);  //
        },
        .impl = impl};
  }

 private:
  const std::string module_name_;
  const aimrt_log_level_t lvl_;
  const std::vector<LoggerBackendBase*>& used_logger_backend_ptr_vec_;

  const aimrt_logger_base_t base_;
};

}  // namespace aimrt::runtime::core::logger