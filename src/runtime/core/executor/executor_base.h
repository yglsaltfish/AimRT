#pragma once

#include <chrono>

#include "aimrt_module_cpp_interface/executor/executor_manager.h"
#include "aimrt_module_cpp_interface/util/function.h"
#include "aimrt_module_cpp_interface/util/string.h"

#include "yaml-cpp/yaml.h"

namespace aimrt::runtime::core::executor {

class ExecutorBase {
 public:
  ExecutorBase() = default;
  virtual ~ExecutorBase() = default;

  ExecutorBase(const ExecutorBase&) = delete;
  ExecutorBase& operator=(const ExecutorBase&) = delete;

  virtual void Initialize(std::string_view name, YAML::Node options_node) = 0;
  virtual void Start() = 0;
  virtual void Shutdown() = 0;

  virtual std::list<std::pair<std::string, std::string>> GenInitializationReport() const { return {}; }

  virtual std::string_view Type() const = 0;  // It should always return the same value
  virtual std::string_view Name() const = 0;  // It should always return the same value

  virtual bool ThreadSafe() const = 0;            // It should always return the same value
  virtual bool SupportTimerSchedule() const = 0;  // It should always return the same value

  /**
   * @brief Check if the caller is executing on this executor
   * @note
   * 1. This method will only be called after 'Initialize' and before 'Shutdown'.
   * 2. If return true, then it must be inside this executor, otherwise it may not be.
   *
   * @return Check result
   */
  virtual bool IsInCurrentExecutor() const = 0;

  /**
   * @brief Execute a task
   * @note
   * 1. This method will only be called after 'Initialize' and before 'Shutdown'.
   * 2. The Executor can define the actual behavior.
   *
   * @param task
   */
  virtual void Execute(aimrt::executor::Task&& task) = 0;

  /**
   * @brief Get the time point in this executor
   * @note
   * 1. This method will only be called after 'Initialize' and before 'Shutdown'.
   * 2. The Executor can define the actual behavior.
   *
   * @return std::chrono::system_clock::time_point
   */
  virtual std::chrono::system_clock::time_point Now() const = 0;

  /**
   * @brief Execute a task at a spectial time point in this executor
   * @note
   * 1. This method will only be called after 'Initialize' and before 'Shutdown'.
   * 2. The Executor can define the actual behavior.
   *
   * @param tp
   * @param task
   */
  virtual void ExecuteAt(std::chrono::system_clock::time_point tp, aimrt::executor::Task&& task) = 0;
};

}  // namespace aimrt::runtime::core::executor
