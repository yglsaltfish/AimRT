#pragma once

#include <cassert>
#include <functional>
#include <source_location>
#include <string>
#include <thread>

#include "util/exception.h"
#include "util/format.h"
#include "util/time_util.h"

namespace aimrt::common::util {

class SimpleLogger {
 public:
  static uint32_t GetLogLevel() { return 0; }

  static void Log(uint32_t lvl,
                  uint32_t line,
                  uint32_t column,
                  const char* file_name,
                  const char* function_name,
                  const char* log_data,
                  size_t log_data_size) {
    static constexpr std::string_view lvl_name_array[] = {
        "Trace", "Debug", "Info", "Warn", "Error", "Fatal"};

    static constexpr uint32_t lvl_name_array_size =
        sizeof(lvl_name_array) / sizeof(lvl_name_array[0]);
    if (lvl >= lvl_name_array_size) lvl = lvl_name_array_size;

    size_t tid;
#if defined(_WIN32)
    tid = std::hash<std::thread::id>{}(std::this_thread::get_id());
#else
    tid = gettid();
#endif

    auto t = std::chrono::system_clock::now();
    uint64_t time_stamp_us =
        std::chrono::duration_cast<std::chrono::microseconds>(t.time_since_epoch()).count();
    std::string log_str = ::aimrt_fmt::format(
        "[{}.{:0>6}][{}][{}][{}:{}:{} @{}]{}",
        GetTimeStr(std::chrono::system_clock::to_time_t(t)),
        (time_stamp_us % 1000000),
        lvl_name_array[lvl],
        tid,
        file_name,
        line,
        column,
        function_name,
        std::string_view(log_data, log_data_size));
    fprintf(stderr, "%s\n", log_str.c_str());
  }
};

template <typename T>
concept LoggerType =
    requires(T t) {
      { t.GetLogLevel() } -> std::same_as<uint32_t>;
      { t.Log(0, 0, 0, nullptr, nullptr, nullptr, 0) };
    };

class LoggerWrapper {
 public:
  template <LoggerType T>
  explicit LoggerWrapper(T* logger_ptr) {
    get_log_level_func_ = [logger_ptr]() -> uint32_t {
      return logger_ptr->GetLogLevel();
    };

    log_func_ = [logger_ptr](uint32_t lvl,
                             uint32_t line,
                             uint32_t column,
                             const char* file_name,
                             const char* function_name,
                             const char* log_data,
                             size_t log_data_size) {
      logger_ptr->Log(
          lvl, line, column, file_name, function_name, log_data, log_data_size);
    };
  }

  // TODO : add concept
  template <LoggerType T>
  explicit LoggerWrapper(T logger) {
    get_log_level_func_ = [logger]() -> uint32_t {
      return logger.GetLogLevel();
    };

    log_func_ = [logger](uint32_t lvl,
                         uint32_t line,
                         uint32_t column,
                         const char* file_name,
                         const char* function_name,
                         const char* log_data,
                         size_t log_data_size) {
      logger.Log(
          lvl, line, column, file_name, function_name, log_data, log_data_size);
    };
  }

  template <LoggerType T = SimpleLogger>
  LoggerWrapper() {
    get_log_level_func_ = []() -> uint32_t {
      return T::GetLogLevel();
    };

    log_func_ = [](uint32_t lvl,
                   uint32_t line,
                   uint32_t column,
                   const char* file_name,
                   const char* function_name,
                   const char* log_data,
                   size_t log_data_size) {
      T::Log(lvl, line, column, file_name, function_name, log_data, log_data_size);
    };
  }

  uint32_t GetLogLevel() const { return get_log_level_func_(); }

  void Log(uint32_t lvl,
           uint32_t line,
           uint32_t column,
           const char* file_name,
           const char* function_name,
           const char* log_data,
           size_t log_data_size) const {
    log_func_(lvl, line, column, file_name, function_name, log_data, log_data_size);
  }

 private:
  std::function<uint32_t(void)> get_log_level_func_;
  std::function<
      void(uint32_t, uint32_t, uint32_t, const char*, const char*, const char*, size_t)>
      log_func_;
};

}  // namespace aimrt::common::util

