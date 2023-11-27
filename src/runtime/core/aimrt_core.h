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

    bool register_signal = true;

    bool auto_set_to_global = true;
  };

  enum class State : uint32_t {
    PreInit,

    PreInitConfigurator,
    PostInitConfigurator,

    PreInitPlugin,
    PostInitPlugin,

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

    PostInit,

    PreStart,
    PostStart,

    PreShutdown,
    PostShutdown,

    MaxStateNum,
  };

  using HookTask = std::function<void()>;
  using SignalHandle = std::function<void(int)>;

 public:
  AimRTCore();
  ~AimRTCore();

  AimRTCore(const AimRTCore&) = delete;
  AimRTCore& operator=(const AimRTCore&) = delete;

  void Initialize(const Options& options);
  void Start();
  void Shutdown();

  State GetState() const { return state_; }

  template <typename... Args>
    requires std::constructible_from<HookTask, Args...>
  void RegisterHookFunc(State state, Args&&... args) {
    hook_task_vec_array_[static_cast<uint32_t>(state)]
        .emplace_back(std::forward<Args>(args)...);
  }

  template <typename... Args>
    requires std::constructible_from<SignalHandle, Args...>
  void RegisterSignalHandle(const std::set<int>& sigs, Args&&... args) {
    signal_handle_vec_.emplace_back(sigs, std::forward<Args>(args)...);
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

  void SetGlobal() {
    assert(global_core_ptr_ == nullptr);
    global_core_ptr_ = this;
  }

  void UnSetGlobal() {
    if (global_core_ptr_ == this) global_core_ptr_ = nullptr;
  }

  static AimRTCore* GetGlobalAimRTCore() { return global_core_ptr_; }

 private:
  void EnterState(State state) {
    state_ = state;
    for (const auto& func : hook_task_vec_array_[static_cast<uint32_t>(state)])
      func();
  }

  aimrt::executor::ExecutorRef GetExecutor(std::string_view executor_name) {
    auto ptr = GetExecutorManager()
                   .GetExecutorManagerProxy(util::ModuleDetailInfo{})
                   .GetExecutor(executor_name);
    return ptr ? aimrt::executor::ExecutorRef(ptr->NativeHandle()) : aimrt::executor::ExecutorRef();
  }

  void InitCoreProxy(const util::ModuleDetailInfo& info, module::CoreProxy& proxy);

  void DumpCfgFile();

  void RegisterSignalToSystem();

  static void SignalHandler(int sig);

 private:
  Options options_;
  std::atomic_bool stop_flag_ = false;

  State state_ = State::PreInit;
  std::vector<HookTask> hook_task_vec_array_[static_cast<uint32_t>(State::MaxStateNum)];

  std::vector<std::pair<std::set<int>, SignalHandle>> signal_handle_vec_;

  executor::MainThreadExecutor main_thread_executor_;

  configurator::ConfiguratorManager configurator_manager_;
  plugin::PluginManager plugin_manager_;
  logger::LoggerManager logger_manager_;
  executor::ExecutorManager executor_manager_;
  rpc::RpcManager rpc_manager_;
  channel::ChannelManager channel_manager_;
  module::ModuleManager module_manager_;

  static AimRTCore* global_core_ptr_;
};

}  // namespace aimrt::runtime::core
