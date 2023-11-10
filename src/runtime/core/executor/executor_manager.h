#pragma once

#include <atomic>
#include <functional>
#include <map>
#include <memory>
#include <string>
#include <vector>

#include "core/executor/executor_proxy.h"
#include "core/util/module_detail_info.h"

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

  using ExecutorGenFunc = std::function<std::unique_ptr<ExecutorBase>()>;

 public:
  ExecutorManager() = default;
  ~ExecutorManager() = default;

  ExecutorManager(const ExecutorManager&) = delete;
  ExecutorManager& operator=(const ExecutorManager&) = delete;

  void Initialize(YAML::Node options_node);
  void Start();
  void Shutdown();

  void RegisterExecutorGenFunc(std::string_view type,
                               ExecutorGenFunc&& executor_gen_func);

  ExecutorManagerProxy& GetExecutorManagerProxy(const util::ModuleDetailInfo& module_info);

 private:
  void RegisterThreadExecutorGenFunc();

 private:
  enum class Status : uint32_t {
    PreInit,
    Init,
    Start,
    Shutdown,
  };

  Options options_;
  std::atomic<Status> status_ = Status::PreInit;

  std::map<std::string, ExecutorGenFunc> executor_gen_func_map_;

  std::vector<std::unique_ptr<ExecutorBase> > executor_vec_;

  std::map<std::string, std::unique_ptr<ExecutorProxy>, std::less<> > executor_proxy_map_;

  std::map<std::string, std::unique_ptr<ExecutorManagerProxy> > executor_manager_proxy_map_;
};

}  // namespace aimrt::runtime::core::executor