/// Log with the specified logger handle
#define AIMRT_HANDLE_LOG(__lgr__, __lvl__, __fmt__, ...)                                 \
  do {                                                                                   \
    if (__lvl__ >= __lgr__.GetLogLevel()) {                                              \
      std::string __log_str__ = ::aimrt_fmt::format(__fmt__, ##__VA_ARGS__);             \
      constexpr auto __location__ = std::source_location::current();                     \
      __lgr__.Log(                                                                       \
          __lvl__, __location__.line(), __location__.column(), __location__.file_name(), \
          __FUNCTION__, __log_str__.c_str(), __log_str__.size());                        \
    }                                                                                    \
  } while (0)

/// Check and log with the specified logger handle
#define AIMRT_HANDLE_CHECK_LOG(__lgr__, __expr__, __lvl__, __fmt__, ...) \
  do {                                                                   \
    if (!(__expr__)) [[unlikely]] {                                      \
      AIMRT_HANDLE_LOG(__lgr__, __lvl__, __fmt__, ##__VA_ARGS__);        \
    }                                                                    \
  } while (0)

/// Log and throw with the specified logger handle
#define AIMRT_HANDLE_LOG_THROW(__lgr__, __lvl__, __fmt__, ...)                           \
  do {                                                                                   \
    std::string __log_str__ = ::aimrt_fmt::format(__fmt__, ##__VA_ARGS__);               \
    if (__lvl__ >= __lgr__.GetLogLevel()) {                                              \
      constexpr auto __location__ = std::source_location::current();                     \
      __lgr__.Log(                                                                       \
          __lvl__, __location__.line(), __location__.column(), __location__.file_name(), \
          __FUNCTION__, __log_str__.c_str(), __log_str__.size());                        \
    }                                                                                    \
    throw aimrt::common::util::AimRTException(std::move(__log_str__));                   \
  } while (0)

/// Check and log and throw with the specified logger handle
#define AIMRT_HANDLE_CHECK_LOG_THROW(__lgr__, __expr__, __lvl__, __fmt__, ...) \
  do {                                                                         \
    if (!(__expr__)) [[unlikely]] {                                            \
      AIMRT_HANDLE_LOG_THROW(__lgr__, __lvl__, __fmt__, ##__VA_ARGS__);        \
    }                                                                          \
  } while (0)

#define AIMRT_HL_TRACE(__lgr__, __fmt__, ...) \
  AIMRT_HANDLE_LOG(__lgr__, 0, __fmt__, ##__VA_ARGS__)
#define AIMRT_HL_DEBUG(__lgr__, __fmt__, ...) \
  AIMRT_HANDLE_LOG(__lgr__, 1, __fmt__, ##__VA_ARGS__)
#define AIMRT_HL_INFO(__lgr__, __fmt__, ...) \
  AIMRT_HANDLE_LOG(__lgr__, 2, __fmt__, ##__VA_ARGS__)
#define AIMRT_HL_WARN(__lgr__, __fmt__, ...) \
  AIMRT_HANDLE_LOG(__lgr__, 3, __fmt__, ##__VA_ARGS__)
#define AIMRT_HL_ERROR(__lgr__, __fmt__, ...) \
  AIMRT_HANDLE_LOG(__lgr__, 4, __fmt__, ##__VA_ARGS__)
#define AIMRT_HL_FATAL(__lgr__, __fmt__, ...) \
  AIMRT_HANDLE_LOG(__lgr__, 5, __fmt__, ##__VA_ARGS__)

#define AIMRT_HL_CHECK_TRACE(__lgr__, __expr__, __fmt__, ...) \
  AIMRT_HANDLE_CHECK_LOG(__lgr__, __expr__, 0, __fmt__, ##__VA_ARGS__)
#define AIMRT_HL_CHECK_DEBUG(__lgr__, __expr__, __fmt__, ...) \
  AIMRT_HANDLE_CHECK_LOG(__lgr__, __expr__, 1, __fmt__, ##__VA_ARGS__)
#define AIMRT_HL_CHECK_INFO(__lgr__, __expr__, __fmt__, ...) \
  AIMRT_HANDLE_CHECK_LOG(__lgr__, __expr__, 2, __fmt__, ##__VA_ARGS__)
#define AIMRT_HL_CHECK_WARN(__lgr__, __expr__, __fmt__, ...) \
  AIMRT_HANDLE_CHECK_LOG(__lgr__, __expr__, 3, __fmt__, ##__VA_ARGS__)
#define AIMRT_HL_CHECK_ERROR(__lgr__, __expr__, __fmt__, ...) \
  AIMRT_HANDLE_CHECK_LOG(__lgr__, __expr__, 4, __fmt__, ##__VA_ARGS__)
#define AIMRT_HL_CHECK_FATAL(__lgr__, __expr__, __fmt__, ...) \
  AIMRT_HANDLE_CHECK_LOG(__lgr__, __expr__, 5, __fmt__, ##__VA_ARGS__)

#define AIMRT_HL_ERROR_THROW(__lgr__, __fmt__, ...) \
  AIMRT_HANDLE_LOG_THROW(__lgr__, 4, __fmt__, ##__VA_ARGS__)
#define AIMRT_HL_FATAL_THROW(__lgr__, __fmt__, ...) \
  AIMRT_HANDLE_LOG_THROW(__lgr__, 5, __fmt__, ##__VA_ARGS__)

#define AIMRT_HL_CHECK_ERROR_THROW(__lgr__, __expr__, __fmt__, ...) \
  AIMRT_HANDLE_CHECK_LOG_THROW(__lgr__, __expr__, 4, __fmt__, ##__VA_ARGS__)
#define AIMRT_HL_CHECK_FATAL_THROW(__lgr__, __expr__, __fmt__, ...) \
  AIMRT_HANDLE_CHECK_LOG_THROW(__lgr__, __expr__, 5, __fmt__, ##__VA_ARGS__)

/// Log with the default logger handle in current scope
#ifndef AIMRT_DEFAULT_LOGGER_HANDLE
  #define AIMRT_DEFAULT_LOGGER_HANDLE GetLogger()
#endif

#define AIMRT_TRACE(__fmt__, ...) \
  AIMRT_HANDLE_LOG(AIMRT_DEFAULT_LOGGER_HANDLE, 0, __fmt__, ##__VA_ARGS__)
#define AIMRT_DEBUG(__fmt__, ...) \
  AIMRT_HANDLE_LOG(AIMRT_DEFAULT_LOGGER_HANDLE, 1, __fmt__, ##__VA_ARGS__)
#define AIMRT_INFO(__fmt__, ...) \
  AIMRT_HANDLE_LOG(AIMRT_DEFAULT_LOGGER_HANDLE, 2, __fmt__, ##__VA_ARGS__)
#define AIMRT_WARN(__fmt__, ...) \
  AIMRT_HANDLE_LOG(AIMRT_DEFAULT_LOGGER_HANDLE, 3, __fmt__, ##__VA_ARGS__)
#define AIMRT_ERROR(__fmt__, ...) \
  AIMRT_HANDLE_LOG(AIMRT_DEFAULT_LOGGER_HANDLE, 4, __fmt__, ##__VA_ARGS__)
#define AIMRT_FATAL(__fmt__, ...) \
  AIMRT_HANDLE_LOG(AIMRT_DEFAULT_LOGGER_HANDLE, 5, __fmt__, ##__VA_ARGS__)

#define AIMRT_CHECK_TRAC(__expr__, __fmt__, ...) \
  AIMRT_HANDLE_CHECK_LOG(AIMRT_DEFAULT_LOGGER_HANDLE, __expr__, 0, __fmt__, ##__VA_ARGS__)
#define AIMRT_CHECK_DEBUG(__expr__, __fmt__, ...) \
  AIMRT_HANDLE_CHECK_LOG(AIMRT_DEFAULT_LOGGER_HANDLE, __expr__, 1, __fmt__, ##__VA_ARGS__)
#define AIMRT_CHECK_INFO(__expr__, __fmt__, ...) \
  AIMRT_HANDLE_CHECK_LOG(AIMRT_DEFAULT_LOGGER_HANDLE, __expr__, 2, __fmt__, ##__VA_ARGS__)
#define AIMRT_CHECK_WARN(__expr__, __fmt__, ...) \
  AIMRT_HANDLE_CHECK_LOG(AIMRT_DEFAULT_LOGGER_HANDLE, __expr__, 3, __fmt__, ##__VA_ARGS__)
#define AIMRT_CHECK_ERROR(__expr__, __fmt__, ...) \
  AIMRT_HANDLE_CHECK_LOG(AIMRT_DEFAULT_LOGGER_HANDLE, __expr__, 4, __fmt__, ##__VA_ARGS__)
#define AIMRT_CHECK_FATAL(__expr__, __fmt__, ...) \
  AIMRT_HANDLE_CHECK_LOG(AIMRT_DEFAULT_LOGGER_HANDLE, __expr__, 5, __fmt__, ##__VA_ARGS__)

#define AIMRT_ERROR_THROW(__fmt__, ...) \
  AIMRT_HANDLE_LOG_THROW(AIMRT_DEFAULT_LOGGER_HANDLE, 4, __fmt__, ##__VA_ARGS__)
#define AIMRT_FATAL_THROW(__fmt__, ...) \
  AIMRT_HANDLE_LOG_THROW(AIMRT_DEFAULT_LOGGER_HANDLE, 5, __fmt__, ##__VA_ARGS__)

#define AIMRT_CHECK_ERROR_THROW(__expr__, __fmt__, ...) \
  AIMRT_HANDLE_CHECK_LOG_THROW(AIMRT_DEFAULT_LOGGER_HANDLE, __expr__, 4, __fmt__, ##__VA_ARGS__)
#define AIMRT_CHECK_FATAL_THROW(__expr__, __fmt__, ...) \
  AIMRT_HANDLE_CHECK_LOG_THROW(AIMRT_DEFAULT_LOGGER_HANDLE, __expr__, 5, __fmt__, ##__VA_ARGS__)
