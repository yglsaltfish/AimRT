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

  virtual std::string_view Type() const = 0;
  virtual std::string_view Name() const = 0;

  virtual bool ThreadSafe() const = 0;
  virtual bool IsInCurrentExecutor() const = 0;
  virtual bool SupportTimerSchedule() const = 0;

  virtual void Execute(aimrt::executor::Task&& task) = 0;

  virtual std::chrono::system_clock::time_point Now() const = 0;
  virtual void ExecuteAt(std::chrono::system_clock::time_point tp, aimrt::executor::Task&& task) = 0;
};

}  // namespace aimrt::runtime::core::executor
