#pragma once

#include <filesystem>
#include <string>
#include <vector>

#include "core/channel/channel_manager.h"
#include "core/configurator/configurator_manager.h"
#include "core/executor/executor_manager.h"
#include "core/executor/main_thread_executor.h"
#include "core/logger/logger_manager.h"
#include "core/module/module_manager.h"
#include "core/plugin/plugin_manager.h"
#include "core/rpc/rpc_manager.h"

namespace aimrt::runtime::core {

class AimRTCore {
 public:
  struct Options {
    std::filesystem::path cfg_file_path;

    bool dump_cfg_file = false;
    std::filesystem::path dump_cfg_file_path;
  };

  enum class HookPoint : uint32_t {
    PreInitMainThread,
    PostInitMainThread,

    PreInitLog,
    PostInitLog,

    PreInitExecutor,
    PostInitExecutor,

    PreInitRpc,
    PostInitRpc,

    PreInitChannel,
    PostInitChannel,

    PreInitModules,
    PostInitModules,

    PreStart,
    PostStart,

    PreShutdown,
    PostShutdown,

    HookPointNum,
  };

 public:
  AimRTCore() = default;
  ~AimRTCore();

  AimRTCore(const AimRTCore&) = delete;
  AimRTCore& operator=(const AimRTCore&) = delete;

  void Initialize(const Options& options);
  void Start();
  void Shutdown();

  template <typename... Args>
    requires std::constructible_from<std::function<void()>, Args...>
  void RegisterHookFunc(HookPoint hook_point, Args&&... args) {
    hook_func_vec_array_[static_cast<uint32_t>(hook_point)].emplace_back(
        std::forward<Args>(args)...);
  }

  executor::MainThreadExecutor& GetMainThreadExecutor() { return main_thread_executor_; }
  const executor::MainThreadExecutor& GetMainThreadExecutor() const { return main_thread_executor_; }

  configurator::ConfiguratorManager& GetConfiguratorManager() { return configurator_manager_; }
  const configurator::ConfiguratorManager& GetConfiguratorManager() const { return configurator_manager_; }

  plugin::PluginManager& GetPluginManager() { return plugin_manager_; }
  const plugin::PluginManager& GetPluginManager() const { return plugin_manager_; }

  logger::LoggerManager& GetLoggerManager() { return logger_manager_; }
  const logger::LoggerManager& GetLoggerManager() const { return logger_manager_; }

  executor::ExecutorManager& GetExecutorManager() { return executor_manager_; }
  const executor::ExecutorManager& GetExecutorManager() const { return executor_manager_; }

  rpc::RpcManager& GetRpcManager() { return rpc_manager_; }
  const rpc::RpcManager& GetRpcManager() const { return rpc_manager_; }

  channel::ChannelManager& GetChannelManager() { return channel_manager_; }
  const channel::ChannelManager& GetChannelManager() const { return channel_manager_; }

  module::ModuleManager& GetModuleManager() { return module_manager_; }
  const module::ModuleManager& GetModuleManager() const { return module_manager_; }

 private:
  void ExecuteHook(HookPoint hook_point) {
    for (const auto& func :
         hook_func_vec_array_[static_cast<uint32_t>(hook_point)])
      func();
  }

  ExecutorRef GetExecutor(std::string_view executor_name) {
    auto ptr = GetExecutorManager()
                   .GetExecutorManagerProxy(util::ModuleDetailInfo{})
                   .GetExecutor(executor_name);
    return ptr ? ExecutorRef(ptr->NativeHandle()) : ExecutorRef();
  }

  void InitCoreProxy(const util::ModuleDetailInfo& info, module::CoreProxy& proxy);

 private:
  Options options_;
  std::atomic_bool stop_flag_ = false;

  std::vector<std::function<void()> >
      hook_func_vec_array_[static_cast<uint32_t>(HookPoint::HookPointNum)];

  executor::MainThreadExecutor main_thread_executor_;

  configurator::ConfiguratorManager configurator_manager_;
  plugin::PluginManager plugin_manager_;
  logger::LoggerManager logger_manager_;
  executor::ExecutorManager executor_manager_;
  rpc::RpcManager rpc_manager_;
  channel::ChannelManager channel_manager_;
  module::ModuleManager module_manager_;
};

}  // namespace aimrt::runtime::core
