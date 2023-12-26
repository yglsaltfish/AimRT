#pragma once

#include <atomic>
#include <functional>
#include <memory>
#include <string>
#include <unordered_map>
#include <vector>

#include "aimrt_module_cpp_interface/executor/executor.h"
#include "core/executor/executor_proxy.h"
#include "core/util/module_detail_info.h"
#include "util/log_util.h"
#include "util/string_util.h"

#include "yaml-cpp/yaml.h"

namespace aimrt::runtime::core::executor {

class ExecutorManager {
 public:
  struct Options {
    struct ExecutorOptions {
      std::string name;
      std::string type;
      YAML::Node options;
    };
    std::vector<ExecutorOptions> executors_options;
  };

  enum class State : uint32_t {
    PreInit,
    Init,
    Start,
    Shutdown,
  };

  using ExecutorGenFunc = std::function<std::unique_ptr<ExecutorBase>()>;

 public:
  ExecutorManager()
      : logger_ptr_(std::make_shared<common::util::LoggerWrapper>()) {}
  ~ExecutorManager() = default;

  ExecutorManager(const ExecutorManager&) = delete;
  ExecutorManager& operator=(const ExecutorManager&) = delete;

  void Initialize(YAML::Node options_node);
  void Start();
  void Shutdown();

  void RegisterExecutorGenFunc(std::string_view type,
                               ExecutorGenFunc&& executor_gen_func);

  ExecutorManagerProxy& GetExecutorManagerProxy(const util::ModuleDetailInfo& module_info);

  State GetState() const { return state_.load(); }

  void SetLogger(const std::shared_ptr<common::util::LoggerWrapper>& logger_ptr) { logger_ptr_ = logger_ptr; }
  const common::util::LoggerWrapper& GetLogger() const { return *logger_ptr_; }

  aimrt::executor::ExecutorRef GetExecutor(std::string_view executor_name);

  const std::vector<std::unique_ptr<ExecutorBase>>& GetAllExecutors() const {
    return executor_vec_;
  }

 private:
  void RegisterAsioExecutorGenFunc();
  void RegisterTBBExecutorGenFunc();

 private:
  Options options_;
  std::atomic<State> state_ = State::PreInit;
  std::shared_ptr<common::util::LoggerWrapper> logger_ptr_;

  std::unordered_map<std::string, ExecutorGenFunc> executor_gen_func_map_;

  std::vector<std::unique_ptr<ExecutorBase>> executor_vec_;

  std::unordered_map<
      std::string,
      std::unique_ptr<ExecutorProxy>,
      aimrt::common::util::StringHash,
      std::equal_to<>>
      executor_proxy_map_;

  std::unordered_map<
      std::string,
      std::unique_ptr<ExecutorManagerProxy>>
      executor_manager_proxy_map_;
};

}  // namespace aimrt::runtime::core::executor
